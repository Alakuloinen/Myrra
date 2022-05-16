// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "GameFramework/Pawn.h"
#include "MyrPhyCreatureMesh.h"
#include "MyrGirdle.h"
#include "AssetStructures/MyrCreatureGenePool.h"
#include "AssetStructures/MyrCreatureAttackInfo.h"
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bRun : 1;			//бежать
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bCrouch : 1;		//красться
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bClimb : 1;		//лазать по деревьям
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bFly : 1;			//летать
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bSoar : 1;		//активно набирать высоту
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bKinematic : 1;	//кинематически перемещать/доводить

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bMoveBack : 1;	//пятиться назад, сохраняя поворот вперед

	//указатель на класс инкарнации игрока
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) class AMyrDaemon* Daemon;

	//имя, которое будет отображаться человеческим языком - может быть одинаково у нескольких
	//при спавне генерируется генофондом по алгоритму у каждого своему
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText HumanReadableName;

	//система атак/действий - в каждый момент всего одно действие, зато можно иметь много разновидностей
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 CurrentAttack = 255;		// действие на цель
	UPROPERTY(EditAnywhere, BlueprintReadonly) EActionPhase CurrentAttackPhase = EActionPhase::ASCEND;
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 CurrentAttackRepeat = 0;
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 CurrentAttackVictimType = 0;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector MoveDirection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector AttackDirection;

	//скаляры различных оперативных параметров
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ExternalGain = 0.0f;			// коэффициент скорости, для разгона извне
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MoveGain = 1.0f;				// штраф к скорости из-за усталости, увечий
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Health = 1.0f;					// здоровье
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Stamina = 1.0f;				// запас сил
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Energy = 1.0f;					// глобальный запас сил, восстанавливается едой
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Pain = 0.0f;					// боль - резкое ухудшение характеристик при просадке здоровья
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float AttackStrength = 0.0f;			// сила атаки, пока используется как дальность прыжка в прыжковой атаке, надо повесить сюда задержку при нажатии кнопки
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Age = 0.0f;					// возраст, просто возраст в секундах, чтобы состаривать и убивать неписей (мож double или int64?)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Stuck = 0.0f;					// степень застревания (плюс = уступ, можно залезть, минус = препятствие, умерить пыл или отойти)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float LightingAtView = 0.0f;			// уровень освещенности в направлении взгляда

	// набор данных воздействия последней съеденной пищи			
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FDigestiveEffects DigestiveEffects;			

	//кэш расстояния между центрами поясов, для позиционирования ведомого пояса
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float SpineLength = 0.0f;			

	// окрас - только для редактора, чтобы тестировать разные окраски на одном существе		
	UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 Coat = 0;		

#if WITH_EDITORONLY_DATA


	//набор битов какие линии отладки рисовать
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = ELimbDebug)) int32 DebugLinesToShow = 0;
	bool IsDebugged(ELimbDebug Ch) { return ((DebugLinesToShow & (1 << (uint32)Ch)) != 0);  }
	void Line(ELimbDebug Ch, FVector A, FVector AB, float W = 1, float Time = 0.02);
	void Line(ELimbDebug Ch, FVector A, FVector AB, FColor Color, float W = 1, float Time = 0.02);
#endif

	//счетчик тактов
	uint16 FrameCounter = 0;

	//конкретные, самодостаточные параметры индивида, прокачиваются, 																			
	//UPROPERTY(EditAnywhere, BlueprintReadWrite) FRoleParams Params;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FPhenotype Phenotype;
	
	// автомат состояний поведения
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EBehaveState CurrentState;			// текущее состояние
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float StateTime = 0.0f;				// время в секундах с момента установки состояния

	//указатель на данные текущего режима поведения, кэш для быстроты, при смене режима перекэшируется
	//отсюда берется базис для ориентира скорости при шаге, беге и т.п.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UMyrCreatureBehaveStateInfo* BehaveCurrentData;

	//текущий пересеченный триггер объём, может быть нуль (пока неясно, заводить ли полный стек
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
	EAttackAttemptResult TestBumpReaction(FBumpReaction* BR, ELimb BumpLimb);

	//вычислить степень уткнутости в препятствие и тут же произвести корректировку
	void WhatIfWeBumpedIn();

	//обновить осознанное замедление скорости по отношению к норме для данного двигательного поведения
	void UpdateMobility(bool IncludeExternalGain = true);

	//быстро уменьшать боль с течением времени
	void RelievePain(float DeltaTime) { Pain *= (0.9 - 0.1 * (Stamina + Health)); }

	//акт привязывания контроллера игрока
	void MakePossessedByDaemon(AActor* aDaemon);

	//применить пришедшую извне модель сил для всего тела
	void AdoptWholeBodyDynamicsModel(FWholeBodyDynamicsModel* DynModel, bool Fully);

	//кинематически телепортировать в новое место
	void TeleportToPlace(FTransform Dst);

	//включить или выключить желание при подходящей поверхности зацепиться за нее
	void SetWannaClimb(bool Set);

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
	void Hurt(float Amount, FVector ExactHitLoc, FVector Normal, EMyrSurface ExactSurface, FSurfaceInfluence* ExactSurfInfo = nullptr);

	//ментально осознать, что пострадали от действий другого существа или наоборот были им обласканы
	void SufferFromEnemy(float Amount, AMyrPhyCreature* Motherfucker);
	void EnjoyAGrace(float Amount, AMyrPhyCreature* Sweetheart);

	//запустить брызг частиц пыли и т.п. из чего состоит заданная поверхность
	void SurfaceBurst(UParticleSystem* Dust, FVector ExactHitLoc, FQuat ExactHitRot, float BodySize, float Scale);
	void SurfaceBurst(class UNiagaraSystem* Dust, FVector ExactHitLoc, FQuat ExactHitRot, float BodySize, float Scale);

	//проиграть звук контакта с поверхностью / шаг, удар
	void SoundOfImpact(FSurfaceInfluence *SurfInfo, EMyrSurface Surface, FVector Loc, float Strength, float Vertical, EBodyImpact ImpactType);

	//проиграть новый звук (подразумевается голос и прочие центральные звуки)
	void MakeVoice(USoundBase* Sound, uint8 strength, bool interrupt);

	//////////////////////////////////////////////////////////////////////

	//принять новое состояние поведения (логически безусловно, но технически может что-то не сработать, посему буль)
	bool AdoptBehaveState(EBehaveState NewState);

	//основная логика движения - свитч с разбором режимов поведения и переходами между режимами
	void ProcessBehaveState(float DeltaTime);

	/////////////////////////////////////////////
	bool GotUnclung()					{ return !Thorax->FixedOnFloor && !Pelvis->FixedOnFloor; }
	bool GotUntouched(float Thr = 0.01)	{ return (Thorax->StandHardness + Pelvis->StandHardness <= Thr); }
	bool GotSlow(float T = 1)			{ return (Pelvis->VelocityAgainstFloor.SizeSquared() < T && Thorax->VelocityAgainstFloor.SizeSquared() < T); }
	bool GotSoaringDown(float T = -50)	{ return (Mesh->GetPhysicsLinearVelocity().Z < T); }
	/////////////////////////////////////////////

	//проверка на провис передней или задней частью тела - внимание, должно вызываться после BewareFall, так как вариант "оба в воздухе" здесь опущен
	bool BewareHangTop() { if(Thorax->StandHardness < 0.01f) return AdoptBehaveState(EBehaveState::hang); else return false; }
	bool BewareHangBottom() { if(Pelvis->StandHardness < 0.01f) return AdoptBehaveState(EBehaveState::hang); else return false; }

	//проверить переворот (берётся порог значения Z вектора вверх спины)си принять пассивное лежание lie или активное tumble
	bool BewareOverturn(float Thresh = 0.0f, bool NowWalking = true);

	//в состоянии перекувыркнутости проверить, если мы выпрямились, встали на ноги
	bool BewareFeetDown(float Thresh = 0.0f) { return (!Mesh->IsSpineDown(Thresh)) ? AdoptBehaveState(EBehaveState::walk) : false; }

	//распознавание натыкания на стену с целью подняться
	bool BewareStuck(float Thresh = 0.5);

	//проверить на опору под ногами
	bool BewareGround(float thr = 0.01)
	{	if (!GotUntouched(thr))							//если хоть одна конечность теперь контачит с опорой
		{	if (Attacking())							//если в это время происходит какая-то атака
				if (GetAttackActionVictim().JumpPhase <= CurrentAttackPhase)
					NewAttackEnd();						//прыжковая атака должна быть экстренно завершена при окончании прыжка
			AdoptBehaveState(EBehaveState::walk);		//walk - это по умолчанию, затем оно быстро переведется в нужное состояние
			return true;								//критерий выхода со сменой запрашиваемого состояния
		} else return false;							//продолжить исследовать другие возможности
	}

	//условия перехода в смерть
	bool BewareDying();

	//////////////////////////////////////////////////////////////////////
	//действия - начать, ударить, прекратить досрочно
	
	EAttackAttemptResult NewAttackStart(int SlotNum, int VictimType = 0);
	EAttackAttemptResult NewAttackStrike();
	EAttackAttemptResult NewAttackStrikePerform();
	void NewAdoptAttackPhase(EActionPhase NewPhase);
	void NewAttackGetReady();
	void NewAttackEnd();					//завершение 
	void NewAttackGoBack();					//прерывание и разворот в спять - редко, чтобы не пересекать вглубь
	bool NewAttackNotifyGiveUp();			//прерывание неначатой
	void NewAttackNotifyEnd();				//поимка закладки		
	void NewAttackNotifyFinish();			//поимка закладки финиш - переход в финальную фазу перед завершением

	//процедуры прыжка из различных состояний повдения
	bool JumpAsAttack();

	//начать, выбрать и прервать самодействие
	void SelfActionStart(int SlotNum);
	void SelfActionCease();
	void SelfActionNewPhase();

	//найти, выбрать и начать анимацию отдыха
	void RelaxActionStart(int Slot);//стартануть релакс уже известный по индексу
	void RelaxActionReachPeace();	// * достичь фазы стабильности (вызывается из закладки анимации MyrAnimNotify)
	void RelaxActionStartGetOut();	//начать выход из фазы стабильности
	void RelaxActionReachEnd();		// * достичь конца и полного выхода (вызывается из закладки анимации MyrAnimNotify)

	//прервать или запустить прерывание деймтвий, для которых сказано прерывать при сильном касании
	void CeaseActionsOnHit();		
	void ActionFindList(bool RelaxNotSelf, TArray<uint8>& OutActionIndices, ECreatureAction Restrict = ECreatureAction::NONE);

	//вообще запустить какое-то действие по точному идентификатору (если есть в генофонде)
	EAttackAttemptResult  ActionFindStart   (ECreatureAction Type);
	EAttackAttemptResult  ActionFindRelease (ECreatureAction Type, UPrimitiveComponent* ExactGoal = nullptr);

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
	
	//отражение действия на более абстрактонм уровне - прокачка роли, эмоция, сюжет
	//возможно, вместо AActor следует ввести UObject, чтобы адресовать и компоненты, и всё прочее
	void CatchMyrLogicEvent(EMyrLogicEvent Event, float Param, UObject* Patient, FMyrLogicEventData* ExplicitEmo = nullptr);

	//передать информацию в анимацию из ИИ (чтобы не светить ИИ в классе анимации)
	void TransferIntegralEmotion(float& Rage, float& Fear, float& Power, float& Amount);

	//зарегистрировать пересекаемый объём с функционалом
	void AddOverlap(class UMyrTriggerComponent* Ov);
	bool DelOverlap(class UMyrTriggerComponent* Ov);

	//подправить курс по триггер-объёму
	bool ModifyMoveDirByOverlap(FVector& INMoveDir, bool AI);

//свои возвращуны
public:	

	//проверить извне, пересекает ли существо выбранный триггер
	bool HasOverlap(class UMyrTriggerComponent* Ov) const { return (Overlap0 == Ov || Overlap1 == Ov || Overlap2 == Ov); }

	//найти объём, в котором есть интересующая реакция, если нет выдать нуль
	UMyrTriggerComponent* HasSuchOverlap(EWhyTrigger r, FTriggerReason*& TR);
	UMyrTriggerComponent* HasSuchOverlap(EWhyTrigger rmin, EWhyTrigger rmax, FTriggerReason*& TR);

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
	UMyrGirdle* GetGirdle(ELimb L) { return Mesh->GetGirdleId(L) ? Thorax : Pelvis; }

	//выдать противоположный пояс
	UMyrGirdle* GetAntiGirdle(UMyrGirdle* G) { return G == Thorax ? Pelvis : Thorax;  }

	//контроллер искусственного интеллекта
	UFUNCTION(BlueprintCallable) class AMyrAI *MyrAI() const { return (AMyrAI*)Controller; }

	//выдать указатель на генофонд вовне
	UFUNCTION(BlueprintCallable) UMyrCreatureGenePool* GetGenePool() const { return GenePool; }

	//текущее состояние, безонтносительно от того, если какой-то переход
	EBehaveState GetBehaveCurrentState() const { return CurrentState; }

	//выдать вовне данные, связанные с текущим состоянием моторики
	UFUNCTION(BlueprintCallable) UMyrCreatureBehaveStateInfo* GetBehaveCurrentData() const { return BehaveCurrentData; }

	//************************************************************
	//новый вариант с единым массивом
	UFUNCTION(BlueprintCallable) UMyrActionInfo* GetAttackAction() const		{ return GenePool->Actions[(int)CurrentAttack]; }
	UFUNCTION(BlueprintCallable) FVictimType&    GetAttackActionVictim() const	{ return GenePool->Actions[(int)CurrentAttack]->VictimTypes[CurrentAttackVictimType]; }
	UFUNCTION(BlueprintCallable) UMyrActionInfo* GetSelfAction() const			{ return GenePool->Actions[(int)CurrentSelfAction]; }
	UFUNCTION(BlueprintCallable) UMyrActionInfo* GetRelaxAction() const			{ return GenePool->Actions[(int)CurrentRelaxAction]; }
	UFUNCTION(BlueprintCallable) UMyrActionInfo* GetAction(int i) const			{ return GenePool->Actions[i]; }
	//************************************************************

	//уровень метаболизма - визуальная скорость дыхания, сердебиения, эффектов в камере
	UFUNCTION(BlueprintCallable) float GetMetabolism() const;

	//участвует ли эта часть тела в непосредственно атаке
	bool IsLimbAttacking(ELimb eLimb);

	//касаемся ли мы/стоим ли мы на этом объекте
	bool IsTouchingThisActor(AActor* A);
	bool IsTouchingThisComponent(UPrimitiveComponent* A);
	bool IsStandingOnThisActor(AActor* A);

	//проверка на фазу атаки - для краткости
	bool NowPhaseStrike() const { return (CurrentAttackPhase == EActionPhase::STRIKE); }
	bool NowPhaseDescend() const { return (CurrentAttackPhase == EActionPhase::DESCEND); }

	//текущая фаза атаки соответствует указанным в сборке этой атаки для особых действий: подготовиться к прыжку, прыгнуть, схватить при касании
	bool NowPhaseToJump() { return (GetAttackActionVictim().JumpPhase == CurrentAttackPhase); }
	bool NowPhaseToHold() { return (GetAttackActionVictim().JumpHoldPhase == CurrentAttackPhase); }

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

	bool PreStrike() const { return (int)CurrentAttackPhase <= (int)EActionPhase::READY; }

	//делает какое-то из трех видов действий
	bool DoesAnyAction() const { return (CurrentAttack < 255) || (CurrentSelfAction<255) || (CurrentRelaxAction<255); }

	//потрачен
	bool Dead() { return (CurrentState == EBehaveState::dead);  }

	//оси ориентации существа (ориентируемся по мешу, положение опоры может сильно отличаться)
	FVector GetAxisForth() const { return ((UPrimitiveComponent*)Mesh)->GetComponentTransform().GetScaledAxis(EAxis::X); }
	FVector GetAxisLateral() const { return ((UPrimitiveComponent*)Mesh)->GetComponentTransform().GetScaledAxis(EAxis::Y); }

	//позиция головы в мировых координатах (берется из скелета меша)
	FVector GetHeadLocation();

	//выдать координаты центра масс конкретной части тела
	FVector GetBodyPartLocation(ELimb Limb);

	//конечная точка для трассировки с целью проверки видимости
	FVector GetVisEndPoint();

	//найти локацию той части тела, которая ближе всего к интересующей точке
	FVector GetClosestBodyLocation(FVector Origin, FVector Earlier, float EarlierD2);

	//вектор, в который тело смотрит глазами - для определения "за затылком"
	FVector GetLookingVector() const;

	//вектор тела вверх
	FVector GetUpVector() const;

	//получить множитель незаметности, создаваемый поверхностью, на которой мы стоим
	float GetCurrentSurfaceStealthFactor() const;

	//целевая скорость, с которой мы хотим двигаться
	UFUNCTION(BlueprintCallable) float GetDesiredVelocity() const { return BehaveCurrentData->MaxVelocity * MoveGain; }

	//длина спины 
	UFUNCTION(BlueprintCallable) float GetBodyLength() const { return Mesh->Bounds.BoxExtent.X*2; }

	//виджет над головой - для карткости
	class UMyrBioStatUserWidget* StatsWidget();

	//межклассовое взаимоотношение - для ИИ, чтобы сформировать отношение при первой встрече с представителем другого класса
	FColor ClassRelationToClass(AActor* obj);
	FColor ClassRelationToDeadClass(AMyrPhyCreature* obj);

	//для отладки, чтобы показывать в худе
	UFUNCTION(BlueprintCallable) FString GetCurrentAttackName()			const { return !NoAttack() && GetAttackAction()			? GetAttackAction()->GetName()			: FString(TEXT("NO")); }
	UFUNCTION(BlueprintCallable) FString GetCurrentSelfActionName()		const { return (!NoSelfAction() && GetSelfAction())		? GetSelfAction()->GetName()			: FString(TEXT("NO")); }
	UFUNCTION(BlueprintCallable) FString GetCurrentRelaxActionName()	const { return (!NoRelaxAction() && GetRelaxAction())	? GetRelaxAction()->GetName()			: FString(TEXT("NO")); }
	UFUNCTION(BlueprintCallable) FText GetCurrentAttackHumanName()		const { return !NoAttack() && GetAttackAction()			? GetAttackAction()->HumanReadableName	: FText(); }
	UFUNCTION(BlueprintCallable) FText GetCurrentSelfActionHumanName()	const { return (!NoSelfAction() && GetSelfAction())		? GetSelfAction()->HumanReadableName	: FText(); }
	UFUNCTION(BlueprintCallable) FText GetCurrentRelaxActionHumanName() const { return (!NoRelaxAction() && GetRelaxAction())	? GetRelaxAction()->HumanReadableName	: FText(); }

};
