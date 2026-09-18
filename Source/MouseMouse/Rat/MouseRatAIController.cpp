#include "Rat/MouseRatAIController.h"

#include "Engine/World.h"
#include "Items/Food/MouseFoodActor.h"
#include "Kismet/GameplayStatics.h"
#include "MouseMouse.h"
#include "Navigation/PathFollowingComponent.h"
#include "Rat/MouseRatCharacter.h"
#include "Rat/MouseRatNest.h"
#include "TimerManager.h"


AMouseRatAIController::AMouseRatAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}


void AMouseRatAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!HasAuthority())
	{
		return;
	}

	FoodBehaviorState = ERatFoodBehaviorState::SeekingFood;
	CurrentFoodTarget = nullptr;
	CurrentHomeMoveTarget = nullptr;
	NextFoodSearchTime = 0.0f;
	NextFoodMoveRetryTime = 0.0f;
	NextHomeNestSearchTime = 0.0f;
	NextHomeMoveRetryTime = 0.0f;
	bHasLoggedMissingHome = false;
	bHasLoggedFoodMoveFailure = false;
	bHasLoggedHomeMoveFailure = false;

	GetWorldTimerManager().SetTimer(
		FoodBehaviorTimerHandle,
		this,
		&AMouseRatAIController::UpdateFoodBehavior,
		BehaviorUpdateInterval,
		true
	);
}


void AMouseRatAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(
		FoodBehaviorTimerHandle
	);

	CurrentFoodTarget = nullptr;
	CurrentHomeMoveTarget = nullptr;
	FoodBehaviorState = ERatFoodBehaviorState::SeekingFood;
	NextFoodSearchTime = 0.0f;
	NextFoodMoveRetryTime = 0.0f;
	NextHomeNestSearchTime = 0.0f;
	NextHomeMoveRetryTime = 0.0f;
	bHasLoggedMissingHome = false;
	bHasLoggedFoodMoveFailure = false;
	bHasLoggedHomeMoveFailure = false;

	Super::OnUnPossess();
}


float AMouseRatAIController::GetWorldTimeSeconds() const
{
	const UWorld* World = GetWorld();

	return World
		? World->GetTimeSeconds()
		: 0.0f;
}


void AMouseRatAIController::SetFoodBehaviorState(
	ERatFoodBehaviorState NewState
)
{
	FoodBehaviorState = NewState;
}


AMouseFoodActor*
AMouseRatAIController::FindClosestAvailableFood() const
{
	const APawn* ControlledPawn = GetPawn();

	if (!ControlledPawn)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> FoodActors;

	// TODO: Replace this global scan with a spatial registry when rat/food counts grow.
	UGameplayStatics::GetAllActorsOfClass(
		World,
		AMouseFoodActor::StaticClass(),
		FoodActors
	);

	AMouseFoodActor* ClosestFood = nullptr;

	float ClosestDistanceSquared =
		TNumericLimits<float>::Max();

	for (AActor* Actor : FoodActors)
	{
		AMouseFoodActor* Food =
			Cast<AMouseFoodActor>(Actor);

		if (!Food ||
			!Food->IsAvailableForRat())
		{
			continue;
		}

		const float DistanceSquared =
			FVector::DistSquared(
				ControlledPawn->GetActorLocation(),
				Food->GetActorLocation()
			);

		if (DistanceSquared <
			ClosestDistanceSquared)
		{
			ClosestDistanceSquared =
				DistanceSquared;

			ClosestFood = Food;
		}
	}

	return ClosestFood;
}


AMouseRatNest*
AMouseRatAIController::FindClosestRatNest() const
{
	const APawn* ControlledPawn = GetPawn();

	if (!ControlledPawn)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> NestActors;

	// TODO: Replace this global scan with a spatial registry when rat/nest counts grow.
	UGameplayStatics::GetAllActorsOfClass(
		World,
		AMouseRatNest::StaticClass(),
		NestActors
	);

	AMouseRatNest* ClosestNest = nullptr;

	float ClosestDistanceSquared =
		TNumericLimits<float>::Max();

	for (AActor* Actor : NestActors)
	{
		AMouseRatNest* Nest =
			Cast<AMouseRatNest>(Actor);

		if (!IsValid(Nest))
		{
			continue;
		}

		const float DistanceSquared =
			FVector::DistSquared(
				ControlledPawn->GetActorLocation(),
				Nest->GetDepositLocation()
			);

		if (DistanceSquared <
			ClosestDistanceSquared)
		{
			ClosestDistanceSquared =
				DistanceSquared;

			ClosestNest = Nest;
		}
	}

	return ClosestNest;
}


AMouseRatNest* AMouseRatAIController::ResolveHomeNest(
	AMouseRatCharacter* Rat
)
{
	if (!IsValid(Rat))
	{
		return nullptr;
	}

	AMouseRatNest* ExistingHomeNest =
		Rat->GetHomeNest();

	if (IsValid(ExistingHomeNest))
	{
		bHasLoggedMissingHome = false;

		return ExistingHomeNest;
	}

	if (ExistingHomeNest)
	{
		Rat->SetHomeNest(nullptr);
	}

	const float CurrentTime = GetWorldTimeSeconds();

	if (CurrentTime < NextHomeNestSearchTime)
	{
		return nullptr;
	}

	AMouseRatNest* ClosestNest = FindClosestRatNest();

	if (!IsValid(ClosestNest))
	{
		NextHomeNestSearchTime =
			CurrentTime + HomeNestSearchRetryInterval;

		if (!bHasLoggedMissingHome)
		{
			UE_LOG(
				LogMouseMouse,
				Warning,
				TEXT("Rat '%s' could not find a home nest; it will keep its food and retry."),
				*GetNameSafe(Rat)
			);

			bHasLoggedMissingHome = true;
		}

		return nullptr;
	}

	Rat->SetHomeNest(ClosestNest);

	NextHomeNestSearchTime = 0.0f;
	NextHomeMoveRetryTime = 0.0f;
	bHasLoggedMissingHome = false;

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Rat '%s' assigned nearest home nest '%s'."),
		*GetNameSafe(Rat),
		*GetNameSafe(ClosestNest)
	);

	return ClosestNest;
}


bool AMouseRatAIController::RequestMoveToFood(
	AMouseFoodActor* Food
)
{
	if (!IsValid(Food))
	{
		return false;
	}

	const EPathFollowingRequestResult::Type MoveResult =
		MoveToActor(
			Food,
			FoodMoveAcceptanceRadius,
			true,
			true,
			true
		);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		if (!bHasLoggedFoodMoveFailure)
		{
			UE_LOG(
				LogMouseMouse,
				Warning,
				TEXT("Rat '%s' failed to move to food '%s'."),
				*GetNameSafe(GetPawn()),
				*GetNameSafe(Food)
			);

			bHasLoggedFoodMoveFailure = true;
		}

		return false;
	}

	bHasLoggedFoodMoveFailure = false;

	return true;
}


bool AMouseRatAIController::RequestMoveToHome(
	AMouseRatNest* HomeNest
)
{
	if (!IsValid(HomeNest))
	{
		return false;
	}

	const EPathFollowingRequestResult::Type MoveResult =
		MoveToLocation(
			HomeNest->GetDepositLocation(),
			HomeMoveAcceptanceRadius,
			false,
			true,
			true,
			true,
			nullptr,
			true
		);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		if (!bHasLoggedHomeMoveFailure)
		{
			UE_LOG(
				LogMouseMouse,
				Warning,
				TEXT("Rat '%s' failed to move to home nest '%s'."),
				*GetNameSafe(GetPawn()),
				*GetNameSafe(HomeNest)
			);

			bHasLoggedHomeMoveFailure = true;
		}

		return false;
	}

	bHasLoggedHomeMoveFailure = false;

	return true;
}


bool AMouseRatAIController::SetFoodTarget(
	AMouseFoodActor* NewTarget
)
{
	CurrentFoodTarget = NewTarget;

	if (!IsValid(CurrentFoodTarget) ||
		!CurrentFoodTarget->IsAvailableForRat())
	{
		CurrentFoodTarget = nullptr;
		StopMovement();

		return false;
	}

	if (!RequestMoveToFood(CurrentFoodTarget))
	{
		CurrentFoodTarget = nullptr;
		StopMovement();

		return false;
	}

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Rat '%s' selected food '%s'."),
		*GetNameSafe(GetPawn()),
		*GetNameSafe(CurrentFoodTarget)
	);

	NextFoodMoveRetryTime =
		GetWorldTimeSeconds() + MoveRetryInterval;

	return true;
}


void AMouseRatAIController::StartReturningHome()
{
	CurrentFoodTarget = nullptr;
	CurrentHomeMoveTarget = nullptr;
	NextHomeMoveRetryTime = 0.0f;

	StopMovement();

	SetFoodBehaviorState(
		ERatFoodBehaviorState::ReturningHome
	);

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Rat '%s' is returning home with food."),
		*GetNameSafe(GetPawn())
	);
}


void AMouseRatAIController::UpdateSeekingFood(
	AMouseRatCharacter* Rat
)
{
	if (!IsValid(Rat))
	{
		return;
	}

	const float CurrentTime = GetWorldTimeSeconds();

	if (CurrentTime < NextFoodSearchTime)
	{
		return;
	}

	AMouseFoodActor* NewTarget =
		FindClosestAvailableFood();

	if (!IsValid(NewTarget))
	{
		NextFoodSearchTime =
			CurrentTime + FoodSearchRetryInterval;

		StopMovement();

		return;
	}

	if (SetFoodTarget(NewTarget))
	{
		SetFoodBehaviorState(
			ERatFoodBehaviorState::MovingToFood
		);

		return;
	}

	NextFoodSearchTime =
		CurrentTime + FoodSearchRetryInterval;
}


void AMouseRatAIController::UpdateMovingToFood(
	AMouseRatCharacter* Rat
)
{
	if (!IsValid(Rat))
	{
		return;
	}

	if (!IsValid(CurrentFoodTarget) ||
		!CurrentFoodTarget->IsAvailableForRat())
	{
		CurrentFoodTarget = nullptr;
		StopMovement();
		NextFoodSearchTime = GetWorldTimeSeconds();

		SetFoodBehaviorState(
			ERatFoodBehaviorState::SeekingFood
		);

		return;
	}

	const float DistanceSquared =
		FVector::DistSquared(
			Rat->GetActorLocation(),
			CurrentFoodTarget->GetActorLocation()
		);

	if (DistanceSquared <= FMath::Square(PickupDistance))
	{
		AMouseFoodActor* FoodToPickup =
			CurrentFoodTarget;

		CurrentFoodTarget = nullptr;

		StopMovement();

		if (Rat->TryPickupFood(FoodToPickup))
		{
			StartReturningHome();
		}
		else
		{
			NextFoodSearchTime = GetWorldTimeSeconds();

			SetFoodBehaviorState(
				ERatFoodBehaviorState::SeekingFood
			);
		}

		return;
	}

	const float CurrentTime = GetWorldTimeSeconds();

	// A completed/failed asynchronous request can leave the controller idle
	// while still outside pickup range. Retry at a low frequency instead of locking.
	if (GetMoveStatus() == EPathFollowingStatus::Idle &&
		CurrentTime >= NextFoodMoveRetryTime)
	{
		if (RequestMoveToFood(CurrentFoodTarget))
		{
			NextFoodMoveRetryTime =
				CurrentTime + MoveRetryInterval;
		}
		else
		{
			CurrentFoodTarget = nullptr;
			StopMovement();
			NextFoodSearchTime =
				CurrentTime + FoodSearchRetryInterval;

			SetFoodBehaviorState(
				ERatFoodBehaviorState::SeekingFood
			);
		}
	}
}


void AMouseRatAIController::UpdateReturningHome(
	AMouseRatCharacter* Rat
)
{
	if (!IsValid(Rat))
	{
		return;
	}

	const float CurrentTime = GetWorldTimeSeconds();

	if (!IsValid(Rat->GetCarriedFood()))
	{
		CurrentHomeMoveTarget = nullptr;
		StopMovement();
		NextFoodSearchTime = CurrentTime;

		SetFoodBehaviorState(
			ERatFoodBehaviorState::SeekingFood
		);

		return;
	}

	AMouseRatNest* HomeNest = ResolveHomeNest(Rat);

	if (!IsValid(HomeNest))
	{
		StopMovement();

		return;
	}

	const float DistanceSquared =
		FVector::DistSquared(
			Rat->GetActorLocation(),
			HomeNest->GetDepositLocation()
		);

	if (DistanceSquared <=
		FMath::Square(AMouseRatNest::DepositDistance))
	{
		StopMovement();

		if (Rat->TryDepositCarriedFood())
		{
			CurrentHomeMoveTarget = nullptr;
			NextFoodSearchTime = CurrentTime;

			SetFoodBehaviorState(
				ERatFoodBehaviorState::SeekingFood
			);
		}

		return;
	}

	if (CurrentHomeMoveTarget.Get() != HomeNest)
	{
		if (CurrentTime < NextHomeMoveRetryTime)
		{
			return;
		}

		CurrentHomeMoveTarget = HomeNest;

		if (RequestMoveToHome(HomeNest))
		{
			NextHomeMoveRetryTime =
				CurrentTime + MoveRetryInterval;
		}
		else
		{
			CurrentHomeMoveTarget = nullptr;
			NextHomeMoveRetryTime =
				CurrentTime + MoveRetryInterval;
		}

		return;
	}

	// As with food, recover from an asynchronous path failure without losing food.
	if (GetMoveStatus() == EPathFollowingStatus::Idle &&
		CurrentTime >= NextHomeMoveRetryTime)
	{
		if (RequestMoveToHome(HomeNest))
		{
			NextHomeMoveRetryTime =
				CurrentTime + MoveRetryInterval;
		}
		else
		{
			CurrentHomeMoveTarget = nullptr;
			NextHomeMoveRetryTime =
				CurrentTime + MoveRetryInterval;
		}
	}
}


void AMouseRatAIController::UpdateFoodBehavior()
{
	if (!HasAuthority())
	{
		return;
	}

	AMouseRatCharacter* Rat =
		Cast<AMouseRatCharacter>(GetPawn());

	if (!Rat)
	{
		return;
	}

	if (IsValid(Rat->GetCarriedFood()))
	{
		if (FoodBehaviorState !=
			ERatFoodBehaviorState::ReturningHome)
		{
			StartReturningHome();
		}

		UpdateReturningHome(Rat);

		return;
	}

	if (FoodBehaviorState ==
		ERatFoodBehaviorState::ReturningHome)
	{
		CurrentHomeMoveTarget = nullptr;
		StopMovement();
		NextFoodSearchTime = GetWorldTimeSeconds();

		SetFoodBehaviorState(
			ERatFoodBehaviorState::SeekingFood
		);
	}

	switch (FoodBehaviorState)
	{
	case ERatFoodBehaviorState::SeekingFood:
		UpdateSeekingFood(Rat);
		break;

	case ERatFoodBehaviorState::MovingToFood:
		UpdateMovingToFood(Rat);
		break;

	case ERatFoodBehaviorState::ReturningHome:
		// Handled above when a carried food is present.
		break;

	default:
		SetFoodBehaviorState(
			ERatFoodBehaviorState::SeekingFood
		);
		break;
	}
}
