// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/MouseMouseGameState.h"

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
