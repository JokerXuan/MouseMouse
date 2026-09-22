// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "RatCaptureAreaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRatCaptured);

/**
 * Server-authoritative overlap area that asks an entering rat to capture
 * itself. The capture conversion intentionally remains owned by the rat.
 */
UCLASS(ClassGroup = (Mouse), meta = (BlueprintSpawnableComponent))
class MOUSEMOUSE_API URatCaptureAreaComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	URatCaptureAreaComponent();

	/** Enables or disables server-side capture and the overlap query. */
	UFUNCTION(BlueprintCallable, Category = "Capture")
	void SetCaptureEnabled(bool bEnabled);

	/** Returns whether this area accepts rats on the authoritative server. */
	UFUNCTION(BlueprintPure, Category = "Capture")
	bool IsCaptureEnabled() const
	{
		return bCaptureEnabled;
	}

	/** Fired on the server after an entering rat successfully converts to a card. */
	UPROPERTY(BlueprintAssignable, Category = "Capture")
	FOnRatCaptured OnRatCaptured;

private:
	UFUNCTION()
	void HandleCaptureBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	/** Server-side runtime gate for future armed/disabled capture tools. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture", meta = (AllowPrivateAccess = "true"))
	bool bCaptureEnabled = true;
};
