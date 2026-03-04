// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Engine/Engine.h"

const FName UIndianPokerSessionSubsystem::IndianPokerSessionName(TEXT("IndianPokerSession"));

// NetMode 확인용 유틸
static FString NetModeToStr(const UWorld* World)
{
	if (!World) return TEXT("NoWorld");
	switch (World->GetNetMode())
	{
	case NM_Standalone: return TEXT("Standalone");
	case NM_ListenServer: return TEXT("ListenServer");
	case NM_DedicatedServer: return TEXT("DedicatedServer");
	case NM_Client: return TEXT("Client");
	default: return TEXT("Unknown");
	}
}

// Initialize에서 Delegate 객체만 만들어두고, 
// 실제 바인딩(Add)은 Host/Find/Join/Destroy “요청 직전”에 한다.
void UIndianPokerSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	/*Super::Initialize(Collection);

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub] OSS is null"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[SessionSub] OnlineSubsystem Name: %s"), *OSS->GetSubsystemName().ToString());*/

	Super::Initialize(Collection);

	// Delegate 객체 생성 (바인딩 Add는 호출 시점에 함)
	CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete);
	FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete);
	JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete);
	DestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete);

	// 서브시스템/세션인터페이스 인식 로그
	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		UE_LOG(LogTemp, Log, TEXT("[SessionSub] OSS=%s"), *OSS->GetSubsystemName().ToString());
		UE_LOG(LogTemp, Log, TEXT("[SessionSub] SessionInterfaceValid=%s"),
			OSS->GetSessionInterface().IsValid() ? TEXT("YES") : TEXT("NO"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub] OSS is NULL"));
	}
}

// 콜백이 오면 Clear로 해제 → 중복 호출/중복 로그/중복 콜백 방지.
void UIndianPokerSessionSubsystem::Deinitialize()
{
	/*UE_LOG(LogTemp, Log, TEXT("[SessionSub] Deinitialize"));
	Super::Deinitialize();*/

	// 안전하게 핸들 해제 (혹시 남아있을 수 있어서)
	if (IOnlineSessionPtr SI = GetSessionInterface())
	{
		if (CreateSessionCompleteHandle.IsValid())
			SI->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);

		if (FindSessionsCompleteHandle.IsValid())
			SI->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);

		if (JoinSessionCompleteHandle.IsValid())
			SI->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);

		if (DestroySessionCompleteHandle.IsValid())
			SI->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}

	Super::Deinitialize();
}

// 세션 인터페이스 접근 함수
IOnlineSessionPtr UIndianPokerSessionSubsystem::GetSessionInterface() const
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub] GetSessionInterface: OSS NULL"));
		return nullptr;
	}

	IOnlineSessionPtr SI = OSS->GetSessionInterface();
	if (!SI.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub] GetSessionInterface: SessionInterface invalid (OSS=%s)"),
			*OSS->GetSubsystemName().ToString());
		return nullptr;
	}

	return SI;
}

// HostSession() + Create 콜백
// CreateSession()은 “요청을 보냈는지”만 bool로 리턴.
// 실제 성공 / 실패는 OnCreateSessionComplete에서 확정.
void UIndianPokerSessionSubsystem::HostSession()
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (!SI.IsValid()) return;

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = bIsLAN;
	Settings.NumPublicConnections = MaxPlayers;
	Settings.bAllowJoinInProgress = true;
	Settings.bShouldAdvertise = true;
	Settings.bUsesPresence = false;
	Settings.bUseLobbiesIfAvailable = false;

	CreateSessionCompleteHandle = SI->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] HostSession -> CreateSession request (LAN=%d Max=%d)"),
		*NetModeToStr(GetWorld()), bIsLAN ? 1 : 0, MaxPlayers);

	const bool bRequestSent = SI->CreateSession(0, IndianPokerSessionName, Settings);
	if (!bRequestSent)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub][%s] CreateSession request FAILED to send"), *NetModeToStr(GetWorld()));
		SI->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	}
}

// (Create 콜백)
void UIndianPokerSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr SI = GetSessionInterface())
	{
		SI->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	}

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] OnCreateSessionComplete: %s Success=%d"),
		*NetModeToStr(GetWorld()), *SessionName.ToString(), bWasSuccessful ? 1 : 0);
}

// FindSessions() + Find 콜백
void UIndianPokerSessionSubsystem::FindSessions()
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (!SI.IsValid()) return;

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = bIsLAN;
	SessionSearch->MaxSearchResults = 20;

	FindSessionsCompleteHandle = SI->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] FindSessions -> request (LAN=%d)"), 
		*NetModeToStr(GetWorld()), bIsLAN ? 1 : 0);

	const bool bRequestSent = SI->FindSessions(0, SessionSearch.ToSharedRef());
	if (!bRequestSent)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub][%s] FindSessions request FAILED to send"), *NetModeToStr(GetWorld()));
		SI->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}
}

// (Find 콜백)
void UIndianPokerSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (IOnlineSessionPtr SI = GetSessionInterface())
	{
		SI->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}

	const int32 Count = (SessionSearch.IsValid()) ? SessionSearch->SearchResults.Num() : 0;

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] OnFindSessionsComplete: Success=%d Results=%d"),
		*NetModeToStr(GetWorld()), bWasSuccessful ? 1 : 0, Count);
}

// JoinSession() (Day6는 JoinFirstSession) + Join 콜백
void UIndianPokerSessionSubsystem::JoinFirstSession()
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (!SI.IsValid()) return;

	if (!SessionSearch.IsValid() || SessionSearch->SearchResults.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] JoinFirstSession: No SearchResults. Call FindSessions first."), *NetModeToStr(GetWorld()));
		return;
	}

	JoinSessionCompleteHandle = SI->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] JoinSession -> request using Result[0]"), *NetModeToStr(GetWorld()));

	const bool bRequestSent = SI->JoinSession(0, IndianPokerSessionName, SessionSearch->SearchResults[0]);
	if (!bRequestSent)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub][%s] JoinSession request FAILED to send"), *NetModeToStr(GetWorld()));
		SI->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}
}

// (Join 콜백) + ConnectString 로그
// Join은“성공했으면 주소를 얻을 수 있냐가 핵심 체크포인트.
// Day6에서는 Travel은 안 붙여도 됨.ConnectString 로그만 찍히면 성공.
static const TCHAR* JoinResultToStr(EOnJoinSessionCompleteResult::Type R)
{
	switch (R)
	{
	case EOnJoinSessionCompleteResult::Success: return TEXT("Success");
	case EOnJoinSessionCompleteResult::SessionIsFull: return TEXT("SessionIsFull");
	case EOnJoinSessionCompleteResult::SessionDoesNotExist: return TEXT("SessionDoesNotExist");
	case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress: return TEXT("CouldNotRetrieveAddress");
	case EOnJoinSessionCompleteResult::AlreadyInSession: return TEXT("AlreadyInSession");
	default: return TEXT("UnknownError");
	}
}

void UIndianPokerSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (SI.IsValid())
	{
		SI->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] OnJoinSessionComplete: %s Result=%s"),
		*NetModeToStr(GetWorld()), *SessionName.ToString(), JoinResultToStr(Result));

	if (Result == EOnJoinSessionCompleteResult::Success && SI.IsValid())
	{
		FString ConnectString;
		const bool bOK = SI->GetResolvedConnectString(SessionName, ConnectString);

		UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] GetResolvedConnectString: %s %s"),
			*NetModeToStr(GetWorld()),
			bOK ? TEXT("OK") : TEXT("FAIL"),
			bOK ? *ConnectString : TEXT(""));
	}
}

// DestroySession() + Destroy 콜백
void UIndianPokerSessionSubsystem::DestroySession()
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (!SI.IsValid()) return;

	DestroySessionCompleteHandle = SI->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] DestroySession -> request"), *NetModeToStr(GetWorld()));

	const bool bRequestSent = SI->DestroySession(IndianPokerSessionName);
	if (!bRequestSent)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub][%s] DestroySession request FAILED to send"), *NetModeToStr(GetWorld()));
		SI->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}
}

// (Destroy 콜백)
void UIndianPokerSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr SI = GetSessionInterface())
	{
		SI->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] OnDestroySessionComplete: %s Success=%d"),
		*NetModeToStr(GetWorld()), *SessionName.ToString(), bWasSuccessful ? 1 : 0);
}

