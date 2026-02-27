// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerGameModeBase.h"
#include "IndianPoker_MP.h"
#include "IndianPokerPlayerState.h"
#include "GameFramework/PlayerController.h"

AIndianPokerGameModeBase::AIndianPokerGameModeBase()
{
	//UE_LOG(LogTemp, Warning, TEXT("%s"), *CALLINFO);
	PRINT_LOG(TEXT("My Log : %s"), TEXT("Indian Poker Project!!!"));
}

void AIndianPokerGameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // GameMode는 서버에만 존재하므로 기본적으로 서버 로직임
    UE_LOG(LogTemp, Warning, TEXT("[GM][PostLogin] Player joined. PC=%s"), *GetNameSafe(NewPlayer));

    if (!NewPlayer)
        return;

    AIndianPokerPlayerState* PS = Cast<AIndianPokerPlayerState>(NewPlayer->PlayerState);
    if (!PS)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GM][PostLogin] PlayerState is not AIndianPokerPlayerState"));
        return;
    }

    // 플레이어마다 다른 값 주기(테스트용)
    // (1) 간단한 방식: 접속 순서 카운터
    static int32 JoinCounter = 0;
    JoinCounter++;

    const int32 Value = JoinCounter * 100; // 100, 200, 300...
    PS->ServerSetTestValue(Value);

    UE_LOG(LogTemp, Warning, TEXT("[GM][PostLogin] Set %s => %d"), *GetNameSafe(PS), Value);
}
