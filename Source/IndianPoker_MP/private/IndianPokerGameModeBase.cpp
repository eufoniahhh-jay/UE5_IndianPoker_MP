// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerGameModeBase.h"
#include "IndianPoker_MP.h"
#include "IndianPokerPlayerState.h"
#include "IndianPokerGameStateBase.h"
#include "IndianPokerPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Algo/RandomShuffle.h"

AIndianPokerGameModeBase::AIndianPokerGameModeBase()
{
	GameStateClass = AIndianPokerGameStateBase::StaticClass();
}

AIndianPokerGameStateBase* AIndianPokerGameModeBase::GetIndianPokerGameState()
{
	return GetGameState<AIndianPokerGameStateBase>();
}

void AIndianPokerGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	const FString MapName = GetWorld()->GetMapName();
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] BeginPlay - Map=%s"), *MapName);

	if (MapName.Contains(TEXT("GameMap")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] GameMap detected -> schedule (Delayed)TryStartRound"));

		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(
			TimerHandle,
			this,
			&AIndianPokerGameModeBase::DelayedTryStartRound,
			0.5f,
			false
		);
	}
}

void AIndianPokerGameModeBase::DelayedTryStartRound()
{
	const int32 PlayerCount = GameState ? GameState->PlayerArray.Num() : -1;

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] DelayedTryStartRound - PlayerArray Num=%d"), PlayerCount);

	if (HasAuthority() && GameState && GameState->PlayerArray.Num() == 2)
	{
		TryStartRound();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] DelayedTryStartRound denied"));
	}
}

void AIndianPokerGameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // GameMode는 서버에만 존재 → 조인 로그는 여기 하나면 충분
    JoinCounter++;

    // GameMode는 서버에만 존재하므로 기본적으로 서버 로직임
    //UE_LOG(LogTemp, Warning, TEXT("[GM][PostLogin] Player joined. PC=%s"), *GetNameSafe(NewPlayer));
    /*UE_LOG(LogTemp, Log, TEXT("[GM][PostLogin] Player joined. Order=%d PC=%s"),
        JoinCounter, *GetNameSafe(NewPlayer));

    if (!NewPlayer)
        return;*/

	// Day9
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] PostLogin - JoinCounter=%d"), JoinCounter);
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] PlayerArray Num=%d"), GameState ? GameState->PlayerArray.Num() : -1);

	/*if (HasAuthority() && GameState && GameState->PlayerArray.Num() == 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Two players joined. Starting round test..."));
		TryStartRound();
	}*/

	// 2명이 들어오고, GameMap이면 라운드 시작
	// -> 그냥 GameModeBase의 BeginPLay에서 조건 확인 후 TryStartRound() 실행하도록 변경 (이 코드는 삭제)
	/*const FString MapName = GetWorld()->GetMapName();

	if (HasAuthority() && GameState && GameState->PlayerArray.Num() == 2 && MapName.Contains(TEXT("GameMap")))
	{
		TryStartRound();
	}*/
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
	// Test Code (Before)
	/*if (!HasAuthority())
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

	UE_LOG(LogTemp, Warning, TEXT("[GM] Phase set to %d"), (int32)NewPhase);*/

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM][SetPhaseServer] Blocked: not server"));
		return;
	}

	AIndianPokerGameStateBase* GS = GetIndianPokerGameState();
	if (!GS)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] SetPhaseServer failed - GameState is null"));
		return;
	}

	GS->SetPhaseServer(NewPhase);

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Phase changed to %d"), static_cast<uint8>(NewPhase));
}

void AIndianPokerGameModeBase::SyncRoundStateToGameState()
{
	AIndianPokerGameStateBase* GS = GetIndianPokerGameState();
	if (!GS)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] SyncRoundStateToGameState failed - GameState is null"));
		return;
	}

	GS->SetPotServer(Pot);
	GS->SetRoundBetServer(RoundBet);

	// 아직 커스텀 PlayerState가 없거나 GetPlayerId() 쓰기 애매하면, 그냥 임시로 0으로 설정도 가능
	int32 FirstActorId = -1;
	int32 CurrentActorId = -1;

	if (FirstActorPS)
	{
		FirstActorId = FirstActorPS->GetPlayerId();
	}

	if (CurrentActorPS)
	{
		CurrentActorId = CurrentActorPS->GetPlayerId();
	}

	GS->SetFirstActorPlayerIdServer(FirstActorId);
	GS->SetCurrentActorPlayerIdServer(CurrentActorId);

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Round state synced to GameState"));
}

bool AIndianPokerGameModeBase::CanStartRound()
{
	if (GameState == nullptr)
	{
		return false;
	}

	if (GameState->PlayerArray.Num() != 2)
	{
		return false;
	}

	return true;
}

void AIndianPokerGameModeBase::TryStartRound()
{
	UE_LOG(LogTemp, Warning, TEXT("[Round] TryStartRound"));

	if (!CanStartRound())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Round] Start denied - player count is not 2"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Round] Round Start Approved"));
	StartRound();
}

void AIndianPokerGameModeBase::StartRound()
{
	UE_LOG(LogTemp, Warning, TEXT("[Round] New Round Start"));

	/*Pot = 2;
	RoundBet = 1;*/
	RequiredToCall = 0;

	SetPhaseServer(EGamePhase::Deal);

	// 임시값
	FirstActorPS = nullptr;
	CurrentActorPS = nullptr;

	GenerateDeck();
	ShuffleDeck();
	DealCards();
	SetVisibleOpponentCards();
	SendVisibleOpponentCardsToClients();
	ApplyAnte();
	InitBettingState();				// SyncRoundStateToGameState 이전에 실행

	SyncRoundStateToGameState();

	SetPhaseServer(EGamePhase::Betting);

	UE_LOG(LogTemp, Warning, TEXT("[Round] StartRound complete"));
}

void AIndianPokerGameModeBase::GenerateDeck()
{
	Deck.Empty();
	Discard.Empty();

	for (int32 Number = 1; Number <= 10; ++Number)
	{
		Deck.Add(Number);
		Deck.Add(Number);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Round] Deck Generated (%d cards)"), Deck.Num());

	FString DeckString;
	for (int32 CardValue : Deck)
	{
		DeckString += FString::Printf(TEXT("%d "), CardValue);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Round] Deck Contents = %s"), *DeckString);
}

void AIndianPokerGameModeBase::ShuffleDeck()
{
	if (Deck.Num() <= 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Round] ShuffleDeck skipped - Deck Num=%d"), Deck.Num());
		return;
	}

	FString BeforeShuffle;
	for (int32 CardValue : Deck)
	{
		BeforeShuffle += FString::Printf(TEXT("%d "), CardValue);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Round] Deck Before Shuffle = %s"), *BeforeShuffle);

	Algo::RandomShuffle(Deck);

	UE_LOG(LogTemp, Warning, TEXT("[Round] Deck Shuffled"));

	FString AfterShuffle;
	for (int32 CardValue : Deck)
	{
		AfterShuffle += FString::Printf(TEXT("%d "), CardValue);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Round] Deck After Shuffle = %s"), *AfterShuffle);
}

void AIndianPokerGameModeBase::DealCards()
{
	if (!GameState || GameState->PlayerArray.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] DealCards failed - PlayerArray Num=%d"),
			GameState ? GameState->PlayerArray.Num() : -1);
		return;
	}

	if (Deck.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] DealCards failed - Not enough cards in deck. Deck Num=%d"), Deck.Num());
		return;
	}

	AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] DealCards failed - PlayerState cast failed"));
		return;
	}

	P1->HiddenCardValue = Deck.Pop();
	P2->HiddenCardValue = Deck.Pop();

	UE_LOG(LogTemp, Warning, TEXT("[Deal] P1 drew card: %d"), P1->HiddenCardValue);
	UE_LOG(LogTemp, Warning, TEXT("[Deal] P2 drew card: %d"), P2->HiddenCardValue);
	UE_LOG(LogTemp, Warning, TEXT("[Deal] Deck Num after deal = %d"), Deck.Num());
}

void AIndianPokerGameModeBase::InitBettingState()
{
	if (!GameState || GameState->PlayerArray.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bet] InitBettingState failed - PlayerArray Num=%d"),
			GameState ? GameState->PlayerArray.Num() : -1);
		return;
	}

	AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bet] InitBettingState failed - PlayerState cast failed"));
		return;
	}

	// Day9 임시 규칙: PlayerArray[0]이 선공
	FirstActorPS = P1;
	CurrentActorPS = P1;
	RequiredToCall = 0;

	/*UE_LOG(LogTemp, Warning, TEXT("[Bet] FirstActor = %s"), *FirstActorPS->GetPlayerName());
	UE_LOG(LogTemp, Warning, TEXT("[Bet] CurrentActor = %s"), *CurrentActorPS->GetPlayerName());*/
	UE_LOG(LogTemp, Warning, TEXT("[Bet] FirstActor PlayerId = %d"), FirstActorPS->GetPlayerId());
	UE_LOG(LogTemp, Warning, TEXT("[Bet] CurrentActor PlayerId = %d"), CurrentActorPS->GetPlayerId());
	UE_LOG(LogTemp, Warning, TEXT("[Bet] RequiredToCall = %d"), RequiredToCall);
}

void AIndianPokerGameModeBase::SetVisibleOpponentCards()
{
	if (!GameState || GameState->PlayerArray.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] SetVisibleOpponentCards failed - PlayerArray Num=%d"),
			GameState ? GameState->PlayerArray.Num() : -1);
		return;
	}

	AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] SetVisibleOpponentCards failed - PlayerState cast failed"));
		return;
	}

	P1->VisibleOpponentCardValue = P2->HiddenCardValue;
	P2->VisibleOpponentCardValue = P1->HiddenCardValue;

	UE_LOG(LogTemp, Warning, TEXT("[Deal] P1 visible opponent card: %d"), P1->VisibleOpponentCardValue);
	UE_LOG(LogTemp, Warning, TEXT("[Deal] P2 visible opponent card: %d"), P2->VisibleOpponentCardValue);
}

void AIndianPokerGameModeBase::SendVisibleOpponentCardsToClients()
{
	if (!GameState || GameState->PlayerArray.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] SendVisibleOpponentCardsToClients failed - PlayerArray Num=%d"),
			GameState ? GameState->PlayerArray.Num() : -1);
		return;
	}

	AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] SendVisibleOpponentCardsToClients failed - PlayerState cast failed"));
		return;
	}

	AIndianPokerPlayerController* PC1 = Cast<AIndianPokerPlayerController>(P1->GetPlayerController());
	AIndianPokerPlayerController* PC2 = Cast<AIndianPokerPlayerController>(P2->GetPlayerController());

	if (!PC1 || !PC2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] SendVisibleOpponentCardsToClients failed - PlayerController cast failed"));
		return;
	}

	PC1->ClientReceiveVisibleOpponentCard(P1->VisibleOpponentCardValue);
	PC2->ClientReceiveVisibleOpponentCard(P2->VisibleOpponentCardValue);

	UE_LOG(LogTemp, Warning, TEXT("[Deal] Sent visible opponent card to P1 client: %d"), P1->VisibleOpponentCardValue);
	UE_LOG(LogTemp, Warning, TEXT("[Deal] Sent visible opponent card to P2 client: %d"), P2->VisibleOpponentCardValue);
}

void AIndianPokerGameModeBase::ApplyAnte()
{
	if (!GameState || GameState->PlayerArray.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bet] ApplyAnte failed - PlayerArray Num=%d"),
			GameState ? GameState->PlayerArray.Num() : -1);
		return;
	}

	AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bet] ApplyAnte failed - PlayerState cast failed"));
		return;
	}

	// Day9 단계에서는 기본 1칩 Ante만 처리
	P1->Chips -= 1;
	P2->Chips -= 1;

	Pot = 2;
	RoundBet = 1;

	UE_LOG(LogTemp, Warning, TEXT("[Bet] Ante Applied"));
	UE_LOG(LogTemp, Warning, TEXT("[Bet] P1 Chips: %d"), P1->Chips);
	UE_LOG(LogTemp, Warning, TEXT("[Bet] P2 Chips: %d"), P2->Chips);
	UE_LOG(LogTemp, Warning, TEXT("[Bet] Pot: %d"), Pot);
	UE_LOG(LogTemp, Warning, TEXT("[Bet] RoundBet: %d"), RoundBet);
}

EGamePhase AIndianPokerGameModeBase::GetNextPhase(EGamePhase Current)
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
