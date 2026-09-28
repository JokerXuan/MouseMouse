// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/MousePickupActor.h"
#include "TimerManager.h"
#include "MouseDeployableCaptureTool.generated.h"

class URatCaptureAreaComponent;

UENUM(BlueprintType)
enum class ECaptureToolState : uint8
{
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

	/** Applies local capture-area behavior and optional Blueprint presentation. */
	void ApplyToolState();

	UFUNCTION(BlueprintImplementableEvent, Category = "Capture")
	void BP_OnToolStateChanged(ECaptureToolState NewState);

	UFUNCTION()
	void OnRep_ToolState();

private:
	UFUNCTION()
	void HandleRatCaptured();

	void ClearArmingTimer();

	FTimerHandle ArmingTimerHandle;
};
