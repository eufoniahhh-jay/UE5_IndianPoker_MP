// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerPlayerState.h"
#include "Net/UnrealNetwork.h"

void AIndianPokerPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AIndianPokerPlayerState, TestValue);
}

void AIndianPokerPlayerState::ServerSetTestValue(int32 NewValue)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PS][SET] BLOCKED on CLIENT | Name=%s"), *GetName());
		return;
	}

	TestValue = NewValue;

	UE_LOG(LogTemp, Warning, TEXT("[PS][SET] SERVER | %s | TestValue=%d"),
		*GetName(), TestValue);
}

void AIndianPokerPlayerState::OnRep_TestValue()
{
	UE_LOG(LogTemp, Warning, TEXT("[PS][ONREP] CLIENT? Auth=%d | %s | TestValue=%d"),
		HasAuthority() ? 1 : 0, *GetName(), TestValue);
}
