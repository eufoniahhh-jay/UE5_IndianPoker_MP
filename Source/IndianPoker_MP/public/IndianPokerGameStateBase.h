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

	// 서버에서만 호출되도록 (실제 호출은 GameMode에서 할 것)
	void SetPhaseServer(EGamePhase NewPhase);

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

	UFUNCTION()
	void OnRep_CurrentPhase();
};
