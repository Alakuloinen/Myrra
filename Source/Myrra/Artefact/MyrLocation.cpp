// Fill out your copyright notice in the Description page of Project Settings.


#include "Artefact/MyrLocation.h"
#include "Artefact/MyrTriggerComponent.h"
#include "Artefact/MyrArtefact.h"
#include "Creature/MyrPhyCreature.h"

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
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 10.0f;


	//источник эффекта частиц запаха
	RootMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RootMesh"));
	RootMesh->SetMobility(EComponentMobility::Static);
	RootComponent = RootMesh;

	PrimaryLocationVolume = CreateDefaultSubobject<UMyrTriggerComponent>(TEXT("PrimaryLocationVolume"));
	PrimaryLocationVolume->SetupAttachment(RootComponent);
	PrimaryLocationVolume->SetRelativeLocation(FVector(0, 0, 0));
	PrimaryLocationVolume->SetUsingAbsoluteScale(true);
	PrimaryLocationVolume->SetMobility(EComponentMobility::Static);

	//главный маркер объёма локации нуждается в специальной реакции
	FTriggerReason Main;
	Main.Why = EWhyTrigger::EnterLocation;
	PrimaryLocationVolume->Reactions.Add(Main);

}

//==============================================================================================================
// Called when the game starts or when spawned
//==============================================================================================================
void AMyrLocation::BeginPlay()
{
	Super::BeginPlay();
	
}

//==============================================================================================================
// в локации тик пока нужен только чтобы время от времени эмоционально воздействовать на обитателей
//==============================================================================================================
void AMyrLocation::Tick(float DeltaTime)
{
	//каждому существу внушить эмоцию с внешними данными из этой локации
	for(auto Cre : Inhabitants)
		Cre->CatchMyrLogicEvent(EMyrLogicEvent::ObjLocationAffect, 1.0f, PrimaryLocationVolume, &EmotionalInducing);
	Super::Tick(DeltaTime);
}

void AMyrLocation::AddCreature(AMyrPhyCreature* New)
{
	//посигналить существу, что оно вошло
	New->CatchMyrLogicEvent(EMyrLogicEvent::ObjEnterLocation, 1.0f, PrimaryLocationVolume);

	//зарегистрировать
	Inhabitants.Add(New);

	//коль скоро хотя бы одно есть, для него запустить тик эмоционального влияния
	PrimaryActorTick.SetTickFunctionEnable(true);

}
void AMyrLocation::RemoveCreature(AMyrPhyCreature* New)
{
	New->CatchMyrLogicEvent(EMyrLogicEvent::ObjExitLocation, 1.0f, PrimaryLocationVolume);
	Inhabitants.Remove(New);

	//если локация опустела, остановить нагнетание эмоций
	if(Inhabitants.Num() == 0)
		PrimaryActorTick.SetTickFunctionEnable(true);

}
void AMyrLocation::AddArtefact(AMyrArtefact* New)
{
	Content.Add(New);
}
void AMyrLocation::RemoveArtefact(AMyrArtefact* New)
{
	Content.Remove(New);
}
