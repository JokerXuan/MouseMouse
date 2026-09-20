// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MouseRatNest.generated.h"

class AMouseFoodActor;
class AMouseRatCharacter;
class USceneComponent;

/**
 * A rat's home and the authoritative destination for stolen food.
 */
UCLASS()
class MOUSEMOUSE_API AMouseRatNest : public AActor
{
	GENERATED_BODY()

public:
	AMouseRatNest();

	/**
	 * Extra distance allowed between the rat capsule surface and DepositPoint.
	 */
	static constexpr float DepositDistance = 100.0f;

	/** Gameplay point rats navigate to before depositing food. */
	USceneComponent* GetDepositPoint() const
	{
		return DepositPoint;
	}

	/** Returns the current world-space deposit location. */
	FVector GetDepositLocation() const;

	float GetStoredFoodValue() const
	{
		return StoredFoodValue;
	}

	/**
	 * Stores food carried by the supplied rat and destroys that food actor.
	 * This is an authoritative gameplay operation and must run on the server.
	 */
	bool TryStoreFood(
		AMouseRatCharacter* DepositingRat,
		AMouseFoodActor* Food
	);

	/** Registers replicated properties. */
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

protected:

	/** Root used by Blueprint children to add the nest's visual components. */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components"
	)
	TObjectPtr<USceneComponent> SceneRoot;

	/** Point at which a rat may deposit carried food. */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components"
	)
	TObjectPtr<USceneComponent> DepositPoint;

	/** Total value successfully brought back to this nest. */
	UPROPERTY(
		Replicated,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Rat Nest"
	)
	float StoredFoodValue = 0.0f;
};
