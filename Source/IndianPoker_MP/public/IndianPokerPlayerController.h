// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IndianPokerPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class INDIANPOKER_MP_API AIndianPokerPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
};
