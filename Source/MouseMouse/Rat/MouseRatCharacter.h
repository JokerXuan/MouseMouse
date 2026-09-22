// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MouseRatCharacter.generated.h"

class USceneComponent;
class AMouseFoodActor;
class AMouseRatNest;

UCLASS()
class MOUSEMOUSE_API AMouseRatCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMouseRatCharacter();

	/**
	 * Extra distance allowed between this rat's capsule surface and a food
	 * actor's bounds when picking it up.
	 */
	static constexpr float PickupReach = 100.0f;

	USceneComponent* GetCarryPoint() const
	{
		return CarryPoint;
	}

	/** Returns the food currently carried by this rat */
	AMouseFoodActor* GetCarriedFood() const
	{
		return CarriedFood;
	}

	/** Returns the persistent home nest assigned to this rat. */
	AMouseRatNest* GetHomeNest() const
	{
		return HomeNest;
	}

	/** Radius within which a player first becomes a threat to this rat. */
	float GetThreatDetectionRadius() const
	{
		return ThreatDetectionRadius;
	}

	/** Radius a detected threat must leave before the rat can become safe. */
	float GetLoseThreatRadius() const
	{
		return LoseThreatRadius;
	}

	/** Time the rat must remain safe before it resumes its food behavior. */
	float GetSafeTimeBeforeResume() const
	{
		return SafeTimeBeforeResume;
	}

	/** Desired horizontal distance from the threat for each flee destination. */
	float GetFleeDistance() const
	{
		return FleeDistance;
	}

	/**
	 * Assigns this rat's home nest.
	 * Must be executed by the server when changed during play.
	 */
	void SetHomeNest(AMouseRatNest* NewHomeNest);

	/**
	 * Returns whether the rat's collision capsule is within ExtraReach of a
	 * world-space point. ExtraReach is measured from the capsule surface.
	 */
	bool IsWithinReachOfPoint(
		const FVector& Point,
		float ExtraReach
	) const;

	/**
	 * Returns whether the rat's collision capsule is within PickupReach of
	 * the food actor's collision bounds.
	 */
	bool IsFoodWithinPickupRange(
		const AMouseFoodActor* Food
	) const;

	/**
	 * Attempts to pick up a food actor.
	 * Must be executed by the server.
	 */
	bool TryPickupFood(AMouseFoodActor* Food);

	/**
	 * Attempts to deposit the currently carried food in this rat's home nest.
	 * Must be executed by the server.
	 */
	bool TryDepositCarriedFood();

	/** Registers replicated properties */
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

protected:

	/** Point where carried items are attached */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components"
	)
	TObjectPtr<USceneComponent> CarryPoint;

	/**
	 * Persistent level-instance relationship for this rat's home.
	 * If empty, the server AI may assign the closest available nest.
	 */
	UPROPERTY(
		EditInstanceOnly,
		Replicated,
		BlueprintReadOnly,
		Category = "Rat|Home"
	)
	TObjectPtr<AMouseRatNest> HomeNest;

	/** Food currently carried by this rat */
	UPROPERTY(
		Replicated,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Rat|Carry"
	)
	TObjectPtr<AMouseFoodActor> CarriedFood;

	/** Radius within which the server first detects a player as a threat. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Rat|Threat",
		meta = (ClampMin = "0.0", UIMin = "0.0")
	)
	float ThreatDetectionRadius = 800.0f;

	/**
	 * Radius a detected player must leave before the rat begins its safe timer.
	 * Configure this greater than ThreatDetectionRadius to avoid boundary churn.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Rat|Threat",
		meta = (ClampMin = "0.0", UIMin = "0.0")
	)
	float LoseThreatRadius = 1100.0f;

	/** Consecutive safe time required before leaving the Fleeing state. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Rat|Threat",
		meta = (ClampMin = "0.0", UIMin = "0.0")
	)
	float SafeTimeBeforeResume = 2.0f;

	/** Distance used when generating navigable flee targets away from a threat. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Rat|Threat",
		meta = (ClampMin = "0.0", UIMin = "0.0")
	)
	float FleeDistance = 800.0f;

private:

	/**
	 * Returns whether the rat capsule is within ExtraReach of the supplied
	 * axis-aligned world bounds.
	 */
	bool IsWithinReachOfBounds(
		const FVector& BoundsOrigin,
		const FVector& BoundsExtent,
		float ExtraReach
	) const;
};
