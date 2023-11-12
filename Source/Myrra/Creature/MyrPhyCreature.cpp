// Fill out your copyright notice in the Description page of Project Settings.


#include "MyrPhyCreature.h"
#include "MyrPhyCreatureMesh.h"							// графическая модель
#include "../MyrraGameInstance.h"						// ядро игры
#include "../MyrraGameModeBase.h"						// текущий уровень - чтобы брать оттуда протагониста
#include "Artefact/MyrArtefact.h"						// неживой предмет - для логики поедания
#include "Artefact/MyrTriggerComponent.h"				// триггер - для посылки ему "я нажал Е!"
#include "Artefact/MyrLocation.h"						// локация по триггеру - обычно имеется в виду помещение
#include "../Control/MyrDaemon.h"						// демон
#include "../Control/MyrCamera.h"						// умная камера
#include "../Control/MyrAIController.h"					// ИИ
#include "../UI/MyrBioStatUserWidget.h"					// индикатор над головой
#include "AssetStructures/MyrCreatureGenePool.h"		// данные генофонда
#include "Animation/MyrPhyCreatureAnimInst.h"			// для доступа к кривым и переменным анимации
#include "Sky/MyrKingdomOfHeaven.h"						// для выяснения погоды и времени суток

#include "Components/WidgetComponent.h"					// ярлык с инфой
#include "Components/AudioComponent.h"					// звук
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"					//декаль следа
#include "PhysicsEngine/PhysicsConstraintComponent.h"	// привязь на физдвижке
#include "Particles/ParticleSystemComponent.h"			//для генерации всплесков шерсти при ударах
#include "NiagaraComponent.h"							//для генерации всплесков шерсти при ударах
#include "NiagaraFunctionLibrary.h"						//для генерации всплесков шерсти при ударах

#include "Kismet/GameplayStatics.h"						// для вызова SpawnEmitterAtLocation и GetAllActorsOfClass

#include "DrawDebugHelpers.h"							// рисовать отладочные линии

#include "../MyrraSaveGame.h"							// чтобы загружать существ из сохранок


//таблица подготовленных значений для целых уровней
float FPhene::LUT[PHENE_CURVE_MAX][MAX_LEVEL+2] = {
	// 0      1     2      3      4      5       6      7      8      9      10     11     12     13     14     15     16     17    18      19     20     21     22     23     24     25     26     27     28     29     30     31    32
	{ 0.00, 0.031, 0.062, 0.093, 0.125, 0.156, 0.187, 0.218, 0.250, 0.281, 0.312, 0.343, 0.375, 0.406, 0.437, 0.468, 0.500, 0.531, 0,562, 0.593, 0.625, 0.656, 0.687, 0.718, 0.750, 0.781, 0.812, 0.843, 0.875, 0.906, 0.937, 0.968, 0.999 },
	{ 0.00, 0.031, 0.062, 0.093, 0.125, 0.156, 0.187, 0.218, 0.250, 0.281, 0.312, 0.343, 0.375, 0.406, 0.437, 0.468, 0.500, 0.531, 0,562, 0.593, 0.625, 0.656, 0.687, 0.718, 0.750, 0.781, 0.812, 0.843, 0.875, 0.906, 0.937, 0.968, 0.999 },
	{ 0.00, 0.031, 0.062, 0.093, 0.125, 0.156, 0.187, 0.218, 0.250, 0.281, 0.312, 0.343, 0.375, 0.406, 0.437, 0.468, 0.500, 0.531, 0,562, 0.593, 0.625, 0.656, 0.687, 0.718, 0.750, 0.781, 0.812, 0.843, 0.875, 0.906, 0.937, 0.968, 0.999 },
	{ 0.00, 0.031, 0.062, 0.093, 0.125, 0.156, 0.187, 0.218, 0.250, 0.281, 0.312, 0.343, 0.375, 0.406, 0.437, 0.468, 0.500, 0.531, 0,562, 0.593, 0.625, 0.656, 0.687, 0.718, 0.750, 0.781, 0.812, 0.843, 0.875, 0.906, 0.937, 0.968, 0.999 },
};


//свой лог
DEFINE_LOG_CATEGORY(LogMyrPhyCreature);

//одной строчкой вся логика ответов на запросы о переходе в новое состояние
#define BEWARE(state, cond, reason) if(cond) if(AdoptBehaveState(EBehaveState::##state,reason)) {break;}
#define BEWAREELSE(state, altst, cond) if(cond) if(!AdoptBehaveState(EBehaveState::##state)) {if(AdoptBehaveState(EBehaveState::##altst)) break;} else break
#define BEWAREA(state, cond, act, reason) if(cond){ act; if(AdoptBehaveState(EBehaveState::##state,reason)) {  break;}}

//позиция головы в мировых координатах
FVector AMyrPhyCreature::GetHeadLocation() { return Mesh->GetHeadLocation(); }

//виджер ярлык над тельцем
UMyrBioStatUserWidget* AMyrPhyCreature::StatsWidget() {  return Cast<UMyrBioStatUserWidget>(LocalStatsWidget->GetUserWidgetObject()); }


//====================================================================================================
// здесь создаются компоненты и значения по умолчанию
//====================================================================================================
AMyrPhyCreature::AMyrPhyCreature()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;
	PrimaryActorTick.EndTickGroup = TG_PrePhysics;
	PrimaryActorTick.bTickEvenWhenPaused = false;
	bMoveBack = 0;
	bClimb = 0;

	//механический лошарик
	Mesh = CreateOptionalDefaultSubobject<UMyrPhyCreatureMesh>(TEXT("Mesh"));
	RootComponent = Mesh;

	//отбойник не должен быть первым, потому что в редакторе его не развернуть горизонтально
	Thorax = CreateDefaultSubobject<UMyrGirdle>(TEXT("Thorax"));
	Thorax->SetupAttachment(RootComponent);
	Thorax->IsThorax = true;
	Thorax->SetGenerateOverlapEvents(true);
	Pelvis = CreateDefaultSubobject<UMyrGirdle>(TEXT("Pelvis"));
	Pelvis->SetupAttachment(RootComponent);
	Pelvis->IsThorax = false;
	Pelvis->SetGenerateOverlapEvents(false);

	//компонент персональных шкал здоровья и прочей хрени (висит над мешем)
	LocalStatsWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Local Stats Widget"));
	LocalStatsWidget->SetupAttachment(Mesh, TEXT("HeadLook"));
	LocalStatsWidget->SetGenerateOverlapEvents(false);
	LocalStatsWidget->SetVisibility(false);

	//источник звука для голоса
	Voice = CreateDefaultSubobject<UAudioComponent>(TEXT("Voice Audio Component"));
	Voice->SetupAttachment(RootComponent);
	Voice->OnAudioPlaybackPercent.AddDynamic(this, &AMyrPhyCreature::OnAudioPlaybackPercent);
	Voice->OnAudioFinished.AddDynamic(this, &AMyrPhyCreature::OnAudioFinished);

	//отсеивать нефункциональные экземпляры класса
	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	//первоначальное направление - вперед
	MoveDirection = GetAxisForth();

	//сразу назаначаем класс ИИ контроллера, который будет создаваться для любого экземпляра и потомка
	AIControllerClass = AMyrAIController::StaticClass();
}

//====================================================================================================
//перет инициализацией компонентов, чтобы компоненты не охренели от родительской пустоты во время своей инициализации
//====================================================================================================
void AMyrPhyCreature::PreInitializeComponents()
{
	//подготовка к игре под капотом
	Super::PreInitializeComponents();

	//инициализируем начальное состояние поведения, чтобы не нарваться на пустоту
	if(GetGenePool()) BehaveCurrentData = *GetGenePool()->BehaveStates.Find(EBehaveState::walk);

	//если происходит загрузка из сохранения (она в любом случае сопровождается перезагрузкой уровня
	//и перевысером всех его обитателей, значит и эта функция будет вызывана) заранее вписать указатель на этот объект
	//в заготовки памяти ИИ всех прочих существ, которые перед сохранением знали это существо,
	//чтобы к моменту начала игры можно было бы просто слить все гештальты в память ИИ и знать
	//что все указатели там указывают на реальных существ на уровне
	ValidateSavedRefsToMe();
}

//====================================================================================================
// появление в игре - финальные донастройки, когда всё остальное готово
//====================================================================================================
void AMyrPhyCreature::BeginPlay()
{
	//против запуска в редакторе блюпринта
	if (IsTemplate()) { Super::BeginPlay(); return; }

	//построить карту действий для быстрого дсотупа, если она еще не построена другим существом того же вида
	if (GenePool->ActionMap.Num() == 0)
		GenePool->AnalyzeActions();
	
	//если существо грузится из сохранки, то для него припасена структура данных,
	//которая после загрузки уровня из сохранки дожидается, когда существо дорастет до BeginPlay
	FCreatureSaveData* CSD = nullptr;
	if (GetMyrGameInst()->JustLoadedSlot)
		CSD = GetMyrGameInst()->JustLoadedSlot->AllCreatures.Find(GetFName());

	//загрузить, если мы нашлись достойными быть сохраненным
	if (CSD) Load(*CSD);

	//если нет сохранения или мы высраны в рантайме / играем тестовов в редакторе
	else
	{
		//сгенерировать человекочитаемое имя по алгоритму из генофонда
		HumanReadableName = GenerateHumanReadableName();

		//задание стартовых, минимальных значений ролевых параметров
		for (int i = 0; i < (int)EPhene::NUM; i++)
		{
			//устанавливаем на первый уровень с произвольным отклонением в обе стороны на всю амплитуду
			Phenotype.ByInt(i).SetRandom(1, MAX_XP_UPLVL*2);
			UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s First Assign Params %s %d"),
				*GetName(), *TXTENUM(EPhene,(EPhene)i), Phenotype.ByInt(i).XP);
		}

		SetCoatTexture(Coat);
	}

	//сгенерировать имя
	if(HumanReadableName.IsEmpty()) HumanReadableName = GenerateHumanReadableName();


	//подготовка к игре под капотом
	Super::BeginPlay();

	//чтобы с самого начала было какое-то состояние, подкрепленное данными
	AdoptBehaveState(EBehaveState::fall);

	//пространство экрана - всегда лицом, не перекрывается
	LocalStatsWidget->SetWidgetSpace(EWidgetSpace::Screen);	

	//отклонение от точки привязки - вверх (ввести спец переменную?)
	//LocalStatsWidget->SetPivot(FVector2D(0, 1));				
	LocalStatsWidget->SetRelativeLocation(FVector::ZeroVector);	

	//дать виджету знать, в контексте чего он будет показываться 
	//ахуеть! если это вызвать до Super::BeginPlay(), то виджет ещё не будет создан, и ничего не сработает
	if (StatsWidget())
	{	StatsWidget()->SetOwnerCreature(this);
		StatsWidget()->MyrAIC = MyrAIController();			
	}

	//рассчитать анимации длины/амплитуды ног - пока здесь, потому что чем позже тем лучше
	//должны быть полностью спозиционированы все кости
	Pelvis->UpdateLegLength();
	Thorax->UpdateLegLength();

	//задать начальное, нормальное значение длины спины до того как якоря поясов будут расставлены
	Mesh->CalcSpineVector(SpineVector, SpineLength);
	Mesh->StartGirdlesDeltaZ = Mesh->GetGirdlesDeltaZ();

	//свойства гашения звука голоса берем из текущего уровня (вероятно, у локаций будут другие)
	Voice->AttenuationSettings = GetMyrGameMode()->SoundAttenuation;

	//звякнуть геймплейным событием - в основном для высираемых мобов
	CatchMyrLogicEvent(EMyrLogicEvent::SelfSpawnedAtLevel, Daemon ? 1 : 0, nullptr);
	SetTickableWhenPaused(false);
	LocalStatsWidget->SetVisibility(true);
}

void AMyrPhyCreature::EndPlay(EEndPlayReason::Type R)
{
	if (GenePool->ActionMap.Num()) GenePool->ActionMap.Empty();
	Super::EndPlay(R);
}

//====================================================================================================
// каждый кадр
//====================================================================================================
void AMyrPhyCreature::Tick(float DeltaTime)
{
	if (HasAnyFlags(RF_ClassDefaultObject)) return;
	if (IsTemplate()) return;
	//if (GetMyrGameMode()->IsPaused()) return;

	//для мертвого ничего больше не нужно, но что конкетно опустить, пока до конца не ясно
	if (CurrentState == EBehaveState::dead) return;

	//набор мощи для атаки/прыжка
	if (AttackForceAccum > 0.0f && AttackForceAccum < 1.0f)
		AttackForceAccum += DeltaTime;

	//для игрока пересчитать курс движения по зажатым кнопкам
	if (Daemon) Daemon->MakeMoveDir(BehaveCurrentData->bOrientIn3D);

	//демон сохраняет свои планы в отдельные переменные, здесь они по идее должны смешиваться с таковыми из ИИ
	//но пока просто тупо присваивается
	ConsumeInputFromControllers(DeltaTime);

	//извлечь степень физической уткнутости (найти членик,  
	if (Mesh->FindBumpingLimb()) BestBumpFactor = 0;

	//нормальная, физическая фаза
	//обработать пояса конечностей
	Thorax->SenseForAFloor(DeltaTime);
	Pelvis->SenseForAFloor(DeltaTime);

	//пересчитать вектор прямой спины
	Mesh->CalcSpineVector(SpineVector, SpineLength);

	//поиск подходящей реакции на затык
	if (Mesh->Bumps()					//если в принципе куда-то утыкается боковыми члениками
		&& !DoesSelfAction())			//если не выполняет самодействие, типа чтоб не прерывать, хотя сомнительно, проще приоритеты растыков занизить
	{
		//нахождения лучшей выборки за череду соударений одним и тем же члеником
		//влияние скорости соударения, берется часть общей скорости пояса в срезе нормали
		auto& L = Mesh->Bumper1();
		float Frontality = Mesh->Bumper1().GetColinea();
		float SpeedFactor = (GetGirdle(L.WhatAmI)->VelocityAtFloor() / (BehaveCurrentData->MaxVelocity + 1)) | (-L.Floor.Normal);
		BestBumpFactor = FMath::Max(Frontality * FMath::Max(SpeedFactor, 0.0f), BestBumpFactor);

		//при довольно сильном затыке в режиме детерминированного полёта
		if (JumpTarget && BestBumpFactor > 0.4) JumpTarget = nullptr;

		//результат вызова реакции на затык
		EResult R = EResult::NO_WILL;
		FString TagForLog;

		//ударяемся задней частью членика - по факту задними члениками: тазом, хвостом - случайность, порог срабатывания большой
		if ((Mesh->GetWholeForth() | L.Floor.Normal) > 0.1)
		{	if (BestBumpFactor > 0.3)
			{	TagForLog = TEXT("Rear");
				if (BestBumpFactor > 0.6)	R = ActionFindStart(EAction::Bump_Rear_Fast);
				else						R = ActionFindStart(EAction::Bump_Rear_Slow);
			}						
		}
		//ударяемся, когда врублен режим лазанья (широкий порог, важно) если стена, куда ударились, лазибельная, то встаем на дыбы
		else if (bClimb && !Thorax->Climbing)
		{	if (BestBumpFactor > 0.2)
			{	TagForLog = TEXT("PreClimb");
				if (L.Floor.IsClimbable())	R = ActionFindStart(EAction::Bump_Climb_GoodWall); else
				if (BestBumpFactor > 0.2)	R = ActionFindStart(EAction::Bump_Climb_BadWall);
			}
		}
		//ударяемся об стену, на которую можно забраться, даже если она не поддерживает карабканье
		else if (L.Floor.CharCanStep() && !Thorax->Ascending && !Thorax->Descending)
		{	if(BestBumpFactor > 0.2)
			{	TagForLog = TEXT("Steppable");
				if(BestBumpFactor > 0.5)	R = ActionFindStart(EAction::Bump_GoodWall_Fast);
				else						R = ActionFindStart(EAction::Bump_GoodWall_Slow);
			}
		}
		//ударяемся об стену, которая полностью глухая, на нее нельзя взобраться
		else
		{	if (BestBumpFactor > 0.2)
			{	TagForLog = TEXT("Obstacle");
				if (BestBumpFactor > 0.5)	R = ActionFindStart(EAction::Bump_BadWall_Fast);
				else						R = ActionFindStart(EAction::Bump_BadWall_Slow);
			}
		}

		if(R != EResult::NO_WILL)
		{	Mesh->EraseBumpTouch();
			BestBumpFactor = 0;
			UE_LOG(LogMyrPhyCreature, Log, TEXT("%s BumpReact %s %s Limb=%s, f=%g"), *GetName(),
				*TXTENUM(ELimb, L.WhatAmI), *TXTENUM(EResult, R), *TXTENUM(ELimb, L.WhatAmI), BestBumpFactor);
		}
	}

	//только для игрока
	if (Daemon)
	{
		//действия с объектом в прицеле
		if (Daemon->ObjectAtFocus)
		{
			//написать имя противника на экране и стереть когда нужно
			if (Daemon->HUDOfThisPlayer())
				Daemon->HUDOfThisPlayer()->SetGoalActor(Daemon->ObjectAtFocus->Obj->GetOwner());

			//при автоприцеле насильно изменить направление атаки в пользу реального направления на цель
			if (Daemon->AutoAim) AttackDirection = Daemon->ObjectAtFocus->Location.DecodeDir();
		}
		//немедленно удалять в отсутствие
		else if (Daemon->HUDOfThisPlayer())
			Daemon->HUDOfThisPlayer()->SetGoalActor(nullptr);
	}

	//восстановление запаса сил (используется та же функция, что и при трате)
	StaminaChange (Mesh->DynModel->StaminaAdd * DeltaTime );										

	//обнаруживание проваливания под землю и вырыв оттуда немедленно
	auto eL = Mesh->DetectPasThrough();
	if (eL != ELimb::NOLIMB)
	{	FTransform T = GetActorTransform();
		T.SetLocation(T.GetLocation() + FVector(0, 0, SpineLength));
		TeleportToPlace(T);
		UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s DetectPasThrough %s"),
			*GetName(), *TXTENUM(ELimb, (ELimb)eL));
	}

	//обработка движений по режимам поведения 
	ProcessBehaveState(DeltaTime);

	//боль - усиленная разность старого, высокого здоровья и нового, сниженного мгновенным уроном. 
	Pain *= 1 - DeltaTime * 0.7 * Health;

	//на стадии подготовеи к прыжку производится активный поиск точных целей
	if (Mesh->DynModel->PreJump && Daemon)
	{	FVector3f DummyVel;
		PrepareJumpVelocity(DummyVel);
	}

	//включение и выключение режима кинематики может происходить в любой кадр, в зависимости от лода
	bool needkin =  GetBehaveCurrentData()->FullyKinematicSinceLOD <= Mesh->GetPredictedLODLevel();
	if(needkin != bKinematicMove) SetKinematic(needkin);

	//если есть что метаболировать из еды, это надо делать каждый кадр
	//по всем текущим эффектам от пищи
	for (int i = DigestiveEffects.Effects.Num() - 1; i >= 0; i--)
	{
		float* Dst = nullptr;
		switch (DigestiveEffects.Effects[i].Target)
		{
		case EDigestiveTarget::Energy:			Dst = &Energy; break;
		case EDigestiveTarget::Health:			Dst = &Health; break;
		case EDigestiveTarget::Stamina:			Dst = &Stamina; break;
		case EDigestiveTarget::Pain:			Dst = &Pain; break;
		case EDigestiveTarget::Psychedelic:     if (Daemon) Dst = &Daemon->Psychedelic; break;
		case EDigestiveTarget::Sleepiness:		if (Daemon) Dst = &Daemon->Sleepiness; break;
		}

		//модифицировать параметр, если порция испита до дна, удалить ее
		if (DigestiveEffects.Effects[i].SpendToParam(Dst, DeltaTime))
			DigestiveEffects.Effects.RemoveAt(i);
	}

	//по идее здесь тичат компоненты
	Super::Tick(DeltaTime);

	FrameCounter++;
}


//==============================================================================================================
// вызывается, когда играется звук (видимо, каждый кадр) и докладывает, какой процент файла проигрался
//====================================================================================================
void AMyrPhyCreature::OnAudioPlaybackPercent(const USoundWave* PlayingSoundWave, const float PlaybackPercent)
{
	//ебанатский анрил 4.27 запускает это сообщение даже когда не играет звук
	if (!Voice->IsPlaying()) return;
	
	//длина строки субитров
	const int Length = PlayingSoundWave->Comment.Len();

	//открывать рот только если в текущем аудио есть субтитры (с версии 4.24 вызывается и когда 100%)
	if (Length > 0 && PlaybackPercent < 1.0)
	{
		//текущий произносимый звук
		//здесь также можно парсить строку, чтобы вносить эмоциональные вариации
		CurrentSpelledSound = (EPhoneme)PlayingSoundWave->Comment[Length * PlaybackPercent];
	}
}

//==============================================================================================================
// вызывается, когда закончен основной звук рта
//====================================================================================================
void AMyrPhyCreature::OnAudioFinished()
{
	//закрыть существу рот
	CurrentSpelledSound = (EPhoneme)0;
}

#if WITH_EDITOR
//==============================================================================================================
//для распространения свойств при сменен генофонда
//==============================================================================================================
void AMyrPhyCreature::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	//окрасить в соответствии с индивидуальностью
	FName PropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	//смена генофонда, то есть видовой принадлежности этого существа
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMyrPhyCreature, GenePool))
	{	if (GenePool)
		{	Mesh->SetSkeletalMesh(GenePool->SkeletalMesh);
			BehaveCurrentData = *GetGenePool()->BehaveStates.Find(EBehaveState::walk);
		}
	}

	//ручная смена расы/окраски в редакторе
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AMyrPhyCreature, Coat))
		SetCoatTexture(Coat);
	
	//генерация имени
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AMyrPhyCreature, HumanReadableName))
		HumanReadableName = GenerateHumanReadableName();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif


//==============================================================================================================
//при рантайм-высере правильно инициализировать
//==============================================================================================================
void AMyrPhyCreature::RegisterAsSpawned(AActor* Spawner)
{
	SpawnDefaultController();
	if (StatsWidget())
		StatsWidget()->MyrAIC = MyrAIController();
}

//==============================================================================================================
//переварить внешнюю сборку командмоторики
//==============================================================================================================
void AMyrPhyCreature::ConsumeDrive(FCreatureDrive *D)
{
	//прямо присваиваем направление атаки/поворота головы
	AttackDirection = D->ActDir;

	//по умолчанию убрать флаг пячки назад, если он еще нужен, он возведется в одной из ветвей
	bMoveBack = false;

	//дабы не считать сложную операцию много раз
	bool NeedNormalizing = false;

	//особый лучай при кинематическом движении по параболе
	if (CurrentState == EBehaveState::project)
		MoveDirection = GetJumpVelAtNow().GetSafeNormal();

	//руление курса движения осуществляется только при наличии тяги
	else if(D->Gain > 0)
	{
		//любой привод тела с тягой показывает желание двигаться
		bMove = true;

		//действия в случае раскорячки тела и курса: для начала просчитать степень этой раскорячки
		if(BehaveCurrentData->TurnReactions.Num()>0)
		{
			//сонаправленность тела и курса, который от него требуют
			float CourseOff = Mesh->GetLimbAxisForth(Mesh->Lumbus) | D->MoveDir;
			FTurnReaction* BestTurnReaction = nullptr;

			//для случая хотя бы одного элемента в сборке
			for (auto TR : BehaveCurrentData->TurnReactions)
			{
				//дополнительные условия для ИИ/игрока, для 3/1лица
				if (IsUnderDaemon())
				{	if (Daemon->IsFirstPerson() && !TR.ForFirstPerson()) continue; else
					if (!Daemon->IsFirstPerson() && !TR.ForThirdPerson()) continue;
				}else if(!TR.ForNPC()) continue;

				//основное условия срабатывания, расскоряка выше порога
				if (CourseOff < TR.Threshold)
				{	BestTurnReaction = &TR;
					break;
				}
			}
			//из всех реакций разворота нашли хоть одну или выбрали лучшую
			if(BestTurnReaction)
			{
				//режимы выруливания на новый курс
				switch(BestTurnReaction->DoIfReached)
				{
					//бессмысленное
					case ETurnMode::None:
						MoveDirection = D->MoveDir;
						break;

					//плавное выруливание с постоянной дельтой
					case ETurnMode::SlowConst:
					{
						//направление вбок как базис для плавного руления
						FVector3f SideWay = Mesh->GetLimbAxisRight(Mesh->Lumbus);
						if((SideWay|D->MoveDir)<0) SideWay = -SideWay;
						MoveDirection = MoveDirection + SideWay * BestTurnReaction->SpeedFactor;

						//укладывание в плоскость ХУ, если в настройках сказано, поза тела может искажать
						if (!BehaveCurrentData->bOrientIn3D) MoveDirection.Z = 0;
						NeedNormalizing = true;
					}
					break;

					//менее плавное выруливание с линейной интерполяцией
					case ETurnMode::SlowLerp:
						MoveDirection = FMath::Lerp(MoveDirection, D->MoveDir, BestTurnReaction->SpeedFactor);
						NeedNormalizing = true;
						break;

					//режим рельсовости
					case ETurnMode::RailBack:

						//направление приводится к вектору тела вперед или назад
						if(CourseOff > 0.0)	MoveDirection = SpineVector;
						else
						{	
							BEWARE(walkback, CurrentState == EBehaveState::walk || CurrentState == EBehaveState::stand, "");
							//MoveDirection = -Thorax->Forward;
							bMoveBack = true;
						}
						//для кручения в плоскости зед убирается, и вектор перестает быть единичным
						if (!BehaveCurrentData->bOrientIn3D)
						{	MoveDirection.Z = 0;
							NeedNormalizing = true;
						}
				}
				//UE_LOG(LogMyrPhyCreature, Log, TEXT("%s: ConsumeDrive turn %s, r=%g"),
				//	*GetOwner()->GetName(), *TXTENUM(ETurnMode, BestTurnReaction->DoIfReached), CourseOff);
			
			//ни одна реакция не подпадает под все условия - делаем по умолчанию
			} else MoveDirection = D->MoveDir;
		}
		//не задано никаких условий по плавному рулению = просто присвоить
		else MoveDirection = D->MoveDir;
	}

	//если есть объём, понуждающий к жесткому курсу, он искажает вектор и точно надо нормализовать
	NeedNormalizing |= ModifyMoveDirByOverlap (MoveDirection, EWhoCame::Player);
	if(NeedNormalizing)
		MoveDirection.Normalize();

	////////////////////////////////////////////////////////////////////////////////////////////////
	//если выше, то линейно увеличиваем, а если ниже, то резко обрываем (нужно остановиться вовремя)
	if(D->Gain != ExternalGain)
	{
		//знаковая разница между новым и текущим уровнем тяги
		float Raznice = D->Gain - ExternalGain;

		//если новый уровень ниже, то есть тормозим
		if (Raznice <= 0.05f)
		{
			//приходится сильно тормозить
			if (Raznice < -0.1f)
			{
				//кинетическая энергия
				float E = FMath::Clamp(Pelvis->Speed * 0.01 - 1, 0.1f, 0.9f);
				ExternalGain -= 0.05f*(1-E);
			}
			//при малой разнице вне зависимости от скорости (для простоты и сходимости) сразу присваивается новый
			else ExternalGain = D->Gain;
		}
		//новый уровень выше - линейно ускориться
		else ExternalGain += 0.05f;

		//для режима лазанья включение бега вдвое ускоряе, а не-бег вдвое замедляет
		if (!bRun && bClimb)
			if(ExternalGain>0.5)
				ExternalGain *= 0.5;

		//расчет сборного коэффициента замедления, который уменьшает ориентировочную скорость
		UpdateMobility();
	}

	//извлечение конкретных команд и выполнение их
	if (D->DoThis != EAction::NONE)
	{
		if (D->Release) ActionFindRelease(D->DoThis);
		else ActionFindStart(D->DoThis);
		D->DoThis = EAction::NONE;
		D->Release = false;
	}
}


//==============================================================================================================
//взять команды и данные из контроллеров игрока и иИИ в нужной пропорции
//==============================================================================================================
void AMyrPhyCreature::ConsumeInputFromControllers(float DeltaTime)
{
	//обнуляется, чтобы подготовит к чтению новых
	bMove = false;

	//играется игроком
	if(Daemon)
	{	
		//ИИ НЕ может частично отбирать управление у игрока
		if(MyrAIController()->AIRuleWeight < 0.1f)
		{
			//заглотить указания
			ConsumeDrive(&Daemon->Drive);

			//особая фигня от первого лица когда смотришь назад
			//чтоб не глазеть внутрь шеи, всё тело активируется движением и поворачивается
			if (Daemon->IsFirstPerson())
			{	FVector3f RealForth = Mesh->GetLimbAxisForth(Mesh->Pectus);
				if((AttackDirection|RealForth)<-0.1)
				{	MoveDirection = AttackDirection;
					MoveDirection.Z = 0;
					MoveDirection.Normalize();
					ExternalGain = 1.0f;
					UpdateMobility();
				}
			}
		}
		//режим раскоряки между управлением 
		else
		{
			FCreatureDrive D(MyrAIController()->Drive, Daemon->Drive, MyrAIController()->AIRuleWeight);
			ConsumeDrive ( &D );
		}
	}
	//непись, только ИИ, стандартно просчитать новые устремления все
	else ConsumeDrive(&MyrAIController()->Drive);
}

//==============================================================================================================
//оценить применимость реакции на уткнутость и если удачно запустить ее
//==============================================================================================================
EResult AMyrPhyCreature::TestBumpReaction(FBumpReaction* BR, ELimb BumpLimb)
{
	auto& L = Mesh->Bumper1();
	float B = Mesh->Bumper1().GetColinea();
	if (BR->Reaction == EAction::NONE)		return EResult::INCOMPLETE_DATA;
	if ((BR->BumperLimb & (1<<(int)L.WhatAmI))==0)	return EResult::WRONG_ACTOR;

	if(L.Floor)
	{	
		if(L.Floor.CharCanStep())
		{	if (BR->Threshold < 0)					return EResult::WRONG_PHASE;
			else if (B < BR->Threshold)				return EResult::OUT_OF_ANGLE;
		}
		else
		{	if (BR->Threshold > 0)					return EResult::WRONG_PHASE;
			else if (B < -BR->Threshold)			return EResult::OUT_OF_ANGLE;
		}
	}else											return EResult::INCOMPLETE_DATA;

	FVector3f BoSp = 0.5*(Mesh->BodySpeed(L) + Thorax->VelocityAtFloor());
	if((BoSp|(-L.Floor.Normal)) < BR->MinVelocity)
													return EResult::OUT_OF_VELOCITY;
	
	ActionFindStart(BR->Reaction);					return EResult::STARTED;
}


//==============================================================================================================
//обновить осознанное замедление скорости по отношению к норме для данного двигательного поведения
//==============================================================================================================
void AMyrPhyCreature::UpdateMobility(bool IncludeExternalGain)
{
	//при усталости скорость падает только когда запас сил меньше 1/10 от максимума
	const float Fatigue = (Stamina < 0.1) ? Stamina * 10 : 1.0f;

	//здесь будет длинная формула
	MoveGain = Fatigue											// источщение запаса сил
				* Phenotype.MoveSpeedFactor()					// общая быстрота индивида 
				* (1.0f - Mesh->GetWholeImmobility())			// суммарное увечье частей тело взвешенное по коэффициентам влияния на подвижность
				* Mesh->DynModel->GetMoveWeaken()				// замедление или остановка хода даже при нажатой кнопке, например для релакса или подтоговки к прыжку
				* FMath::Max(1.0f - Pain, 0.0f);				// боль уменьшает скорость (если макс.боль выше 1, то на просто парализует на долгое время, пока она не спадет)

	//если тягу в расчёт не включать, то мобильность превращается в коэффициент замедления, а не прямое указание к силе движения
	if (IncludeExternalGain)
	{
		//если есть сильная уткнутость в препятствие, скорость понижается
		if (Mesh->Bumps())
			if(Mesh->Bumper1().GetBumpCoaxis() > 0.8) MoveGain *= FMath::Max(5*(1 - Mesh->Bumper1().GetBumpCoaxis()), 0.0f);

		//особый случай когда в центральных члениках задана команда двигаться даже без тяги
		if (Mesh->DynModel->MoveWithNoExtGain)
			MoveGain *= FMath::Max(Mesh->DynModel->MotionGain, ExternalGain);

		//стандартный случай, если тяги нет, тело, зафиксированное на опоре, кинематически обездвиживается
		else MoveGain *= ExternalGain;
	}
}


//==============================================================================================================
//акт привязывания контроллера игрока
//==============================================================================================================
void AMyrPhyCreature::MakePossessedByDaemon(AActor* aDaemon)
{
	//если подан нуль - текущий демон нас покидает
	if (!aDaemon)
	{	Daemon = nullptr;
		if (MyrAIController()) MyrAIController()->AIRuleWeight = 1.0f;
	}

	//проверяем типы и если сходятся присваиваем указатель
	else if (AMyrDaemon* daDaemon = Cast<AMyrDaemon>(aDaemon))
	{	Daemon = daDaemon;
		if (MyrAIController()) MyrAIController()->AIRuleWeight = 0.0f;
	}
}

//==============================================================================================================
//применить пришедшую извне модель сил для всего тела
//==============================================================================================================
void AMyrPhyCreature::AdoptWholeBodyDynamicsModel(FWholeBodyDynamicsModel* DynModel)
{
	//явная ошибка
	if (!DynModel) return;

	//голос/общий звук прикреплен к дин-модели, хотя не физика/хз, нужно ли здесь и как управлять силой
	MakeVoice(DynModel->Sound.Get(), 1, false);

	//если предлагаемая модель не предназначена для внедрения, найти в стеке более раннюю
	if (!DynModel->Use)
	{	auto Subst = GetPriorityModel();
		if (Mesh->DynModel == Subst) return; else
			Mesh->DynModel = Subst;
	}
	else Mesh->DynModel = DynModel;

	//если не для галочки, принять общие настройки для всех поясов
	if (Mesh->DynModel->Use)
	{
		//напрячь или расслабить спину
		Mesh->SetSpineStiffness(Mesh->DynModel->SpineStiffness * FMath::Min(Health * 5, 1.0f));

		//фаза подготовки и стартовая доля силы
		if (Mesh->DynModel->PreJump)
			AttackForceAccum = 0.1f;

		//текущая фаза - прыгнуть
		if (Mesh->DynModel->JumpImpulse > 0 || Mesh->DynModel->MotionGain > 1)
		{	FVector3f TempVelo(0,0,0);			// явная скорость
			PrepareJumpVelocity(TempVelo);			// последний или единственный раз поискать прямые цели
			JumpAsAttack(TempVelo);				// нашли, направить скорость именно к цели
		}

		//применить новые настройки динамики движения/поддержки для частей тела поясов
		Thorax->AdoptDynModel(DynModel->Thorax);
		Pelvis->AdoptDynModel(DynModel->Pelvis);

		//эффекты для эгрока
		if (Daemon)
		{	Daemon->SetTimeDilation(DynModel->TimeDilation);	//замедление времени
			Daemon->EnableJumpTrajectory(DynModel->PreJump);	//отрисовка параболы будущего прыжка
		}

		//в дин-моделях есть замедлитель тяги, чтобы его прменить, нужно вызвать вот это
		UpdateMobility();
	}
	
}

//==============================================================================================================
//кинематически двигать в новое место - нах отдельная функция, слишком простое
//==============================================================================================================
void AMyrPhyCreature::KinematicMove(FTransform Dst)
{
	//сначала безусловно подвинуть тушку в нужное место
	Mesh->SetWorldTransform(Dst, false, nullptr, ETeleportType::TeleportPhysics);

	//затем вручную пристыковать якоря к поясам конечностей, так как они не наследуют движение
	Thorax->ForceMoveToBody();
	Pelvis->ForceMoveToBody();
}

//==============================================================================================================
//телепортировать на место - разовая акция, теперь возможно, лишнее
//==============================================================================================================
void AMyrPhyCreature::TeleportToPlace(FTransform Dst)
{
	Mesh->SetSimulatePhysics(false);
	KinematicMove(Dst);
	Mesh->SetSimulatePhysics(true);
	UE_LOG(LogMyrPhyCreature, Log, TEXT("%s: TeleportToPlace %s"), *GetName(),  *Dst.GetLocation().ToString());
	if (Daemon)	Daemon->PoseInsideCreature();
}

//==============================================================================================================
//перейти в кинематическое состояние или вернуться в детальную физическую симуляцию
//==============================================================================================================
void AMyrPhyCreature::SetKinematic(bool Set)
{
	//для кинематики тела должны быть прозрачными и не сталкиваться с рельефом
	Mesh->SetPhyBodiesBumpable(!Set);

	//и вообще не загружаться физикой, раз кинематика
	Mesh->SetMachineSimulatePhysics(!Set);
	bKinematicMove = Set;

	//для объёмов секций установить абсолютные координаты, хотя хз, насколько нужно, они в принципе не столь нужны
	Thorax->SetAbsolute(!Set, !Set, false);
	Pelvis->SetAbsolute(!Set, !Set, false);

	//здесь надо осторожнее, ибо при выходе обнуляется камера
	if (!Set && Daemon)	Daemon->PoseInsideCreature(false);

}


//==============================================================================================================
// включить или выключить желание при подходящей поверхности зацепиться за нее
//==============================================================================================================
void AMyrPhyCreature::ClimbTryActions()
{
	//удачно зацепились - прервать все действия, чтоб не мешались
	if (Thorax->Climbing || Pelvis->Climbing)
		CeaseActionsOnHit(1000);

	//для чистого ИИ не подпрыгивать, он сам должен понимать, когда перелезать
	if(!Daemon) return;

	//не получилось сразу зацепиться, выяснить почему
	else
	{
		//поверхность не пригодна для лазанья
		if (!Mesh->Thorax.Floor.IsClimbable())
		{
			//попытаться все же подпрыгнуть одной из чстей тела
			if(SpineVector.Z < 0.7)
				ActionFindStart(EAction::RaiseFront);
			else ActionFindStart(EAction::RaiseBack);
		}
	}
}

//установить образ объекта в фокусе - нужно чтоб для демона вызвался через ИИ,
void AMyrPhyCreature::SetObjectAtFocus(FGestalt* GO){	if (Daemon)	Daemon->ObjectAtFocus = GO; }


//==============================================================================================================
//разовый отъём запаса сил - вызывается при силоёмких действиях / или восполнение запаса сил - в редком тике
//==============================================================================================================
void AMyrPhyCreature::StaminaChange(float delta)
{
	//если дельта больше нуля предполагается восстановление стамины
	//однако этому восстановлению мешает эмоциональное напряжение
	if (delta > 0)
	{	
		//для игра стамину снижают
		if(Daemon)
		{ 
			//сырость и сонность
			delta *= (1 - Daemon->Sleepiness);
			delta -= Daemon->Wetness;
		}

		//злоба и страх также отрубают восстановление сил
		delta -= (MyrAIController()->IntegralEmotion.fFear() * 0.4 + MyrAIController()->IntegralEmotion.fRage() * 0.2) * Phenotype.StaminaCurseByEmotion();


		//но всё же не отнимают сверх меры
		if (delta < 0) delta = 0;
	}

	// прирастить запас сил на дельту (или отнять), вычисленную извне
	Stamina += delta;								

	// сил и так нет, а от нас ещё требуют
	if (Stamina < 0)								
	{	Stamina = 0;								// восстановить ноль - сил как не было, так и нет
		Energy -= delta * 0.02f;					// а вот расход пищевой энергии всё равно происходит, да ещё сильнее
	}
	// запас сил имеется (или нашёлся)
	else											
	{
		if (Stamina > 1.0f) Stamina = 1.0f;			// запас сил и так восстановлен, дальше некуда
		else if (delta > 0)							// если же есть куда восстанавливать запас сил (происходит именно восстановление, не расход)...
			Energy -= delta * 0.01f;				// запас сил восстанавливается за счёт расхода пищевой энергии
	}
}

//==============================================================================================================
//после перемещения на ветку - здесь выравнивается камера - возможно, уже не нужен
//==============================================================================================================
void AMyrPhyCreature::PostTeleport()
{
	if(Daemon)
		Daemon->ResetCamera();
}

//==============================================================================================================
//восстановление здоровья - восстановление в медленном тике постоянно, отъём при уроне
//==============================================================================================================
void AMyrPhyCreature::HealthChange(float delta)
{
	if(delta>0)
	{
		float dH = delta * FMath::Sqrt(Energy);			// низкая энергия = голод = слабая регенерация, за одно не даст энергии прогнуться ниже нуля
		if(Daemon)
			if(Daemon->Sleepiness>0.5)
				dH -= Daemon->Sleepiness - 0.5;			// сонность
		Health += dH;									// приращение
		if (Health >= 1.0f) Health = 1.0f;				// полностью восстановленное здоровье не надо дополнительно восстанавливать
		else Energy -= dH * 0.01;						// отъем энергии для восстановления здоровья
	}
	else												//пока неясно, что еще делать при отъеме здоровья
	{	Health += delta;	}
}

//==============================================================================================================
//редкий тик, вызывается пару раз в секунду - проще всего выполнять на мощах ИИ-контроллера
//==============================================================================================================
void AMyrPhyCreature::RareTick(float DeltaTime)
{
	//исключить метаболизм параметров в мертвом состоянии
	if (Health <= 0)
	{	if (CurrentState != EBehaveState::dying && CurrentState != EBehaveState::dead)
			AdoptBehaveState(EBehaveState::dying, "Natural turn to dead, outside the behave machine");
		return;
	}

	//напрячь или расслабить спину
	if(Health < 0.2)
		Mesh->SetSpineStiffness(Mesh->DynModel->SpineStiffness * Health * 5);

	//учёт воздействия поверхности, к которой мы прикасаемся любой частью тела, на здоровье
	EMyrSurface CurSu = EMyrSurface::Surface_0;
	FSurfaceInfluence* SurfIn = nullptr;
	float SurfaceHealthModifier = 0.0f;
	for (int i = 0; i < (int)ELimb::NOLIMB; i++)
	{	auto& Limb = Mesh->GetLimb((ELimb)i);
		if(Limb.Stepped && Limb.Floor)
		{	
			//для избегания поиска того, что уже найдено на предыдущем шаге
			if(Limb.Floor.Surface != CurSu)
			{	SurfIn = GetMyrGameInst()->Surfaces.Find(Limb.Floor.Surface);
				CurSu = Limb.Floor.Surface;
			}
			//в сборке данных по поверхности указана дельта урона, поэтому с минусмом
			if(SurfIn) SurfaceHealthModifier -= SurfIn->HealthDamage;
		}
	}

	//восстановление здоровья (стамина перенесена в основной тик, чтоб реагировать на быструю смену фаз атаки)
	HealthChange ((Mesh->DynModel->HealthAdd + SurfaceHealthModifier ) * DeltaTime	);	

	//энергия пищевая тратится на эмоции, чем возбужденнее, тем сильнее
	//здесь нужен какой-то коэффициент, нагруженный параметром фенотипа
	Energy -= 0.001*DeltaTime*MyrAIController()->IntegralEmotion.UniPower();

	//пересчитать здоровье и мобильность тела
	UpdateMobility();

	//возраст здесь, а не в основном тике, ибо по накоплению большого числа мелкие приращения могут быть меньше разрядной сетки
	Age += DeltaTime;

	//очень редко + если на уровне присутствует небо и смена дня и ночи
	if (GetMyrGameMode()->Sky && GetMyrGameMode()->Sky->TimeOfDay.GetSeconds() < 3)
	{
		//разделка суток по периодам
		auto TimesOfDay = GetMyrGameMode()->Sky->MorningDayEveningNight();

	}

	//если повезёт, здесь высосется код связанный в блюпринтах
	RareTickInBlueprint(DeltaTime);

}


//==============================================================================================================
//полный спектр действий от (достаточно сильного) удара между этим существом и некой поверхностью
//==============================================================================================================
void AMyrPhyCreature::Hurt(FLimb& Limb, float Amount, FVector ExactHitLoc, FVector3f Normal, EMyrSurface ExactSurface, AMyrPhyCreature* Hitter)
{
	if(!Mesh->InjureAtHit)	{	UE_LOG(LogMyrPhyCreature, Log, TEXT("%s No Hurt %g at %s: Mesh.InjureAtHit disabled"),		*GetName(), Amount, *TXTENUM(ELimb, Limb.WhatAmI));	return;	}
	if(Limb.LastlyHurt)		{	UE_LOG(LogMyrPhyCreature, Log, TEXT("%s No Hurt %g at %s: limb already hit this frame"),	*GetName(), Amount, *TXTENUM(ELimb, Limb.WhatAmI));	return;	}
	if(Amount <= 0.01)		{	UE_LOG(LogMyrPhyCreature, Log, TEXT("%s No Hurt %g at %s: too little damage applied"),		*GetName(), Amount, *TXTENUM(ELimb, Limb.WhatAmI));	return;	}
	else						UE_LOG(LogMyrPhyCreature, Log, TEXT("%s DO Hurt %g at %s"),									*GetName(), Amount, *TXTENUM(ELimb, Limb.WhatAmI));

	//для простоты вводится нормаль, а уже тут переводится в кватернион, для позиционирования источника частиц
	FQuat4f ExactHitRot = FRotationMatrix44f::MakeFromX(Normal).ToQuat();

	//при сильном ударе членик выключается из прочих ударов на целый кадр, чтобы физдвижок не набрасивал касаний за кадр
	if (Amount > 0.3)
	{	MakeVoice(GenePool->SoundAtPain, FMath::Min(Amount, 1.0f) * 4, true);	// вскрикнуть (добавить парметры голоса)
		Limb.LastlyHurt = true;													// возможно, сразу отключать генерацию хита, а не этот жалкий флаг
	}
	
	//общее
	Limb.Damage += Amount;									// увеличить урон сразу здесь, ибо в иных случаях эта переменная не увеличивается
	Pain += Amount;											// привнести толику в общую боль
	CeaseActionsOnHit(Amount);								// возможно, некоторые действия следует прервать при данном уровне увечья

	//действия на уровне ИИ
	MyrAIController()->NotifyNoise(ExactHitLoc, Amount);							// прозвучать для всех просто как звуковой удар
	if (Hitter)																		// если удар нанесен кем-то
	{	Hitter->MyrAIController()->ExactVictim = Mesh;								// в поле жертвы явно задать, чтобы в ИИ по эвенту жертва поняла, что это она
		Hitter->MyrAIController()->LetOthersNotice(EHowSensed::PERCUTER, Amount);	// сгенерировать сообщение агенса атаки
		this -> MyrAIController()->LetOthersNotice(EHowSensed::PERCUTED, Amount);	// сгенерировать сообщение пациенса атаки
	}
	else MyrAIController()->LetOthersNotice(EHowSensed::HURT, Amount);				// сгенерировать сообщение объекта не атаки, но увечья


	//сборка данных по поверхности, которая осприкоснулась с этим существом
	FSurfaceInfluence* ExactSurfInfo = GetMyrGameInst()->Surfaces.Find(Limb.Floor.Surface);
	if (!ExactSurfInfo) return;

	//◘здесь генерируется всплеск пыли от шага - сразу два! - касание и отпуск, а чо?!
	auto DustAt = ExactSurfInfo->ImpactBurstHit; if (DustAt) SurfaceBurst (DustAt, ExactHitLoc, ExactHitRot, GetBodyLength(), Amount);
	DustAt = ExactSurfInfo->ImpactBurstRaise; 	 if (DustAt) SurfaceBurst (DustAt, ExactHitLoc, ExactHitRot, GetBodyLength(), Amount);
}

void AMyrPhyCreature::EnjoyAGrace(float Amount, AMyrPhyCreature* Sweetheart)
{	//MyrAI()->DesideHowToReactOnGrace(Amount, Sweetheart);
	UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s EnjoyAGrace from %s for %g"), *GetName(), *Sweetheart->GetName(), Amount);
}

//==============================================================================================================
//прервать или запустить прерывание деймтвий, для которых сказано прерывать при сильном касании
//==============================================================================================================
void AMyrPhyCreature::CeaseActionsOnHit(float Damage)
{
	//если выполняется самодействие или действие-период, но указано его отменять при ударе
	if (!NoRelaxAction())
		if (Damage > GetRelaxAction()->MinHitDamageToCease)
			RelaxActionStartGetOut();
	if(!NoSelfAction())
		if(Damage > GetSelfAction()->MinHitDamageToCease)
			SelfActionCease();
	if (DoesAttackAction())
	{	if (Damage > GetAttackAction()->MinHitDamageToCease)
			if(!(bClimb && NowPhaseToJump())) //если прыгаем из лазанья, нельзя, иначе снова притянет к опоре
				AttackEnd();
	}
}



//==============================================================================================================
//сделать шаг - 4 эффекта: реальный звук, ИИ-сигнал, облачко пыли, декал-след
//==============================================================================================================
void AMyrPhyCreature::MakeStep(ELimb eLimb, bool Raise)
{
	FLimb* Limb = &Mesh->GetLimb(eLimb);
	if(!Limb->Floor) return;
	auto Girdle = GetGirdle(eLimb);

	//тип поверхности - это не точно, так как не нога а колесо - пока неясно, делать трассировку в пол или нет
	EMyrSurface ExplicitSurface = Limb->Floor.Surface;
	auto Trans = Mesh->GetSocketTransform(Mesh->FleshGene(eLimb)->GrabSocket, RTS_World);

	//точное место шага - пока не ясно, стоит ли делать трассировку
	FVector ExactStepLoc = Trans.GetLocation();

	//сила шага, ее надо сложно рассчитывать, нужна чтоб мелкие животные не пылили
	float StepForce = 1.0f;

	//поворот для декала - тут все просто
	FQuat4f ExactStepRot = (FQuat4f)Trans.GetRotation();

	//поворот для источника частиц - вот тут нужен вектор скорости в направлении нормали
	FQuat4f ExactBurstRot = ExactStepRot;

	//расчёт скорости движущегося кончика ноги в точке шага
	FVector3f Vel; float VelScalar; FVector3f VelDir;
	FLimb* WorkingLimb = Limb;

	//если кинематический членик есть
	if (Mesh->HasFlesh(eLimb))
	{
		WorkingLimb = Limb;
		Vel = Mesh->BodySpeed(*Limb, 0);
		StepForce = FMath::Min(Mesh->GetFleshBody(*Limb, 0)->GetBodyMass(), 2.0f);
	}
	//если ничего нет по ноге, откуда можно выдрать физику
	else
	{
		WorkingLimb = &Girdle->GetLimb(EGirdleRay::Center);
		Vel = Mesh->BodySpeed(*WorkingLimb);
		StepForce = FMath::Min(Mesh->GetMachineBody(*Limb)->GetBodyMass(), 2.0f);
	}

	//если зафиксирована опора, вычесть ее скорость чтоб оносительное движение было
	if (WorkingLimb->Floor && WorkingLimb->Floor.IsMovable()) Vel -= (FVector3f)WorkingLimb->Floor.Speed(ExactStepLoc);
	if ((Vel | Limb->Floor.Normal) < 0) Vel = -Vel;
	Vel.ToDirectionAndLength(VelDir, VelScalar);

	//в качестве нормы берем скорость ходьбы этого существа, она должна быть у всех, хотя, если честно, тут уместнее твердая константа
	StepForce *= VelScalar / 200;


	//если слабый шаг, то брызги больше вверх, если шаг сильнее, то всё больше в сторону движения 
	ExactBurstRot = FMath::Lerp(
		FRotationMatrix44f::MakeFromX(Limb->Floor.Normal).ToQuat(),
		FRotationMatrix44f::MakeFromX(VelDir).ToQuat(),
		FMath::Min(StepForce, 1.0f));
	//DrawDebugLine(GetWorld(), ExactStepLoc, ExactStepLoc + Vel*0.1,	FColor(Raise?255:0, StepForce*255, 0), false, 2, 100, 1);


	//выдрать параметры поверхности из глобального контейнера (индекс хранится здесь, в компоненте)
	if(auto StepSurfInfo = GetMyrGameInst()->Surfaces.Find (ExplicitSurface))
	{
		//◘ вызвать звук по месту со всеми параметрами
		SoundOfImpact (StepSurfInfo, ExplicitSurface, ExactStepLoc,
			0.5f  + 0.05 * Mesh->GetMass()  + 0.001* Girdle->Speed,
			1.0f - Limb->Floor.Normal.Z,
			Raise ? EBodyImpact::StepRaise : EBodyImpact::StepHit);

		//◘ прозвучать в ИИ, чтобы другие существа могли слышать
		MyrAIController()->NotifyNoise(ExactStepLoc, StepForce);

		//только на максимальной детализации = минимальном расстоянии, для оптимизации 
		if (Mesh->GetPredictedLODLevel() == 0)
		{

			//◘здесь генерируется всплеск пыли от шага
			auto NiagaraDustAt = Raise ? StepSurfInfo->ImpactBurstRaise : StepSurfInfo->ImpactBurstHit;
			if(NiagaraDustAt) SurfaceBurst(NiagaraDustAt, ExactStepLoc, ExactStepRot, GetBodyLength(), StepForce);

			//отпечаток ноги виден только после поднятия ноги
			if (Raise)
			{
				//отпечатки ног - они уже не могут быть свойствами поверхности как таковой, они - свойства существа
				if (auto DecalMat = GetGenePool()->FeetDecals.Find(ExplicitSurface))
				{
					//выворот плоскости текстуры следа
					FQuat4f offsetRot(FRotator3f(-90.0f, 0.0f, 0.0f));
					FRotator3f Rotation = (ExactStepRot * offsetRot).Rotator();

					//◘ генерация компонента следа
					auto Decal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), *DecalMat, FVector(20, 20, 20), ExactStepLoc, (FRotator)Rotation, 10);
					Decal->SetFadeOut(0, 10, true);
					Decal->SetSortOrder(20);
				}
			}
		}
	}

}

//==============================================================================================================
//проиграть звук контакта с поверхностью / шаг, удар
//==============================================================================================================
void AMyrPhyCreature::SoundOfImpact(FSurfaceInfluence* StepSurfInfo, EMyrSurface Surface, FVector ExactStepLoc, float Strength, float Vertical, EBodyImpact ImpactType)
{
	//если функция вызывается извне, то доступ к сборке данных по поверхности может быть затруднён - тогда ввозится 0, и найти прямо здесь
	//если вызов из этого же класса, и сборка уже до вызова найдена, нет смысла ее еще раз искать - тогда вводится она сама
	if (!StepSurfInfo)
	{	StepSurfInfo = GetMyrGameInst()->Surfaces.Find(Surface);
		if (!StepSurfInfo) return;
	}

	//если в глобальном контейнере не завезли звук удара - то нет смысла продолжать функцию.
	auto SoundAt = ImpactType == EBodyImpact::StepRaise ? StepSurfInfo->SoundAtRaise : StepSurfInfo->SoundAtImpact;
	if (SoundAt)
	{
		//мутная функция по созданию источника звука ad-hoc
		auto Noiser = UGameplayStatics::SpawnSoundAttached(
			SoundAt, Mesh, NAME_None,
			ExactStepLoc, FRotator(),
			EAttachLocation::KeepWorldPosition, true,
			Strength, 1.0f, 0.0f,
			GetCurSoundAttenuation(), nullptr, true);

		//установить параметры скорости (влияет на громкость) и вертикальности (ступание-крабканье-скольжение)
		if (Noiser)
		{
			Noiser->SetIntParameter(TEXT("Impact"), (int)ImpactType);
			Noiser->SetFloatParameter(TEXT("Velocity"), Strength);
			Noiser->SetFloatParameter(TEXT("Vertical"), Vertical);
		}
		else UE_LOG(LogMyrPhyCreature, Error, TEXT("%s SoundOfImpact WTF no noiser type:%s, surf:%s"), *GetName(), *TXTENUM(EBodyImpact, ImpactType), *TXTENUM(EMyrSurface, Surface));
	}
	UE_LOG(LogMyrPhyCreature, Verbose, TEXT("%s SoundOfImpact type:%s, surf:%s"), *GetName(), *TXTENUM(EBodyImpact,ImpactType), *TXTENUM(EMyrSurface, Surface));
}

//==============================================================================================================
//новая версия с ниагарой
//==============================================================================================================
void AMyrPhyCreature::SurfaceBurst(UNiagaraSystem* Dust, FVector ExactHitLoc, FQuat4f ExactHitRot, float BodySize, float Scale)
{
	auto ParticleSystem = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Dust, ExactHitLoc, FRotator((FQuat)ExactHitRot), FVector(1, 1, 1), true);
	ParticleSystem->SetRenderCustomDepth(true);

	//параметр, определяемый размерами тела - размер спрайтов (хорошо бы еще размер области генерации)
	ParticleSystem->SetFloatParameter(TEXT("BodySize"), BodySize);

	//параметры силы удара - непрозрачность пыли, количество пылинок
	ParticleSystem->SetFloatParameter(TEXT("Amount"), Scale);
	ParticleSystem->SetFloatParameter(TEXT("VelocityScale"), FMath::Min(Scale, 1.0f));
	ParticleSystem->SetFloatParameter(TEXT("FragmentStartAlpha"), Scale);

	//брызги сильно отличаются на мокрой и схой поверхности, но для доступа к влажности нужен протагонист
	if (GetMyrGameMode())
		if (GetMyrGameMode()->Protagonist)
			ParticleSystem->SetFloatParameter(TEXT("Wetness"), GetMyrGameMode()->Protagonist->Wetness);

}

//==============================================================================================================
//проиграть новый звук (подразумевается голос и прочие центральные звуки)
//==============================================================================================================
void AMyrPhyCreature::MakeVoice(USoundBase* Sound, uint8 strength, bool interrupt)
{
	if (Sound)
	{
		if (!interrupt && CurrentSpelledSound != (EPhoneme)0)
		{	UE_LOG(LogMyrPhyCreature, Warning, TEXT("ACTOR %s MakeVoice not interrupted by %s"), *GetName(), *Sound->GetName());
			return;
		}
		CurrentSpelledSound = EPhoneme::S0;
		Voice->SetSound(Sound);
		Voice->SetIntParameter(TEXT("Strength"), strength);
		Voice->Play();
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s MakeVoice %s"), *GetName(), *Sound->GetName());
	}
}

//◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘
//==============================================================================================================
//принять новое состояние поведения (логически безусловно, но технически может что-то не сработать, посему буль)
//==============================================================================================================
bool AMyrPhyCreature::AdoptBehaveState(EBehaveState NewState, const FString Reason)
{
	//если мы уже в запрашиваеом состоянии - быстрый выход, сообщить ложь, чтобы вовне не прерывались
	if (CurrentState == NewState) return false;

	//если "карточки" данных для такого состояния в генофонде нет, то нельзя переходить
	auto BehaveNewData = GetGenePool()->BehaveStates.Find(NewState);
	if (!BehaveNewData)	return false;

	//нельзя принимать новое состояние, еси текущая скорость меньше минимальной, указанной
	if ((*BehaveNewData)->MinVelocityToAdopt > 0)
	if (FMath::Abs(Thorax->Speed) < (*BehaveNewData)->MinVelocityToAdopt)
	if (FMath::Abs(Pelvis->Speed) < (*BehaveNewData)->MinVelocityToAdopt)
	{	UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s AdoptBehaveState %s cancelled by min velocity"), *GetName(), *TXTENUM(EBehaveState, NewState));
		return false;
	}

	//сбросить счетчик секунд на состояние
	StateTime = 0.0f;
	UE_LOG(LogMyrPhyCreature, Log, TEXT("%s AdoptBehaveState  + (%s >> %s), \tReason: %s"), *GetName(),
		*TXTENUM(EBehaveState, CurrentState),
		*TXTENUM(EBehaveState, NewState), *Reason);

	//если включено расслабить всё тело
	if((*BehaveNewData)->MakeAllBodyLax != BehaveCurrentData->MakeAllBodyLax)
	Mesh->SetFullBodyLax((*BehaveNewData)->MakeAllBodyLax);

	//если текущий лод (не) подходит под условие полной кинематичности, и при этом текущие параметры не соответствуют новым
	bool NeedKin = (*BehaveNewData)->FullyKinematicSinceLOD <= Mesh->GetPredictedLODLevel();
	if (NeedKin != bKinematicMove) SetKinematic(NeedKin);

	//счастливый финал
	CurrentState = NewState;
	BehaveCurrentData = *BehaveNewData;

	//обновить скорость djccnfyj
	Mesh->HealRate = BehaveCurrentData->HealRate;

	//урон физическими ударами может быть включен или отключен
	Mesh->InjureAtHit = BehaveCurrentData->HurtAtImpacts;

	//если действие может только в отдельных состояниях и новое состояние в таковые не входит - тут же запустить выход
	if (DoesSelfAction())
	{	if (GetSelfAction()->Condition.StatesToCeaseAtTransit.Contains(NewState))
			SelfActionCease();
	}
	if (DoesAttackAction())
	{	if (GetAttackAction()->Condition.StatesToCeaseAtTransit.Contains(NewState))
			AttackEnd();
	}
	if (!NoRelaxAction())
	{	if (GetRelaxAction()->Condition.StatesToCeaseAtTransit.Contains(NewState))
			RelaxActionStartGetOut();
	}

	//применить новые настройки динамики движения/поддержки для частей тела поясов
	//ВНИМАНИЕ, только когда нет атаки, в атаках свои дин-модели по фазам, если применить, эти фазы не отработают
	if(NoAnyAction())
		AdoptWholeBodyDynamicsModel(&BehaveCurrentData->WholeBodyDynamicsModel);

	return true;
}

//==============================================================================================================
//основная логика движения - свитч с разбором режимов поведения и переходами между режимами
//==============================================================================================================
void AMyrPhyCreature::ProcessBehaveState(float DeltaTime)
{
	//увеличиваем таймер текущего состояния
	StateTime += DeltaTime;

	//разбор разных состояний
	switch (CurrentState)
	{
		//простой шаг по поверхности
		case EBehaveState::stand:
			BEWARE(fall,		!GotLandedAny(),							"fall if none of the 2 girdles has stand hardness 100+, i.e. at least one paw on floor");
			BEWARE(crouch,		bCrouch,									"simply obey the flag sent through drive from player or AI");
			BEWARE(soar,		bSoar && MoveGain > 0.1,					"obey the flag if WASD pressed or AI wants to move");
			BEWARE(run,			bRun && Stamina > 0.1f,						"obey the flag if we have forces enough to run");
			BEWARE(walk,		MoveGain > 0 || bRun || !GotLandedBoth(),	"WASD pressed OR we are too tired to run, OR we raise one of our girdles");
			BEWARE(pullup,		bClimb && Thorax->Climbing,					"hang on upper limbs first if we've clung to climb by the upper girdle");
			break;

		//шаг по поверхности (боимся упасть, опрокинуться и сбавить скорость до стояния)
		case EBehaveState::walk:
			BEWARE(fall,		Thorax->IsInAir() && Pelvis->IsInAir(),		"fall only if both girdles have lost their floors completely");
			BEWARE(stand,		MoveGain==0 && GotSlow(10),					"stop moving if our speed (ceased from drive) has fallen to 10cm/s");
			BEWARE(mount,		Thorax->Ascending && MoveGain>0.1,			"upper girdle found a steep, but mountable slope and set the flag");
			BEWARE(crouch,		bCrouch,									"simply obey the flag sent through drive from player or AI");
			BEWARE(soar,		bSoar && MoveGain > 0.1,					"obey the flag if WASD pressed or AI wants to move");
			BEWARE(lie,			GotLying() && Daemon && MoveGain<0.3,		"special conditions for lying, but NPC don't lie, and will to move must be low enough");
			BEWARE(tumble,		GotLying(),									"special conditions for lying, but trying to rise immediately, if it's NPC or move gain added");
			BEWARE(pullup,		bClimb && Thorax->Climbing,					"hang on upper limbs first if we've clung to climb by the upper girdle");
			BEWARE(sprint,		bRun && bCrouch && Stamina > 0.1f,			"ultrafast running, when both SHIFT and CTRL pressed, need walk or run first");
			BEWARE(run,			bRun && Stamina > 0.1f,						"obey the flag if we have forces enough to run");
			break;

		//шаг украдкой по поверхности
		case EBehaveState::crouch:
			BEWARE(fall,		!GotLandedAny(),							"fall if none of the 2 girdles has at least 1 paw on floor");
			BEWARE(soar,		bSoar && MoveGain > 0.1,					"obey the flag if WASD pressed or AI wants to move");
			BEWARE(pullup,		bClimb && Thorax->Climbing,					"hang on upper limbs first if we've clung to climb by the upper girdle");
			BEWARE(lie,			GotLying() && Daemon && MoveGain<0.3,		"special conditions for lying, but NPC don't lie, and will to move must be low enough");
			BEWARE(tumble,		GotLying(),									"special conditions for lying, but trying to rise immediately, if it's NPC or move gain added");
			BEWARE(sprint,		bRun && bCrouch && Stamina > 0.1f,			"ultrafast running, when both SHIFT and CTRL pressed, need walk or run first");
			BEWARE(run,			bRun && Stamina > 0.1f,						"obey the flag if we have forces enough to run");
			BEWARE(walk,		!bCrouch,									"obey the flag, immediately stop crouhing");	
			break;

		//бег-тыгыдык по поверхности
		case EBehaveState::run:
			BEWARE(fall,		!GotLandedAny(),							"fall if none of the 2 girdles has at least 1 paw on floor");
			BEWARE(walk,		GotSlow(10) || !bMove || !bRun,				"got running slow enough, or WASD unpressed, or simply the flag (SHIFT) cleared");
			BEWARE(lie,			GotLying() && Daemon && MoveGain<0.3,		"special conditions for lying, but NPC don't lie, and will to move must be low enough");
			BEWARE(tumble,		GotLying(),									"special conditions for lying, but trying to rise immediately, if it's NPC or move gain added");
			BEWARE(pullup,		bClimb && Thorax->Climbing,					"hang on upper limbs first if we've clung to climb by the upper girdle");
			BEWARE(sprint,		bRun && bCrouch && Stamina > 0.1f,			"ultrafast running, when both SHIFT and CTRL pressed, need walk or run first");
			break;

		//быстрый бег по поверхности
		case EBehaveState::sprint:
			BEWARE(fall,		!GotLandedAny(),							"fall if none of the 2 girdles has at least 1 paw on floor");
			BEWARE(walk,		Stamina<0.1f,								"if we are too tired to run in any manner, roll down to walking directly");
			BEWARE(walk,		GotSlow(10) || !bMove,						"got running slow enough (e.g. by bumping/sticking), or WASD unpressed");
			BEWARE(run,			bRun && !bCrouch,							"when crouch flag released, but run is still on, and above tiredness condition failed");
			BEWARE(lie,			GotLying() && Daemon && MoveGain<0.3,		"special conditions for lying, but NPC don't lie, and will to move must be low enough");
			BEWARE(tumble,		GotLying(),									"special conditions for lying, but trying to rise immediately, if it's NPC or move gain added");
			BEWARE(pullup,		bClimb && Thorax->Climbing,					"hang on upper limbs first if we've clung to climb by the upper girdle");
			break;

		//забраться на уступ
		case EBehaveState::mount:
			BEWARE(soar,		bSoar && MoveGain > 0.1,					"obey the flag if WASD pressed or AI wants to move");
			BEWARE(fall,		!GotLandedAny(),							"fall if none of 2 girdles has a paw on floor");		
			BEWARE(fall,		MoveGain < 0.1,								"fall if WASD not pressed, cuz mount only with force applied");		
			BEWARE(fall,		StateTime>2,								"fall if time over, prevent climbing-like endless mounting");		
			BEWARE(fall,		Stamina < 0.1,								"fall if stamina over");		
			BEWARE(pullup,		bClimb && Thorax->Climbing,					"hang on upper limbs first if we've clung to climb by the upper girdle");
			BEWARE(walk,		!Thorax->Ascending,							"when upper limb met a flat floor");
			BEWARE(run,			bRun && Stamina > 0.1f,						"obey the flag if we have forces enough to run");
			break;

		case EBehaveState::pullup:
			BEWARE(soar,		bSoar && MoveGain > 0.1,					"obey the flag if WASD pressed or AI wants to move");
			BEWARE(fall,		Thorax->StandHardness < 100,				"fall if none of 2 girdles has a paw on floor");
			BEWARE(fall,		!Thorax->Climbing,							"fall if upper limbs lost climbing base");
			BEWARE(climb,		Pelvis->Climbing,							"turn to full climbing if lower limbs have clung to base");
			break;

		//лазанье по проивольно крутым поверхностям
		case EBehaveState::climb:
			BEWARE(soar,		bSoar && MoveGain > 0.1,					"obey the flag if WASD pressed or AI wants to move");
			BEWARE(fall,		!GotLandedAny() || GotUnclung(),			"losing all girdles attachment considered as strating to fall even if limbs floors are sitll present");
			BEWARE(pullup,		!Pelvis->Climbing,							"turn to upper-only climbing if lower limbs lost the attachment, but upper still have");
			break;

		//набирание высоты (никаких волевых переходов, только ждать земли и перегиба скорости)
		case EBehaveState::soar:
			BEWARE(fly, 		!bSoar,										"obey the flag, usually unpressed button");
			BEWARE(run,			GotLandedAny(200) && bRun,					"any girdle landed on 2 paws while the run flag set means transition to running directly");
			BEWARE(crouch,		GotLandedAny(200),							"any girdle landed on 2 paws with no run flag set means transition to crouching to make legs bend low");
			BEWARE(project,		JumpTarget!=nullptr,						"go to kinematic moving to a target if this target suddenly appeared");
			break;
			
		//падение (никаких волевых переходов, только ждать земли)
		case EBehaveState::fall:
			BEWARE(soar,		bSoar,										"obey the flag");
			BEWARE(fly,			bFly,										"obey the flag");
			BEWARE(project,		JumpTarget != nullptr,						"obey the flag, suddenly jump target appeared");
			BEWARE(land,		StateTime>0.5 && !GotLandedAny(1),			"continuous falling for 0.5s with no walls touching causes transition to a `long falling` in slowmo");
			BEWARE(lie,			GotLying() && Daemon && MoveGain<0.3,		"special conditions for lying, but NPC don't lie, and move strength must be low enough");
			BEWARE(tumble,		GotLying(),									"special conditions for lying, but trying to rise immediately, if it's NPC or move gain added");
			BEWARE(run,			GotLandedAny(200) && bRun,					"any girdle landed on 2 paws while the run flag set means transition to running directly");
			BEWARE(crouch,		GotLandedAny(200),							"any girdle landed on 2 paws with no run flag set means transition to crouching to make legs bend low");
			break;

		//специальное долгое падение, которое включается после некоторрого времени случайного падения
		//на него вешается слоумо для выворота тела в полете
		case EBehaveState::land:
			BEWARE(soar,		bSoar,										"obey the flag");
			BEWARE(fly,			bFly,										"obey the flag");
			BEWARE(lie,			GotLying() && Daemon && MoveGain<0.3,		"special conditions for lying, but NPC don't lie, and move strength must be low enough");
			BEWARE(tumble,		GotLying(),									"special conditions for lying, but trying to rise immediately, if it's NPC or move gain added");
			BEWARE(run,			GotLandedAny(200) && bRun,					"any girdle landed on 2 paws while the run flag set means transition to running directly");
			BEWARE(crouch,		GotLandedAny(200),							"any girdle landed on 2 paws with no run flag set means transition to crouching to make legs bend low");
			break;

		//режим полёта
		case EBehaveState::fly:
			BEWARE(fall,		!bFly || Stamina*Health<0.1 || MoveGain==0,	"fly to fall: too tired and/or injured, or stop wingflapping, or just cleared the flag");
			BEWARE(soar,		bSoar,										"obey the flag");
			BEWARE(run,			GotLandedAny(200) && bRun,					"any girdle landed on 2 paws while the run flag set means transition to running directly");
			BEWARE(crouch,		GotLandedAny(200),							"any girdle landed on 2 paws with no run flag set means transition to crouching to make legs bend low");
			break;

		//режим движения в воздухе по траектории
		case EBehaveState::project:
			BEWARE(fall,		JumpTarget==nullptr,						"sudden loosing target cause immediate cancelling the trajectory to general phy. falling");
			BEWAREA(fall,		GotStandBoth(120), JumpTarget = nullptr,	"quick touching anything by at least one paw, cancels the projectile motion to general phy. falling");
			BEWAREA(crouch,		GotStandBoth(300), JumpTarget = nullptr,	"touching by at least 3 paws, cancels the projectile motion to landing crouching");
			BEWAREA(tumble,		GotLying(), JumpTarget = nullptr,			"touching with lying pose, cancels the projectile motion to lying active");
			break;

		//опрокидывание (пассивное лежание выход через действия а не состояния) 
		case EBehaveState::lie:
			BEWARE(fall,		!GotLandedAny(1),							"lying is fragile so only complete loosing all floors causes transition to falling");
			BEWARE(tumble,		ExternalGain > 0.2f,						"WASD pressed, start tumbling to rise");
			BEWARE(crouch,		!GotLying(),								"any lying conditions lost - turn to a ducked pose, i.e.crouch");
			break;

		//кувыркание, активная попытка встать со спины на ноги
		case EBehaveState::tumble:
			BEWARE(fall,		!GotLandedAny(5),							"tumbling is not so fragile, as lying, so threshold in stand hardness is higher");
			BEWARE(lie,			GotLying() && Daemon && MoveGain < 0.1f,	"passive lying if WASD released");
			BEWARE(crouch,		!GotLying(),								"any lying conditions lost - turn to a ducked pose, i.e.crouch");
			break;
	
		//здоровье достигло нуля, просто по рэгдоллиться пару секнуд перед окончательной смертью
		case EBehaveState::dying:
			BEWARE(dead, StateTime>2,										"dying longs 2sec, then any actions ceased, the creature considered dead");
			if(Stamina == -1) break;
			ActionFindStart(EAction::SELF_DYING1);
			CatchMyrLogicEvent(EMyrLogicEvent::SelfDied, 1.0f, nullptr);
			MyrAIController()->LetOthersNotice(EHowSensed::DIED);
			bClimb = 0;
			Health = 0.0f;
			Stamina = -1;
			Pain = 0.0f;
			Thorax->DetachFromBody();
			Pelvis->DetachFromBody();
			break;

		//окончательная смерть с окоченением, поджиманием лапок
		case EBehaveState::dead:
			break;

		//схвачен, удерживается в руках или зубах
		case EBehaveState::held:
			break;

	}

}


//==============================================================================================================
//начать атаку (внимание, сложных проверок нет, применимость атаки должна проверяться заранее)
//==============================================================================================================
EResult AMyrPhyCreature::AttackActionStart(int SlotNum, USceneComponent* ExactVictim)
{
	//в генофонде ни одна атака не заполнена на кнопку, из которой вызвалась эта функция
	if (SlotNum == 255) return EResult::WRONG_PHASE;//◘◘>;

	//даже если атака не начнётся (а она в этом случае обязательно не начнётся)
	//но в ней прописано, что в начальной фазе надо съесть то, что мы держим
	if (GenePool->Actions[SlotNum]->VictimTypes[0].EatHeldPhase == EActionPhase::ASCEND)
		if (Eat()) return EResult::ATE_INSTEAD_OF_RUNNING;//◘◘> 

	//уже выполняется какая-то атака, нельзя просто так взять и прервать её
	if (CurrentAttack != 255) return EResult::ALREADY_RUNNING;//◘◘>

	//полная связка настроек
	auto Action = GenePool->Actions[SlotNum];
	auto& VictimMod = Action->VictimTypes[0];

	//отправить просто звук атаки, чтобы можно было разбегаться (кстати, ввести эту возможность)
	MyrAIController()->LetOthersNotice(EHowSensed::ATTACKSTART);

	//для ИИ важно пометить цель атаки, если сам ИИ инициирует атаку, он знает, на кого замахивается
	MyrAIController()->ExactVictim = ExactVictim;

	//теперь эта атака точно разрешена к началу, врубить атаку текущей
	CurrentAttack = SlotNum;
	AttackChangePhase(EActionPhase::ASCEND);

	//для игрока
	if (Daemon)
	{
		//показать в худе имя начатой атаки
		Daemon->HUDOfThisPlayer()->OnAction(0, true);

	}
	//для непися, которого мы имеем в целях ИИ = значит боремся или просто стоим около, тогда спецеффекты его атак также должны влиять на нас
	else if(GetMyrGameMode()->GetProtagonist()->MyrAIController()->MeInYourMemory(this))
	{
		//если атака требует размытия, то в любом случае находим протагониста и рамываем у него
	}

	//обнаружить, что мы пересекаем умный объём и передать ему, что мы совершаем, возможно, судьбоносное действие
	if (VictimMod.SendApprovalToTriggerOverlapperOnStart)
		SendApprovalToOverlapper();

	//здесь фиксируется только субъективный факт атаки, применения силы, а не результат
	if (VictimMod.TactileDamage>0)
		 CatchMyrLogicEvent ( EMyrLogicEvent::SelfHarmBegin,  VictimMod.TactileDamage, nullptr);
	else CatchMyrLogicEvent ( EMyrLogicEvent::SelfGraceBegin,-VictimMod.TactileDamage, nullptr);
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s AttackStart %s"), *GetName(), *Action->GetName());
	return EResult::STARTED;//◘◘>
}

//==============================================================================================================
// вызвать переход атаки из подготовки в удар (внимание, проверок нет, применимость атаки должна проверяться заранее)
//==============================================================================================================
EResult AMyrPhyCreature::AttackActionStrike()
{
	//если просят применить атаку, а ее нет
	if (CurrentAttack == 255) return EResult::WRONG_PHASE;//◘◘>

	//полная связка настроек
	auto Action = GenePool->Actions[CurrentAttack];
	auto& OnVictim = Action->VictimTypes[0];

	//в зависимости от того, в какой фазе вызвана просьба свершить действие
	switch (CurrentAttackPhase)
	{
		//подготовка еще не завершена, отправить на ожидание, атака свершится по закладке
		case EActionPhase::ASCEND:
			AttackChangePhase(EActionPhase::RUSH);
			return EResult::RUSHED_TO_STRIKE;//◘◘>

		//подготовка завершена, можно вдарить сразу
		case EActionPhase::READY:
			return AttackActionStrikePerform();//№№№◘◘>

		//переудар поверх удара - пока есть бюджет повторов, отправить вспять, чтобы ударить повторно
		case EActionPhase::STRIKE:
			if (OnVictim.NumInstantRepeats > CurrentAttackRepeat)
			{	AttackChangePhase(EActionPhase::DESCEND);
				return EResult::GONE_TO_STRIKE_AGAIN;//◘◘>
			}

		//остальные фазы не приводят к смене
		case EActionPhase::RUSH:
		case EActionPhase::DESCEND:
			return EResult::WRONG_PHASE;//◘◘>
	}
	//по умолчанию непонятно отчего не получилось
	return EResult::FAILED_TO_STRIKE;//◘◘>
}

//==============================================================================================================
//непосредственное применение атаки - переход в активную фазу
//==============================================================================================================
EResult AMyrPhyCreature::AttackActionStrikePerform()
{
	//полная связка настроек
	auto Action = GenePool->Actions[CurrentAttack];
	auto& OnVictim = Action->VictimTypes[0];

	//уведомить мозги и через них ноосферу о свершении в мире атаки
	if (MyrAIController()) MyrAIController()->LetOthersNotice(EHowSensed::ATTACKSTRIKE);

	//перевести в фазу боя (внимание, если атака-прыжок, то подбрасывание тела здесь, внутри)
	AttackChangePhase(EActionPhase::STRIKE);

	//для непися, которого мы имеем в целях ИИ = значит боремся или просто стоим около, тогда спецеффекты его атак также должны влиять на нас
	if (GetMyrGameMode()->GetProtagonist()->MyrAIController()->MeInYourMemory(this))
	{
		//если атака требует размытия, то в любом случае находим протагониста и рамываем у него
		//GetMyrGameMode()->Protagonist->GetMyrCamera()->SetMotionBlur(Action->MotionBlurBeginToStrike);
	}


	//обнаружить, что мы пересекаем умный объём и передать ему, что мы совершаем, возможно, судьбоносное действие
	if (OnVictim.SendApprovalToTriggerOverlapperOnStrike)	SendApprovalToOverlapper();

	//статистика по числу атак
	GetMyrGameInst()->Statistics.AttacksMade++;

	//диагностика
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s AttackActionStrikePerform %s"), *GetName(), *Action->HumanReadableName.ToString());
	return EResult::STROKE;//◘◘>
}

//==============================================================================================================
//переключить фазу атаки со всеми сопутствующими
//==============================================================================================================
void AMyrPhyCreature::AttackChangePhase(EActionPhase NewPhase)
{
	//присвоить
	CurrentAttackPhase = NewPhase;

	//полная связка настроек
	auto Action = GenePool->Actions[CurrentAttack];
	auto& OnVictim = Action->VictimTypes[0];

	//не уровне меша принять приготовления атакующих / опасных частей тела
	for (ELimb li = ELimb::PELVIS; li != ELimb::NOLIMB; li = (ELimb)((int)li+1))
	{
		//для текущей новой фазы определить, вписана ли текущая часть тела как боевая или нет
		//во время атаки важно иметь непрерывные траектории, чтобы не проходить через тела и не вызывать распидорашивание
		//аналогично при падении важно не пробить землю на большой скорости
		const bool NeedBetterPhy = IsLimbAttacking(li) || IS_MACRO(CurrentState,AIR);
		if (Mesh->HasPhy(li))
			Mesh->GetMachineBody(li)->SetUseCCD(NeedBetterPhy);

		if (Mesh->HasFlesh(li) && Mesh->GetFleshBody(li)->bSimulatePhysics)
			Mesh->GetFleshBody(li)->SetUseCCD(NeedBetterPhy);
	}

	//текущая новая фаза - отцепить то, что зацеплено было
	if(OnVictim.UngrabPhase == CurrentAttackPhase) UnGrab();

	//настроить физику членов в соответствии с фазой атаки (UpdateMobility внутри)
	AdoptWholeBodyDynamicsModel(&Action->DynModelsPerPhase[(int)CurrentAttackPhase]);

}

//==============================================================================================================
//реакция на достижение закладки GetReady в анимации любым из путей - вызывается только в этой самой закладке
//==============================================================================================================
void AMyrPhyCreature::AttackNotifyGetReady()
{
	//идём штатно с начала атаки - сменить режим
	if( CurrentAttackPhase == EActionPhase::ASCEND)
		AttackChangePhase(EActionPhase::READY);

	//если вернулись в точку принятия решения вспять, на режиме DESCEND, это мы хотим немедленно повторить атаку
	else if(CurrentAttackPhase == EActionPhase::DESCEND)
	{
		//только если данный тип атаки позволяет делать дополнительные повторы в требуемом количестве
		//иначе так и остаться на режиме DESCEND до самого края анимации
		if(GetAttackActionVictim().NumInstantRepeats > CurrentAttackRepeat)
		{	AttackActionStrikePerform();
			CurrentAttackRepeat++;
		}
	}

	//если атака одобрена заранее, мы домчались до порога и просто реализуем атаку
	else if (CurrentAttackPhase == EActionPhase::RUSH) AttackActionStrikePerform ();
}

//==============================================================================================================
//полностью завершить атаку
//==============================================================================================================
void AMyrPhyCreature::AttackEnd()
{
	//если ваще нет атаки, то какого оно вызвалось?
	if(NoAttack())
	{	UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s AttackEnd WTF no attack"), *GetName());
		return;	}

	//аварийный сброс всех удерживаемых предметов, буде таковые имеются
	UnGrab();

	//для игрока, несущего интерфейс
	if (Daemon)
	{	Daemon->HUDOfThisPlayer()->OnAction(0, false);	// удалить в худе имя начатой атаки
		Daemon->AutoAim = false;						// если было автонацеливание, сбросить, иначе вектор атаки не будет следовать за камерой
	}

	//финально сбросить все переменные режима
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s AttackEnd %s "), *GetName(), *GetCurrentAttackName());
	CurrentAttackPhase = EActionPhase::NONE;
	CurrentAttack = 255;
	CurrentAttackRepeat = 0;

	//безусловно удалить из ИИ указатель на цель, так как цели без текущей атаки быть не может
	MyrAIController()->ExactVictim = nullptr;

	//снять дин-модель, данную этой завершенной атакой, в пользу нижележащей (обычно это из behavestate)
	AdoptWholeBodyDynamicsModel(GetPriorityModel());

}

//==============================================================================================================
//мягко остановить атаку, или исчезнуть, или повернуть всяпять - хз где теперь используется
//==============================================================================================================
void AMyrPhyCreature::AttackGoBack()
{
	if(NoAttack()){	UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s AttackGoBack WTF no attack"), *GetName());	return;	}

	//если финальный этап, то просто исчезнуть атаку
	if(CurrentAttackPhase == EActionPhase::FINISH)
	{	AttackEnd(); return; }

	//отход назад
	else AttackChangePhase(EActionPhase::DESCEND);
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s AttackGoBack %s "), *GetName(), *GetCurrentAttackName());

}
//==============================================================================================================
// атаки - отменить начинаемое действие, если еще можно
// для ИИ - чтобы прерывать начатую атаку, например, при уходе цели из виду - в любое время до фазы STRIKE (ASCEND, READY, RUSH)
// для игрока - по достижении закладки в ролике анимации, когда игрок зажал кнопку, но долго думает с отпусканием (READY)
//==============================================================================================================
bool AMyrPhyCreature::AttackNotifyGiveUp()
{
	//смысл точки - решение здаться, когда истекает ожидание (READY), в остальных фазах закладка не имеет смысла
	if(CurrentAttackPhase != EActionPhase::READY) return false;
	AttackChangePhase(EActionPhase::DESCEND);
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s AttackNotifyGiveUp %d "), *GetName(), (int)CurrentAttack);
	return true;
}

//==============================================================================================================
//когда между активно фазой и концом есть еще период безопасного закругления, и метка этой фазытолько получена
//==============================================================================================================
void AMyrPhyCreature::AttackNotifyFinish()
{
	//если до этого атака была в активной фазе, то финиш - это ее логичное окончание
	if(CurrentAttackPhase == EActionPhase::STRIKE)
	{
		//перевести ее в фазу финального закругления
		AttackChangePhase(EActionPhase::FINISH);
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s AttackNotifyFinish %s "),
			*GetName(), *GetCurrentAttackName());
	}
	else
	{
		UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s AttackNotifyFinish %s WTF wrong state, phase = %s"),
		*GetName(), *GetCurrentAttackName(), *TXTENUM(EActionPhase, CurrentAttackPhase));
		AttackEnd();
	}
}

//==============================================================================================================
//достижение финальной закладки
//==============================================================================================================
void AMyrPhyCreature::AttackNotifyEnd()
{
	//приезжаем к закладке "конец ролика", небудучи в правильном состоянии
	if (CurrentAttackPhase != EActionPhase::STRIKE && CurrentAttackPhase != EActionPhase::FINISH)
		UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s AttackNotifyEnd %s WTF wrong state, phase = %s"),
		*GetName(), *GetCurrentAttackName(), *TXTENUM(EActionPhase, CurrentAttackPhase));
	AttackEnd();
}

//==============================================================================================================
//общая процедура для всех видов прыжка, корректно закрывающая предыдущее состояние
//==============================================================================================================
bool AMyrPhyCreature::JumpAsAttack(FVector3f ExplicitVel)
{
	//скорость теперь считается заранее на этапе поиска целей
	if (ExplicitVel.Z < 1)													// если скорость нулевая - устали или ранены
		ActionFindStart(EAction::COMPLAIN_TIRED);					// выказать невозможность прыгнуть
	else
	{
		Thorax->PhyPranceDirect(ExplicitVel);								// подбросить передок		
		Pelvis->PhyPranceDirect(ExplicitVel);								// подбросить задок
		CatchMyrLogicEvent(EMyrLogicEvent::SelfJump, AttackForceAccum, 0);	// повлияет на геймплей
	}
	AttackForceAccum = 0;													// сбросить счетчик накопленной силы
	return (ExplicitVel.Z < 1);

}

//==============================================================================================================
// для каждого кадра искать положение самой удобной цели прыжка
//==============================================================================================================
FVector AMyrPhyCreature::PrepareJumpVelocity(FVector3f& jV)
{
	float Time = 0;													//отценка времени полета
	FVector jA = Mesh->GetComponentLocation();						//начальная точка, место старта, обычно задние ноги, то есть СК мэша
	FVector jB = jA + FVector(AttackDirection) * SpineLength * 5;	//конечная точка, определяется позицией цели или просто впереди на расстоянии
	auto jDM = FindJumpDynModel();									//найти дин-модель в текущем действии, которая отвечает за саму физику прыжка
	if (jDM)														//только если есть, если нет, прыжок не состоится
	{
		//пересчитать подвижность безотносительно тяги(которой может не быть если WASD не нажаты)
		UpdateMobility(false);

		//влияние прокачки на силу прыжка
		float JumExperience = jDM->GetJumpAlongImpulse() < 0 ?	Phenotype.JumpBackForceFactor() : Phenotype.JumpForceFactor();

		//случай самодействий с внезапным прыжком без подготовительной фазы - пусть прыжок будет сразу сильным, но не очень
		if (AttackForceAccum == 0) AttackForceAccum = 0.7;

		//множитель скорости прыжка: время прижатия кнопки * моторное здоровье * уровень навыка прыжков
		float JumpStrength = AttackForceAccum * MoveGain * JumExperience;							

		//перебрать всё, с чем пересекаемся, и выбрать самый ближний и передний
		uint8 BestOvI = FindMoveTarget(jDM->GetJumpAlongImpulse() * 2, 0.5f);

		//найдена точная цель для прыжка - направленный расчитанный прыжок точно в ценль
		if (BestOvI < 255 && JumpStrength > 0)
		{	Time = 2;
			jB = OverlapByIndex(BestOvI)->GetComponentLocation();
			jV = FindBallisticStartVelocity(jB, jA, JumpStrength * jDM->GetJumpUpImpulse(), JumpStrength * jDM->GetJumpAlongImpulse(), Time);
			if(jV.ContainsNaN())
				UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s PrepareJumpVelocity WTF V nan V=%g/%g, t=%g"),
					*GetName(), JumpStrength * jDM->GetJumpAlongImpulse(), JumpStrength * jDM->GetJumpUpImpulse(), Time);

			//нулевая скорость - прыжок не состоится
			if (jV.IsNearlyZero()) return jA;
		}

		//функция выдала положительное время = нашлись адекватные корни
		if (Time > 0)
		{
			//подготовить режим прямой цели, 
			Pelvis->SetCachedPreJumpPos();
			Thorax->SetCachedPreJumpPos();
			PackJumpStartVelocity(jV);
			JumpTarget = OverlapByIndex(BestOvI);
		}
		//целей для прыжка поблизости нет - свободный прыжок, сотканный из горизонтальной и вертикальной скорости
		else
		{	//если посчитанный импульс существенен - полностью родить начальную скорость свободного прыжка
			if (JumpStrength > 0)
			{	FVector3f OutDirXY = FVector3f::VectorPlaneProject(AttackDirection, Mesh->Pelvis.Floor.Normal).GetSafeNormal();
				float Vxy = JumpStrength * jDM->GetJumpAlongImpulse();
				float Vz = JumpStrength * jDM->GetJumpUpImpulse();
				jV = OutDirXY * Vxy + FVector3f(0, 0, 1) * Vz;
			}
			JumpTarget = nullptr;
		}

		//сдвинуть начало трассировки траектории вверх условно до головы, чтобы не захватывать пол на старте, а только на конце
		float OffsetTime = SpineLength / jDM->GetJumpUpImpulse();
		FVector OffsetOnTraj = PosOnBallisticTrajectory(jV, jA, OffsetTime);

		//теперь предложенную траекторию хорошо бы проверить 
		//зарядка встроенной системы трассировки по траектории, очень медленно, 
		FPredictProjectilePathParams PaParam(0, OffsetOnTraj, FVector(jV), 2);
		PaParam.ActorsToIgnore.Add(this);
		PaParam.DrawDebugType = EDrawDebugTrace::None;
		PaParam.ProjectileRadius = 0;
		PaParam.bTraceWithCollision = 0;
		PaParam.SimFrequency = 5;
		PaParam.bTraceWithCollision = true;
		PaParam.bTraceWithChannel = false;
		FPredictProjectilePathResult PaResul;
		UGameplayStatics::PredictProjectilePath(GetWorld(), PaParam, PaResul);

		//если на пути встретилась преграда, пометить конечной точкой именно ее
		if (PaResul.HitResult.bBlockingHit)
		{	jB = PaResul.HitResult.Location;
			Time = PaResul.HitResult.Time;
		}
		//если траектория ушла в воздух, пометить просто максимально пройденный параболой путь
		else
		{	jB = PaResul.LastTraceDestination.Location;
			Time = PaResul.LastTraceDestination.Time;
		}

		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s PrepareJumpVelocity V=%s, pB=%s, t=%g"), *GetName(), *jV.ToString(), *jB.ToString(), Time);

		//показывать траекторию надо только главному герою, (хотя хорошо бы и для его ближайших противников)
		if (Daemon)	Daemon->ShowJumpTrajectory(OffsetOnTraj, (FVector)jV, Time, jB);

		//вернуть правильное значение для подвижности с включённой внешнея тягой
		MoveGain *= ExternalGain;

		//выдать конечную точку траектории
		return jB;
	}
	return jA;
}

//==============================================================================================================
//отпустить всё, что имелось активно схваченным (в зубах)
//==============================================================================================================
void AMyrPhyCreature::UnGrab()
{
	//не ясно, как, перебирать все конечности и вообще как правильно сигнализировать о наличии хвата
	auto Released = Mesh->UnGrabAll();

	//послать на уровень глобального замысла сигнал, что именно этот объект был отпущен/потерян
	if(Released) CatchMyrLogicEvent(EMyrLogicEvent::ObjUnGrab, 1.0f, Released);
}

//==============================================================================================================
//взять из генофонда текстуру, соответствующую номеру и перекрасить материал тушки
//==============================================================================================================
void AMyrPhyCreature::SetCoatTexture(int Num)
{
	//если в генофонде прописан список расцветок, и номер соответствует какой-то текстуре в этом списке
	if (GenePool->StaticCoatTextures.Num() > Num)
	{
		//создать рычаг для изменения материала реального тела
		auto MainMeshPart = GetMesh()->GetMaterialIndex(TEXT("Body"));
		if (MainMeshPart == INDEX_NONE) MainMeshPart = 0;
		UMaterialInterface* Material = GetMesh()->GetMaterial(MainMeshPart); 
		UMaterialInstanceDynamic* matInstance = GetMesh()->CreateDynamicMaterialInstance(MainMeshPart, Material);
		matInstance->SetTextureParameterValue(TEXT("Coat"), GenePool->StaticCoatTextures[Num]);

		//посмотреть, может пользователем в редакторе добавлены другие компоненты-шкуры скелетного типа - шерсть, волосы
		for (auto& C : GetComponents())
		{	if(C->IsA(USkeletalMeshComponent::StaticClass()))
			{ 
				USkeletalMeshComponent* sC = (USkeletalMeshComponent*)C;
				UMaterialInterface* fMaterial = sC->GetMaterial(0);
				UMaterialInstanceDynamic* fmatInstance = sC->CreateDynamicMaterialInstance(0, fMaterial);
				fmatInstance->SetTextureParameterValue(TEXT("Coat"), GenePool->StaticCoatTextures[Num]);
			}
		}
	}
}

//==============================================================================================================
//в процессе сохранения игры сохранить инфу по этому кокретному существу
//==============================================================================================================
void AMyrPhyCreature::Save(FCreatureSaveData& Dst)
{
	//скопировать позицию и поворот 
	Dst.Transform = Mesh->GetComponentTransform();

	//возможно, поднять над землей, чтоб не вмуровался
	Dst.Transform.SetLocation(Dst.Transform.GetLocation() + FVector(0, 0, 10));

	//сохранить класс внутренний существа
	Dst.CreatureClass = GetClass();
		
	Dst.Phenotype = Phenotype;
	Dst.Damages.SetNum((int)ELimb::NOLIMB);
	for(int i=0; i<(int)ELimb::NOLIMB; i++)
		Dst.Damages[i] = Mesh->GetLimb((ELimb)i).Damage;
	Dst.Stamina = Stamina;
	Dst.Health  = Health;
	Dst.Energy  = Energy;
	Dst.EmoResource = MyrAIController()->EmoResource;

	//сохранение памяти ИИ
	for (auto Gestalt : MyrAIController()->Memory)
	{
		//перевод указателя в имя актора
		FSocialMemory Tuple;
		if(Gestalt.IsCreature())
			Tuple.Key = Gestalt.Creature()->GetFName();
		else
			Tuple.Key = Gestalt.Obj->GetFName();
		Tuple.Value = Gestalt;
		Tuple.Value.Obj = nullptr;
		Dst.Memory.Add(Tuple);
	}
	Dst.Mich = MyrAIController()->Mich;

	//определить, какая шкурка используется в основном меше
	auto MainMeshPart = GetMesh()->GetMaterialIndex(TEXT("Body"));
	if (MainMeshPart == INDEX_NONE) MainMeshPart = 0;
	UMaterialInterface* Material = GetMesh()->GetMaterial(MainMeshPart);

	UTexture* tiq;
	Material->GetTextureParameterDefaultValue(TEXT("Coat"), tiq);
	for(int i = 0; i < GenePool->StaticCoatTextures.Num(); i++)
	{
		if (tiq == GenePool->StaticCoatTextures[i]) Dst.Coat = i;
	}
}

//==============================================================================================================
//в процессе загрузки игры загрузить инфу по этому кокретному существу
//==============================================================================================================
void AMyrPhyCreature::Load(const FCreatureSaveData& Src)
{
	//тупо скопировать все параметры касаемо роли
	Phenotype = Src.Phenotype; 

	//скаляры
	Stamina = Src.Stamina;
	Health = Src.Health;
	Energy = Src.Energy;
	for(int i=0; i<(int)ELimb::NOLIMB; i++)
		Mesh->GetLimb((ELimb)i).Damage = Src.Damages[i];

	SetCoatTexture(Src.Coat);

	//непосредственно поместить это существо в заданное место
	TeleportToPlace(Src.Transform);

	//загрузка ИИ
	MyrAIController()->EmoResource = Src.EmoResource;
	MyrAIController()->Mich = Src.Mich;

	//загрузка воспоминаний с ссылками на объекты
	for(auto &Meme : Src.Memory)
		if(Meme.Value.Obj.IsValid())
			MyrAIController()->Memory.Add(Meme.Value);
}
//==============================================================================================================
//обыскать слот сохранения и проставить во всех ссылках на это существо (в памятях других существ) реальный указатель
//==============================================================================================================
void AMyrPhyCreature::ValidateSavedRefsToMe()
{
	if (!GetMyrGameInst()) return;
	if (!GetMyrGameInst()->JustLoadedSlot) return;
	for (auto Cre : GetMyrGameInst()->JustLoadedSlot->AllCreatures)
	{	for (auto Ges : Cre.Value.Memory)
		{	if (Ges.Key == GetFName())
				Ges.Value.Obj = GetMesh();
		}
	}
}

//==============================================================================================================
//отражение действия на более абстрактонм уровне - прокачка роли, эмоция, сюжет
//вызывается из разных классов, из ИИ, из триггеров...
//==============================================================================================================
void AMyrPhyCreature::CatchMyrLogicEvent(EMyrLogicEvent Event, float Param, UObject* Patiens, FMyrLogicEventData* ExplicitEmo)
{
	if (!GenePool) return;						//нет генофонда вообще нонсенс

	//для начала инструкции по обслуживанию события должны быть внутри
/*	auto EventInfo = MyrAI()->MyrLogicGetData(Event);
	if (!EventInfo) EventInfo = ExplicitEmo;
	if (EventInfo)
	{
		//большая часть аргументов - это компоненты с которыми взаимодействие, но может быть и нет
		auto Goal = Cast<UPrimitiveComponent>(Patiens);

		//ну уровне ИИ изменяем эмоции (внутри он найдёт цель и положит событие в память эмоциональных событий)
		if (MyrAI()) MyrAI()->RecordMyrLogicEvent(Event, Param, Goal, EventInfo);

		//прокачка навыков - для всех указанных в сборке навыков, на которые влияет это событие
		for (auto& i : EventInfo->ExperienceIncrements)
		{	Phenotype.ByEnum(i.Key).AddExperience(i.Value);
			if (Daemon)
				if (auto Wid = Daemon->HUDOfThisPlayer())
					Wid->OnExpGain(i.Key, Phenotype.ByEnum(i.Key).GetStatus());
		}
	}
	UE_LOG(LogMyrPhyCreature, Warning, TEXT("ACTOR %s CatchMyrLogicEvent %s %g"), *GetName(), *TXTENUM(EMyrLogicEvent, Event), Param);
*/
	//подняться на уровень сюжета и посмотреть, мож тригернет продвижение истории
	GetMyrGameInst()->MyrLogicEventCheckForStory(Event, this, Param, Patiens);

}


//◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘
//==============================================================================================================
//подали на вход только идентификатор действия, а что это за действие неизвестно, так что нужно его поискать во всех списках и запустить по своему
//==============================================================================================================
EResult AMyrPhyCreature::ActionFindStart(EAction Type)
{
	//особые действия, которые не сводятся к вызову ресурса, сводятся, в данном случае, к установке режима (или замене)
	auto R = EResult::STARTED;
	switch (Type)
	{
		case EAction::Move:									return R;//◘◘>
		case EAction::Run:		bRun =		1;				return R;//◘◘>
		case EAction::Crouch:	bCrouch =	1;				return R;//◘◘>
		case EAction::Fly:		bFly = 1;	bSoar = 0;		return R;//◘◘>
		case EAction::Soar:		bSoar = 1;					return R;//◘◘>
		case EAction::Walk:		bRun=bCrouch=bSoar=bFly=0;	return R;//◘◘>
		case EAction::Climb:	bClimb = 1;					return R;//◘◘>
	}

	/////////////////////////////////////////////////////////////////////////
	//единый список
	/////////////////////////////////////////////////////////////////////////
	R = EResult::INCOMPLETE_DATA;

	//массив действий с одинаковым ярлычком, найденный через карту
	TArray<uint8> SameActions;

	//найти все действия с данным ярлычком
	GenePool->ActionMap.MultiFind(Type, SameActions, true);

	//если хоть одно нашли
	if (SameActions.Num()>0)
	{	
		//ячейка с индексом лучшего действия в массиве
		uint8 BestAction = SameActions[0];

		//сразу протестировать применимость на себя первого действия в охапке найденных
		R = GenePool->Actions[BestAction]->IsActionFitting(this, false);
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s ActionFindStart %s found %d"), *GetName(), *TXTENUM(EAction, Type), SameActions.Num());
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s ActionFindStart step %d, %s, result %s"), *GetName(),
			0, *GenePool->Actions[SameActions[0]]->HumanReadableName.ToString(), *TXTENUM(EResult, R));

		//поиск лучшего в охапке - начиная со второго (а скорее всего второго и нет)
		for (int i = 1; i < SameActions.Num(); i++)
		{
			//если другое такое же действие менее затратно
			//ИЛИ ЖЕ если действие более затратно, однако ранее рассмотренное действие не подходит по общим критериям
			if (GenePool->Actions[SameActions[i]]->Better(GenePool->Actions[BestAction]) || !ActOk(R))
			{
				//проверить применимость, если ОК, то  - итая или дешевле, или одна лишь подходит, или всё вместе
				auto R2 = GenePool->Actions[SameActions[i]]->IsActionFitting(this, false);
				if (ActOk(R2))
				{	R = R2;
					BestAction = SameActions[i];
				}
				UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s ActionFindStart step %d, %s, result %s"), *GetName(),
					i, *GenePool->Actions[SameActions[i]]->HumanReadableName.ToString(), *TXTENUM(EResult, R2));
			}
		}

		//если в конечном счёте удалось найти пригодное действие
		if (ActOk(R)) R = ActionStart (BestAction);
	}
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s ActionFindStart %s %s"), *GetName(),
		*TXTENUM(EAction, Type), *TXTENUM(EResult,R));
	return R;//◘◘>
}


//==============================================================================================================
//завершить (или наоборот вдарить после подготовки) конкретное действие
//==============================================================================================================
EResult AMyrPhyCreature::ActionFindRelease(EAction Type, UPrimitiveComponent* ExactGoal)
{
	//отсев действий, не являющихся атаками
	auto R = EResult::STARTED;
	switch (Type)
	{
		
		case EAction::Run:	bRun = 0;		return R;//◘◘>
		case EAction::Crouch:	bCrouch = 0;	return R;//◘◘>
		case EAction::Fly:		bFly = 0;		return R;//◘◘>
		case EAction::Soar:		bSoar = 0;		return R;//◘◘>
		case EAction::Climb:		bClimb = 0;		return R;//◘◘>
	}

	//по умолчанию для неудачи непонятной
	R = EResult::FAILED_TO_STRIKE;

	//если выполняется действие-атака и именно то выполняетсяч действие, которое мы хотим продолжить
	if (DoesAttackAction() && GetAttackAction()->UsedAs.Contains(Type))
	{
		//не проверяем, муторно - этот тракт не вызывается из ИИ - сразу вдаряем
		//однако...
		//направление атаки может быть отдано ИИ для лучшего прицеливания
		if (Daemon && Daemon->ObjectAtFocus)
		{
			//мало быть в фокусе, нужно быть еще физически доставаемым атакой
			if (GetAttackAction()->QuickAimResult(this, *Daemon->ObjectAtFocus, 0.7f))
			{
				//если угол и радиус подходят, то можно помочь в нациливании
				AttackDirection = Daemon->ObjectAtFocus->Location.DecodeDir();
				Daemon->AutoAim = true;
			}
		}

		//само действие, тут уже вряд ли что-то пойдет не так
		R = AttackActionStrike();
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s ActionFindRelease %s %s"), *GetName(),
			*GetAttackAction()->GetName(), *TXTENUM(EResult, R));
	}
	//выполняется действие-релакс - отжатие есть старт выхода из релакса
	else if(DoesRelaxAction() && GetRelaxAction()->UsedAs.Contains(Type))
		RelaxActionStartGetOut();

	//такое не бывает, но надо предусмотреть
	else if (DoesSelfAction() && GetSelfAction()->UsedAs.Contains(Type))
	{	UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s ActionFindRelease WTF SelfAction Release %s"), *GetName(),
			*TXTENUM(EAction, Type ));
	}

	//прочие соотношения, неясно, стоит или их рассматривать
	else
	{

	}

	//неведомое умолчание
	return R;
}

//==============================================================================================================
//распознать суть действия: атака/самодействие/релакс и запустить в нужном слоте, должно вызываться после проверок
//==============================================================================================================
EResult	AMyrPhyCreature::ActionStart(uint8 Number)
{
	if (GenePool->Actions[Number]->IsAttack())
		return AttackActionStart(Number);

	else if(GenePool->Actions[Number]->IsRelax())
		return RelaxActionStart(Number);

	else return SelfActionStart(Number);
}


//◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘
//==============================================================================================================
//начать самодействие (внимание, проверка применимости здесь не производится, надо раньше проверять)
//==============================================================================================================
EResult AMyrPhyCreature::SelfActionStart(int SlotNum)
{
	//если в это же время происходит атака, которая не допускает поверх себя самодействия
	if(Attacking())
		if (GetAttackActionVictim().ForbidSelfActions)
		{	UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s SelfActionStart NO - forbidden by current attack"), *GetName());
			return EResult::LOW_PRIORITY;
		}

	//самодействия могут выбивать друг друга, кроме самих себя
	if (CurrentSelfAction != SlotNum)
	{
		//выбить старое самодействие
		if (CurrentSelfAction != 255) SelfActionCease();

		//теперь можно стартовать
		CurrentSelfAction = SlotNum;
		AdoptWholeBodyDynamicsModel(&GetSelfAction()->DynModelsPerPhase[SelfActionPhase]);

		//отправить символический звук выражения, чтобы очевидцы приняли во внимание и эмоционально пережили
		//здесь вообще-то нужно не для каждого самодействия, а только для значимых и видных
		MyrAIController()->LetOthersNotice(EHowSensed::EXPRESSED); 
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s SelfActionStart OK %s"), *GetName(), *GetCurrentSelfActionName());

		//показать в худе имя начатого действия
		if(Daemon) Daemon->HUDOfThisPlayer()->OnAction(1, true);
		return EResult::STARTED;
	}
	return EResult::FAILED_TO_START;
}

//==============================================================================================================
//инкрементировать фазу самодействия и применить слепки моделей динамики для новой фазы
//==============================================================================================================
void AMyrPhyCreature::SelfActionNewPhase()
{
	//если слот самодействия доступен
	if (CurrentSelfAction != 255)
	{	SelfActionPhase++;
		AdoptWholeBodyDynamicsModel(&GetSelfAction()->DynModelsPerPhase[SelfActionPhase]);
	}
}

//==============================================================================================================
//прервать самодействие
//==============================================================================================================
void AMyrPhyCreature::SelfActionCease()
{
	//если есть, что прерывать
	if (CurrentSelfAction != 255)
	{
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s SelfActionCease %s"),
			*GetName(), *GetCurrentSelfActionName());

		//если к самодействию привязан еще и звук
		if (Mesh->DynModel->Sound.IsValid())
		{
			//прервать этот звук
			Voice->Stop();
			CurrentSpelledSound = EPhoneme::S0;
		}

		//собственно, очистить переменную 
		CurrentSelfAction = 255;
		SelfActionPhase = 0;

		//сняв модель самодействия, вернуть нижележащую модель
		AdoptWholeBodyDynamicsModel(GetPriorityModel());

		//удалить в худе имя начатого действия
		if (Daemon) Daemon->HUDOfThisPlayer()->OnAction(1, false);
	}
}

//==============================================================================================================
//найти список применимых в данный момент самодействий и внести их в массив
//==============================================================================================================
void AMyrPhyCreature::ActionFindList(bool RelaxNotSelf, TArray<uint8>& OutActionIndices, EAction Restrict)
{
	//отыскать подходящее, изнутри вызвав оценку применимости
	for(int i=0; i<GenePool->Actions.Num(); i++)
	{
		//сопоставление сути очередного действия с тем, что нужно - самодействия или ралкс-действия
		if (GenePool->Actions[i]->IsRelax() == RelaxNotSelf)

			//если ограничение на константу темы действия
			if (Restrict == EAction::NONE || GenePool->Actions[i]->UsedAs.Contains(Restrict))
			{

				//полная проверка всех критериев применимости, за исключение вероятности
				auto R = GenePool->Actions[i]->IsActionFitting(this, false);

				//адресация по индексу, адресат должен иметь указатель на генофонд существа для получения списка
				if (ActOk(R)) OutActionIndices.Add(i);
				else
					UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s ActionFindList Reject %s cause %s"),
						*GetName(), *GenePool->Actions[i]->HumanReadableName.ToString(), *TXTENUM(EResult, R));
			}
	}
		
}

//==============================================================================================================
// 	найти наиболее подходящее по контексту действие (в основном для ИИ)
//==============================================================================================================
uint8 AMyrPhyCreature::ActionFindBest(USceneComponent* VictimBody, float Damage, float Dist, float Coaxis, bool ByAI, bool Chance)
{
	//возвращается индекс в массиве, если остлся 255, значит ничего не нашли
	uint8 IndexOfBest = 255;
	for (int i = 0; i < GenePool->Actions.Num(); i++)
	{
		//вначале отсеиваем те, которые не выдерживают текущей опасности, это для контратаки, в остальных не нужно, по умолчанию 0
		auto A = GenePool->Actions[i];
		if (Damage < A->MaxDamageWeCounterByThis)
		{	
			//стандартная проверка в полном контексте (пока неясно, что делать с типом жертвы, будет ли ненулевой)
			auto R = A->IsActionFitting (this, VictimBody, Dist, Coaxis, ByAI, Chance);
			if (ActOk(R))
			{	if(IndexOfBest == 255)							IndexOfBest = i; else
				if(A->Better(GenePool->Actions[IndexOfBest]))	IndexOfBest = i;
			}
		}
	}
	return IndexOfBest;
}


//==============================================================================================================
//стартануть релакс уже известный по индексу
//==============================================================================================================
EResult AMyrPhyCreature::RelaxActionStart(int Slot)
{
	//присвоить, анимация сама это далее воспримет
	CurrentRelaxAction = Slot;

	//начать с первой, нулевой фазы, фазы перехода от стояния к лежанию и т.п.
	RelaxActionPhase = 0;

	//применить новые настройки динамики движения/поддержки для частей тела поясов
	AdoptWholeBodyDynamicsModel(&GetRelaxAction()->DynModelsPerPhase[RelaxActionPhase]);
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s RelaxActionStart %s"), *GetName(), *GetRelaxAction()->GetName());
	return EResult::STARTED;
}

//==============================================================================================================
//достичь фазы стабильности действия отдыха - начать сам отдых (вызывается из закладки анимации MyrAnimNotify)
//==============================================================================================================
void AMyrPhyCreature::RelaxActionReachPeace()
{
	//если реально делаем жест отдыха в начальной фазе - перейти к следующей фазе, фазе стабильности
	if (CurrentRelaxAction != 255)
	{	
		//если поймали первую закладку - собственно перейти в фазу "ок, легли, отдыхаем"
		//если поймали вторую закладку, то фаза уже была установлена в момент нажатия кнопки
		if(RelaxActionPhase==0) RelaxActionPhase = 1;

		//в обоих случаях применить новую фазу на уровне физики тела
		AdoptWholeBodyDynamicsModel(&GetRelaxAction()->DynModelsPerPhase[RelaxActionPhase]);
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s RelaxActionReachPeace %s"), *GetName(), *GetRelaxAction()->GetName());

		//отметить событие
		CatchMyrLogicEvent(EMyrLogicEvent::SelfRelaxStart, 1.0f, GetRelaxAction());
	}
}

//==============================================================================================================
//начать выход из фазы стабильности - загругляться с отдыхом
//==============================================================================================================
void AMyrPhyCreature::RelaxActionStartGetOut()
{
	//если реально делаем жест отдыха в фазе установившегося действия - перейти к следующей фазе, закруглению
	if (CurrentRelaxAction != 255 && RelaxActionPhase == 1)
	{
		//внимание, парадокс, фаза заранее устанавливается в 2, но анимация еще не дошла до 2 закладки
		//зато скорость анимации уже по этой 2ке устанавливается (в UMyrPhyCreatureAnimInst::NativeUpdateAnimation)
		//так чтобы ролик доиграл до закладки с финальной фазой
		RelaxActionPhase = 2;
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s RelaxActionStartGetOut %s"), *GetName(), *GetRelaxAction()->GetName());
	}
}

//==============================================================================================================
//достичь конца и полного выхода (вызывается из закладки анимации MyrAnimNotify)
//==============================================================================================================
void AMyrPhyCreature::RelaxActionReachEnd()
{
	//если реально делаем жест отдыха в фазе установившегося действия - перейти к следующей фазе, закруглению
	if (CurrentRelaxAction != 255 && RelaxActionPhase == 2)
	{		
		//отметить событие
		CatchMyrLogicEvent(EMyrLogicEvent::SelfRelaxEnd, 1.0f, GetRelaxAction());
		RelaxActionPhase = 0;
		CurrentRelaxAction = 255;

		//вернуть настройки динамики уровня выше - уровня состояния поведения
		AdoptWholeBodyDynamicsModel(GetPriorityModel());
	}
}

//◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘


//==============================================================================================================
// найти, что схесть - живая еда должна быть подвешена (взята) в одну из конечностей
//==============================================================================================================
UPrimitiveComponent* AMyrPhyCreature::FindWhatToEat(FDigestiveEffects*& D, FLimb*& LimbWith)
{
	UPrimitiveComponent* Food = nullptr;
	FDigestiveEffects* dFood = nullptr;
	FLimb* LimbWithVictim = nullptr;

	//по всем частям тела - поиск объектов, которых они так или иначе касаются
	for (int i = 0; i < (int)ELimb::NOLIMB; i++)
	{
		//сначала найти то, что держится в этом члене
		FLimb& Limb = Mesh->GetLimb((ELimb)i);
		if(!Limb.Floor) continue;

		//глобальная функция по поиску пищевой ценности в произвольном объекте
		dFood = GetDigestiveEffects (Limb.Floor.Actor());
		if(dFood)
		{
			//можно есть лишь тех, которые зажаты в зубах
			//чтоб не съесть того, кто держит нас или не пересчитывать соотношение масс
			//для неподвижных предметов поедание осуществляется вместо Grab, и эта функция не вызывается
			if (!Limb.Grabs) continue;

			//исключить те объекты, которые не восполняют энергию, то есть несъедобные
			if (dFood->Empty()) continue;
			Food = Limb.Floor.Component();
			LimbWith = &Limb;
			D = dFood;
		}
	}
	return Food;
}

//==============================================================================================================
//абстрактно съесть конкретную вещь в мире = поглотть ее энергию, может, отравиться
//==============================================================================================================
bool AMyrPhyCreature::EatConsume(UPrimitiveComponent* Food, FDigestiveEffects* D, float Part)
{
	//время действия пищи должно быть положительным, иначе это не пища
	if (D->Empty())
	{	UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s EatConsume: Uneatable! %s"), *GetName(), *Food->GetName());
		return false;
	}

	//логически съесть = перенести эффекты пищи из объекта в себя
	bool AteAll = DigestiveEffects.Transfer(D, Part);

	//здесь фиксируется только субъективный факт поедания, применения силы, а не результат
	CatchMyrLogicEvent(EMyrLogicEvent::ObjEat, Part, Food);

	//вывести сообщение, что кого-то съели
	if (Daemon)
		if (auto Wid = Daemon->HUDOfThisPlayer())
			Wid->OnEat(Food->GetOwner());

	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s EatConsume %s"), *GetName(), *Food->GetOwner()->GetName());
	return true;
}


//==============================================================================================================
// поедание пояманного существа - общая функция, внаале поиск
//==============================================================================================================
bool  AMyrPhyCreature::Eat()
{
	//сначала найти, что кушать
	FDigestiveEffects* dFood = nullptr;
	FLimb* LimbWithVictim = nullptr;
	UPrimitiveComponent* Food = FindWhatToEat(dFood, LimbWithVictim);
	if (!Food) return false;

	//№№№№№№№№№№№№№№
	//абстрактно, в переменных съесть, пополнить энергию
	bool result = EatConsume(Food, dFood);
	//№№№№№№№№№№№№№№


	//поскольку хватание сопровождалось атакой, после съедания надо всякую атаку остановить
	AttackEnd();

	//не смогли съесть, остальное значит не нужно
	if (!result) return false;

	//найти и создать объект останков (с новой системой надо переделать, видимо из генофонда
	//AActor* Remains = nullptr;
	//if (dFood->WhatRemains)
	//	Remains = GetWorld()->SpawnActor(*dFood->WhatRemains, &Food->GetComponentTransform());


	//перед деструкцией будет отвязан ИИ
	Food->GetOwner()->Destroy();
	GetMyrGameInst()->Statistics.FoodEaten++;
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s Eat() %s"), *GetName(), *Food->GetOwner()->GetName());
	return true;
}

//==============================================================================================================
//обнаружить, что мы пересекаем умный объём и передать ему, что мы совершаем, возможно, судьбоносное действие
//==============================================================================================================
void AMyrPhyCreature::SendApprovalToOverlapper()
{
	if(Overlap0) Overlap0->ReceiveActiveApproval(this);
	if(Overlap1) Overlap1->ReceiveActiveApproval(this);
	if(Overlap2) Overlap2->ReceiveActiveApproval(this);
}

//==============================================================================================================
//уведомить триггер при пересечении, что его функция может быть сразу задействована,
//потому что условие активного включение было выполнено до пересечения
//==============================================================================================================
bool AMyrPhyCreature::CouldSendApprovalToTrigger(class UMyrTriggerComponent* Sender)
{
	if (Attacking())
	{
		//если атака посылает сигнал со старта - он действует весь период атаки
		if (GetAttackActionVictim().SendApprovalToTriggerOverlapperOnStart)
			return true;
		if (GetAttackActionVictim().SendApprovalToTriggerOverlapperOnStrike)
			if (CurrentAttackPhase == EActionPhase::RUSH || CurrentAttackPhase == EActionPhase::STRIKE || CurrentAttackPhase == EActionPhase::RUSH)
				return true;
	}
	return false;
}

void AMyrPhyCreature::WidgetOnGrab(AActor* Victim)
{	if (Daemon)	if (auto Wid = Daemon->HUDOfThisPlayer())	Wid->OnGrab(Victim); }

void AMyrPhyCreature::WidgetOnUnGrab(AActor* Victim)
{	if (Daemon)	if (auto Wid = Daemon->HUDOfThisPlayer())	Wid->OnUnGrab(Victim); }

//==============================================================================================================
//показать на виджете худа то, что пересечённый григгер-волюм имел нам сообщить
//==============================================================================================================
void AMyrPhyCreature::WigdetOnTriggerNotify(EWhyTrigger ExactReaction, AActor* What, USceneComponent* WhatIn, bool On)
{
	if (Daemon)
		if (auto Wid = Daemon->HUDOfThisPlayer())
			Wid->OnTriggerNotify(ExactReaction, What, On);

}



//==============================================================================================================
//передать информацию в анимацию из ИИ (чтобы не светить ИИ в классе анимации)
//==============================================================================================================
void AMyrPhyCreature::TransferIntegralEmotion(float& Rage, float& Fear, float& Power, float &Amount)
{
	//Rage = MyrAI()->IntegralEmotionRage;
	//Fear = MyrAI()->IntegralEmotionFear;
	//Power = MyrAI()->IntegralEmotionPower;
	//Amount = FMath::Lerp(Amount,
	//	NoAttack() ? GetBehaveCurrentData()->ExpressEmotionPower : 0.0f, 0.1f);
}

//==============================================================================================================
//зарегистрировать пересекаемый объём с функционалом - вызывается из TriggerComponent
//==============================================================================================================
void AMyrPhyCreature::AddOverlap(UMyrTriggerComponent* Ov)
{
	//простейший стек
	if (Overlap0)
	{	if (Overlap1)
			Overlap2 = Overlap1;
		Overlap1 = Overlap0;
	}
	Overlap0 = Ov;

	//случай входа на локацию
	if(Ov->GetOwner()->IsA<AMyrLocation>() && Daemon)
	{	Daemon->CurLocNoRain = ((AMyrLocation*)Ov->GetOwner())->FullyCoversRain;
		Daemon->CurLocFliesMod = ((AMyrLocation*)Ov->GetOwner())->FliesAmount;
	}
}

//==============================================================================================================
//исключить пересекаемый объём при выходе из него - вызывается из TriggerComponent
//==============================================================================================================
bool AMyrPhyCreature::DelOverlap(UMyrTriggerComponent* Ov)
{
	//случай покидания локации
	if(Ov->GetOwner()->IsA<AMyrLocation>() && Daemon)
	{	Daemon->CurLocNoRain = false;
		Daemon->CurLocFliesMod = 255;
	}

	//удаляем только те, еоторые ранее были зарегистрированы
	if(Ov == Overlap0)
	{	Overlap0 = Overlap1;
		Overlap1 = Overlap2;
		Overlap2 = nullptr;
		return true;
	}
	else if(Ov == Overlap1)
	{	Overlap1 = Overlap2;
		Overlap2 = nullptr;
		return true;
	}
	else if(Ov == Overlap2)
	{	Overlap2 = nullptr;
		return true;
	}
	return false;
}

//найти объём, в котором есть интересующая реакция, если нет выдать нуль
UMyrTriggerComponent* AMyrPhyCreature::HasSuchOverlap(EWhyTrigger r, FTriggerReason*& TR)
{
	if (!&TR) return nullptr;
	if (Overlap0) { TR = Overlap0->HasReaction(r); if (TR) return Overlap0; }
	if (Overlap1) { TR = Overlap1->HasReaction(r); if (TR) return Overlap1; }
	if (Overlap2) { TR = Overlap2->HasReaction(r); if (TR) return Overlap2; }
	return nullptr;
}

//найти объём, в котором есть интересующая реакция, если нет выдать нуль
UMyrTriggerComponent* AMyrPhyCreature::HasSuchOverlap(EWhyTrigger r, FTriggerReason*& TR, EWhoCame WhoAmI)
{
	if (!&TR) return nullptr;
	if (Overlap0) { TR = Overlap0->HasReaction(r, WhoAmI); if (TR) return Overlap0; }
	if (Overlap1) { TR = Overlap1->HasReaction(r, WhoAmI); if (TR) return Overlap1; }
	if (Overlap2) { TR = Overlap2->HasReaction(r, WhoAmI); if (TR) return Overlap2; }
	return nullptr;
}
UMyrTriggerComponent* AMyrPhyCreature::HasSuchOverlap(EWhyTrigger rmin, EWhyTrigger rmax, FTriggerReason*& TR)
{
	if (!&TR) return nullptr;
	if (Overlap0) { TR = Overlap0->HasReaction(rmin, rmax); if (TR) return Overlap0; }
	if (Overlap1) { TR = Overlap1->HasReaction(rmin, rmax); if (TR) return Overlap1; }
	if (Overlap2) { TR = Overlap2->HasReaction(rmin, rmax); if (TR) return Overlap2; }
	return nullptr;
}
UMyrTriggerComponent* AMyrPhyCreature::HasGoalOverlap(FTriggerReason*& TR)
{
	if (Overlap0) { TR = Overlap0->HasReaction(EWhyTrigger::KinematicLerpToCenter); if (TR) return Overlap0; }
	if (Overlap1) { TR = Overlap1->HasReaction(EWhyTrigger::KinematicLerpToCenter); if (TR) return Overlap1; }
	if (Overlap2) { TR = Overlap2->HasReaction(EWhyTrigger::KinematicLerpToCenter); if (TR) return Overlap2; }

	return nullptr;
}

AMyrLocation* AMyrPhyCreature::IsInLocation() const
{
	if (Overlap0) { if (Overlap0->GetOwner()->IsA<AMyrLocation>()) return (AMyrLocation*)Overlap0->GetOwner(); }
	if (Overlap1) { if (Overlap1->GetOwner()->IsA<AMyrLocation>()) return (AMyrLocation*)Overlap1->GetOwner(); }
	if (Overlap2) { if (Overlap2->GetOwner()->IsA<AMyrLocation>()) return (AMyrLocation*)Overlap2->GetOwner(); }
	return nullptr;
}

//==============================================================================================================
//найти среди объёмов самую лучшую цель
//==============================================================================================================
int AMyrPhyCreature::FindMoveTarget(float Radius, float Coaxis)
{
	uint8 BestOvI = 255;									//индекс, номер по счёту
	float BestOvRa = 0;										//рейтинг цели для прыжка, ничего не значит, только сравнение
	UMyrTriggerComponent** Ovs = &Overlap0;					//для адресации массивом
	for (int i = 0; i < 3; i++)
		if (Ovs[i])
		{	float R = Ovs[i]->JumpGoalRating(this, Radius, Coaxis);
			if (R > BestOvRa) { BestOvRa = R; BestOvI = i; }
		}
	return BestOvI;
}
//==============================================================================================================
//извлечь вектор из триггера и направить тело по нему - внимание, здесь нет нормализации!
//==============================================================================================================
bool AMyrPhyCreature::ModifyMoveDirByOverlap(FVector3f& InMoveDir, EWhoCame WhoAmI)
{
	//проверить есть ли заданный триггер вокруг нас
	FTriggerReason *TR = nullptr;
	UMyrTriggerComponent* UsedOv = nullptr;
	FVector3f Force(0);

	UsedOv = HasSuchOverlap ( EWhyTrigger::VectorFieldMover, TR, WhoAmI);
	if(UsedOv) 	Force = UsedOv->ReactVectorFieldMove(*TR, this);
	else UsedOv = HasSuchOverlap  ( EWhyTrigger::GravityPit, TR, WhoAmI);
	if (UsedOv) Force = UsedOv->ReactGravityPitMove(*TR, this);
	else return false;

	//исключить вертикальную составляющую если текущий профиль поведения двигает только в плоскости
	if (!GetBehaveCurrentData()->bOrientIn3D) Force.Z = 0;
	InMoveDir += Force;
	return true;
}

//==============================================================================================================
//в момент исчезновения объекта сделать так, чтобы в памяти ИИ ничего не сломалось
//==============================================================================================================
void AMyrPhyCreature::HaveThisDisappeared(USceneComponent* This)
{
	//MyrAIController()->LetOthersNotice(EHowSensed::REMOVE);
	//if(MyrAI()) MyrAI()->SeeThatObjectDisappeared((UPrimitiveComponent*)This);
}



//==============================================================================================================
//отладочная линия
//==============================================================================================================
#if WITH_EDITOR
void AMyrPhyCreature::Line(ELimbDebug Ch, FVector A, FVector AB, float W, float Time)
{
	if (IsDebugged(Ch))
	{
		auto C = GetMyrGameInst()->DebugColors.Find(Ch);
		DrawDebugLine(GetOwner()->GetWorld(), A, A + AB,
			(C ? (*C) : FColor(255, 255, 255)), false, Time, 100, W);
	}
}
void AMyrPhyCreature::Linet(ELimbDebug Ch, FVector A, FVector AB, float Tint, float W, float Time)
{
	if (IsDebugged(Ch))
	{
		auto C = GetMyrGameInst()->DebugColors.Find(Ch);
		DrawDebugLine(GetOwner()->GetWorld(), A, A + AB,
			(FLinearColor((C?(*C):FColor(255, 255, 255))) * Tint).ToFColor(false), false, Time, 100, W);
	}
}
void AMyrPhyCreature::Line(ELimbDebug Ch, FVector A, FVector AB, FColor Color, float W, float Time)
{
	if (IsDebugged(Ch))
		DrawDebugLine(GetOwner()->GetWorld(), A, A + AB,
			Color, false, Time, 100, W);
}
#endif

//◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘
//==============================================================================================================
// сейчас включен режим от первого лица - чтоб знали другие компоненты
//==============================================================================================================
bool AMyrPhyCreature::IsFirstPerson()
{
	if (Daemon) return Daemon->IsFirstPerson();
	return false;
}

//==============================================================================================================
//выдать координаты центра масс конкретной части тела
//==============================================================================================================
FVector AMyrPhyCreature::GetBodyPartLocation(ELimb Limb)
{
	if(Mesh->HasPhy(Limb))
		return Mesh->GetMachineBody(Limb)->GetCOMPosition();
	else return Mesh->GetMachineBody(ELimb::LUMBUS)->GetCOMPosition();
}

//==============================================================================================================
//конечная точка для трассировки с целью проверки видимости
//==============================================================================================================
FVector AMyrPhyCreature::GetVisEndPoint()
{
	//в каждый момент щупаем разные, случайно выбранные части тела
	//возможно, стоит ввести коэффициенты вероятности, чтобы выбирать осознанно
	int li = FUrGestalt::RandVar&15; if(li >= (int)ELimb::NOLIMB) li = 15-li;
	return GetBodyPartLocation((ELimb)li);
}

//==============================================================================================================
//касаемся ли мы/стоим ли мы на этом объекте
//==============================================================================================================
bool AMyrPhyCreature::IsStandingOnThisActor(AActor* A) { return Mesh->IsStandingOnActor(A); }
bool AMyrPhyCreature::IsTouchingThisActor(AActor* A) { return Mesh->IsTouchingActor(A); }
bool AMyrPhyCreature::IsTouchingThisComponent(UPrimitiveComponent* A) { return Mesh->IsTouchingComponent(A); }



//==============================================================================================================
//найти локацию той части тела, которая ближе всего к интересующей точке
//==============================================================================================================
FVector AMyrPhyCreature::GetClosestBodyLocation(FVector Origin, FVector Earlier, float EarlierD2)
{
	//расстояние посчитанное другими средствами (новое расстояние должно быть не больше)
	float accumD2 = EarlierD2;

	//предыдущее значение точки, если не найдём лучшего, оно сохранится
	FVector ret = Earlier;

	//чисто для параметра функции, пока не нужно
	FVector LocOnBody;

	for(int i=0; i<(int)ELimb::NOLIMB; i++)
		if(Mesh->HasPhy((ELimb)i))
		{	float AnotherD2;
			FVector Loc;
			bool GotIt = Mesh -> GetMachineBody(ELimb(i)) -> GetSquaredDistanceToBody(Origin, AnotherD2, Loc);
			if (GotIt && AnotherD2 < accumD2) { accumD2 = AnotherD2; ret = Loc; }
		}

	return ret;
}
//==============================================================================================================
//вектор, в который тело смотрит глазами - для определения "за затылком"
//==============================================================================================================
FVector3f AMyrPhyCreature::GetLookingVector() const
{
	return Mesh->GetLimbAxisForth(Mesh->Head);
}
//==============================================================================================================
//вектор вверх тела
//==============================================================================================================
FVector3f AMyrPhyCreature::GetUpVector() const
{
	return Mesh->GetLimbAxisUp(Mesh->Thorax);
}

//==============================================================================================================
//получить множитель незаметности, создаваемый поверхностью, на которой мы стоим - вызывается из ИИ
//==============================================================================================================
float AMyrPhyCreature::GetCurrentSurfaceStealthFactor() const
{
	//если нет явных модификаций, коэффициент = единица, то есть ничего не меняется
	float ThoraxStealthFactor = 1.0f, PelvisStealthFactor = 1.0f;

	//поверхность из ног при ходьбе передался в членик-промежность, оттуда по типу поверхности берем сборку
	//для каждого пояса конечностей проверяется его призмеденность - иначе поверхности опоры или нет, или она устарела
	if(Thorax->StandHardness > 100)
	{	auto pg = GetMyrGameInst()->Surfaces.Find(Mesh->Thorax.Floor.Surface);
		if (pg) ThoraxStealthFactor = pg->StealthFactor;
	}
	if(Pelvis->StandHardness > 100)
	{	auto pg = GetMyrGameInst()->Surfaces.Find(Mesh->Pelvis.Floor.Surface);
		if (pg) PelvisStealthFactor = pg->StealthFactor;
	}

	//берем минимальный из двух частей тела (почему минимальный?)
	return FMath::Max(PelvisStealthFactor, ThoraxStealthFactor);
}

//выдать, если есть, цель, на которую можно кинематически наброситься. может выдать нуль
USceneComponent* AMyrPhyCreature::GetRushGoal()
{
	//if(MyrAI()->Goal_1().Object && (MyrAI()->Goal_1().LookAtDist < SpineLength*2))
	//	return MyrAI()->Goal_1().Object; else
	 return nullptr;
}

//рассеяние звука текущее - от уровня и локации
USoundAttenuation* AMyrPhyCreature::GetCurSoundAttenuation()
{
	return GetMyrGameMode()->SoundAttenuation;
}


//==============================================================================================================
//выдать скорость метаболизма - пока она только в аним-инстансе
//==============================================================================================================
float AMyrPhyCreature::GetMetabolism() const
{
	auto I = (UMyrPhyCreatureAnimInst*)Mesh->GetAnimInstance();
	return I ? I->Metabolism : 1.0f;
}


//==============================================================================================================
//участвует ли эта часть тела в непосредственно атаке
//вообще функционально эта функция должна быть в классе Mesh, но фактически все исходные данные здесь
//==============================================================================================================
bool AMyrPhyCreature::IsLimbAttacking(ELimb eLimb)
{	if (Attacking())
		if (auto AFS = GetAttackActionVictim().WhatLimbsUsed.Find(eLimb))
			if ((uint32)AFS->Phases & 1<<(uint32)CurrentAttackPhase)
				return true;
	return false;
}

USceneComponent* AMyrPhyCreature::GetObjectIStandOn()
{
	return Mesh->Pelvis.Floor.Component();
}
