// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/MouseMouseGameState.h"

#include "MouseMouse.h"
#include "Net/UnrealNetwork.h"
#include "Rat/RatDefinition.h"

AMouseMouseGameState::AMouseMouseGameState()
{
	// GameStateBase is replicated by the framework; keep this explicit because
	// TeamRatCodex is authoritative match state.
	bReplicates = true;
}

void AMouseMouseGameState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(
		AMouseMouseGameState,
		TeamRatCodex
	);
	DOREPLIFETIME(AMouseMouseGameState, RoundState);
}

bool AMouseMouseGameState::TryTransitionRoundPhase(ERoundPhase NewPhase)
{
	if (!HasAuthority())
	{
		return false;
	}

	const bool bStarting = RoundState.Phase == ERoundPhase::Waiting &&
		NewPhase == ERoundPhase::InProgress;
	const bool bFinishing = RoundState.Phase == ERoundPhase::InProgress &&
		(NewPhase == ERoundPhase::Succeeded || NewPhase == ERoundPhase::Failed);
	if (!bStarting && !bFinishing)
	{
		return false;
	}

	const FMouseRoundState PreviousState = RoundState;
	RoundState.Phase = NewPhase;
	if (bStarting)
	{
		++RoundState.RoundId;
	}

	ForceNetUpdate();
	// RepNotify is not invoked automatically for authoritative C++ assignments.
	OnRep_RoundState(PreviousState);
	return true;
}

void AMouseMouseGameState::OnRep_RoundState(FMouseRoundState PreviousState)
{
	// Snapshot the new state so a server listener can safely finish during the callback.
	const FMouseRoundState NewState = RoundState;
	UE_LOG(LogMouseMouse, Log, TEXT("Round %d: %s -> %s (%s)."),
		NewState.RoundId,
		*UEnum::GetValueAsString(PreviousState.Phase),
		*UEnum::GetValueAsString(NewState.Phase),
		HasAuthority() ? TEXT("Server") : TEXT("Client"));
	OnRoundStateChanged.Broadcast(PreviousState, NewState);
}

bool AMouseMouseGameState::RegisterCapturedRat(
	URatDefinition* CapturedRatDefinition,
	int32& OutCapturedCount,
	bool& bOutFirstDiscovery
)
{
	OutCapturedCount = 0;
	bOutFirstDiscovery = false;

	if (!HasAuthority() ||
		!IsValid(CapturedRatDefinition))
	{
		return false;
	}

	for (FRatCodexEntry& Entry : TeamRatCodex)
	{
		if (Entry.RatDefinition != CapturedRatDefinition)
		{
			continue;
		}

		++Entry.CapturedCount;
		OutCapturedCount = Entry.CapturedCount;
		ForceNetUpdate();

		return true;
	}

	FRatCodexEntry& NewEntry = TeamRatCodex.AddDefaulted_GetRef();
	NewEntry.RatDefinition = CapturedRatDefinition;
	NewEntry.CapturedCount = 1;

	OutCapturedCount = NewEntry.CapturedCount;
	bOutFirstDiscovery = true;
	ForceNetUpdate();

	return true;
}

void AMouseMouseGameState::MulticastRatCaptureRegistered_Implementation(
	URatDefinition* CapturedRatDefinition,
	APlayerState* CapturingPlayerState,
	int32 CapturedCount,
	bool bFirstDiscovery
)
{
	BP_OnRatCaptureRegistered(
		CapturedRatDefinition,
		CapturingPlayerState,
		CapturedCount,
		bFirstDiscovery
	);
}
