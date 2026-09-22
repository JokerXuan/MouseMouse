// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MouseRatAIController.generated.h"

class AMouseFoodActor;
class AMouseMouseCharacter;
class AMouseRatCharacter;
class AMouseRatNest;

/** Server-only decision state for the rat's food and threat behavior. */
enum class ERatFoodBehaviorState : uint8
{
	SeekingFood,
	MovingToFood,
	ReturningHome,
	Fleeing
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

	/** Updates server-only threat tracking and returns whether fleeing has priority. */
	bool UpdateThreatDetection(AMouseRatCharacter* Rat);

	/** Finds the nearest player-controlled MouseMouse character within SearchRadius. */
	AMouseMouseCharacter* FindClosestValidPlayerCharacter(
		float SearchRadius
	) const;

	/** Returns a loss radius that remains larger than the detection radius. */
	float GetEffectiveLoseThreatRadius(
		const AMouseRatCharacter* Rat
	) const;

	/** Stops the current task and switches the rat into server-only fleeing. */
	void EnterFleeing();

	/** Continues movement away from the current threat while fleeing. */
	void UpdateFleeing(AMouseRatCharacter* Rat);

	/** Re-evaluates normal behavior from current carried-food state after fleeing. */
	void ResumeFromFleeing(AMouseRatCharacter* Rat);

	/** Selects a reachable NavMesh flee point and begins moving to it. */
	bool SelectAndMoveToFleePoint(AMouseRatCharacter* Rat);

	/** Finds a simple reachable flee point behind or diagonally behind the rat. */
	bool FindFleePoint(
		AMouseRatCharacter* Rat,
		FVector& OutFleePoint
	) const;

	/** Returns whether the active flee destination still leads away from the threat. */
	bool DoesCurrentFleePointLeadAwayFromThreat(
		const AMouseRatCharacter* Rat
	) const;

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

	/** Issues a movement request toward a previously projected flee point. */
	bool RequestMoveToFleePoint(const FVector& FleePoint);

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

	/** Current server-only player threat. This is deliberately not replicated. */
	UPROPERTY(Transient)
	TObjectPtr<AMouseMouseCharacter> CurrentThreat;

	/** Server-only behavior state; clients only observe replicated gameplay results. */
	ERatFoodBehaviorState FoodBehaviorState =
		ERatFoodBehaviorState::SeekingFood;

	/** Server-only NavMesh location currently used for fleeing. */
	FVector CurrentFleePoint = FVector::ZeroVector;

	/** Whether CurrentFleePoint contains a valid selected destination. */
	bool bHasCurrentFleePoint = false;

	FTimerHandle FoodBehaviorTimerHandle;

	/** Prevents global food scans while no food is currently available. */
	float NextFoodSearchTime = 0.0f;

	/** Prevents repeated movement requests after an asynchronous path failure. */
	float NextFoodMoveRetryTime = 0.0f;

	/** Prevents global nest scans while the map has no usable nest. */
	float NextHomeNestSearchTime = 0.0f;

	/** Keeps a failed home movement request from retrying every behavior update. */
	float NextHomeMoveRetryTime = 0.0f;

	/** Time at which the current threat first remained outside the loss radius. */
	float ThreatSafeStartTime = -1.0f;

	/** Keeps an unreachable flee target from retrying every behavior update. */
	float NextFleeMoveRetryTime = 0.0f;

	/** Throttles immediate redirects when a player crosses in front of the rat. */
	float NextFleeThreatRedirectTime = 0.0f;

	/** Suppresses repeated warnings while a map is missing a nest. */
	bool bHasLoggedMissingHome = false;

	/** Suppresses repeated warnings for the same immediately-failed food move. */
	bool bHasLoggedFoodMoveFailure = false;

	/** Suppresses repeated warnings for the same immediately-failed home move. */
	bool bHasLoggedHomeMoveFailure = false;

	/** Suppresses repeated warnings while all current flee destinations fail. */
	bool bHasLoggedFleeMoveFailure = false;

	static constexpr float BehaviorUpdateInterval = 0.25f;
	static constexpr float FoodSearchRetryInterval = 1.0f;
	static constexpr float HomeNestSearchRetryInterval = 2.0f;
	static constexpr float MoveRetryInterval = 1.0f;
	static constexpr float FleePointAcceptanceRadius = 75.0f;
	static constexpr float FleePointReachedDistance = 100.0f;
	static constexpr float FleeProjectionExtentXY = 300.0f;
	static constexpr float FleeProjectionExtentZ = 500.0f;
	static constexpr float FleeThreatRedirectInterval = 0.25f;
	static constexpr float FleePointAwayDotThreshold = 0.1f;
	static constexpr float MinimumLoseThreatRadiusDifference = 1.0f;
};
