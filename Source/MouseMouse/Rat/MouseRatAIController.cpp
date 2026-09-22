#include "Rat/MouseRatAIController.h"

#include "Engine/World.h"
#include "Items/Food/MouseFoodActor.h"
#include "Kismet/GameplayStatics.h"
#include "MouseMouse.h"
#include "MouseMouseCharacter.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Rat/MouseRatCharacter.h"
#include "Rat/MouseRatNest.h"
#include "TimerManager.h"


AMouseRatAIController::AMouseRatAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}


void AMouseRatAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!HasAuthority())
	{
		return;
	}

	FoodBehaviorState = ERatFoodBehaviorState::SeekingFood;
	CurrentFoodTarget = nullptr;
	CurrentHomeMoveTarget = nullptr;
	CurrentThreat = nullptr;
	CurrentFleePoint = FVector::ZeroVector;
	bHasCurrentFleePoint = false;
	NextFoodSearchTime = 0.0f;
	NextFoodMoveRetryTime = 0.0f;
	NextHomeNestSearchTime = 0.0f;
	NextHomeMoveRetryTime = 0.0f;
	ThreatSafeStartTime = -1.0f;
	NextFleeMoveRetryTime = 0.0f;
	NextFleeThreatRedirectTime = 0.0f;
	bHasLoggedMissingHome = false;
	bHasLoggedFoodMoveFailure = false;
	bHasLoggedHomeMoveFailure = false;
	bHasLoggedFleeMoveFailure = false;

	GetWorldTimerManager().SetTimer(
		FoodBehaviorTimerHandle,
		this,
		&AMouseRatAIController::UpdateFoodBehavior,
		BehaviorUpdateInterval,
		true
	);
}


void AMouseRatAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(
		FoodBehaviorTimerHandle
	);

	CurrentFoodTarget = nullptr;
	CurrentHomeMoveTarget = nullptr;
	CurrentThreat = nullptr;
	CurrentFleePoint = FVector::ZeroVector;
	bHasCurrentFleePoint = false;
	FoodBehaviorState = ERatFoodBehaviorState::SeekingFood;
	NextFoodSearchTime = 0.0f;
	NextFoodMoveRetryTime = 0.0f;
	NextHomeNestSearchTime = 0.0f;
	NextHomeMoveRetryTime = 0.0f;
	ThreatSafeStartTime = -1.0f;
	NextFleeMoveRetryTime = 0.0f;
	NextFleeThreatRedirectTime = 0.0f;
	bHasLoggedMissingHome = false;
	bHasLoggedFoodMoveFailure = false;
	bHasLoggedHomeMoveFailure = false;
	bHasLoggedFleeMoveFailure = false;

	Super::OnUnPossess();
}


float AMouseRatAIController::GetWorldTimeSeconds() const
{
	const UWorld* World = GetWorld();

	return World
		? World->GetTimeSeconds()
		: 0.0f;
}


AMouseMouseCharacter*
AMouseRatAIController::FindClosestValidPlayerCharacter(
	float SearchRadius
) const
{
	const APawn* ControlledPawn = GetPawn();

	if (!ControlledPawn ||
		SearchRadius <= 0.0f)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> PlayerActors;

	// TODO: When player/AI counts grow, replace this periodic global scan with
	// a player registration or spatial sensing mechanism.
	UGameplayStatics::GetAllActorsOfClass(
		World,
		AMouseMouseCharacter::StaticClass(),
		PlayerActors
	);

	AMouseMouseCharacter* ClosestPlayer = nullptr;
	float ClosestDistanceSquared =
		FMath::Square(SearchRadius);

	for (AActor* Actor : PlayerActors)
	{
		AMouseMouseCharacter* PlayerCharacter =
			Cast<AMouseMouseCharacter>(Actor);

		if (!IsValid(PlayerCharacter) ||
			!PlayerCharacter->IsPlayerControlled())
		{
			continue;
		}

		const float DistanceSquared =
			FVector::DistSquared(
				ControlledPawn->GetActorLocation(),
				PlayerCharacter->GetActorLocation()
			);

		if (DistanceSquared <= ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestPlayer = PlayerCharacter;
		}
	}

	return ClosestPlayer;
}


float AMouseRatAIController::GetEffectiveLoseThreatRadius(
	const AMouseRatCharacter* Rat
) const
{
	if (!IsValid(Rat))
	{
		return 0.0f;
	}

	return FMath::Max(
		Rat->GetLoseThreatRadius(),
		Rat->GetThreatDetectionRadius() +
			MinimumLoseThreatRadiusDifference
	);
}


bool AMouseRatAIController::UpdateThreatDetection(
	AMouseRatCharacter* Rat
)
{
	if (!HasAuthority() ||
		!IsValid(Rat))
	{
		return false;
	}

	if (!IsValid(CurrentThreat))
	{
		CurrentThreat = nullptr;
	}

	const float SearchRadius = IsValid(CurrentThreat)
		? GetEffectiveLoseThreatRadius(Rat)
		: Rat->GetThreatDetectionRadius();

	AMouseMouseCharacter* ClosestThreat =
		FindClosestValidPlayerCharacter(SearchRadius);

	if (IsValid(ClosestThreat))
	{
		const bool bDetectedNewThreat =
			!IsValid(CurrentThreat);

		CurrentThreat = ClosestThreat;
		ThreatSafeStartTime = -1.0f;

		if (bDetectedNewThreat)
		{
			UE_LOG(
				LogMouseMouse,
				Log,
				TEXT("Rat '%s' detected threat '%s'."),
				*GetNameSafe(Rat),
				*GetNameSafe(CurrentThreat)
			);
		}

		return true;
	}

	if (!IsValid(CurrentThreat))
	{
		ThreatSafeStartTime = -1.0f;

		return false;
	}

	const float CurrentTime = GetWorldTimeSeconds();

	if (ThreatSafeStartTime < 0.0f)
	{
		ThreatSafeStartTime = CurrentTime;
	}

	if (CurrentTime - ThreatSafeStartTime <
		FMath::Max(0.0f, Rat->GetSafeTimeBeforeResume()))
	{
		return true;
	}

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Rat '%s' lost threat '%s'."),
		*GetNameSafe(Rat),
		*GetNameSafe(CurrentThreat)
	);

	CurrentThreat = nullptr;
	ThreatSafeStartTime = -1.0f;

	return false;
}


void AMouseRatAIController::SetFoodBehaviorState(
	ERatFoodBehaviorState NewState
)
{
	FoodBehaviorState = NewState;
}


AMouseFoodActor*
AMouseRatAIController::FindClosestAvailableFood() const
{
	const APawn* ControlledPawn = GetPawn();

	if (!ControlledPawn)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> FoodActors;

	// TODO: Replace this global scan with a spatial registry when rat/food counts grow.
	UGameplayStatics::GetAllActorsOfClass(
		World,
		AMouseFoodActor::StaticClass(),
		FoodActors
	);

	AMouseFoodActor* ClosestFood = nullptr;

	float ClosestDistanceSquared =
		TNumericLimits<float>::Max();

	for (AActor* Actor : FoodActors)
	{
		AMouseFoodActor* Food =
			Cast<AMouseFoodActor>(Actor);

		if (!Food ||
			!Food->IsAvailableForRat())
		{
			continue;
		}

		const float DistanceSquared =
			FVector::DistSquared(
				ControlledPawn->GetActorLocation(),
				Food->GetActorLocation()
			);

		if (DistanceSquared <
			ClosestDistanceSquared)
		{
			ClosestDistanceSquared =
				DistanceSquared;

			ClosestFood = Food;
		}
	}

	return ClosestFood;
}


AMouseRatNest*
AMouseRatAIController::FindClosestRatNest() const
{
	const APawn* ControlledPawn = GetPawn();

	if (!ControlledPawn)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> NestActors;

	// TODO: Replace this global scan with a spatial registry when rat/nest counts grow.
	UGameplayStatics::GetAllActorsOfClass(
		World,
		AMouseRatNest::StaticClass(),
		NestActors
	);

	AMouseRatNest* ClosestNest = nullptr;

	float ClosestDistanceSquared =
		TNumericLimits<float>::Max();

	for (AActor* Actor : NestActors)
	{
		AMouseRatNest* Nest =
			Cast<AMouseRatNest>(Actor);

		if (!IsValid(Nest))
		{
			continue;
		}

		const float DistanceSquared =
			FVector::DistSquared(
				ControlledPawn->GetActorLocation(),
				Nest->GetDepositLocation()
			);

		if (DistanceSquared <
			ClosestDistanceSquared)
		{
			ClosestDistanceSquared =
				DistanceSquared;

			ClosestNest = Nest;
		}
	}

	return ClosestNest;
}


AMouseRatNest* AMouseRatAIController::ResolveHomeNest(
	AMouseRatCharacter* Rat
)
{
	if (!IsValid(Rat))
	{
		return nullptr;
	}

	AMouseRatNest* ExistingHomeNest =
		Rat->GetHomeNest();

	if (IsValid(ExistingHomeNest))
	{
		bHasLoggedMissingHome = false;

		return ExistingHomeNest;
	}

	if (ExistingHomeNest)
	{
		Rat->SetHomeNest(nullptr);
	}

	const float CurrentTime = GetWorldTimeSeconds();

	if (CurrentTime < NextHomeNestSearchTime)
	{
		return nullptr;
	}

	AMouseRatNest* ClosestNest = FindClosestRatNest();

	if (!IsValid(ClosestNest))
	{
		NextHomeNestSearchTime =
			CurrentTime + HomeNestSearchRetryInterval;

		if (!bHasLoggedMissingHome)
		{
			UE_LOG(
				LogMouseMouse,
				Warning,
				TEXT("Rat '%s' could not find a home nest; it will keep its food and retry."),
				*GetNameSafe(Rat)
			);

			bHasLoggedMissingHome = true;
		}

		return nullptr;
	}

	Rat->SetHomeNest(ClosestNest);

	NextHomeNestSearchTime = 0.0f;
	NextHomeMoveRetryTime = 0.0f;
	bHasLoggedMissingHome = false;

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Rat '%s' assigned nearest home nest '%s'."),
		*GetNameSafe(Rat),
		*GetNameSafe(ClosestNest)
	);

	return ClosestNest;
}


bool AMouseRatAIController::RequestMoveToFood(
	AMouseFoodActor* Food
)
{
	if (!IsValid(Food))
	{
		return false;
	}

	// Keep navigation's 2D reach test conservative. Adding overlap radii can
	// report arrival before the 3D capsule-to-food gameplay reach is valid.
	const EPathFollowingRequestResult::Type MoveResult =
		MoveToActor(
			Food,
			AMouseRatCharacter::PickupReach,
			false,
			true,
			true
		);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		if (!bHasLoggedFoodMoveFailure)
		{
			UE_LOG(
				LogMouseMouse,
				Warning,
				TEXT("Rat '%s' failed to move to food '%s'."),
				*GetNameSafe(GetPawn()),
				*GetNameSafe(Food)
			);

			bHasLoggedFoodMoveFailure = true;
		}

		return false;
	}

	bHasLoggedFoodMoveFailure = false;

	return true;
}


bool AMouseRatAIController::RequestMoveToHome(
	AMouseRatNest* HomeNest
)
{
	if (!IsValid(HomeNest))
	{
		return false;
	}

	// Deposit uses the same conservative rule as food so a completed path is
	// always close enough for the authoritative 3D capsule reach check.
	const EPathFollowingRequestResult::Type MoveResult =
		MoveToLocation(
			HomeNest->GetDepositLocation(),
			AMouseRatNest::DepositDistance,
			false,
			true,
			true,
			true,
			nullptr,
			true
		);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		if (!bHasLoggedHomeMoveFailure)
		{
			UE_LOG(
				LogMouseMouse,
				Warning,
				TEXT("Rat '%s' failed to move to home nest '%s'."),
				*GetNameSafe(GetPawn()),
				*GetNameSafe(HomeNest)
			);

			bHasLoggedHomeMoveFailure = true;
		}

		return false;
	}

	bHasLoggedHomeMoveFailure = false;

	return true;
}


bool AMouseRatAIController::RequestMoveToFleePoint(
	const FVector& FleePoint
)
{
	const EPathFollowingRequestResult::Type MoveResult =
		MoveToLocation(
			FleePoint,
			FleePointAcceptanceRadius,
			false,
			true,
			true,
			true,
			nullptr,
			false
		);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		if (!bHasLoggedFleeMoveFailure)
		{
			UE_LOG(
				LogMouseMouse,
				Warning,
				TEXT("Rat '%s' Flee MoveTo failed for target (%.0f, %.0f, %.0f)."),
				*GetNameSafe(GetPawn()),
				FleePoint.X,
				FleePoint.Y,
				FleePoint.Z
			);

			bHasLoggedFleeMoveFailure = true;
		}

		return false;
	}

	bHasLoggedFleeMoveFailure = false;

	return true;
}


bool AMouseRatAIController::SetFoodTarget(
	AMouseFoodActor* NewTarget
)
{
	CurrentFoodTarget = NewTarget;

	if (!IsValid(CurrentFoodTarget) ||
		!CurrentFoodTarget->IsAvailableForRat())
	{
		CurrentFoodTarget = nullptr;
		StopMovement();

		return false;
	}

	if (!RequestMoveToFood(CurrentFoodTarget))
	{
		CurrentFoodTarget = nullptr;
		StopMovement();

		return false;
	}

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Rat '%s' selected food '%s'."),
		*GetNameSafe(GetPawn()),
		*GetNameSafe(CurrentFoodTarget)
	);

	NextFoodMoveRetryTime =
		GetWorldTimeSeconds() + MoveRetryInterval;

	return true;
}


void AMouseRatAIController::EnterFleeing()
{
	CurrentFoodTarget = nullptr;
	CurrentHomeMoveTarget = nullptr;
	CurrentFleePoint = FVector::ZeroVector;
	bHasCurrentFleePoint = false;
	NextFleeMoveRetryTime = 0.0f;
	NextFleeThreatRedirectTime = 0.0f;

	StopMovement();

	SetFoodBehaviorState(
		ERatFoodBehaviorState::Fleeing
	);

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Rat '%s' entered Fleeing."),
		*GetNameSafe(GetPawn())
	);
}


bool AMouseRatAIController::FindFleePoint(
	AMouseRatCharacter* Rat,
	FVector& OutFleePoint
) const
{
	if (!IsValid(Rat) ||
		!IsValid(CurrentThreat))
	{
		return false;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	UNavigationSystemV1* NavigationSystem =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

	if (!NavigationSystem)
	{
		return false;
	}

	const FVector RatLocation = Rat->GetActorLocation();
	const FVector ThreatLocation =
		CurrentThreat->GetActorLocation();

	FVector AwayDirection = RatLocation - ThreatLocation;
	AwayDirection.Z = 0.0f;
	AwayDirection = AwayDirection.GetSafeNormal();

	if (AwayDirection.IsNearlyZero())
	{
		AwayDirection = Rat->GetActorForwardVector().GetSafeNormal2D();
	}

	if (AwayDirection.IsNearlyZero())
	{
		AwayDirection = FVector::ForwardVector;
	}

	const FVector SideDirection =
		FVector::CrossProduct(
			FVector::UpVector,
			AwayDirection
		).GetSafeNormal2D();

	TArray<FVector, TInlineAllocator<3>> CandidateDirections;
	CandidateDirections.Add(AwayDirection);
	CandidateDirections.Add(
		(AwayDirection + SideDirection).GetSafeNormal2D()
	);
	CandidateDirections.Add(
		(AwayDirection - SideDirection).GetSafeNormal2D()
	);

	const float FleeDistance = FMath::Max(
		0.0f,
		Rat->GetFleeDistance()
	);

	if (FleeDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector ProjectionExtent(
		FleeProjectionExtentXY,
		FleeProjectionExtentXY,
		FleeProjectionExtentZ
	);

	const float CurrentThreatDistanceSquared =
		FVector::DistSquared2D(
			RatLocation,
			ThreatLocation
		);

	bool bFoundFleePoint = false;
	bool bFoundPointFurtherFromThreat = false;
	float BestThreatDistanceSquared = 0.0f;

	for (const FVector& CandidateDirection : CandidateDirections)
	{
		const FVector CandidateLocation =
			RatLocation + CandidateDirection * FleeDistance;

		FNavLocation ProjectedLocation;

		if (!NavigationSystem->ProjectPointToNavigation(
			CandidateLocation,
			ProjectedLocation,
			ProjectionExtent
		))
		{
			continue;
		}

		UNavigationPath* NavigationPath =
			UNavigationSystemV1::FindPathToLocationSynchronously(
				World,
				RatLocation,
				ProjectedLocation.Location,
				Rat
			);

		if (!IsValid(NavigationPath) ||
			!NavigationPath->IsValid() ||
			NavigationPath->IsPartial())
		{
			continue;
		}

		const float CandidateThreatDistanceSquared =
			FVector::DistSquared2D(
				ProjectedLocation.Location,
				ThreatLocation
			);

		const bool bCandidateIsFurtherFromThreat =
			CandidateThreatDistanceSquared >
			CurrentThreatDistanceSquared;

		if (!bFoundFleePoint ||
			(bCandidateIsFurtherFromThreat &&
				!bFoundPointFurtherFromThreat) ||
			(bCandidateIsFurtherFromThreat ==
				bFoundPointFurtherFromThreat &&
				CandidateThreatDistanceSquared >
					BestThreatDistanceSquared))
		{
			OutFleePoint = ProjectedLocation.Location;
			BestThreatDistanceSquared =
				CandidateThreatDistanceSquared;
			bFoundFleePoint = true;
			bFoundPointFurtherFromThreat =
				bCandidateIsFurtherFromThreat;
		}
	}

	return bFoundFleePoint;
}


bool AMouseRatAIController::DoesCurrentFleePointLeadAwayFromThreat(
	const AMouseRatCharacter* Rat
) const
{
	if (!IsValid(Rat) ||
		!IsValid(CurrentThreat) ||
		!bHasCurrentFleePoint)
	{
		return false;
	}

	FVector AwayDirection =
		Rat->GetActorLocation() -
		CurrentThreat->GetActorLocation();
	AwayDirection.Z = 0.0f;
	AwayDirection = AwayDirection.GetSafeNormal();

	if (AwayDirection.IsNearlyZero())
	{
		AwayDirection = Rat->GetActorForwardVector().GetSafeNormal2D();
	}

	const FVector FleePointDirection =
		(CurrentFleePoint - Rat->GetActorLocation()).GetSafeNormal2D();

	if (AwayDirection.IsNearlyZero() ||
		FleePointDirection.IsNearlyZero())
	{
		return false;
	}

	return FVector::DotProduct(
		AwayDirection,
		FleePointDirection
	) > FleePointAwayDotThreshold;
}


bool AMouseRatAIController::SelectAndMoveToFleePoint(
	AMouseRatCharacter* Rat
)
{
	FVector NewFleePoint;

	if (!FindFleePoint(Rat, NewFleePoint))
	{
		CurrentFleePoint = FVector::ZeroVector;
		bHasCurrentFleePoint = false;

		if (!bHasLoggedFleeMoveFailure)
		{
			UE_LOG(
				LogMouseMouse,
				Warning,
				TEXT("Rat '%s' Flee MoveTo failed: no reachable NavMesh flee point."),
				*GetNameSafe(Rat)
			);

			bHasLoggedFleeMoveFailure = true;
		}

		return false;
	}

	StopMovement();

	if (!RequestMoveToFleePoint(NewFleePoint))
	{
		CurrentFleePoint = FVector::ZeroVector;
		bHasCurrentFleePoint = false;

		return false;
	}

	CurrentFleePoint = NewFleePoint;
	bHasCurrentFleePoint = true;

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Rat '%s' selected new flee target (%.0f, %.0f, %.0f)."),
		*GetNameSafe(Rat),
		CurrentFleePoint.X,
		CurrentFleePoint.Y,
		CurrentFleePoint.Z
	);

	return true;
}


void AMouseRatAIController::UpdateFleeing(
	AMouseRatCharacter* Rat
)
{
	if (!IsValid(Rat) ||
		!IsValid(CurrentThreat))
	{
		return;
	}

	// UpdateThreatDetection deliberately keeps CurrentThreat during the safe
	// delay after it leaves the loss radius. Continue choosing flee points for
	// that period so reaching the previous point never turns Fleeing into a
	// visible two-second pause.

	const float CurrentTime = GetWorldTimeSeconds();
	const bool bReachedCurrentFleePoint =
		bHasCurrentFleePoint &&
		FVector::DistSquared2D(
			Rat->GetActorLocation(),
			CurrentFleePoint
		) <= FMath::Square(FleePointReachedDistance);

	const bool bFleeMoveIsIdle =
		GetMoveStatus() == EPathFollowingStatus::Idle;

	const bool bNeedsThreatRedirect =
		bHasCurrentFleePoint &&
		!DoesCurrentFleePointLeadAwayFromThreat(Rat);

	if (!bHasCurrentFleePoint ||
		bReachedCurrentFleePoint ||
		bFleeMoveIsIdle ||
		bNeedsThreatRedirect)
	{
		if (bNeedsThreatRedirect)
		{
			if (CurrentTime < NextFleeThreatRedirectTime)
			{
				return;
			}

			NextFleeThreatRedirectTime =
				CurrentTime + FleeThreatRedirectInterval;
		}
		else if (CurrentTime < NextFleeMoveRetryTime)
		{
			return;
		}

		SelectAndMoveToFleePoint(Rat);

		NextFleeMoveRetryTime =
			CurrentTime + MoveRetryInterval;
	}
}


void AMouseRatAIController::ResumeFromFleeing(
	AMouseRatCharacter* Rat
)
{
	if (!IsValid(Rat))
	{
		return;
	}

	StopMovement();

	CurrentFoodTarget = nullptr;
	CurrentHomeMoveTarget = nullptr;
	CurrentFleePoint = FVector::ZeroVector;
	bHasCurrentFleePoint = false;
	NextFleeMoveRetryTime = 0.0f;
	NextFleeThreatRedirectTime = 0.0f;
	bHasLoggedFleeMoveFailure = false;

	if (IsValid(Rat->GetCarriedFood()))
	{
		NextHomeMoveRetryTime = 0.0f;

		SetFoodBehaviorState(
			ERatFoodBehaviorState::ReturningHome
		);

		UE_LOG(
			LogMouseMouse,
			Log,
			TEXT("Rat '%s' resumed ReturningHome after fleeing."),
			*GetNameSafe(Rat)
		);

		return;
	}

	NextFoodSearchTime = GetWorldTimeSeconds();
	NextFoodMoveRetryTime = 0.0f;

	SetFoodBehaviorState(
		ERatFoodBehaviorState::SeekingFood
	);

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Rat '%s' resumed SeekingFood after fleeing."),
		*GetNameSafe(Rat)
	);
}


void AMouseRatAIController::StartReturningHome()
{
	CurrentFoodTarget = nullptr;
	CurrentHomeMoveTarget = nullptr;
	NextHomeMoveRetryTime = 0.0f;

	StopMovement();

	SetFoodBehaviorState(
		ERatFoodBehaviorState::ReturningHome
	);

	UE_LOG(
		LogMouseMouse,
		Log,
		TEXT("Rat '%s' is returning home with food."),
		*GetNameSafe(GetPawn())
	);
}


void AMouseRatAIController::UpdateSeekingFood(
	AMouseRatCharacter* Rat
)
{
	if (!IsValid(Rat))
	{
		return;
	}

	const float CurrentTime = GetWorldTimeSeconds();

	if (CurrentTime < NextFoodSearchTime)
	{
		return;
	}

	AMouseFoodActor* NewTarget =
		FindClosestAvailableFood();

	if (!IsValid(NewTarget))
	{
		NextFoodSearchTime =
			CurrentTime + FoodSearchRetryInterval;

		StopMovement();

		return;
	}

	if (SetFoodTarget(NewTarget))
	{
		SetFoodBehaviorState(
			ERatFoodBehaviorState::MovingToFood
		);

		return;
	}

	NextFoodSearchTime =
		CurrentTime + FoodSearchRetryInterval;
}


void AMouseRatAIController::UpdateMovingToFood(
	AMouseRatCharacter* Rat
)
{
	if (!IsValid(Rat))
	{
		return;
	}

	if (!IsValid(CurrentFoodTarget) ||
		!CurrentFoodTarget->IsAvailableForRat())
	{
		CurrentFoodTarget = nullptr;
		StopMovement();
		NextFoodSearchTime = GetWorldTimeSeconds();

		SetFoodBehaviorState(
			ERatFoodBehaviorState::SeekingFood
		);

		return;
	}

	if (Rat->IsFoodWithinPickupRange(CurrentFoodTarget))
	{
		AMouseFoodActor* FoodToPickup =
			CurrentFoodTarget;

		CurrentFoodTarget = nullptr;

		StopMovement();

		if (Rat->TryPickupFood(FoodToPickup))
		{
			StartReturningHome();
		}
		else
		{
			NextFoodSearchTime = GetWorldTimeSeconds();

			SetFoodBehaviorState(
				ERatFoodBehaviorState::SeekingFood
			);
		}

		return;
	}

	const float CurrentTime = GetWorldTimeSeconds();

	// A completed/failed asynchronous request can leave the controller idle
	// while still outside pickup range. Retry at a low frequency instead of locking.
	if (GetMoveStatus() == EPathFollowingStatus::Idle &&
		CurrentTime >= NextFoodMoveRetryTime)
	{
		if (RequestMoveToFood(CurrentFoodTarget))
		{
			NextFoodMoveRetryTime =
				CurrentTime + MoveRetryInterval;
		}
		else
		{
			CurrentFoodTarget = nullptr;
			StopMovement();
			NextFoodSearchTime =
				CurrentTime + FoodSearchRetryInterval;

			SetFoodBehaviorState(
				ERatFoodBehaviorState::SeekingFood
			);
		}
	}
}


void AMouseRatAIController::UpdateReturningHome(
	AMouseRatCharacter* Rat
)
{
	if (!IsValid(Rat))
	{
		return;
	}

	const float CurrentTime = GetWorldTimeSeconds();

	if (!IsValid(Rat->GetCarriedFood()))
	{
		CurrentHomeMoveTarget = nullptr;
		StopMovement();
		NextFoodSearchTime = CurrentTime;

		SetFoodBehaviorState(
			ERatFoodBehaviorState::SeekingFood
		);

		return;
	}

	AMouseRatNest* HomeNest = ResolveHomeNest(Rat);

	if (!IsValid(HomeNest))
	{
		StopMovement();

		return;
	}

	if (Rat->IsWithinReachOfPoint(
		HomeNest->GetDepositLocation(),
		AMouseRatNest::DepositDistance
	))
	{
		StopMovement();

		if (Rat->TryDepositCarriedFood())
		{
			CurrentHomeMoveTarget = nullptr;
			NextFoodSearchTime = CurrentTime;

			SetFoodBehaviorState(
				ERatFoodBehaviorState::SeekingFood
			);
		}

		return;
	}

	if (CurrentHomeMoveTarget.Get() != HomeNest)
	{
		if (CurrentTime < NextHomeMoveRetryTime)
		{
			return;
		}

		CurrentHomeMoveTarget = HomeNest;

		if (RequestMoveToHome(HomeNest))
		{
			NextHomeMoveRetryTime =
				CurrentTime + MoveRetryInterval;
		}
		else
		{
			CurrentHomeMoveTarget = nullptr;
			NextHomeMoveRetryTime =
				CurrentTime + MoveRetryInterval;
		}

		return;
	}

	// As with food, recover from an asynchronous path failure without losing food.
	if (GetMoveStatus() == EPathFollowingStatus::Idle &&
		CurrentTime >= NextHomeMoveRetryTime)
	{
		if (RequestMoveToHome(HomeNest))
		{
			NextHomeMoveRetryTime =
				CurrentTime + MoveRetryInterval;
		}
		else
		{
			CurrentHomeMoveTarget = nullptr;
			NextHomeMoveRetryTime =
				CurrentTime + MoveRetryInterval;
		}
	}
}


void AMouseRatAIController::UpdateFoodBehavior()
{
	if (!HasAuthority())
	{
		return;
	}

	AMouseRatCharacter* Rat =
		Cast<AMouseRatCharacter>(GetPawn());

	if (!Rat)
	{
		return;
	}

	if (UpdateThreatDetection(Rat))
	{
		if (FoodBehaviorState !=
			ERatFoodBehaviorState::Fleeing)
		{
			EnterFleeing();
		}

		UpdateFleeing(Rat);

		return;
	}

	if (FoodBehaviorState ==
		ERatFoodBehaviorState::Fleeing)
	{
		ResumeFromFleeing(Rat);
	}

	if (IsValid(Rat->GetCarriedFood()))
	{
		if (FoodBehaviorState !=
			ERatFoodBehaviorState::ReturningHome)
		{
			StartReturningHome();
		}

		UpdateReturningHome(Rat);

		return;
	}

	if (FoodBehaviorState ==
		ERatFoodBehaviorState::ReturningHome)
	{
		CurrentHomeMoveTarget = nullptr;
		StopMovement();
		NextFoodSearchTime = GetWorldTimeSeconds();

		SetFoodBehaviorState(
			ERatFoodBehaviorState::SeekingFood
		);
	}

	switch (FoodBehaviorState)
	{
	case ERatFoodBehaviorState::SeekingFood:
		UpdateSeekingFood(Rat);
		break;

	case ERatFoodBehaviorState::MovingToFood:
		UpdateMovingToFood(Rat);
		break;

	case ERatFoodBehaviorState::ReturningHome:
		// Handled above when a carried food is present.
		break;

	case ERatFoodBehaviorState::Fleeing:
		// A lost threat is normally resumed above before this switch.
		ResumeFromFleeing(Rat);
		break;

	default:
		SetFoodBehaviorState(
			ERatFoodBehaviorState::SeekingFood
		);
		break;
	}
}
