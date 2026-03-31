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

	/*Day5: PlayerState에서 별도 Replicated 변수 사용 안 하면
	GetLifetimeReplicatedProps 오버라이드 필요 없음.*/

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


public:
	// Day9. 카드/칩 최소 변수 추가
	//UPROPERTY(BlueprintReadOnly, Category = "Round")
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "IndianPoker|Betting")
	int32 Chips = 10;

	//UPROPERTY(Replicated, BlueprintReadOnly, Category = "IndianPoker|Betting") 
	UPROPERTY(BlueprintReadOnly, Category = "IndianPoker|Betting")
	int32 HiddenCardValue = -1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "IndianPoker|Betting")
	int32 VisibleOpponentCardValue = -1;

	// Day10. 
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "IndianPoker|Betting")
	bool bFolded = false;

	// Day12. 현재 라운드 누적 베팅액 변수
	// Replicated -> 서버가 값을 바꾸면 클라이언트에도 동기화되게 하기 위함
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "IndianPoker|Betting")
	int32 CurrentRoundContribution = 0;
};
