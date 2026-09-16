// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/MousePickupActor.h"

#include "Components/StaticMeshComponent.h"
#include "MouseMouseCharacter.h"
#include "Net/UnrealNetwork.h"


AMousePickupActor::AMousePickupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("Mesh")
	);

	SetRootComponent(Mesh);
}

void AMousePickupActor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(
		AMousePickupActor,
		HolderCharacter
	);
}

void AMousePickupActor::SetHolder(
	AMouseMouseCharacter* NewHolder
)
{
	if (!HasAuthority())
	{
		return;
	}

	HolderCharacter = NewHolder;

	// OnRep normally runs on clients, so the server applies it manually.
	ApplyHolderState();

	ForceNetUpdate();
}

void AMousePickupActor::OnRep_HolderCharacter()
{
	ApplyHolderState();
}

void AMousePickupActor::ApplyHolderState()
{
	if (!Mesh)
	{
		return;
	}

	if (IsValid(HolderCharacter))
	{
		// ----- HELD STATE -----

		Mesh->SetSimulatePhysics(false);

		SetActorEnableCollision(false);

		if (USceneComponent* HoldPoint =
			HolderCharacter->GetHoldPoint())
		{
			AttachToComponent(
				HoldPoint,
				FAttachmentTransformRules::
				SnapToTargetNotIncludingScale
			);
		}
	}
	else
	{
		// ----- DROPPED STATE -----

		DetachFromActor(
			FDetachmentTransformRules::
			KeepWorldTransform
		);

		SetActorEnableCollision(true);

		Mesh->SetSimulatePhysics(true);
		Mesh->WakeAllRigidBodies();
	}
}

void AMousePickupActor::Interact_Implementation(
	AActor* Interactor
)
{
	AMouseMouseCharacter* Character =
		Cast<AMouseMouseCharacter>(Interactor);

	if (!Character)
	{
		return;
	}

	Character->TryPickupActor(this);
}

