// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Food/MouseFoodActor.h"

AMouseFoodActor::AMouseFoodActor()
{
}

bool AMouseFoodActor::IsAvailableForRat() const
{
	return GetHolder() == nullptr;
}

