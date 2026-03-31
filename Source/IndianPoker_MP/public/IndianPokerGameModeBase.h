// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IndianPokerGameStateBase.h"
#include "BettingTypes.h"
#include "IndianPokerGameModeBase.generated.h"

class AIndianPokerGameStateBase;
class AIndianPokerPlayerState;
class AIndianPokerPlayerController;
class ACardActor;

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
	virtual void BeginPlay() override;

	UFUNCTION()
	void DelayedTryStartRound();

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

	// Day9. 라운드 시작 용도
	UFUNCTION(BlueprintCallable, Category = "Round")
	void TryStartRound();		// 바로 StartRound() 하지 말고, 먼저 검사하는 관문

	UFUNCTION(BlueprintCallable, Category = "Round")
	void StartRound();

	// Day13. 선공 선택용 변수 (선공은 매 라운드 번갈아가며)
	bool bNextRoundStartsWithP1 = true;

protected:
	// Day9
	// 라운드 흐름 함수
	void GenerateDeck();
	void ShuffleDeck();
	void DealCards();
	void InitBettingState();
	void SetVisibleOpponentCards();					// 상대방 카드 정보 설정
	void SendVisibleOpponentCardsToClients();		// 플레이어에게 보내는 함수
	void ApplyAnte();

	// 유틸
	bool CanStartRound();
	AIndianPokerGameStateBase* GetIndianPokerGameState();
	void SyncRoundStateToGameState();

private:
	// Day4? 5?
	// 접속 순서 카운터 (지금은 꼭 필요하진 않지만, 조인 로그용으로 유지)
	int32 JoinCounter = 0;

	// 다음 Phase 계산(순환)
	EGamePhase GetNextPhase(EGamePhase Current);

protected:
	// Day9. 서버 내부 라운드 상태
	UPROPERTY()
	TArray<int32> Deck;

	UPROPERTY()
	TArray<int32> Discard;

	UPROPERTY()
	int32 Pot = 0;

	UPROPERTY()
	int32 RoundBet = 0;

	UPROPERTY()
	int32 RequiredToCall = 0;

	// Day14. HUD
	UPROPERTY()
	int32 CurrentRoundNumber = 0;

	UPROPERTY()
	FString LastActionText = TEXT("");

	// 지금은 단순히 포인터로 시작(Day9)
	/*UPROPERTY()
	AIndianPokerPlayerState* FirstActorPS = nullptr;

	UPROPERTY()
	AIndianPokerPlayerState* CurrentActorPS = nullptr;*/

	// Day11. PS 포인터 대신 플레이어ID 기준으로 관리하도록 변경 
	// (그래도 여전히 플레이어데이터는 PS에. PID기준으로 PS찾도록 하는 것)
	UPROPERTY()
	int32 AuthFirstActorPlayerId = INDEX_NONE;

	UPROPERTY()
	int32 AuthCurrentActorPlayerId = INDEX_NONE;

	// Day11. 그리고 그냥 authoritative로 PlayerState를 들고있기 (라운드 시작시 2명을 캐시하기)
	UPROPERTY()
	TObjectPtr<AIndianPokerPlayerState> RoundP1PS = nullptr;

	UPROPERTY()
	TObjectPtr<AIndianPokerPlayerState> RoundP2PS = nullptr; 

public:
	// Day10. 
	bool bRoundEnded = false;
	bool bHasOpeningCheck = false;
	// 베팅 액션의 메인 진입점
	void HandlePlayerAction(AIndianPokerPlayerController* RequestingPC, EBettingActionType ActionType, int32 RaiseExtra);

	// 공통 검증 함수 (룰 파단)
	bool ValidateActionRequest(
		AIndianPokerPlayerController* RequestingPC,
		AIndianPokerPlayerState*& OutRequestingPS,
		AIndianPokerPlayerState*& OutOpponentPS
	);

	// Check 액션 함수
	bool HandleAction_Check(
		AIndianPokerPlayerState* RequestingPS,
		AIndianPokerPlayerState* OpponentPS
	);

	// CheckCall 액션 함수
	bool HandleAction_CheckCall(
		AIndianPokerPlayerState* RequestingPS,
		AIndianPokerPlayerState* OpponentPS
	);

	// Fold 액션 함수
	bool HandleAction_Fold(
		AIndianPokerPlayerState* RequestingPS,
		AIndianPokerPlayerState* OpponentPS
	);

	// Day12. Call / Raise 액션 함수
	bool HandleAction_Call(
		AIndianPokerPlayerState* RequestingPS, 
		AIndianPokerPlayerState* OpponentPS
	);

	bool HandleAction_Raise(
		AIndianPokerPlayerState* RequestingPS,
		AIndianPokerPlayerState* OpponentPS,
		int32 RaiseExtra);

	// Fold 후 정산 함수
	void ResolveFoldRound(
		AIndianPokerPlayerState* FolderPS,
		AIndianPokerPlayerState* WinnerPS
	);

	// Day13. Showdown 결과 처리 함수
	void ResolveShowdown();

	// Day13. 라운드 종료 공통 처리 함수 (전체흐름제어/종료판정담당/종료처리담당) 3가지 
	void AdvanceAfterRound(float delay);
	bool IsMatchEnded();
	void HandleMatchEnd();
	FTimerHandle NextRoundTimerHandle;

public:
	// Day11. 누구인지 기억하는 건 ID, 실제 상태가 필요할 때만 헬퍼로 PlayerState를 찾는다
	// 헬퍼 함수
	bool GetRoundPlayerStates(
		AIndianPokerPlayerState*& OutP1,
		AIndianPokerPlayerState*& OutP2
	) const;

	AIndianPokerPlayerState* FindRoundPlayerStateById(int32 PlayerId);

	// 시작시 최초 참가자 캐시하기(tryStartRound 내에서 한 번만)
	bool EnsureMatchPlayersCached();

	// 라운드 시작시 참가자 캐시하기
	bool GetCachedRoundPlayers(
		AIndianPokerPlayerState*& OutP1,
		AIndianPokerPlayerState*& OutP2
	);

	// Day18. 실제 월드의 PlayerController를 기준으로 준비된 플레이어 2명을 모으는 헬퍼 함수
	bool GatherReadyMatchPlayersFromControllers(
		AIndianPokerPlayerState*& OutP1,
		AIndianPokerPlayerState*& OutP2
	);

	//  PlayerState로 현재 playerController를 다시 찾는 방식
	AIndianPokerPlayerController* FindControllerByPlayerState(AIndianPokerPlayerState* TargetPS) const;

protected:
	// Day12. RequiredToCall 계산용 헬퍼 3개
	// PlayerId로 내 PlayerState 찾기 / PlayerId로 상대 PlayerState 찾기/ 두 기여량 차이로 RequiredToCall 계산하기
	AIndianPokerPlayerState* GetPlayerStateByPlayerId(int32 PlayerId) const;
	AIndianPokerPlayerState* GetOpponentPlayerStateByPlayerId(int32 PlayerId) const;
	int32 CalcRequiredToCall(int32 RequestPlayerId) const;

protected:
	// Day16. 카드액터 참조 변수
	UPROPERTY()
	ACardActor* P1WorldCard = nullptr;

	UPROPERTY()
	ACardActor* P2WorldCard = nullptr;

	void CacheWorldCardActors();

	// 텍스처 매핑. CardFrontTextures[0] = 카드 1
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<TObjectPtr<UTexture2D>> CardFrontTextures;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UTexture2D> CardBackTexture = nullptr;

	// 카드 값에 맞는 텍스쳐 가져오는 함수
	UTexture2D* GetFrontTextureForCardValue(int32 CardValue) const;

	// 실제 월드 갱신 함수
	void UpdateWorldCardVisuals();
};
