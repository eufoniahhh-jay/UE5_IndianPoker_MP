#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "IndianPokerGameInstance.generated.h"

UCLASS()
class INDIANPOKER_MP_API UIndianPokerGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintPure, Category = "IndianPoker|Network")
	FString GetPendingDisconnectMessage() const { return PendingDisconnectMessage; }

	UFUNCTION(BlueprintCallable, Category = "IndianPoker|Network")
	void ClearPendingDisconnectMessage();

protected:
	void HandleNetworkFailure(
		UWorld* World,
		UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType,
		const FString& ErrorString
	);

	void HandleTravelFailure(
		UWorld* World,
		ETravelFailure::Type FailureType,
		const FString& ErrorString
	);

	void ReturnToMainMenuAfterDisconnect(const FString& InMessage);

protected:
	UPROPERTY()
	FString PendingDisconnectMessage = TEXT("");
};