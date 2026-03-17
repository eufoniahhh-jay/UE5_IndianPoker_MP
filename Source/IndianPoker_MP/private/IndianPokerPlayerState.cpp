// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerPlayerState.h"
#include "Net/UnrealNetwork.h"

AIndianPokerPlayerState::AIndianPokerPlayerState()
{
	Chips = 10;
	HiddenCardValue = -1;
	VisibleOpponentCardValue = -1;
}

void AIndianPokerPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AIndianPokerPlayerState, Chips);
    DOREPLIFETIME(AIndianPokerPlayerState, VisibleOpponentCardValue);
    DOREPLIFETIME(AIndianPokerPlayerState, bFolded);
    DOREPLIFETIME(AIndianPokerPlayerState, CurrentRoundContribution);
}