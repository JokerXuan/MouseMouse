// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MouseRatCharacter.generated.h"

class USceneComponent;
class AMouseFoodActor;

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

	/**
	 * Attempts to pick up a food actor.
	 * Must be executed by the server.
	 */
	bool TryPickupFood(AMouseFoodActor* Food);

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

	/** Food currently carried by this rat */
	UPROPERTY(
		Replicated,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Rat|Carry"
	)
	TObjectPtr<AMouseFoodActor> CarriedFood;
};
