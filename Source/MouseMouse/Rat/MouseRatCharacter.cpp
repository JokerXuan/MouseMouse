#include "Rat/MouseRatCharacter.h"

#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/Food/MouseFoodActor.h"
#include "MouseMouse.h"
#include "Net/UnrealNetwork.h"
#include "Rat/MouseRatAIController.h"
#include "Rat/MouseRatNest.h"


AMouseRatCharacter::AMouseRatCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	CarryPoint = CreateDefaultSubobject<USceneComponent>(
		TEXT("Carry Point")
	);

	CarryPoint->SetupAttachment(GetRootComponent());

	CarryPoint->SetRelativeLocation(
		FVector(15.0f, 0.0f, 0.0f)
	);

	AIControllerClass =
		AMouseRatAIController::StaticClass();

	AutoPossessAI =
		EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 250.0f;
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

	const float DistanceSquared =
		FVector::DistSquared(
			GetActorLocation(),
			HomeNest->GetDepositLocation()
		);

	if (DistanceSquared >
		FMath::Square(AMouseRatNest::DepositDistance))
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
