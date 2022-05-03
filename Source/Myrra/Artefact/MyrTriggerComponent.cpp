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
//свершить генерацию объекта согласно внутренним правилам
//==============================================================================================================
bool UMyrTriggerComponent::DoSpawn(int ind)
{
	//сгенерировать случайный
	if (ind == -1)
	{
		//если заведен только один тип объекта, его и генерировать
		if (Spawnables.Num() == 1)
			return SpawnIt(Spawnables[0]);

		//вариантов существ много
		else
		{	//взвешенное случайное - геморная хня через новый массив 
			TArray<uint16> WeightLadder;
			WeightLadder.SetNum(Spawnables.Num());
			int Sum = 0;
			for (int i = 0; i < Spawnables.Num(); i++)
			{
				Sum += Spawnables[i].Chance;
				WeightLadder[i] = Sum;
			}
			int R = FMath::RandRange(0, Sum);
			for (int i = 0; i < WeightLadder.Num(); i++)
			{
				if (R < WeightLadder[i])
					return SpawnIt(Spawnables[i]);
			}
		}
	}
	//сгенерировать определенный
	else
	{
		if (ind < Spawnables.Num())
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
	if (SpawnTypeInfo.MaxNumberOfPresent <= SpawnTypeInfo.Spawned.Num())
		return false;
	if (SpawnTypeInfo.MaxNumberOfEverSpawned <= SpawnTypeInfo.NumberOfEverSpawned)
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
	AActor* Novus = GetWorld()->SpawnActor(SpawnTypeInfo.WhatToSpawn.Get(), &Place, &cR, cAsp);
	if (Novus)
	{
		//если это существо, вместе с ним создать его ИИ, сам по себе он почему-то не создаётся
		if (auto nP = Cast<AMyrPhyCreature>(Novus))
			nP->RegisterAsSpawned(GetOwner());

		//занести в список
		SpawnTypeInfo.Spawned.Add(Novus);
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
//действия по изменению расстояния камеры
//==============================================================================================================
bool UMyrTriggerComponent::ReactionCameraDist(class AMyrDaemon *D, FTriggerReason& R, bool Release)
{
	//в конце вернуть прежнюю дистанцию
	if (Release)
	{
		//заранее вызываем удаление объёма, чтобы при восстановлении расстояния камеры прочесть предыдущую запись
		D->GetOwnedCreature()->DelOverlap(this);
		float WhatRemains = D->ResetCameraPos();
		UE_LOG(LogTemp, Log, TEXT("%s: ReactionCameraDist End %g"), *GetName(), WhatRemains);
	}
	//в начале считать новую дистанцию
	else
	{
		//если этот триггер-объём завязан на меш, который сейчас отключён - то и функция триггера отключается
		if (auto SC = Cast<USwitchableStaticMeshComponent>(GetAttachParent()))
			if (!SC->IsOn())
				 return false;

		if (R.Notify == ETriggerNotify::CanEat)
			if (auto A = Cast<AMyrArtefact>(GetOwner()))
				return !A->EffectsWhileEaten.Empty();

		//выдрать из текста парамтера значение, на которое приближать камеру
		float CamDist = FCString::Atof(*R.Value);
		D->ChangeCameraPos(CamDist);
		UE_LOG(LogTemp, Log, TEXT("%s: ReactionCameraDist Begin %g"), *GetName(), CamDist);
	}
	return true;
}

//==============================================================================================================
//реакция на установку векшней позиции камеры
//==============================================================================================================
bool UMyrTriggerComponent::ReactCameraExtPoser(AMyrDaemon* D, bool Release)
{
	if (D)
	{	
		if (Release) { D->DeleteExtCameraPoser(); }
		else
		{
			USceneComponent* CamPos = nullptr;
			if (GetAttachChildren().Num())
				CamPos = GetAttachChildren()[0];
			else return false;
			D->AdoptExtCameraPoser(CamPos);
		}
		UE_LOG(LogTemp, Log, TEXT("%s: ReactCameraExtPoser %d"), *GetName(), Release);

	}
	return true;
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
bool UMyrTriggerComponent::ReactionEat(AMyrPhyCreature* C, bool* EndChain, FTriggerReason& R)
{
	//кушать можно только акторы-артефакты
	if (auto A = Cast<AMyrArtefact>(GetOwner()))
	{
		//текущее воплощение предмета еды изначально не содержит пищевой ценности - возможно, его уже съели
		if(A->EffectsWhileEaten.Empty())
		{	UE_LOG(LogTemp, Log, TEXT("%s: No Edible Object"), *GetName());
			return false;
		}

		auto SM = Cast<USwitchableStaticMeshComponent>(GetAttachParent());

		//количество пищи за один кусь указывается явно в параметрах
		float MeatForBit = 1.0f;
			
		//для многоликого меша число кусей определяется числом вариаций, отсюда доля на каждый кусь
		if (SM)	MeatForBit = 1.0f / (SM->Variants.Num() - 1);
		else	MeatForBit = FCString::Atof(*R.Value);
		if (MeatForBit <= 0.0f) MeatForBit = 1.0f;

		// акт логического поедания, удачность означает успех всей функции
		if (C->EatConsume(this, &A->EffectsWhileEaten, MeatForBit))
		{
			//если многоликий меш - продвинуть на новый образ, в котором меньше осталось еды
			if (SM)	SM->SetMesh(SM->GetCurrent() + 1);

			//если всё съели, триггер считается досрочно пересеченным и не подсказывает о съедобности
			if (EndChain) *EndChain = A->EffectsWhileEaten.Empty();

			return true;
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
//или удаление со сцены предмета, пока неясно, по каким критериям
//==============================================================================================================
bool UMyrTriggerComponent::ReactDestroy(class AMyrPhyCreature* C, AMyrArtefact* A, FTriggerReason& R)
{
	if(C)
		if(C->HasAnyFlags(RF_Transient))
			if (C->Age > FCString::Atof(*R.Value))
			{	C->Destroy();
				return true;
			}
	if(A)
		if(A->HasAnyFlags(RF_Transient))
		{	C->Destroy();
			return true;
		}
	return false;
}

//==============================================================================================================
//кинематически переместить себя или предмет в другое место
//==============================================================================================================
bool UMyrTriggerComponent::ReactTeleport(FTriggerReason& R, class AMyrPhyCreature* C, class AMyrArtefact* A, bool Deferred)
{
	//найти высиратель по имени в том же акторе и высрать из него то, что он умеет
	auto S = Cast<UMyrTriggerComponent>(GetOwner()->GetDefaultSubobjectByName(FName(*R.Value)));
	if(!S) S = this;
	if (C)
	{	
		//извлечение режима (полностью, только позиция, позиция плюс ориентация без знака) из базового режима (сразу или постепенно)
		int ExactReason = Deferred ? ((int)R.Why - (int)EWhyTrigger::KinematicLerpToCenter) : ((int)R.Why - (int)EWhyTrigger::Teleport);

		//основное различие режимов - как вращать существо, поэтому сюда будут сваливаться все случаи
		FQuat ResRot = S->GetComponentQuat();

		//вторая константа в группе = только локация, а значит надо сохранить уже имеющееся вращение существа
		if (ExactReason == 1) ResRot = C->GetActorQuat();

		//третья константа в группе = локация плюс ориентация по оси, в какую сторону ближе
		else if (ExactReason == 2)
		{
			if ((C->GetActorForwardVector() | S->GetComponentTransform().GetUnitAxis(EAxis::X)) > 0)
				ResRot = S->GetComponentQuat();
			else  ResRot = S->GetComponentQuat().Inverse();
		}

		//долгое приведение, вызываеся каждый кадр
		if (Deferred)
		{
			//если за все тики существо уже достаточно прибилзилось к центру
			float d2 = FVector::DistSquared(C->GetActorLocation(), S->GetComponentLocation());

			//непонятно, откуда брать квант - пока что из размеров самого триггера, но возможно, надо из размеров существа
			float quant = FMath::Square(Bounds.SphereRadius * 0.1);
			if (d2 <= quant)
			{
				//финальный шаг эквивалентен телепорту
				if(!C->GetMesh()->IsBumpable())
					C->TeleportToPlace(FTransform(ResRot, S->GetComponentLocation()));

				//но если уже телепортили и сейчас просто удерживаем, значит более упрощенная форма
				else
				{
					C->GetMesh()->SetSimulatePhysics(false);
					C->SetActorTransform(FTransform(ResRot, S->GetComponentLocation()), false, nullptr, ETeleportType::TeleportPhysics);
					C->GetMesh()->SetSimulatePhysics(true);
				}

				//убрать размовение
				if (C->Daemon) C->Daemon->SetMotionBlur(0);

				//возвратить колизии телу существа перед выходом из кинематики
				if(!C->GetMesh()->IsBumpable()) C->GetMesh()->SetPhyBodiesBumpable(true);
				return true;
			}
			//за предыдущий тик дорогу не осилили, продолжать рутинное приближение
			else
			{
				//скорость перемещения брать из параметров триггера
				float Alpha = FCString::Atof(*R.Value);
				if (Alpha == 0) Alpha = 0.2;

				//по мере приближения широта шага растёт, чтоб не замедляться перед целью, ибо афизично
				Alpha = FMath::Min(1.0f, Alpha + quant / d2);

				//вычислить новые позицю и вращение
				FQuat NewRot = FMath::Lerp(C->GetActorQuat(), ResRot, Alpha);
				FVector NewLoc = FMath::Lerp(C->GetActorLocation(), S->GetComponentLocation(), Alpha);

				//применить перемещение 
				C->SetActorTransform(FTransform(NewRot, NewLoc), false, nullptr, ETeleportType::TeleportPhysics);

				//на всякий случай погасить скорость, ибо кажется из-за нее перелетает;
				C->GetMesh()->GetBodyInstance()->SetLinearVelocity(FVector(0), false);
				return false;
			}
		}

		//мгновенная телепортация
		else
		{	
			//вызывается непосредственно из этого класса при пересечении
			C->TeleportToPlace(FTransform(ResRot, S->GetComponentLocation()));
			C->GetMesh()->SetPhysicsLinearVelocity(FVector(0));
			return true;
		}
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
//обработать случай входа и выхода из локации -это нужно явно, потому что в локации могут быть другие триггеры
//которые не эквивалентны полному объёму локации
//==============================================================================================================
bool UMyrTriggerComponent::ReactEnterLocation(class AMyrPhyCreature* C, class AMyrArtefact* A, bool Enter)
{
	if(auto L = Cast<AMyrLocation>(GetOwner()))
	{	if(Enter)
		{	if(C) L->AddCreature(C);
			if(A) L->AddArtefact(A);
		}
		else
		{	if(C) L->RemoveCreature(C);
			if(A) L->RemoveArtefact(A);
		}
	}
	return true;
}

//==============================================================================================================
//сформировать вектор дрейфа по векторному полю, вызывается не отсюда, а из ИИ
//==============================================================================================================
FVector UMyrTriggerComponent::ReactVectorFieldMove(class AMyrPhyCreature* C)
{
	//накопитль вектора
	FVector Accu(0,0,0);
	if(!C) return Accu;

	//вектора задаются дочерними компонентами (для наглядности стрелками), откуда берется ось Х
	TArray<USceneComponent*> Children = GetAttachChildren();

	//если вектор только один, взвешенной суммы не требуется, просто вернуть его
	if(Children.Num()==1)
		return Children[0]->GetComponentTransform().GetUnitAxis(EAxis::X);

	//нормировочный коэффициент = максимальный размер всего объёма, чтобы расстояния до стрелок были весами меньше единицы
	float Normer = 0.5 / (Bounds.SphereRadius);
	float Denominator = 0;
	for(auto Ch : Children)
	{
		//весовой вклад вектора максимален при близости и ноль при дальности на другом конце поля
		float Weight = 1.0f - FVector::DistSquared(C->GetActorLocation(), Ch->GetComponentLocation()) * Normer * Normer;
		if(Weight < 0) Weight = 0;
		Accu += Ch->GetComponentTransform().GetUnitAxis(EAxis::X) * Weight;
		Denominator += Weight;
	}
	if(Denominator > 1) Accu /= Denominator;
	return Accu;
}


//==============================================================================================================
//прореагировать одну строку реакции - возвращает тру когда объект еще включается, для интерфейса написать
//==============================================================================================================
bool UMyrTriggerComponent::ReactSingle(FTriggerReason& Reaction, class AMyrPhyCreature* C, class AMyrArtefact* A, bool Release, bool* EndChain)
{
	switch (Reaction.Why)
	{
		case EWhyTrigger::CameraDist:
			if (C->Daemon) return ReactionCameraDist(C->Daemon, Reaction, Release);
			break;

		case EWhyTrigger::CameraExternalPos:
			if (C->Daemon) ReactCameraExtPoser(C->Daemon, Release);
			break;

		case EWhyTrigger::UnlockDoorLightButton:
		case EWhyTrigger::UnlockDoorDimButton:
		case EWhyTrigger::UnlockDoorFlashButton:
			ReactionOpenDoor(C, Reaction, Release);
			break;

		case EWhyTrigger::Eat:
			if (Release) return ReactionEat(C, EndChain, Reaction);
			break;

		case EWhyTrigger::SpawnAtComeIn:
		case EWhyTrigger::SpawnAtComeOut:
			ReactSpawn(Reaction, Release);
			break;

		case EWhyTrigger::SpawnAtInDestroyAtOut:
			if(Release) ReactDestroy(C, A, Reaction);
			else ReactSpawn(Reaction, Release);
			break;

		case EWhyTrigger::Destroy:
			if(!Release) ReactDestroy(C, A, Reaction);
			break;

		case EWhyTrigger::Teleport:
		case EWhyTrigger::TeleportOnlyLocation:
		case EWhyTrigger::TeleportLocOrient:
			if(!Release) ReactTeleport(Reaction, C, A, false);
			break;

		case EWhyTrigger::KinematicLerpToCenter:
		case EWhyTrigger::KinematicLerpToCenterLocationOnly:
		case EWhyTrigger::KinematicLerpToCenterLocationOrientation:
			C->bKinematic = !Release;
			if (C->bKinematic)
			{	C->GetMesh()->SetPhyBodiesBumpable(false);
				if (C->Daemon)
					C->Daemon->SetMotionBlur(10.0f);
			}
			break;

		case EWhyTrigger::Quiet:
			ReactQuiet(C, Release);
			break;

		case EWhyTrigger::EnterLocation:
			if(!Release) ReactEnterLocation(C, A, !Release);
			break;

		//внимание, эта реакция просто делегируется сюда квестом, 
		//внутри триггера она не должна использоваться
		case EWhyTrigger::PlaceMarker:
			if (C->Daemon) C->Daemon->PlaceMarker(this);
			break;
	}
	return true;
}

//==============================================================================================================
//выбор реакции в ответ на вход в объём или выход из объёма
//==============================================================================================================
void UMyrTriggerComponent::React(class AMyrPhyCreature* C, class AMyrArtefact* A, bool Release)
{

	//не запускать, если не отработана выдержка до завершения или между концом и новым началом
	if(Release)
	{ if (!CheckTimeInterval(SecondsToHold, TEXT("SecondsToHold"))) return;	}
	else if(!CheckTimeInterval(SecondsToRest, TEXT("SecondsToRest"))) return;

	//зарегистрировать, если на входе
	if (!Release) C->AddOverlap(this);

	//на выходе, проверить, не вышли ли заранее, если вышли, то ничего не делать
	else if(!C->HasOverlap(this))
		return;

	//включенность объекта проверяется внутри и влияет на выдачу уведомлений, для каждой реакции в отдельности
	bool ItemTurnedOn = false;

	//если тру, то этой функцией считать завершенным пересечение,
	bool EndChain = true;

	//перебор всяческих реакций
	for(auto& Reaction : Reactions)
	{
		//пропарсить отдельную реакцию
		ItemTurnedOn = ReactSingle(Reaction, C, A, Release, &EndChain);

		//раздача нотификаций на экран
		if (!Release) C->WigdetOnTriggerNotify(Reaction.Notify, GetOwner(), GetAttachParent(), ItemTurnedOn);
	}

	//если по итогам исполнения больше от этого объёма ничего не нужно, удалить его из стека
	if (Release && EndChain)
	{
		UE_LOG(LogTemp, Log, TEXT("%s Early deleting overlapper %s"), *GetName(), *C->GetName());
		C->DelOverlap(this);
	}
}





//==============================================================================================================
//принять от существа подтверждение, что нажата кнопка и действие совершено
//==============================================================================================================
void UMyrTriggerComponent::ReceiveActiveApproval(AMyrPhyCreature* Sender)
{
	//существо вызывает эту функцию само, когда внутри себя видит пересечение
	//поэтому реагировать на него нужно только если предусмотрен активный досрочный спуск
	//почему false?
	if (PerformOnlyByApprovalFromCreature)
	{
		UE_LOG(LogTemp, Log, TEXT("%s React by instant approval"), *GetOwner()->GetName());
		React(Sender, nullptr, true);
	}
}

//==============================================================================================================
//начало пересечения с внешним объектом 
//==============================================================================================================
UFUNCTION() void UMyrTriggerComponent::OverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//пересечение поясов не должно срабатывать, чтоб не получать множественные
	if (OtherComp->IsA<UMyrGirdle>()) return;

	//только если пересекло существо
	auto C = Cast<AMyrPhyCreature>(OtherActor);
	auto A = Cast<AMyrArtefact>(OtherActor);
	if (C || A)
	{
		//вызывать начяальную стадию реакции
		React(C, A, false);

		//только пересеклись с существом, а оно уже заранее успело активировать атаку, по которой этот триггер должен срабатывать
		if (PerformOnlyByApprovalFromCreature)
			if (C->CouldSendApprovalToTrigger(this))
			{
				UE_LOG(LogTemp, Log, TEXT("%s React by deferred approval"), *GetOwner()->GetName());
				React(C, A, true);
			}

		//возможно, этот триггер помимо всего прочего используется для влияния на целую игру
		if (GenerateMyrLogicMsgOnIn)
			C->CatchMyrLogicEvent(EMyrLogicEvent::ObjAffectTrigger, 1.0f, this);
	}

	UE_LOG(LogTemp, Log, TEXT("%s.%s: OverlapBegin %s.%s.%d"),
		*GetOwner()->GetName(), *GetName(), *OtherActor->GetName(), *OtherComp->GetName(), OtherBodyIndex);
}

//==============================================================================================================
//конец пересечения с внешним объектом
//==============================================================================================================
UFUNCTION() void UMyrTriggerComponent::OverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//пересечение поясов не должно срабатывать, чтоб не получать множественные
	if (OtherComp->IsA<UMyrGirdle>()) return;

	//только если пересекло существо
	auto C = Cast<AMyrPhyCreature>(OtherActor);
	auto A = Cast<AMyrArtefact>(OtherActor);
	if (C || A)
	{
		//автоматически по выходу из объёма применять функцию только в том случае, если не включено применение по действию
		if (!PerformOnlyByApprovalFromCreature)
		React(C, A, true);

		//повторно вызывается если в реакте так и не вызвалось
		C->DelOverlap(this);

		//возможно, этот триггер помимо всего прочего используется для влияния на целую игру
		if (GenerateMyrLogicMsgOnOut)
			C->CatchMyrLogicEvent(EMyrLogicEvent::ObjAffectTrigger, 1.0f, this);
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

	//ещё бы как-то сделать направитель по вектору

	//под капотом
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
