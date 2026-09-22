#include "Rat/MouseRatCharacter.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/Food/MouseFoodActor.h"
#include "Items/RatCard/MouseRatCardActor.h"
#include "MouseMouse.h"
#include "Net/UnrealNetwork.h"
#include "Rat/MouseRatAIController.h"
#include "Rat/MouseRatNest.h"


namespace
{
float GetSquaredDistanceFromPointToBounds(
	const FVector& Point,
	const FVector& BoundsMin,
	const FVector& BoundsMax
)
{
	const FVector ClosestPoint(
		FMath::Clamp(Point.X, BoundsMin.X, BoundsMax.X),
		FMath::Clamp(Point.Y, BoundsMin.Y, BoundsMax.Y),
		FMath::Clamp(Point.Z, BoundsMin.Z, BoundsMax.Z)
	);

	return FVector::DistSquared(Point, ClosestPoint);
}


float GetSquaredDistanceFromSegmentToBounds(
	const FVector& SegmentStart,
	const FVector& SegmentEnd,
	const FVector& BoundsMin,
	const FVector& BoundsMax
)
{
	const FVector Direction = SegmentEnd - SegmentStart;

	// The closest point on an axis-aligned box changes only where the segment
	// crosses one of its planes. Within each resulting interval, the squared
	// distance is a simple quadratic with at most one interior minimum.
	TArray<float, TInlineAllocator<8>> IntervalBounds;
	IntervalBounds.Reserve(8);
	IntervalBounds.Add(0.0f);
	IntervalBounds.Add(1.0f);

	for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
	{
		const float DirectionComponent = Direction[AxisIndex];

		if (FMath::IsNearlyZero(DirectionComponent))
		{
			continue;
		}

		const float MinPlaneTime =
			(BoundsMin[AxisIndex] - SegmentStart[AxisIndex]) /
			DirectionComponent;

		const float MaxPlaneTime =
			(BoundsMax[AxisIndex] - SegmentStart[AxisIndex]) /
			DirectionComponent;

		if (MinPlaneTime > 0.0f &&
			MinPlaneTime < 1.0f)
		{
			IntervalBounds.Add(MinPlaneTime);
		}

		if (MaxPlaneTime > 0.0f &&
			MaxPlaneTime < 1.0f)
		{
			IntervalBounds.Add(MaxPlaneTime);
		}
	}

	IntervalBounds.Sort();

	float SquaredDistance = TNumericLimits<float>::Max();

	for (const float Time : IntervalBounds)
	{
		SquaredDistance = FMath::Min(
			SquaredDistance,
			GetSquaredDistanceFromPointToBounds(
				SegmentStart + Direction * Time,
				BoundsMin,
				BoundsMax
			)
		);
	}

	for (int32 IntervalIndex = 0;
		IntervalIndex < IntervalBounds.Num() - 1;
		++IntervalIndex)
	{
		const float IntervalStart = IntervalBounds[IntervalIndex];
		const float IntervalEnd = IntervalBounds[IntervalIndex + 1];

		if (FMath::IsNearlyEqual(IntervalStart, IntervalEnd))
		{
			continue;
		}

		const FVector MidPoint = SegmentStart +
			Direction * ((IntervalStart + IntervalEnd) * 0.5f);

		float QuadraticLinearTerm = 0.0f;
		float QuadraticSquaredTerm = 0.0f;

		for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
		{
			float NearestBound = 0.0f;

			if (MidPoint[AxisIndex] < BoundsMin[AxisIndex])
			{
				NearestBound = BoundsMin[AxisIndex];
			}
			else if (MidPoint[AxisIndex] > BoundsMax[AxisIndex])
			{
				NearestBound = BoundsMax[AxisIndex];
			}
			else
			{
				continue;
			}

			const float DirectionComponent = Direction[AxisIndex];
			const float Offset =
				SegmentStart[AxisIndex] - NearestBound;

			QuadraticLinearTerm += DirectionComponent * Offset;
			QuadraticSquaredTerm +=
				DirectionComponent * DirectionComponent;
		}

		if (QuadraticSquaredTerm <= SMALL_NUMBER)
		{
			continue;
		}

		const float NearestTime = FMath::Clamp(
			-QuadraticLinearTerm / QuadraticSquaredTerm,
			IntervalStart,
			IntervalEnd
		);

		SquaredDistance = FMath::Min(
			SquaredDistance,
			GetSquaredDistanceFromPointToBounds(
				SegmentStart + Direction * NearestTime,
				BoundsMin,
				BoundsMax
			)
		);
	}

	return SquaredDistance;
}
}


AMouseRatCharacter::AMouseRatCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bUseControllerRotationYaw = false;

	CarryPoint = CreateDefaultSubobject<USceneComponent>(
		TEXT("Carry Point")
	);

	// The visual mesh remains aligned to the rat body when its capsule is
	// resized in Blueprint, unlike the capsule-center root component.
	CarryPoint->SetupAttachment(GetMesh());

	CarryPoint->SetRelativeLocation(
		FVector(15.0f, 0.0f, 0.0f)
	);

	AIControllerClass =
		AMouseRatAIController::StaticClass();

	AutoPossessAI =
		EAutoPossessAI::PlacedInWorldOrSpawned;

	UCharacterMovementComponent* MovementComponent =
		GetCharacterMovement();

	MovementComponent->bOrientRotationToMovement = true;
	MovementComponent->bUseControllerDesiredRotation = false;
	MovementComponent->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	MovementComponent->MaxWalkSpeed = 250.0f;
}


void AMouseRatCharacter::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(
		AMouseRatCharacter,
		CarriedFood
	);

	DOREPLIFETIME(
		AMouseRatCharacter,
		HomeNest
	);
}


void AMouseRatCharacter::SetHomeNest(
	AMouseRatNest* NewHomeNest
)
{
	if (!HasAuthority())
	{
		return;
	}

	if (HomeNest == NewHomeNest)
	{
		return;
	}

	HomeNest = NewHomeNest;

	ForceNetUpdate();
}


bool AMouseRatCharacter::IsWithinReachOfPoint(
	const FVector& Point,
	float ExtraReach
) const
{
	return IsWithinReachOfBounds(
		Point,
		FVector::ZeroVector,
		ExtraReach
	);
}


bool AMouseRatCharacter::IsFoodWithinPickupRange(
	const AMouseFoodActor* Food
) const
{
	if (!IsValid(Food))
	{
		return false;
	}

	FVector FoodBoundsOrigin;
	FVector FoodBoundsExtent;

	Food->GetActorBounds(
		true,
		FoodBoundsOrigin,
		FoodBoundsExtent,
		false
	);

	// Food meshes may be configured without collision. Their visual bounds are
	// still the best available representation of the food's physical extent.
	if (FoodBoundsExtent.IsNearlyZero())
	{
		Food->GetActorBounds(
			false,
			FoodBoundsOrigin,
			FoodBoundsExtent,
			false
		);
	}

	return IsWithinReachOfBounds(
		FoodBoundsOrigin,
		FoodBoundsExtent,
		PickupReach
	);
}


bool AMouseRatCharacter::IsWithinReachOfBounds(
	const FVector& BoundsOrigin,
	const FVector& BoundsExtent,
	float ExtraReach
) const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();

	const FVector CapsuleCenter = Capsule
		? Capsule->GetComponentLocation()
		: GetActorLocation();

	const FVector CapsuleAxis = Capsule
		? Capsule->GetUpVector()
		: FVector::UpVector;

	const float CapsuleRadius = Capsule
		? Capsule->GetScaledCapsuleRadius()
		: 0.0f;

	const float CapsuleHalfHeight = Capsule
		? Capsule->GetScaledCapsuleHalfHeight()
		: 0.0f;

	const float CapsuleSegmentHalfLength = FMath::Max(
		0.0f,
		CapsuleHalfHeight - CapsuleRadius
	);

	const FVector SafeBoundsExtent(
		FMath::Max(0.0f, BoundsExtent.X),
		FMath::Max(0.0f, BoundsExtent.Y),
		FMath::Max(0.0f, BoundsExtent.Z)
	);

	const FVector BoundsMin = BoundsOrigin - SafeBoundsExtent;
	const FVector BoundsMax = BoundsOrigin + SafeBoundsExtent;
	const float TotalReach =
		CapsuleRadius + FMath::Max(0.0f, ExtraReach);

	return GetSquaredDistanceFromSegmentToBounds(
		CapsuleCenter - CapsuleAxis * CapsuleSegmentHalfLength,
		CapsuleCenter + CapsuleAxis * CapsuleSegmentHalfLength,
		BoundsMin,
		BoundsMax
	) <= FMath::Square(TotalReach);
}


bool AMouseRatCharacter::TryPickupFood(
	AMouseFoodActor* Food
)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!IsValid(Food))
	{
		return false;
	}

	if (IsValid(CarriedFood))
	{
		return false;
	}

	if (!CarryPoint)
	{
		return false;
	}

	if (!Food->IsAvailableForRat())
	{
		return false;
	}

	if (!IsFoodWithinPickupRange(Food))
	{
		return false;
	}

	CarriedFood = Food;

	Food->SetOwner(this);
	Food->SetHolder(this);

	ForceNetUpdate();
	Food->ForceNetUpdate();

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Rat '%s' picked up food '%s'."),
		*GetNameSafe(this),
		*GetNameSafe(Food)
	);

	return true;
}


bool AMouseRatCharacter::TryDepositCarriedFood()
{
	if (!HasAuthority())
	{
		return false;
	}

	AMouseFoodActor* FoodToDeposit = CarriedFood;

	if (!IsValid(FoodToDeposit))
	{
		if (CarriedFood)
		{
			CarriedFood = nullptr;
			ForceNetUpdate();
		}

		return false;
	}

	// A stale carry reference must not let this rat deposit somebody else's food.
	if (FoodToDeposit->GetHolder() != this)
	{
		CarriedFood = nullptr;
		ForceNetUpdate();

		UE_LOG(
			LogMouseMouse,
			Warning,
			TEXT("Rat '%s' cleared an invalid carried-food reference '%s'."),
			*GetNameSafe(this),
			*GetNameSafe(FoodToDeposit)
		);

		return false;
	}

	if (!IsValid(HomeNest))
	{
		return false;
	}

	if (!IsWithinReachOfPoint(
		HomeNest->GetDepositLocation(),
		AMouseRatNest::DepositDistance
	))
	{
		return false;
	}

	if (!HomeNest->TryStoreFood(this, FoodToDeposit))
	{
		return false;
	}

	CarriedFood = nullptr;

	ForceNetUpdate();

	return true;
}


bool AMouseRatCharacter::TryCapture()
{
	if (!HasAuthority() ||
		bCaptureInProgress ||
		!RatCardClass)
	{
		return false;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	bCaptureInProgress = true;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AMouseRatCardActor* RatCard =
		World->SpawnActor<AMouseRatCardActor>(
			RatCardClass,
			GetActorTransform(),
			SpawnParameters
		);

	if (!IsValid(RatCard))
	{
		bCaptureInProgress = false;

		return false;
	}

	// A newly spawned pickup must enter the same unheld, dropped state used by
	// the existing food and generic pickup actors.
	RatCard->SetOwner(nullptr);
	RatCard->SetHolder(nullptr);
	RatCard->ForceNetUpdate();

	AMouseFoodActor* FoodToDrop = CarriedFood;

	if (IsValid(FoodToDrop))
	{
		FoodToDrop->SetOwner(nullptr);
		FoodToDrop->SetHolder(nullptr);
		FoodToDrop->ForceNetUpdate();
	}

	CarriedFood = nullptr;
	ForceNetUpdate();

	if (AAIController* AIController =
		Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}

	Destroy();

	return true;
}
