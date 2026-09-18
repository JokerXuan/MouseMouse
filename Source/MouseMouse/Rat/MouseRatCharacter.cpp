#include "Rat/MouseRatCharacter.h"

#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/Food/MouseFoodActor.h"
#include "Net/UnrealNetwork.h"
#include "Rat/MouseRatAIController.h"


AMouseRatCharacter::AMouseRatCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	CarryPoint = CreateDefaultSubobject<USceneComponent>(
		TEXT("Carry Point")
	);

	CarryPoint->SetupAttachment(GetRootComponent());

	CarryPoint->SetRelativeLocation(
		FVector(15.0f, 0.0f, 0.0f)
	);

	AIControllerClass =
		AMouseRatAIController::StaticClass();

	AutoPossessAI =
		EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 250.0f;
}


void AMouseRatCharacter::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(
		AMouseRatCharacter,
		CarriedFood
	);
}


bool AMouseRatCharacter::TryPickupFood(
	AMouseFoodActor* Food
)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!IsValid(Food))
	{
		return false;
	}

	if (IsValid(CarriedFood))
	{
		return false;
	}

	if (!CarryPoint)
	{
		return false;
	}

	if (!Food->IsAvailableForRat())
	{
		return false;
	}

	CarriedFood = Food;

	Food->SetOwner(this);
	Food->SetHolder(this);

	ForceNetUpdate();
	Food->ForceNetUpdate();

	return true;
}
