#pragma once

#include "CoreMinimal.h"
#include "BettingTypes.generated.h"

UENUM(BlueprintType)
enum class EBettingActionType : uint8
{
	Check		UMETA(DisplayName = "Check"),
	CheckCall	UMETA(DisplayName = "CheckCall"),
	Fold		UMETA(DisplayName = "Fold")
};