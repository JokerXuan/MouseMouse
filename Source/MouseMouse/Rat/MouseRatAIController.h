// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MouseRatAIController.generated.h"

class AMouseFoodActor;
/**
 * 
 */
UCLASS()
class MOUSEMOUSE_API AMouseRatAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AMouseRatAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	void SearchForFood();

	AMouseFoodActor* FindClosestAvailableFood() const;

	FTimerHandle FoodSearchTimerHandle;
};
