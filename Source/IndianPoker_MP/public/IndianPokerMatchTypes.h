#pragma once

#include "CoreMinimal.h"
#include "IndianPokerMatchTypes.generated.h"

UENUM(BlueprintType)
enum class EIndianPokerMatchMode : uint8
{
	PvP UMETA(DisplayName = "PvP"),
	PvE UMETA(DisplayName = "PvE")
};