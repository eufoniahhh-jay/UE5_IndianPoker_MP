// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerPlayerController.h"
#include "Engine/Engine.h"
#include "IndianPokerPlayerState.h"
#include "IndianPokerGameStateBase.h"
#include "GameFramework/GameStateBase.h"

void AIndianPokerPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	const ENetMode NetMode = World ? World->GetNetMode() : NM_Standalone;
	FString MapName = World ? World->GetMapName() : TEXT("NoWorld");

	const bool bAuth = HasAuthority();
	const bool bLocal = IsLocalController();

	// PIE 인스턴스 ID (멀티 PIE에서 서버=0, 클라=1... 식으로 구분됨)
	const int32 PieId = GPlayInEditorID;

	const TCHAR* NetModeStr =
		(NetMode == NM_Standalone) ? TEXT("Standalone") :
		(NetMode == NM_ListenServer) ? TEXT("ListenServer") :
		(NetMode == NM_DedicatedServer) ? TEXT("DedicatedServer") :
		(NetMode == NM_Client) ? TEXT("Client") : TEXT("Unknown");

	UE_LOG(LogTemp, Warning, TEXT("[PC BeginPlay] Map=%s | NetMode=%s | HasAuthority=%d | IsLocal=%d | Name=%s"),
		*MapName, NetModeStr, bAuth ? 1 : 0, bLocal ? 1 : 0, *GetName());

	// Travel 후 PlayerState 유지되는지 확인용 디버깅
	AIndianPokerPlayerState* PS = Cast<AIndianPokerPlayerState>(PlayerState);
	if (PS)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[PC][GameMapCheck] PS=%s | TestValue=%d"),
		//	*PS->GetName(),
		//	//*PS->GetUniqueId().ToString(),
		//	PS->GetTestValue());   // Getter 함수 사용
		UE_LOG(LogTemp, Warning,
			TEXT("[PC][PSCHECK] Map=%s | PS=%s | PSAddr=%p | TestValue=%d"),
			*MapName,
			*GetNameSafe(PS),
			PS,
			PS ? PS->GetTestValue() : -1
		);
	}

	// 약간 딜레이 후, PlayerState 출력 디버깅 (PlayerState 동기화 보장)
	// PlayerState가 네트워크로 완전히 동기화 된 뒤에 출력하기 위해 딜레이 주는 것
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(
		Handle,
		this,
		&AIndianPokerPlayerController::DebugPrintPlayerStates,
		1.5f,
		false
	);

	if (GEngine)
	{
		// const FString Msg = FString::Printf(TEXT("[PC] %s | Auth=%d Local=%d"),
		// 	NetModeStr, bAuth ? 1 : 0, bLocal ? 1 : 0);
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Msg);

		// 같은 키로 덮어써서 “누적(-1)”로 인한 혼선을 줄임
		// PIE 인스턴스/로컬 여부에 따라 키를 다르게 주면 더 깔끔함
		const int32 MsgKey = 1000 + PieId * 10 + (bLocal ? 1 : 0);

		const FString Msg = FString::Printf(
			TEXT("[PC][PIE=%d] %s | Auth=%d Local=%d"),
			PieId, NetModeStr, bAuth ? 1 : 0, bLocal ? 1 : 0);

		GEngine->AddOnScreenDebugMessage(MsgKey, 5.f, FColor::Yellow, Msg);
	}

	/*if (GEngine && IsLocalController())
	{
		const int32 MsgKey = 1000 + PieId; // 로컬 1줄만
		const FString Msg = FString::Printf(TEXT("[PC][PIE=%d] %s | Auth=%d Local=%d"),
			PieId, NetModeStr, bAuth ? 1 : 0, 1);

		GEngine->AddOnScreenDebugMessage(MsgKey, 5.f, FColor::Yellow, Msg);
	}*/
}

void AIndianPokerPlayerController::DebugPrintPlayerStates()
{
	AIndianPokerPlayerState* MyPS = GetPlayerState<AIndianPokerPlayerState>();

	if (!MyPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC][PSCHECK] MyPS is NULL"));
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[PC][PSCHECK] MyPS = %s | TestValue=%d"),
		*MyPS->GetName(), MyPS->GetTestValue());

	AGameStateBase* GSBase = GetWorld()->GetGameState();
	if (!GSBase)
		return;

	for (APlayerState* PS : GSBase->PlayerArray)
	{
		AIndianPokerPlayerState* IPS = Cast<AIndianPokerPlayerState>(PS);
		if (!IPS)
			continue;

		if (IPS != MyPS)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[PC][PSCHECK] OtherPS = %s | TestValue=%d"),
				*IPS->GetName(), IPS->GetTestValue());
		}
	}
}

void AIndianPokerPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::F, IE_Pressed, this,
		&AIndianPokerPlayerController::Server_RequestIncrease);
}

// Server_RequestIncrease()는 클라에서 호출
// _Implementation()은 서버에서 실행
void AIndianPokerPlayerController::Server_RequestIncrease_Implementation()
{
	AIndianPokerPlayerState* MyPS = GetPlayerState<AIndianPokerPlayerState>();
	if (!MyPS)
		return;

	int32 NewValue = MyPS->GetTestValue() + 10;
	MyPS->ServerSetTestValue(NewValue);

	UE_LOG(LogTemp, Warning,
		TEXT("[PC][RPC] Server_RequestIncrease executed. NewValue=%d"),
		NewValue);
}