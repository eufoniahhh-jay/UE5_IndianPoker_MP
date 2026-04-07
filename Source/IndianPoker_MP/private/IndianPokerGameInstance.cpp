#include "IndianPokerGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Kismet/GameplayStatics.h"

void UIndianPokerGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UIndianPokerGameInstance::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &UIndianPokerGameInstance::HandleTravelFailure);

		UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Network/Travel failure handlers bound"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Init failed - GEngine is null"));
	}
}

void UIndianPokerGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	Super::Shutdown();
}

void UIndianPokerGameInstance::ClearPendingDisconnectMessage()
{
	PendingDisconnectMessage.Empty();
}

void UIndianPokerGameInstance::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType,
	const FString& ErrorString
)
{
	const FString WorldName = World ? World->GetMapName() : TEXT("NoWorld");

	UE_LOG(LogTemp, Warning,
		TEXT("[GameInstance][NetworkFailure] World=%s FailureType=%d Error=%s"),
		*WorldName,
		static_cast<int32>(FailureType),
		*ErrorString);

	FString DisconnectMessage = TEXT("Host disconnected");

	switch (FailureType)
	{
	case ENetworkFailure::ConnectionLost:
		DisconnectMessage = TEXT("Host disconnected (ConnectionLost)");
		break;

	case ENetworkFailure::ConnectionTimeout:
		DisconnectMessage = TEXT("Host disconnected (ConnectionTimeout)");
		break;

	case ENetworkFailure::FailureReceived:
		DisconnectMessage = TEXT("Host disconnected (FailureReceived)");
		break;

	/*case ENetworkFailure::ConnectionFailed:
		DisconnectMessage = TEXT("Connection failed");
		break;*/

	default:
		DisconnectMessage = TEXT("Network connection ended");
		break;
	}

	ReturnToMainMenuAfterDisconnect(DisconnectMessage);
}

void UIndianPokerGameInstance::HandleTravelFailure(
	UWorld* World,
	ETravelFailure::Type FailureType,
	const FString& ErrorString
)
{
	const FString WorldName = World ? World->GetMapName() : TEXT("NoWorld");

	UE_LOG(LogTemp, Warning,
		TEXT("[GameInstance][TravelFailure] World=%s FailureType=%d Error=%s"),
		*WorldName,
		static_cast<int32>(FailureType),
		*ErrorString);

	ReturnToMainMenuAfterDisconnect(TEXT("Travel failed - returning to main menu"));
}

void UIndianPokerGameInstance::ReturnToMainMenuAfterDisconnect(const FString& InMessage)
{
	PendingDisconnectMessage = InMessage;

	UE_LOG(LogTemp, Warning,
		TEXT("[GameInstance] ReturnToMainMenuAfterDisconnect -> %s"),
		*PendingDisconnectMessage);

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameInstance] ReturnToMainMenuAfterDisconnect failed - World is null"));
		return;
	}

	const FString CurrentMapName = World->GetMapName();

	// 이미 MainMenu면 중복 OpenLevel 방지
	if (CurrentMapName.Contains(TEXT("MainMenuMap")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Already in MainMenuMap - skip OpenLevel"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] CurrentMap before return = %s"), *CurrentMapName);
	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] OpenLevel -> MainMenuMap"));
	UGameplayStatics::OpenLevel(World, FName(TEXT("MainMenuMap")));
}