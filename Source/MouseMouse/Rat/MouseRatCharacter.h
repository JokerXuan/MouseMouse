// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MouseRatCharacter.generated.h"

class USceneComponent;

UCLASS()
class MOUSEMOUSE_API AMouseRatCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMouseRatCharacter();

	USceneComponent* GetCarryPoint() const
	{
		return CarryPoint;
	}

protected:

	/** Point where carried items are attached */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components"
	)
	TObjectPtr<USceneComponent> CarryPoint;
};
