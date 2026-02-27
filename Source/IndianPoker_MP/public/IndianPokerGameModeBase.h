// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IndianPokerGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class INDIANPOKER_MP_API AIndianPokerGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AIndianPokerGameModeBase();

protected:
	// GameState는“상태 보관”이 역할이고,
	// 플레이어가 들어왔다 이벤트는 GameMode가 책임
	virtual void PostLogin(APlayerController* NewPlayer) override;
};
