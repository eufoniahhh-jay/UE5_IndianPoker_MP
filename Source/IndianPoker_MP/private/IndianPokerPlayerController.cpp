// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "IndianPokerGameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "IndianPokerSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void AIndianPokerPlayerController::BeginPlay()
{
	Super::BeginPlay();

	//UE_LOG(LogTemp, Warning, TEXT("[PC] Class=%s"), *GetClass()->GetPathName());

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

	UE_LOG(LogTemp, Warning, TEXT("[PC][%s] Class=%s"), NetModeStr, *GetClass()->GetPathName());

	// 로컬 플레이어(각 창)에서만 매핑 추가
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys =
			LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (IMC_Player)
			{
				Subsys->AddMappingContext(IMC_Player, 0);

				// 새 매핑 반영 강제
				FModifyContextOptions Options;
				Options.bForceImmediately = true;          // 즉시 반영(있으면 켜기)
				Options.bIgnoreAllPressedKeysUntilRelease = false;

				Subsys->RequestRebuildControlMappings(Options);

				UE_LOG(LogTemp, Warning, TEXT("[PC] IMC added + Rebuild mappings"));
			}
		}
	}

	// 아직 UI 없고 테스트 단계면 일단 GameOnly로 잡아도 됨
	// (UI 버튼까지 들어가면 GameAndUI로 바꿀 예정)
	//SetShowMouseCursor(true);

	UE_LOG(LogTemp, Warning, TEXT("[PC] IMC=%s Look=%s Aim=%s Test=%s"),
		*GetNameSafe(IMC_Player), *GetNameSafe(IA_Look), *GetNameSafe(IA_AimLook), *GetNameSafe(IA_Test));

	// 세션테스트 맵에서 입력이 무조건 되게 (세션 테스트용)
	const FString CleanMapName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefix*/ true);

	if (CleanMapName == TEXT("SessionTestMap"))
	{
		FInputModeGameOnly Mode;
		SetInputMode(Mode);
		bShowMouseCursor = false;

		// 멀티 프로세스/포커스 꼬임 대비
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);

		UE_LOG(LogTemp, Warning, TEXT("[PC] SessionTestMap detected -> Force GameOnly input"));
	}

	// Day8. LobbyMap 진입 후 Host 후속 처리
	if (bLocal && CleanMapName == TEXT("LobbyMap"))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] LobbyMap detected"));

		// 클라이언트는 여기서 세션 생성 시도를 하지 않음
		if (NetMode != NM_Client)
		{
			// 다음 틱으로 넘기기
			/*FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimerForNextTick(this, &AIndianPokerPlayerController::HandleLobbyHostSetup);*/

			FTimerHandle LobbyHostSetupTimerHandle;
			GetWorldTimerManager().SetTimer(
				LobbyHostSetupTimerHandle,
				this,
				&AIndianPokerPlayerController::HandleLobbyHostSetup,
				1.5f,   // 일단 0.5초 딜레이
				false
			);

			// HandleLobbyHostSetup 쪽으로 이동
			/*if (UGameInstance* GI = GetGameInstance())
			{
				if (UIndianPokerSessionSubsystem* SessionSub = GI->GetSubsystem<UIndianPokerSessionSubsystem>())
				{
					UE_LOG(LogTemp, Warning, TEXT("[PC] Calling TryCreateSessionAfterLobbyOpened()"));
					SessionSub->TryCreateSessionAfterLobbyOpened();
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[PC] SessionSubsystem is nullptr"));
				}
			}*/
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[PC] LobbyMap on Client -> Skip TryCreateSessionAfterLobbyOpened"));
		}
	}
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

	// Day6 세션 테스트용 바인딩
	/*InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ThisClass::TestHost);
	InputComponent->BindKey(EKeys::W, IE_Pressed, this, &ThisClass::TestFind);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ThisClass::TestJoin);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ThisClass::TestDestroy);*/
	if (IA_SessionHost)
		EIC->BindAction(IA_SessionHost, ETriggerEvent::Started, this, &AIndianPokerPlayerController::TestHost);

	if (IA_SessionFind)
		EIC->BindAction(IA_SessionFind, ETriggerEvent::Started, this, &AIndianPokerPlayerController::TestFind);

	if (IA_SessionJoin)
		EIC->BindAction(IA_SessionJoin, ETriggerEvent::Started, this, &AIndianPokerPlayerController::TestJoin);

	if (IA_SessionDestroy)
		EIC->BindAction(IA_SessionDestroy, ETriggerEvent::Started, this, &AIndianPokerPlayerController::TestDestroy);
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

// Subsystem 가져오는 helper 함수 (Day6 테스트용)
static UIndianPokerSessionSubsystem* GetSessionSubsystem(UObject* WorldContext)
{
	if (!WorldContext) return nullptr;

	if (UWorld* World = WorldContext->GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<UIndianPokerSessionSubsystem>();
		}
	}

	return nullptr;
}

void AIndianPokerPlayerController::TestHost()
{
	if (auto* SessionSub = GetSessionSubsystem(this))
	{
		SessionSub->HostSession();
	}
}

void AIndianPokerPlayerController::TestFind()
{
	UE_LOG(LogTemp, Warning, TEXT("[PC][Client?=%d] W Pressed"), IsLocalController() ? 1 : 0);

	if (auto* SessionSub = GetSessionSubsystem(this))
	{
		SessionSub->FindSessions();
	}
}

void AIndianPokerPlayerController::TestJoin()
{
	if (auto* SessionSub = GetSessionSubsystem(this))
	{
		SessionSub->JoinFirstSession();
		//SessionSub->JoinSessionByIndex(0);
	}
}

void AIndianPokerPlayerController::TestDestroy()
{
	if (auto* SessionSub = GetSessionSubsystem(this))
	{
		SessionSub->DestroySession();
	}
}

void AIndianPokerPlayerController::HandleLobbyHostSetup()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UIndianPokerSessionSubsystem* SessionSub = GI->GetSubsystem<UIndianPokerSessionSubsystem>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[PC] Delayed call -> TryCreateSessionAfterLobbyOpened()"));
			SessionSub->TryCreateSessionAfterLobbyOpened();
		}
	}
}