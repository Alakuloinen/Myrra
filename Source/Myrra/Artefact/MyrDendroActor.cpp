// Fill out your copyright notice in the Description page of Project Settings.


#include "Artefact/MyrDendroActor.h"
#include "AssetStructures/MyrDendroInfo.h"

// Sets default values
AMyrDendroActor::AMyrDendroActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BaseForm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseForm"));
	RootComponent = BaseForm;

}

// Called when the game starts or when spawned
void AMyrDendroActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyrDendroActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

