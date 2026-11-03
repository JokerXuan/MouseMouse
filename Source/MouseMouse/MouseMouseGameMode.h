// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MouseMouseGameMode.generated.h"

/**
 * Server-authoritative control of an objective-driven round.
 */
UCLASS(abstract)
class MOUSEMOUSE_API AMouseMouseGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMouseMouseGameMode();

	virtual void StartPlay() override;

	/** Starts only from Waiting. Returns false on a rejected or repeated call. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Round")
	bool StartRound();

	/** Ends only an InProgress round. The first result is final; there is no restart. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Round")
	bool FinishRound(bool bSucceeded);

protected:
	/** Starts after world BeginPlay. Objectives must explicitly finish the round. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round")
	bool bAutoStartRound = true;
};



