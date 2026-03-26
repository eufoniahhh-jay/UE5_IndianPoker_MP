// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "CardActor.generated.h"

UCLASS()
class INDIANPOKER_MP_API ACardActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACardActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<class UStaticMeshComponent> CardMesh;

public:
	// Day16.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Card")
	int32 CardSlotId = -1;

	// BP에서 만든 SetCardFrontOnly 호출 
	UFUNCTION(BlueprintImplementableEvent, Category = "Card")
	void BP_SetCardFrontOnly(UTexture2D* InFrontTexture);

public:
	// Day16. 각자 로컬에서 머티리얼을 갱신하는 구조로 변경 위함
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetCardValueServer(int32 InCardValue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Card")
	void BP_OnCardValueChanged(int32 InCardValue);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentCardValue, BlueprintReadOnly, Category = "Card")
	int32 CurrentCardValue = 0;

	UFUNCTION()
	void OnRep_CurrentCardValue();
};
