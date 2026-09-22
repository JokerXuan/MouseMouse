// Fill out your copyright notice in the Description page of Project Settings.

#include "Capture/RatCaptureAreaComponent.h"

#include "Rat/MouseRatCharacter.h"


URatCaptureAreaComponent::URatCaptureAreaComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(true);

	OnComponentBeginOverlap.AddDynamic(
		this,
		&URatCaptureAreaComponent::HandleCaptureBeginOverlap
	);
}


void URatCaptureAreaComponent::SetCaptureEnabled(
	bool bEnabled
)
{
	AActor* Owner = GetOwner();

	if (!Owner ||
		!Owner->HasAuthority())
	{
		return;
	}

	bCaptureEnabled = bEnabled;

	SetGenerateOverlapEvents(bCaptureEnabled);
	SetCollisionEnabled(
		bCaptureEnabled
			? ECollisionEnabled::QueryOnly
			: ECollisionEnabled::NoCollision
	);
}


void URatCaptureAreaComponent::HandleCaptureBeginOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/
)
{
	AActor* Owner = GetOwner();

	if (!bCaptureEnabled ||
		!Owner ||
		!Owner->HasAuthority())
	{
		return;
	}

	AMouseRatCharacter* Rat =
		Cast<AMouseRatCharacter>(OtherActor);

	if (!IsValid(Rat))
	{
		return;
	}

	if (Rat->TryCapture())
	{
		OnRatCaptured.Broadcast();
	}
}
