// Fill out your copyright notice in the Description page of Project Settings.

#include "Capture/MouseDeployableCaptureTool.h"

#include "Camera/CameraComponent.h"
#include "Capture/RatCaptureAreaComponent.h"
#include "CollisionShape.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInterface.h"
#include "MouseMouseCharacter.h"
#include "Net/UnrealNetwork.h"


namespace
{
constexpr float DeploymentGroundClearance = 2.0f;
constexpr float DeploymentRequestTolerance = 100.0f;
constexpr float MinimumPlacementExtent = 5.0f;

FVector GetAbsoluteScale(const FVector& Scale)
{
	return FVector(
		FMath::Abs(Scale.X),
		FMath::Abs(Scale.Y),
		FMath::Abs(Scale.Z)
	);
}
}


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

	// A world pickup is inert until a player explicitly deploys it.
	SetToolState(ECaptureToolState::Inactive);
}

void AMouseDeployableCaptureTool::EndPlay(
	const EEndPlayReason::Type EndPlayReason
)
{
	ClearArmingTimer();
	EndLocalPrimaryUse();

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

void AMouseDeployableCaptureTool::PrimaryUseStarted()
{
	if (!IsLocallyHeldByPlayer() ||
		ToolState != ECaptureToolState::Held)
	{
		return;
	}

	BeginLocalPrimaryUse();
}

void AMouseDeployableCaptureTool::PrimaryUseTriggered()
{
	if (!bPrimaryUseHeld)
	{
		return;
	}

	if (!IsLocallyHeldByPlayer() ||
		ToolState != ECaptureToolState::Held)
	{
		EndLocalPrimaryUse();

		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		EndLocalPrimaryUse();

		return;
	}

	if (!bPlacementMode)
	{
		const float HeldTime =
			World->GetTimeSeconds() - PrimaryUseStartTime;

		if (HeldTime < HoldToDeployTime)
		{
			return;
		}

		bPlacementMode = true;
	}

	FVector ViewLocation;
	FVector ViewDirection;

	if (!GetLocalPlacementView(
		ViewLocation,
		ViewDirection
	))
	{
		return;
	}

	AMouseMouseCharacter* Holder =
		Cast<AMouseMouseCharacter>(GetHolder());

	FTransform CandidateTransform;
	const bool bIsValid = FindValidPlacementTransform(
		Holder,
		ViewLocation,
		ViewDirection,
		CandidateTransform
	);

	if (!bIsValid)
	{
		CandidateTransform = FTransform(
			FRotator(0.0f, ViewDirection.Rotation().Yaw, 0.0f),
			ViewLocation + ViewDirection * MaxDeployDistance,
			GetActorScale3D()
		);
	}

	LocalPlacementTransform = CandidateTransform;
	bHasValidPlacement = bIsValid;

	UpdateLocalPlacementPreview(
		CandidateTransform,
		bIsValid
	);
}

void AMouseDeployableCaptureTool::PrimaryUseCompleted()
{
	if (!bPrimaryUseHeld)
	{
		return;
	}

	const bool bShouldRequestDeploy =
		bPlacementMode &&
		bHasValidPlacement &&
		ToolState == ECaptureToolState::Held &&
		IsLocallyHeldByPlayer();

	const FTransform CandidateTransform = LocalPlacementTransform;

	EndLocalPrimaryUse();

	if (!bShouldRequestDeploy)
	{
		return;
	}

	if (HasAuthority())
	{
		TryDeployOnServer(CandidateTransform);
	}
	else
	{
		ServerRequestDeploy(CandidateTransform);
	}
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
		ClearArmingTimer();
		SetToolState(ECaptureToolState::Inactive);
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

	if (ToolState != ECaptureToolState::Held)
	{
		EndLocalPrimaryUse();
	}

	BP_OnToolStateChanged(ToolState);
}

void AMouseDeployableCaptureTool::OnRep_ToolState()
{
	ApplyToolState();
}

void AMouseDeployableCaptureTool::ServerRequestDeploy_Implementation(
	FTransform CandidateTransform
)
{
	TryDeployOnServer(CandidateTransform);
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

bool AMouseDeployableCaptureTool::IsLocallyHeldByPlayer() const
{
	const AMouseMouseCharacter* Holder =
		Cast<AMouseMouseCharacter>(GetHolder());

	return IsValid(Holder) &&
		Holder->IsLocallyControlled() &&
		Holder->GetHeldActor() == this;
}

bool AMouseDeployableCaptureTool::GetLocalPlacementView(
	FVector& OutLocation,
	FVector& OutDirection
) const
{
	AMouseMouseCharacter* Holder =
		Cast<AMouseMouseCharacter>(GetHolder());

	if (!IsValid(Holder) ||
		!Holder->IsLocallyControlled())
	{
		return false;
	}

	if (UCameraComponent* Camera =
		Holder->GetFirstPersonCameraComponent())
	{
		OutLocation = Camera->GetComponentLocation();
		OutDirection = Camera->GetForwardVector().GetSafeNormal();
	}
	else
	{
		FRotator ViewRotation;
		Holder->GetActorEyesViewPoint(
			OutLocation,
			ViewRotation
		);

		OutDirection = ViewRotation.Vector();
	}

	return !OutDirection.IsNearlyZero();
}

bool AMouseDeployableCaptureTool::FindValidPlacementTransform(
	AMouseMouseCharacter* Holder,
	const FVector& ViewLocation,
	const FVector& ViewDirection,
	FTransform& OutTransform
) const
{
	if (!IsValid(Holder) ||
		MaxDeployDistance <= 0.0f)
	{
		return false;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	const FVector NormalizedDirection =
		ViewDirection.GetSafeNormal();

	if (NormalizedDirection.IsNearlyZero())
	{
		return false;
	}

	FHitResult SurfaceHit;
	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(CaptureToolDeployTrace),
		false,
		this
	);

	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(Holder);

	const bool bHitSurface = World->LineTraceSingleByChannel(
		SurfaceHit,
		ViewLocation,
		ViewLocation + NormalizedDirection * MaxDeployDistance,
		ECC_Visibility,
		QueryParams
	);

	if (!bHitSurface ||
		!SurfaceHit.bBlockingHit)
	{
		return false;
	}

	const float MinUpNormal = FMath::Cos(
		FMath::DegreesToRadians(
			FMath::Clamp(MaxGroundSlopeAngle, 0.0f, 89.0f)
		)
	);

	if (FVector::DotProduct(
		SurfaceHit.ImpactNormal.GetSafeNormal(),
		FVector::UpVector
	) < MinUpNormal)
	{
		return false;
	}

	const FTransform PlacementTransform(
		FRotator(
			0.0f,
			NormalizedDirection.Rotation().Yaw,
			0.0f
		),
		SurfaceHit.ImpactPoint +
			FVector::UpVector * GetPlacementGroundOffset(),
		GetActorScale3D()
	);

	if (!IsPlacementAreaClear(
		PlacementTransform,
		Holder
	))
	{
		return false;
	}

	OutTransform = PlacementTransform;

	return true;
}

bool AMouseDeployableCaptureTool::IsPlacementAreaClear(
	const FTransform& PlacementTransform,
	AMouseMouseCharacter* Holder
) const
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(CaptureToolDeployClearance),
		false,
		this
	);

	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(Holder);

	const FCollisionShape PlacementShape =
		FCollisionShape::MakeBox(GetPlacementCollisionExtent());

	return !World->OverlapBlockingTestByChannel(
		GetPlacementCollisionCenter(PlacementTransform),
		PlacementTransform.GetRotation(),
		ECC_Visibility,
		PlacementShape,
		QueryParams
	);
}

FVector AMouseDeployableCaptureTool::GetPlacementCollisionExtent() const
{
	if (!Mesh)
	{
		return FVector(MinimumPlacementExtent);
	}

	FVector LocalBoundsMin;
	FVector LocalBoundsMax;

	Mesh->GetLocalBounds(
		LocalBoundsMin,
		LocalBoundsMax
	);

	const FVector LocalExtent =
		(LocalBoundsMax - LocalBoundsMin) * 0.5f;

	if (LocalExtent.IsNearlyZero())
	{
		return FVector(MinimumPlacementExtent);
	}

	const FVector Scale = GetAbsoluteScale(GetActorScale3D());

	return FVector(
		FMath::Max(MinimumPlacementExtent, LocalExtent.X * Scale.X),
		FMath::Max(MinimumPlacementExtent, LocalExtent.Y * Scale.Y),
		FMath::Max(MinimumPlacementExtent, LocalExtent.Z * Scale.Z)
	);
}

FVector AMouseDeployableCaptureTool::GetPlacementCollisionCenter(
	const FTransform& PlacementTransform
) const
{
	if (!Mesh)
	{
		return PlacementTransform.GetLocation();
	}

	FVector LocalBoundsMin;
	FVector LocalBoundsMax;

	Mesh->GetLocalBounds(
		LocalBoundsMin,
		LocalBoundsMax
	);

	const FVector LocalCenter =
		(LocalBoundsMin + LocalBoundsMax) * 0.5f;
	const FVector Scale = GetAbsoluteScale(
		PlacementTransform.GetScale3D()
	);
	const FVector ScaledLocalCenter(
		LocalCenter.X * Scale.X,
		LocalCenter.Y * Scale.Y,
		LocalCenter.Z * Scale.Z
	);

	return PlacementTransform.GetLocation() +
		PlacementTransform.GetRotation().RotateVector(
			ScaledLocalCenter
		);
}

float AMouseDeployableCaptureTool::GetPlacementGroundOffset() const
{
	if (!Mesh)
	{
		return DeploymentGroundClearance;
	}

	FVector LocalBoundsMin;
	FVector LocalBoundsMax;

	Mesh->GetLocalBounds(
		LocalBoundsMin,
		LocalBoundsMax
	);

	const FVector Scale = GetAbsoluteScale(GetActorScale3D());

	return FMath::Max(
		0.0f,
		-LocalBoundsMin.Z * Scale.Z
	) + DeploymentGroundClearance;
}

void AMouseDeployableCaptureTool::BeginLocalPrimaryUse()
{
	EndLocalPrimaryUse();

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	bPrimaryUseHeld = true;
	PrimaryUseStartTime = World->GetTimeSeconds();
}

void AMouseDeployableCaptureTool::UpdateLocalPlacementPreview(
	const FTransform& PreviewTransform,
	bool bIsValid
)
{
	CreatePlacementPreviewMesh();

	if (PlacementPreviewMesh)
	{
		FTransform PreviewMeshTransform = PreviewTransform;

		PlacementPreviewMesh->SetWorldTransform(
			PreviewMeshTransform,
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);

		UMaterialInterface* PreviewMaterial = bIsValid
			? ValidPreviewMaterial.Get()
			: InvalidPreviewMaterial.Get();

		if (Mesh)
		{
			for (int32 MaterialIndex = 0;
				MaterialIndex < PlacementPreviewMesh->GetNumMaterials();
				++MaterialIndex)
			{
				PlacementPreviewMesh->SetMaterial(
					MaterialIndex,
					PreviewMaterial
						? PreviewMaterial
						: Mesh->GetMaterial(MaterialIndex)
				);
			}
		}

		PlacementPreviewMesh->SetVisibility(true);
	}

	BP_OnPlacementPreviewUpdated(
		PreviewTransform,
		bIsValid
	);
}

void AMouseDeployableCaptureTool::EndLocalPrimaryUse()
{
	const bool bHadPlacementPreview =
		bPlacementMode ||
		IsValid(PlacementPreviewMesh);

	bPrimaryUseHeld = false;
	bPlacementMode = false;
	bHasValidPlacement = false;
	PrimaryUseStartTime = 0.0f;

	DestroyPlacementPreviewMesh();

	if (bHadPlacementPreview)
	{
		BP_OnPlacementPreviewEnded();
	}
}

void AMouseDeployableCaptureTool::CreatePlacementPreviewMesh()
{
	if (PlacementPreviewMesh ||
		!GetWorld())
	{
		return;
	}

	UStaticMeshComponent* NewPreviewMesh =
		NewObject<UStaticMeshComponent>(
			this,
			NAME_None,
			RF_Transient
		);

	if (!NewPreviewMesh)
	{
		return;
	}

	NewPreviewMesh->SetIsReplicated(false);
	NewPreviewMesh->SetStaticMesh(
		Mesh
			? Mesh->GetStaticMesh()
			: nullptr
	);
	NewPreviewMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);
	NewPreviewMesh->SetGenerateOverlapEvents(false);
	NewPreviewMesh->SetCastShadow(false);
	NewPreviewMesh->SetHiddenInGame(false);
	NewPreviewMesh->RegisterComponentWithWorld(GetWorld());

	PlacementPreviewMesh = NewPreviewMesh;
}

void AMouseDeployableCaptureTool::DestroyPlacementPreviewMesh()
{
	if (PlacementPreviewMesh)
	{
		PlacementPreviewMesh->DestroyComponent();
		PlacementPreviewMesh = nullptr;
	}
}

void AMouseDeployableCaptureTool::TryDeployOnServer(
	const FTransform& CandidateTransform
)
{
	if (!HasAuthority())
	{
		return;
	}

	AMouseMouseCharacter* Holder =
		Cast<AMouseMouseCharacter>(GetHolder());

	FTransform PlacementTransform;

	if (!ValidateDeployRequest(
		Holder,
		CandidateTransform,
		PlacementTransform
	))
	{
		return;
	}

	if (!Holder->ReleaseHeldPickup(this))
	{
		return;
	}

	MoveIntoDeployedWorldState(PlacementTransform);
	StartArming();
}

bool AMouseDeployableCaptureTool::ValidateDeployRequest(
	AMouseMouseCharacter* Holder,
	const FTransform& CandidateTransform,
	FTransform& OutPlacementTransform
) const
{
	if (!IsValid(Holder) ||
		Holder != GetHolder() ||
		Holder->GetHeldActor() != this ||
		GetOwner() != Holder ||
		ToolState != ECaptureToolState::Held ||
		CandidateTransform.GetLocation().ContainsNaN())
	{
		return false;
	}

	FVector ServerViewLocation;
	FRotator ServerViewRotation;

	Holder->GetActorEyesViewPoint(
		ServerViewLocation,
		ServerViewRotation
	);

	FTransform ServerPlacementTransform;

	if (!FindValidPlacementTransform(
		Holder,
		ServerViewLocation,
		ServerViewRotation.Vector(),
		ServerPlacementTransform
	))
	{
		return false;
	}

	if (FVector::DistSquared(
		CandidateTransform.GetLocation(),
		ServerPlacementTransform.GetLocation()
	) > FMath::Square(DeploymentRequestTolerance))
	{
		return false;
	}

	OutPlacementTransform = ServerPlacementTransform;

	return true;
}

void AMouseDeployableCaptureTool::MoveIntoDeployedWorldState(
	const FTransform& PlacementTransform
)
{
	if (Mesh)
	{
		Mesh->SetSimulatePhysics(false);
	}

	SetActorTransform(
		PlacementTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	SetActorEnableCollision(true);
	ForceNetUpdate();
}

void AMouseDeployableCaptureTool::ClearArmingTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ArmingTimerHandle);
	}
}
