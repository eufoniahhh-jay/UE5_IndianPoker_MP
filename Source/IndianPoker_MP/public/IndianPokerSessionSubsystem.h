// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "IndianPokerMatchTypes.h"
#include "IndianPokerSessionSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSessionRowData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString HostName = TEXT("Unknown");

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Ping = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 SearchResultIndex = INDEX_NONE;
};

// UI로 결과를 알려줄 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSessionsFound,
	bool, bSuccess,
	const TArray<FSessionRowData>&, Results
);

UCLASS(BlueprintType)
class INDIANPOKER_MP_API UIndianPokerSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Day6 핵심 API 4개
	UFUNCTION(BlueprintCallable)
	void HostSession();
	UFUNCTION(BlueprintCallable)
	void FindSessions();
	void JoinFirstSession();     // Day6는 Result[0]만 조인하는 간단 버전 
	UFUNCTION(BlueprintCallable)
	void DestroySession();

private:
	// 1) 세션 인터페이스 접근
	IOnlineSessionPtr GetSessionInterface() const;

	// 2) Delegate 콜백
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

private:
	// 세션 이름(상수화)
	// 세션을 식별하는 이름. Host/Join/Destroy가 모두 이 이름을 쓴다.
	static const FName IndianPokerSessionName;

	// Find 결과 저장. Join에서 SearchResults[0]로 조인할 거라 필수.
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	// Delegate + Handle
	// Delegate는 “콜백 함수 연결 정보”
	// Handle은 “바인딩 해제할 때 필요한 키”
	// -> 요청 전 Add / 콜백에서 Clear 패턴이 안정적.
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteHandle;

	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteHandle;

	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteHandle;

	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteHandle;

	// 옵션(오늘은 하드코딩 최소)
	int32 HostMaxPlayers = 2;
	bool bIsLAN = true;

private:
	// Day7. Find 결과를 저장할 공간
	// UI는 FOnlineSessionSearchResult를 직접 다루지 않게 할 거니까, Subsystem 안에
	// 원본 검색 결과 캐시 / UI용 변환 결과 캐시 2개를 둔다.
	//TSharedPtr<FOnlineSessionSearch> LastSessionSearch;		// 그냥 기존의 SessionSearch 사용으로 대체
	TArray<FOnlineSessionSearchResult> CachedSearchResults;		// 실제 join할 때 필요

	UPROPERTY()
	TArray<FSessionRowData> CachedRowData;						// UI 표시용(블루프린트에서 보기 쉬운 데이터)

	bool bIsFindingSessions = false;

	// Day18-2. Join용 pending 상태값을 추가. (Host와 똑같이 Destroy -> Join 재시도 흐름 위함)
	bool bPendingJoinAfterDestroy = false;
	int32 PendingJoinIndex = INDEX_NONE;

public:
	UPROPERTY(BlueprintAssignable)
	FOnSessionsFound OnSessionsFound;

	// n번째 검색 결과로 Join하는 함수
	UFUNCTION(BlueprintCallable)
	void JoinSessionByIndex(int32 Index);

	UFUNCTION(BlueprintPure)
	TArray<FSessionRowData> GetLastSessionRows() const { return CachedRowData; }

	UFUNCTION(BlueprintPure)
	bool IsFindingSessions() const { return bIsFindingSessions; }

// Day8. wbp에서 host 버튼을 누르면,
// RequestHostLobby(wbp에서 호출) -> destroy -> openlevel -> host 순으로 가게끔
private:
	bool bPendingHostAfterDestroy = false;
	bool bPendingCreateSessionInLobby = false;
	int32 PendingMaxPlayers = 2;
	bool bPendingLAN = true;

public:
	void TryCreateSessionAfterLobbyOpened();
	// Day8 핵심 함수. WBP_MainMenuAdvanced에서 host 버튼을 누르면 이거랑 연결할거임
	UFUNCTION(BlueprintCallable)
	void RequestHostLobby();

private:
	// Day19. 매치모드(PVP or PVE)
	UPROPERTY()
	EIndianPokerMatchMode SelectedMatchMode = EIndianPokerMatchMode::PvP;

public:
	UFUNCTION(BlueprintCallable, Category = "Session")
	void SetSelectedMatchMode(EIndianPokerMatchMode NewMode);

	UFUNCTION(BlueprintPure, Category = "Session")
	EIndianPokerMatchMode GetSelectedMatchMode() const;
};
