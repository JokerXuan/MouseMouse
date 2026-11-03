// Copyright Epic Games, Inc. All Rights Reserved.

#include "MouseMouseGameMode.h"
#include "Core/MouseMouseGameState.h"
#include "MouseMouse.h"

AMouseMouseGameMode::AMouseMouseGameMode()
{
	GameStateClass = AMouseMouseGameState::StaticClass();
}

void AMouseMouseGameMode::StartPlay()
{
	Super::StartPlay();

	if (HasAuthority() && bAutoStartRound)
	{
		StartRound();
	}
}

bool AMouseMouseGameMode::StartRound()
{
	if (!HasAuthority())
	{
		return false;
	}

	AMouseMouseGameState* MouseGameState = GetGameState<AMouseMouseGameState>();
	if (!IsValid(MouseGameState))
	{
		UE_LOG(LogMouseMouse, Error, TEXT("StartRound requires AMouseMouseGameState."));
		return false;
	}

	return MouseGameState->TryTransitionRoundPhase(ERoundPhase::InProgress);
}

bool AMouseMouseGameMode::FinishRound(bool bSucceeded)
{
	if (!HasAuthority())
	{
		return false;
	}

	AMouseMouseGameState* MouseGameState = GetGameState<AMouseMouseGameState>();
	if (!IsValid(MouseGameState))
	{
		UE_LOG(LogMouseMouse, Error, TEXT("FinishRound requires AMouseMouseGameState."));
		return false;
	}

	return MouseGameState->TryTransitionRoundPhase(
		bSucceeded ? ERoundPhase::Succeeded : ERoundPhase::Failed
	);
}
