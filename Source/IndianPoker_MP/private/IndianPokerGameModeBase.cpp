// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerGameModeBase.h"
#include "IndianPoker_MP.h"
#include "IndianPokerPlayerState.h"
#include "IndianPokerGameStateBase.h"
#include "GameFramework/PlayerController.h"

AIndianPokerGameModeBase::AIndianPokerGameModeBase()
{

}

void AIndianPokerGameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // GameMode는 서버에만 존재 → 조인 로그는 여기 하나면 충분
    JoinCounter++;

    // GameMode는 서버에만 존재하므로 기본적으로 서버 로직임
    //UE_LOG(LogTemp, Warning, TEXT("[GM][PostLogin] Player joined. PC=%s"), *GetNameSafe(NewPlayer));
    UE_LOG(LogTemp, Log, TEXT("[GM][PostLogin] Player joined. Order=%d PC=%s"),
        JoinCounter, *GetNameSafe(NewPlayer));

    if (!NewPlayer)
        return;
}

void AIndianPokerGameModeBase::AdvancePhaseServer()
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM][AdvancePhaseServer] Blocked: not server"));
		return;
	}

	AIndianPokerGameStateBase* GS = GetGameState<AIndianPokerGameStateBase>();
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM][AdvancePhaseServer] GameState cast failed"));
		return;
	}

	const EGamePhase Next = GetNextPhase(GS->GetCurrentPhase());
	SetPhaseServer(Next);
}

void AIndianPokerGameModeBase::SetPhaseServer(EGamePhase NewPhase)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM][SetPhaseServer] Blocked: not server"));
		return;
	}

	AIndianPokerGameStateBase* GS = GetGameState<AIndianPokerGameStateBase>();
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM][SetPhaseServer] GameState cast failed"));
		return;
	}

	GS->SetPhaseServer(NewPhase);

	UE_LOG(LogTemp, Warning, TEXT("[GM] Phase set to %d"), (int32)NewPhase);
}

EGamePhase AIndianPokerGameModeBase::GetNextPhase(EGamePhase Current) const
{
	switch (Current)
	{
	case EGamePhase::Lobby:       return EGamePhase::Deal;
	case EGamePhase::Deal:        return EGamePhase::Betting;
	case EGamePhase::Betting:     return EGamePhase::Showdown;
	case EGamePhase::Showdown:    return EGamePhase::RoundResult;
	case EGamePhase::RoundResult: return EGamePhase::MatchEnd;
	case EGamePhase::MatchEnd:    return EGamePhase::Lobby;
	default:                      return EGamePhase::Lobby;
	}
}
