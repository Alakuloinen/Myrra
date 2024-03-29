// Fill out your copyright notice in the Description page of Project Settings.

#include "Artefact/MyrTriggerComponent.h"
#include "Myrra.h"
#include "../MyrraGameModeBase.h"									//для доступа к распорядителю всего уровня
#include "Artefact/MyrArtefact.h"								//актор, потенциально съедобный, в который такие компоненты обычно входят
#include "Artefact/MyrLocation.h"								//актор, статичный но чуть меняемый модуль здания
#include "Creature/MyrPhyCreature.h"							//существо вызывает событие
#include "MyrraDoor.h"											//чтобы отпирать двери
#include "SwitchableStaticMeshComponent.h"						//чтобы переключать многоликие меши
#include "../Control/MyrDaemon.h"								//чтобы работать с камерой игрока
#include "AIModule/Classes/Perception/AISenseConfig_Hearing.h"	//чтобы генерировать зовы в функции аттрактора

//свои дебаг записи
DEFINE_LOG_CATEGORY(LogMyrTrigger);


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
	{	UE_LOG(LogMyrTrigger, Warning, TEXT("%s.%s: Too Early %s %g < %g"),
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

	//если требуется высрать строго на поверхности земли
	if (SpawnTypeInfo.TraceForGround)
	{
		//ХУ берем рандом, а в высоту трассируем вниз до опоры
		Place += FVector(FMath::RandPointInCircle(Bounds.GetSphere().W), 0.0f);
		FHitResult Hit(ForceInit);
		GetWorld()->LineTraceSingleByChannel(Hit, Place, Place + 100 * FVector::DownVector, ECollisionChannel::ECC_WorldStatic);
		if (Hit.bBlockingHit) Place.Z = Hit.ImpactPoint.Z;
	}

	//по умолчанию в рандомном месте в пределах объёма
	else Place = FMath::RandPointInBox(Bounds.GetBox());

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
		UE_LOG(LogMyrTrigger, Log, TEXT(" UMyrTriggerComponent %s spawned %s"), *GetName(), *Novus->GetName());
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
		UE_LOG(LogMyrTrigger, Log, TEXT("%s: ReactionCameraDist End %g"), *GetName(), WhatRemains);
	}
	//в начале считать новую дистанцию
	else
	{
		//если этот триггер-объём завязан на меш, который сейчас отключён - то и функция триггера отключается
		if (auto SC = Cast<USwitchableStaticMeshComponent>(GetAttachParent()))
			if (!SC->IsOn())
				 return false;

		//выдрать из текста парамтера значение, на которое приближать камеру
		float CamDist = FCString::Atof(*R.Value);
		D->ChangeCameraPos(CamDist);
		UE_LOG(LogMyrTrigger, Log, TEXT("%s: ReactionCameraDist Begin %g"), *GetName(), CamDist);
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
		UE_LOG(LogMyrTrigger, Log, TEXT("%s: ReactCameraExtPoser %d"), *GetName(), Release);

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
		{	UE_LOG(LogMyrTrigger, Log, TEXT("%s: No Edible Object"), *GetName());
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
			UE_LOG(LogMyrTrigger, Log, TEXT("%s: Eaten %g part"), *GetName(), MeatForBit);

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
//реакция - уничтожение высраного объекта
//==============================================================================================================
bool UMyrTriggerComponent::ReactDestroySpawned(FTriggerReason& R)
{
	//найти высиратель по имени в том же акторе и высрать из него то, что он умеет
	auto Context = Cast<UMyrTriggerComponent>(GetOwner()->GetDefaultSubobjectByName(FName(*R.Value)));
	if (!Context) Context = this;

	//по всем классам высеров ищем все уже высранные и убираем их
	for (auto& SI : Context->Spawnables)
	{	for (auto& SA : SI.Spawned)
		{
			UE_LOG(LogMyrTrigger, Log, TEXT("%s ReactDestroySpawned destroying %s"), *GetName(), *SA->GetName());
			SA->Destroy();
		}
		//теперь множество пусто, его усечь
		SI.Spawned.Empty();
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
		//извлечение режима трансформации (полностью, только позиция, позиция плюс ориентация без знака) из базового режима (сразу или постепенно)
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
//вывести наэкран инфу относительно содержимого актора, к которому приадлежит этот триггер объём
//==============================================================================================================
bool UMyrTriggerComponent::ReactNotify(FTriggerReason& R, class AMyrPhyCreature* C)
{
	if(!C) return true;

	//если в строке не указано ничего, искать ответственный компонент в родителе нас
	if(R.Value.IsEmpty())
	{	if (auto V = Cast<USwitchableStaticMeshComponent>(GetAttachParent()))
		{	C -> WigdetOnTriggerNotify (R.Why, GetOwner(), V, V->IsOn());
			return V->IsOn();
		}
	}

	//если же строка есть - в ней имя компонента откуда брать сведения о включенности
	else if (auto S = Cast<USwitchableStaticMeshComponent>( GetOwner()->GetDefaultSubobjectByName(FName(*R.Value))) )
	{	C -> WigdetOnTriggerNotify (R.Why, GetOwner(), GetAttachParent(), S->IsOn());
		return S->IsOn();
	}

	return true;
}


//==============================================================================================================
//сформировать вектор дрейфа по векторному полю, вызывается не отсюда, а из ИИ
//==============================================================================================================
FVector3f UMyrTriggerComponent::ReactVectorFieldMove(FTriggerReason& R, class AMyrPhyCreature* C)
{
	//накопитль вектора
	FVector3f Accu(0,0,0);
	if(!C) return Accu;

	//множитель силы указывается строкой
	float Coef = FCString::Atof(*R.Value);
	if (Coef > 0) Coef = 1;

	//вектора задаются дочерними компонентами (для наглядности стрелками), откуда берется ось Х
	TArray<USceneComponent*> Children = GetAttachChildren();

	//если вектор только один, взвешенной суммы не требуется, просто вернуть его
	if(Children.Num()==1)
		return (FVector3f)Children[0]->GetComponentTransform().GetUnitAxis(EAxis::X)*Coef;

	//нормировочный коэффициент = максимальный размер всего объёма, чтобы расстояния до стрелок были весами меньше единицы
	float Normer = 0.5 / (Bounds.SphereRadius);
	float Denominator = 0;
	for(auto Ch : Children)
	{
		//весовой вклад вектора максимален при близости и ноль при дальности на другом конце поля
		float Weight = 1.0f - FVector::DistSquared(C->GetActorLocation(), Ch->GetComponentLocation()) * Normer * Normer;
		if(Weight < 0) Weight = 0;
		Accu += (FVector3f)Ch->GetComponentTransform().GetUnitAxis(EAxis::X) * Weight;
		Denominator += Weight;
	}
	if(Denominator > 1) Accu /= Denominator;
	return Accu*Coef;
}

//==============================================================================================================
//сформировать вектор тяги в пределы зоны
//==============================================================================================================
FVector3f UMyrTriggerComponent::ReactGravityPitMove(FTriggerReason& R, AMyrPhyCreature* C)
{
	//вектор от существа к центру, полноразмерный
	FVector3f Ra =  (FVector3f)(GetComponentLocation() - C->GetActorLocation());
	float RaDist2 = Ra.SizeSquared();

	//такого не случится, потому что пересёк с существом пропадёт, но вдруг...
	if (RaDist2 > FMath::Square(Bounds.SphereRadius)) return Ra;

	//существо в пределах сферы
	else
	{
		//в параметре указывается или мягкость перехода (отрицательное) или точный радуус начала (положительное)
		//в противных случаях радиус равен половине габаритов, а мягкость единице
		float RadiusToStart = Bounds.SphereRadius / 2;
		float Smoothness = -FCString::Atof(*R.Value);

		//меньше нуля значит исходно было больше нуля, значит это был введен радиус
		if (Smoothness < 0)
		{	
			//радиус должен быть меньше общего, иначе не будет области тяги
			if(-Smoothness < Bounds.SphereRadius)
				RadiusToStart = -Smoothness;

			//убрать плавность в дефолт
			Smoothness = 1.0f;
		}
		
		//если внутри области свободного движения, то вектор нулевой
		if (RaDist2 <= FMath::Square(RadiusToStart))
			return FVector3f::ZeroVector;

		//при выходе за область вектор начинает расти 
		else return Ra * Smoothness * (FMath::Sqrt(RaDist2) - RadiusToStart) / (Bounds.SphereRadius - RadiusToStart);
		
	}
}



//==============================================================================================================
//если в этом тригер объеме содержится цель прыжка, то выдает степень пригодности этой цели для прыжка, чтобы существо выбрало лучший из триггеров
//==============================================================================================================
float UMyrTriggerComponent::JumpGoalRating(AMyrPhyCreature* C, float Radius, float Coaxis)
{
	auto TR = HasReaction(EWhyTrigger::JumpTarget);
	if (TR)
	{
		auto Dir = FVector3f(GetComponentLocation() - C->GetActorLocation());
		float Dist = Dir.SizeSquared();
		if (Dist < Radius * Radius)
		{
			Dir *= FMath::InvSqrtEst(Dist);
			float CosAngle = Dir | C->AttackDirection;
			if (CosAngle >= Coaxis)
				return CosAngle * Dist;
		}
	}
	return 0.0f;
}

//==============================================================================================================
//прореагировать одну строку реакции - возвращает тру когда объект еще включается, для интерфейса написать
//==============================================================================================================
bool UMyrTriggerComponent::ReactSingle(FTriggerReason& Reaction, class AMyrPhyCreature* C, class AMyrArtefact* A, bool Release, bool* EndChain)
{
	UE_LOG(LogMyrTrigger, Verbose, TEXT("%s ReactSingle %s %d"), *GetName(), *TXTENUM(EWhyTrigger, Reaction.Why), Release);
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

		case EWhyTrigger::DestroyWhoComesIn:
			if(!Release) ReactDestroy(C, A, Reaction);
			break;

		case EWhyTrigger::DestroyWhoComesOut:
			if(Release) ReactDestroy(C, A, Reaction);
			break;

		case EWhyTrigger::DestroySpawnedAtComeOut:
			if(Release) ReactDestroySpawned(Reaction);
			break;

		case EWhyTrigger::Teleport:
		case EWhyTrigger::TeleportOnlyLocation:
		case EWhyTrigger::TeleportLocOrient:
			if(!Release) ReactTeleport(Reaction, C, A, false);
			break;

/*		case EWhyTrigger::KinematicLerpToCenter:
		case EWhyTrigger::KinematicLerpToCenterLocationOnly:
		case EWhyTrigger::KinematicLerpToCenterLocationOrientation:
			C->bKinematicRefine = !Release;
			if (C->bKinematicRefine)
			{	C->GetMesh()->SetPhyBodiesBumpable(false);
			}
			break;*/

		case EWhyTrigger::Quiet:
			ReactQuiet(C, Release);
			break;

		case EWhyTrigger::ChangeProtagonist:
			GetMyrGameMode()->ChangeProtagonist(Reaction, C);
			break;

		case EWhyTrigger::EnterLocation:
			if(!Release) ReactEnterLocation(C, A, !Release);
			break;

		//внимание, эта реакция просто делегируется сюда квестом, 
		//внутри триггера она не должна использоваться
		case EWhyTrigger::PlaceMarker:
			if (C->Daemon) C->Daemon->PlaceMarker(this);
			break;

		case EWhyTrigger::NotifyEat:
		case EWhyTrigger::NotifyClimb:
		case EWhyTrigger::NotifySleep:
			if (!Release) ReactNotify (Reaction, C);
			break;

		case EWhyTrigger::GenerateMyrLogicMsgAtIn:
			if(!Release) C->CatchMyrLogicEvent(EMyrLogicEvent::ObjAffectTrigger, 1.0f, this);
			break;

		case EWhyTrigger::GenerateMyrLogicMsgAtOut:
			if (Release) C->CatchMyrLogicEvent(EMyrLogicEvent::ObjAffectTrigger, 1.0f, this);
			break;

	}
	return true;
}

//==============================================================================================================
//выбор реакции в ответ на вход в объём или выход из объёма
//==============================================================================================================
void UMyrTriggerComponent::React(class AMyrPhyCreature* C, class AMyrArtefact* A, class AMyrDaemon* D, bool Release, EWhoCame Who)
{

	//не запускать, если не отработана выдержка до завершения или между концом и новым началом
	if(Release)
	{ if (!CheckTimeInterval(SecondsToHold, TEXT("SecondsToHold"))) return;	}
	else if(!CheckTimeInterval(SecondsToRest, TEXT("SecondsToRest"))) return;


	//включенность объекта проверяется внутри и влияет на выдачу уведомлений
	//важно, чтобы правильное значение сформировалось раньше, чем стоит реакция, для которой она важна
	bool ItemTurnedOn = false;

	//если тру, то этой функцией считать завершенным пересечение,
	bool EndChain = true;

	//чтобы покрыть ситуацию, когда пересеченный объект не подходит ни под одну реакцию, и его не надо добавлять
	bool Matters = false;

	//перебор реакций, важна правильная последовательность
	for(auto& Reaction : Reactions)
	{
		//только если задетектирован тот объект, который нужен
		if (Reaction.MayIt(Who))
		{	ItemTurnedOn = ReactSingle(Reaction, C, A, Release, &EndChain);
			Matters = true;
		}
	}

	//если существо, то у него есть встроенный стек охваченных объемов
	if (C && Matters)
	{
		//зарегистрировать, если на входе
		if (!Release) C->AddOverlap(this);

		//на выходе, проверить, не вышли ли заранее, если вышли, то ничего не делать
		else if (!C->HasOverlap(this))
			return;
	}

	//если по итогам исполнения больше от этого объёма ничего не нужно, удалить его из стека
	if (Release && EndChain)
	{
		UE_LOG(LogMyrTrigger, Log, TEXT("%s Early deleting overlapper %s"), *GetName(), *C->GetName());
		C->DelOverlap(this);
	}
}

AMyrraGameModeBase* UMyrTriggerComponent::GetMyrGameMode()
{	return GetWorld()->GetAuthGameMode<AMyrraGameModeBase>();
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
		UE_LOG(LogMyrTrigger, Log, TEXT("%s React by instant approval"), *GetOwner()->GetName());
		React(Sender, nullptr, Sender->Daemon, true, EWhoCame::Creature);
	}
}

//==============================================================================================================
//зонтик для входов и выходов из пересечения
//==============================================================================================================
void UMyrTriggerComponent::OverlapEvent(AActor* OtherActor, UPrimitiveComponent* OtherComp, bool ComingOut)
{
	//пересечение поясов не должно срабатывать, чтоб не получать множественные
	//if (OtherComp->IsA<UMyrGirdle>()) return;

	//только если пересекло существо или артефакт (артефакт, например, перенесся в зубах на место)
	EWhoCame WhoCame = EWhoCame::NONE;
	AMyrPhyCreature* C = nullptr;
	AMyrArtefact* A = nullptr;
	AMyrDaemon* D = nullptr;

	if (OtherActor->IsA<AMyrPhyCreature>())
	{	C = (AMyrPhyCreature*)OtherActor;
		D = C->Daemon;
		if (C->CurrentState != EBehaveState::project)
		{	if (C->IsUnderDaemon()) WhoCame = EWhoCame::Player;
			else WhoCame = EWhoCame::Creature;
		}
	}
	else if(OtherActor->IsA<AMyrDaemon>())
	{	WhoCame = EWhoCame::PlayerBubble;
		D = (AMyrDaemon*)OtherActor;
		C = D->OwnedCreature;
	}
	else if (OtherActor->IsA<AMyrArtefact>())
	{	WhoCame = EWhoCame::Artefact;
		A = (AMyrArtefact*)OtherActor;
	}

	//если пересечение произошло с одним из важных для игры акторов
	if (WhoCame != EWhoCame::NONE)
	{
		//вызывать начяальную стадию реакции
		if (!ComingOut) React(C, A, D, ComingOut, WhoCame);

		//досрочное завершение
		if(PerformOnlyByApprovalFromCreature != ComingOut)
			if (ComingOut || C->CouldSendApprovalToTrigger(this))
				React(C, A, D, true, WhoCame);

		//повторно вызывается если в реакте так и не вызвалось
		if (ComingOut) C->DelOverlap(this);
	}
	UE_LOG(LogMyrTrigger, Log, TEXT("Overlap%s %s.%s ------------ %s.%s %s"),
		ComingOut?TEXT("Out"):TEXT("In"), *GetOwner()->GetName(), *GetName(), *OtherActor->GetName(), *OtherComp->GetName(), *TXTENUM(EWhoCame, WhoCame));

}


//==============================================================================================================
//начало пересечения с внешним объектом 
//==============================================================================================================
UFUNCTION() void UMyrTriggerComponent::OverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogMyrTrigger, Verbose, TEXT("%s.%s: OverlapBegin %s.%s.%d"),
		*GetOwner()->GetName(), *GetName(), *OtherActor->GetName(), *OtherComp->GetName(), OtherBodyIndex);
	OverlapEvent(OtherActor, OtherComp, false);
}

//==============================================================================================================
//конец пересечения с внешним объектом
//==============================================================================================================
UFUNCTION() void UMyrTriggerComponent::OverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_LOG(LogMyrTrigger, Verbose, TEXT("%s.%s: OverlapEnd %s"),
		*GetOwner()->GetName(), *GetName(), *OtherActor->GetName());
	OverlapEvent(OtherActor, OtherComp, true);
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
