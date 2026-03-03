// Fill out your copyright notice in the Description page of Project Settings.

#include "IndianPokerGameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "IndianPokerPlayerState.h"

AIndianPokerGameStateBase::AIndianPokerGameStateBase()
{
	// GameState는 기본적으로 Replicate 되지만, 명시해도 안전함
	bReplicates = true;
	CurrentPhase = EGamePhase::Lobby;
}

void AIndianPokerGameStateBase::BeginPlay()
{
	Super::BeginPlay();

	// <임시> 일단 시작 Phase 확인용(스팸 방지로 1회만)
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GS][BeginPlay][SERVER] Initial Phase=%d"), (int32)CurrentPhase);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GS][BeginPlay][CLIENT] Initial Phase=%d"), (int32)CurrentPhase);
	}
}

void AIndianPokerGameStateBase::SetPhaseServer(EGamePhase NewPhase)
{
	// 방어코드: 혹시라도 클라에서 호출되면 즉시 차단
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GS][SetPhaseServer] Blocked: called on CLIENT"));
		return;
	}

	if (CurrentPhase == NewPhase)
	{
		return;
	}

	CurrentPhase = NewPhase;

	// 서버에서 변경 로그 (1회)
	UE_LOG(LogTemp, Warning, TEXT("[GS][SERVER] Phase changed to %d"), (int32)CurrentPhase);

	// 서버는 OnRep가 자동 호출되지 않으니, 서버에서도 동일한 반응/로그가 필요하면 직접 호출 가능
	// OnRep_CurrentPhase();
}

void AIndianPokerGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AIndianPokerGameStateBase, CurrentPhase);
}

void AIndianPokerGameStateBase::OnRep_CurrentPhase()
{
	// 클라에서 동기화 되었는지 확인하는 핵심 로그(딱 1줄만 남기기)
	UE_LOG(LogTemp, Warning, TEXT("[GS][OnRep][CLIENT] Phase replicated: %d"), (int32)CurrentPhase);
}
