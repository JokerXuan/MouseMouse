// Fill out your copyright notice in the Description page of Project Settings.


#include "Rat/MouseRatCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SceneComponent.h"
#include "Rat/MouseRatAIController.h"


// Sets default values
AMouseRatCharacter::AMouseRatCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
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
