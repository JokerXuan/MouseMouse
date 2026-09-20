// Fill out your copyright notice in the Description page of Project Settings.

#include "Rat/MouseRatNest.h"

#include "Components/SceneComponent.h"
#include "Items/Food/MouseFoodActor.h"
#include "MouseMouse.h"
#include "Net/UnrealNetwork.h"
#include "Rat/MouseRatCharacter.h"


AMouseRatNest::AMouseRatNest()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(
		TEXT("Scene Root")
	);

	SetRootComponent(SceneRoot);

	DepositPoint = CreateDefaultSubobject<USceneComponent>(
		TEXT("Deposit Point")
	);

	DepositPoint->SetupAttachment(SceneRoot);
}


FVector AMouseRatNest::GetDepositLocation() const
{
	return DepositPoint
		? DepositPoint->GetComponentLocation()
		: GetActorLocation();
}


void AMouseRatNest::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(
		AMouseRatNest,
		StoredFoodValue
	);
}


bool AMouseRatNest::TryStoreFood(
	AMouseRatCharacter* DepositingRat,
	AMouseFoodActor* Food
)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!IsValid(DepositingRat) ||
		!IsValid(Food))
	{
		return false;
	}

	// The nest only accepts the food that this rat is actually carrying.
	if (Food->GetHolder() != DepositingRat ||
		DepositingRat->GetCarriedFood() != Food)
	{
		return false;
	}

	if (!DepositingRat->IsWithinReachOfPoint(
		GetDepositLocation(),
		DepositDistance
	))
	{
		return false;
	}

	const float FoodValue = Food->GetFoodValue();
	const FString FoodName = GetNameSafe(Food);

	if (FoodValue < 0.0f)
	{
		return false;
	}

	// Do not credit the nest if this actor cannot be removed from the world.
	if (!Food->Destroy())
	{
		UE_LOG(
			LogMouseMouse,
			Warning,
			TEXT("Rat nest '%s' could not destroy deposited food '%s'."),
			*GetNameSafe(this),
			*FoodName
		);

		return false;
	}

	StoredFoodValue += FoodValue;

	ForceNetUpdate();

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Rat '%s' deposited food '%s' in nest '%s' (+%.2f, total %.2f)."),
		*GetNameSafe(DepositingRat),
		*FoodName,
		*GetNameSafe(this),
		FoodValue,
		StoredFoodValue
	);

	return true;
}
