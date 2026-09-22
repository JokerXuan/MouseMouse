// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/MousePickupActor.h"
#include "MouseRatCardActor.generated.h"

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
};
