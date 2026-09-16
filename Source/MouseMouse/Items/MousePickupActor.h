// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MouseInteractable.h"
#include "MousePickupActor.generated.h"

class UStaticMeshComponent;
class AMouseMouseCharacter;

UCLASS()
class MOUSEMOUSE_API AMousePickupActor
	: public AActor,
	public IMouseInteractable
{
	GENERATED_BODY()

public:
	AMousePickupActor();

	virtual void Interact_Implementation(
		AActor* Interactor
	) override;

	/**
	 * Server sets who currently holds this item.
	 * nullptr means the item is dropped.
	 */
	void SetHolder(AMouseMouseCharacter* NewHolder);

	AMouseMouseCharacter* GetHolder() const
	{
		return HolderCharacter;
	}

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

protected:

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components"
	)
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Character currently holding this item */
	UPROPERTY(ReplicatedUsing = OnRep_HolderCharacter)
	TObjectPtr<AMouseMouseCharacter> HolderCharacter;

	UFUNCTION()
	void OnRep_HolderCharacter();

	/**
	 * Applies physics, collision and attachment
	 * based on HolderCharacter.
	 */
	void ApplyHolderState();
};
