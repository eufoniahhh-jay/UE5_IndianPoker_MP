// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "IndianPokerGameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputTriggers.h"

void AIndianPokerPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	const ENetMode NetMode = World ? World->GetNetMode() : NM_Standalone;
	FString MapName = World ? World->GetMapName() : TEXT("NoWorld");

	const bool bAuth = HasAuthority();
	const bool bLocal = IsLocalController();

	// PIE 인스턴스 ID (멀티 PIE에서 서버=0, 클라=1... 식으로 구분됨)
	// const int32 PieId = GPlayInEditorID;

	const TCHAR* NetModeStr =
		(NetMode == NM_Standalone) ? TEXT("Standalone") :
		(NetMode == NM_ListenServer) ? TEXT("ListenServer") :
		(NetMode == NM_DedicatedServer) ? TEXT("DedicatedServer") :
		(NetMode == NM_Client) ? TEXT("Client") : TEXT("Unknown");

	UE_LOG(LogTemp, Warning, TEXT("[PC BeginPlay] Map=%s | NetMode=%s | HasAuthority=%d | IsLocal=%d | Name=%s"),
		*MapName, NetModeStr, bAuth ? 1 : 0, bLocal ? 1 : 0, *GetName());

	// 로컬 플레이어(각 창)에서만 매핑 추가
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys =
			LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (IMC_Player)
			{
				Subsys->AddMappingContext(IMC_Player, 0);
			}
		}
	}

	// 아직 UI 없고 테스트 단계면 일단 GameOnly로 잡아도 됨
	// (UI 버튼까지 들어가면 GameAndUI로 바꿀 예정)
	//SetShowMouseCursor(true);

	UE_LOG(LogTemp, Warning, TEXT("[PC] IMC=%s Look=%s Aim=%s Test=%s"),
		*GetNameSafe(IMC_Player), *GetNameSafe(IA_Look), *GetNameSafe(IA_AimLook), *GetNameSafe(IA_Test));
}

//void AIndianPokerPlayerController::SetupInputComponent()
//{
//	Super::SetupInputComponent();
//
//	InputComponent->BindKey(EKeys::F, IE_Pressed, this,
//		&AIndianPokerPlayerController::Input_AdvancePhase);
//}

void AIndianPokerPlayerController::Input_AdvancePhase()
{
	// 성공 기준: Phase는 서버(Host)에서만 변경되어야 함
	// 즉, 클라이언트에서 키 눌러도 실제 Phase 변경은 일어나면 안 됨
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] AdvancePhase blocked (CLIENT cannot change phase)."));
		return;
	}

	AIndianPokerGameModeBase* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AIndianPokerGameModeBase>() : nullptr;
	if (!GM)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] AdvancePhase failed: GameMode cast failed."));
		return;
	}

	GM->AdvancePhaseServer();
}

// IA 바인딩
void AIndianPokerPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] EnhancedInputComponent not found"));
		return;
	}

	if (IA_AimLook)
	{
		EIC->BindAction(IA_AimLook, ETriggerEvent::Started, this, &AIndianPokerPlayerController::OnAimLookStarted);
		EIC->BindAction(IA_AimLook, ETriggerEvent::Completed, this, &AIndianPokerPlayerController::OnAimLookCompleted);
	}

	if (IA_Look)
	{
		// 마우스는 계속 들어오므로 Triggered에서 처리
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AIndianPokerPlayerController::OnLookTriggered);
	}

	if (IA_Test)
	{
		EIC->BindAction(IA_Test, ETriggerEvent::Started, this, &AIndianPokerPlayerController::OnTestPressed);
	}
}

// RMB 누를 때만 Look 구현. 
// RMB 누르면 마우스 커서를 숨기고, 떼면 커서를 다시 보여주면서 UI와 충돌 방지
void AIndianPokerPlayerController::OnAimLookStarted()
{
	bAimLookHeld = true;

	// RMB 누르는 동안은 카메라 회전용으로 마우스 캡처
	SetShowMouseCursor(false);

	FInputModeGameOnly Mode;
	SetInputMode(Mode);
}

void AIndianPokerPlayerController::OnAimLookCompleted()
{
	bAimLookHeld = false;

	// RMB 떼면 다시 UI 클릭 가능 상태로
	SetShowMouseCursor(true);

	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
}

// Look 처리
void AIndianPokerPlayerController::OnLookTriggered(const FInputActionValue& Value)
{
	if (!bAimLookHeld)
		return;

	const FVector2D Look = Value.Get<FVector2D>();

	AddYawInput(Look.X);
	AddPitchInput(-Look.Y); // 보통 마우스 Y는 반전이 자연스러워서 - 붙임(취향)

	//UE_LOG(LogTemp, Warning, TEXT("[PC] Look: %s"), *Value.ToString());
}

// Day5 F키로 nextPhase 기능 테스트
void AIndianPokerPlayerController::OnTestPressed()
{
	UE_LOG(LogTemp, Warning, TEXT("[PC] IA_Test pressed"));

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] AdvancePhase blocked (CLIENT cannot change phase)."));
		return;
	}

	// Day5 Phase 테스트를 여기에 연결하면 됨 (서버만)
	if (HasAuthority())
	{
		// GameMode->AdvancePhaseServer() 호출 등
		
		AIndianPokerGameModeBase* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AIndianPokerGameModeBase>() : nullptr;
		if (!GM)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PC] AdvancePhase failed: GameMode cast failed."));
			return;
		}
		GM->AdvancePhaseServer();

		UE_LOG(LogTemp, Warning, TEXT("[PC] (SERVER) would advance phase here"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] (CLIENT) blocked"));
	}
}