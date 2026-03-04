// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "IndianPokerSessionSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class INDIANPOKER_MP_API UIndianPokerSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Day6 핵심 API 4개
	void HostSession();
	void FindSessions();
	void JoinFirstSession();     // Day6는 Result[0]만 조인하는 간단 버전
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
	int32 MaxPlayers = 2;
	bool bIsLAN = true;
};
