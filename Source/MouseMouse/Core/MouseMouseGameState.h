// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MouseMouseGameState.generated.h"

class APlayerState;
class AMouseMouseGameMode;
class URatDefinition;

/** Objective-driven round phases; results are terminal for this world. */
UENUM(BlueprintType)
enum class ERoundPhase : uint8
{
	Waiting,
	InProgress,
	Succeeded,
	Failed
};

/** Persistent round snapshot. RoundId is zero before the first start, then one. */
USTRUCT(BlueprintType)
struct MOUSEMOUSE_API FMouseRoundState
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Round")
	ERoundPhase Phase = ERoundPhase::Waiting;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Round")
	int32 RoundId = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnMouseRoundStateChanged,
	const FMouseRoundState&, PreviousState,
	const FMouseRoundState&, NewState
);

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

	/** Read-only snapshot on server or client, including the state for late joiners. */
	UFUNCTION(BlueprintPure, Category = "Round")
	FMouseRoundState GetRoundState() const
	{
		return RoundState;
	}

	/**
	 * Local notification on the server and receiving clients. Bind future HUDs here
	 * and also read GetRoundState() when binding, since replication may arrive first.
	 * Replication delivers current state; it may coalesce intermediate transitions.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Round")
	FOnMouseRoundStateChanged OnRoundStateChanged;

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

private:
	friend class AMouseMouseGameMode;

	/** Only GameMode can request a transition; no client setter or RPC is exposed. */
	bool TryTransitionRoundPhase(ERoundPhase NewPhase);

	UFUNCTION()
	void OnRep_RoundState(FMouseRoundState PreviousState);

	UPROPERTY(ReplicatedUsing = OnRep_RoundState)
	FMouseRoundState RoundState;
};
