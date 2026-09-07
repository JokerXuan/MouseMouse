// Copyright Epic Games, Inc. All Rights Reserved.

#include "MouseMouseGameMode.h"
#include "Core/MouseMouseGameState.h"

AMouseMouseGameMode::AMouseMouseGameMode()
{
	GameStateClass = AMouseMouseGameState::StaticClass();
}
