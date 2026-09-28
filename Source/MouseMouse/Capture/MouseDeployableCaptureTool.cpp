// Fill out your copyright notice in the Description page of Project Settings.

#include "Capture/MouseDeployableCaptureTool.h"

#include "Capture/RatCaptureAreaComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"


AMouseDeployableCaptureTool::AMouseDeployableCaptureTool()
{
	PrimaryActorTick.bCanEverTick = false;

	CaptureArea = CreateDefaultSubobject<URatCaptureAreaComponent>(
		TEXT("Capture Area")
	);

	CaptureArea->SetupAttachment(GetRootComponent());

	// RatCaptureAreaComponent remains enabled by default for existing uses.
	// This tool always begins disabled and is enabled only by Armed on the server.
	CaptureArea->SetGenerateOverlapEvents(false);
	CaptureArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CaptureArea->OnRatCaptured.AddDynamic(
		this,
		&AMouseDeployableCaptureTool::HandleRatCaptured
	);
}

void AMouseDeployableCaptureTool::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		ApplyToolState();

		return;
	}

	if (IsValid(GetHolder()))
	{
		SetToolState(ECaptureToolState::Held);

		return;
	}

	// A tool placed directly in a level starts as a fresh deployment.
	StartArming();
}

void AMouseDeployableCaptureTool::EndPlay(
	const EEndPlayReason::Type EndPlayReason
)
{
	ClearArmingTimer();

	Super::EndPlay(EndPlayReason);
}

void AMouseDeployableCaptureTool::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(
		AMouseDeployableCaptureTool,
		ToolState
	);
}

void AMouseDeployableCaptureTool::OnHolderChanged(
	ACharacter* OldHolder,
	ACharacter* NewHolder
)
{
	if (!HasAuthority())
	{
		return;
	}

	if (IsValid(NewHolder))
	{
		ClearArmingTimer();
		SetToolState(ECaptureToolState::Held);

		return;
	}

	if (IsValid(OldHolder) &&
		NewHolder == nullptr)
	{
		StartArming();
	}
}

void AMouseDeployableCaptureTool::SetToolState(
	ECaptureToolState NewState
)
{
	if (!HasAuthority())
	{
		return;
	}

	ToolState = NewState;

	// OnRep normally runs on clients, so the server applies it manually.
	ApplyToolState();

	ForceNetUpdate();
}

void AMouseDeployableCaptureTool::StartArming()
{
	if (!HasAuthority())
	{
		return;
	}

	ClearArmingTimer();

	if (IsValid(GetHolder()))
	{
		SetToolState(ECaptureToolState::Held);

		return;
	}

	if (ArmDelay <= 0.0f)
	{
		SetToolState(ECaptureToolState::Armed);

		return;
	}

	SetToolState(ECaptureToolState::Arming);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ArmingTimerHandle,
			this,
			&AMouseDeployableCaptureTool::FinishArming,
			ArmDelay,
			false
		);
	}
}

void AMouseDeployableCaptureTool::FinishArming()
{
	if (!HasAuthority() ||
		ToolState != ECaptureToolState::Arming ||
		IsValid(GetHolder()))
	{
		return;
	}

	ClearArmingTimer();
	SetToolState(ECaptureToolState::Armed);
}

void AMouseDeployableCaptureTool::ApplyToolState()
{
	if (CaptureArea)
	{
		if (HasAuthority())
		{
			CaptureArea->SetCaptureEnabled(
				ToolState == ECaptureToolState::Armed
			);
		}
		else
		{
			// Clients only receive the replicated state and presentation. They do
			// not perform capture overlap queries or influence server gameplay.
			CaptureArea->SetGenerateOverlapEvents(false);
			CaptureArea->SetCollisionEnabled(
				ECollisionEnabled::NoCollision
			);
		}
	}

	BP_OnToolStateChanged(ToolState);
}

void AMouseDeployableCaptureTool::OnRep_ToolState()
{
	ApplyToolState();
}

void AMouseDeployableCaptureTool::HandleRatCaptured()
{
	if (!HasAuthority() ||
		ToolState != ECaptureToolState::Armed)
	{
		return;
	}

	SetToolState(ECaptureToolState::Triggered);
}

void AMouseDeployableCaptureTool::ClearArmingTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ArmingTimerHandle);
	}
}
