// Fill out your copyright notice in the Description page of Project Settings.

#include "Artefact/MyrTriggerComponent.h"
#include "Myrra.h"
#include "Artefact/MyrArtefact.h"								//актор, потенциально съедобный, в который такие компоненты обычно входят
#include "Artefact/MyrLocation.h"								//актор, статичный но чуть меняемый модуль здания
#include "Creature/MyrPhyCreature.h"							//существо вызывает событие
#include "MyrraDoor.h"											//чтобы отпирать двери
#include "SwitchableStaticMeshComponent.h"						//чтобы переключать многоликие меши
#include "../Control/MyrDaemon.h"								//чтобы работать с камерой игрока
#include "AIModule/Classes/Perception/AISenseConfig_Hearing.h"	//чтобы генерировать зовы в функции аттрактора


//==============================================================================================================
//правильный конструктор
//==============================================================================================================
UMyrTriggerComponent::UMyrTriggerComponent(const FObjectInitializer& ObjectInitializer) :Super(ObjectInitializer)
{
	//чтобы вызывалась функция OverlapEnd, когда из зубов бросают
	SetGenerateOverlapEvents(true);

	//осторожно, этот пресет должен уже быть в проекте
	SetCollisionProfileName(TEXT("MyrTrigger"));
	OnComponentBeginOverlap.AddDynamic(this, &UMyrTriggerComponent::OverlapBegin);
	OnComponentEndOverlap.AddDynamic(this, &UMyrTriggerComponent::OverlapEnd);
	LastOverlapEndTime = FDateTime(0);
}

//==============================================================================================================
//проверить время с последнего финала до нового начинания
//==============================================================================================================
bool UMyrTriggerComponent::CheckTimeInterval(float& MyParam, FString Msg)
{
	//еще не было предыдущих запусков, ничто не ограничивает
	if (LastOverlapEndTime.GetTicks() == 0) return true;

	//между прошлым временем активации этой функции и этим
	FTimespan Di = FDateTime::Now() - LastOverlapEndTime;
	if (Di.GetTotalSeconds() < MyParam)
	{	UE_LOG(LogTemp, Warning, TEXT("%s.%s: Too Early %s %g < %g"),
			*GetOwner()->GetName(), *GetName(), *Msg, Di.GetTotalSeconds(), MyParam);
		return false;
	}

	//новый цикл
	LastOverlapEndTime = FDateTime::Now();
	return true;
}

//==============================================================================================================
//действия по изменению расстояния камеры
//==============================================================================================================
bool UMyrTriggerComponent::ReactionCameraDist(class AMyrDaemon *D, FTriggerReason& R, bool Release)
{
	//если этот триггер-объём завязан на меш, который сейчас отключён - то и функция триггера отключается
	bool Available = true;
	if (auto SC = Cast<USwitchableStaticMeshComponent>(GetAttachParent()))
		if (!SC->IsOn())
			Available = false;

	//выдрать из текста парамтера значение, на которое приближать камеру
	float CamDist = Available ? FCString::Atof(*R.Value) : 1.0f;

	//в конце вернуть прежнюю дистанцию
	if(Release)	D->ResetCameraPos();
	else D->ChangeCameraPos(CamDist);
	return Available;
}

//==============================================================================================================
//действия по открытию двери кнопкой
//==============================================================================================================
void UMyrTriggerComponent::ReactionOpenDoor(class AMyrPhyCreature* C, FTriggerReason& R, bool Release)
{
	//если этот триггер-объем привязан к некоему переключаемому мешу - тракторвать его как кнопку
	//и зажигать соответственно установленному режиму
	if (auto Button = Cast<USwitchableStaticMeshComponent>(GetAttachParent()))
	{
		//засветить кнопкой: в начале, когда надо сверкнуть, в конце, когда надо оставить зажженной
		bool ButtLight = (R.Why == (Release ? EWhyTrigger::UnlockDoorLightButton : EWhyTrigger::UnlockDoorFlashButton));
		Button->TurnOnOrOff(ButtLight);
	}

	//сама дверь отпирается только на выходе
	if(!Release) return;

	//найти дверь по имени в том же акторе и отпереть её - и награть сущство за это
	if (auto Door = Cast<UMyrDoorLeaf>(GetOwner()->GetDefaultSubobjectByName(FName(*R.Value))))
	{	Door->Unlock();
		C->CatchMyrLogicEvent(EMyrLogicEvent::ObjUnlockDoor, 1.0f, this);
	}
}

//==============================================================================================================
//действия по съеданию
//==============================================================================================================
bool UMyrTriggerComponent::ReactionEat(AMyrPhyCreature* C)
{
	//кушать можно только акторы-артефакты
	if (auto A = Cast<AMyrArtefact>(GetOwner()))
	{
		//если данный триггер объем прикреплен к вариативному мешу, который и отображает предмет еды
		if (auto M = Cast<USwitchableStaticMeshComponent>(GetAttachParent()))
		{
			//текущее воплощение предмета еды не содержит пищевой ценности - возможно, его уже съели
			if (M->GetCurrent() > A->WhatItDoesIfEaten.Num() - 1)
			{	UE_LOG(LogTemp, Log, TEXT("%s: No Edible Object"), *GetName());
				return false;
			}

			//съесть и протолкнуть меш на новый образ (пустая тарелка, например)
			if (C->EatConsume(this, &(A->WhatItDoesIfEaten[M->GetCurrent()])))
			{	M->SetMesh(M->GetCurrent() + 1);
				return true;
			}
		}
	}
	return false;
}

//==============================================================================================================
//реакция - высер нового объекта
//==============================================================================================================
bool UMyrTriggerComponent::ReactSpawn(FTriggerReason& R, bool Release)
{
	//хитрым образом соединяем два условия, чтобы получить вызов вовремя - в начале или в конце пересечения
	if((R.Why == EWhyTrigger::SpawnAtComeOut) == Release)
	{
		//найти высиратель по имени в том же акторе и высрать из него то, что он умеет
		if (auto S = Cast<UMyrTriggerComponent>(GetOwner()->GetDefaultSubobjectByName(FName(*R.Value))))
			return S->DoSpawn();

		//если же параметр не указан - высирать прямо из себя (если есть что)
		else if(Spawnables.Num()>0)	DoSpawn();
	}
	return false;
}

//==============================================================================================================
//реакция - убийство попавшего в объём существа, которое достаточно старо, чтобы подпадать под условия
//==============================================================================================================
bool UMyrTriggerComponent::ReactDestroy(class AMyrPhyCreature* C, FTriggerReason& R)
{
	if(C->HasAnyFlags(RF_Transient))
		if (C->Age > FCString::Atof(*R.Value))
		{
			C->Destroy();
			return true;
		}
	return false;
}

//==============================================================================================================
//залезть в коробку умиротворения
//==============================================================================================================
bool UMyrTriggerComponent::ReactQuiet(AMyrPhyCreature* C, bool Release)
{

	//дополнительно отключить, если этот триггер приделан к многоликому мешу, который в состоянии "выкл"
	//например закрытый ящик
	if (auto V = Cast<USwitchableStaticMeshComponent>(GetAttachParent()))
		if (!V->IsOn()) return false;
	if(Release)	C->CatchMyrLogicEvent(EMyrLogicEvent::ObjExitQuietPlace, 1.0f, this);
	else		C->CatchMyrLogicEvent(EMyrLogicEvent::ObjEnterQuietPlace, 1.0f, this);
	return true;
}


//==============================================================================================================
//выбор реакции в ответ на вход в объём или выход из объёма
//==============================================================================================================
void UMyrTriggerComponent::React(class AMyrPhyCreature* C, bool Release)
{
	//не запускать (повторно), если показываеет что пересекатель уже окучен, но еще не вышел из объёма 
	if (BurntOutAffectors.Contains(C)) return;

	//не запускать, если не отработана выдержка до завершения или между концом и новым началом
	if(Release)
	{ if (!CheckTimeInterval(SecondsToHold, TEXT("SecondsToHold"))) return;	}
	else if(!CheckTimeInterval(SecondsToRest, TEXT("SecondsToRest"))) return;

	//перебор всяческих реакций
	bool ItemTurnedOn = false;
	for(auto Reaction : Reactions)
	{
		switch (Reaction.Why)
		{
			case EWhyTrigger::CameraDist:
				if (C->Daemon)
					ItemTurnedOn = ReactionCameraDist(C->Daemon, Reaction, Release);
				break;

			case EWhyTrigger::UnlockDoorLightButton:
			case EWhyTrigger::UnlockDoorDimButton:
			case EWhyTrigger::UnlockDoorFlashButton:
				ReactionOpenDoor(C, Reaction, Release);
				break;

			case EWhyTrigger::Eat:
				if(Release) ReactionEat(C);

			case EWhyTrigger::SpawnAtComeIn:
			case EWhyTrigger::SpawnAtComeOut:
				ReactSpawn(Reaction, Release);
				break;

			case EWhyTrigger::SpawnAtInDestroyAtOut:
				if(Release) ReactDestroy(C, Reaction);
				else ReactSpawn(Reaction, Release);
				break;

			case EWhyTrigger::Destroy:
				if(!Release) ReactDestroy(C, Reaction);
				break;

			case EWhyTrigger::Quiet:
				ItemTurnedOn = ReactQuiet(C, Release);
				break;
		}

		//запомнить, чтобы для этого пересекателя больше не применять
		if(Release)	BurntOutAffectors.Add(C);

		//раздача нотификаций на экран
		else C->WigdetOnTriggerNotify(Reaction.Notify, GetOwner(), GetAttachParent(), ItemTurnedOn);
	}
}


//==============================================================================================================
//свершить генерацию объекта согласно внутренним правилам
//==============================================================================================================
bool UMyrTriggerComponent::DoSpawn(int ind)
{
	//сгенерировать случайный
	if(ind == -1)
	{
		//если заведен только один тип объекта, его и генерировать
		if(Spawnables.Num() == 1)
			return SpawnIt(Spawnables[0]);

		//вариантов существ много
		else
		{	//взвешенное случайное - геморная хня через новый массив 
			TArray<uint16> WeightLadder;
			WeightLadder.SetNum(Spawnables.Num());
			int Sum = 0;
			for (int i = 0; i < Spawnables.Num(); i++)
			{	Sum += Spawnables[i].Chance;
				WeightLadder[i] = Sum;
			}
			int R = FMath::RandRange(0, Sum);
			for (int i = 0; i < WeightLadder.Num(); i++)
			{	if (R < WeightLadder[i])
					return SpawnIt(Spawnables[i]);
			}
		}
	}
	//сгенерировать определенный
	else
	{	if(ind < Spawnables.Num())
			return SpawnIt(Spawnables[ind]);
	}
	return false;
}

//==============================================================================================================
//высрать вполне определенный объект
//==============================================================================================================
bool UMyrTriggerComponent::SpawnIt(FSpawnableData& SpawnTypeInfo)
{
	//не создавать, если превысится количество живых
	if(SpawnTypeInfo.MaxNumberOfPresent <= SpawnTypeInfo.Spawned.Num())
		return false;
	if(SpawnTypeInfo.MaxNumberOfEverSpawned <= SpawnTypeInfo.NumberOfEverSpawned)
		return false;

	//не создавать, если слишком мало времени с момента последнего создания
	FTimespan Di = FDateTime::Now() - SpawnTypeInfo.LastlySpawned;
	if (Di.GetTotalSeconds() < SpawnTypeInfo.MinSecondsBetween)
		return false;

	//хз, надо посмотреть, что здесь еще
	FActorSpawnParameters Asp;
	Asp.Owner = GetOwner();
	const FActorSpawnParameters& cAsp = Asp;

	//место, где высирать
	FVector Place = GetComponentLocation();
	Place = FMath::RandPointInBox(Bounds.GetBox());
	const FVector* cP = &Place;
	const auto cR = GetComponentRotation();

	//высрать
	AActor* Novus = GetWorld()->SpawnActor ( SpawnTypeInfo.WhatToSpawn.Get(), &Place, &cR, cAsp);
	if(Novus)
	{
		//если это существо, вместе с ним создать его ИИ, сам по себе он почему-то не создаётся
		if(auto nP = Cast<AMyrPhyCreature>(Novus))
			nP->RegisterAsSpawned(GetOwner());

		//занести в список
		SpawnTypeInfo.Spawned.Add (Novus);
		SpawnTypeInfo.NumberOfEverSpawned++;

		//зафиксировать время последнего высера
		SpawnTypeInfo.LastlySpawned = FDateTime::Now();

		//привязать сигнал, когда он сдохнет, чтоб мы сразу об этом узнали
		Novus->OnEndPlay.AddDynamic(this, &UMyrTriggerComponent::OnSpawnedEndPlay);
		UE_LOG(LogTemp, Log, TEXT(" UMyrTriggerComponent %s spawned %s"), *GetName(), *Novus->GetName());
		return true;
	}
	return false;
}


//==============================================================================================================
//принять от существа подтверждение, что нажата кнопка и действие совершено
//==============================================================================================================
void UMyrTriggerComponent::ReceiveActiveApproval(AMyrPhyCreature* Sender)
{
	//существо вызывает эту функцию само, когда внутри себя видит пересечение
	//поэтому реагировать на него нужно только если предусмотрен активный досрочный спуск
	if (PerformOnlyByApprovalFromCreature)	React(Sender, false);
}

//==============================================================================================================
//начало пересечения с внешним объектом 
//==============================================================================================================
UFUNCTION() void UMyrTriggerComponent::OverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//только если пересекло существо
	if (auto C = Cast<AMyrPhyCreature>(OtherActor))
	{
		//вызывать начяальную стадию реакции
		React(C, false);

		//только пересеклись с существом, а оно уже заранее успело активировать атаку, по которой этот триггер должен срабатывать
		if (PerformOnlyByApprovalFromCreature)
			if (C->CouldSendApprovalToTrigger(this))
				React(C, true);

		//возможно, этот триггер помимо всего прочего используется для влияния на целую игру
		if (GenerateMyrLogicMsgOnIn)
			C->CatchMyrLogicEvent(EMyrLogicEvent::ObjAffectTrigger, 1.0f, this);

		if(auto L = Cast<AMyrLocation>(GetOwner()))
			C->CatchMyrLogicEvent(EMyrLogicEvent::ObjEnterLocation, 1.0f, this);
	}

	UE_LOG(LogTemp, Log, TEXT("%s.%s: OverlapBegin %s.%s.%d"),
		*GetOwner()->GetName(), *GetName(), *OtherActor->GetName(), *OtherComp->GetName(), OtherBodyIndex);
}

//==============================================================================================================
//конец пересечения с внешним объектом
//==============================================================================================================
UFUNCTION() void UMyrTriggerComponent::OverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//только если пересекло существо
	if (auto C = Cast<AMyrPhyCreature>(OtherActor))
	{
		//автоматически по выходу из объёма применять функцию только в том случае, если не включено применение по действию
		if (!PerformOnlyByApprovalFromCreature)
			React(C, true);

		//исключить отработанный, чтобы в следующий раз можно было заново начать
		BurntOutAffectors.Remove(C);

		//возможно, этот триггер помимо всего прочего используется для влияния на целую игру
		if (GenerateMyrLogicMsgOnOut)
			C->CatchMyrLogicEvent(EMyrLogicEvent::ObjAffectTrigger, 1.0f, this);

		if(auto L = Cast<AMyrLocation>(GetOwner()))
			C->CatchMyrLogicEvent(EMyrLogicEvent::ObjExitLocation, 1.0f, this);
	}

	UE_LOG(LogTemp, Log, TEXT("%s.%s: OverlapEnd %s"),
		*GetOwner()->GetName(), *GetName(), *OtherActor->GetName());
}

//==============================================================================================================
//вызывается, когда один из высранных акторов умирает
//==============================================================================================================
void UMyrTriggerComponent::OnSpawnedEndPlay(AActor* Actor, EEndPlayReason::Type Why)
{
	//удалить нового мертвеца из множества
	for (auto SI : Spawnables)
		if (SI.WhatToSpawn == Actor->StaticClass())
			if (SI.Spawned.Contains(Actor))
				SI.Spawned.Remove(Actor);
}

//==============================================================================================================
//перед самим началом игры
//==============================================================================================================
void UMyrTriggerComponent::BeginPlay()
{
	//без этого ругаеся
	Super::BeginPlay();

	//если этот триггер используется как маяк, то включить возможность тика
	//хотя неясно, можно ли это делать позже конструктора
	//неясно, включать его сразу же или при первой генерации
	//сразу же - расточительно, но зато этот триггер может не высирать а только собирать высранных другими
	for(auto R:Reactions)
		if(R.Why == EWhyTrigger::Attract)
			if(Spawnables.Num()>0)
			{
				//произвольная частота, чтобы все аттракторы в сцене примерно равно
				PrimaryComponentTick.TickInterval = FMath::RandRange(0.8f, 1.8f);
				PrimaryComponentTick.bCanEverTick = true;
				PrimaryComponentTick.SetTickFunctionEnable(true);
			}
}

//==============================================================================================================
//изредка тикать, если надо подзывать к себе высранных существ
//==============================================================================================================
void UMyrTriggerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	//если сигнал из локации
	if(auto L = Cast<AMyrLocation>(GetOwner()))
	{
		//дать знать существам, принявшим сигнал, куда бежать
		L->CurrentAISignalSource = this;
	}

	//прозвучать "виртуально", позвать домой, если существо опознает дом, и если пора, оно побежит
	UAISense_Hearing::ReportNoiseEvent(GetOwner(),
		GetComponentLocation(),
		(float)EHowSensed::CALL_OF_HOME,
		GetOwner(),
		0,	//здесь можно дальность распространения, но пока неясно, насколько нужно							
		TEXT("HeardSpecial"));


	//под капотом
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
