// Fill out your copyright notice in the Description page of Project Settings.


#include "Rat/MouseRatAIController.h"

#include "Items/Food/MouseFoodActor.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"


AMouseRatAIController::AMouseRatAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}


void AMouseRatAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// AI decisions should only be made by the server.
	if (!HasAuthority())
	{
		return;
	}

	// Let the rat make its first decision immediately.
	SearchForFood();

	// Then re-evaluate periodically.
	GetWorldTimerManager().SetTimer(
		FoodSearchTimerHandle,
		this,
		&AMouseRatAIController::SearchForFood,
		1.0f,
		true
	);
}

AMouseFoodActor* AMouseRatAIController::FindClosestAvailableFood() const
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

		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestFood = Food;
		}
	}

	return ClosestFood;
}

void AMouseRatAIController::SearchForFood()
{
	if (!HasAuthority())
	{
		return;
	}

	AMouseFoodActor* Food =
		FindClosestAvailableFood();

	if (!Food)
	{
		StopMovement();
		return;
	}

	MoveToActor(
		Food,
		80.0f,
		true,
		true,
		true
	);
}
