// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MouseRatAIController.generated.h"

class AMouseFoodActor;
class AMouseRatCharacter;

UCLASS()
class MOUSEMOUSE_API AMouseRatAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AMouseRatAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	/**
	 * Periodically updates the rat's current food behavior.
	 *
	 * It does not always choose a new target.
	 * If a valid target already exists, the rat keeps pursuing it.
	 */
	void UpdateFoodBehavior();

	/** Finds the nearest food currently available to rats */
	AMouseFoodActor* FindClosestAvailableFood() const;

	/** Starts pursuing a new food target */
	void SetFoodTarget(AMouseFoodActor* NewTarget);

	/**
	 * Current food this rat intends to acquire.
	 *
	 * This is server-side AI decision state.
	 * It does NOT need replication.
	 */
	UPROPERTY()
	TObjectPtr<AMouseFoodActor> CurrentFoodTarget;

	FTimerHandle FoodBehaviorTimerHandle;
};
