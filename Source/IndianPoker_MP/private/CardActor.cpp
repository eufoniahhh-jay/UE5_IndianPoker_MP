// Fill out your copyright notice in the Description page of Project Settings.


#include "CardActor.h"

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


