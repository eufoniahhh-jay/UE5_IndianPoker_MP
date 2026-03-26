// Fill out your copyright notice in the Description page of Project Settings.


#include "CardActor.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ACardActor::ACardActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardMesh"));
	SetRootComponent(CardMesh);

	bReplicates = true;
	SetReplicateMovement(true);

	// 충돌은 카드에선 보통 필요 없으니 일단 꺼두는 편이 편함(선택)
	CardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned
void ACardActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACardActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACardActor, CurrentCardValue);
}

void ACardActor::SetCardValueServer(int32 InCardValue)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentCardValue = InCardValue;

	UE_LOG(LogTemp, Warning, TEXT("[CardActor] SetCardValueServer | %s | Value=%d"),
		*GetName(), CurrentCardValue);

	// 서버도 자기 화면에서 즉시 갱신해야 하므로 직접 호출
	BP_OnCardValueChanged(CurrentCardValue);
}

void ACardActor::OnRep_CurrentCardValue()
{
	UE_LOG(LogTemp, Warning, TEXT("[CardActor] OnRep_CurrentCardValue | %s | Value=%d"),
		*GetName(), CurrentCardValue);

	BP_OnCardValueChanged(CurrentCardValue);
}

