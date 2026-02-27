// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerGameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"


AIndianPokerGameStateBase::AIndianPokerGameStateBase()
{
	// GameState는 기본적으로 Replicate 되지만, 명시해도 안전함
	bReplicates = true;
}

void AIndianPokerGameStateBase::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 값 변경(권위)
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			TestTimerHandle,
			this,
			&AIndianPokerGameStateBase::ServerTickTestNumber,
			1.0f,
			true
		);

		UE_LOG(LogTemp, Warning, TEXT("[GS][SETUP] Timer started on SERVER"));

		//// PlayerState들이 생성된 뒤에 한번 초기값 세팅
		//FTimerHandle InitHandle;
		//GetWorldTimerManager().SetTimer(
		//	InitHandle,
		//	this,
		//	&AIndianPokerGameStateBase::ServerInitPlayerStateTestValues,
		//	1.0f,
		//	false
		//);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GS][SETUP] CLIENT - timer not started"));
	}

	// 시작 시 현재 값 한번 찍기 (초기값 확인 용)
	//OnRep_TestNumber();
}

void AIndianPokerGameStateBase::ServerTickTestNumber()
{
	// 방어코드: 혹시라도 클라에서 호출되면 즉시 차단
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GS][SET] BLOCKED: called on CLIENT"));
		return;
	}

	TestNumber++;

	const ENetMode NetMode = GetWorld() ? GetWorld()->GetNetMode() : NM_Standalone;
	UE_LOG(LogTemp, Warning, TEXT("[GS][SET] NetMode=%d Auth=1 TestNumber=%d"), (int32)NetMode, TestNumber);

	// 리슨서버 창에서도 즉시 확인용(서버는 RepNotify가 안 불릴 수도 있어서 직접 출력)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(100, 1.2f, FColor::Green,
			FString::Printf(TEXT("[SERVER SET] TestNumber = %d"), TestNumber));
	}
}

void AIndianPokerGameStateBase::OnRep_TestNumber()
{
	const bool bAuth = HasAuthority();
	const ENetMode NetMode = GetWorld() ? GetWorld()->GetNetMode() : NM_Standalone;

	UE_LOG(LogTemp, Warning, TEXT("[GS][ONREP] NetMode=%d Auth=%d TestNumber=%d"), (int32)NetMode, bAuth ? 1 : 0, TestNumber);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(101, 1.2f, FColor::Yellow,
			FString::Printf(TEXT("[ONREP] Auth=%d TestNumber = %d"), bAuth ? 1 : 0, TestNumber));
	}
}

void AIndianPokerGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AIndianPokerGameStateBase, TestNumber);
}

#include "IndianPokerPlayerState.h"

//void AIndianPokerGameStateBase::ServerInitPlayerStateTestValues()
//{
//	if (!HasAuthority())
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[GS][PSINIT] BLOCKED on CLIENT"));
//		return;
//	}
//
//	UE_LOG(LogTemp, Warning, TEXT("[GS][PSINIT] SERVER init PlayerState test values. PlayerArray=%d"), PlayerArray.Num());
//
//	int32 Index = 0;
//	for (APlayerState* PS : PlayerArray)
//	{
//		if (AIndianPokerPlayerState* IPS = Cast<AIndianPokerPlayerState>(PS))
//		{
//			const int32 Value = (Index + 1) * 100; // 100, 200...
//			IPS->ServerSetTestValue(Value);
//
//			UE_LOG(LogTemp, Warning, TEXT("[GS][PSINIT] Set %s => %d"), *IPS->GetName(), Value);
//			Index++;
//		}
//	}
//}