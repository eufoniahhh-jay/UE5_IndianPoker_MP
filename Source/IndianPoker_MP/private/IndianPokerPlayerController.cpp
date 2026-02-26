// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerPlayerController.h"
#include "Engine//Engine.h"

void AIndianPokerPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	const ENetMode NetMode = World ? World->GetNetMode() : NM_Standalone;

	const bool bAuth = HasAuthority();
	const bool bLocal = IsLocalController();

	const TCHAR* NetModeStr =
		(NetMode == NM_Standalone) ? TEXT("Standalone") :
		(NetMode == NM_ListenServer) ? TEXT("ListenServer") :
		(NetMode == NM_DedicatedServer) ? TEXT("DedicatedServer") :
		(NetMode == NM_Client) ? TEXT("Client") : TEXT("Unknown");

	UE_LOG(LogTemp, Warning, TEXT("[PC BeginPlay] NetMode=%s | HasAuthority=%d | IsLocal=%d | Name=%s"),
		NetModeStr, bAuth ? 1 : 0, bLocal ? 1 : 0, *GetName());

	if (GEngine)
	{
		const FString Msg = FString::Printf(TEXT("[PC] %s | Auth=%d Local=%d"),
			NetModeStr, bAuth ? 1 : 0, bLocal ? 1 : 0);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Msg);
	}
}
