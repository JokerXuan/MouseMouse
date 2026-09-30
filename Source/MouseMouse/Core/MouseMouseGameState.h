// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MouseMouseGameState.generated.h"

class APlayerState;
class URatDefinition;

/** One team-wide capture record for a rat definition. */
USTRUCT(BlueprintType)
struct MOUSEMOUSE_API FRatCodexEntry
{
	GENERATED_BODY()

	/** Immutable rat identity shared by all cards of this type. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Rat Codex")
	TObjectPtr<URatDefinition> RatDefinition = nullptr;

	/** Number of distinct RatCard actors registered for this rat type. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Rat Codex")
	int32 CapturedCount = 0;
};

/**
 * Replicated match-wide state for the mouse team.
 */
UCLASS()
class MOUSEMOUSE_API AMouseMouseGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AMouseMouseGameState();

	/** Persistent, team-shared codex. Late joiners receive this through replication. */
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Rat Codex")
	TArray<FRatCodexEntry> TeamRatCodex;

	/**
	 * Server-only codex mutation for the first player pickup of a RatCard.
	 * Returns false when the supplied definition is invalid or this is not the
	 * authoritative GameState. On success, the out values describe this capture.
	 */
	bool RegisterCapturedRat(
		URatDefinition* CapturedRatDefinition,
		int32& OutCapturedCount,
		bool& bOutFirstDiscovery
	);

	/**
	 * Transient presentation event for players currently connected to the match.
	 * It deliberately is not retained, unlike TeamRatCodex.
	 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRatCaptureRegistered(
		URatDefinition* CapturedRatDefinition,
		APlayerState* CapturingPlayerState,
		int32 CapturedCount,
		bool bFirstDiscovery
	);

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

	/** Blueprint presentation hook; no concrete UI is owned by GameState. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Rat Codex|Broadcast")
	void BP_OnRatCaptureRegistered(
		URatDefinition* CapturedRatDefinition,
		APlayerState* CapturingPlayerState,
		int32 CapturedCount,
		bool bFirstDiscovery
	);
};
