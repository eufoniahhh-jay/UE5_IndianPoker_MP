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

	void DebugPrintPlayerStates();

protected:
	virtual void SetupInputComponent() override;

	// 이렇게 해두면 엔진이 내부적으로 알아서...
	// Server_RequestIncrease()는 클라에서 호출
	// _Implementation()은 서버에서 실행
	UFUNCTION(Server, Reliable)
	void Server_RequestIncrease();
};
