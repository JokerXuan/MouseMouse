// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/RatCard/MouseRatCardActor.h"

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
