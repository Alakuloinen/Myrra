// Fill out your copyright notice in the Description page of Project Settings.

#include "MyrArtefact.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"	// привязь на физдвижке

//это нужно, чтобы непосредственно влиять на ИИ существ
#include "AIModule/Classes/Perception/AISenseConfig_Sight.h"
#include "AIModule/Classes/Perception/AISenseConfig_Hearing.h"
#include "AIModule/Classes/Perception/AIPerceptionTypes.h"
#include "AIModule/Classes/Perception/AIPerceptionSystem.h"

#include "MyrSmellEmitter.h"				// для генерации облака запаха

#include "../Creature/MyrPhyCreature.h"		// когда замечает нас существо
#include "../Creature/MyrPhyCreatureMesh.h"	// тело существа, для обработки касаний

//==============================================================================================================
// Sets default values
//==============================================================================================================
AMyrArtefact::AMyrArtefact()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 2.0f;

	//источник эффекта частиц запаха
	SmellEmitter = CreateDefaultSubobject<UMyrSmellEmitter>(TEXT("Smell Emitter"));
	RootComponent = SmellEmitter;
	SmellEmitter->SetRelativeLocation(FVector(0, 0, 0));
	SmellEmitter->SetUsingAbsoluteRotation(true);
}

//==============================================================================================================
//после загрузки
//==============================================================================================================
void AMyrArtefact::PostLoad()
{
	Super::PostLoad();

	//поиск материального воплощения
	for (auto& C : GetComponents())
	{	if(auto SMC = Cast<USkeletalMeshComponent>(C))	{	Skeletal = true;	Mesh = SMC;	break;	} else
		if(auto TMC = Cast<UStaticMeshComponent>(C))	{	Skeletal = false;	Mesh = TMC;	break;	}
	}

	//подвязка обработчиков столкновений
	if(Mesh) Mesh->OnComponentHit.AddDynamic(this, &AMyrArtefact::OnHit);

}

//==============================================================================================================
// Called when the game starts or when spawned
//==============================================================================================================
void AMyrArtefact::BeginPlay()
{
	//зарегистрировать источник зрительных сигналов - артефакт должно быть видно
	//хотя может быть оставить только слух, а то артефакты - в основном для игрока, а неписи не должны работать точно
	UAIPerceptionSystem::RegisterPerceptionStimuliSource(this, UAISense_Hearing::StaticClass(), this);
	Super::BeginPlay();
}

//==============================================================================================================
// Called every frame
//==============================================================================================================
void AMyrArtefact::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

#if WITH_EDITOR
//==============================================================================================================
//для запуска перестройки структуры после изменения параметров
//==============================================================================================================
void AMyrArtefact::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	//найти компоненты, добавленные в редакторе и подцепить их в указатели
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

//удар твердлого тела
void AMyrArtefact::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
}

//охватить этот артефакт при загрузке и сохранении игры
void AMyrArtefact::Save(FArtefactSaveData& Dst)
{
}
void AMyrArtefact::Load(const FArtefactSaveData& Src)
{
}
#endif
