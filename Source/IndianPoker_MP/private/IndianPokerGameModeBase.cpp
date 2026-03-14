// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerGameModeBase.h"
#include "IndianPoker_MP.h"
#include "IndianPokerGameStateBase.h"
#include "IndianPokerPlayerState.h"
#include "IndianPokerPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
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
	UE_LOG(LogTemp, Warning, TEXT("[SyncRoundStateToGameState] Pot=%d RoundBet=%d FirstActorPlayerId=%d CurrentActorPlayerId=%d"),
		Pot, RoundBet, AuthFirstActorPlayerId, AuthCurrentActorPlayerId);

	AIndianPokerGameStateBase* GS = GetIndianPokerGameState();
	if (!GS)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] SyncRoundStateToGameState failed - GameState is null"));
		return;
	}

	GS->SetPotServer(Pot);
	GS->SetRoundBetServer(RoundBet);

	// 아직 커스텀 PlayerState가 없거나 GetPlayerId() 쓰기 애매하면, 그냥 임시로 0으로 설정도 가능
	/*int32 FirstActorId = -1;
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
	GS->SetCurrentActorPlayerIdServer(CurrentActorId);*/

	GS->SetFirstActorPlayerIdServer(AuthFirstActorPlayerId);
	GS->SetCurrentActorPlayerIdServer(AuthCurrentActorPlayerId);

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Sync FirstActorPlayerId=%d"), AuthFirstActorPlayerId);
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Sync CurrentActorPlayerId=%d"), AuthCurrentActorPlayerId);
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
	bRoundEnded = false;

	//SetPhaseServer(EGamePhase::Deal);

	// 임시값
	/*FirstActorPS = nullptr;
	CurrentActorPS = nullptr;*/

	AuthFirstActorPlayerId = INDEX_NONE;
	AuthCurrentActorPlayerId = INDEX_NONE;

	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	// Day11. PlayerState 라운드 시작시 PlayerState 캐시하기
	AIndianPokerGameStateBase* GS = GetIndianPokerGameState();

	RoundP1PS = Cast<AIndianPokerPlayerState>(GS->PlayerArray[0]);
	RoundP2PS = Cast<AIndianPokerPlayerState>(GS->PlayerArray[1]);

	// 플레이어별 라운드 상태 초기화
	RoundP1PS->bFolded = false;
	RoundP2PS->bFolded = false;

	UE_LOG(LogTemp, Warning, TEXT("[Round] Reset Round State - bRoundEnded=false, P1/P2 Folded=false"));

	SetPhaseServer(EGamePhase::Deal);

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

	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	if (!GetCachedRoundPlayers(P1, P2))
	{
		return;
	}

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

	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	if (!GetCachedRoundPlayers(P1, P2))
	{
		return;
	}

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bet] InitBettingState failed - PlayerState cast failed"));
		return;
	}

	// 베팅 상태 초기화
	bHasOpeningCheck = false;

	// Day9 임시 규칙: PlayerArray[0]이 선공
	/*FirstActorPS = P1;
	CurrentActorPS = P1;*/

	// Day11
	AuthFirstActorPlayerId = P1 ? P1->GetPlayerId() : INDEX_NONE;
	AuthCurrentActorPlayerId = P1 ? P1->GetPlayerId() : INDEX_NONE;

	RequiredToCall = 0;

	/*UE_LOG(LogTemp, Warning, TEXT("[Bet] FirstActor = %s"), *FirstActorPS->GetPlayerName());
	UE_LOG(LogTemp, Warning, TEXT("[Bet] CurrentActor = %s"), *CurrentActorPS->GetPlayerName());*/
	/*UE_LOG(LogTemp, Warning, TEXT("[Bet] FirstActor PlayerId = %d"), FirstActorPS->GetPlayerId());
	UE_LOG(LogTemp, Warning, TEXT("[Bet] CurrentActor PlayerId = %d"), CurrentActorPS->GetPlayerId());*/
	/*UE_LOG(LogTemp, Warning, TEXT("[Bet] FirstActorPS=%s Ptr=%p Id=%d"),
		*GetNameSafe(FirstActorPS),
		FirstActorPS,
		FirstActorPS ? FirstActorPS->GetPlayerId() : -1);

	UE_LOG(LogTemp, Warning, TEXT("[Bet] CurrentActorPS=%s Ptr=%p Id=%d"),
		*GetNameSafe(CurrentActorPS),
		CurrentActorPS,
		CurrentActorPS ? CurrentActorPS->GetPlayerId() : -1);*/

	UE_LOG(LogTemp, Warning, TEXT("[InitBettingState] P1=%s Ptr=%p Id=%d Chips=%d"),
		*GetNameSafe(P1), P1, P1 ? P1->GetPlayerId() : -1, P1 ? P1->Chips : -1);

	UE_LOG(LogTemp, Warning, TEXT("[InitBettingState] P2=%s Ptr=%p Id=%d Chips=%d"),
		*GetNameSafe(P2), P2, P2 ? P2->GetPlayerId() : -1, P2 ? P2->Chips : -1);

	/*UE_LOG(LogTemp, Warning, TEXT("[InitBettingState] FirstActorPS=%s Ptr=%p Id=%d"),
		*GetNameSafe(FirstActorPS), FirstActorPS,
		FirstActorPS ? FirstActorPS->GetPlayerId() : -1);

	UE_LOG(LogTemp, Warning, TEXT("[InitBettingState] CurrentActorPS=%s Ptr=%p Id=%d"),
		*GetNameSafe(CurrentActorPS), CurrentActorPS,
		CurrentActorPS ? CurrentActorPS->GetPlayerId() : -1);*/

	UE_LOG(LogTemp, Warning, TEXT("[InitBettingState] FirstActorPlayerId=%d"), AuthFirstActorPlayerId);
	UE_LOG(LogTemp, Warning, TEXT("[InitBettingState] CurrentActorPlayerId=%d"), AuthCurrentActorPlayerId);

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

	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	if (!GetCachedRoundPlayers(P1, P2))
	{
		return;
	}

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

	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	if (!GetCachedRoundPlayers(P1, P2))
	{
		return;
	}

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

	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	if (!GetCachedRoundPlayers(P1, P2))
	{
		return;
	}

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bet] ApplyAnte failed - PlayerState cast failed"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ApplyAnte] P1=%s Ptr=%p Id=%d ChipsBefore=%d"),
		*GetNameSafe(P1), P1, P1 ? P1->GetPlayerId() : -1, P1 ? P1->Chips : -1);

	UE_LOG(LogTemp, Warning, TEXT("[ApplyAnte] P2=%s Ptr=%p Id=%d ChipsBefore=%d"),
		*GetNameSafe(P2), P2, P2 ? P2->GetPlayerId() : -1, P2 ? P2->Chips : -1);

	// Day9 단계에서는 기본 1칩 Ante만 처리
	P1->Chips -= 1;
	P2->Chips -= 1;

	Pot = 2;
	RoundBet = 1;

	UE_LOG(LogTemp, Warning, TEXT("[Bet] Ante Applied"));
	UE_LOG(LogTemp, Warning, TEXT("[Bet] P1 ChipsAfter: %d"), P1->Chips);
	UE_LOG(LogTemp, Warning, TEXT("[Bet] P2 ChipsAfter: %d"), P2->Chips);
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

void AIndianPokerGameModeBase::HandlePlayerAction(AIndianPokerPlayerController* RequestingPC, EBettingActionType ActionType, int32 RaiseExtra)
{
	const UEnum* ActionEnum = StaticEnum<EBettingActionType>();
	const FString ActionString = ActionEnum
		? ActionEnum->GetNameStringByValue(static_cast<int64>(ActionType))
		: TEXT("InvalidAction");

	//APlayerState* RequestingPS = RequestingPC ? RequestingPC->GetPlayerState<APlayerState>() : nullptr;

	AIndianPokerPlayerState* RequestingPS = nullptr;
	AIndianPokerPlayerState* OpponentPS = nullptr;

	/*UE_LOG(LogTemp, Warning, TEXT("[GameMode] HandlePlayerAction - Player=%s, Action=%s, RaiseExtra=%d"),
		*GetNameSafe(RequestingPS),
		*ActionString,
		RaiseExtra);*/

	if (!ValidateActionRequest(RequestingPC, RequestingPS, OpponentPS))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Validated Action - Player=%s Id=%d Action=%s"),
		*GetNameSafe(RequestingPS),
		RequestingPS ? RequestingPS->GetPlayerId() : -1,
		*ActionString);

	switch (ActionType)
	{
	case EBettingActionType::Check:
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Check branch entered"));
		HandleAction_Check(RequestingPS, OpponentPS);
		break;

	case EBettingActionType::CheckCall:
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] CheckCall branch entered"));
		HandleAction_CheckCall(RequestingPS, OpponentPS);
		break;

	case EBettingActionType::Fold:
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Fold branch entered"));
		HandleAction_Fold(RequestingPS, OpponentPS);
		break;

	default:
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Unknown action type"));
		break;
	}
}

bool AIndianPokerGameModeBase::ValidateActionRequest(
	AIndianPokerPlayerController* RequestingPC,
	AIndianPokerPlayerState*& OutRequestingPS,
	AIndianPokerPlayerState*& OutOpponentPS)
{
	OutRequestingPS = nullptr;
	OutOpponentPS = nullptr;

	if (!RequestingPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - RequestingPC is null"));
		return false;
	}

	/*OutRequestingPS = RequestingPC->GetPlayerState<AIndianPokerPlayerState>();
	if (!OutRequestingPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - RequestingPS is null"));
		return false;
	}*/
	AIndianPokerPlayerState* ControllerPS = RequestingPC->GetPlayerState<AIndianPokerPlayerState>();

	if (!ControllerPS) {
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - ControllerPS is null"));
		return false;
	}

	const int32 RequestingId = ControllerPS->GetPlayerId();

	if (!GameState || GameState->PlayerArray.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - Invalid PlayerArray Num=%d"),
			GameState ? GameState->PlayerArray.Num() : -1);
		return false;
	}

	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	// Day11. 헬퍼 사용으로 변경
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	
	//if (!GetRoundPlayerStates(P1, P2))
	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - GetRoundPlayerStates failed"));
		return false;
	}

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - PlayerState cast failed"));
		return false;
	}

	/*if (OutRequestingPS == P1)
	{
		OutOpponentPS = P2;
	}
	else if (OutRequestingPS == P2)
	{
		OutOpponentPS = P1;
	}*/
	if (P1->GetPlayerId() == RequestingId)
	{
		OutRequestingPS = P1;
		OutOpponentPS = P2;
	}
	else if (P2->GetPlayerId() == RequestingId)
	{
		OutRequestingPS = P2;
		OutOpponentPS = P1;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - RequestingPS not found in PlayerArray"));
		return false;
	}

	AIndianPokerGameStateBase* GS = GetGameState<AIndianPokerGameStateBase>();
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - GameState is null"));
		return false;
	}

	if (GS->GetCurrentPhase() != EGamePhase::Betting)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - Phase is not Betting"));
		return false;
	}

	if (bRoundEnded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - Round already ended"));
		return false;
	}

	if (OutRequestingPS->bFolded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - Player already folded. PlayerId=%d"),
			OutRequestingPS->GetPlayerId());
		return false;
	}

	/*if (CurrentActorPS != OutRequestingPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - Not current actor. Requesting=%d CurrentActor=%d"),
			OutRequestingPS->GetPlayerId(),
			CurrentActorPS ? CurrentActorPS->GetPlayerId() : -1);
		return false;
	}*/
	/*if (!CurrentActorPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - CurrentActorPS is null"));
		return false;
	}

	if (CurrentActorPS->GetPlayerId() != OutRequestingPS->GetPlayerId())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - Not current actor. Requesting=%d CurrentActor=%d"),
			OutRequestingPS->GetPlayerId(),
			CurrentActorPS->GetPlayerId());
		return false;
	}*/
	// Day11.
	if (AuthCurrentActorPlayerId == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - CurrentActorPlayerId is INDEX_NONE"));
		return false;
	}

	if (AuthCurrentActorPlayerId != OutRequestingPS->GetPlayerId())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - Not current actor. Requesting=%d CurrentActor=%d"),
			OutRequestingPS->GetPlayerId(),
			AuthCurrentActorPlayerId);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Success - Requesting=%d Opponent=%d"),
		OutRequestingPS->GetPlayerId(),
		OutOpponentPS->GetPlayerId());

	/*UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] RequestingPS=%s Ptr=%p Id=%d"),
		*GetNameSafe(OutRequestingPS),
		OutRequestingPS,
		OutRequestingPS ? OutRequestingPS->GetPlayerId() : -1);

	UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] CurrentActorPS=%s Ptr=%p Id=%d"),
		*GetNameSafe(CurrentActorPS),
		CurrentActorPS,
		CurrentActorPS ? CurrentActorPS->GetPlayerId() : -1);*/

	UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] CurrentActorPlayerId=%d"), AuthCurrentActorPlayerId);

	UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] RequestingId=%d CurrentActorPlayerId=%d FirstActorPlayerId=%d"),
		OutRequestingPS ? OutRequestingPS->GetPlayerId() : -1,
		AuthCurrentActorPlayerId,
		AuthFirstActorPlayerId);

	return true;
}

bool AIndianPokerGameModeBase::HandleAction_Check(
	AIndianPokerPlayerState* RequestingPS,
	AIndianPokerPlayerState* OpponentPS)
{
	if (!RequestingPS || !OpponentPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Check] Failed - RequestingPS or OpponentPS is null"));
		return false;
	}

	/*if (!FirstActorPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Check] Failed - FirstActorPS is null"));
		return false;
	}

	if (RequestingPS->GetPlayerId() != FirstActorPS->GetPlayerId())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Check] Failed - Only FirstActor can Check. Requesting=%d FirstActor=%d"),
			RequestingPS->GetPlayerId(),
			FirstActorPS->GetPlayerId());
		return false;
	}*/
	// Day11
	if (AuthFirstActorPlayerId == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Check] Failed - FirstActorPlayerId is INDEX_NONE"));
		return false;
	}

	if (RequestingPS->GetPlayerId() != AuthFirstActorPlayerId)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Check] Failed - Only FirstActor can Check. Requesting=%d FirstActor=%d"),
			RequestingPS->GetPlayerId(),
			AuthFirstActorPlayerId);
		return false;
	}

	if (RequiredToCall != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Check] Failed - RequiredToCall is not zero. RequiredToCall=%d"),
			RequiredToCall);
		return false;
	}

	// Day11
	UE_LOG(LogTemp, Warning, TEXT("[CheckDebug] RequestingPS=%s Ptr=%p Id=%d"),
		*GetNameSafe(RequestingPS),
		RequestingPS,
		RequestingPS ? RequestingPS->GetPlayerId() : -1);

	UE_LOG(LogTemp, Warning, TEXT("[CheckDebug] OpponentPS=%s Ptr=%p Id=%d"),
		*GetNameSafe(OpponentPS),
		OpponentPS,
		OpponentPS ? OpponentPS->GetPlayerId() : -1);

	UE_LOG(LogTemp, Warning, TEXT("[CheckDebug] Before Assign CurrentActorPlayerId=%d"), AuthCurrentActorPlayerId);

	// bHasOpeningCheck는 후공이 나중에 CheckCall을 할 수 있는 근거
	bHasOpeningCheck = true;		
	//CurrentActorPS = OpponentPS;
	AuthCurrentActorPlayerId = OpponentPS ? OpponentPS->GetPlayerId() : INDEX_NONE;
	UE_LOG(LogTemp, Warning, TEXT("[CheckDebug] After Assign CurrentActorPlayerId=%d"), AuthCurrentActorPlayerId);

	UE_LOG(LogTemp, Warning, TEXT("[CheckDebug] Before Sync CurrentActorPlayerId=%d"), AuthCurrentActorPlayerId);
	SyncRoundStateToGameState();


	UE_LOG(LogTemp, Warning, TEXT("[Check] Success - PlayerId=%d checked"), RequestingPS->GetPlayerId());
	UE_LOG(LogTemp, Warning, TEXT("[Check] bHasOpeningCheck=true"));
	UE_LOG(LogTemp, Warning, TEXT("[Check] RequestingId=%d FirstActorPlayerId=%d CurrentActorPlayerId=%d"),
		RequestingPS->GetPlayerId(), AuthFirstActorPlayerId, AuthCurrentActorPlayerId);
	UE_LOG(LogTemp, Warning, TEXT("[Check] Turn passed to PlayerId=%d"), AuthCurrentActorPlayerId);
		/*CurrentActorPS ? CurrentActorPS->GetPlayerId() : -1);*/
	UE_LOG(LogTemp, Warning, TEXT("[Check] Pot=%d RoundBet=%d RequiredToCall=%d"),
		Pot, RoundBet, RequiredToCall);

	return true;
}

bool AIndianPokerGameModeBase::HandleAction_CheckCall(
	AIndianPokerPlayerState* RequestingPS,
	AIndianPokerPlayerState* OpponentPS)
{
	if (!RequestingPS || !OpponentPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CheckCall] Failed - RequestingPS or OpponentPS is null"));
		return false;
	}

	if (!bHasOpeningCheck)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CheckCall] Failed - bHasOpeningCheck is false"));
		return false;
	}

	if (RequiredToCall != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CheckCall] Failed - RequiredToCall is not zero. RequiredToCall=%d"),
			RequiredToCall);
		return false;
	}

	SetPhaseServer(EGamePhase::Showdown);

	UE_LOG(LogTemp, Warning, TEXT("[CheckCall] Success - PlayerId=%d check-called"), RequestingPS->GetPlayerId());
	UE_LOG(LogTemp, Warning, TEXT("[CheckCall] bHasOpeningCheck=%s"), bHasOpeningCheck ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("[CheckCall] Enter Showdown"));
	UE_LOG(LogTemp, Warning, TEXT("[CheckCall] Pot=%d RoundBet=%d RequiredToCall=%d"),
		Pot, RoundBet, RequiredToCall);

	return true;
}

bool AIndianPokerGameModeBase::HandleAction_Fold(
	AIndianPokerPlayerState* RequestingPS,
	AIndianPokerPlayerState* OpponentPS)
{
	if (!RequestingPS || !OpponentPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Fold] Failed - RequestingPS or OpponentPS is null"));
		return false;
	}

	if (RequestingPS->bFolded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Fold] Failed - Player already folded. PlayerId=%d"),
			RequestingPS->GetPlayerId());
		return false;
	}

	RequestingPS->bFolded = true;

	UE_LOG(LogTemp, Warning, TEXT("[Fold] PlayerId=%d folded"), RequestingPS->GetPlayerId());

	ResolveFoldRound(RequestingPS, OpponentPS);
	return true;
}

void AIndianPokerGameModeBase::ResolveFoldRound(
	AIndianPokerPlayerState* FolderPS,
	AIndianPokerPlayerState* WinnerPS)
{
	if (!FolderPS || !WinnerPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Failed - FolderPS or WinnerPS is null"));
		return;
	}

	const int32 PotBeforeAward = Pot;
	const int32 WinnerChipsBefore = WinnerPS->Chips;
	const int32 FolderChipsBefore = FolderPS->Chips;
	const int32 FolderHiddenCard = FolderPS->HiddenCardValue;

	// 승자에게 Pot 지급
	WinnerPS->Chips += PotBeforeAward;

	// 10 카드 폴드 패널티
	bool bAppliedTenPenalty = false;
	if (FolderHiddenCard == 10)
	{
		FolderPS->Chips -= 10;
		bAppliedTenPenalty = true;
	}

	const int32 WinnerChipsAfter = WinnerPS->Chips;
	const int32 FolderChipsAfter = FolderPS->Chips;

	bRoundEnded = true;
	//CurrentActorPS = nullptr;
	AuthCurrentActorPlayerId = INDEX_NONE;

	SetPhaseServer(EGamePhase::RoundResult);

	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Round Ended by Fold"));
	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Winner=%s Ptr=%p Id=%d ChipsBefore=%d"),
		*GetNameSafe(WinnerPS), WinnerPS,
		WinnerPS ? WinnerPS->GetPlayerId() : -1,
		WinnerChipsBefore);

	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Folder=%s Ptr=%p Id=%d ChipsBefore=%d"),
		*GetNameSafe(FolderPS), FolderPS,
		FolderPS ? FolderPS->GetPlayerId() : -1,
		FolderChipsBefore);

	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Pot Awarded=%d"), PotBeforeAward);
	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Winner Chips: %d -> %d"),
		WinnerChipsBefore, WinnerChipsAfter);
	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Folder Chips: %d -> %d"),
		FolderChipsBefore, FolderChipsAfter);

	if (bAppliedTenPenalty)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Ten-card penalty applied to PlayerId=%d"),
			FolderPS->GetPlayerId());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] No ten-card penalty"));
	}

	Pot = 0;
	RoundBet = 0;
	RequiredToCall = 0;

	SyncRoundStateToGameState();

	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] State Reset After Fold - Pot=%d RoundBet=%d RequiredToCall=%d"),
		Pot, RoundBet, RequiredToCall);
}

//void AIndianPokerGameModeBase::ResolveFoldRound(
//	AIndianPokerPlayerState* FolderPS,
//	AIndianPokerPlayerState* WinnerPS)
//{
//	if (!FolderPS || !WinnerPS)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Failed - FolderPS or WinnerPS is null"));
//		return;
//	}
//
//	const int32 PotBeforeAward = Pot;
//	const int32 WinnerChipsBefore = WinnerPS->Chips;
//	const int32 FolderChipsBefore = FolderPS->Chips;
//
//	// 승자에게 Pot 지급
//	WinnerPS->Chips += Pot;
//	//WinnerPS->SetChips(WinnerPS->GetChips() + Pot);
//
//	// 10 카드 폴드 패널티
//	bool bAppliedTenPenalty = false;
//	if (FolderPS->HiddenCardValue == 10)
//	{
//		//FolderPS->SetChips(FolderPS->GetChips() - 10);
//		FolderPS->Chips -= 10;
//		bAppliedTenPenalty = true;
//	}
//
//	bRoundEnded = true;
//	CurrentActorPS = nullptr;
//
//	SetPhaseServer(EGamePhase::RoundResult);
//
//	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Round Ended by Fold"));
//	/*UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Folder PlayerId=%d"), FolderPS->GetPlayerId());
//	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Winner PlayerId=%d"), WinnerPS->GetPlayerId());*/
//	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Winner=%s Ptr=%p Id=%d ChipsBefore=%d"),
//		*GetNameSafe(WinnerPS), WinnerPS,
//		WinnerPS ? WinnerPS->GetPlayerId() : -1,
//		WinnerPS ? WinnerPS->Chips : -1);
//
//	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Folder=%s Ptr=%p Id=%d ChipsBefore=%d"),
//		*GetNameSafe(FolderPS), FolderPS,
//		FolderPS ? FolderPS->GetPlayerId() : -1,
//		FolderPS ? FolderPS->Chips : -1);
//
//	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Pot Awarded=%d"), PotBeforeAward);
//	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Winner Chips: %d -> %d"),
//		WinnerChipsBefore, WinnerPS->Chips);
//	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Folder Chips: %d -> %d"),
//		FolderChipsBefore, FolderPS->Chips);
//
//	if (bAppliedTenPenalty)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Ten-card penalty applied to PlayerId=%d"),
//			FolderPS->GetPlayerId());
//	}
//	else
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] No ten-card penalty"));
//	}
//
//	Pot = 0;
//	RoundBet = 0;
//	RequiredToCall = 0;
//
//	SyncRoundStateToGameState();
//
//	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] State Reset After Fold - Pot=%d RoundBet=%d RequiredToCall=%d"),
//		Pot, RoundBet, RequiredToCall);
//}

bool AIndianPokerGameModeBase::GetRoundPlayerStates(
	AIndianPokerPlayerState*& OutP1,
	AIndianPokerPlayerState*& OutP2) const
{
	OutP1 = nullptr;
	OutP2 = nullptr;

	const AIndianPokerGameStateBase* GS = GetGameState<AIndianPokerGameStateBase>();
	if (!GS)
	{
		return false;
	}

	if (GS->PlayerArray.Num() != 2)
	{
		return false;
	}

	OutP1 = Cast<AIndianPokerPlayerState>(GS->PlayerArray[0]);
	OutP2 = Cast<AIndianPokerPlayerState>(GS->PlayerArray[1]);

	return (OutP1 && OutP2);
}

AIndianPokerPlayerState* AIndianPokerGameModeBase::FindRoundPlayerStateById(int32 PlayerId) const
{
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;

	//if (!GetRoundPlayerStates(P1, P2))
	if (!GetCachedRoundPlayers(P1, P2))
	{
		return nullptr;
	}

	if (P1 && P1->GetPlayerId() == PlayerId)
	{
		return P1;
	}

	if (P2 && P2->GetPlayerId() == PlayerId)
	{
		return P2;
	}

	return nullptr;
}

bool AIndianPokerGameModeBase::GetCachedRoundPlayers(
	AIndianPokerPlayerState*& OutP1,
	AIndianPokerPlayerState*& OutP2) const
{
	OutP1 = RoundP1PS;
	OutP2 = RoundP2PS;
	return (OutP1 && OutP2);
}