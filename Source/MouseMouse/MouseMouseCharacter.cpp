// Copyright Epic Games, Inc. All Rights Reserved.

#include "MouseMouseCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/MouseInteractionComponent.h"
#include "Interaction/MouseInteractable.h"
#include "Net/UnrealNetwork.h"
#include "Items/MousePickupActor.h"

#include "MouseMouse.h"

AMouseMouseCharacter::AMouseMouseCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the interaction component
	InteractionComponent = CreateDefaultSubobject<UMouseInteractionComponent>(TEXT("Interaction Component"));

	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// Create the point where held objects will be attached
	HoldPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Hold Point"));
	HoldPoint->SetupAttachment(FirstPersonCameraComponent);
	HoldPoint->SetRelativeLocation(FVector(80.0f, 0.0f, -20.0f));

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
}

void AMouseMouseCharacter::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(
		AMouseMouseCharacter,
		HeldActor
	);
}

bool AMouseMouseCharacter::TryPickupActor(
	AActor* ActorToPickup
)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!IsValid(ActorToPickup))
	{
		return false;
	}

	if (IsValid(HeldActor))
	{
		return false;
	}

	if (!HoldPoint)
	{
		return false;
	}

	if (ActorToPickup == this)
	{
		return false;
	}

	// Only MousePickupActor can use the pickup system
	AMousePickupActor* PickupActor =
		Cast<AMousePickupActor>(ActorToPickup);

	if (!PickupActor)
	{
		return false;
	}

	// Prevent two characters from holding the same item
	if (IsValid(PickupActor->GetHolder()))
	{
		return false;
	}

	// Character authoritative state
	HeldActor = PickupActor;

	// Gameplay / network ownership
	PickupActor->SetOwner(this);

	// PickupActor is responsible for physics,
	// collision and attachment.
	PickupActor->SetHolder(this);

	ForceNetUpdate();
	PickupActor->ForceNetUpdate();

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Server: '%s' picked up '%s'."),
		*GetNameSafe(this),
		*GetNameSafe(PickupActor)
	);

	return true;
}

bool AMouseMouseCharacter::TryDropHeldActor()
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!IsValid(HeldActor))
	{
		return false;
	}

	AMousePickupActor* PickupActor =
		Cast<AMousePickupActor>(HeldActor);

	if (!PickupActor)
	{
		return false;
	}

	// Save before clearing Character state
	AActor* ActorToDrop = HeldActor;

	// Character no longer holds anything
	HeldActor = nullptr;

	// Remove Gameplay / network ownership
	ActorToDrop->SetOwner(nullptr);

	// nullptr means "dropped".
	// MousePickupActor will detach itself,
	// enable collision and enable physics.
	PickupActor->SetHolder(nullptr);

	ForceNetUpdate();
	ActorToDrop->ForceNetUpdate();

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Server: '%s' dropped '%s'."),
		*GetNameSafe(this),
		*GetNameSafe(ActorToDrop)
	);

	return true;
}

void AMouseMouseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMouseMouseCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMouseMouseCharacter::DoJumpEnd);

		// Interacting
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AMouseMouseCharacter::InteractInput);
		}

		// Dropping held item
		if (DropAction)
		{
			EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &AMouseMouseCharacter::DropInput);
		}

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMouseMouseCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMouseMouseCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMouseMouseCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogMouseMouse, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AMouseMouseCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AMouseMouseCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AMouseMouseCharacter::InteractInput()
{
	if (!InteractionComponent)
	{
		return;
	}

	AActor* TargetActor = InteractionComponent->FindInteractable();

	if (!TargetActor)
	{
		return;
	}

	if (HasAuthority())
	{
		TryExecuteInteraction(TargetActor);
	}
	else
	{
		ServerInteract(TargetActor);
	}
}

void AMouseMouseCharacter::DropInput()
{
	if (HasAuthority())
	{
		TryDropHeldActor();
	}
	else
	{
		ServerDropHeldActor();
	}
}

void AMouseMouseCharacter::ServerInteract_Implementation(AActor* TargetActor)
{
	TryExecuteInteraction(TargetActor);
}

void AMouseMouseCharacter::ServerDropHeldActor_Implementation()
{
	TryDropHeldActor();
}

void AMouseMouseCharacter::TryExecuteInteraction(
	AActor* TargetActor)
{
	// 这个函数只允许服务器真正执行 Gameplay 交互
	if (!HasAuthority())
	{
		return;
	}

	if (!InteractionComponent)
	{
		return;
	}

	// 目标必须仍然存在
	if (!IsValid(TargetActor))
	{
		return;
	}

	// 目标必须真的实现 MouseInteractable
	if (!TargetActor->GetClass()->ImplementsInterface(
		UMouseInteractable::StaticClass()))
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	// 获取服务器认为的玩家视点
	FVector ViewLocation;
	FRotator ViewRotation;

	GetActorEyesViewPoint(
		ViewLocation,
		ViewRotation
	);

	// 给网络延迟和 Actor Pivot 留一点容差
	const float MaxInteractionDistance =
		InteractionComponent->GetInteractionDistance() + 100.0f;

	const float DistanceSquared =
		FVector::DistSquared(
			ViewLocation,
			TargetActor->GetActorLocation()
		);

	if (DistanceSquared >
		FMath::Square(MaxInteractionDistance))
	{
		UE_LOG(
			LogMouseMouse,
			Warning,
			TEXT("Server rejected interaction: target '%s' is too far away."),
			*GetNameSafe(TargetActor)
		);

		return;
	}

	// 再检查玩家和目标之间有没有墙或其他阻挡物
	FHitResult HitResult;

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(ServerInteractionTrace),
		false,
		this
	);

	const bool bHit =
		World->LineTraceSingleByChannel(
			HitResult,
			ViewLocation,
			TargetActor->GetActorLocation(),
			ECC_Visibility,
			QueryParams
		);

	if (!bHit || HitResult.GetActor() != TargetActor)
	{
		UE_LOG(
			LogMouseMouse,
			Warning,
			TEXT("Server rejected interaction: target '%s' is blocked."),
			*GetNameSafe(TargetActor)
		);

		return;
	}

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Server accepted interaction with: %s"),
		*GetNameSafe(TargetActor)
	);

	// 所有验证通过以后，才真正执行 Gameplay 交互
	IMouseInteractable::Execute_Interact(
		TargetActor,
		this
	);
}

void AMouseMouseCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMouseMouseCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AMouseMouseCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void AMouseMouseCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}
