// Fill out your copyright notice in the Description page of Project Settings.


#include "MyrPhyCreature.h"
#include "MyrPhyCreatureMesh.h"							// графическая модель
#include "../MyrraGameInstance.h"						// ядро игры
#include "../MyrraGameModeBase.h"						// текущий уровень - чтобы брать оттуда протагониста
#include "Artefact/MyrArtefact.h"						// неживой предмет - для логики поедания
#include "Artefact/MyrTriggerComponent.h"				// триггер - для посылки ему "я нажал Е!"
#include "../Control/MyrDaemon.h"						// демон
#include "../Control/MyrAI.h"							// ИИ
#include "../UI/MyrBioStatUserWidget.h"					// индикатор над головой
#include "AssetStructures/MyrCreatureGenePool.h"		// данные генофонда
#include "Animation/MyrPhyCreatureAnimInst.h"			// для доступа к кривым и переменным анимации

#include "Components/WidgetComponent.h"					// ярлык с инфой
#include "Components/AudioComponent.h"					// звук
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"					//декаль следа
#include "PhysicsEngine/PhysicsConstraintComponent.h"	// привязь на физдвижке
#include "Particles/ParticleSystemComponent.h"			//для генерации всплесков шерсти при ударах

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
#define TRANSIT(state, cond) case EBehaveState::##state: if(cond) AdoptBehaveState(EBehaveState::##state); break;
#define TRANSIT2(state, stateE, cond) case EBehaveState::##state: if(cond) if(!AdoptBehaveState(EBehaveState::##state)) AdoptBehaveState(EBehaveState::##stateE);
#define BEWARE(cond, state) if(cond) if(AdoptBehaveState(EBehaveState::##state)) break
#define BEWAREELSE(cond, state, altst) if(cond) if(!AdoptBehaveState(EBehaveState::##state)) {if(AdoptBehaveState(EBehaveState::##altst)) break;} else break

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
	bMoveBack = 0;

	//механический лошарик
	Mesh = CreateOptionalDefaultSubobject<UMyrPhyCreatureMesh>(TEXT("Mesh"));
	RootComponent = Mesh;

	//отбойник не должен быть первым, потому что в редакторе его не развернуть горизонтально
	Thorax = CreateDefaultSubobject<UMyrGirdle>(TEXT("Thorax"));
	Thorax->SetupAttachment(RootComponent);
	Thorax->IsThorax = true;
	Pelvis = CreateDefaultSubobject<UMyrGirdle>(TEXT("Pelvis"));
	Pelvis->SetupAttachment(RootComponent);
	Pelvis->IsThorax = false;

	//компонент персональных шкал здоровья и прочей хрени (висит над мешем)
	LocalStatsWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Local Stats Widget"));
	LocalStatsWidget->SetupAttachment(Mesh, TEXT("HeadLook"));
	LocalStatsWidget->SetGenerateOverlapEvents(false);

	//источник звука для голоса
	Voice = CreateDefaultSubobject<UAudioComponent>(TEXT("Voice Audio Component"));
	Voice->SetupAttachment(RootComponent);
	Voice->OnAudioPlaybackPercent.AddDynamic(this, &AMyrPhyCreature::OnAudioPlaybackPercent);
	Voice->OnAudioFinished.AddDynamic(this, &AMyrPhyCreature::OnAudioFinished);

	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	//первоначальное направление - вперед
	MoveDirection = GetAxisForth();

	//сразу назаначаем класс ИИ контроллера, который будет создаваться для любого экземпляра и потомка
	AIControllerClass = AMyrAI::StaticClass();
}

//====================================================================================================
//перет инициализацией компонентов, чтобы компоненты не охренели от родительской пустоты во время своей инициализации
//====================================================================================================
void AMyrPhyCreature::PreInitializeComponents()
{
	//подготовка к игре под капотом
	Super::PreInitializeComponents();

	//инициализируем начальное состояние поведение, чтобы не нарваться на пустоту
	if(GetGenePool()) BehaveCurrentData = *GetGenePool()->BehaveStates.Find(EBehaveState::walk);
}

//====================================================================================================
// появление в игре - финальные донастройки, когда всё остальное готово
//====================================================================================================
void AMyrPhyCreature::BeginPlay()
{
	//против запуска в редакторе блюпринта
	if (IsTemplate()) { Super::BeginPlay(); return; }
	
	//если существо грузится из сохранки, то для него припасена структура данных,
	//которая после загрузки уровня из сохранки дожидается, когда существо дорастет до BeginPlay
	
	FCreatureSaveData* CSD = nullptr;
	if (GetMyrGameInst()->JustLoadedSlot)
		CSD = GetMyrGameInst()->JustLoadedSlot->AllCreatures.Find(GetFName());

	//загрузить, если мы нашлись достойными быть сохраненным
	if (CSD) Load(*CSD);

	//если нет сохранения или мы высраны в рантайме
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
		SetCoatTexture(0);
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
	{	StatsWidget()->MyrOwner = this;					
		StatsWidget()->MyrAI = MyrAI();			
	}

	//рассчитать анимации длины/амплитуды ног - пока здесь, потому что чем позже тем лучше
	//должны быть полностью спозиционированы все кости
	Pelvis->UpdateLegLength();
	Thorax->UpdateLegLength();

	//задать начальное, нормальное значение длины спины до того как якоря поясов будут расставлены
	SpineLength = FVector::Dist(
		Mesh->GetMachineBody(Mesh->Thorax)->GetCOMPosition(),
		Mesh->GetMachineBody(Mesh->Pelvis)->GetCOMPosition());

	//свойства гашения звука голоса берем из текущего уровня (вероятно, у локаций будут другие)
	Voice->AttenuationSettings = GetMyrGameMode()->SoundAttenuation;

	//звякнуть геймплейным событием - в основном для высираемых мобов
	CatchMyrLogicEvent(EMyrLogicEvent::SelfSpawnedAtLevel, Daemon ? 1 : 0, nullptr);
}

//====================================================================================================
// каждый кадр
//====================================================================================================
void AMyrPhyCreature::Tick(float DeltaTime)
{
	FrameCounter++;
	if (HasAnyFlags(RF_ClassDefaultObject)) return;
	if (IsTemplate()) return;

	//набор мощи для атаки/прыжка
	if (AttackForceAccum > 0.0f && AttackForceAccum < 1.0f)
		AttackForceAccum += DeltaTime;

	//для игрока пересчитать курс движения по зажатым кнопкам
	if (Daemon) Daemon->MakeMoveDir(BehaveCurrentData->bOrientIn3D);

	//демон сохраняет свои планы в отдельные переменные, здесь они по идее должны смешиваться с таковыми из ИИ
	//но пока просто тупо присваивается
	ConsumeInputFromControllers(DeltaTime);

	//обработка уткнутости в препятствие и простейшая корректировка курса
	WhatIfWeBumpedIn();

	//боль - усиленная разность старого, высокого здоровья и нового, сниженного мгновенным уроном. 
	Pain *= 0.99f;

	//восстановление запаса сил (используется та же функция, что и при трате)
	StaminaChange (Mesh->DynModel->StaminaAdd * DeltaTime );										

	//обработка движений по режимам поведения 
	ProcessBehaveState(DeltaTime);

	//обработать пояса конечностей
	Thorax->Procede(DeltaTime);
	Pelvis->Procede(DeltaTime);

	//по идее здесь тичат компоненты
	Super::Tick(DeltaTime);
}


//==============================================================================================================
// вызывается, когда играется звук (видимо, каждый кадр) и докладывает, какой процент файла проигрался
//====================================================================================================
void AMyrPhyCreature::OnAudioPlaybackPercent(const USoundWave* PlayingSoundWave, const float PlaybackPercent)
{
	//ебанатский анрил 4.27 запускает это сообщение даже когда не играет звук
	if (!Voice->IsPlaying()) return;

	//длина строки субитров
	const int Length = PlayingSoundWave->SpokenText.Len();

	//открывать рот только если в текущем аудио есть субтитры (с версии 4.24 вызывается и когда 100%)
	if (Length > 0 && PlaybackPercent < 1.0)
	{
		//текущий произносимый звук
		//здесь также можно парсить строку, чтобы вносить эмоциональные вариации
		CurrentSpelledSound = (EPhoneme)PlayingSoundWave->SpokenText[Length * PlaybackPercent];
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
		StatsWidget()->MyrAI = MyrAI();
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

	//руление курса движения осуществляется только при наличии тяги
	if(D->Gain>0)
	{
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
						FVector SideWay = Mesh->GetLimbAxisRight(Mesh->Lumbus);
						if((SideWay|D->MoveDir)<0) SideWay = -SideWay;
						MoveDirection = MoveDirection + SideWay * BestTurnReaction->SpeedFactor;

						//укладывание в плоскость ХУ, если в настройках сказано, поза тела может искажать
						if (!BehaveCurrentData->bOrientIn3D) MoveDirection.Z = 0;

						//нормализация, ибо аддитивно
						MoveDirection.Normalize();
					}
					break;

					//менее плавное выруливание с линейной интерполяцией
					case ETurnMode::SlowLerp:
						MoveDirection = FMath::Lerp(MoveDirection, D->MoveDir, BestTurnReaction->SpeedFactor);
						MoveDirection.Normalize();
						break;

					//режим рельсовости
					case ETurnMode::RailBack:

						//направление приводится к вектору тела вперед или назад
						if(CourseOff > 0.0)	MoveDirection = Thorax->Forward;
						else
						{	MoveDirection = -Thorax->Forward;
							bMoveBack = true;
						}
						if (!BehaveCurrentData->bOrientIn3D)
						{	MoveDirection.Z = 0;
							MoveDirection.Normalize();
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

	////////////////////////////////////////////////////////////////////////////////////////////////
	//если выше, то линейно увеличиваем, а если ниже, то резко обрываем (нужно остановиться вовремя)
	if(D->Gain != ExternalGain)
	{	float Raznice = D->Gain - ExternalGain;
		if (Raznice <= 0.05)
			ExternalGain = D->Gain;
		else ExternalGain += 0.05f;

		//для режима лазанья включение бега вдвое ускоряе, а не-бег вдвое замедляет
		if (!bRun && bClimb)
			if(ExternalGain>0.5)
				ExternalGain *= 0.5;

		//расчет сборного коэффициента замедления, который уменьшает ориентировочную скорость
		UpdateMobility();
	}

	//извлечение конкретных команд и выполнение их
	if (D->DoThis != ECreatureAction::NONE)
	{
		if (D->Release) ActionFindRelease(D->DoThis);
		else ActionFindStart(D->DoThis);
		D->DoThis = ECreatureAction::NONE;
		D->Release = false;
	}

}


//==============================================================================================================
//взять команды и данные из контроллеров игрока и иИИ в нужной пропорции
//==============================================================================================================
void AMyrPhyCreature::ConsumeInputFromControllers(float DeltaTime)
{
	//играется игроком
	if(Daemon)
	{	
		//ИИ может частично отбирать управление у игрока
		if(MyrAI()->AIRuleWeight < 0.1f)
		{
			//заглотить указания
			ConsumeDrive(&Daemon->Drive);

			//особая фигня от первого лица когда смотришь назад
			//чтоб не глазеть внутрь шеи, всё тело активируется движением и поворачивается
			if (Daemon->IsFirstPerson())
			{
				FVector RealForth = Mesh->GetLimbAxisForth(Mesh->Pectus);
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
		{	auto D = MyrAI()->MixWithOtherDrive (&Daemon->Drive);
			ConsumeDrive (&D);
		}

		//направление атаки может быть отдано ИИ для лучшего прицеливания
		if(Daemon->UseActDirFromAI) AttackDirection = MyrAI()->Drive.ActDir;
	}
	//непись, только ИИ, стандартно просчитать новые устремления все
	else ConsumeDrive(&MyrAI()->Drive);
}

//==============================================================================================================
//оценить применимость реакции на уткнутость и если удачно запустить ее
//==============================================================================================================
EAttackAttemptResult AMyrPhyCreature::TestBumpReaction(FBumpReaction* BR, ELimb BumpLimb)
{
	if (BR->Reaction == ECreatureAction::NONE)		return EAttackAttemptResult::INCOMPLETE_DATA;
	if ((BR->BumperLimb & (1<<(int)BumpLimb))==0)	return EAttackAttemptResult::WRONG_ACTOR;
	if (!NoSelfAction())							return EAttackAttemptResult::ALREADY_RUNNING;
	if (BR->Threshold > 0 && Stuck < BR->Threshold)	return EAttackAttemptResult::OUT_OF_RADIUS;
	if (BR->Threshold < 0 && Stuck > BR->Threshold)	return EAttackAttemptResult::OUT_OF_RADIUS;
	if(Thorax->SpeedAlongFront() < BR->MinVelocity)	return EAttackAttemptResult::OUT_OF_VELOCITY;
	
	ActionFindStart(BR->Reaction);					return EAttackAttemptResult::STARTED;
}

//==============================================================================================================
//вычислить степень уткнутости в препятствие и тут же произвести корректировку
//==============================================================================================================
void AMyrPhyCreature::WhatIfWeBumpedIn()
{
	//расчёт сумарной уткнутости - если положительное, то сразу
	//если перед нами непролазное препятствие (-) то постепенно опомниваться от такой колизии
	ELimb Bumper = ELimb::NOLIMB;
	Stuck = Mesh->StuckToStepAmount(Bumper);

	//поиск подходящей реакции на затык
	for (auto R : BehaveCurrentData->BumpReactions)
	{	auto LaunchResult = TestBumpReaction(&R, Bumper);
		if (LaunchResult == EAttackAttemptResult::STARTED) break;
	}
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
				* Mesh->DynModel->MotionGain					// если модель заданая состоянием, самодействием, релаксом и т.п. предполагает замедление
				* FMath::Max(1.0f - Pain, 0.0f);				// боль уменьшает скорость (если макс.боль выше 1, то на просто парализует на долгое время, пока она не спадет)

	//если тягу в расчёт не включать, то мобильность превращается в коэффициент замедления, а не прямое указание к силе движения
	if (IncludeExternalGain) MoveGain *= ExternalGain;

	//чтобы не анимировать странный улиточный микрошаг, пусть лучше стоит на месте
	if (MoveGain < 0.2) MoveGain = 0;

}


//==============================================================================================================
//акт привязывания контроллера игрока
//==============================================================================================================
void AMyrPhyCreature::MakePossessedByDaemon(AActor* aDaemon)
{
	//если подан нуль - текущий демон нас покидает
	if (!aDaemon)
	{	Daemon = nullptr;
		if (MyrAI()) MyrAI()->AIRuleWeight = 1.0f;
	}

	//проверяем типы и если сходятся присваиваем указатель
	else if (AMyrDaemon* daDaemon = Cast<AMyrDaemon>(aDaemon))
	{	Daemon = daDaemon;
		if (MyrAI()) MyrAI()->AIRuleWeight = 0.0f;
	}
}

//==============================================================================================================
//применить пришедшую извне модель сил для всего тела
//==============================================================================================================
void AMyrPhyCreature::AdoptWholeBodyDynamicsModel(FWholeBodyDynamicsModel* DynModel, bool Fully)
{
	//полная замена дин модели, не только звук
	if (Fully)
	{
		//применить новые настройки динамики движения/поддержки для частей тела поясов
		Mesh->DynModel = DynModel;
		Thorax->AdoptDynModel(DynModel->Thorax);
		Pelvis->AdoptDynModel(DynModel->Pelvis);

		//в дин-моделях есть замедлитель тяги, чтобы его прменить, нужно вызвать вот это
		UpdateMobility();
	}

	//голос/общий звук прикреплен к дин-модели, хотя не физика/хз, нужно ли здесь и как управлять силой
	MakeVoice(DynModel->Sound.Get(), 1, false);

}

//==============================================================================================================
//кинематически телепортировать в новое место
//==============================================================================================================
void AMyrPhyCreature::TeleportToPlace(FTransform Dst, bool Rotation)
{
	//непосредственно поместить это существо в заданное место
	FHitResult Hit;
	Mesh->SetSimulatePhysics(false);
	if(Rotation)
	{	Dst.SetScale3D(FVector(1, 1, 1));
		Mesh->SetWorldTransform(Dst, false, &Hit, ETeleportType::TeleportPhysics);
	}else
		Mesh->SetWorldLocation(Dst.GetLocation(), false, &Hit, ETeleportType::TeleportPhysics);
	//Thorax->Procede(0.0);
	//Pelvis->Procede(0.0);
	Mesh->SetSimulatePhysics(true);
	if (Daemon)
		Daemon->PoseInsideCreature();
	UE_LOG(LogMyrPhyCreature, Log, TEXT("%s: TeleportTo %s"), *GetName(),  *Dst.GetLocation().ToString());
}

//==============================================================================================================
//включить или выключить желание при подходящей поверхности зацепиться за нее
//==============================================================================================================
void AMyrPhyCreature::SetWannaClimb(bool Set)
{
	//подробная реализация, с подпрыгом, если нужно
	//Mesh->ClingGetReady(Set);
		//только происходит включение
	if (Set)
	{
		//флаг, что получилось зацепиться вот прям сразу
		auto CanCling = Thorax->CanGirdleCling();
		UE_LOG(LogMyrPhyCreature, Log, TEXT("%s: ClingGetReady %s"), *GetOwner()->GetName(), *TXTENUM(EClung, CanCling));
		switch (CanCling)
		{
			//можно цепляться прямо сейчас
			case EClung::Recreate:
				//UpdateCentralConstraint(gThorax, gThorax.Vertical, gThorax.NoTurnAround, true);
				break;

			//нормально не стоим - попытаться сначала встать
			case EClung::BadAngle:
				ActionFindStart(ECreatureAction::SELF_WRIGGLE1);
				break;

			//нет нормальной опоры - попытаться встать на дыбы передом или задом (это больше для прочих дел)
			case EClung::BadSurface:
			case EClung::NoClimbableSurface:
				if (Mesh->Erection() < 0.7)
					ActionFindStart(ECreatureAction::SELF_PRANCE1);
				else ActionFindStart(ECreatureAction::SELF_PRANCE_BACK1);
		}
	}
}


//==============================================================================================================
//разовый отъём запаса сил - вызывается при силоёмких действиях / или восполнение запаса сил - в редком тике
//==============================================================================================================
void AMyrPhyCreature::StaminaChange(float delta)
{
	//если дельта больше нуля предполагается восстановление стамины
	//однако этому восстановлению мешает эмоциональное напряжение
	if (delta > 0)
	{	
		//сонность мягко ухудшает восстановление сил
		delta *= (1 - Sleepiness);

		//злоба и страх также отрубают восстановление сил
		delta -= (MyrAI()->IntegralEmotionFear*0.4 + MyrAI()->IntegralEmotionRage*0.2) * Phenotype.StaminaCurseByEmotion();

		//сырость также снижает
		if(Daemon) delta -= Daemon->Wetness;

		//но всё же не отнимают сверх меры
		if (delta < 0) delta = 0;
	}

	//внимание! пищевая энергия расходуется только при восстановлении запаса сил и при попытке отнять силы, когда их уже нет
	Stamina += delta;								// прирастить запас сил на дельту (или отнять), вычисленную извне

	if (Stamina < 0)								// сил и так нет, а от нас ещё требуют
	{	Stamina = 0;								// восстановить ноль - сил как не было, так и нет
		Energy -= delta * 0.02f;					// а вот расход пищевой энергии всё равно происходит, да ещё сильнее
	}
	else											// запас сил имеется (или нашёлся)
	{
		if (Stamina > 1.0f) Stamina = 1.0f;			// запас сил и так восстановлен, дальше некуда
		else if (delta > 0)							// если же есть куда восстанавливать запас сил (происходит именно восстановление, не расход)...
			Energy -= delta * 0.01f;				// запас сил восстанавливается за счёт расхода пищевой энергии
	}
}

//==============================================================================================================
//после перемещения на ветку - здесь выравнивается камера
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
		if(Sleepiness>0.5) dH -= Sleepiness - 0.5;		// сонность
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
	if (Health <= 0) return;

	//влияние содержимого желудка на здоровье
	if(Stomach.Time > 0)
	{	Health += Stomach.DeltaHealth * DeltaTime;
		Stamina += Stomach.DeltaStamina * DeltaTime;
		Energy += Stomach.DeltaEnergy * DeltaTime;
		Stomach.Time -= DeltaTime;
	}

	//учёт воздействия поверхности, к которой мы прикасаемся любой частью тела, на здоровье
	EMyrSurface CurSu = EMyrSurface::Surface_0;
	FSurfaceInfluence* SurfIn = nullptr;
	float SurfaceHealthModifier = 0.0f;
	for (int i = 0; i < (int)ELimb::NOLIMB; i++)
	{	auto& Limb = Mesh->GetLimb((ELimb)i);
		if(Limb.Stepped)
		{	
			//для избегания поиска того, что уже найдено на предыдущем шаге
			if(Limb.Surface != CurSu)
			{	SurfIn = GetMyrGameInst()->Surfaces.Find(Limb.Surface);
				CurSu = Limb.Surface;
			}
			//в сборке данных по поверхности указана дельта урона, поэтому с минусмом
			if(SurfIn) SurfaceHealthModifier -= SurfIn->HealthDamage;
		}
	}

	//восстановление здоровья (стамина перенесена в основной тик, чтоб реагировать на быструю смену фаз атаки)
	HealthChange ((Mesh->DynModel->HealthAdd + SurfaceHealthModifier ) * DeltaTime	);	

	//пока неясно, как нормально выставлять приращение сонности
	Sleepiness += 0.001;

	//пересчитать здоровье и мобильность тела
	UpdateMobility();

	//возраст здесь, а не в основном тике, ибо по накоплению большого числа мелкие приращения могут быть меньше разрядной сетки
	Age += DeltaTime;

}

//==============================================================================================================
//найти объект в фокусе - для демона, просто ретранслятор из ИИ, чтобы демон не видел ИИ
//==============================================================================================================
int AMyrPhyCreature::FindObjectInFocus(const float Devia2D, const float Devia3D, AActor*& Result, FText& ResultName)
{
	//на уровне ИИ для простоты цель ищется по отдельности для каждой ячаейки
	if (!MyrAI()) return 0;
	auto R = MyrAI()->FindGoalInView (AttackDirection, Devia2D, Devia3D, false, Result);
	if(!R) R = MyrAI()->FindGoalInView (AttackDirection, Devia2D, Devia3D, true, Result);
	if (R == 1) ResultName = ((AMyrPhyCreature*)(Result))->HumanReadableName; else
	if (R == 2) ResultName = ((AMyrArtefact*)(Result))->HumanReadableName; 
	else ResultName = FText();
	return R;
}


//==============================================================================================================
//полный спектр действий от (достаточно сильного) удара между этим существом и некой поверхностью
//==============================================================================================================
void AMyrPhyCreature::Hurt(float Amount, FVector ExactHitLoc, FVector Normal, EMyrSurface ExactSurface, FSurfaceInfluence* ExactSurfInfo)
{
	//для простоты вводится нормаль, а уже тут переводится в кватернион, для позиционирования источника частиц
	UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s Hurt %g"), *GetName(), Amount);
	FQuat ExactHitRot = FRotationMatrix::MakeFromX(Normal).ToQuat();

	//если выполняется самодействие или действие-период, но указано его отменять при ударе
	CeaseActionsOnHit();

	//вскрикнуть (добавить парметры голоса)
	MakeVoice(GenePool->SoundAtPain, FMath::Min(Amount, 1.0f) * 4, true);

	//◘ прозвучать в ИИ, чтобы другие существа могли слышать
	MyrAI()->NotifyNoise(ExactHitLoc, Amount);

	//сборка данных по поверхности, которая осприкоснулась с этим существом
	if (!ExactSurfInfo) return;

	//◘здесь генерируется всплеск пыли от шага - сразу два! - касание и отпуск, а чо?!
	auto DustAt = ExactSurfInfo->ImpactVisualHit; if (DustAt) SurfaceBurst (DustAt, ExactHitLoc, ExactHitRot, GetBodyLength(), Amount);
	DustAt = ExactSurfInfo->ImpactVisualRaise; 	  if (DustAt) SurfaceBurst (DustAt, ExactHitLoc, ExactHitRot, GetBodyLength(), Amount);
}

//==============================================================================================================
//ментально осознать, что пострадали от действий другого существа
//==============================================================================================================
void AMyrPhyCreature::SufferFromEnemy(float Amount, AMyrPhyCreature* Motherfucker)
{
	//если выполняется самодействие или действие-период, но указано его отменять при ударе
	CeaseActionsOnHit();

	//для осознания нашей связи с противником прогнать его через наш ИИ и уже оттуда вызвать факт геймплейного события
	MyrAI()->DesideHowToReactOnAggression(Amount, Motherfucker);
	UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s SufferFromEnemy by %s for %g"), *GetName(), *Motherfucker->GetName(), Amount);
}
void AMyrPhyCreature::EnjoyAGrace(float Amount, AMyrPhyCreature* Sweetheart)
{	MyrAI()->DesideHowToReactOnGrace(Amount, Sweetheart);
	UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s EnjoyAGrace from %s for %g"), *GetName(), *Sweetheart->GetName(), Amount);
}

//==============================================================================================================
//прервать или запустить прерывание деймтвий, для которых сказано прерывать при сильном касании
//==============================================================================================================
void AMyrPhyCreature::CeaseActionsOnHit()
{
	//если выполняется самодействие или действие-период, но указано его отменять при ударе
	if (!NoRelaxAction())
		if (GetRelaxAction()->CeaseAtHit)
			RelaxActionStartGetOut();
	if(!NoSelfAction())
		if(GetSelfAction()->CeaseAtHit)
			SelfActionCease();
	if (!NoAttack())
	{	if (GetAttackAction()->CeaseAtHit)
			NewAttackEnd();
	}
}



//==============================================================================================================
//сделать шаг - 4 эффекта: реальный звук, ИИ-сигнал, облачко пыли, декал-след
//==============================================================================================================
void AMyrPhyCreature::MakeStep(ELimb eLimb, bool Raise)
{
	//если колесо ноги не стоит на полу, то и шага нет (это неточно, нужно трассировать)
	FLimb* Limb = &Mesh->GetLimb(eLimb);

	//пояс
	auto Girdle = GetGirdle(eLimb);

	//если для конечности не заведен физ-членик, вероятно существо безногое, и надо брать данные из родителя-туловища
	if (!Mesh->HasPhy(eLimb)) Limb = &Mesh->GetLimbParent(eLimb);

	//а вот если физически нога или туловище не касается поверхности, шаг нельзязасчитывать
	if(!Limb->Stepped) return;

	//тип поверхности - это не точно, так как не нога а колесо - пока неясно, делать трассировку в пол или нет
	EMyrSurface ExplicitSurface = Limb->Surface;
	auto Trans = Mesh->GetSocketTransform(Mesh->FleshGene(eLimb)->GrabSocket, RTS_World);

	//точное место шага - пока не ясно, стоит ли делать трассировку
	FVector ExactStepLoc = Trans.GetLocation();

	//сила шага, ее надо сложно рассчитывать 
	float StepForce = FMath::Min(Mesh->GetMachineBody(*Limb)->GetBodyMass(), 2.0f);


	//поворот для декала - тут все просто
	FQuat ExactStepRot = Trans.GetRotation();

	//поворот для источника частиц - вот тут нужен вектор скорости в направлении нормали
	FQuat ExactBurstRot = ExactStepRot;

	//расчёт скорости движущегося кончика ноги в точке шага
	float VelScalar; FVector VelDir;

	//если к кончику лапы приделан членик, то берем его, иначе (для мышей) через общую скорость
	auto FootBody = Mesh->HasFlesh(eLimb) ? Mesh->GetFleshBody(*Limb, 0) : Girdle->GetBody(EGirdleRay::Center);
	FVector Vel = FootBody->GetUnrealWorldVelocity();
	if (Limb->Floor) Vel -= Limb->Floor->GetUnrealWorldVelocity();
	if ((Vel | Limb->ImpactNormal) < 0) Vel = -Vel;
	Vel.ToDirectionAndLength(VelDir, VelScalar);

	//в качестве нормы берем скорость ходьбы этого существа, она должна быть у всех, хотя, если честно, тут уместнее твердая константа
	StepForce *= VelScalar / GenePool->BehaveStates[EBehaveState::walk]->MaxVelocity;

	//если слабый шаг, то брызги больше вверх, если шаг сильнее, то всё больше в сторону движения 
	ExactBurstRot = FMath::Lerp(
		FRotationMatrix::MakeFromX(Limb->ImpactNormal).ToQuat(),
		FRotationMatrix::MakeFromX(VelDir).ToQuat(),
		FMath::Min(StepForce, 1.0f));
	//DrawDebugLine(GetWorld(), ExactStepLoc, ExactStepLoc + Vel*0.1,	FColor(Raise?255:0, StepForce*255, 0), false, 2, 100, 1);


	//выдрать параметры поверхности из глобального контейнера (индекс хранится здесь, в компоненте)
	if(auto StepSurfInfo = GetMyrGameInst()->Surfaces.Find (ExplicitSurface))
	{
		//◘ вызвать звук по месту со всеми параметрами
		SoundOfImpact (StepSurfInfo, ExplicitSurface, ExactStepLoc,
			0.5f  + 0.05 * Mesh->GetMass()  + 0.001* Girdle->SpeedAlongFront(),
			1.0f - Limb->ImpactNormal.Z,
			Raise ? EBodyImpact::StepRaise : EBodyImpact::StepHit);

		//◘ прозвучать в ИИ, чтобы другие существа могли слышать
		MyrAI()->NotifyNoise(ExactStepLoc, StepForce);

		//только на максимальной детализации = минимальном расстоянии, для оптимизации 
		if (Mesh->GetPredictedLODLevel() == 0)
		{

			//◘здесь генерируется всплеск пыли от шага
			auto DustAt = Raise ? StepSurfInfo->ImpactVisualRaise : StepSurfInfo->ImpactVisualHit;
			if (DustAt)
			{
				SurfaceBurst(DustAt, ExactStepLoc, ExactStepRot, GetBodyLength(), StepForce);
			}

			//отпечаток ноги виден только после поднятия ноги
			if (Raise)
			{
				//отпечатки ног - они уже не могут быть свойствами поверхности как таковой, они - свойства существа
				if (auto DecalMat = GetGenePool()->FeetDecals.Find(ExplicitSurface))
				{
					//выворот плоскости текстуры следа
					FQuat offsetRot(FRotator(-90.0f, 0.0f, 0.0f));
					FRotator Rotation = (ExactStepRot * offsetRot).Rotator();

					//◘ генерация компонента следа
					auto Decal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), *DecalMat, FVector(20, 20, 20), ExactStepLoc, Rotation, 10);
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
			GetMyrGameMode()->SoundAttenuation, nullptr, true);

		//установить параметры скорости (влияет на громкость) и вертикальности (ступание-крабканье-скольжение)
		if (Noiser)
		{
			Noiser->SetIntParameter(TEXT("Impact"), (int)ImpactType);
			Noiser->SetFloatParameter(TEXT("Velocity"), Strength);
			Noiser->SetFloatParameter(TEXT("Vertical"), Vertical);
		}
		else UE_LOG(LogMyrPhyCreature, Error, TEXT("%s SoundOfImpact WTF no noiser type:%s, surf:%s"), *GetName(), *TXTENUM(EBodyImpact, ImpactType), *TXTENUM(EMyrSurface, Surface));
	}
	UE_LOG(LogMyrPhyCreature, Log, TEXT("%s SoundOfImpact type:%s, surf:%s"), *GetName(), *TXTENUM(EBodyImpact,ImpactType), *TXTENUM(EMyrSurface, Surface));
}

//==============================================================================================================
//запустить брызг частиц пыли и т.п. из чего состоит заданная поверхность
// здесь, возможно, ещё задавать цвет, чтобы, например, черный кот не брызгал белой шерстью
//==============================================================================================================
void AMyrPhyCreature::SurfaceBurst(UParticleSystem* Dust, FVector ExactHitLoc, FQuat ExactHitRot, float BodySize, float Scale)
{
	//◘ создать ad-hoc компонент источник и тут же запустить на нем найденную сборку частиц
	auto ParticleSystem = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Dust,
		ExactHitLoc, FRotator(ExactHitRot), FVector(1, 1, 1),
		true, EPSCPoolMethod::AutoRelease, true);

	//чтобы частицы травы также приминали саму траву
	ParticleSystem->SetRenderCustomDepth(true);

	//параметр, определяемый размерами тела - размер спрайтов (хорошо бы еще размер области генерации)
	ParticleSystem->SetVectorParameter(TEXT("BodySize"), FVector(BodySize));

	//параметры силы удара - непрозрачность пыли, количество пылинок
	ParticleSystem->SetFloatParameter(TEXT("Amount"), Scale);
	ParticleSystem->SetFloatParameter(TEXT("Velocity"), Scale);
	ParticleSystem->SetFloatParameter(TEXT("FragmentStartAlpha"), Scale);
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
bool AMyrPhyCreature::AdoptBehaveState(EBehaveState NewState)
{
	//если мы уже в запрашиваеом состоянии - быстрый выход, сообщить ложь, чтобы вовне не прерывались
	if (CurrentState == NewState) return false;

	//если "карточки" данных для такого состояния в генофонде нет, то нельзя переходить
	auto BehaveNewData = GetGenePool()->BehaveStates.Find(NewState);
	if (!BehaveNewData)	return false;

	//нельзя принимать новое состояние, если текущая скорость меньше минимальной, указанной
	if ((*BehaveNewData)->MinVelocityToAdopt > 0)
	if (FMath::Abs(Thorax->SpeedAlongFront()) < (*BehaveNewData)->MinVelocityToAdopt)
	if (FMath::Abs(Pelvis->SpeedAlongFront()) < (*BehaveNewData)->MinVelocityToAdopt)
	{	UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s AdoptBehaveState %s cancelled by min velocity"), *GetName(), *TXTENUM(EBehaveState, NewState));
		return false;
	}

	//сбросить счетчик секунд на состояние
	StateTime = 0.0f;
	UE_LOG(LogMyrPhyCreature, Log, TEXT("%s AdoptBehaveState  + (%s >> %s)"), *GetName(),
		*TXTENUM(EBehaveState, CurrentState),
		*TXTENUM(EBehaveState, NewState)	);

	//если включено расслабить всё тело
	if((*BehaveNewData)->MakeAllBodyLax != BehaveCurrentData->MakeAllBodyLax)
	Mesh->SetFullBodyLax((*BehaveNewData)->MakeAllBodyLax);

	//пока для теста, вообще неясно - на этот раз увязчаем корень тела, чтоб не болталось при лазаньи
	if (NewState == EBehaveState::climb) Mesh->GetRootBody()->LinearDamping = Mesh->GetRootBody()->AngularDamping = 100;
	else Mesh->GetRootBody()->LinearDamping = Mesh->GetRootBody()->AngularDamping = 0;
	Mesh->GetRootBody()->UpdateDampingProperties();


	//счастливый финал
	CurrentState = NewState;
	BehaveCurrentData = *BehaveNewData;

	//обновить скорость djccnfyj
	Mesh->HealRate = BehaveCurrentData->HealRate;

	//урон физическими ударами может быть включен или отключен
	Mesh->InjureAtHit = BehaveCurrentData->HurtAtImpacts;

	//если самодействие может только в отдельных состояниях и новое состояние в таковые не входит - тут же запустить выход
	if (!NoRelaxAction())
	{	if (GetRelaxAction()->Condition.States.Contains(NewState) == GetRelaxAction()->Condition.StatesForbidden)
			RelaxActionStartGetOut();
	}


	//применить новые настройки динамики движения/поддержки для частей тела поясов
	//ВНИМАНИЕ, только когда нет атаки, в атаках свои дин-модели по фазам, если применить, эти фазы не отработают
	if(NoAttack())
		AdoptWholeBodyDynamicsModel(&BehaveCurrentData->WholeBodyDynamicsModel, true);

	//если управляется демоном игрока
	if (Daemon)
	{
		//установить эффекты
		Daemon->SetTimeDilation(BehaveCurrentData->TimeDilation);
		Daemon->SetMotionBlur(BehaveCurrentData->MotionBlurAmount);
	}
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
		//шаг по поверхности
		case EBehaveState::stand:
			if (BewareDying()) break;
			BEWARE(GotUntouched(), fall);
			BEWARE(ExternalGain>0, walk);
			BEWARE(bCrouch, crouch);
			BEWARE(bSoar && Stamina > 0.1f, soar);
			BEWAREELSE(bRun && Stamina > 0.1f, run, walk);
			break;

		//шаг по поверхности (боимся упасть, опрокинуться и сбавить скорость до стояния)
		case EBehaveState::walk:
			if(BewareDying()) break;
			BEWARE(GotUntouched(), fall);
			BEWARE(ExternalGain==0 && GotSlow(), stand);
			BEWARE(Stuck > 0.5 || Thorax->GuidedMoveDir.Z>0.7, mount);
			BEWARE(bCrouch, crouch);
			BEWARE(bSoar && Stamina > 0.1f, soar);
			if(BewareOverturn(0.5f - 0.2* MoveGain)) break;
			BEWAREELSE(bRun && Stamina > 0.1f, run, walk);
			BEWARE(bClimb && !GotUnclung(), climb);
			break;

		//шаг украдкой по поверхности
		case EBehaveState::crouch:
			if (BewareDying()) break;
			BEWARE(GotUntouched(), fall);
			BEWARE(!bCrouch, walk);
			BEWARE(bSoar && Stamina > 0.1f, soar);
			BEWAREELSE(bRun && Stamina > 0.1f, run, walk);
			BEWARE(bClimb && !GotUnclung(), climb);
			break;

		//бег по поверхности
		case EBehaveState::run:
			if (BewareDying()) break;
			BEWARE(GotUntouched(), fall);
			BEWARE(GotSlow(10), walk);
			BEWARE(Stuck > 0.7 || Thorax->GuidedMoveDir.Z > 0.8, mount);
			if (BewareOverturn (0.5f - 0.2 * MoveGain)) break;
			BEWARE(!bRun, walk);
			BEWARE(bClimb && !GotUnclung(), climb);
			break;

		//забраться на уступ
		case EBehaveState::mount:
			if (BewareDying()) break;
			BEWARE(GotUntouched(), fall);
			BEWARE(Stuck < 0.5 && Pelvis->GuidedMoveDir.Z<0.65, walk);	//порог выхода меньше, чтоб убрать дребезг
			BEWAREELSE(bRun && Stamina > 0.1f, run, walk);
			BEWARE(bClimb && !GotUnclung(), climb);
			break;

		//лазанье по проивольно крутым поверхностям
		case EBehaveState::climb:
			if (BewareDying()) break;
			//if (BewareHangTop()) break;
			BEWARE(bSoar && Stamina > 0.1f, soar);
			BEWARE(GotUntouched(), fall);
			BEWARE(GotUnclung(), fall);
			break;

		//набирание высоты (никаких волевых переходов, только ждать земли и перегиба скорости)
		case EBehaveState::soar:
			if(BewareDying()) break;
			BEWARE(!bSoar, fly);
			BewareGround();
			break;
			
		//падение (никаких волевых переходов, только ждать земли)
		case EBehaveState::fall:
			if(BewareDying()) break;
			BEWARE(bSoar, soar);
			BEWARE(bFly, fly);
			BEWARE(StateTime>0.5, land);
			BewareGround();
			break;

		//специальное долгое падение, которое включается после некоторрого времени случайного падения
		//на него вешается слоумо для выворота тела в полете
		case EBehaveState::land:
			if (BewareDying()) break;
			BEWARE(bSoar, soar);
			BEWARE(bFly, fly);
			BewareGround();
			break;

		//режим полёта
		case EBehaveState::fly:
			if(BewareDying()) break;
			BEWARE(!bFly || Stamina*Health<0.1 || ExternalGain==0, fall);
			BEWARE(bSoar, soar);
			BEWARE(!GotUntouched(), walk);
			break;

		//опрокидывание (пассивное лежание (выход через действия а не состояния) и активный кувырок помимо воли)
		case EBehaveState::lie:
		case EBehaveState::tumble:
			if(BewareDying()) break;
			BEWARE(GotUntouched(), fall);
			BewareOverturn(0.6,false);	// изменение tumble <-> lie <-> walk
			break;
	
		//здоровье достигло нуля, просто по рэгдоллиться пару секнуд перед окончательной смертью
		case EBehaveState::dying:
			BEWARE(StateTime>2, dead);
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
//проверить переворот (берётся порог значения Z вектора вверх спины)
//==============================================================================================================
bool AMyrPhyCreature::BewareOverturn(float Thresh, bool NowWalking)
{
	//спина смотрит вниз - уже неправильно
	if (Mesh->IsSpineDown(Thresh))		
	{	
		//если спина вниз, а вот ноги хорошо стоят - анаомальная раскоряка мостиком, надо срочно исправлять состоянием Tumble
		if (Thorax->StandHardness > 0.8 || Pelvis->StandHardness > 0.8)	
			return AdoptBehaveState(EBehaveState::tumble);

		//спина вниз, и лапы в воздухе - для игрока обычное лежание, пусть сам выкарабкивается
		else if(Daemon) return AdoptBehaveState(EBehaveState::lie);	

		//но для NPC в нем нет смысла, надо быстрее встать
		else return AdoptBehaveState(EBehaveState::tumble);	
		
	}
	//если функция вызывается из наземного состояния (по умолчанию) - то оно и остаётся
	else if(NowWalking) return false;

	//если функция вызывается из tumble - эта веть конец лежания и переход в состояние по умолчанию
	else return AdoptBehaveState(EBehaveState::walk);
}

//==============================================================================================================
//распознавание натыкания на стену с целью подняться
//==============================================================================================================
bool AMyrPhyCreature::BewareStuck(float Thresh)
{
	if(Stuck > Thresh)
		return AdoptBehaveState(EBehaveState::mount);
	return false;
}

//==============================================================================================================
//условия перехода в смерть
//==============================================================================================================
bool AMyrPhyCreature::BewareDying()
{
	//при замечании нулевого здоровья перейти в режим Эпоперек стиксаЭ
	//в котором вызвать конвульсии и посигналить ИИ что сдох
	if (Health <= 0)
	{	AdoptBehaveState(EBehaveState::dying);
		ActionFindStart(ECreatureAction::SELF_DYING1);
		MyrAI()->LetOthersNotice(EHowSensed::DIED);
		Health = 0.0f;
		return true;
	}
	return false;
}

//==============================================================================================================
//начать атаку (внимание, проверок нет, применимость атаки должна проверяться заранее)
//==============================================================================================================
EAttackAttemptResult AMyrPhyCreature::NewAttackStart(int SlotNum, int VictimType)
{
	//в генофонде ни одна атака не заполнена на кнопку, из которой вызвалась эта функция
	if (SlotNum == 255) return EAttackAttemptResult::WRONG_PHASE;//◘◘>;

	//даже если атака не начнётся (а она в этом случае обязательно не начнётся)
	//но в ней прописано, что в начальной фазе надо съесть то, что мы держим
	if (GenePool->Actions[SlotNum]->VictimTypes[0].EatHeldPhase == EActionPhase::ASCEND)
		if (Eat()) return EAttackAttemptResult::ATE_INSTEAD_OF_RUNNING;//◘◘> 

	//уже выполняется какая-то атака, нельзя просто так взять и прервать её
	if (CurrentAttack != 255) return EAttackAttemptResult::ALREADY_RUNNING;//◘◘>

	//полная связка настроек
	auto Action = GenePool->Actions[SlotNum];
	auto VictimMod = Action->VictimTypes[VictimType];

	//отправить просто звук атаки, чтобы можно было разбегаться (кстати, ввести эту возможность)
	MyrAI()->LetOthersNotice(EHowSensed::ATTACKSTART);

	//врубить атаку текущей
	CurrentAttack = SlotNum;
	CurrentAttackVictimType = VictimType;
	NewAdoptAttackPhase(EActionPhase::ASCEND);

	//для игрока
	if (Daemon)
	{
		//если ИИ детектирует рядом объект точного нацеливания, точно нацелиться на него
		if (MyrAI()->AimBetter(AttackDirection, 0.5)) Daemon->UseActDirFromAI = true;

		//если эта фаза такаи подразумевает эффектное размытие резкого движения, то врубить его
		Daemon->SetMotionBlur(Action->MotionBlurBeginToStrike);
	}
	//для непися, которого мы имеем в целях ИИ = значит боремся или просто стоим около, тогда спецеффекты его атак также должны влиять на нас
	else if(GetMyrGameMode()->GetProtagonist()->MyrAI()->FindAmongGoals(GetMesh()))
	{
		//если атака требует размытия, то в любом случае находим протагониста и рамываем у него
		GetMyrGameMode()->Protagonist->SetMotionBlur(Action->MotionBlurBeginToStrike);
	}

	//настроить физику членов в соответствии с фазой атаки (UpdateMobility внутри)
	AdoptWholeBodyDynamicsModel(&Action->DynModelsPerPhase[(int)CurrentAttackPhase], true);

	//обнаружить, что мы пересекаем умный объём и передать ему, что мы совершаем, возможно, судьбоносное действие
	if (VictimMod.SendApprovalToTriggerOverlapperOnStart)
		SendApprovalToOverlapper();

	//здесь фиксируется только субъективный факт атаки, применения силы, а не результат
	if (VictimMod.TactileDamage>0)
		 CatchMyrLogicEvent ( EMyrLogicEvent::SelfHarmBegin,  VictimMod.TactileDamage, nullptr);
	else CatchMyrLogicEvent ( EMyrLogicEvent::SelfGraceBegin,-VictimMod.TactileDamage, nullptr);
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s NewAttackStart %s"), *GetName(), *Action->GetName());
	return EAttackAttemptResult::STARTED;//◘◘>
}

//==============================================================================================================
// вызвать переход атаки из подготовки в удар (внимание, проверок нет, применимость атаки должна проверяться заранее)
//==============================================================================================================
EAttackAttemptResult AMyrPhyCreature::NewAttackStrike()
{
	//если просят применить атаку, а ее нет
	if (CurrentAttack == 255) return EAttackAttemptResult::WRONG_PHASE;//◘◘>

	//уведомить мозги и через них ноосферу о свершении в мире атаки
	if (MyrAI()) MyrAI()->LetOthersNotice(EHowSensed::ATTACKSTRIKE);

	//полная связка настроек
	auto Action = GenePool->Actions[CurrentAttack];
	auto OnVictim = Action->VictimTypes[CurrentAttackVictimType];

	//в зависимости от того, в какой фазе вызвана просьба свершить действие
	switch (CurrentAttackPhase)
	{
		//подготовка еще не завершена, отправить на ожидание
		case EActionPhase::ASCEND:
			NewAdoptAttackPhase(EActionPhase::RUSH);
			return EAttackAttemptResult::RUSHED_TO_STRIKE;//◘◘>

		//подготовка завершена, можно вдарить
		case EActionPhase::READY:
			return NewAttackStrikePerform();//№№№◘◘>

		//переудар поверх удара - пока есть бюджет повторов, отправить вспять, чтобы ударить повторно
		case EActionPhase::STRIKE:
			if (OnVictim.NumInstantRepeats > CurrentAttackRepeat)
			{	NewAdoptAttackPhase(EActionPhase::DESCEND);
				return EAttackAttemptResult::GONE_TO_STRIKE_AGAIN;//◘◘>
			}

		//остальные фазы не приводят к смене
		case EActionPhase::RUSH:
		case EActionPhase::DESCEND:
			return EAttackAttemptResult::WRONG_PHASE;//◘◘>
	}
	//по умолчанию непонятно отчего не получилось
	return EAttackAttemptResult::FAILED_TO_STRIKE;//◘◘>
}

//==============================================================================================================
//непосредственное применение атаки - переход в активную фазу
//==============================================================================================================
EAttackAttemptResult AMyrPhyCreature::NewAttackStrikePerform()
{
	//полная связка настроек
	auto Action = GenePool->Actions[CurrentAttack];
	auto OnVictim = Action->VictimTypes[CurrentAttackVictimType];

	//перевести в фазу боя (внимание, если атака-прыжок, то подбрасывание тела здесь, внутри)
	NewAdoptAttackPhase(EActionPhase::STRIKE);

	//для игрока, не ИИ
	if (Daemon)
	{
		//если эта фаза такаи подразумевает эффектное размытие резкого движения, то врубить его
		Daemon->SetMotionBlur ( Action -> MotionBlurStrikeToEnd);

		//если ранее не вставал вопрос о помощи ИИ в нацеливании, попробовать снова
		if(!Daemon->UseActDirFromAI)
			if(MyrAI()->AimBetter(AttackDirection, 0.5))
				Daemon->UseActDirFromAI = true;
	}
	//для непися, которого мы имеем в целях ИИ = значит боремся или просто стоим около, тогда спецеффекты его атак также должны влиять на нас
	else if (GetMyrGameMode()->GetProtagonist()->MyrAI()->FindAmongGoals(GetMesh()))
	{
		//если атака требует размытия, то в любом случае находим протагониста и рамываем у него
		GetMyrGameMode()->Protagonist->SetMotionBlur(Action->MotionBlurBeginToStrike);
	}


	//обнаружить, что мы пересекаем умный объём и передать ему, что мы совершаем, возможно, судьбоносное действие
	if (OnVictim.SendApprovalToTriggerOverlapperOnStrike)	SendApprovalToOverlapper();

	//диагностика
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s NewAttackStrikePerform %d"), *GetName(), CurrentAttack);
	return EAttackAttemptResult::STROKE;//◘◘>
}

//==============================================================================================================
//переключить фазу атаки со всеми сопутстивующими
//==============================================================================================================
void AMyrPhyCreature::NewAdoptAttackPhase(EActionPhase NewPhase)
{
	//присвоить
	CurrentAttackPhase = NewPhase;

	//полная связка настроек
	auto Action = GenePool->Actions[CurrentAttack];
	auto& OnVictim = Action->VictimTypes[CurrentAttackVictimType];

	//не уровне меша принять приготовления атакующих / опасных частей тела
	for (ELimb li = ELimb::PELVIS; li != ELimb::NOLIMB; li = (ELimb)((int)li+1))
	{
		//для текущей новой фазы определить, вписана ли текущая часть тела как боевая или нет
		const bool AtOn = IsLimbAttacking(li);

		//если имеется реальный двигательный членик у этой части тела
		if (Mesh->HasPhy(li))
		{
			//хз насколько поможет
			Mesh->GetMachineBody(li)->SetUseCCD(AtOn);
		}
		//если имеется реальный кинематический / навесной мясной членик
		if (Mesh->HasFlesh(li))
		{
			//хз насколько поможет
			Mesh->GetFleshBody(li)->SetUseCCD(AtOn);
		}
	}

	//атака "пригнуться и прыгнуть" - используется аккумулятор мощи
	//взводится до "чуть-чуть не нуль", чтобы число также содержало логику вкл/выкл
	if(OnVictim.JumpHoldPhase == CurrentAttackPhase) AttackForceAccum = 0.1;

	//текущая фаза - прыгнуть
	else if(OnVictim.JumpPhase == CurrentAttackPhase) JumpAsAttack();

	//текущая новая фаза - отцепить то, что зацеплено было
	else if(OnVictim.UngrabPhase == CurrentAttackPhase) UnGrab();

	//настроить физику членов в соответствии с фазой атаки (UpdateMobility внутри)
	AdoptWholeBodyDynamicsModel(&Action->DynModelsPerPhase[(int)CurrentAttackPhase], Action->UseDynModels);

}

//==============================================================================================================
//реакция на достижение закладки GetReady в анимации любым из путей - вызывается только в этой самой закладке
//==============================================================================================================
void AMyrPhyCreature::NewAttackGetReady()
{
	//идём штатно с начала атаки - сменить режим
	if( CurrentAttackPhase == EActionPhase::ASCEND)
		NewAdoptAttackPhase(EActionPhase::READY);

	//если вернулись в точку принятия решения вспять, на режиме DESCEND, это мы хотим немедленно повторить атаку
	else if(CurrentAttackPhase == EActionPhase::DESCEND)
	{
		//только если данный тип атаки позволяет делать дополнительные повторы в требуемом количестве
		//иначе так и остаться на режиме DESCEND до самого края анимации
		if(GetAttackActionVictim().NumInstantRepeats > CurrentAttackRepeat)
		{	NewAttackStrikePerform();
			CurrentAttackRepeat++;
		}
	}

	//если атака одобрена заранее, мы домчались до порога и просто реализуем атаку
	else if (CurrentAttackPhase == EActionPhase::RUSH) NewAttackStrikePerform ();
}

//==============================================================================================================
//полностью завершить атаку
//==============================================================================================================
void AMyrPhyCreature::NewAttackEnd()
{
	//если ваще нет атаки, то какого оно вызвалось?
	if(NoAttack())
	{	UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s NewAttackEnd WTF no attack"), *GetName());
		return;	}

	//что бы то ни было, а накопитель силы сбросить
	AttackForceAccum = 0.0f;

	//аварийный сброс всех удерживаемых предметов, буде таковые имеются
	UnGrab();

	//возвратить режимноуровневые настройки динамики для поясов конечностей
	//тут некорректно, если выполняется еще и самодействие, но пуская пока так
	AdoptWholeBodyDynamicsModel(&BehaveCurrentData->WholeBodyDynamicsModel, true);

	//если атака сопровождалась эффектами размытия в движении, сейчас она по любому законсилась и надо вернуть старое состояниеспецифичное значение
	if (Daemon)
	{	Daemon->SetMotionBlur(BehaveCurrentData->MotionBlurAmount);
		Daemon->SetTimeDilation(BehaveCurrentData->TimeDilation);
	}

	//финально сбросить все переменные режима
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s NewAttackEnd %s "), *GetName(), *GetCurrentAttackName());
	CurrentAttackVictimType = 0;
	CurrentAttackPhase = EActionPhase::NONE;
	CurrentAttack = 255;
	CurrentAttackRepeat = 0;

}

//==============================================================================================================
//мягко остановить атаку, или исчезнуть, или повернуть всяпять - хз где теперь используется
//==============================================================================================================
void AMyrPhyCreature::NewAttackGoBack()
{
	if(NoAttack()){	UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s NewAttackGoBack WTF no attack"), *GetName());	return;	}

	//если финальный этап, то просто исчезнуть атаку
	if(CurrentAttackPhase == EActionPhase::FINISH)
	{	NewAttackEnd(); return; }

	//отход назад
	else NewAdoptAttackPhase(EActionPhase::DESCEND);
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s NewAttackGoBack %s "), *GetName(), *GetCurrentAttackName());

}
//==============================================================================================================
// атаки - отменить начинаемое действие, если еще можно
// для ИИ - чтобы прерывать начатую атаку, например, при уходе цели из виду - в любое время до фазы STRIKE (ASCEND, READY, RUSH)
// для игрока - по достижении закладки в ролике анимации, когда игрок зажал кнопку, но долго думает с отпусканием (READY)
//==============================================================================================================
bool AMyrPhyCreature::NewAttackNotifyGiveUp()
{
	//смысл точки - решение здаться, когда истекает ожидание (READY), в остальных фазах закладка не имеет смысла
	if(CurrentAttackPhase != EActionPhase::READY) return false;
	NewAdoptAttackPhase(EActionPhase::DESCEND);
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s NewAttackNotifyGiveUp %d "), *GetName(), (int)CurrentAttack);
	return true;
}

//==============================================================================================================
//когда между активно фазой и концом есть еще период безопасного закругления, и метка этой фазытолько получена
//==============================================================================================================
void AMyrPhyCreature::NewAttackNotifyFinish()
{
	//если до этого атака была в активной фазе, то
	if(CurrentAttackPhase == EActionPhase::STRIKE)
	{
		//если в атаке использовалось автонацеливание, завершить его, полностью передать управление игроку
		if(Daemon)
			if(Daemon->UseActDirFromAI)
				Daemon->UseActDirFromAI = false;

		//перевести ее в фазу финального закругления
		NewAdoptAttackPhase(EActionPhase::FINISH);
	}
}

//==============================================================================================================
//достижение финальной закладки
//==============================================================================================================
void AMyrPhyCreature::NewAttackNotifyEnd()
{
	//приезжаем к закладке "конец ролика", небудучи в правильном состоянии
	if (CurrentAttackPhase != EActionPhase::STRIKE && CurrentAttackPhase != EActionPhase::FINISH)
		UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s AttackNotifyEnd %s WTF wrong state, phase = %s"),
		*GetName(), *GetCurrentAttackName(), *TXTENUM(EActionPhase, CurrentAttackPhase));
	NewAttackEnd();
}


//==============================================================================================================
//общая процедура для всех видов прыжка, корректно закрывающая предыдущее состояние
//==============================================================================================================
bool AMyrPhyCreature::JumpAsAttack()
{
	//исключить совсем уж недобор мощи, чтобы были всё же прыжки, а не дёрганья
	if (AttackForceAccum == 0.0f) AttackForceAccum = 1.0f;
	if (AttackForceAccum < 0.5f) AttackForceAccum = 0.5f;

	//пересчитать подвижность безотносительно тяги(которой может не быть если WASD не нажаты)
	UpdateMobility(false);	

	//влияние прокачки на силу прыжка
	float JumExperience = GetAttackAction()->ReverseJumpDir ?
		Phenotype.JumpBackForceFactor() : Phenotype.JumpForceFactor();

	//базис скорости прыжка пока что в генофонде общий для всех существ данного класса
	float JumpImpulse = GetAttackActionVictim().JumpVelocity	// скорость прыжка, из сборки
		* FMath::Min(AttackForceAccum, 1.0f)		// учет времени прижатия кнопки/ног
		* MoveGain									// учет моторного здоровья
		* JumExperience;							// ролевая прокачиваемая сила

	//как правильно и универсально добывать направление прыжка, пока неясно
	FVector JumpDir = GetAttackAction()->ReverseJumpDir ? -AttackDirection : AttackDirection;
	JumpDir = FVector::VectorPlaneProject(JumpDir, Mesh->Thorax.ImpactNormal);

	//прыжок - важное сильновое упражнение, регистрируется 
	CatchMyrLogicEvent(EMyrLogicEvent::SelfJump, AttackForceAccum, nullptr);

	//техническая часть прыжка - одновременное гарцевание обоими поясами
	Thorax->PhyPrance(JumpDir, JumpImpulse, JumpImpulse);
	Pelvis->PhyPrance(JumpDir, JumpImpulse, JumpImpulse);

	//переход в состояние вознесения нужен для фиксации анимации, а не для новой дин-модели, которая всё равно затрётся дальше по ходу атакой
	AdoptBehaveState(EBehaveState::fall);
	AttackForceAccum = 0;
	return false;
}


//==============================================================================================================
//отпустить всё, что имелось активно схваченным (в зубах)
//==============================================================================================================
void AMyrPhyCreature::UnGrab()
{
	//не ясно, как, перебирать все конечности и вообще как правильно сигнализировать о наличии хвата
	auto Released = Mesh->UnGrabAll();

	//послать на уровень глобального замысла сигнал, что именно этот объект был отпущен/потерян
	CatchMyrLogicEvent(EMyrLogicEvent::ObjUnGrab, 1.0f, Released);
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
		
	Dst.Phenotype = Phenotype;
	Dst.Damages.SetNum((int)ELimb::NOLIMB);
	for(int i=0; i<(int)ELimb::NOLIMB; i++)
		Dst.Damages[i] = Mesh->GetLimb((ELimb)i).Damage;
	Dst.Stamina = (uint8)(FMath::Clamp (Stamina, 0.0f, 1.0f) * 255.0f);
	Dst.Health  = (uint8)(FMath::Clamp (Health, 0.0f, 1.0f) * 255.0f);
	Dst.Energy  = (uint8)(FMath::Clamp (Energy, 0.0f, 1.0f) * 255.0f);

	//сохранение памяти ИИ
	for (auto G : MyrAI()->Memory)
	{
		//перевод указателя в имя актора
		FSocialMemory Tuple;
		Tuple.Key = G.Key->GetFName();
		Tuple.Value = G.Value;
		Dst.AIMemory.Add(Tuple);
	}

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
	Stamina = (float)Src.Stamina / 255.0f;
	Health = (float)Src.Health / 255.0f;
	Energy = (float)Src.Energy / 255.0f;
	for(int i=0; i<(int)ELimb::NOLIMB; i++)
		Mesh->GetLimb((ELimb)i).Damage = Src.Damages[i];

	SetCoatTexture(Src.Coat);

	//непосредственно поместить это существо в заданное место
	TeleportToPlace(Src.Transform, true);

	//auto MainMeshPart = GetMesh()->GetMaterialIndex(TEXT("Body"));
	//if (MainMeshPart == INDEX_NONE) MainMeshPart = 0;
	//UMaterialInterface* Material = GetMesh()->GetMaterial(MainMeshPart);
}

//==============================================================================================================
//отражение действия на более абстрактонм уровне - прокачка роли, эмоция, сюжет
//вызывается из разных классов, из ИИ, из триггеров...
//==============================================================================================================
void AMyrPhyCreature::CatchMyrLogicEvent(EMyrLogicEvent Event, float Param, UObject* Patiens, FMyrLogicEventData* ExplicitEmo)
{
	if (!GenePool) return;						//нет генофонда вообще нонсенс
	if (!GenePool->MyrLogicReactions) return;	//забыли вставить в генофонд список реакций
	

	//для начала инструкции по обслуживанию события должны быть внутри
	auto EventInfo = MyrAI()->MyrLogicGetData(Event);
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

	//подняться на уровень сюжета и посмотреть, мож тригернет продвижение истории
	GetMyrGameInst()->MyrLogicEventCheckForStory(Event, this, Param, Patiens);

}

//◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘
//==============================================================================================================
//подали на вход только идентификатор действия, а что это за действие неизвестно, так что нужно его поискать во всех списках и запустить по своему
//==============================================================================================================
EAttackAttemptResult AMyrPhyCreature::ActionFindStart(ECreatureAction Type)
{
	//особые действия, которые не сводятся к вызову ресурса, сводятся, в данном случае, к установке режима (или замене)
	auto R = EAttackAttemptResult::STARTED;
	switch (Type)
	{
		case ECreatureAction::TOGGLE_MOVE:									return R;//◘◘>
		case ECreatureAction::TOGGLE_HIGHSPEED:	bRun = 1;					return R;//◘◘>
		case ECreatureAction::TOGGLE_CROUCH:	bCrouch = 1;				return R;//◘◘>
		case ECreatureAction::TOGGLE_FLY:		bFly = 1;	bSoar = 0;		return R;//◘◘>
		case ECreatureAction::TOGGLE_SOAR:		bSoar = 1;					return R;//◘◘>
		case ECreatureAction::TOGGLE_WALK:		bRun=bCrouch=bSoar=bFly=0;	return R;//◘◘>
		case ECreatureAction::TOGGLE_CLIMB:		bClimb = 1;

			CeaseActionsOnHit();	// прервать действия чтобы начать новые
			SetWannaClimb(true);	// запустить возможно новые действия, помогающие быстрее зацепиться
			return R;//◘◘>
	}

	/////////////////////////////////////////////////////////////////////////
	//единый список
	/////////////////////////////////////////////////////////////////////////
	R = EAttackAttemptResult::INCOMPLETE_DATA;

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
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s ActionFindStart found %d"), *GetName(), SameActions.Num());
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s ActionFindStart step %d, %s, result %s"), *GetName(),
			0, *GenePool->Actions[SameActions[0]]->HumanReadableName.ToString(), *TXTENUM(EAttackAttemptResult, R));

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
					i, *GenePool->Actions[SameActions[i]]->HumanReadableName.ToString(), *TXTENUM(EAttackAttemptResult, R2));
			}
		}

		//если в конечном счёте удалось найти пригодное действие
		if (ActOk(R))
		{ 
			//это действие - атака, так как есть сборки жертв 
			//увы, юзается только 1-ый модуль жертвы, иначе надо искать,
			//что за зверь перед носом, какой тип для него лучше
			//а мы условились, что ИИ не вызывает эту функцию для пользовательских атак
			//ИИ перебирает их отдельно и вызывает у NewAttackStart себя сам
			if (GenePool->Actions[BestAction]->IsAttack())
				R = NewAttackStart(BestAction);

			//это действие - релакс, так как 3 фазы четко соответствуют входу, пребыванию, выходу
			else if(GenePool->Actions[BestAction]->IsRelax())
				RelaxActionStart(BestAction);

			//всё проечее - самодействия
			else SelfActionStart(BestAction);
		}
	}
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s ActionFindStart %s %s"), *GetName(),
		*TXTENUM(ECreatureAction, Type), *TXTENUM(EAttackAttemptResult, R));
	return R;//◘◘>
}

//==============================================================================================================
//завершить (или наоборот вдарить после подготовки) конкретное действие
//==============================================================================================================
EAttackAttemptResult AMyrPhyCreature::ActionFindRelease(ECreatureAction Type, UPrimitiveComponent* ExactGoal)
{
	//отсев действий, не являющихся атаками
	auto R = EAttackAttemptResult::STARTED;
	switch (Type)
	{
		//выключение бега
		case ECreatureAction::TOGGLE_HIGHSPEED:	bRun = false;		return R;//◘◘>
		case ECreatureAction::TOGGLE_CROUCH:	bCrouch = false;	return R;//◘◘>
		case ECreatureAction::TOGGLE_FLY:		bFly = false;		return R;//◘◘>
		case ECreatureAction::TOGGLE_SOAR:		bSoar = false;		return R;//◘◘>
		case ECreatureAction::TOGGLE_CLIMB:		bClimb = false;
			SetWannaClimb(false);
			return R;//◘◘>
	}

	//по умолчанию для неудачи непонятной
	R = EAttackAttemptResult::FAILED_TO_STRIKE;

	//если выполняется действие-атака
	if (DoesAttackAction() && GetAttackAction()->Type == Type)
	{
		//не проверяем, муторно - этот тракт не вызывается из ИИ - сразу вдаряем
		R = NewAttackStrike();
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s ActionFindRelease %s %s"), *GetName(),
			*GetAttackAction()->GetName(), *TXTENUM(EAttackAttemptResult, R));
	}
	//выполняется действие-релакс - отжатие есть старт выхода из релакса
	else if(DoesRelaxAction() && GetRelaxAction()->Type == Type)
		RelaxActionStartGetOut();

	//неведомое умолчание
	return R;
}

//◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘
//==============================================================================================================
//начать самодействие (внимание, проверка применимости здесь не производится, надо раньше проверять)
//==============================================================================================================
void AMyrPhyCreature::SelfActionStart(int SlotNum)
{
	//если в это же время происходит атака, которая не допускает поверх себя самодействия
	if(Attacking())
		if (GetAttackActionVictim().ForbidSelfActions)
		{	UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s SelfActionStart NO - forbidden by current attack"), *GetName());
			return;
		}

	//если слот самодействия доступен
	if (CurrentSelfAction == 255)
	{
		//теперь можно стартовать
		CurrentSelfAction = SlotNum;
		AdoptWholeBodyDynamicsModel(&GetSelfAction()->DynModelsPerPhase[SelfActionPhase], GetSelfAction()->UseDynModels);

		//отправить символический звук выражения, чтобы очевидцы приняли во внимание и эмоционально пережили
		MyrAI()->LetOthersNotice(EHowSensed::EXPRESSED); 
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s SelfActionStart OK %s"), *GetName(), *GetCurrentSelfActionName());
	}
}

//==============================================================================================================
//инкрементировать фазу самодействия и применить слепки моделей динамики для новой фазы
//==============================================================================================================
void AMyrPhyCreature::SelfActionNewPhase()
{
	//если слот самодействия доступен
	if (CurrentSelfAction != 255)
	{	SelfActionPhase++;
		AdoptWholeBodyDynamicsModel(&GetSelfAction()->DynModelsPerPhase[SelfActionPhase], GetSelfAction()->UseDynModels);
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

		//применить новые настройки динамики движения/поддержки для частей тела поясов
		AdoptWholeBodyDynamicsModel(&BehaveCurrentData->WholeBodyDynamicsModel, true);
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s SelfActionCease"), *GetName());
	}
}

//==============================================================================================================
//найти список применимых в данный момент самодействий и внести их в массив
//==============================================================================================================
void AMyrPhyCreature::ActionFindList(bool RelaxNotSelf, TArray<uint8>& OutActionIndices, ECreatureAction Restrict)
{
	//отыскать подходящее, изнутри вызвав оценку применимости
	for(int i=0; i<GenePool->Actions.Num(); i++)
	{
		//сопоставление сути очередного действия с тем, что нужно - самодействия или ралкс-действия
		if(GenePool->Actions[i]->IsRelax() == RelaxNotSelf)

			//если ограничение на константу темы действия
			if(Restrict==ECreatureAction::NONE || GenePool->Actions[i]->Type == Restrict)

				//полная проверка всех критериев применимости, за исключение вероятности
				if(ActOk(GenePool->Actions[i]->IsActionFitting(this, false)))

					//адресация по индексу, адресат должен иметь указатель на генофонд существа для получения списка
					OutActionIndices.Add(i);
	}
		
}


//==============================================================================================================
//стартануть релакс уже известный по индексу
//==============================================================================================================
void AMyrPhyCreature::RelaxActionStart(int Slot)
{
	//присвоить, анимация сама это далее воспримет
	CurrentRelaxAction = Slot;

	//начать с первой, нулевой фазы, фазы перехода от стояния к лежанию и т.п.
	RelaxActionPhase = 0;

	//применить новые настройки динамики движения/поддержки для частей тела поясов
	AdoptWholeBodyDynamicsModel(&GetRelaxAction()->DynModelsPerPhase[RelaxActionPhase], GetRelaxAction()->UseDynModels);
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s RelaxActionStart %s"), *GetName(), *GetRelaxAction()->GetName());

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
		AdoptWholeBodyDynamicsModel(&GetRelaxAction()->DynModelsPerPhase[RelaxActionPhase], GetRelaxAction()->UseDynModels);
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
		//вернуть настройки динамики уровня выше - уровня состояния поведения
		AdoptWholeBodyDynamicsModel(&BehaveCurrentData->WholeBodyDynamicsModel, true);

		//отметить событие
		CatchMyrLogicEvent(EMyrLogicEvent::SelfRelaxEnd, 1.0f, GetRelaxAction());
		RelaxActionPhase = 0;
		CurrentRelaxAction = 255;
	}
}

//◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘
//==============================================================================================================
// найти, что схесть - живая еда должна быть подвешена (взята) в одну из конечностей
//==============================================================================================================
UPrimitiveComponent* AMyrPhyCreature::FindWhatToEat(FDigestivity*& D, FLimb*& LimbWith)
{
	UPrimitiveComponent* Food = nullptr;
	FDigestivity* dFood = nullptr;
	FLimb* LimbWithVictim = nullptr;

	//по всем частям тела - поиск объектов, которых они так или иначе касаются
	for (int i = 0; i < (int)ELimb::NOLIMB; i++)
	{
		//сначала найти то, что держится в этом члене
		FLimb& Limb = Mesh->GetLimb((ELimb)i);
		if(!Limb.Floor) continue;

		//глобальная функция по поиску пищевой ценности в произвольном объекте
		dFood = GetDigestivity (Limb.Floor->OwnerComponent->GetOwner());
		if(dFood)
		{
			//можно есть лишь тех, которые зажаты в зубах
			//чтоб не съесть того, кто держит нас или не пересчитывать соотношение масс
			//для неподвижных предметов поедание осуществляется вместо Grab, и эта функция не вызывается
			if (!Limb.Grabs) continue;

			//исключить те объекты, которые не восполняют энергию, то есть несъедобные
			if (dFood->Time <= 0) continue;
			Food = Limb.Floor->OwnerComponent.Get();
			LimbWith = &Limb;
			D = dFood;
		}
	}
	return Food;
}

//==============================================================================================================
//абстрактно съесть конкретную вещь в мире = поглотть ее энергию, может, отравиться
//==============================================================================================================
bool AMyrPhyCreature::EatConsume(UPrimitiveComponent* Food, FDigestivity* D)
{
	//время действия пищи должно быть положительным, иначе это не пища
	if (D->Time <= 0)
	{	UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s EatConsume: Uneatable! %s"), *GetName(), *Food->GetName());
		return false;
	}

	//желудок пустой и полностью загружается новой едой
	if (Stomach.Time <= 0)
		Stomach = *D;
	else
	{
		//новая еда подмешивается к уже находящейся в желудке
		float Whole = Stomach.DeltaEnergy * Stomach.Time + D->DeltaEnergy * D->Time;
		float WholeInvTime = 1 / (Stomach.Time + D->Time);
		Stomach.Time = Whole * WholeInvTime;
		Stomach.DeltaEnergy = FMath::Clamp(FMath::Lerp(Stomach.DeltaEnergy, D->DeltaEnergy, D->Time * WholeInvTime), 0.0f, 1.0f);
		Stomach.DeltaHealth = FMath::Clamp(FMath::Lerp(Stomach.DeltaHealth, D->DeltaHealth, D->Time * WholeInvTime), 0.0f, 1.0f);
		Stomach.DeltaStamina = FMath::Clamp(FMath::Lerp(Stomach.DeltaStamina, D->DeltaStamina, D->Time * WholeInvTime), 0.0f, 1.0f);
	}

	//здесь фиксируется только субъективный факт поедания, применения силы, а не результат
	CatchMyrLogicEvent(EMyrLogicEvent::ObjEat, D->Time * D->DeltaEnergy, Food);

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
	FDigestivity* dFood = nullptr;
	FLimb* LimbWithVictim = nullptr;
	UPrimitiveComponent* Food = FindWhatToEat(dFood, LimbWithVictim);
	if (!Food) return false;

	//№№№№№№№№№№№№№№
	//абстрактно, в переменных съесть, пополнить энергию
	bool result = EatConsume(Food, dFood);
	//№№№№№№№№№№№№№№


	//поскольку хватание сопровождалось атакой, после съедания надо всякую атаку остановить
	NewAttackEnd();

	//не смогли съесть, остальное значит не нужно
	if (!result) return false;

	//найти и создать объект останков
	AActor* Remains = nullptr;
	if (dFood->WhatRemains)
		Remains = GetWorld()->SpawnActor(*dFood->WhatRemains, &Food->GetComponentTransform());


	//перед деструкцией будет отвязан ИИ
	Food->GetOwner()->Destroy();
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
void AMyrPhyCreature::WigdetOnTriggerNotify(ETriggerNotify Notify, AActor* What, USceneComponent* WhatIn, bool On)
{
	if (Daemon)
		if (auto Wid = Daemon->HUDOfThisPlayer())
			switch (Notify)
			{
			case ETriggerNotify::CanClimb:	 Wid->OnTriggerCanClimb(What, On); break;
			case ETriggerNotify::CanEat:	 Wid->OnTriggerCanEat(What, On); break;
			case ETriggerNotify::CanSleep:	 Wid->OnTriggerCanSleep(What, On); break;
			}
}

//==============================================================================================================
//передать информацию в анимацию из ИИ (чтобы не светить ИИ в классе анимации)
//==============================================================================================================
void AMyrPhyCreature::TransferIntegralEmotion(float& Rage, float& Fear, float& Power)
{
	Rage = MyrAI()->IntegralEmotionRage;
	Fear = MyrAI()->IntegralEmotionFear;

	//при атаках эмоции не выражать, атаки сами выбираются из диапазона эмоций и уже включают их
	if (NoAttack())
	{
		Power = MyrAI()->IntegralEmotionPower;
		Power *= GetBehaveCurrentData()->ExpressEmotionPower;
		if(!NoRelaxAction()) GetRelaxAction()->MetabolismMult;
		if(!NoSelfAction()) GetSelfAction()->MetabolismMult;
	}
	else Power = 0;
}

//==============================================================================================================
//зарегистрировать пересекаемый объём с функционалом
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
}

//==============================================================================================================
//исключить пересекаемый объём при выходе из него
//==============================================================================================================
bool AMyrPhyCreature::DelOverlap(UMyrTriggerComponent* Ov)
{
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

bool AMyrPhyCreature::ModifyMoveDirByOverlap(FVector& InMoveDir)
{
	auto UsedOv = Overlap0;
	FTriggerReason *TR = nullptr;
	if (!UsedOv) UsedOv = Overlap1; else
	{	TR = UsedOv->HasReaction(EWhyTrigger::VectorFieldMover);
		if(!TR) UsedOv = Overlap1;
	}
	if(!UsedOv) UsedOv = Overlap2; else
	{	TR = UsedOv->HasReaction(EWhyTrigger::VectorFieldMover);
		if (!TR) UsedOv = Overlap2;
	}
	if(!UsedOv) return false; else
	{	TR = UsedOv->HasReaction(EWhyTrigger::VectorFieldMover);
		if (!TR) return false;
	}

	FVector Force = UsedOv->ReactVectorFieldMove(this);
	float Coef = FCString::Atof(*TR->Value);
	if (Coef > 0) Force *= Coef;
	InMoveDir += Force;
	InMoveDir.Normalize();
	return true;
}



//==============================================================================================================
//отладочная линия
//==============================================================================================================
#if WITH_EDITOR
void AMyrPhyCreature::Line(ELimbDebug Ch, FVector A, FVector AB, float W, float Time)
{
	if (IsDebugged(Ch))
		DrawDebugLine(GetOwner()->GetWorld(), A, A + AB,
			GetMyrGameInst()->DebugLineChannel(Ch), false, Time, 100, W);
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
	int li = MyrAI()->RandVar&15; if(li >= (int)ELimb::NOLIMB) li = 15-li;
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
FVector AMyrPhyCreature::GetLookingVector() const
{
	return Mesh->GetLimbAxisForth(Mesh->Head);
}
//==============================================================================================================
//вектор вверх тела
//==============================================================================================================
FVector AMyrPhyCreature::GetUpVector() const
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
	if(Thorax->StandHardness > 0.4)
	{	auto pg = GetMyrGameInst()->Surfaces.Find(Mesh->Thorax.Surface);
		if (pg) ThoraxStealthFactor = pg->StealthFactor;
	}
	if(Pelvis->StandHardness > 0.4)
	{	auto pg = GetMyrGameInst()->Surfaces.Find(Mesh->Pelvis.Surface);
		if (pg) PelvisStealthFactor = pg->StealthFactor;
	}

	//берем минимальный из двух частей тела (почему минимальный?)
	return FMath::Max(PelvisStealthFactor, ThoraxStealthFactor);
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
//пищевая ценность этого существа как еды для другого существа
//==============================================================================================================
float AMyrPhyCreature::GetNutrition() const
{	return GenePool->Digestivity.DeltaEnergy * GenePool->Digestivity.Time;
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


//==============================================================================================================
//межклассовое взаимоотношение - для ИИ, чтобы сформировать отношение при первой встрече с представителем другого класса
//==============================================================================================================
FColor AMyrPhyCreature::ClassRelationToClass(AActor* obj)
{
	//сам себя - начальное отношение полная любовь
	if (obj == this) return FColor::Green;

	//найти сборку типовых отношений в генофонде, если не нашли - к неизвестному отношение "никак"
	FAttitudeTo* res = GenePool->AttitudeToOthers.Find(obj->GetClass());
	if (!res) return FColor::Black;

	//если нашли, зависит от того, живое или мёртвое
	else if (auto MyrAim = Cast<AMyrPhyCreature>(obj))
	{	if (MyrAim->Health <= 0)
			return res->EmotionToDeadColor;
	}
	return res->EmotionToLivingColor;
}

FColor AMyrPhyCreature::ClassRelationToDeadClass(AMyrPhyCreature* obj)
{
	//найти сборку типовых отношений в генофонде, если не нашли - к неизвестному отношение "никак"
	FAttitudeTo* res = GenePool->AttitudeToOthers.Find(obj->GetClass());
	if (!res) return FColor::Black;
	return res->EmotionToDeadColor;
}