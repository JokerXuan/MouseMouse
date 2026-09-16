// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/MousePickupActor.h"
#include "MouseFoodActor.generated.h"

/**
 * 
 */
UCLASS()
class MOUSEMOUSE_API AMouseFoodActor : public AMousePickupActor
{
	GENERATED_BODY()

public:
	AMouseFoodActor();

	float GetFoodValue() const
	{
		return FoodValue;
	}

	bool IsAvailableForRat() const;

protected:

	/** How valuable this food is to rats */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food", meta = (ClampMin = "0.0"))
	float FoodValue = 1.0f;
	
};
