// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

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
	Settings.NumPublicConnections = HostMaxPlayers;
	Settings.bAllowJoinInProgress = true;
	Settings.bShouldAdvertise = true;

	Settings.bUsesPresence = false;
	Settings.bUseLobbiesIfAvailable = false;
	Settings.bAllowJoinViaPresence = false;

	CreateSessionCompleteHandle = SI->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] HostSession -> CreateSession request (LAN=%d Max=%d)"),
		*NetModeToStr(GetWorld()), bIsLAN ? 1 : 0, HostMaxPlayers);

	//const bool bRequestSent = SI->CreateSession(0, IndianPokerSessionName, Settings);
	const bool bRequestSent = SI->CreateSession(0, NAME_GameSession, Settings);
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

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionSub] CreateSession failed. Skip ServerTravel."));
		return;
	}

	/*UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub] GetWorld() is nullptr. Cannot ServerTravel."));
		return;
	}*/

	// Day8. ServerTravel 은 말 그대로 이미 서버인 월드가 다른 맵으로 이동할 때 쓰는 느낌이 강한데,
	// 지금 Host 버튼 누르기 전의 MainMenuMap 월드는 로그상 Standalone 이니까
	// 아직 서버가 아닌 독립 월드이므로, 여기서 '리스닝 서버'로 맵을 열어야 함
	/*const FString TravelURL = TEXT("/Game/Maps/LobbyMap?listen");
	UE_LOG(LogTemp, Warning, TEXT("[SessionSub] ServerTravel -> %s"), *TravelURL);
	World->ServerTravel(TravelURL);*/

	/*UE_LOG(LogTemp, Warning, TEXT("[SessionSub] OpenLevel -> LobbyMap?listen"));
	UGameplayStatics::OpenLevel(World, FName(TEXT("LobbyMap")), true, TEXT("listen"));*/

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub] CreateSession succeeded."));
}

// FindSessions() + Find 콜백
void UIndianPokerSessionSubsystem::FindSessions()
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (!SI.IsValid()) return;

	// Day7 - 새 검색 시작 전에 이전 결과를 비우는 로직(3줄)
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

		bIsFindingSessions = false;
	}
}

// (Find 콜백)
void UIndianPokerSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	// Day6
	/*if (IOnlineSessionPtr SI = GetSessionInterface())
	{
		SI->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}

	const int32 Count = (SessionSearch.IsValid()) ? SessionSearch->SearchResults.Num() : 0;

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] OnFindSessionsComplete: Success=%d Results=%d"),
		*NetModeToStr(GetWorld()), bWasSuccessful ? 1 : 0, Count);*/

	// Day7
	// FindSessions 완료 콜백 구조 변경
	// Find 완료 -> 결과를 CachedSearchResults에 저장 -> FSessionRowData로 변환 -> OnSessionsFound.Broadcast()
	bIsFindingSessions = false;

	if (IOnlineSessionPtr SI = GetSessionInterface())
	{
		SI->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}

	CachedSearchResults.Empty();
	CachedRowData.Empty();

	const int32 Count = (SessionSearch.IsValid()) ? SessionSearch->SearchResults.Num() : 0;

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] OnFindSessionsComplete: Success=%d Results=%d"),
		*NetModeToStr(GetWorld()), bWasSuccessful ? 1 : 0, Count);

	if (bWasSuccessful && SessionSearch.IsValid())
	{
		const TArray<FOnlineSessionSearchResult>& Results = SessionSearch->SearchResults;

		for (int32 i = 0; i < Results.Num(); ++i)
		{
			const FOnlineSessionSearchResult& SearchResult = Results[i];
			CachedSearchResults.Add(SearchResult);

			FSessionRowData RowData;

			// HostName
			FString HostName = SearchResult.Session.OwningUserName;
			if (HostName.IsEmpty())
			{
				HostName = TEXT("UnknownHost");
			}

			// Players
			const int32 MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
			const int32 OpenConnections = SearchResult.Session.NumOpenPublicConnections;
			const int32 CurrentPlayers = FMath::Max(0, MaxPlayers - OpenConnections);

			// RowData 채우기
			RowData.HostName = HostName;
			RowData.CurrentPlayers = CurrentPlayers;
			RowData.MaxPlayers = MaxPlayers;
			RowData.Ping = SearchResult.PingInMs;
			RowData.SearchResultIndex = i;

			CachedRowData.Add(RowData);

			UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] Result[%d] Host=%s Players=%d/%d Ping=%d"),
				*NetModeToStr(GetWorld()),
				i,
				*RowData.HostName,
				RowData.CurrentPlayers,
				RowData.MaxPlayers,
				RowData.Ping);
		}
	}

	OnSessionsFound.Broadcast(bWasSuccessful, CachedRowData);
}

// JoinSession() (Day6는 JoinFirstSession) + Join 콜백
void UIndianPokerSessionSubsystem::JoinFirstSession()
{
	//IOnlineSessionPtr SI = GetSessionInterface();
	//if (!SI.IsValid()) return;

	//if (!SessionSearch.IsValid() || SessionSearch->SearchResults.Num() == 0)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] JoinFirstSession: No SearchResults. Call FindSessions first."), *NetModeToStr(GetWorld()));
	//	return;
	//}

	//JoinSessionCompleteHandle = SI->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	//UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] JoinSession -> request using Result[0]"), *NetModeToStr(GetWorld()));

	////const bool bRequestSent = SI->JoinSession(0, IndianPokerSessionName, SessionSearch->SearchResults[0]);
	//const bool bRequestSent = SI->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[0]);
	//if (!bRequestSent)
	//{
	//	UE_LOG(LogTemp, Error, TEXT("[SessionSub][%s] JoinSession request FAILED to send"), *NetModeToStr(GetWorld()));
	//	SI->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	//}

	JoinSessionByIndex(0);
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

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionSub] JoinSession failed. Skip ClientTravel."));
		return;
	}

	if (!SI.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub] SessionInterface is invalid."));
		return;
	}

	FString ConnectString;
	const bool bOK = SI->GetResolvedConnectString(SessionName, ConnectString);

	if (Result == EOnJoinSessionCompleteResult::Success && SI.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] GetResolvedConnectString: %s %s"),
			*NetModeToStr(GetWorld()),
			bOK ? TEXT("OK") : TEXT("FAIL"),
			bOK ? *ConnectString : TEXT(""));
	}

	if (!bOK)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub] Failed to resolve connect string."));
		return;
	}

	
	// Day8. 계속 포트를 0으로 잡아서... 여기는 잠시 보류하고,
	// TODO: OSS Null + PIE 환경에서 GetResolvedConnectString()가 :0을 반환하는 문제 임시 우회(하단 추가 코드)
	/*UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub] GetWorld() is nullptr."));
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub] FirstPlayerController is nullptr."));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] ClientTravel -> %s"),
		*NetModeToStr(World), *ConnectString);

	PC->ClientTravel(ConnectString, TRAVEL_Absolute);*/

	// Day8. 임시 우회: OSS Null에서 :0 이 반환되면 7777로 보정
	FString TravelAddress = ConnectString;

	if (TravelAddress.EndsWith(TEXT(":0")))
	{
		TravelAddress = TravelAddress.LeftChop(2) + TEXT(":7777");

		UE_LOG(LogTemp, Warning, TEXT("[SessionSub] ConnectString port was 0. Fallback -> %s"),
			*TravelAddress);
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub] GetWorld() is nullptr."));
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub] FirstPlayerController is nullptr."));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] ClientTravel -> %s"),
		*NetModeToStr(World), *TravelAddress);

	PC->ClientTravel(TravelAddress, TRAVEL_Absolute);
}

// DestroySession() + Destroy 콜백
void UIndianPokerSessionSubsystem::DestroySession()
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (!SI.IsValid()) return;

	DestroySessionCompleteHandle = SI->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] DestroySession -> request"), *NetModeToStr(GetWorld()));

	//const bool bRequestSent = SI->DestroySession(IndianPokerSessionName);
	const bool bRequestSent = SI->DestroySession(NAME_GameSession);
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

	// Day18-2
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionSub] DestroySession failed."));
		bPendingJoinAfterDestroy = false;
		PendingJoinIndex = INDEX_NONE;
		bPendingHostAfterDestroy = false;
		return;
	}

	// Join 대기 중이면 Destroy 후 바로 Join 재시도
	if (bPendingJoinAfterDestroy)
	{
		const int32 SavedJoinIndex = PendingJoinIndex;

		bPendingJoinAfterDestroy = false;
		PendingJoinIndex = INDEX_NONE;

		UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] Destroy complete -> retry JoinSessionByIndex(%d)"),
			*NetModeToStr(GetWorld()), SavedJoinIndex);

		JoinSessionByIndex(SavedJoinIndex);
		return;
	}

	// Day8. Destroy 완료 후 로비맵 열기 - Day18-2. 결국 Host 대기중이면 Destroy 후 로비 오픈하게 되는 것
	if (bPendingHostAfterDestroy)
	{
		bPendingHostAfterDestroy = false;

		UWorld* World = GetWorld();
		if (World)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SessionSub] OpenLevel -> LobbyMap?listen"));
			UGameplayStatics::OpenLevel(World, FName(TEXT("LobbyMap")), true, TEXT("listen"));
		}
	}
}

void UIndianPokerSessionSubsystem::JoinSessionByIndex(int32 Index)
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (!SI.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub][%s] JoinSessionByIndex failed - SessionInterface invalid"),
			*NetModeToStr(GetWorld()));
		return;
	}

	if (!CachedSearchResults.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub][%s] JoinSessionByIndex failed - invalid index %d"),
			*NetModeToStr(GetWorld()), Index);
		return;
	}

	// Day18-2. 기존 세션이 있으면 먼저 Destroy 후 Join
	if (SI->GetNamedSession(NAME_GameSession) != nullptr)
	{
		bPendingJoinAfterDestroy = true;
		PendingJoinIndex = Index;

		UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] JoinSessionByIndex -> Existing session found, DestroySession first (Index=%d)"),
			*NetModeToStr(GetWorld()), Index);

		DestroySession();
		return;
	}

	JoinSessionCompleteHandle =
		SI->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] JoinSessionByIndex -> request (Index=%d)"),
		*NetModeToStr(GetWorld()), Index);

	const bool bRequestSent = SI->JoinSession(0, NAME_GameSession, CachedSearchResults[Index]);

	if (!bRequestSent)
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub][%s] JoinSessionByIndex request FAILED to send (Index=%d)"),
			*NetModeToStr(GetWorld()), Index);

		SI->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}
}

void UIndianPokerSessionSubsystem::TryCreateSessionAfterLobbyOpened()
{
	if (UWorld* World = GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionSub] World URL = %s"), *World->URL.ToString());
		UE_LOG(LogTemp, Warning, TEXT("[SessionSub] World URL Port = %d"), World->URL.Port);
	}

	if (!bPendingCreateSessionInLobby)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionSub] TryCreateSessionAfterLobbyOpened skipped - no pending request"));
		return;
	}

	bPendingCreateSessionInLobby = false;

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] TryCreateSessionAfterLobbyOpened -> HostSession"),
		*NetModeToStr(GetWorld()));

	HostSession();
}

void UIndianPokerSessionSubsystem::RequestHostLobby()
{
	/*bPendingHostAfterDestroy = true;
	bPendingCreateSessionInLobby = true;
	PendingMaxPlayers = 2;
	bPendingLAN = true;

	UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] RequestHostLobby -> DestroySession first"),
		*NetModeToStr(GetWorld()));

	DestroySession();*/

	bPendingCreateSessionInLobby = true;
	PendingMaxPlayers = 2;
	bPendingLAN = true;

	UWorld* World = GetWorld();
	IOnlineSessionPtr SI = GetSessionInterface();

	if (!SI.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionSub][%s] RequestHostLobby -> SessionInterface invalid"),
			*NetModeToStr(World));
		return;
	}

	// 기존 세션 존재 여부 확인
	if (SI->GetNamedSession(NAME_GameSession) != nullptr)
	{
		bPendingHostAfterDestroy = true;

		UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] RequestHostLobby -> Existing session found, DestroySession first"),
			*NetModeToStr(World));

		DestroySession();
	}
	else
	{
		bPendingHostAfterDestroy = false;

		UE_LOG(LogTemp, Warning, TEXT("[SessionSub][%s] RequestHostLobby -> No existing session, OpenLevel directly"),
			*NetModeToStr(World));

		if (World)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SessionSub] OpenLevel -> LobbyMap?listen"));
			UGameplayStatics::OpenLevel(World, FName(TEXT("LobbyMap")), true, TEXT("listen"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[SessionSub] RequestHostLobby -> GetWorld() is nullptr"));
		}
	}
}

void UIndianPokerSessionSubsystem::SetSelectedMatchMode(EIndianPokerMatchMode NewMode)
{
	SelectedMatchMode = NewMode;
	UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] SelectedMatchMode set to %s"),
		NewMode == EIndianPokerMatchMode::PvE ? TEXT("PvE") : TEXT("PvP"));
}

EIndianPokerMatchMode UIndianPokerSessionSubsystem::GetSelectedMatchMode() const
{
	return SelectedMatchMode;
}