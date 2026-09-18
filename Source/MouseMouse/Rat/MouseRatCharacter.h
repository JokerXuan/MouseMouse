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

	/**
	 * Assigns this rat's home nest.
	 * Must be executed by the server when changed during play.
	 */
	void SetHomeNest(AMouseRatNest* NewHomeNest);

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
};
