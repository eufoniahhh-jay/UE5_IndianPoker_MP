// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "BettingTypes.h"
#include "IndianPokerPlayerController.generated.h"

/**
 * 
 */

class UInputMappingContext;
class UInputAction;

UCLASS()
class INDIANPOKER_MP_API AIndianPokerPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// 이렇게 해두면 엔진이 내부적으로 알아서...
	// Server_RequestIncrease()는 클라에서 호출
	// _Implementation()은 서버에서 실행
	// UFUNCTION(Server, Reliable)
	// void Server_RequestIncrease();

private:
	// Day5 테스트: Host(서버)만 Phase 진행
	void Input_AdvancePhase();

private:
	// IMC, IA 관련
	// IMC / IA 에셋을 에디터에서 지정
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* IMC_Player = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Look = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_AimLook = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Test = nullptr;

	bool bAimLookHeld = false;

	void OnAimLookStarted();
	void OnAimLookCompleted();

	void OnLookTriggered(const FInputActionValue& Value);
	void OnTestPressed();

private:
	// Day6 Session 테스트용 함수
	void TestHost();
	void TestFind();
	void TestJoin();
	void TestDestroy();

	UPROPERTY(EditDefaultsOnly, Category = "Input|Session")
	UInputAction* IA_SessionHost;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Session")
	UInputAction* IA_SessionFind;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Session")
	UInputAction* IA_SessionJoin;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Session")
	UInputAction* IA_SessionDestroy;

public:
	// Day8. BeginPlay -> HostSession()을 다음 틱으로 미뤄서 
	// 세션 생성 성공 시점에 서버의 실제 listen 포트가 아직 세션 정보에 제대로 반영되지 않을 수 있는 문제 해결 위함
	void HandleLobbyHostSetup();

public:
	// Day9. Client RPC 추가 (상대 카드 정보 확인)
	UPROPERTY(BlueprintReadOnly, Category = "Round")
	int32 ClientVisibleOpponentCardValue = -1;

	UFUNCTION(Client, Reliable)
	void ClientReceiveVisibleOpponentCard(int32 InCardValue);

public:
	//Day10. 클라이언트 입력으로 액션 요청
	UFUNCTION(Server, Reliable)
	void Server_RequestAction(EBettingActionType ActionType, int32 RaiseExtra);

	void RequestCheck();
	void RequestCheckCall();
	void RequestFold();

	UPROPERTY(EditDefaultsOnly, Category = "Input|Betting")
	UInputAction* IA_Check;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Betting")
	UInputAction* IA_CheckCall;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Betting")
	UInputAction* IA_Fold;
};
