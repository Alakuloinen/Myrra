// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "GameFramework/Pawn.h"
#include "MyrPhyCreatureMesh.h"
#include "MyrGirdle.h"
#include "AssetStructures/MyrCreatureGenePool.h"
#include "AssetStructures/MyrActionInfo.h"
#include "AssetStructures/MyrCreatureBehaveStateInfo.h"	// данные текущего состояния моторики
#include "MyrPhyCreature.generated.h"

//свои дебаг записи
DECLARE_LOG_CATEGORY_EXTERN(LogMyrPhyCreature, Log, All);

//###################################################################################################################
//###################################################################################################################
UCLASS() class MYRRA_API AMyrPhyCreature : public APawn
{
	GENERATED_BODY()

	//физическая модель тела на колёсиках
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	class UMyrPhyCreatureMesh* Mesh;

	//возвращение к корням, отбойник для всего тела для кинематического таскания по ветвям
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	class UMyrGirdle* Thorax;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	class UMyrGirdle* Pelvis;

	//индикатор здоровья и прочей фигни над головой (возможно, нафиг, пусть будет в худе, в уголке по наведению глаза)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	class UWidgetComponent* LocalStatsWidget;

	//аудио-компонент исключительно для голосовых нужд, в том числе и речи, отдельно от аудиокомпонентов для шагов и ушибов
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	class UAudioComponent* Voice;

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bMove : 1;		//ваще как либо хотеть двигаться
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bRun : 1;			//бежать
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bCrouch : 1;		//красться
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bClimb : 1;		//лазать по деревьям
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bWannaClimb : 1;	//момент запроса на карабканье
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bFly : 1;			//летать
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bSoar : 1;		//активно набирать высоту
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bKinematicMove : 1;//кинематически перемещать

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bMoveBack : 1;	//пятиться назад, сохраняя поворот вперед


	//указатель на класс инкарнации игрока
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) class AMyrDaemon* Daemon = nullptr;

	//текущая цель прыжка (из оверлапа или из ИИ цели)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	USceneComponent* JumpTarget = nullptr;


	//имя, которое будет отображаться человеческим языком - может быть одинаково у нескольких
	//при спавне генерируется генофондом по алгоритму у каждого своему
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText HumanReadableName;

	//система атак/действий - в каждый момент всего одно действие, зато можно иметь много разновидностей
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 CurrentAttack = 255;		// действие на цель
	UPROPERTY(EditAnywhere, BlueprintReadonly) EActionPhase CurrentAttackPhase = EActionPhase::ASCEND;
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 CurrentAttackRepeat = 0;
	UPROPERTY(EditAnywhere, BlueprintReadonly) float AttackForceAccum = 0.0f;		//аккумуоятор пока что только силы прыжка со временем

	//самодействия - вызываются сами время от времени по ситуации
	UPROPERTY(VisibleAnywhere, BlueprintReadonly) uint8 CurrentSelfAction = 255;	// реально выполняется
	UPROPERTY(VisibleAnywhere, BlueprintReadonly) uint8 SelfActionToTryNow = 0;		// рассмотреть, если подходит, то выполнить (счетчик)
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 SelfActionPhase = 0;			// у самодействия может быть несколько фаз (по закладкам в ролике) с разной физикой

	//самодействия отдыха, отделены поскольку вызываются отдельно по кнопке и имеют строго 3 фазы: вход, пребывание, выход
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 CurrentRelaxAction = 255;
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 RelaxActionPhase = 0;			

	//канал, на котором "виден" запах существ этого класса
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)  uint8 SmellChannel = 0;

	//ныне произносимый звук - нужно для анимации формы рта
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EPhoneme CurrentSpelledSound = EPhoneme::S0;

	//весь целиковый кусок всех статических данных для данного класса
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UMyrCreatureGenePool* GenePool;

	//единичные вектора направления действий
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector3f MoveDirection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector3f AttackDirection;

	//скаляры различных оперативных параметров
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ExternalGain = 0.0f;			// коэффициент скорости, для разгона извне
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MoveGain = 1.0f;				// штраф к скорости из-за усталости, увечий
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Health = 1.0f;					// здоровье
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Stamina = 1.0f;				// запас сил
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Energy = 1.0f;					// глобальный запас сил, восстанавливается едой
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Pain = 0.0f;					// боль - резкое ухудшение характеристик при просадке здоровья
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float AttackStrength = 0.0f;			// сила атаки, пока используется как дальность прыжка в прыжковой атаке, надо повесить сюда задержку при нажатии кнопки
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Age = 0.0f;					// возраст, просто возраст в секундах, чтобы состаривать и убивать неписей (мож double или int64?)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float LightingAtView = 0.0f;			// уровень освещенности в направлении взгляда

	UPROPERTY(EditAnywhere, BlueprintReadOnly) float BestBumpFactor = 0.0f;			// для теста

	// набор данных воздействия последней съеденной пищи			
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FDigestiveEffects DigestiveEffects;	

	//кэш расстояния между центрами поясов, для позиционирования ведомого пояса
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float SpineLength = 0.0f;

	//единичный вектор от заднего пояса к переднему
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector3f SpineVector;

	// окрас, номер текстуры из списка в генофонде		
	UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 Coat = 0;		

	// телосложение - номер локальной 1-кадровогой анимации из генофонда	
	UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 Physique = 0;

#if WITH_EDITORONLY_DATA

	//набор битов какие линии отладки рисовать
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = ELimbDebug)) int32 DebugLinesToShow = 0;
	bool IsDebugged(ELimbDebug Ch) { return ((DebugLinesToShow & (1 << (uint32)Ch)) != 0);  }
	void Line(ELimbDebug Ch, FVector A, FVector AB, float W = 1, float Time = 0.04);
	void Line(ELimbDebug Ch, FVector A, FVector AB, FColor Color, float W = 1, float Time = 0.04);
	void Linet(ELimbDebug Ch, FVector A, FVector AB, float Tint, float W = 1, float Time = 0.04);
#endif

	//счетчик тактов
	uint16 FrameCounter = 0;

	//конкретные, самоLOостаточные параметры индивида, прокачиваются, 																			
	//UPROPERTY(EditAnywhere, BlueprintReadWrite) FRoleParams Params;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FPhenotype Phenotype;
	
	// автомат состояний поведения
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EBehaveState CurrentState;			// текущее состояние
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float StateTime = 0.0f;				// время в секундах с момента установки состояния

	//указатель на данные текущего режима поведения, кэш для быстроты, при смене режима перекэшируется
	//отсюда берется базис для ориентира скорости при шаге, беге и т.п.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UMyrCreatureBehaveStateInfo* BehaveCurrentData;

	//текущий пересеченный триггер объём, может быть нуль (пока неясно, заводить ли полный стек)
	//объёмы сами 
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UMyrTriggerComponent* Overlap0 = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UMyrTriggerComponent* Overlap1 = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UMyrTriggerComponent* Overlap2 = nullptr;

public:
	//здесь создаются компоненты и значения по умолчанию
	AMyrPhyCreature();

	//перет инициализацией компонентов, чтобы компоненты не охренели от родительской пустоты во время своей инициализации
	virtual void PreInitializeComponents() override;

	//каждый кадр
	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	
	//для распространения свойств при сменен генофонда
	virtual void PostEditChangeProperty (FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	//появление в игре - финальные донастройки, когда всё остальное готово
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type R) override;

	//события в результате пригрывания звука (центрального звука, например голоса - LipSync и т.п.)
	UFUNCTION()	void OnAudioPlaybackPercent(
		const class USoundWave* PlayingSoundWave,
		const float PlaybackPercent);
	UFUNCTION()	void OnAudioFinished();

//свои процедуры
public:	

	//сгенерировать имя для высранного артефакта этого вида
	//реализуется прямо в блупринтах, так как у каждого всё по разному
	UFUNCTION(BlueprintImplementableEvent) FText GenerateHumanReadableName();
	
	//при рантайм-высере правильно инициализировать
	void RegisterAsSpawned(AActor* Spawner);

	//переварить внешнюю сборку командмоторики
	void ConsumeDrive(FCreatureDrive *D);

	//взять команды и данные из контроллеров игрока и ИИ в нужной пропорции
	void ConsumeInputFromControllers(float DeltaTime);

	//оценить применимость реакции на уткнутость и если удачно запустить ее
	EResult TestBumpReaction(FBumpReaction* BR, ELimb BumpLimb);

	//обновить осознанное замедление скорости по отношению к норме для данного двигательного поведения
	void UpdateMobility(bool IncludeExternalGain = true);

	//быстро уменьшать боль с течением времени
	void RelievePain(float DeltaTime) { Pain *= (0.9 - 0.1 * (Stamina + Health)); }

	//акт привязывания контроллера игрока
	void MakePossessedByDaemon(AActor* aDaemon);

	//применить пришедшую извне модель сил для всего тела
	void AdoptWholeBodyDynamicsModel(FWholeBodyDynamicsModel* DynModel);

	//кинематически телепортировать в новое место
	void TeleportToPlace(FTransform Dst);
	void KinematicMove(FTransform Dst);
	void SetKinematic(bool Set);

	//включить или выключить желание при подходящей поверхности зацепиться за нее
	void ClimbTryActions();

	//найти объект в фокусе - для демона, просто ретранслятор из ИИ, чтобы демон не видел ИИ
	int FindObjectInFocus(const float Devia2D, const float Devia3D, AActor*& Result, FText& ResultName);

	//прирастить запас сил (отдохнуть) если аргумент>0, или отнять (устать) - вызывается из редкого тика
	void StaminaChange(float Dp);
	void HealthChange(float Dp);

	//после перемещения на ветку - здесь выравнивается камера
	void PostTeleport();

	//редкий тик, вызывается пару раз в секунду
	void RareTick(float DeltaTime);

	//сделать шаг - сгенерировать след, облако пыли, звук
	void MakeStep(ELimb WhatFoot, bool Raise);

	//пострадать от физических повреждений (всплывает из меша)
	void Hurt(FLimb& Limb, float Amount, FVector ExactHitLoc, FVector3f Normal, EMyrSurface ExactSurface);
	void Hurt(FLimb& Limb, float Amount, FVector ExactHitLoc)
	{	Hurt(Limb, Amount, ExactHitLoc, Limb.Floor.Normal, Limb.Floor.Surface);	}

	//ментально осознать, что пострадали от действий другого существа или наоборот были им обласканы
	void SufferFromEnemy(float Amount, AMyrPhyCreature* Motherfucker);
	void EnjoyAGrace(float Amount, AMyrPhyCreature* Sweetheart);
	void MeHavingHit(float Amount, AMyrPhyCreature* Victim);

	//запустить брызг частиц пыли и т.п. из чего состоит заданная поверхность
	void SurfaceBurst(UParticleSystem* Dust, FVector ExactHitLoc, FQuat ExactHitRot, float BodySize, float Scale);
	void SurfaceBurst(class UNiagaraSystem* Dust, FVector ExactHitLoc, FQuat4f ExactHitRot, float BodySize, float Scale);

	//проиграть звук контакта с поверхностью / шаг, удар
	void SoundOfImpact(FSurfaceInfluence *SurfInfo, EMyrSurface Surface, FVector Loc, float Strength, float Vertical, EBodyImpact ImpactType);

	//проиграть новый звук (подразумевается голос и прочие центральные звуки)
	void MakeVoice(USoundBase* Sound, uint8 strength, bool interrupt);

	//////////////////////////////////////////////////////////////////////

	//принять новое состояние поведения (логически безусловно, но технически может что-то не сработать, посему буль)
	bool AdoptBehaveState(EBehaveState NewState, const FString Reason = "");

	//основная логика движения - свитч с разбором режимов поведения и переходами между режимами
	void ProcessBehaveState(float DeltaTime);

	/////////////////////////////////////////////
	bool GotUnclung()					{ return !Thorax->Climbing && !Pelvis->Climbing; }
	bool GotGroundBoth()				{ return Mesh->Thorax.Stepped && Mesh->Pelvis.Stepped; }
	bool GotStandBoth(int Thr = 300)	{ return (Thorax->StandHardness + Pelvis->StandHardness > Thr); }
	bool GotLandedAny(uint8 Thr = 100)  { return (Thorax->Stands(Thr) || Pelvis->Stands(Thr)); }
	bool GotLandedBoth(uint8 Thr = 100) { return (Thorax->Stands(Thr) && Pelvis->Stands(Thr)); }
	bool GotSlow(float T = 1)			{ return (Pelvis->VelocityAgainstFloor.SizeSquared() < T && Thorax->VelocityAgainstFloor.SizeSquared() < T); }
	bool GotSoaringDown(float T = -50)	{ return (Mesh->GetPhysicsLinearVelocity().Z < T); }
	bool GotLying()						{ return (Thorax->Lies() && Pelvis->Lies() && SpineVector.Z < 0.5f); }
	bool GotReachTarget()				{ return JumpTarget && FVector::Distance(Mesh->GetComponentLocation(), JumpTarget->GetComponentLocation()) < 10; }

	//висим низом
	bool HangBack()						{ return (Thorax->StandHardness >= 200 && Pelvis->StandHardness <= 35); }
	//bool GotHungFront()						{ return Thorax->Stands() && !Pelvis->Stands() && SpineVector.Z }

	//////////////////////////////////////////////////////////////////////
	//действия - начать, ударить, прекратить досрочно
	
	EResult AttackActionStart(int SlotNum, USceneComponent* ExactVictim = nullptr);
	EResult AttackActionStrike();
	EResult AttackActionStrikePerform();
	void AttackChangePhase(EActionPhase NewPhase);
	void AttackNotifyGetReady();
	void AttackEnd();					//завершение 
	void AttackGoBack();					//прерывание и разворот в спять - редко, чтобы не пересекать вглубь
	bool AttackNotifyGiveUp();			//прерывание неначатой
	void AttackNotifyEnd();				//поимка закладки		
	void AttackNotifyFinish();			//поимка закладки финиш - переход в финальную фазу перед завершением

	//процедуры прыжка из различных состояний повдения
	bool JumpAsAttack(FVector3f EplicitVel);

	//просканировать объёмы вокруг и выбрать цель
	FVector PrepareJumpVelocity(FVector3f& OutVelo);

	//начать, выбрать и прервать самодействие
	EResult SelfActionStart(int SlotNum);
	void SelfActionCease();
	void SelfActionNewPhase();

	//найти, выбрать и начать анимацию отдыха
	EResult RelaxActionStart(int Slot);//стартануть релакс уже известный по индексу
	void RelaxActionReachPeace();	// * достичь фазы стабильности (вызывается из закладки анимации MyrAnimNotify)
	void RelaxActionStartGetOut();	//начать выход из фазы стабильности
	void RelaxActionReachEnd();		// * достичь конца и полного выхода (вызывается из закладки анимации MyrAnimNotify)

	//прервать или запустить прерывание деймтвий, для которых сказано прерывать при сильном касании
	void CeaseActionsOnHit(float Damage);		
	void ActionFindList(bool RelaxNotSelf, TArray<uint8>& OutActionIndices, EAction Restrict = EAction::NONE);

	//найти наиболее подходящее по контексту действие (в основном для ИИ)
	uint8 ActionFindBest(USceneComponent *VictimBody, float Damage = 0, float Dist = 1e6, float Coaxis = 0, bool ByAI = false, bool Chance = false);

	//вообще запустить какое-то действие по точному идентификатору (если есть в генофонде)
	EResult  ActionFindStart   (EAction Type);
	EResult  ActionFindRelease (EAction Type, UPrimitiveComponent* ExactGoal = nullptr);
	EResult	 ActionStart(uint8 Number);

	//отпустить всё, что имелось активно схваченным (в зубах)
	void UnGrab();	

	//показать на виджете то, что мы взяли и положили что-то
	void WidgetOnGrab(AActor* Victim);
	void WidgetOnUnGrab(AActor* Victim);

	////////////////////////////прием пищи////////////////////////////////

	//поиск еды среди членов тела (схваченная добыча)
	UPrimitiveComponent* FindWhatToEat(FDigestiveEffects*& D, FLimb*& LimbWith);	

	//логическое поедание конкретного найденного объекта (живого или не живого)
	bool EatConsume(UPrimitiveComponent* Food, FDigestiveEffects* D, float Part = 1);

	//поедиание жтвого объекта в ходе атаки ртом или захвата
	bool Eat();

	//////////////////////реакция на внешний триггер//////////////////////

	//обнаружить, что мы пересекаем умный объём и передать ему, что мы совершаем, возможно, судьбоносное действие
	void SendApprovalToOverlapper();

	//уведомить триггер при пересечении, что его функция может быть сразу задействована,
	//потому что условие активного включение было выполнено до пересечения
	bool CouldSendApprovalToTrigger(class UMyrTriggerComponent* Sender);

	//показать на виджете худа то, что пересечённый григгер-волюм имел нам сообщить
	void WigdetOnTriggerNotify(EWhyTrigger ExactReaction, AActor* What, USceneComponent* WhatIn, bool On);

	//////////////////////////////////////////////////////////////////////

	//взять из генофонда текстуру, соответствующую номеру и перекрасить материал тушки
	void SetCoatTexture(int Num);

	//охватить это существо при загрузке и сохранении игры
	UFUNCTION(BlueprintCallable) void Save(FCreatureSaveData& Dst);
	UFUNCTION(BlueprintCallable) void Load(const FCreatureSaveData& Src);

	//обыскать слот сохранения и проставить во всех ссылках на это существо (в памятях других существ) реальный указатель
	UFUNCTION(BlueprintCallable) void ValidateSavedRefsToMe();

	//отражение действия на более абстрактонм уровне - прокачка роли, эмоция, сюжет
	//возможно, вместо AActor следует ввести UObject, чтобы адресовать и компоненты, и всё прочее
	void CatchMyrLogicEvent(EMyrLogicEvent Event, float Param, UObject* Patient, FMyrLogicEventData* ExplicitEmo = nullptr);

	//передать информацию в анимацию из ИИ (чтобы не светить ИИ в классе анимации)
	//старое, возможно, переделать
	void TransferIntegralEmotion(float& Rage, float& Fear, float& Power, float& Amount);

	//зарегистрировать пересекаемый объём с функционалом
	void AddOverlap(class UMyrTriggerComponent* Ov);
	bool DelOverlap(class UMyrTriggerComponent* Ov);

	//подправить курс по триггер-объёму
	bool ModifyMoveDirByOverlap(FVector3f& INMoveDir, EWhoCame WhoAmI);

	//вывод в блюпринт для редкого тика, чтобы добавлять действия специфичные для конкретного существа
	UFUNCTION(BlueprintImplementableEvent)	void RareTickInBlueprint(float DeltaTime);

	//упаковать значение посчитанной скорости прыжка в неиспользуемое поле
	void PackJumpStartVelocity(FVector3f Ve)	{ ((USceneComponent*)Voice)->ComponentVelocity = FVector(Ve); }

	//в момент исчезновения объекта сделать так, чтобы в памяти ИИ ничего не сломалось
	void HaveThisDisappeared(USceneComponent* It);


//свои возвращуны
public:	

	FVector& GetJumpStartVelocity() const { return ((USceneComponent*)Voice)->ComponentVelocity; }
	FVector3f GetJumpStartVelocityF() const { return FVector3f(const_cast<AMyrPhyCreature*>(this)->GetJumpStartVelocity()); }
	FVector GetJumpTrajAtNow(FVector& Start) const		{ return Start + GetJumpStartVelocity() * StateTime + FVector(0, 0, -981) * StateTime * StateTime / 2; }
	FVector3f GetJumpVelAtNow() const					{ return GetJumpStartVelocityF() + FVector3f(0, 0, -981) * StateTime; }

	//проверить извне, пересекает ли существо выбранный триггер
	bool HasOverlap(class UMyrTriggerComponent* Ov) const { return (Overlap0 == Ov || Overlap1 == Ov || Overlap2 == Ov); }

	//найти объём, в котором есть интересующая реакция, если нет выдать нуль
	UMyrTriggerComponent* HasSuchOverlap(EWhyTrigger r, FTriggerReason*& TR);
	UMyrTriggerComponent* HasSuchOverlap(EWhyTrigger r, FTriggerReason*& TR, EWhoCame WhoAmI);
	UMyrTriggerComponent* HasSuchOverlap(EWhyTrigger rmin, EWhyTrigger rmax, FTriggerReason*& TR);
	UMyrTriggerComponent* HasGoalOverlap(FTriggerReason*& TR);

	class AMyrLocation* IsInLocation() const;

	//объём по индексу
	UMyrTriggerComponent*& OverlapByIndex(int i = 0) { return (&Overlap0)[i]; }

	//найти среди объёмов самую лучшую цель
	int FindMoveTarget(float Radius, float Coaxis);

	//доступ к глобальным вещам
	UFUNCTION(BlueprintCallable) class UMyrraGameInstance* GetMyrGameInst() const { return (UMyrraGameInstance*)GetGameInstance(); }
	UFUNCTION(BlueprintCallable) class AMyrraGameModeBase* GetMyrGameMode() const { return (AMyrraGameModeBase*)GetWorld()->GetAuthGameMode(); }

	//выдать подставку по параметру
	UMyrPhyCreatureMesh* GetMesh() { return Mesh; }

	//выдать пояс
	UMyrGirdle* GetThorax() { return Thorax; }
	UMyrGirdle* GetPelvis() { return Pelvis; }

	//управляется игроком
	bool IsUnderDaemon() { return Daemon != nullptr; }

	//включён вид от первого лица (вызывается класс демона)
	bool IsFirstPerson();

	//может ли существо летать
	bool CanFly() const { return Phenotype.Agility.BareLevel() >= GenePool->MinLevelToFly;  }

	//выдать пояс, к которому принадлежит эта часть тела, по идентификатору чати тела
	UMyrGirdle* GetGirdle(ELimb L) { return LimbSection(L) ? Thorax : Pelvis; }

	//выдать противоположный пояс
	UMyrGirdle* GetAntiGirdle(UMyrGirdle* G) { return G == Thorax ? Pelvis : Thorax;  }

	//контроллер искусственного интеллекта
	//UFUNCTION(BlueprintCallable) class AMyrAI *MyrAI() const { return (AMyrAI*)Controller; }
	UFUNCTION(BlueprintCallable) class AMyrAIController *MyrAIController() const { return (AMyrAIController*)Controller; }

	//выдать указатель на генофонд вовне
	UFUNCTION(BlueprintCallable) UMyrCreatureGenePool* GetGenePool() const { return GenePool; }

	//текущее состояние, безонтносительно от того, если какой-то переход
	EBehaveState GetBehaveCurrentState() const { return CurrentState; }

	//выдать вовне данные, связанные с текущим состоянием моторики
	UFUNCTION(BlueprintCallable) UMyrCreatureBehaveStateInfo* GetBehaveCurrentData() const { return BehaveCurrentData; }

	//************************************************************
	//новый вариант с единым массивом
	UFUNCTION(BlueprintCallable) UMyrActionInfo* GetAttackAction() const		{ return GenePool->Actions[(int)CurrentAttack]; }
	UFUNCTION(BlueprintCallable) FVictimType&    GetAttackActionVictim() const	{ return GenePool->Actions[(int)CurrentAttack]->VictimTypes[0]; }
	UFUNCTION(BlueprintCallable) UMyrActionInfo* GetSelfAction() const			{ return GenePool->Actions[(int)CurrentSelfAction]; }
	UFUNCTION(BlueprintCallable) UMyrActionInfo* GetRelaxAction() const			{ return GenePool->Actions[(int)CurrentRelaxAction]; }
	UFUNCTION(BlueprintCallable) UMyrActionInfo* GetAction(int i) const			{ return GenePool->Actions[i]; }
	//************************************************************

//учёт и поиск "корней" динамических моделей тела
public:

	FWholeBodyDynamicsModel* GetAttackDynModel()	{ return DoesAttackAction() ? &GetAttackAction()->DynModelsPerPhase[(int)CurrentAttackPhase] : nullptr; }
	FWholeBodyDynamicsModel* GetSelfDynModel()		{ return DoesSelfAction() ? &GetSelfAction()->DynModelsPerPhase[SelfActionPhase] : nullptr; }
	FWholeBodyDynamicsModel* GetRelaxDynModel()		{ return DoesRelaxAction() ? &GetRelaxAction()->DynModelsPerPhase[RelaxActionPhase] : nullptr; }
	FGirdleDynModels* GetPriorityModel(bool Tho)
	{	auto R = GetSelfDynModel();
		if(!R) R = GetAttackDynModel();							else if (!R->Girdle(Tho).Use) R = nullptr;
		if(!R) R = GetRelaxDynModel();							else if (!R->Girdle(Tho).Use) R = nullptr;
		if(!R) R = &BehaveCurrentData->WholeBodyDynamicsModel;	else if (!R->Girdle(Tho).Use) R = nullptr;
		if(!R) R = &FWholeBodyDynamicsModel::Default();			return &R->Girdle(Tho);
	}
	FWholeBodyDynamicsModel* GetPriorityModel()
	{	auto R = GetSelfDynModel();
		if (!R) R = GetAttackDynModel();						else if (!R->Use) R = nullptr;
		if (!R) R = GetRelaxDynModel();							else if (!R->Use) R = nullptr;
		if (!R) R = &BehaveCurrentData->WholeBodyDynamicsModel;	else if (!R->Use) R = nullptr;
		if (!R) R = &FWholeBodyDynamicsModel::Default();		return R;
	}

	FWholeBodyDynamicsModel* FindJumpDynModel()
	{	FWholeBodyDynamicsModel* D = nullptr;
		auto R = GetSelfDynModel();		if (R) if (R == Mesh->DynModel) if ((D = GetSelfAction()->FindJumpDynModel(SelfActionPhase))!=0)  return D;
		R = GetAttackDynModel();		if (R) if (R == Mesh->DynModel) if ((D = GetAttackAction()->FindJumpDynModel((int)CurrentAttackPhase)) != 0) return D;
		R = GetRelaxDynModel();			if (R) if (R == Mesh->DynModel) if ((D = GetRelaxAction()->FindJumpDynModel(RelaxActionPhase)) != 0) return D;
		return nullptr;
	}
	//************************************************************

	//уровень метаболизма - визуальная скорость дыхания, сердебиения, эффектов в камере
	UFUNCTION(BlueprintCallable) float GetMetabolism() const;

	bool MountTime() { return StateTime < 0.3 + 1.0f*Stamina; }
	bool Mounts() { return (CurrentState == EBehaveState::mount && MountTime()); }

	//участвует ли эта часть тела в непосредственно атаке
	bool IsLimbAttacking(ELimb eLimb);

	//касаемся ли мы/стоим ли мы на этом объекте
	bool IsTouchingThisActor(AActor* A);
	bool IsTouchingThisComponent(UPrimitiveComponent* A);
	bool IsStandingOnThisActor(AActor* A);
	USceneComponent* GetObjectIStandOn();

	//проверка на фазу атаки - для краткости
	bool NowPhaseStrike() const { return (CurrentAttackPhase == EActionPhase::STRIKE); }
	bool NowPhaseDescend() const { return (CurrentAttackPhase == EActionPhase::DESCEND); }

	//текущая фаза атаки соответствует указанным в сборке этой атаки для особых действий: подготовиться к прыжку, прыгнуть, схватить при касании
	bool NowPhaseToJump() { return (Mesh->DynModel->JumpImpulse > 1); }
	bool NowPhaseToHold() { return Mesh->DynModel->PreJump; }

	//сейчас фаза чтобы схватить - тут сложнее, надо ещё проверить, чтобы не совпадало с фазой отпускания, иначе нет смысла
	//вызывается в MyrPhyCreatureMesh.cpp
	bool NowPhaseToGrab()
	{ 	auto P = GetAttackActionVictim().GrabAtTouchPhase;
			if (P == CurrentAttackPhase)
				if (GetAttackActionVictim().UngrabPhase != CurrentAttackPhase) return true;
		return false;
	}

	//проверка на совершение действий
	bool NoAttack() const { return (CurrentAttack == 255); }
	bool Attacking() const { return (CurrentAttack != 255); }
	bool DoesAttackAction() const { return (CurrentAttack != 255); }

	bool NoSelfAction() const { return (CurrentSelfAction == 255); }
	bool DoesSelfAction() const { return CurrentSelfAction<255; }

	bool NoRelaxAction() const { return (CurrentRelaxAction == 255); }
	bool DoesRelaxAction() const { return (CurrentRelaxAction < 255); }

	bool NoAnyAction() const { return (NoAttack() && NoSelfAction() && NoRelaxAction()); }

	bool PreStrike() const { return (int)CurrentAttackPhase <= (int)EActionPhase::READY; }

	//делает какое-то из трех видов действий
	bool DoesAnyAction() const { return (CurrentAttack < 255) || (CurrentSelfAction<255) || (CurrentRelaxAction<255); }

	float GetDamage(ELimb L) const { return Mesh->GetLimb(L).Damage; }

	//потрачен
	bool Dead() { return (CurrentState == EBehaveState::dead);  }

	//оси ориентации существа (ориентируемся по мешу, положение опоры может сильно отличаться)
	FVector3f GetAxisForth() const { return (FVector3f)((UPrimitiveComponent*)Mesh)->GetComponentTransform().GetScaledAxis(EAxis::X); }
	FVector3f GetAxisLateral() const { return (FVector3f)((UPrimitiveComponent*)Mesh)->GetComponentTransform().GetScaledAxis(EAxis::Y); }

	//позиция головы в мировых координатах (берется из скелета меша)
	FVector GetHeadLocation();

	//вектор между голову в сторону чужой головы
	FVector3f HeadToYourHead(AMyrPhyCreature* You) { return (FVector3f)(GetHeadLocation() - You->GetHeadLocation()); }

	//выдать координаты центра масс конкретной части тела
	FVector GetBodyPartLocation(ELimb Limb);

	//конечная точка для трассировки с целью проверки видимости
	FVector GetVisEndPoint();

	//найти локацию той части тела, которая ближе всего к интересующей точке
	FVector GetClosestBodyLocation(FVector Origin, FVector Earlier, float EarlierD2);

	//вектор, в который тело смотрит глазами - для определения "за затылком"
	FVector3f GetLookingVector() const;

	//вектор тела вверх
	FVector3f GetUpVector() const;

	//получить множитель незаметности, создаваемый поверхностью, на которой мы стоим
	float GetCurrentSurfaceStealthFactor() const;

	//выдать, если есть, цель, на которую можно кинематически наброситься. может выдать нуль
	UFUNCTION(BlueprintCallable) USceneComponent* GetRushGoal();

	//рассеяние звука текущее - от уровня и локации
	class USoundAttenuation* GetCurSoundAttenuation();

	//целевая скорость, с которой мы хотим двигаться
	UFUNCTION(BlueprintCallable) float GetDesiredVelocity() const { return BehaveCurrentData->MaxVelocity * MoveGain; }

	//длина спины 
	UFUNCTION(BlueprintCallable) float GetBodyLength() const { return Mesh->Bounds.BoxExtent.X*2; }

	//виджет над головой - для карткости
	class UMyrBioStatUserWidget* StatsWidget();

	//для отладки, чтобы показывать в худе
	UFUNCTION(BlueprintCallable) FString GetCurrentAttackName()			const { return !NoAttack() && GetAttackAction()			? GetAttackAction()->GetName()			: FString(TEXT("NO")); }
	UFUNCTION(BlueprintCallable) FString GetCurrentSelfActionName()		const { return (!NoSelfAction() && GetSelfAction())		? GetSelfAction()->GetName()			: FString(TEXT("NO")); }
	UFUNCTION(BlueprintCallable) FString GetCurrentRelaxActionName()	const { return (!NoRelaxAction() && GetRelaxAction())	? GetRelaxAction()->GetName()			: FString(TEXT("NO")); }
	UFUNCTION(BlueprintCallable) FText GetCurrentAttackHumanName()		const { return !NoAttack() && GetAttackAction()			? GetAttackAction()->HumanReadableName	: FText(); }
	UFUNCTION(BlueprintCallable) FText GetCurrentSelfActionHumanName()	const { return (!NoSelfAction() && GetSelfAction())		? GetSelfAction()->HumanReadableName	: FText(); }
	UFUNCTION(BlueprintCallable) FText GetCurrentRelaxActionHumanName() const { return (!NoRelaxAction() && GetRelaxAction())	? GetRelaxAction()->HumanReadableName	: FText(); }

};
