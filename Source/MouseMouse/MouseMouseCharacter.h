// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MouseMouseCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UMouseInteractionComponent;
struct FInputActionValue;
class USceneComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class AMouseMouseCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Point where a held item is attached */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* HoldPoint;

	/** Actor currently being held by this character */
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> HeldActor;

	/** Handles player interaction detection and execution */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UMouseInteractionComponent* InteractionComponent;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Interact Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* InteractAction;

	/** Drop Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* DropAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;
	
public:
	AMouseMouseCharacter();

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Called when the player presses the interact input */
	void InteractInput();

	/** Called when the player presses the drop input */
	void DropInput();

	/** Sends an interaction request to the server */
	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* TargetActor);

	/** Validates and executes interaction on the server */
	void TryExecuteInteraction(AActor* TargetActor);

	/** Sends a drop request to the server */
	UFUNCTION(Server, Reliable)
	void ServerDropHeldActor();

	/** Drops the currently held actor on the server */
	bool TryDropHeldActor();

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/** Returns interaction component **/
	UMouseInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

	/** Returns the point where held items are attached **/
	USceneComponent* GetHoldPoint() const { return HoldPoint; }

	/** Returns the actor currently held by this character **/
	AActor* GetHeldActor() const { return HeldActor; }

	/**
	 * Attempts to pick up an actor.
	 * This Gameplay operation must be executed by the server.
	 **/
	bool TryPickupActor(AActor* ActorToPickup);

	/** Registers replicated properties for this character **/
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;
};

