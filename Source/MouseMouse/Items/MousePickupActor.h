// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MouseInteractable.h"
#include "MousePickupActor.generated.h"

class UStaticMeshComponent;
class ACharacter;

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
	void SetHolder(ACharacter* NewHolder);

	ACharacter* GetHolder() const
	{
		return HolderCharacter;
	}

	/**
	 * Local input extension points for an item held by a player. Subclasses own
	 * any gameplay requests they need to make to the server.
	 */
	virtual void PrimaryUseStarted();
	virtual void PrimaryUseTriggered();
	virtual void PrimaryUseCompleted();

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
	TObjectPtr<ACharacter> HolderCharacter;

	UFUNCTION()
	void OnRep_HolderCharacter();

	/**
	 * Applies physics, collision and attachment
	 * based on HolderCharacter.
	 */
	void ApplyHolderState();

	/**
	 * Server-only extension point for pickup subclasses that need to react to
	 * their holder changing. The base pickup behavior remains unchanged.
	 */
	virtual void OnHolderChanged(
		ACharacter* OldHolder,
		ACharacter* NewHolder
	);
};
