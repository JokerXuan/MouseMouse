#include "Rat/MouseRatAIController.h"

#include "Items/Food/MouseFoodActor.h"
#include "Kismet/GameplayStatics.h"
#include "Rat/MouseRatCharacter.h"
#include "TimerManager.h"
#include "Navigation/PathFollowingComponent.h"

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

	GetWorldTimerManager().SetTimer(
		FoodBehaviorTimerHandle,
		this,
		&AMouseRatAIController::UpdateFoodBehavior,
		0.25f,
		true
	);
}


void AMouseRatAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(
		FoodBehaviorTimerHandle
	);

	CurrentFoodTarget = nullptr;

	Super::OnUnPossess();
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

		if (!Food)
		{
			continue;
		}

		if (!Food->IsAvailableForRat())
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


void AMouseRatAIController::SetFoodTarget(
	AMouseFoodActor* NewTarget
)
{
	CurrentFoodTarget = NewTarget;

	if (!IsValid(CurrentFoodTarget))
	{
		StopMovement();
		return;
	}

	const EPathFollowingRequestResult::Type MoveResult =
		MoveToActor(
			CurrentFoodTarget,
			80.0f,
			true,
			true,
			true
		);

	if (MoveResult ==
		EPathFollowingRequestResult::Failed)
	{
		CurrentFoodTarget = nullptr;
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

	// This vertical slice only supports one carried food.
	if (IsValid(Rat->GetCarriedFood()))
	{
		CurrentFoodTarget = nullptr;

		StopMovement();

		return;
	}

	// Acquire a new target only when the current
	// decision is no longer usable.
	if (!IsValid(CurrentFoodTarget) ||
		!CurrentFoodTarget->IsAvailableForRat())
	{
		SetFoodTarget(
			FindClosestAvailableFood()
		);

		if (!IsValid(CurrentFoodTarget))
		{
			return;
		}
	}

	const float DistanceSquared =
		FVector::DistSquared(
			Rat->GetActorLocation(),
			CurrentFoodTarget->GetActorLocation()
		);

	constexpr float PickupDistance = 100.0f;

	if (DistanceSquared <=
		FMath::Square(PickupDistance))
	{
		StopMovement();

		AMouseFoodActor* FoodToPickup =
			CurrentFoodTarget;

		Rat->TryPickupFood(FoodToPickup);

		CurrentFoodTarget = nullptr;

		return;
	}
}
