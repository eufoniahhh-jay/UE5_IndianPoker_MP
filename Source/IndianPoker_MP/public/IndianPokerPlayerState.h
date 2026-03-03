// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "IndianPokerPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class INDIANPOKER_MP_API AIndianPokerPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	AIndianPokerPlayerState() = default;

	// Day5: PlayerState에서 별도 Replicated 변수 사용 안 하면
	// GetLifetimeReplicatedProps 오버라이드 필요 없음.
};
