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
	//AIndianPokerPlayerState() = default;
	AIndianPokerPlayerState();

	// Day5: PlayerState에서 별도 Replicated 변수 사용 안 하면
	// GetLifetimeReplicatedProps 오버라이드 필요 없음.

public:
	// Day9. 카드/칩 최소 변수 추가
	UPROPERTY(BlueprintReadOnly, Category = "Round")
	int32 Chips = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Round")
	int32 HiddenCardValue = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Round")
	int32 VisibleOpponentCardValue = -1;

public:
	// Day10. 
	bool bFolded = false;
};
