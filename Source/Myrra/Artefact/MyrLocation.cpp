// Fill out your copyright notice in the Description page of Project Settings.


#include "Artefact/MyrLocation.h"
#include "Artefact/MyrTriggerComponent.h"

//==============================================================================================================
//охватить это строение при загрузке и сохранении игры
//==============================================================================================================
void AMyrLocation::Save(FLocationSaveData& Dst)
{
}

//==============================================================================================================
//охватить это строение при загрузке и сохранении игры
//==============================================================================================================
void AMyrLocation::Load(const FLocationSaveData& Src)
{
}

//==============================================================================================================
// Sets default values
//==============================================================================================================
AMyrLocation::AMyrLocation()
{
	PrimaryActorTick.bCanEverTick = false;


	//источник эффекта частиц запаха
	RootMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RootMesh"));
	RootMesh->SetMobility(EComponentMobility::Static);
	RootComponent = RootMesh;

	PrimaryLocationVolume = CreateDefaultSubobject<UMyrTriggerComponent>(TEXT("PrimaryLocationVolume"));
	PrimaryLocationVolume->SetupAttachment(RootComponent);
	PrimaryLocationVolume->SetRelativeLocation(FVector(0, 0, 0));
	PrimaryLocationVolume->SetUsingAbsoluteScale(true);
	PrimaryLocationVolume->SetMobility(EComponentMobility::Static);

}

//==============================================================================================================
// Called when the game starts or when spawned
//==============================================================================================================
void AMyrLocation::BeginPlay()
{
	Super::BeginPlay();
	
}

//==============================================================================================================
// Called every frame
//==============================================================================================================
void AMyrLocation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

