// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/RatCard/MouseRatCardActor.h"

#include "Core/MouseMouseGameState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "MouseMouseCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Rat/RatDefinition.h"


AMouseRatCardActor::AMouseRatCardActor()
{
	// AMousePickupActor supplies replication, movement replication, collision,
	// physics, holder handling, and player interaction for a dropped card.
}

void AMouseRatCardActor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(
		AMouseRatCardActor,
		RatDefinition
	);

	DOREPLIFETIME(
		AMouseRatCardActor,
		bHasRegisteredCapture
	);
}

void AMouseRatCardActor::SetRatDefinition(
	URatDefinition* NewRatDefinition
)
{
	if (!HasAuthority() ||
		RatDefinition == NewRatDefinition)
	{
		return;
	}

	RatDefinition = NewRatDefinition;
	ForceNetUpdate();
}

void AMouseRatCardActor::OnHolderChanged(
	ACharacter* OldHolder,
	ACharacter* NewHolder
)
{
	if (!HasAuthority())
	{
		return;
	}

	Super::OnHolderChanged(OldHolder, NewHolder);

	AMouseMouseCharacter* CapturingCharacter =
		Cast<AMouseMouseCharacter>(NewHolder);

	if (bHasRegisteredCapture ||
		!IsValid(CapturingCharacter) ||
		!IsValid(RatDefinition))
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	AMouseMouseGameState* MouseGameState =
		World->GetGameState<AMouseMouseGameState>();

	if (!IsValid(MouseGameState))
	{
		return;
	}

	int32 CapturedCount = 0;
	bool bFirstDiscovery = false;

	if (!MouseGameState->RegisterCapturedRat(
		RatDefinition,
		CapturedCount,
		bFirstDiscovery
	))
	{
		return;
	}

	// Mark the physical card before notifying clients so future drops, pickups,
	// and handoffs cannot register this capture again.
	bHasRegisteredCapture = true;
	ForceNetUpdate();

	MouseGameState->MulticastRatCaptureRegistered(
		RatDefinition,
		CapturingCharacter->GetPlayerState(),
		CapturedCount,
		bFirstDiscovery
	);
}
