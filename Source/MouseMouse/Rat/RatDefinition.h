// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RatDefinition.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ERatRarity : uint8
{
	Common UMETA(DisplayName = "普通"),
	Epic UMETA(DisplayName = "史诗"),
	Mythic UMETA(DisplayName = "神话"),
	Mutant UMETA(DisplayName = "变异种")
};

/** Immutable identity data shared by a rat and its captured card. */
UCLASS(BlueprintType)
class MOUSEMOUSE_API URatDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rat Definition")
	FName RatId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rat Definition")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rat Definition")
	ERatRarity Rarity = ERatRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rat Definition")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rat Definition")
	int32 BaseCaptureValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rat Definition")
	TObjectPtr<UTexture2D> Icon;
};
