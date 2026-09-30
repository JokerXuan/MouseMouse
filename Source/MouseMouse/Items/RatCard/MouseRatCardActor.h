// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/MousePickupActor.h"
#include "MouseRatCardActor.generated.h"

class URatDefinition;

/**
 * World pickup created when a rat is captured.
 *
 * It deliberately inherits the existing pickup implementation so cards keep
 * its replicated movement, physics, holder state, and player interaction.
 */
UCLASS()
class MOUSEMOUSE_API AMouseRatCardActor : public AMousePickupActor
{
	GENERATED_BODY()

public:
	AMouseRatCardActor();

	URatDefinition* GetRatDefinition() const
	{
		return RatDefinition;
	}

	/** Sets the card identity on the server before replication. */
	void SetRatDefinition(URatDefinition* NewRatDefinition);

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

protected:
	/**
	 * A card registers its capture exactly once, when it first enters a player's
	 * hands on the server. Dropping or trading the persistent world actor does
	 * not change the team codex again.
	 */
	virtual void OnHolderChanged(
		ACharacter* OldHolder,
		ACharacter* NewHolder
	) override;

	UPROPERTY(
		Replicated,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Rat Card|Identity"
	)
	TObjectPtr<URatDefinition> RatDefinition;

	/** Server-owned one-shot guard for this physical card actor. */
	UPROPERTY(
		Replicated,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Rat Card|Codex"
	)
	bool bHasRegisteredCapture = false;
};
