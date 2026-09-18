// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MouseRatAIController.generated.h"

class AMouseFoodActor;
class AMouseRatCharacter;
class AMouseRatNest;

/** Server-only decision state for the rat's food loop. */
enum class ERatFoodBehaviorState : uint8
{
	SeekingFood,
	MovingToFood,
	ReturningHome
};

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
	 */
	void UpdateFoodBehavior();

	/** Chooses a food target only when this rat needs a new one. */
	void UpdateSeekingFood(AMouseRatCharacter* Rat);

	/** Keeps pursuing the selected food and attempts the final pickup. */
	void UpdateMovingToFood(AMouseRatCharacter* Rat);

	/** Returns carried food to the rat's resolved home nest. */
	void UpdateReturningHome(AMouseRatCharacter* Rat);

	/** Finds the nearest food currently available to rats */
	AMouseFoodActor* FindClosestAvailableFood() const;

	/** Finds the nearest available nest for an unassigned rat. */
	AMouseRatNest* FindClosestRatNest() const;

	/** Returns the rat's configured home, or assigns the closest one on the server. */
	AMouseRatNest* ResolveHomeNest(AMouseRatCharacter* Rat);

	/** Starts pursuing a new food target */
	bool SetFoodTarget(AMouseFoodActor* NewTarget);

	/** Issues a movement request toward the current food target. */
	bool RequestMoveToFood(AMouseFoodActor* Food);

	/** Issues a movement request toward a nest's DepositPoint location. */
	bool RequestMoveToHome(AMouseRatNest* HomeNest);

	/** Transitions from carrying food to the return-home behavior. */
	void StartReturningHome();

	/** Changes server-only behavior state without replicating AI internals. */
	void SetFoodBehaviorState(ERatFoodBehaviorState NewState);

	/** Returns the server world's current time for low-frequency retry throttles. */
	float GetWorldTimeSeconds() const;

	/**
	 * Current food this rat intends to acquire.
	 *
	 * This is server-side AI decision state.
	 * It does NOT need replication.
	 */
	UPROPERTY()
	TObjectPtr<AMouseFoodActor> CurrentFoodTarget;

	/** Nest currently receiving a home movement request. Server-only AI state. */
	UPROPERTY()
	TObjectPtr<AMouseRatNest> CurrentHomeMoveTarget;

	/** Server-only behavior state; clients only observe replicated gameplay results. */
	ERatFoodBehaviorState FoodBehaviorState =
		ERatFoodBehaviorState::SeekingFood;

	FTimerHandle FoodBehaviorTimerHandle;

	/** Prevents global food scans while no food is currently available. */
	float NextFoodSearchTime = 0.0f;

	/** Prevents repeated movement requests after an asynchronous path failure. */
	float NextFoodMoveRetryTime = 0.0f;

	/** Prevents global nest scans while the map has no usable nest. */
	float NextHomeNestSearchTime = 0.0f;

	/** Keeps a failed home movement request from retrying every behavior update. */
	float NextHomeMoveRetryTime = 0.0f;

	/** Suppresses repeated warnings while a map is missing a nest. */
	bool bHasLoggedMissingHome = false;

	/** Suppresses repeated warnings for the same immediately-failed food move. */
	bool bHasLoggedFoodMoveFailure = false;

	/** Suppresses repeated warnings for the same immediately-failed home move. */
	bool bHasLoggedHomeMoveFailure = false;

	static constexpr float BehaviorUpdateInterval = 0.25f;
	static constexpr float FoodSearchRetryInterval = 1.0f;
	static constexpr float HomeNestSearchRetryInterval = 2.0f;
	static constexpr float MoveRetryInterval = 1.0f;
	static constexpr float PickupDistance = 100.0f;
	static constexpr float FoodMoveAcceptanceRadius = 80.0f;
	static constexpr float HomeMoveAcceptanceRadius = 80.0f;
};
