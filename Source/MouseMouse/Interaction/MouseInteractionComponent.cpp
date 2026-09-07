// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/MouseInteractionComponent.h"

#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Interaction/MouseInteractable.h"
#include "MouseMouseCharacter.h"

// Sets default values for this component's properties
UMouseInteractionComponent::UMouseInteractionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

AActor* UMouseInteractionComponent::FindInteractable() const
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return nullptr;
	}

	// 1. InteractionComponent 的 Owner 应该是我们的玩家 Character
	const AMouseMouseCharacter* Character = Cast<AMouseMouseCharacter>(GetOwner());

	if (!Character)
	{
		return nullptr;
	}

	// 2. 获取第一人称摄像机
	const UCameraComponent* Camera = Character->GetFirstPersonCameraComponent();

	if (!Camera)
	{
		return nullptr;
	}

	// 3. 射线起点：摄像机当前位置
	const FVector TraceStart = Camera->GetComponentLocation();

	// 4. 射线终点：
	//    摄像机位置 + 摄像机前方向 * 最大交互距离
	const FVector TraceEnd =
		TraceStart +
		Camera->GetForwardVector() * InteractionDistance;

	// 5. 设置射线查询参数
	FCollisionQueryParams QueryParams;

	// 不允许射线击中玩家自己
	QueryParams.AddIgnoredActor(Character);

	// 6. 保存射线检测结果
	FHitResult HitResult;

	// 7. 向世界发射一条 Visibility 射线
	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	// 8. 开发阶段绘制 Debug 射线
	if (bDrawDebugTrace)
	{
		const FColor TraceColor = bHit ? FColor::Green : FColor::Red;

		DrawDebugLine(
			World,
			TraceStart,
			TraceEnd,
			TraceColor,
			false,
			1.0f,
			0,
			1.5f
		);
	}

	// 9. 没有撞到任何东西
	if (!bHit)
	{
		return nullptr;
	}

	// 10. 获取被射线击中的 Actor
	AActor* HitActor = HitResult.GetActor();

	if (!HitActor)
	{
		return nullptr;
	}

	// 11. 判断 Actor 是否实现了 MouseInteractable 接口
	if (!HitActor->GetClass()->ImplementsInterface(
		UMouseInteractable::StaticClass()))
	{
		return nullptr;
	}

	// 12. 找到了合法可交互 Actor
	return HitActor;
}