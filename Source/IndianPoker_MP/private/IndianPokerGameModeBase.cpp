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
#include "CardActor.h"
#include "Kismet/GameplayStatics.h"
#include "IndianPokerSessionSubsystem.h"

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

	InitializeMatchModeFromSessionSubsystem();
	EnsureBotParticipantCreated();

	const FString MapName = GetWorld()->GetMapName();
	const float TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f;

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] BeginPlay - Map=%s | Time=%.2f"),
		*MapName,
		TimeSeconds);

	if (MapName.Contains(TEXT("GameMap")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] GameMap detected -> schedule DelayedTryStartRound after 2.0 sec"));

		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(
			TimerHandle,
			this,
			&AIndianPokerGameModeBase::DelayedTryStartRound,
			2.0f,
			false
		); 
	}

	// Day16. 카드 액터 캐싱
	CacheWorldCardActors();
}

void AIndianPokerGameModeBase::DelayedTryStartRound()
{
	const float TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f;
	const int32 PlayerCount = GameState ? GameState->PlayerArray.Num() : -1;

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] DelayedTryStartRound FIRED | Time=%.2f |  PlayerArray Num=%d"), TimeSeconds, PlayerCount);

	/*if (HasAuthority() && GameState && GameState->PlayerArray.Num() == 2)
	{
		TryStartRound();
	}*/
	if (CanStartRound())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] DelayedTryStartRound -> ready, starting"));
		TryStartRound();
		return;
	}
	/*else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] DelayedTryStartRound denied"));
	}*/
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] DelayedTryStartRound -> not ready, retry"));
	GetWorldTimerManager().SetTimerForNextTick(this, &AIndianPokerGameModeBase::DelayedTryStartRound);
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

	// Day14
	GS->SetCurrentRoundNumberServer(CurrentRoundNumber);
	GS->SetLastActionTextServer(LastActionText);

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Sync FirstActorPlayerId=%d"), AuthFirstActorPlayerId);
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Sync CurrentActorPlayerId=%d"), AuthCurrentActorPlayerId);
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Round state synced to GameState"));

	// Day15
	GS->SetHasOpeningCheckServer(bHasOpeningCheck); 
}

//bool AIndianPokerGameModeBase::CanStartRound()
//{
//	if (GameState == nullptr)
//	{
//		return false;
//	}
//
//	if (GameState->PlayerArray.Num() != 2)
//	{
//		return false;
//	}
//
//	return true;
//}

bool AIndianPokerGameModeBase::CanStartRound()
{
	if (!HasAuthority())
	{
		return false;
	}

	AIndianPokerGameStateBase* GS = GetIndianPokerGameState();
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CanStartRound] Failed - GameState is null"));
		return false;
	}

	// Day19. 이제 PvE일 경우에는 인간 1명 + Bot 1명 조합 가능하면 OK (PvP는 여전히 인간 2명 필요)
	if (CurrentMatchMode == EIndianPokerMatchMode::PvP)
	{
		if (GS->PlayerArray.Num() != 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CanStartRound] PvP failed - PlayerArray Num=%d"), GS->PlayerArray.Num());
			return false;
		}
	}
	else if (CurrentMatchMode == EIndianPokerMatchMode::PvE)
	{
		if (GS->PlayerArray.Num() < 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CanStartRound] PvE failed - PlayerArray Num=%d"), GS->PlayerArray.Num());
			return false;
		}
	}
	/*if (GS->PlayerArray.Num() != 2) 
	{
		UE_LOG(LogTemp, Warning, TEXT("[CanStartRound] Failed - PlayerArray Num=%d"), GS->PlayerArray.Num());
		return false;
	}*/

	AIndianPokerPlayerState* ReadyP1 = nullptr;
	AIndianPokerPlayerState* ReadyP2 = nullptr;

	/*if (!GatherReadyMatchPlayersFromControllers(ReadyP1, ReadyP2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CanStartRound] Failed - ready players not gathered yet"));
		return false;
	}*/
	if (!BuildMatchParticipants(ReadyP1, ReadyP2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CanStartRound] Failed - BuildMatchParticipants failed"));
		return false;
	}

	if (!IsValid(ReadyP1) || !IsValid(ReadyP2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CanStartRound] Failed - gathered players invalid"));
		return false;
	}

	return true;
}

void AIndianPokerGameModeBase::TryStartRound()
{
	const float TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f;
	UE_LOG(LogTemp, Warning, TEXT("[Round] TryStartRound | Time=%.2f"), TimeSeconds);

	if (!CanStartRound())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Round] Start denied - participants not ready"));
		return;
	}

	// Day13. 최초 플레이어 캐시하기
	if (!EnsureMatchPlayersCached())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Round] TryStartRound failed - player cache not ready"));
        return;
    }

	UE_LOG(LogTemp, Warning, TEXT("[Round] Round Start Approved"));
	StartRound();
}

void AIndianPokerGameModeBase::StartRound()
{
	UE_LOG(LogTemp, Warning, TEXT("[Round] New Round Start"));

	CurrentRoundNumber++;
	LastActionText = FString::Printf(TEXT("Round %d Start"), CurrentRoundNumber);

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
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Round] StartRound failed - GameState is null"));
		return;
	}

	/*RoundP1PS = Cast<AIndianPokerPlayerState>(GS->PlayerArray[0]);
	RoundP2PS = Cast<AIndianPokerPlayerState>(GS->PlayerArray[1]);*/
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Round] StartRound failed - GetCachedRoundPlayers failed"));
		return;
	}

	// 플레이어별 라운드 상태 초기화
	/*RoundP1PS->bFolded = false;
	RoundP2PS->bFolded = false;*/
	P1->bFolded = false;
	P2->bFolded = false;

	UE_LOG(LogTemp, Warning, TEXT("[Round] Reset Round State - bRoundEnded=false, P1/P2 Folded=false"));

	// Day17. 카드 재분배 전에 이전 라운드 showdown에서 설정한 카드 공개 상태 끄기
	// Day17. showdown 들어온 순간 카드 공개 
	if (P1WorldCard) {
		P1WorldCard->SetRevealState(false);
	}
	if (P2WorldCard) {
		P2WorldCard->SetRevealState(false);
	}

	SetPhaseServer(EGamePhase::Deal);

	// Day12: 이번 라운드 베팅 기여량 초기화 (ApplyAnte보다 먼저!)
	/*RoundP1PS->CurrentRoundContribution = 0;
	RoundP2PS->CurrentRoundContribution = 0;*/
	P1->CurrentRoundContribution = 0;
	P2->CurrentRoundContribution = 0;

	GenerateDeck();
	ShuffleDeck();
	DealCards();
	SetVisibleOpponentCards();
	SendVisibleOpponentCardsToClients();
	ApplyAnte();
	InitBettingState();				// SyncRoundStateToGameState 이전에 실행

	SyncRoundStateToGameState();

	// Day16. 카드 분배가 끝난 직후, 실제 월드 카드 갱신
	UpdateWorldCardVisuals();

	SetPhaseServer(EGamePhase::Betting);

	// Day19. 라운드 시작 직후 선공이 Bot이면 자동 행동 예약
	TryScheduleBotTurn();

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
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	// Day19. Bot 사용을 위해 PlayerArray.Num() == 2 검사 없애기
	/*if (!GameState || GameState->PlayerArray.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] DealCards failed - PlayerArray Num=%d"),
			GameState ? GameState->PlayerArray.Num() : -1);
		return;
	}*/
	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] DealCards failed - GetCachedRoundPlayers failed"));
		return;
	}

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] DealCards failed - PlayerState cast failed"));
		return;
	}

	if (Deck.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] DealCards failed - Not enough cards in deck. Deck Num=%d"), Deck.Num());
		return;
	}

	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	

	P1->HiddenCardValue = Deck.Pop();
	P2->HiddenCardValue = Deck.Pop();

	UE_LOG(LogTemp, Warning, TEXT("[Deal] P1 drew card: %d"), P1->HiddenCardValue);
	UE_LOG(LogTemp, Warning, TEXT("[Deal] P2 drew card: %d"), P2->HiddenCardValue);
	UE_LOG(LogTemp, Warning, TEXT("[Deal] Deck Num after deal = %d"), Deck.Num());
}

void AIndianPokerGameModeBase::InitBettingState()
{
	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	// Day19. Bot 사용을 위해 PlayerArray.Num() == 2 검사 없애기
	/*if (!GameState || GameState->PlayerArray.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bet] InitBettingState failed - PlayerArray Num=%d"),
			GameState ? GameState->PlayerArray.Num() : -1);
		return;
	}*/
	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bet] ApplyAnte failed - GetCachedRoundPlayers failed"));
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

	// Day11. P1이 선공
	/*AuthFirstActorPlayerId = P1 ? P1->GetPlayerId() : INDEX_NONE;
	AuthCurrentActorPlayerId = P1 ? P1->GetPlayerId() : INDEX_NONE;*/

	// Day13. 선공을 번갈아서
	if (bNextRoundStartsWithP1)
	{
		AuthFirstActorPlayerId = P1->GetPlayerId();
		AuthCurrentActorPlayerId = P1->GetPlayerId();
	}
	else
	{
		AuthFirstActorPlayerId = P2->GetPlayerId();
		AuthCurrentActorPlayerId = P2->GetPlayerId();
	}
	bNextRoundStartsWithP1 = !bNextRoundStartsWithP1;

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

	// Day12. 테스트로그
	const int32 P1Required = CalcRequiredToCall(P1->GetPlayerId());
	const int32 P2Required = CalcRequiredToCall(P2->GetPlayerId());

	UE_LOG(LogTemp, Warning,
		TEXT("[BetState] P1 Chips=%d Contrib=%d Required=%d | P2 Chips=%d Contrib=%d Required=%d | Pot=%d"),
		RoundP1PS->Chips,
		RoundP1PS->CurrentRoundContribution,
		P1Required,
		RoundP2PS->Chips,
		RoundP2PS->CurrentRoundContribution,
		P2Required,
		Pot);

	UE_LOG(LogTemp, Warning, TEXT("[Bet] RequiredToCall = %d"), RequiredToCall);
}

void AIndianPokerGameModeBase::SetVisibleOpponentCards()
{
	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	
	// Day19. Bot 사용을 위해 PlayerArray.Num() == 2 검사 없애기
	/*if (!GameState || GameState->PlayerArray.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] SetVisibleOpponentCards failed - PlayerArray Num=%d"),
			GameState ? GameState->PlayerArray.Num() : -1);
		return;
	}*/
	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] SetVisibleOpponentCards failed - GetCachedRoundPlayers failed"));
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
	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] SendVisibleOpponentCardsToClients failed - GetCachedRoundPlayers failed"));
		return;
	}

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] SendVisibleOpponentCardsToClients failed - PlayerState cast failed"));
		return;
	}

	// Day19. Bot 사용을 위해 PlayerArray.Num() == 2 검사 없애기
	/*if (!GameState || GameState->PlayerArray.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Deal] SendVisibleOpponentCardsToClients failed - PlayerArray Num=%d"),
			GameState ? GameState->PlayerArray.Num() : -1);
		return;
	}*/

	// Day19. 이제 컨트롤러 찾기를 PvP, PvE 분기로 나눠줘야 함
	/*//AIndianPokerPlayerController* PC1 = Cast<AIndianPokerPlayerController>(P1->GetPlayerController());
	//AIndianPokerPlayerController* PC2 = Cast<AIndianPokerPlayerController>(P2->GetPlayerController());
	AIndianPokerPlayerController* PC1 = FindControllerByPlayerState(P1);
	AIndianPokerPlayerController* PC2 = FindControllerByPlayerState(P2);

	if (!PC1 || !PC2)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[Deal] SendVisibleOpponentCardsToClients failed - PlayerController cast failed"));
		UE_LOG(LogTemp, Warning, TEXT("[Deal] SendVisibleOpponentCardsToClients failed - PlayerController lookup failed"));
		return;
	}

	PC1->ClientReceiveVisibleOpponentCard(P1->VisibleOpponentCardValue);
	PC2->ClientReceiveVisibleOpponentCard(P2->VisibleOpponentCardValue);

	UE_LOG(LogTemp, Warning, TEXT("[Deal] Sent visible opponent card to P1 client: %d"), P1->VisibleOpponentCardValue);
	UE_LOG(LogTemp, Warning, TEXT("[Deal] Sent visible opponent card to P2 client: %d"), P2->VisibleOpponentCardValue);*/
	if (CurrentMatchMode == EIndianPokerMatchMode::PvP)
	{
		AIndianPokerPlayerController* PC1 = FindControllerByPlayerState(P1);
		AIndianPokerPlayerController* PC2 = FindControllerByPlayerState(P2);

		if (!PC1 || !PC2)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Deal] SendVisibleOpponentCardsToClients PvP failed - PlayerController lookup failed"));
			return;
		}

		PC1->ClientReceiveVisibleOpponentCard(P1->VisibleOpponentCardValue);
		PC2->ClientReceiveVisibleOpponentCard(P2->VisibleOpponentCardValue);

		UE_LOG(LogTemp, Warning, TEXT("[Deal] Sent visible opponent card to P1 client: %d"), P1->VisibleOpponentCardValue);
		UE_LOG(LogTemp, Warning, TEXT("[Deal] Sent visible opponent card to P2 client: %d"), P2->VisibleOpponentCardValue);
		return;
	}

	if (CurrentMatchMode == EIndianPokerMatchMode::PvE)
	{
		AIndianPokerPlayerState* HumanPS = nullptr;

		if (P1 && !P1->IsBot())
		{
			HumanPS = P1;
		}
		else if (P2 && !P2->IsBot())
		{
			HumanPS = P2;
		}

		if (!HumanPS)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Deal] SendVisibleOpponentCardsToClients PvE failed - HumanPS not found"));
			return;
		}

		AIndianPokerPlayerController* HumanPC = FindControllerByPlayerState(HumanPS);
		if (!HumanPC)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Deal] SendVisibleOpponentCardsToClients PvE failed - HumanPC not found"));
			return;
		}

		HumanPC->ClientReceiveVisibleOpponentCard(HumanPS->VisibleOpponentCardValue);

		UE_LOG(LogTemp, Warning, TEXT("[Deal] PvE sent visible opponent card to Human Id=%d Value=%d"),
			HumanPS->GetPlayerId(),
			HumanPS->VisibleOpponentCardValue);

		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Deal] SendVisibleOpponentCardsToClients failed - unknown match mode"));
}

void AIndianPokerGameModeBase::ApplyAnte()
{
	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;

	// Day19. Bot 사용을 위해 PlayerArray.Num() == 2 검사 없애기
	/*if (!GameState || GameState->PlayerArray.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bet] ApplyAnte failed - PlayerArray Num=%d"),
			GameState ? GameState->PlayerArray.Num() : -1);
		return;
	}*/
	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bet] ApplyAnte failed - GetCachedRoundPlayers failed"));
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

	// Day12
	P1->CurrentRoundContribution += 1;
	P2->CurrentRoundContribution += 1;

	// 무승부 이월 Pot이 있을 수 있으므로, 덮어쓰지 않고 누적
	Pot += 2;
	RoundBet = 1;

	UE_LOG(LogTemp, Warning, TEXT("[Bet] Ante Applied"));
	UE_LOG(LogTemp, Warning, TEXT("[Bet] P1 ChipsAfter: %d"), P1->Chips);
	UE_LOG(LogTemp, Warning, TEXT("[Bet] P2 ChipsAfter: %d"), P2->Chips);
	UE_LOG(LogTemp, Warning, TEXT("[Bet] P1 Contribution : %d"), P1->CurrentRoundContribution);
	UE_LOG(LogTemp, Warning, TEXT("[Bet] P2 Contribution : %d"), P2->CurrentRoundContribution);
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

	case EBettingActionType::Call:
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Call branch entered"));
		HandleAction_Call(RequestingPS, OpponentPS);
		break;

	case EBettingActionType::Raise:
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Raise branch entered"));
		HandleAction_Raise(RequestingPS, OpponentPS, RaiseExtra);
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

	// Day19. Bot 사용을 위해 PlayerArray.Num() == 2 검사 없애기
	/*if (!GameState || GameState->PlayerArray.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - Invalid PlayerArray Num=%d"),
			GameState ? GameState->PlayerArray.Num() : -1);
		return false;
	}*/

	/*AIndianPokerPlayerState* P1 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[0]);
	AIndianPokerPlayerState* P2 = Cast<AIndianPokerPlayerState>(GameState->PlayerArray[1]);*/
	// Day11. 헬퍼 사용으로 변경
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	
	//if (!GetRoundPlayerStates(P1, P2))
	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - GetCachedRoundPlayers failed"));
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
		//UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - RequestingPS not found in PlayerArray"));
		UE_LOG(LogTemp, Warning, TEXT("[ActionValidation] Failed - RequestingPS not found in cached participants"));
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

	
	/*if (RequiredToCall != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Check] Failed - RequiredToCall is not zero. RequiredToCall=%d"),
			RequiredToCall);
		return false;
	}*/
	// Day13. 액션 가능 여부는 무조건 CalcRequiredToCall 기준으로 리팩토링
	const int32 RequiredAmount = CalcRequiredToCall(RequestingPS->GetPlayerId());

	if (RequiredAmount != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Check] Failed - CalcRequiredToCall is not zero. Required=%d"),
			RequiredAmount);
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

	// Day14
	LastActionText = FString::Printf(TEXT("Player %d Checked"), RequestingPS->GetPlayerId());

	SyncRoundStateToGameState();
	// Day19. 턴이 BOT에게 넘어가는 시점에서 호출
	TryScheduleBotTurn();

	UE_LOG(LogTemp, Warning, TEXT("[Check] Success - PlayerId=%d checked"), RequestingPS->GetPlayerId());
	UE_LOG(LogTemp, Warning, TEXT("[Check] bHasOpeningCheck=true"));
	UE_LOG(LogTemp, Warning, TEXT("[Check] RequestingId=%d FirstActorPlayerId=%d CurrentActorPlayerId=%d"),
		RequestingPS->GetPlayerId(), AuthFirstActorPlayerId, AuthCurrentActorPlayerId);
	UE_LOG(LogTemp, Warning, TEXT("[Check] Turn passed to PlayerId=%d"), AuthCurrentActorPlayerId);
		/*CurrentActorPS ? CurrentActorPS->GetPlayerId() : -1);*/
	UE_LOG(LogTemp, Warning, TEXT("[Check] Pot=%d RoundBet=%d Required=%d"),
		Pot, RoundBet, RequiredAmount);

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

	const int32 RequiredAmount = CalcRequiredToCall(RequestingPS->GetPlayerId());

	/*if (RequiredToCall != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CheckCall] Failed - RequiredToCall is not zero. RequiredToCall=%d"),
			RequiredToCall);
		return false;
	}*/
	// Day13. 액션 가능 여부는 무조건 CalcRequiredToCall 기준으로 리팩토링
	if (RequiredAmount != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CheckCall] Failed - CalcRequiredToCall is not zero. Required=%d"),
			RequiredAmount);
		return false;
	}

	// Day13. 베팅 종료 후 AuthCurrentActorPlayerId 정리. 
	AuthCurrentActorPlayerId = INDEX_NONE;

	// Day14. HUD
	LastActionText = FString::Printf(TEXT("Player %d Check-Called"), RequestingPS->GetPlayerId());

	SyncRoundStateToGameState();

	SetPhaseServer(EGamePhase::Showdown);
	ResolveShowdown();

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

// Day12 Call / Raise
bool AIndianPokerGameModeBase::HandleAction_Call(
	AIndianPokerPlayerState* RequestingPS,
	AIndianPokerPlayerState* OpponentPS)
{
	if (!RequestingPS || !OpponentPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Call] Failed - RequestingPS or OpponentPS is null"));
		return false;
	}

	const int32 RequestingId = RequestingPS->GetPlayerId();
	const int32 RequiredAmount = CalcRequiredToCall(RequestingId);

	if (RequiredAmount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Call] Failed - RequiredToCall must be > 0. RequiredToCall=%d"),
			RequiredAmount);
		return false;
	}

	if (RequestingPS->Chips < RequiredAmount)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Call] Failed - Not enough chips. Chips=%d Required=%d"),
			RequestingPS->Chips,
			RequiredAmount);
		return false;
	}

	RequestingPS->Chips -= RequiredAmount;
	RequestingPS->CurrentRoundContribution += RequiredAmount;
	Pot += RequiredAmount;

	// Day13. 베팅 종료 후 AuthCurrentActorPlayerId 정리. 
	AuthCurrentActorPlayerId = INDEX_NONE;

	// Day14. HUD
	LastActionText = FString::Printf(
		TEXT("Player %d Called %d"),
		RequestingPS->GetPlayerId(),
		RequiredAmount
	);

	SyncRoundStateToGameState();

	UE_LOG(LogTemp, Warning, TEXT("[Call] Success - PlayerId=%d called for %d"), RequestingId, RequiredAmount);
	UE_LOG(LogTemp, Warning, TEXT("[Call] RequestingPS Chips=%d Contribution=%d"),
		RequestingPS->Chips,
		RequestingPS->CurrentRoundContribution);
	UE_LOG(LogTemp, Warning, TEXT("[Call] OpponentPS Chips=%d Contribution=%d"),
		OpponentPS->Chips,
		OpponentPS->CurrentRoundContribution);
	UE_LOG(LogTemp, Warning, TEXT("[Call] Pot=%d RoundBet=%d"), Pot, RoundBet);

	const int32 RequestingRequiredAfter = CalcRequiredToCall(RequestingPS->GetPlayerId());
	const int32 OpponentRequiredAfter = CalcRequiredToCall(OpponentPS->GetPlayerId());

	UE_LOG(LogTemp, Warning, TEXT("[Call] After Update - Requesting RequiredToCall=%d Opponent RequiredToCall=%d"),
		RequestingRequiredAfter,
		OpponentRequiredAfter);

	SetPhaseServer(EGamePhase::Showdown);
	ResolveShowdown();

	UE_LOG(LogTemp, Warning, TEXT("[Call] Enter Showdown"));

	return true;
}

bool AIndianPokerGameModeBase::HandleAction_Raise(
	AIndianPokerPlayerState* RequestingPS,
	AIndianPokerPlayerState* OpponentPS,
	int32 RaiseExtra)
{
	if (!RequestingPS || !OpponentPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Raise] Failed - RequestingPS or OpponentPS is null"));
		return false;
	}

	const int32 RequestingId = RequestingPS->GetPlayerId();
	const int32 RequiredAmount = CalcRequiredToCall(RequestingId);
	const int32 TotalPay = RequiredAmount + RaiseExtra;

	UE_LOG(LogTemp, Warning,
		TEXT("[Raise] Attempt - PlayerId=%d MyContrib=%d OppContrib=%d RequiredToCall=%d RaiseExtra=%d TotalPay=%d Chips=%d"),
		RequestingId,
		RequestingPS->CurrentRoundContribution,
		OpponentPS->CurrentRoundContribution,
		RequiredAmount,
		RaiseExtra,
		TotalPay,
		RequestingPS->Chips);

	if (RaiseExtra < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Raise] Failed - RaiseExtra must be >= 1. RaiseExtra=%d"), RaiseExtra);
		return false;
	}

	if (RequestingPS->Chips < TotalPay)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Raise] Failed - Not enough chips. Chips=%d TotalPay=%d"),
			RequestingPS->Chips,
			TotalPay);
		return false;
	}

	const int32 MyNewContribution = RequestingPS->CurrentRoundContribution + TotalPay;
	const int32 OpponentRequiredAfterRaise = MyNewContribution - OpponentPS->CurrentRoundContribution;

	if (OpponentPS->Chips < OpponentRequiredAfterRaise)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Raise] Failed - Opponent cannot respond. OpponentChips=%d RequiredAfterRaise=%d"),
			OpponentPS->Chips,
			OpponentRequiredAfterRaise);
		return false;
	}

	RequestingPS->Chips -= TotalPay;
	RequestingPS->CurrentRoundContribution += TotalPay;
	Pot += TotalPay;

	AuthCurrentActorPlayerId = OpponentPS->GetPlayerId();

	// Day14. HUD
	LastActionText = FString::Printf(
		TEXT("Player %d Raised +%d"),
		RequestingPS->GetPlayerId(),
		RaiseExtra
	);

	SyncRoundStateToGameState();
	// Day19. 턴이 BOT에게 넘어가는 시점에서 호출
	TryScheduleBotTurn();

	UE_LOG(LogTemp, Warning,
		TEXT("[Raise] Success - PlayerId=%d Paid=%d | Chips=%d | Contribution=%d | Pot=%d"),
		RequestingId,
		TotalPay,
		RequestingPS->Chips,
		RequestingPS->CurrentRoundContribution,
		Pot);

	UE_LOG(LogTemp, Warning, TEXT("[Raise] Turn passed to PlayerId=%d"), AuthCurrentActorPlayerId);

	const int32 RequestingRequiredAfter = CalcRequiredToCall(RequestingPS->GetPlayerId());
	const int32 OpponentRequiredAfter = CalcRequiredToCall(OpponentPS->GetPlayerId());

	UE_LOG(LogTemp, Warning,
		TEXT("[BetState][After Raise] Req Chips=%d Contrib=%d Required=%d | Opp Chips=%d Contrib=%d Required=%d | Pot=%d"),
		RequestingPS->Chips,
		RequestingPS->CurrentRoundContribution,
		RequestingRequiredAfter,
		OpponentPS->Chips,
		OpponentPS->CurrentRoundContribution,
		OpponentRequiredAfter,
		Pot);

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

	// Fold 라운드 종료 시에도 카드 공개
	if (P1WorldCard)
	{
		P1WorldCard->SetRevealState(true);
	}
	if (P2WorldCard)
	{
		P2WorldCard->SetRevealState(true);
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
		FolderPS->Chips -= 5;
		WinnerPS->Chips += 5;
		bAppliedTenPenalty = true;
	}

	const int32 WinnerChipsAfter = WinnerPS->Chips;
	const int32 FolderChipsAfter = FolderPS->Chips;

	bRoundEnded = true;
	//CurrentActorPS = nullptr;
	AuthCurrentActorPlayerId = INDEX_NONE;

	SetPhaseServer(EGamePhase::RoundResult);

	UE_LOG(LogTemp, Warning, TEXT("[FoldResolve] Round Ended by Fold"));

	// Day14.
	LastActionText = FString::Printf(
		TEXT("Player %d Folded - Player %d Wins"),
		FolderPS->GetPlayerId(),
		WinnerPS->GetPlayerId()
	);

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

	AdvanceAfterRound(2.0);
}

void AIndianPokerGameModeBase::ResolveShowdown()
{
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;

	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Showdown] Failed - GetCachedRoundPlayers failed"));
		return;
	}

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Showdown] Failed - P1 or P2 is null"));
		return;
	}

	// Day17. showdown 들어온 순간 카드 공개 
	if (P1WorldCard) {
		P1WorldCard->SetRevealState(true);
	}
	if (P2WorldCard) {
		P2WorldCard->SetRevealState(true);
	}

	const int32 P1Card = P1->HiddenCardValue;
	const int32 P2Card = P2->HiddenCardValue;

	UE_LOG(LogTemp, Warning, TEXT("[Showdown] Start - P1 Id=%d Card=%d | P2 Id=%d Card=%d | Pot=%d"),
		P1->GetPlayerId(),
		P1Card,
		P2->GetPlayerId(),
		P2Card,
		Pot
	);

	if (P1Card > P2Card)
	{
		P1->Chips += Pot;

		UE_LOG(LogTemp, Warning, TEXT("[Showdown] Winner=P1 Id=%d Awarded Pot=%d"), P1->GetPlayerId(), Pot);
		UE_LOG(LogTemp, Warning, TEXT("[Showdown] P1 Chips=%d | P2 Chips=%d"), P1->Chips, P2->Chips);
		LastActionText = FString::Printf(TEXT("Showdown - Player %d Wins"), P1->GetPlayerId());

		Pot = 0;
	}
	else if (P2Card > P1Card)
	{
		P2->Chips += Pot;

		UE_LOG(LogTemp, Warning, TEXT("[Showdown] Winner=P2 Id=%d Awarded Pot=%d"), P2->GetPlayerId(), Pot);
		UE_LOG(LogTemp, Warning, TEXT("[Showdown] P1 Chips=%d | P2 Chips=%d"), P1->Chips, P2->Chips);
		LastActionText = FString::Printf(TEXT("Showdown - Player %d Wins"), P2->GetPlayerId());

		Pot = 0;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Showdown] Draw - Pot carries over. Pot=%d"), Pot);
		UE_LOG(LogTemp, Warning, TEXT("[Showdown] P1 Chips=%d | P2 Chips=%d"), P1->Chips, P2->Chips);
		LastActionText = TEXT("Showdown - Draw");
	}

	// Day14. Showdown에서 Pot이 0으로 바뀌어도 그 직후 동기화가 없어서 HUD 반영이 늦을 수 있어서 넣어줌
	SyncRoundStateToGameState();

	AdvanceAfterRound(3.0);
}

void AIndianPokerGameModeBase::AdvanceAfterRound(float delay)
{
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;

	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AdvanceAfterRound] Failed - GetCachedRoundPlayers failed"));
		return;
	}

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AdvanceAfterRound] Failed - P1 or P2 is null"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[AdvanceAfterRound] Begin"));
	UE_LOG(LogTemp, Warning, TEXT("[AdvanceAfterRound] Cached P1=%p P2=%p"), RoundP1PS.Get(), RoundP2PS.Get());
	UE_LOG(LogTemp, Warning, TEXT("[AdvanceAfterRound] P1 Id=%d Chips=%d | P2 Id=%d Chips=%d | Pot=%d"),
		P1->GetPlayerId(), P1->Chips,
		P2->GetPlayerId(), P2->Chips,
		Pot);

	if (IsMatchEnded())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AdvanceAfterRound] MatchEnd condition met"));
		HandleMatchEnd();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[AdvanceAfterRound] Match continues -> Start next round"));

	// 상황에 따라 너무 빠르게 다음 라운드가 시작되는 것 방지
	GetWorldTimerManager().ClearTimer(NextRoundTimerHandle);
	GetWorldTimerManager().SetTimer(
		NextRoundTimerHandle,
		this,
		&AIndianPokerGameModeBase::TryStartRound,
		delay,
		false
	);
	//TryStartRound();
}

bool AIndianPokerGameModeBase::IsMatchEnded()
{
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;

	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchEnd] IsMatchEnded failed - GetCachedRoundPlayers failed"));
		return false;
	}

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchEnd] IsMatchEnded failed - P1 or P2 is null"));
		return false;
	}

	return (P1->Chips <= 0 || P2->Chips <= 0);
}

void AIndianPokerGameModeBase::HandleMatchEnd()
{
	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;

	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchEnd] Failed - GetCachedRoundPlayers failed"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[AdvanceAfterRound] Cached P1=%p P2=%p"), RoundP1PS.Get(), RoundP2PS.Get());

	if (!P1 || !P2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchEnd] Failed - P1 or P2 is null"));
		return;
	}

	const int32 WinnerId = (P1->Chips > P2->Chips) ? P1->GetPlayerId() : P2->GetPlayerId();
	LastActionText = FString::Printf(TEXT("Match End - Player %d Wins"), WinnerId);

	AuthCurrentActorPlayerId = INDEX_NONE;
	SyncRoundStateToGameState();

	SetPhaseServer(EGamePhase::MatchEnd);

	UE_LOG(LogTemp, Warning, TEXT("[MatchEnd] Match Ended"));
	UE_LOG(LogTemp, Warning, TEXT("[MatchEnd] P1 Id=%d Chips=%d | P2 Id=%d Chips=%d"),
		P1->GetPlayerId(), P1->Chips,
		P2->GetPlayerId(), P2->Chips);
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

// 이제 미사용
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

AIndianPokerPlayerState* AIndianPokerGameModeBase::FindRoundPlayerStateById(int32 PlayerId)
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

//bool AIndianPokerGameModeBase::EnsureMatchPlayersCached()
//{
//	UE_LOG(LogTemp, Warning, TEXT("[EnsureMatchPlayersCached] Before - P1=%p P2=%p"), RoundP1PS.Get(), RoundP2PS.Get());
//
//	if (RoundP1PS && RoundP2PS)
//	{
//		return true;
//	}
//
//	AIndianPokerGameStateBase* GS = GetIndianPokerGameState();
//	if (!GS)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Failed - GameState is null"));
//		return false;
//	}
//
//	if (GS->PlayerArray.Num() != 2)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Failed - PlayerArray Num=%d"), GS->PlayerArray.Num());
//		return false;
//	}
//
//	RoundP1PS = Cast<AIndianPokerPlayerState>(GS->PlayerArray[0]);
//	RoundP2PS = Cast<AIndianPokerPlayerState>(GS->PlayerArray[1]);
//
//	if (!RoundP1PS || !RoundP2PS)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Failed - RoundP1PS or RoundP2PS is null"));
//		return false;
//	}
//
//	UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Cached P1=%s Id=%d | P2=%s Id=%d"),
//		*GetNameSafe(RoundP1PS), RoundP1PS->GetPlayerId(),
//		*GetNameSafe(RoundP2PS), RoundP2PS->GetPlayerId());
//
//	return true;
//}

//bool AIndianPokerGameModeBase::EnsureMatchPlayersCached()
//{
//	UE_LOG(LogTemp, Warning, TEXT("[EnsureMatchPlayersCached] Before - P1=%s Ptr=%p Id=%d | P2=%s Ptr=%p Id=%d"),
//		*GetNameSafe(RoundP1PS), RoundP1PS.Get(), RoundP1PS ? RoundP1PS->GetPlayerId() : -1,
//		*GetNameSafe(RoundP2PS), RoundP2PS.Get(), RoundP2PS ? RoundP2PS->GetPlayerId() : -1);
//
//	const bool bP1Valid = IsValid(RoundP1PS) && RoundP1PS->GetPlayerId() != INDEX_NONE;
//	const bool bP2Valid = IsValid(RoundP2PS) && RoundP2PS->GetPlayerId() != INDEX_NONE;
//
//	if (bP1Valid && bP2Valid)
//	{
//		return true;
//	}
//
//	RoundP1PS = nullptr;
//	RoundP2PS = nullptr;
//
//	AIndianPokerGameStateBase* GS = GetIndianPokerGameState();
//	if (!GS)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Failed - GameState is null"));
//		return false;
//	}
//
//	TArray<AIndianPokerPlayerState*> ValidPlayers;
//
//	for (APlayerState* PSBase : GS->PlayerArray)
//	{
//		AIndianPokerPlayerState* PS = Cast<AIndianPokerPlayerState>(PSBase);
//		if (!IsValid(PS))
//		{
//			continue;
//		}
//
//		if (PS->GetPlayerId() == INDEX_NONE)
//		{
//			continue;
//		}
//
//		// 필요하면 이 줄은 잠깐 빼고 테스트 가능
//		if (!PS->GetPlayerController())
//		{
//			UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Skip PS=%s Id=%d - no PlayerController"),
//				*GetNameSafe(PS), PS->GetPlayerId());
//			continue;
//		}
//
//		ValidPlayers.Add(PS);
//
//		UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Candidate PS=%s Ptr=%p Id=%d PC=%s"),
//			*GetNameSafe(PS),
//			PS,
//			PS->GetPlayerId(),
//			*GetNameSafe(PS->GetPlayerController()));
//	}
//
//	if (ValidPlayers.Num() != 2)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Failed - ValidPlayers Num=%d"), ValidPlayers.Num());
//		return false;
//	}
//
//	RoundP1PS = ValidPlayers[0];
//	RoundP2PS = ValidPlayers[1];
//
//	UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Rebuilt P1=%s Ptr=%p Id=%d | P2=%s Ptr=%p Id=%d"),
//		*GetNameSafe(RoundP1PS), RoundP1PS.Get(), RoundP1PS ? RoundP1PS->GetPlayerId() : -1,
//		*GetNameSafe(RoundP2PS), RoundP2PS.Get(), RoundP2PS ? RoundP2PS->GetPlayerId() : -1);
//
//	return IsValid(RoundP1PS) && IsValid(RoundP2PS);
//}

//bool AIndianPokerGameModeBase::EnsureMatchPlayersCached()
//{
//	UE_LOG(LogTemp, Warning, TEXT("[EnsureMatchPlayersCached] Before - P1=%s Ptr=%p Id=%d | P2=%s Ptr=%p Id=%d"),
//		*GetNameSafe(RoundP1PS), RoundP1PS.Get(), RoundP1PS ? RoundP1PS->GetPlayerId() : -1,
//		*GetNameSafe(RoundP2PS), RoundP2PS.Get(), RoundP2PS ? RoundP2PS->GetPlayerId() : -1);
//
//	const bool bP1Valid = IsValid(RoundP1PS) && RoundP1PS->GetPlayerId() != INDEX_NONE;
//	const bool bP2Valid = IsValid(RoundP2PS) && RoundP2PS->GetPlayerId() != INDEX_NONE;
//
//	if (bP1Valid && bP2Valid)
//	{
//		return true;
//	}
//
//	RoundP1PS = nullptr;
//	RoundP2PS = nullptr;
//
//	AIndianPokerPlayerState* GatheredP1 = nullptr;
//	AIndianPokerPlayerState* GatheredP2 = nullptr;
//
//	if (!GatherReadyMatchPlayersFromControllers(GatheredP1, GatheredP2))
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Failed - GatherReadyMatchPlayersFromControllers failed"));
//		return false;
//	}
//
//	if (!IsValid(GatheredP1) || !IsValid(GatheredP2))
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Failed - Gathered players invalid"));
//		return false;
//	}
//
//	RoundP1PS = GatheredP1;
//	RoundP2PS = GatheredP2;
//
//	UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Rebuilt P1=%s Ptr=%p Id=%d | P2=%s Ptr=%p Id=%d"),
//		*GetNameSafe(RoundP1PS), RoundP1PS.Get(), RoundP1PS ? RoundP1PS->GetPlayerId() : -1,
//		*GetNameSafe(RoundP2PS), RoundP2PS.Get(), RoundP2PS ? RoundP2PS->GetPlayerId() : -1);
//
//	return IsValid(RoundP1PS) && IsValid(RoundP2PS);
//}
bool AIndianPokerGameModeBase::EnsureMatchPlayersCached()
{
	UE_LOG(LogTemp, Warning, TEXT("[EnsureMatchPlayersCached] Before - P1=%s Ptr=%p Id=%d | P2=%s Ptr=%p Id=%d"),
		*GetNameSafe(RoundP1PS), RoundP1PS.Get(), RoundP1PS ? RoundP1PS->GetPlayerId() : -1,
		*GetNameSafe(RoundP2PS), RoundP2PS.Get(), RoundP2PS ? RoundP2PS->GetPlayerId() : -1);

	AIndianPokerPlayerState* GatheredP1 = nullptr;
	AIndianPokerPlayerState* GatheredP2 = nullptr;

	/*if (!GatherReadyMatchPlayersFromControllers(GatheredP1, GatheredP2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Failed - GatherReadyMatchPlayersFromControllers failed"));
		RoundP1PS = nullptr;
		RoundP2PS = nullptr;
		return false;
	}*/
	// Day19. PvE에서는 참가자 캐시가 Bot 포함으로 잡히도록 BuildMatchParticipants 함수 기반으로 수정)
	if (!BuildMatchParticipants(GatheredP1, GatheredP2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Failed - BuildMatchParticipants failed"));
		RoundP1PS = nullptr;
		RoundP2PS = nullptr;
		return false;
	}

	if (!IsValid(GatheredP1) || !IsValid(GatheredP2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Failed - Gathered players invalid"));
		RoundP1PS = nullptr;
		RoundP2PS = nullptr;
		return false;
	}

	const bool bCacheMatchesGathered =
		(RoundP1PS == GatheredP1 && RoundP2PS == GatheredP2) ||
		(RoundP1PS == GatheredP2 && RoundP2PS == GatheredP1);

	if (bCacheMatchesGathered)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Cache already matches gathered players"));
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Cache mismatch detected -> recache"));
	UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Gathered P1=%s Ptr=%p Id=%d | Gathered P2=%s Ptr=%p Id=%d"),
		*GetNameSafe(GatheredP1), GatheredP1, GatheredP1 ? GatheredP1->GetPlayerId() : -1,
		*GetNameSafe(GatheredP2), GatheredP2, GatheredP2 ? GatheredP2->GetPlayerId() : -1);

	RoundP1PS = GatheredP1;
	RoundP2PS = GatheredP2;

	UE_LOG(LogTemp, Warning, TEXT("[MatchCache] Rebuilt P1=%s Ptr=%p Id=%d | P2=%s Ptr=%p Id=%d"),
		*GetNameSafe(RoundP1PS), RoundP1PS.Get(), RoundP1PS ? RoundP1PS->GetPlayerId() : -1,
		*GetNameSafe(RoundP2PS), RoundP2PS.Get(), RoundP2PS ? RoundP2PS->GetPlayerId() : -1);

	return true;
}

//bool AIndianPokerGameModeBase::GetCachedRoundPlayers(
//	AIndianPokerPlayerState*& OutP1,
//	AIndianPokerPlayerState*& OutP2)
//{
//	OutP1 = RoundP1PS;
//	OutP2 = RoundP2PS;
//
//	UE_LOG(LogTemp, Warning, TEXT("[GetCachedRoundPlayers] RoundP1PS=%s Ptr=%p Id=%d | RoundP2PS=%s Ptr=%p Id=%d"),
//		*GetNameSafe(RoundP1PS), RoundP1PS.Get(), RoundP1PS ? RoundP1PS->GetPlayerId() : -1,
//		*GetNameSafe(RoundP2PS), RoundP2PS.Get(), RoundP2PS ? RoundP2PS->GetPlayerId() : -1
//	);
//
//	return (OutP1 && OutP2);
//}

//bool AIndianPokerGameModeBase::GetCachedRoundPlayers(
//	AIndianPokerPlayerState*& OutP1,
//	AIndianPokerPlayerState*& OutP2)
//{
//	OutP1 = nullptr;
//	OutP2 = nullptr;
//
//	const bool bP1Valid = IsValid(RoundP1PS) && RoundP1PS->GetPlayerId() != INDEX_NONE;
//	const bool bP2Valid = IsValid(RoundP2PS) && RoundP2PS->GetPlayerId() != INDEX_NONE;
//
//	if (!bP1Valid || !bP2Valid)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[GetCachedRoundPlayers] Cache invalid -> rebuild attempt"));
//		if (!EnsureMatchPlayersCached())
//		{
//			UE_LOG(LogTemp, Warning, TEXT("[GetCachedRoundPlayers] Rebuild failed"));
//			return false;
//		}
//	}
//
//	OutP1 = RoundP1PS;
//	OutP2 = RoundP2PS;
//
//	UE_LOG(LogTemp, Warning, TEXT("[GetCachedRoundPlayers] P1=%s Ptr=%p Id=%d | P2=%s Ptr=%p Id=%d"),
//		*GetNameSafe(OutP1), OutP1, OutP1 ? OutP1->GetPlayerId() : -1,
//		*GetNameSafe(OutP2), OutP2, OutP2 ? OutP2->GetPlayerId() : -1);
//
//	return IsValid(OutP1) && IsValid(OutP2);
//}

bool AIndianPokerGameModeBase::GetCachedRoundPlayers(
	AIndianPokerPlayerState*& OutP1,
	AIndianPokerPlayerState*& OutP2)
{
	OutP1 = nullptr;
	OutP2 = nullptr;

	if (!EnsureMatchPlayersCached())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GetCachedRoundPlayers] EnsureMatchPlayersCached failed"));
		return false;
	}

	OutP1 = RoundP1PS;
	OutP2 = RoundP2PS;

	UE_LOG(LogTemp, Warning, TEXT("[GetCachedRoundPlayers] P1=%s Ptr=%p Id=%d | P2=%s Ptr=%p Id=%d"),
		*GetNameSafe(OutP1), OutP1, OutP1 ? OutP1->GetPlayerId() : -1,
		*GetNameSafe(OutP2), OutP2, OutP2 ? OutP2->GetPlayerId() : -1);

	return IsValid(OutP1) && IsValid(OutP2);
}

// 인간 플레이어용 사람2명 찾기 함수
bool AIndianPokerGameModeBase::GatherReadyMatchPlayersFromControllers(
	AIndianPokerPlayerState*& OutP1,
	AIndianPokerPlayerState*& OutP2)
{
	OutP1 = nullptr;
	OutP2 = nullptr;

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GatherReadyPlayers] Failed - not authority"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GatherReadyPlayers] Failed - World is null"));
		return false;
	}

	TArray<AIndianPokerPlayerState*> ReadyPlayers;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* BasePC = It->Get();
		AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(BasePC);
		if (!IsValid(PC))
		{
			continue;
		}

		AIndianPokerPlayerState* PS = PC->GetPlayerState<AIndianPokerPlayerState>();
		if (!IsValid(PS))
		{
			UE_LOG(LogTemp, Warning, TEXT("[GatherReadyPlayers] Skip PC=%s - PlayerState invalid"),
				*GetNameSafe(PC));
			continue;
		}

		if (PS->GetPlayerId() == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GatherReadyPlayers] Skip PS=%s - PlayerId invalid"),
				*GetNameSafe(PS));
			continue;
		}
		if (ReadyPlayers.Contains(PS))
		{
			UE_LOG(LogTemp, Warning, TEXT("[GatherReadyPlayers] Skip duplicate PS=%s Ptr=%p Id=%d"),
				*GetNameSafe(PS),
				PS,
				PS->GetPlayerId());
			continue;
		}

		ReadyPlayers.Add(PS);

		UE_LOG(LogTemp, Warning, TEXT("[GatherReadyPlayers] Candidate PC=%s | PS=%s Ptr=%p Id=%d | IsLocal=%d"),
			*GetNameSafe(PC),
			*GetNameSafe(PS),
			PS,
			PS->GetPlayerId(),
			PC->IsLocalController() ? 1 : 0);
	}

	if (ReadyPlayers.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GatherReadyPlayers] Failed - ReadyPlayers Num=%d"), ReadyPlayers.Num());
		return false;
	}

	OutP1 = ReadyPlayers[0];
	OutP2 = ReadyPlayers[1];

	UE_LOG(LogTemp, Warning, TEXT("[GatherReadyPlayers] Success - P1=%s Id=%d | P2=%s Id=%d"),
		*GetNameSafe(OutP1), OutP1 ? OutP1->GetPlayerId() : -1,
		*GetNameSafe(OutP2), OutP2 ? OutP2->GetPlayerId() : -1);

	return true;
}

bool AIndianPokerGameModeBase::BuildMatchParticipants(
	AIndianPokerPlayerState*& OutP1,
	AIndianPokerPlayerState*& OutP2)
{
	OutP1 = nullptr;
	OutP2 = nullptr;

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BuildMatchParticipants] Failed - not authority"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BuildMatchParticipants] Failed - World is null"));
		return false;
	}

	TArray<AIndianPokerPlayerState*> HumanPlayers;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* BasePC = It->Get();
		AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(BasePC);
		if (!IsValid(PC))
		{
			continue;
		}

		AIndianPokerPlayerState* PS = PC->GetPlayerState<AIndianPokerPlayerState>();
		if (!IsValid(PS))
		{
			continue;
		}

		if (PS->GetPlayerId() == INDEX_NONE)
		{
			continue;
		}

		if (HumanPlayers.Contains(PS))
		{
			continue;
		}

		HumanPlayers.Add(PS);

		UE_LOG(LogTemp, Warning, TEXT("[BuildMatchParticipants] Human candidate=%s Ptr=%p Id=%d"),
			*GetNameSafe(PS), PS, PS->GetPlayerId());
	}

	if (CurrentMatchMode == EIndianPokerMatchMode::PvP)
	{
		if (HumanPlayers.Num() != 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BuildMatchParticipants] PvP failed - HumanPlayers Num=%d"),
				HumanPlayers.Num());
			return false;
		}

		OutP1 = HumanPlayers[0];
		OutP2 = HumanPlayers[1];
	}
	else if (CurrentMatchMode == EIndianPokerMatchMode::PvE)
	{
		if (HumanPlayers.Num() < 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BuildMatchParticipants] PvE failed - no human player"));
			return false;
		}

		if (!IsValid(BotPlayerState) || !BotPlayerState->IsBot())
		{
			UE_LOG(LogTemp, Warning, TEXT("[BuildMatchParticipants] PvE failed - BotPlayerState invalid"));
			return false;
		}

		OutP1 = HumanPlayers[0];
		OutP2 = BotPlayerState;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BuildMatchParticipants] Failed - unknown match mode"));
		return false;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[BuildMatchParticipants] Success | P1=%s Id=%d Bot=%d | P2=%s Id=%d Bot=%d"),
		*GetNameSafe(OutP1),
		OutP1 ? OutP1->GetPlayerId() : -1,
		OutP1 ? (OutP1->IsBot() ? 1 : 0) : 0,
		*GetNameSafe(OutP2),
		OutP2 ? OutP2->GetPlayerId() : -1,
		OutP2 ? (OutP2->IsBot() ? 1 : 0) : 0);

	return IsValid(OutP1) && IsValid(OutP2);
}

AIndianPokerPlayerController* AIndianPokerGameModeBase::FindControllerByPlayerState(AIndianPokerPlayerState* TargetPS) const
{
	if (!IsValid(TargetPS))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FindControllerByPlayerState] Failed - TargetPS invalid"));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FindControllerByPlayerState] Failed - World is null"));
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* BasePC = It->Get();
		AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(BasePC);
		if (!IsValid(PC))
		{
			continue;
		}

		AIndianPokerPlayerState* PS = PC->GetPlayerState<AIndianPokerPlayerState>();
		if (!IsValid(PS))
		{
			continue;
		}

		if (PS == TargetPS)
		{
			UE_LOG(LogTemp, Warning, TEXT("[FindControllerByPlayerState] Found PC=%s for PS=%s Ptr=%p Id=%d"),
				*GetNameSafe(PC),
				*GetNameSafe(TargetPS),
				TargetPS,
				TargetPS->GetPlayerId());

			return PC;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[FindControllerByPlayerState] Failed - no PC found for PS=%s Ptr=%p Id=%d"),
		*GetNameSafe(TargetPS),
		TargetPS,
		TargetPS ? TargetPS->GetPlayerId() : -1);

	return nullptr;
}

AIndianPokerPlayerState* AIndianPokerGameModeBase::GetPlayerStateByPlayerId(int32 PlayerId) const
{
	if (RoundP1PS && RoundP1PS->GetPlayerId() == PlayerId)
	{
		return RoundP1PS;
	}

	if (RoundP2PS && RoundP2PS->GetPlayerId() == PlayerId)
	{
		return RoundP2PS;
	}

	UE_LOG(LogTemp, Warning, TEXT("[GetPlayerStateByPlayerId] No match for PlayerId=%d"), PlayerId);
	return nullptr;
}

AIndianPokerPlayerState* AIndianPokerGameModeBase::GetOpponentPlayerStateByPlayerId(int32 PlayerId) const
{
	if (RoundP1PS && RoundP1PS->GetPlayerId() == PlayerId)
	{
		return RoundP2PS;
	}

	if (RoundP2PS && RoundP2PS->GetPlayerId() == PlayerId)
	{
		return RoundP1PS;
	}

	UE_LOG(LogTemp, Warning, TEXT("[GetOpponentPlayerStateByPlayerId] No opponent match for PlayerId=%d"), PlayerId);
	return nullptr;
}

int32 AIndianPokerGameModeBase::CalcRequiredToCall(int32 RequestPlayerId) const
{
	AIndianPokerPlayerState* MyPS = GetPlayerStateByPlayerId(RequestPlayerId);
	AIndianPokerPlayerState* OpponentPS = GetOpponentPlayerStateByPlayerId(RequestPlayerId);

	if (!MyPS || !OpponentPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CalcRequiredToCall] Invalid PlayerState for PlayerId=%d"), RequestPlayerId);
		return 0;
	}

	const int32 RawRequired = OpponentPS->CurrentRoundContribution - MyPS->CurrentRoundContribution;
	const int32 FinalRequired = FMath::Max(0, RawRequired);

	UE_LOG(LogTemp, Warning,
		TEXT("[CalcRequiredToCall] RequestPlayerId=%d | MyContrib=%d | OppContrib=%d | Required=%d"),
		RequestPlayerId,
		MyPS->CurrentRoundContribution,
		OpponentPS->CurrentRoundContribution,
		FinalRequired);

	return FinalRequired;
}

void AIndianPokerGameModeBase::CacheWorldCardActors()
{
	UE_LOG(LogTemp, Warning, TEXT("[Card] CacheWorldCardActors called."));

	P1WorldCard = nullptr;
	P2WorldCard = nullptr;

	TArray<AActor*> FoundCards;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACardActor::StaticClass(), FoundCards);

	UE_LOG(LogTemp, Warning, TEXT("[Card] Found %d card actors in world."), FoundCards.Num());

	for (AActor* Actor : FoundCards)
	{
		ACardActor* Card = Cast<ACardActor>(Actor);
		if (!Card)
		{
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("[Card] Inspecting %s | CardSlotId=%d"),
			*Card->GetName(), Card->CardSlotId);

		if (Card->CardSlotId == 0)
		{
			P1WorldCard = Card;
			UE_LOG(LogTemp, Warning, TEXT("[Card] Cached P1WorldCard: %s"), *Card->GetName());
		}
		else if (Card->CardSlotId == 1)
		{
			P2WorldCard = Card;
			UE_LOG(LogTemp, Warning, TEXT("[Card] Cached P2WorldCard: %s"), *Card->GetName());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Card] Cache result | P1=%s | P2=%s"),
		P1WorldCard ? *P1WorldCard->GetName() : TEXT("Null"),
		P2WorldCard ? *P2WorldCard->GetName() : TEXT("Null"));

	if (!P1WorldCard || !P2WorldCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Card] Failed to cache one or more world cards."));
	}
}

UTexture2D* AIndianPokerGameModeBase::GetFrontTextureForCardValue(int32 CardValue) const
{
	const int32 Index = CardValue - 1;

	if (!CardFrontTextures.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Card] Invalid card value for texture lookup: %d"), CardValue);
		return nullptr;
	}

	return CardFrontTextures[Index];
}

void AIndianPokerGameModeBase::UpdateWorldCardVisuals()
{
	if (!RoundP1PS || !RoundP2PS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Card] Round player states are invalid."));
		return;
	}

	if (!P1WorldCard || !P2WorldCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Card] World card actors are not cached."));
		return;
	}

	const int32 P1CardValue = RoundP1PS->HiddenCardValue;
	const int32 P2CardValue = RoundP2PS->HiddenCardValue;

	/*UTexture2D* P1FrontTexture = GetFrontTextureForCardValue(P1CardValue);
	UTexture2D* P2FrontTexture = GetFrontTextureForCardValue(P2CardValue);

	if (P1FrontTexture)
	{
		P1WorldCard->BP_SetCardFrontOnly(P1FrontTexture);
	}

	if (P2FrontTexture)
	{
		P2WorldCard->BP_SetCardFrontOnly(P2FrontTexture);
	}*/

	// GameMode는 더 이상 텍스처를 직접 넘길 필요가 없으므로 이렇게 변경
	P1WorldCard->SetCardValueServer(P1CardValue);
	P2WorldCard->SetCardValueServer(P2CardValue);

	UE_LOG(LogTemp, Warning, TEXT("[Card] Updated world card visuals | P1=%d, P2=%d"), P1CardValue, P2CardValue);
}

void AIndianPokerGameModeBase::InitializeMatchModeFromSessionSubsystem()
{
	if (!HasAuthority())
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] InitializeMatchModeFromSessionSubsystem - GameInstance is null"));
		return;
	}

	UIndianPokerSessionSubsystem* SessionSubsystem = GI->GetSubsystem<UIndianPokerSessionSubsystem>();
	if (!SessionSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] InitializeMatchModeFromSessionSubsystem - SessionSubsystem is null"));
		return;
	}

	CurrentMatchMode = SessionSubsystem->GetSelectedMatchMode();

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] CurrentMatchMode = %s"),
		CurrentMatchMode == EIndianPokerMatchMode::PvE ? TEXT("PvE") : TEXT("PvP"));
}

//void AIndianPokerGameModeBase::EnsureBotParticipantCreated()
//{
//	if (!HasAuthority())
//	{
//		return;
//	}
//
//	if (CurrentMatchMode != EIndianPokerMatchMode::PvE)
//	{
//		return;
//	}
//
//	if (IsValid(BotPlayerState))
//	{
//		return;
//	}
//
//	BotPlayerState = NewObject<AIndianPokerPlayerState>(this, AIndianPokerPlayerState::StaticClass());
//
//	if (!BotPlayerState)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Failed to create BotPlayerState"));
//		return;
//	}
//
//	BotPlayerState->SetPlayerId(999);
//	BotPlayerState->SetPlayerName(TEXT("Bot"));
//	BotPlayerState->SetIsBot(true);
//
//	UE_LOG(LogTemp, Warning, TEXT("[GameMode] BotPlayerState created. PlayerId=%d Name=%s"),
//		BotPlayerState->GetPlayerId(),
//		*BotPlayerState->GetPlayerName());
//}

void AIndianPokerGameModeBase::EnsureBotParticipantCreated()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentMatchMode != EIndianPokerMatchMode::PvE)
	{
		return;
	}

	if (IsValid(BotPlayerState))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] EnsureBotParticipantCreated - World is null"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	BotPlayerState = World->SpawnActor<AIndianPokerPlayerState>(
		AIndianPokerPlayerState::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams);

	if (!BotPlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Failed to spawn BotPlayerState"));
		return;
	}

	BotPlayerState->SetPlayerId(9999);
	BotPlayerState->SetPlayerName(TEXT("Bot"));
	BotPlayerState->SetIsBot(true);

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] BotPlayerState spawned. PlayerId=%d Name=%s"),
		BotPlayerState->GetPlayerId(),
		*BotPlayerState->GetPlayerName());
}

bool AIndianPokerGameModeBase::IsCurrentActorBot() const
{
	if (!IsValid(BotPlayerState))
	{
		return false;
	}

	return AuthCurrentActorPlayerId == BotPlayerState->GetPlayerId();
}

void AIndianPokerGameModeBase::TryScheduleBotTurn()
{
	if (!HasAuthority())
	{
		return;
	}

	AIndianPokerGameStateBase* GS = GetIndianPokerGameState();
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BotAI] TryScheduleBotTurn failed - GameState is null"));
		return;
	}

	if (GS->GetCurrentPhase() != EGamePhase::Betting)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BotAI] TryScheduleBotTurn skipped - Phase is not Betting"));
		return;
	}

	if (!IsCurrentActorBot())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BotAI] Bot turn detected. Schedule ExecuteBotTurn"));

	GetWorldTimerManager().ClearTimer(BotTurnTimerHandle);
	GetWorldTimerManager().SetTimer(
		BotTurnTimerHandle,
		this,
		&AIndianPokerGameModeBase::ExecuteBotTurn,
		1.5f,
		false
	);
}

EBettingActionType AIndianPokerGameModeBase::DecideBotAction(int32& OutRaiseExtra) const
{
	OutRaiseExtra = 1;

	if (!IsValid(BotPlayerState))
	{
		return EBettingActionType::Fold;
	}

	const int32 OpponentCard = BotPlayerState->VisibleOpponentCardValue;
	const int32 RequiredAmount = CalcRequiredToCall(BotPlayerState->GetPlayerId());

	int32 Roll = FMath::RandRange(1, 100);

	UE_LOG(LogTemp, Warning, TEXT("[BotAI] DecideBotAction | OpponentCard=%d Required=%d Roll=%d"),
		OpponentCard, RequiredAmount, Roll);

	// 콜 필요 없음
	if (RequiredAmount == 0)
	{
		const bool bUseCheckCall = bHasOpeningCheck;

		// 1~2, 10
		if ((OpponentCard >= 1 && OpponentCard <= 2) || OpponentCard == 10)
		{
			if (Roll <= 90)
			{
				return EBettingActionType::Raise;
			}
			return bUseCheckCall ? EBettingActionType::CheckCall : EBettingActionType::Check;
		}

		// 3~5
		if (OpponentCard >= 3 && OpponentCard <= 5)
		{
			if (Roll <= 70)
			{
				return EBettingActionType::Raise;
			}
			return bUseCheckCall ? EBettingActionType::CheckCall : EBettingActionType::Check;
		}

		// 6~7
		if (OpponentCard >= 6 && OpponentCard <= 7)
		{
			if (Roll <= 30)
			{
				return EBettingActionType::Raise;
			}
			return bUseCheckCall ? EBettingActionType::CheckCall : EBettingActionType::Check;
		}

		// 8~9
		if (OpponentCard >= 8 && OpponentCard <= 9)
		{
			if (Roll <= 10)
			{
				return EBettingActionType::Raise;
			}
			return bUseCheckCall ? EBettingActionType::CheckCall : EBettingActionType::Check;
		}

		return bUseCheckCall ? EBettingActionType::CheckCall : EBettingActionType::Check;
	}

	// 콜 필요 있음
	// 1~2, 10 : Raise 80 / Call 15 / Fold 5
	if ((OpponentCard >= 1 && OpponentCard <= 2) || OpponentCard == 10)
	{
		if (Roll <= 80) return EBettingActionType::Raise;
		if (Roll <= 95) return EBettingActionType::Call;
		return EBettingActionType::Fold;
	}

	// 3~5 : Raise 55 / Call 30 / Fold 15
	if (OpponentCard >= 3 && OpponentCard <= 5)
	{
		if (Roll <= 55) return EBettingActionType::Raise;
		if (Roll <= 85) return EBettingActionType::Call;
		return EBettingActionType::Fold;
	}

	// 6~7 : Raise 15 / Call 40 / Fold 45
	if (OpponentCard >= 6 && OpponentCard <= 7)
	{
		if (Roll <= 15) return EBettingActionType::Raise;
		if (Roll <= 55) return EBettingActionType::Call;
		return EBettingActionType::Fold;
	}

	// 8~9 : Raise 5 / Call 25 / Fold 70
	if (OpponentCard >= 8 && OpponentCard <= 9)
	{
		if (Roll <= 5) return EBettingActionType::Raise;
		if (Roll <= 30) return EBettingActionType::Call;
		return EBettingActionType::Fold;
	}

	return EBettingActionType::Fold;
}

void AIndianPokerGameModeBase::ExecuteBotTurn()
{
	if (!HasAuthority())
	{
		return;
	}

	AIndianPokerGameStateBase* GS = GetIndianPokerGameState();
	if (!GS || GS->GetCurrentPhase() != EGamePhase::Betting)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BotAI] ExecuteBotTurn aborted - not in Betting phase"));
		return;
	}

	if (!IsCurrentActorBot())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BotAI] ExecuteBotTurn aborted - current actor is not bot"));
		return;
	}

	AIndianPokerPlayerState* P1 = nullptr;
	AIndianPokerPlayerState* P2 = nullptr;
	if (!GetCachedRoundPlayers(P1, P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BotAI] ExecuteBotTurn failed - GetCachedRoundPlayers failed"));
		return;
	}

	AIndianPokerPlayerState* BotPS = nullptr;
	AIndianPokerPlayerState* OpponentPS = nullptr;

	if (P1 && P1->IsBot())
	{
		BotPS = P1;
		OpponentPS = P2;
	}
	else if (P2 && P2->IsBot())
	{
		BotPS = P2;
		OpponentPS = P1;
	}

	if (!BotPS || !OpponentPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BotAI] ExecuteBotTurn failed - BotPS or OpponentPS invalid"));
		return;
	}

	int32 RaiseExtra = 1;
	EBettingActionType ChosenAction = DecideBotAction(RaiseExtra);

	UE_LOG(LogTemp, Warning, TEXT("[BotAI] ChosenAction=%s RaiseExtra=%d"),
		*StaticEnum<EBettingActionType>()->GetNameStringByValue((int64)ChosenAction),
		RaiseExtra);

	bool bSuccess = false;

	switch (ChosenAction)
	{
	case EBettingActionType::Check:
		bSuccess = HandleAction_Check(BotPS, OpponentPS);
		break;

	case EBettingActionType::CheckCall:
		bSuccess = HandleAction_CheckCall(BotPS, OpponentPS);
		break;

	case EBettingActionType::Call:
		bSuccess = HandleAction_Call(BotPS, OpponentPS);
		break;

	case EBettingActionType::Raise:
		bSuccess = HandleAction_Raise(BotPS, OpponentPS, RaiseExtra);
		if (!bSuccess)
		{
			// Raise 실패 시 대체 행동
			if (CalcRequiredToCall(BotPS->GetPlayerId()) == 0)
			{
				bSuccess = bHasOpeningCheck
					? HandleAction_CheckCall(BotPS, OpponentPS)
					: HandleAction_Check(BotPS, OpponentPS);
			}
			else
			{
				bSuccess = HandleAction_Call(BotPS, OpponentPS);
				if (!bSuccess)
				{
					bSuccess = HandleAction_Fold(BotPS, OpponentPS);
				}
			}
		}
		break;

	case EBettingActionType::Fold:
		bSuccess = HandleAction_Fold(BotPS, OpponentPS);
		break;

	default:
		break;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BotAI] ExecuteBotTurn result = %s"),
		bSuccess ? TEXT("Success") : TEXT("Failed"));
}