// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "IndianPokerGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class INDIANPOKER_MP_API AIndianPokerGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AIndianPokerGameStateBase();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버에서 1초마다 호출될 함수
	void ServerTickTestNumber();

	// 이렇게 바인딩해두면, 클라에서 해당 변수가 네트워크로 갱신(업뎃)되는 순간 
	// 자동으로 OnRep_TestNumber()를 호출
	UPROPERTY(ReplicatedUsing = OnRep_TestNumber)
	int32 TestNumber = 0;

	UFUNCTION()
	void OnRep_TestNumber();

private:
	FTimerHandle TestTimerHandle;

protected:
	// PlayerState 값 세팅 
	//void ServerInitPlayerStateTestValues();
};
