// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MouseInteractionComponent.generated.h"


UCLASS( ClassGroup=(MouseMouse), meta=(BlueprintSpawnableComponent) )
class MOUSEMOUSE_API UMouseInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMouseInteractionComponent();

	//如果找到实现 MouseInteractable 接口的 Actor，则返回该 Actor；
	AActor* FindInteractable() const;

	float GetInteractionDistance() const
	{
		return InteractionDistance;
	}



protected:
	/** 玩家最大交互距离，单位为厘米 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "50.0"))
	float InteractionDistance = 300.0f;

	/** 开发阶段是否绘制交互检测射线 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Debug")
	bool bDrawDebugTrace = true;
};
