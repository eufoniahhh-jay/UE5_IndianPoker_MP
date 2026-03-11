// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "IndianPokerGameStateBase.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	Lobby       UMETA(DisplayName = "Lobby"),
	Deal        UMETA(DisplayName = "Deal"),
	Betting     UMETA(DisplayName = "Betting"),
	Showdown    UMETA(DisplayName = "Showdown"),
	RoundResult UMETA(DisplayName = "RoundResult"),
	MatchEnd    UMETA(DisplayName = "MatchEnd"),
};

UCLASS()
class INDIANPOKER_MP_API AIndianPokerGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AIndianPokerGameStateBase();

	UFUNCTION(BlueprintCallable, Category = "Phase")
	EGamePhase GetCurrentPhase() const { return CurrentPhase; }

	// Day9 추가 함수들
	UFUNCTION(BlueprintCallable, Category = "Round")
	int32 GetPot() const { return Pot; }

	UFUNCTION(BlueprintCallable, Category = "Round")
	int32 GetRoundBet() const { return RoundBet; }

	UFUNCTION(BlueprintCallable, Category = "Round")
	int32 GetCurrentActorPlayerId() const { return CurrentActorPlayerId; }

	UFUNCTION(BlueprintCallable, Category = "Round")
	int32 GetFirstActorPlayerId() const { return FirstActorPlayerId; }

	// 서버에서만 호출되도록 (실제 호출은 GameMode에서 할 것)
	void SetPhaseServer(EGamePhase NewPhase);
	// Day9
	void SetPotServer(int32 NewPot);
	void SetRoundBetServer(int32 NewRoundBet);
	void SetCurrentActorPlayerIdServer(int32 NewPlayerId);
	void SetFirstActorPlayerIdServer(int32 NewPlayerId);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 이렇게 바인딩해두면, 클라에서 해당 변수가 네트워크로 갱신(업뎃)되는 순간 
	// 자동으로 OnRep_TestNumber()를 호출
	//UPROPERTY(ReplicatedUsing = OnRep_TestNumber)
	//int32 TestNumber = 0;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase, BlueprintReadOnly, Category = "Phase")
	EGamePhase CurrentPhase = EGamePhase::Lobby;

	// Day9
	UPROPERTY(ReplicatedUsing = OnRep_Pot, BlueprintReadOnly, Category = "Round")
	int32 Pot = 0;

	UPROPERTY(ReplicatedUsing = OnRep_RoundBet, BlueprintReadOnly, Category = "Round")
	int32 RoundBet = 0;

	// Day9. 어느 플레이어 차례인지 공용 표시용
	UPROPERTY(ReplicatedUsing = OnRep_CurrentActorPlayerId, BlueprintReadOnly, Category = "Round")
	int32 CurrentActorPlayerId = -1;

	UPROPERTY(ReplicatedUsing = OnRep_FirstActorPlayerId, BlueprintReadOnly, Category = "Round")
	int32 FirstActorPlayerId = -1;

	UFUNCTION()
	void OnRep_CurrentPhase();
	// Day9
	UFUNCTION()
	void OnRep_Pot();

	UFUNCTION()
	void OnRep_RoundBet();

	UFUNCTION()
	void OnRep_CurrentActorPlayerId();

	UFUNCTION()
	void OnRep_FirstActorPlayerId();
};
