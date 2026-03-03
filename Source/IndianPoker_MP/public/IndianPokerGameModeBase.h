// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IndianPokerGameStateBase.h"
#include "IndianPokerGameModeBase.generated.h"

class AIndianPokerGameStateBase;

// GamePhase enum을 GameState 헤더에 정의할 예정이므로,
// 여기서는 GameState 헤더 include를 cpp에서 하고 forward 선언만 둠.
// enum class EGamePhase : uint8;

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

public:
	// Day5 테스트용: 서버에서만 Phase 진행
	UFUNCTION(BlueprintCallable, Category = "Phase")
	void AdvancePhaseServer();

	UFUNCTION(BlueprintCallable, Category = "Phase")
	void SetPhaseServer(EGamePhase NewPhase);

private:
	// 접속 순서 카운터 (지금은 꼭 필요하진 않지만, 조인 로그용으로 유지)
	int32 JoinCounter = 0;

	// 다음 Phase 계산(순환)
	EGamePhase GetNextPhase(EGamePhase Current) const;
};
