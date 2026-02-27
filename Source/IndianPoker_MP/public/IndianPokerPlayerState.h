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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버에서만 값 변경용 함수
	void ServerSetTestValue(int32 NewValue);

protected:
	// 이렇게 바인딩해두면, 클라에서 해당 변수가 네트워크로 갱신(업뎃)되는 순간 
	// 자동으로 OnRep_TestValue()를 호출
	UPROPERTY(ReplicatedUsing = OnRep_TestValue)
	int32 TestValue = 0;

	UFUNCTION()
	void OnRep_TestValue();

public:
	// PlayerController에서 PlayerState 디버깅 시 사용
	int32 GetTestValue() const { return TestValue; }
};
