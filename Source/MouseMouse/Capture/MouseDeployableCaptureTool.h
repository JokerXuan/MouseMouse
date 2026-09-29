// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/MousePickupActor.h"
#include "TimerManager.h"
#include "MouseDeployableCaptureTool.generated.h"

class URatCaptureAreaComponent;
class AMouseMouseCharacter;
class UMaterialInterface;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ECaptureToolState : uint8
{
	Inactive,
	Held,
	Arming,
	Armed,
	Triggered
};

/**
 * A reusable pickup that arms its server-authoritative capture area only
 * after it has been placed in the world for its configured delay.
 */
UCLASS()
class MOUSEMOUSE_API AMouseDeployableCaptureTool : public AMousePickupActor
{
	GENERATED_BODY()

public:
	AMouseDeployableCaptureTool();

	UFUNCTION(BlueprintPure, Category = "Capture")
	ECaptureToolState GetToolState() const
	{
		return ToolState;
	}

	UFUNCTION(BlueprintPure, Category = "Capture")
	bool IsArmed() const
	{
		return ToolState == ECaptureToolState::Armed;
	}

	virtual void PrimaryUseStarted() override;
	virtual void PrimaryUseTriggered() override;
	virtual void PrimaryUseCompleted() override;

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason
	) override;

	virtual void OnHolderChanged(
		ACharacter* OldHolder,
		ACharacter* NewHolder
	) override;
	virtual bool ShouldSimulatePhysicsWhenUnheld() const override;

	/** The server-owned overlap area that asks rats to capture themselves. */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components"
	)
	TObjectPtr<URatCaptureAreaComponent> CaptureArea;

	/** Seconds a newly placed tool waits before it can capture a rat. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Capture",
		meta = (ClampMin = "0.0", UIMin = "0.0")
	)
	float ArmDelay = 1.0f;

	/** Seconds the player must hold primary use before placement preview begins. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Capture|Deploy",
		meta = (ClampMin = "0.0", UIMin = "0.0")
	)
	float HoldToDeployTime = 0.4f;

	/** Maximum camera-to-surface trace distance for a deployment. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Capture|Deploy",
		meta = (ClampMin = "0.0", UIMin = "0.0")
	)
	float MaxDeployDistance = 500.0f;

	/** Steepest walkable surface angle on which this tool may be deployed. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Capture|Deploy",
		meta = (ClampMin = "0.0", ClampMax = "89.0", UIMin = "0.0", UIMax = "89.0")
	)
	float MaxGroundSlopeAngle = 45.0f;

	/** Optional material for a locally valid placement preview. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Capture|Preview"
	)
	TObjectPtr<UMaterialInterface> ValidPreviewMaterial;

	/** Optional material for a locally invalid placement preview. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Capture|Preview"
	)
	TObjectPtr<UMaterialInterface> InvalidPreviewMaterial;

	/** Replicated server-authoritative state for this deployment. */
	UPROPERTY(
		ReplicatedUsing = OnRep_ToolState,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Capture"
	)
	ECaptureToolState ToolState = ECaptureToolState::Held;

	/** Server-only state setter. It also applies the state locally. */
	void SetToolState(ECaptureToolState NewState);

	/** Server-only transition from a dropped tool into its arm delay. */
	void StartArming();

	/** Server-only timer callback that activates an unheld arming tool. */
	void FinishArming();

	/** Applies capture, physics, and optional Blueprint presentation state. */
	void ApplyToolState();

	UFUNCTION(BlueprintImplementableEvent, Category = "Capture")
	void BP_OnToolStateChanged(ECaptureToolState NewState);

	/** Owning-client presentation hook for a local placement candidate. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Capture|Preview")
	void BP_OnPlacementPreviewUpdated(
		const FTransform& PreviewTransform,
		bool bIsValid
	);

	/** Owning-client presentation hook when a local placement preview ends. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Capture|Preview")
	void BP_OnPlacementPreviewEnded();

	UFUNCTION()
	void OnRep_ToolState();

	/** Final one-shot client request; the server rebuilds and validates it. */
	UFUNCTION(Server, Reliable)
	void ServerRequestDeploy(FTransform CandidateTransform);

private:
	UFUNCTION()
	void HandleRatCaptured();

	bool IsLocallyHeldByPlayer() const;
	bool GetLocalPlacementView(
		FVector& OutLocation,
		FVector& OutDirection
	) const;
	bool FindValidPlacementTransform(
		AMouseMouseCharacter* Holder,
		const FVector& ViewLocation,
		const FVector& ViewDirection,
		FTransform& OutTransform
	) const;
	bool IsPlacementAreaClear(
		const FTransform& PlacementTransform,
		AMouseMouseCharacter* Holder
	) const;
	FVector GetPlacementCollisionExtent() const;
	FVector GetPlacementCollisionCenter(
		const FTransform& PlacementTransform
	) const;
	float GetPlacementGroundOffset() const;

	void BeginLocalPrimaryUse();
	void UpdateLocalPlacementPreview(
		const FTransform& PreviewTransform,
		bool bIsValid
	);
	void EndLocalPrimaryUse();
	void CreatePlacementPreviewMesh();
	void DestroyPlacementPreviewMesh();

	void TryDeployOnServer(const FTransform& CandidateTransform);
	bool ValidateDeployRequest(
		AMouseMouseCharacter* Holder,
		const FTransform& CandidateTransform,
		FTransform& OutPlacementTransform
	) const;
	void MoveIntoDeployedWorldState(
		const FTransform& PlacementTransform
	);

	void ClearArmingTimer();

	FTimerHandle ArmingTimerHandle;

	/** Runtime-only component created exclusively by the locally owning player. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> PlacementPreviewMesh;

	bool bPrimaryUseHeld = false;
	bool bPlacementMode = false;
	bool bHasValidPlacement = false;
	float PrimaryUseStartTime = 0.0f;
	FTransform LocalPlacementTransform;
};
