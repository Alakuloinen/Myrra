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

//свой лог
DEFINE_LOG_CATEGORY(LogMyrPhyCreature);

//одной строчкой вся логика ответов на запросы о переходе в новое состояние
#define TRANSIT(state, cond) case EBehaveState::##state: if(cond) AdoptBehaveState(EBehaveState::##state); break;
#define TRANSIT2(state, stateE, cond) case EBehaveState::##state: if(cond) if(!AdoptBehaveState(EBehaveState::##state)) AdoptBehaveState(EBehaveState::##stateE);

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



	//механический лошарик
	Mesh = CreateOptionalDefaultSubobject<UMyrPhyCreatureMesh>(TEXT("Mesh"));
	RootComponent = Mesh;

	//отбойник не должен быть первым, потому что в редакторе его не развернуть горизонтально
	//KinematicSweeper = CreateDefaultSubobject<UCapsuleComponent>(TEXT("KinematicSweeper"));
	//KinematicSweeper->SetCapsuleSize(10, 15, true);
	//KinematicSweeper->SetupAttachment(RootComponent);

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
		for (int i = 0; i < (int)ERoleAxis::NUM; i++)
		{	auto R = GenePool->RoleRanges.Find((ERoleAxis)i);
			if(R) Params.P[i] = FMath::Lerp(R->GetLowerBoundValue(), R->GetUpperBoundValue(), FMath::RandRange(0.0f, 0.1f));
			UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s First Assign Params %d %g"), *GetName(), i, Params.P[i]);
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
	LocalStatsWidget->SetPivot(FVector2D(0, 1));				
	LocalStatsWidget->SetRelativeLocation(FVector::ZeroVector);	

	//дать виджету знать, в контексте чего он будет показываться 
	//ахуеть! если это вызвать до Super::BeginPlay(), то виджет ещё не будет создан, и ничего не сработает
	if (StatsWidget())
	{	StatsWidget()->MyrOwner = this;					
		StatsWidget()->MyrAI = MyrAI();			
	}

	//рассчитать анимации длины/амплитуды ног - пока здесь, потому что чем позже тем лучше
	//должны быть полностью спозиционированы все кости
	Mesh->InitFeetLengths(Mesh->gThorax);
	Mesh->InitFeetLengths(Mesh->gPelvis);

	//свойства гашения звука голоса
	Voice->AttenuationSettings = GetMyrGameMode()->SoundAttenuation;

	//в основном для высираемых мобов
	CatchMyrLogicEvent(EMyrLogicEvent::SelfSpawnedAtLevel, Daemon ? 1 : 0, nullptr);
}

//====================================================================================================
// каждый кадр
//====================================================================================================
void AMyrPhyCreature::Tick(float DeltaTime)
{
	FrameCounter++;
	Super::Tick(DeltaTime);
	if (HasAnyFlags(RF_ClassDefaultObject)) return;
	if (IsTemplate()) return;

	//набор мощи для атаки/прыжка
	if (AttackForceAccum > 0.0f && AttackForceAccum < 1.0f)
		AttackForceAccum += DeltaTime;

	//применить внешние команды управления от игрока, если таковой есть
	if (Daemon)
	{
		//осуществить покадровые обязанности по управлению существом
		Daemon->ProcessMotionOutput(BehaveCurrentData->bOrientIn3D);
	}

	//демон сохраняет свои планы в отдельные переменные, здесь они по идее должны смешиваться с таковыми из ИИ
	//но пока просто тупо присваивается
	ConsumeInputFromControllers(DeltaTime);

	//обработка уткнутости в препятствие и простейшая корректировка курса
	WhatIfWeBumpedIn();

	//боль - усиленная разность старого, высокого здоровья и нового, сниженного мгновенным уроном. 
	Pain *= 0.99f;

	//обработка движений по режимам поведения 
	ProcessBehaveState(Mesh->GetPelvisBody(), DeltaTime);
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

	//плавно стремим, если возрастает, а если убывает, то резко обрываем - нужно остановиться вовремя
	if(D->Gain != ExternalGain)
	{	float Raznice = D->Gain - ExternalGain;
		if (Raznice <= 0.05)
			ExternalGain = D->Gain;
		else ExternalGain += 0.05f;
	}

	//вектор движения изменяем только если есть скаляр тяги, иначе он остается тот же или нулевой
	if(D->Gain > 0.0f)
	{	MoveDirection = D->MoveDir;
		
		//расчет сборного коэффициента замедления, который уменьшает ориентировочную скорость
		UpdateMobility();

	}
	//в моторной сборка указывается "желаемое" состояние, оно почти безусловно переносится в "запрашиваемое"
	if (D->NewState != UpcomingState) QueryBehaveState(D->NewState);
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
			if (Daemon->IsFirstPerson())
			{
				MoveDirection = AttackDirection;
				FVector RealForth = Mesh->GetLimbAxisForth(Mesh->Pectus);
				if((MoveDirection|RealForth)<-0.1)
				{	ExternalGain = 1.0f;
					if(CurrentState == EBehaveState::stand)
						QueryBehaveState(EBehaveState::walk);
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

		//эксперимент с телепортажем
		/*if(Daemon->bSupport && !Mesh->WaitingForTeleport)
		{
			//досрочно, до следующего зажатия кнопки
			Mesh->WaitingForTeleport = true;
			Daemon->bSupport = false;	
		}*/
		
	}
	//непись, только ИИ
	else 
	{
		
		//сохранить старое значение направления движенния
		FVector OldMoveDirection = MoveDirection;

		//стандартно просчитать новые устремления все
		ConsumeDrive(&MyrAI()->Drive);

		//дурацкий костыль по сглаживанию рваного курса 
		TurnMoveDirection(OldMoveDirection, 0.5, -0.8, 0.1);
	}
}

//==============================================================================================================
//плавно крутить направление движения
//==============================================================================================================
bool AMyrPhyCreature::TurnMoveDirection(FVector OldMoveDir, float StartThresh, float SidewayThresh, float SmoothGradus)
{
	//степень сонаправленнности старого направления и нового
	float Colinea = MoveDirection | OldMoveDir;
	if (Colinea < StartThresh)
	{
		//серьёзно назад, почти напрямую взад, нельзя просто развернуть вектор, нужно через сторону
		if (Colinea < SidewayThresh)
		{
			//плавно начать поворот в сторону, наиболее колинеарную самому телу
			FVector SideWay = Mesh->GetLimbAxisRight(Mesh->Lumbus);

			//если эта сторона передней части тела больше развёрнута в сторону нового курса
			//то использовать ее, иначе прибавить противоположный вектор
			if ((GetAxisLateral() | MoveDirection) > 0.0f)
				MoveDirection = OldMoveDir + SideWay * SmoothGradus;
			else MoveDirection = OldMoveDir - SideWay * SmoothGradus;
		}
		//просто сглажено
		else MoveDirection = OldMoveDir + SmoothGradus * MoveDirection;

		//нормализация, ибо аддитивно
		MoveDirection.Normalize();
		return true;
	}
	return false;
}

//==============================================================================================================
//вычислить степень уткнутости в препятствие и тут же произвести корректировку
//==============================================================================================================
void AMyrPhyCreature::WhatIfWeBumpedIn()
{
	//расчёт сумарной уткнутости - если положительное, то сразу
	//если перед нами непролазное препятствие (-) то постепенно опомниваться от такой колизии
	ELimb Bumper = ELimb::NOLIMB;
	Stuck = Mesh->StuckToStepAmount(MoveDirection, Bumper);
	DrawDebugString(GetWorld(), Mesh->Bodies[1]->GetCOMPosition(), FString::SanitizeFloat(Stuck, 1),
		nullptr, FColor(255, -255 * Stuck, 255 * Stuck, 255), 0.02, false, 1.0f);

	//если хоть какое-то препятствие найдено
	if (Bumper != ELimb::NOLIMB)
	{
		//уткнулись головой - специфично для четвероногих
		if (Bumper == ELimb::HEAD)
			if (NoSelfAction())

				//только если серьёзно уткнулись, а на по касательной
				if (Stuck > 0.5)
				{
					//встать на дыбы и больше ничего не делать
					SelfActionFindStart(ECreatureAction::SELF_PRANCE_QUICK_AT_BUMP);
					return;
				}

		//для игрока
		if (IsUnderDaemon())
		{
			//если антураж порождает уткнутость, которую не вылечить залазаньем на стену - прекратить всякое движение
			//осторожно, можно застрять, но поидее при смене вектора круса - Stuck тоже сменится
			if (Stuck < -0.3) MoveDirection = -MoveDirection;
			else if (Stuck < -0.5) ExternalGain = 0.0f;

		}
		//для ИИ
		else
		{
			//для непися даже слабое утыкание приводит к кратковременной смене траектории на противоположную
			if (Stuck < -0.3)
			{
				//инвертировать курс также в намерениях ИИ (на весь промежуток между тактами ИИ)
				MyrAI()->Drive.MoveDir = -MyrAI()->Drive.MoveDir;
				MoveDirection = -MoveDirection;
			}
		}
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
				* Params.GetAgility()							// общая быстрота индивида 
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
		Mesh->AdoptDynModelForGirdle(Mesh->gThorax, DynModel->Thorax);
		Mesh->AdoptDynModelForGirdle(Mesh->gPelvis, DynModel->Pelvis);

		//в дин-моделях есть замедлитель тяги, чтобы его прменить, нужно вызвать вот это
		UpdateMobility();
	}

	//голос/общий звук прикреплен к дин-модели, хотя не физика/хз, нужно ли здесь и как управлять силой
	MakeVoice(DynModel->Sound, 1, false);

}

//==============================================================================================================
//перейти в новую фазу атаки
//==============================================================================================================
void AMyrPhyCreature::AdoptAttackPhase(EActionPhase NewPhase)
{
	//присвоить
	CurrentAttackPhase = NewPhase;

	//не уровне меша принять приготовления атакующих / опасных частей тела
	auto Attack = GetCurrentAttackInfo();
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
	if(NowPhaseToHold())	AttackForceAccum = 0.1;

	//текущая фаза - прыгнуть
	else if(NowPhaseToJump())	JumpAsAttack();

	//текущая новая фаза - отцепить то, что зацеплено было
	else if(GetCurrentAttackInfo()->UngrabPhase == NewPhase) UnGrab();

}

//==============================================================================================================
//включить или выключить желание при подходящей поверхности зацепиться за нее
//==============================================================================================================
void AMyrPhyCreature::SetWannaClimb(bool Set)
{
	//подробная реализация, с подпрыгом, если нужно
	Mesh->ClingGetReady(Set);
}


//==============================================================================================================
//разовый отъём запаса сил - вызывается при силоёмких действиях / или восполнение запаса сил - в редком тике
//==============================================================================================================
void AMyrPhyCreature::StaminaChange(float delta)
{
	//внимание! пищевая энергия расходуется только при восстановлении запаса сил и при попытке отнять силы, когда их уже нет
	Stamina += delta;								// прирастить запас сил на дельту (или отнять), вычисленную извне

	if (Stamina < 0)								// сил и так нет, а от нас ещё требуют
	{	Stamina = 0;								// восстановить ноль - сил как не было, так и нет
		Energy -= delta * 0.02f;					// а вот расход пищевой энергии всё равно происходит, да ещё сильнее
	}
	else											// запас сил имеется (или нашёлся)
	{	if (Stamina > 1.0f) Stamina = 1.0f;			// запас сил и так восстановлен, дальше некуда
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

	//учёт воздействия поверхности, к которой мы прикасаемся любой яастью тела, на здоровье
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

	//восстановление запаса сил (используется та же функция, что и при трате)
	StaminaChange (Mesh->DynModel->StaminaAdd * DeltaTime	);										

	//восстановление здоровья 
	HealthChange ((Mesh->DynModel->HealthAdd + SurfaceHealthModifier ) * DeltaTime	);										

	//пересчитать здоровье и мобильность тела
	UpdateMobility();

	//возраст здесь, а не в основном тике, ибо по накоплению большого числа мелкие приращения могут быть меньше разрядной сетки
	Age += DeltaTime;


	//////////////////////////////////////////////////////////////////////////////////////
	//перебор самодействий и вероятностый запуск (перенесено из ИИ, ибо вне ИИ) 
	//////////////////////////////////////////////////////////////////////////////////////
	if(GenePool->NumSelfActions())
	{
		//если мы уже не совершаем никаких действий
		if (CurrentSelfAction == 255)
		{
			//насколько применимая сейчас акция выдалась, если удачно - устанавливаем индекс
			auto NewSAInfo = GenePool -> SelfActionInfo (SelfActionToTryNow);
			if(NewSAInfo -> IsActionFitting (this) == EAttackAttemptResult::OKAY_FOR_NOW)
				SelfActionStart ( SelfActionToTryNow );
		}
		//если уже выполняем самодействие, но хотим выполнить другое
		//какого хрена хотим? просто так затмевать намеренное действие? вообще здесь ничего не должно быть
		/*else
		{
			//инфа по текущему самодействию
			auto CurSAInfo = GenePool -> SelfActionInfo (CurrentSelfAction);

			//если дейсвтие надо останавливать при атаке, и сейчас атака - снимаем действие
			if (CurSAInfo->Condition.SkipDuringAttack && !NoAttack())
				SelfActionCease();

			//если действие призвано прерываться при говорении, и мы говорим, прервать
			if (CurSAInfo->Condition.SkipDuringSpeaking && CurrentSpelledSound!=(EPhoneme)0)
				SelfActionCease();
		}*/

		//цикл перебора вне зависимости от удачи в выборе каждого конкретного действия
		uint8& Queue = (uint8&)SelfActionToTryNow;	Queue++;
		if (Queue >= GenePool->NumSelfActions()) Queue = 0;
	}
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
	if(!NoRelaxAction())
		if(GetCurrentRelaxActionInfo()->CeaseAtHit)
			RelaxActionStartGetOut();
	if(!NoSelfAction())
		if(GetCurrentSelfActionInfo()->CeaseAtHit)
			SelfActionCease();
}



//==============================================================================================================
//сделать шаг - 4 эффекта: реальный звук, ИИ-сигнал, облачко пыли, декал-след
//==============================================================================================================
void AMyrPhyCreature::MakeStep(ELimb eLimb, bool Raise)
{
	//если колесо ноги не стоит на полу, то и шага нет (это неточно, нужно трассировать)
	FLimb* Limb = &Mesh->GetLimb(eLimb);

	//если для конечности не заведен физ-членик, вероятно существо безногое, и надо брать данные из родителя-туловища
	if (!Mesh->HasFlesh(eLimb)) Limb = &Mesh->GetLimbParent(eLimb);

	//а вот если физически нога или туловище не касается поверхности, шаг нельзязасчитывать
	if(!Limb->Stepped) return;

	//тип поверхности - это не точно, так как не нога а колесо - пока неясно, делать трассировку в пол или нет
	EMyrSurface ExplicitSurface = Limb->Surface;
	auto Trans = Mesh->GetSocketTransform(Mesh->FleshGene(eLimb)->GrabSocket, RTS_World);

	//точное место шага - пока не ясно, стоит ли делать трассировку
	FVector ExactStepLoc = Trans.GetLocation();

	//сила шага, ее надо сложно рассчитывать 
	float StepForce = 1.0f;

	//поворот для декала - тут все просто
	FQuat ExactStepRot = Trans.GetRotation();

	//поворот для источника частиц - вот тут нужен вектор скорости в направлении нормали
	FQuat ExactBurstRot = ExactStepRot;

	//расчёт скорости движущегося кончика ноги в точке шага
	float VelScalar; FVector VelDir;

	//если к кончику лапы приделан членик, то берем его, иначе (для мышей) через общую скорость
	auto FootBody = Mesh->HasFlesh(eLimb) ? Mesh->GetFleshBody(*Limb, 0) : Mesh->GetGirdleBody(Mesh->GetGirdle(eLimb), EGirdleRay::Center);
	FVector Vel = FootBody->GetUnrealWorldVelocity();
	if (Limb->Floor) Vel -= Limb->Floor->GetUnrealWorldVelocity();
	if ((Vel | Limb->ImpactNormal) < 0) Vel = -Vel;
	Vel.ToDirectionAndLength(VelDir, VelScalar);

	//в качестве нормы берем скорость ходьбы этого существа, она должна быть у всех, хотя, если честно, тут уместнее твердая константа
	StepForce = VelScalar / GenePool->BehaveStates[EBehaveState::walk]->MaxVelocity;

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
			0.5f  + 0.05 * Mesh->GetMass()  + 0.001* Mesh->GetGirdleDirectVelocity (Mesh->GetGirdle(eLimb)),
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
//запросить извне (управляющим объектом) новое состояние
//==============================================================================================================
void AMyrPhyCreature::QueryBehaveState(EBehaveState NewState)
{
	UE_LOG(LogMyrPhyCreature, Log, TEXT("%s QueryBehaveState - (%s >> %s)"), *GetName(),
		*TXTENUM(EBehaveState, UpcomingState),
		*TXTENUM(EBehaveState, NewState));

	//пока неясно, для чего понадобится такой свитч и как их лучше отсеивать
	switch (UpcomingState)
	{
	case EBehaveState::walk:
	case EBehaveState::run:
	default:
		UpcomingState = NewState;
	}
}

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
	if (FMath::Abs(Mesh->GetGirdleDirectVelocity(Mesh->gThorax)) < (*BehaveNewData)->MinVelocityToAdopt)
	if (FMath::Abs(Mesh->GetGirdleDirectVelocity(Mesh->gPelvis)) < (*BehaveNewData)->MinVelocityToAdopt)
	{	UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s AdoptBehaveState %s cancelled by min velocity"), *GetName(), *TXTENUM(EBehaveState, NewState));
		return false;
	}

	//сбросить счетчик секунд на состояние
	StateTime = 0.0f;
	UE_LOG(LogMyrPhyCreature, Log, TEXT("%s AdoptBehaveState  + (%s >> %s)"), *GetName(),
		*TXTENUM(EBehaveState, CurrentState),
		*TXTENUM(EBehaveState, NewState)	);

	//если включено расслабить всё тело
	if((*BehaveNewData)->MakeAllBodyLax!= BehaveCurrentData->MakeAllBodyLax)
	Mesh->SetFullBodyLax((*BehaveNewData)->MakeAllBodyLax);

	//включение и выключение кинематической формы перемещения
	if ((*BehaveNewData)->Kinematic != !Mesh->GetRootBody()->bSimulatePhysics)
		Mesh->GetRootBody()->SetInstanceSimulatePhysics(!(*BehaveNewData)->Kinematic);


	//пока для теста, вообще неясно - на этот раз увязчаем корень тела, чтоб не болталось при лазаньи
	if (NewState == EBehaveState::climb) Mesh->GetRootBody()->LinearDamping = Mesh->GetRootBody()->AngularDamping = 100;
	else Mesh->GetRootBody()->LinearDamping = Mesh->GetRootBody()->AngularDamping = 0;
	Mesh->GetRootBody()->UpdateDampingProperties();


	//счастливый финал
	CurrentState = NewState;
	BehaveCurrentData = *BehaveNewData;

	//обновить скорость заживления тела
	Mesh->HealRate = BehaveCurrentData->HealRate;


	//если самодействие может только в отдельных состояниях и новое состояние в таковые не входит - тут же запустить выход
	if (!NoRelaxAction())
	{	if (GetCurrentRelaxActionInfo()->Condition.States.Contains(NewState) == GetCurrentRelaxActionInfo()->Condition.StatesForbidden)
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
void AMyrPhyCreature::ProcessBehaveState(FBodyInstance* BodyInst, float DeltaTime)
{
	//увеличиваем таймер текущего состояния
	StateTime += DeltaTime;

	//разбор разных состояний
	switch (CurrentState)
	{
		//шаг по поверхности
		case EBehaveState::stand:
			if (BewareDying()) break;
			if (BewareFall()) break;
			switch (UpcomingState) {
				TRANSIT (walk,		true)
				TRANSIT (crouch,	true)
				TRANSIT2 (run,walk,	true)}
			break;

		//шаг по поверхности (боимся упасть, опрокинуться и сбавить скорость до стояния)
		case EBehaveState::walk:
		case EBehaveState::sidewalk:
			if(BewareDying()) break;
			if(BewareFall()) break;
			if(BewareLowVel(EBehaveState::stand)) break;
			if(BewareOverturn(0.5f - 0.2* MoveGain)) break;
			if(BewareStuck(0.5f)) break;
			switch(UpcomingState) {
				TRANSIT (crouch,	true )
				TRANSIT (walk,		true )
				TRANSIT (sidewalk,	true )
				TRANSIT2(run,walk,	Stamina>0.1f)
				TRANSIT (climb,		Mesh->gThorax.Clung || Mesh->gPelvis.Clung)
			}
			break;

		//шаг украдкой по поверхности
		case EBehaveState::crouch:
			if (BewareDying()) break;
//			if (BewareFall()) break;
//			switch (UpcomingState) {
//				TRANSIT(walk, true)
//				TRANSIT(run, Stamina > 0.1f)
//			}
			break;

		//бег по поверхности
		case EBehaveState::run:
			if (BewareDying()) break;
			if (BewareFall()) break;
			if (BewareLowVel(EBehaveState::walk, 10.0f)) break;
			if (BewareOverturn (0.5f - 0.2 * MoveGain)) break;
			switch (UpcomingState) {
				TRANSIT(walk, true)
			}
			break;

		//забраться на уступ
		case EBehaveState::mount:
			if (BewareDying()) break;
			if (BewareFall()) break;
			if (BewareLowVel(EBehaveState::stand)) break;
			switch (UpcomingState) {
				TRANSIT(walk, Stuck < 0.5)
				TRANSIT(run, Stamina > 0.1f)
			}
			break;

		//лазанье по проивольно крутым поверхностям
		case EBehaveState::climb:
			if (BewareDying()) break;
			if (BewareFall()) break;
			if (BewareHangTop()) break;
			switch (UpcomingState) {
				TRANSIT(fall, !Mesh->gThorax.Clung && !Mesh->gPelvis.Clung)
			}
			break;

		//набирание высоты (никаких волевых переходов, только ждать земли и перегиба скорости)
		case EBehaveState::soar:
			if(BewareDying()) break;
			if(BewareSoarFall()) break;
			BewareGround();				
			break;
			
		//падение (никаких волевых переходов, только ждать земли)
		case EBehaveState::fall:
			if(BewareDying()) break;
			BewareGround();
			break;

		//опрокидывание (пассивное лежание (выход через действия а не состояния) и активный кувырок помимо воли)
		case EBehaveState::lie:
		case EBehaveState::tumble:
			if(BewareDying()) break;
			if(BewareFall()) break;
			BewareOverturn(0.6,false);	// изменение tumble <-> lie <-> walk
			break;
	
		//здоровье достигло нуля, просто по рэгдоллиться пару секнуд перед окончательной смертью
		case EBehaveState::dying:
			if(StateTime>2) AdoptBehaveState(EBehaveState::dead);
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
		if (Mesh->gThorax.StandHardness > 0.8 || Mesh->gPelvis.StandHardness > 0.8)	
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
		SelfActionFindStart(ECreatureAction::SELF_DYING1, false);
		MyrAI()->LetOthersNotice(EHowSensed::DIED);
		Health = 0.0f;
		return true;
	}
	return false;
}

//==============================================================================================================
//общая процедура для всех видов прыжка, корректно закрывающая предыдущее состояние
//==============================================================================================================
bool AMyrPhyCreature::JumpAsAttack()
{
	//исключить совсем уж недобор мощи, чтобы были всё же прыжки, а не дёрганья
	if (AttackForceAccum == 0.0f) AttackForceAccum = 1.0f;
	if (AttackForceAccum < 0.5f) AttackForceAccum = 0.5f;

	UpdateMobility(false);					//пересчитать подвижность безотносительно тяги(которой может не быть если WASD не нажаты)
	UpcomingState = EBehaveState::NO;		//сбросить ранее поставленное soar, чтобы не мешалось

	//берем данные о состоянии в прыжке до прыжка, чтобы иметь также данные о текущем состоянии
	auto SoarData = GetGenePool()->BehaveStates.Find(EBehaveState::soar);
	if (!SoarData) return false;
	float JumpImpulse = (*SoarData)->MaxVelocity	// скорость прыжка, из сборки
		* FMath::Min(AttackForceAccum, 1.0f)		// учет времени прижатия кнопки/ног
		* MoveGain									// учет моторного здоровья
		* Params.GetStrength();						// ролевая прокачиваемая сила

	//как правильно и универсально добывать направление прыжка, пока неясно
	FVector JumpDir = GetCurrentAttackInfo()->ReverseJumpDir ? -AttackDirection : AttackDirection;
	JumpDir = FVector::VectorPlaneProject(JumpDir, Mesh->Thorax.ImpactNormal);

	//прыжок - важное сильновое упражнение, регистрируется 
	CatchMyrLogicEvent(EMyrLogicEvent::SelfJump, AttackForceAccum, nullptr);

	//техническая часть прыжка
	Mesh->PhyJump(JumpDir, JumpImpulse, JumpImpulse);

	//переход в состояние вознесения нужен для фиксации анимации, а не для новой дин-модели, которая всё равно затрётся дальше по ходу атакой
	AdoptBehaveState(EBehaveState::soar);
	AttackForceAccum = 0;
	return false;
}


//==============================================================================================================
//отпустить всё, что имелось активно схваченным (в зубах)
//==============================================================================================================
void AMyrPhyCreature::UnGrab()
{
	//не ясно, как, перебирать все конечности и вообще как правильно сигнализировать о наличии хвата
	Mesh->UnGrabAll();
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
		
	//дабл не понят анрилом, так что вручную переливание в массив байтов
	FMemoryWriter ArWriter(Dst.Params);
	ArWriter << Params.UpLevelPending;
	for (int i = 0; i < (int)ERoleAxis::NUM; i++)
		ArWriter << Params.P[i];


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
	//это важно сделать до перемещения, чтоб не испытать бесконечную боль
	//дабл не понят анрилом, так что вручную переливание в массив байтов
	FMemoryReader ArReader(Src.Params);
	ArReader.Serialize(&Params.UpLevelPending, 1);
	ArReader.Serialize(Params.P, (int)ERoleAxis::NUM * sizeof(double));

	//скаляры
	Stamina = (float)Src.Stamina / 255.0f;
	Health = (float)Src.Health / 255.0f;
	Energy = (float)Src.Energy / 255.0f;
	for(int i=0; i<(int)ELimb::NOLIMB; i++)
		Mesh->GetLimb((ELimb)i).Damage = Src.Damages[i];

	SetCoatTexture(Src.Coat);

	//непосредственно поместить это существо в заданное место
	FHitResult Hit;
	Mesh->SetSimulatePhysics(false);
	Mesh->SetWorldTransform(Src.Transform, false, &Hit, ETeleportType::TeleportPhysics);
	Mesh->SetSimulatePhysics(true);

	if (Daemon)
		Daemon->ClingToCreature(this);

	//auto MainMeshPart = GetMesh()->GetMaterialIndex(TEXT("Body"));
	//if (MainMeshPart == INDEX_NONE) MainMeshPart = 0;
	//UMaterialInterface* Material = GetMesh()->GetMaterial(MainMeshPart);
}

//==============================================================================================================
//отражение действия на более абстрактонм уровне - прокачка роли, эмоция, сюжет
//вызывается из разных классов, из ИИ, из триггеров...
//==============================================================================================================
void AMyrPhyCreature::CatchMyrLogicEvent(EMyrLogicEvent Event, float Param, UPrimitiveComponent* Goal)
{
	if (GenePool) return;						//нет генофонда вообще нонсенс
	if (!GenePool->MyrLogicReactions) return;	//забыли вставить в генофонд список реакций
	

	//для начала инструкции по обслуживанию события должны быть внутри
	auto EventInfo = MyrAI()->MyrLogicGetData(Event);
	if (EventInfo)
	{
		//ну уровне ИИ изменяем эмоции (внутри он найдёт цель и положит событие в память эмоциональных событий)
		if (MyrAI()) MyrAI()->RecordMyrLogicEvent(Event, Param, Goal, EventInfo);

		//прокачка навыков - для всех указанных в сборке навыков, на которые влияет это событие
		for (auto& i : EventInfo->ExperienceIncrements)
		{
			//восстанавливаем рамки навыка, чтобы вычислить квант
			auto Range = GenePool->RoleRanges.Find(i.Key);
			if (Range)
			{
				auto result = Params.AddExperience(i.Key, *Range, i.Value);
				UE_LOG(LogMyrPhyCreature, Warning, TEXT("ACTOR %s AddExperience %s %g => %g"), *GetName(), *TXTENUM(ERoleAxis, i.Key), i.Value, Params.CurProgress(i.Key, *Range));

				//вывести сообщение аплевела (хз нужно или нет, если честно, пока для теста
				//выводить нужно, если набирается целый уровень
				if (Daemon)
					if (auto Wid = Daemon->HUDOfThisPlayer())
						Wid->OnExpGain(i.Key, result);

			}
		}
	}

	//подняться на уровень сюжета и посмотреть, мож тригернет продвижение истории
	GetMyrGameInst()->MyrLogicEventCheckForStory(Event, this, Param, Goal);

	UE_LOG(LogMyrPhyCreature, Warning, TEXT("ACTOR %s CatchMyrLogicEvent %s %g"), *GetName(), *TXTENUM(EMyrLogicEvent, Event), Param);
}

//◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘
//==============================================================================================================
//подали на вход только идентификатор действия, а что это за действие неизвестно, так что нужно его поискать во всех списках и запустить по своему
//==============================================================================================================
bool AMyrPhyCreature::ActionFindStart(ECreatureAction Type)
{
	//особые действия, которые не сводятся к вызову ресурса
	switch (Type)
	{
		//включить зацепа для лазанья
		case ECreatureAction::CLING_START:
			CeaseActionsOnHit();
			SetWannaClimb(true);
			return true;

	}

	//если по ID найдена только одна атака, то она безусловно запускается, если больше оной
	//запускается полная проверка применимости каждой из них
	int fitted = -1;
	for (int i = 0; i < GenePool->AttackActions.Num(); i++)
	{	
		if (GenePool->AttackActions[i]->Type == Type)
		{	if (fitted >= 0)
			{	if (GenePool->AttackActions[i]->IsItBetterThanTheOther(this, GenePool->AttackActions[fitted]))
					fitted = i;
			}
			else fitted = i;
		}
	}
	if (fitted >= 0)
	{	auto res = AttackStart(fitted);
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s ActionFindStart %s = %s"), *GetName(), *TXTENUM(EAttackAttemptResult,res), *GenePool->AttackActions[fitted]->GetName());
		if(res == EAttackAttemptResult::STARTED) return true;
	}

	////////////////////////////////////////////////////////
	for (int i = 0; i < GenePool->SelfActions.Num(); i++)
		if (GenePool->SelfActions[i]->Type == Type)
			if(GenePool->SelfActions[i]->IsActionFitting(this) == EAttackAttemptResult::OKAY_FOR_NOW)
			{	SelfActionStart(i);	return true;	}

	for (int i = 0; i < GenePool->RelaxActions.Num(); i++)
		if (GenePool->RelaxActions[i]->Type == Type)
			if (GenePool->RelaxActions[i]->IsActionFitting(this) == EAttackAttemptResult::OKAY_FOR_NOW)
			{	RelaxActionStart(i); return true;	}

	//ничего не подобралось
	return false;
}

//==============================================================================================================
// атаки - начать одну определенную, ExactGoal - это если вызывается из ИИ и уже ясна цель, если же игроком
// цель на этом этапе априори не ясна и может отсутствовать, поэтому ее надо уточнять
//==============================================================================================================
EAttackAttemptResult AMyrPhyCreature::AttackStart (int SlotNum, UPrimitiveComponent* ExactGoal)
{
	//в генофонде ни одна атака не заполнена на кнопку, из которой вызвалась эта функция
	if (SlotNum == 255) return EAttackAttemptResult::WRONG_PHASE;//◘◘>;

	//даже если атака не начнётся (а она в этом случае обязательно не начнётся)
	//но в ней прописано, что в начальной фазе надо съесть то, что мы держим
	//сделать это - внутри функции Eat - весь фарш
	if(GenePool->AttackActions[SlotNum]->EatHeldPhase == EActionPhase::ASCEND)
	 	if(Eat()) return EAttackAttemptResult::ATE_INSTEAD_OF_RUNNING;//◘◘> 
	

	//уже выполняется какая-то атака, нельзя просто так взять и прервать её
	if (CurrentAttack != 255) return EAttackAttemptResult::ALREADY_RUNNING;//◘◘>;

	//полная связка настроек
	auto Attack = GenePool->AttackActions[SlotNum];

	//если точная цель не введена, очевидно, это от игрока, и надо дополнительно проверить возможность 
	//ежели цель введена, её нашёл ИИ, значит ИИ уже посчитал целесообразность, и лишняя проверка не нужна
	if(!ExactGoal)
	{
		//проверка всех препонов, которые могут возникнуть на пути выполнения действия
		auto CheckRes = Attack -> ShouldIStartThisAttack(this, nullptr, nullptr, false);
		if(CheckRes != EAttackAttemptResult::OKAY_FOR_NOW) return CheckRes;//◘#>
	}

	//отправить просто звук атаки, чтобы можно было разбегаться (кстати, ввести эту возможность)
	MyrAI()->LetOthersNotice(EHowSensed::ATTACKSTART);

	//врубить атаку текущей
	CurrentAttack = SlotNum;
	AdoptAttackPhase(EActionPhase::ASCEND);

	if (Daemon)
	{
		//применить ИИ для более точного нацеливания
		if(MyrAI()->AimBetter(AttackDirection, 0.5)) Daemon->UseActDirFromAI = true;

		//если эта фаза такаи подразумевает эффектное размытие резкого движения, то врубить его
		Daemon->SetMotionBlur ( Attack -> MotionBlurBeginToStrike );
	}

	//настроить физику членов в соответствии с фазой атаки (UpdateMobility внутри)
	AdoptWholeBodyDynamicsModel(&Attack->DynModelsPerPhase[(int)CurrentAttackPhase], true);

	//обнаружить, что мы пересекаем умный объём и передать ему, что мы совершаем, возможно, судьбоносное действие
	if (Attack->SendApprovalToTriggerOverlapperOnStart)
		SendApprovalToOverlapper();

	//###########################################################################################
	//здесь фиксируется только субъективный факт атаки, применения силы, а не результат
	if(Attack->TactileDamage > 0)	CatchMyrLogicEvent(EMyrLogicEvent::SelfHarmBegin, Attack->TactileDamage, nullptr);
	else							CatchMyrLogicEvent(EMyrLogicEvent::SelfGraceBegin, -Attack->TactileDamage, nullptr);
	//###########################################################################################
	
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s AttackStart %s"), *GetName(), *Attack->GetName());
	return EAttackAttemptResult::STARTED;//◘◘>

}

//==============================================================================================================
//ранее начатую атаку перевести или запланировать перевод в активную фазу
//==============================================================================================================
EAttackAttemptResult AMyrPhyCreature::AttackStrike (ECreatureAction ExactType, UPrimitiveComponent* ExactGoal)
{

	//действие по отлипанию от стены 
	if(ExactType == ECreatureAction::CLING_START)
		if(Mesh->gThorax.SeekingToCling)
		{	SetWannaClimb(false);
			return EAttackAttemptResult::OKAY_FOR_NOW;
		}

	//если просят применить атаку, а ее нет
	if (CurrentAttack == 255) return EAttackAttemptResult::WRONG_PHASE;//◘◘>

	//уведомить мозги и через них ноосферу о свершении в мире атаки
	if(MyrAI()) MyrAI()->LetOthersNotice(EHowSensed::ATTACKSTRIKE);

	//полная связка настроек
	auto Attack = GenePool->AttackActions[CurrentAttack];

	//в зависимости от того, в какой фазе вызвана просьба свершить действие
	switch (CurrentAttackPhase)
	{
	//фаза становления - перейти в бросок, но отложить удар до конца фазы
	case EActionPhase::ASCEND:
		AdoptAttackPhase(EActionPhase::RUSH);
		AdoptWholeBodyDynamicsModel(&Attack->DynModelsPerPhase[(int)CurrentAttackPhase], true);
		return EAttackAttemptResult::RUSHED_TO_STRIKE;//◘◘>

	//действие успешно запущено, но до готовности еще надо дойти
	case EActionPhase::READY:
		return AttackStrikePerform(ExactGoal);//◘◘>

	//поверх уже и так активной фазы - пока есть бюджет повторов, отправить вспять, чтобы ударить повторно
	case EActionPhase::STRIKE:
		if (Attack->NumInstantRepeats > CurrentAttackRepeat)
		{	AdoptAttackPhase(EActionPhase::DESCEND);
			AdoptWholeBodyDynamicsModel(&Attack->DynModelsPerPhase[(int)CurrentAttackPhase], true);
			return EAttackAttemptResult::GONE_TO_STRIKE_AGAIN;//◘◘>
		}

	//остальные фазы не приводят к смене действия
	case EActionPhase::RUSH:
	case EActionPhase::DESCEND:
		break;
	}
	return EAttackAttemptResult::FAILED_TO_STRIKE;//◘◘>
}

//==============================================================================================================
//непосредственный перевод начатой атаки в активную фазу
//==============================================================================================================
EAttackAttemptResult AMyrPhyCreature::AttackStrikePerform(UPrimitiveComponent* ExactGoal)
{
	//полная связка настроек
	auto Attack = GenePool->AttackActions[CurrentAttack];

	//проверка всех препонов, которые могут возникнуть на пути выполнения действия
	auto CheckRes = Attack -> ShouldIStartThisAttack(this, nullptr, nullptr, false);
	if(CheckRes != EAttackAttemptResult::OKAY_FOR_NOW) return CheckRes;//◘#>

	//перевести в фазу боя (внимание, если атака-прыжок, то подбрасывание тела здесь, внутри)
	AdoptAttackPhase(EActionPhase::STRIKE);

	//применить модель сил фазы страйк - ВНИМАНИЕ, если прыжок, то ранее была применена модель от имени режима поведения Soar, но она затёрлась
	AdoptWholeBodyDynamicsModel (&Attack->DynModelsPerPhase[(int)CurrentAttackPhase], true);

	//обнаружить, что мы пересекаем умный объём и передать ему, что мы совершаем, возможно, судьбоносное действие
	if (Attack->SendApprovalToTriggerOverlapperOnStrike)
		SendApprovalToOverlapper();

	if (Daemon)
	{
		//если эта фаза такаи подразумевает эффектное размытие резкого движения, то врубить его
		Daemon->SetMotionBlur ( Attack -> MotionBlurStrikeToEnd);

		//применить ИИ для более точного нацеливания
		if(!Daemon->UseActDirFromAI)
			if(MyrAI()->AimBetter(AttackDirection, 0.5))
				Daemon->UseActDirFromAI = true;
	}
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s AttackStrikePerform %d"), *GetName(), CurrentAttack);
	return EAttackAttemptResult::STROKE;//◘◘>
}


//==============================================================================================================
//реакция на достижение закладки GetReady в анимации любым из путей - вызывается только в этой самой закладке
//==============================================================================================================
void AMyrPhyCreature::AttackGetReady()
{
	//идём штатно с начала атаки - сменить режим
	if( CurrentAttackPhase == EActionPhase::ASCEND)
	{	AdoptAttackPhase(EActionPhase::READY);
		AdoptWholeBodyDynamicsModel (&GetCurrentAttackInfo()->DynModelsPerPhase[(int)CurrentAttackPhase], true);
	}

	//если вернулись в точку принятия решения вспять, на режиме DESCEND, это мы хотим немедленно повторить атаку
	else if(CurrentAttackPhase == EActionPhase::DESCEND)
	{
		//только если данный тип атаки позволяет делать дополнительные повторы в требуемом количестве
		//иначе так и остаться на режиме DESCEND до самого края анимации
		if(GetCurrentAttackInfo()->NumInstantRepeats > CurrentAttackRepeat)
		{	AttackStrikePerform();
			CurrentAttackRepeat++;
		}
	}

	//если атака одобрена заранее, мы домчались до порога и просто реализуем атаку
	else if (CurrentAttackPhase == EActionPhase::RUSH) AttackStrikePerform ();
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

	// что бы то ни было, а накопитель силы сбросить
	AttackForceAccum = 0.0f;

	// аварийный сброс всех удерживаемых предметов, буде таковые имеются
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
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s AttackEnd %s "), *GetName(), *GetCurrentAttackName());
	CurrentAttackPhase = EActionPhase::NONE;
	CurrentAttack = 255;
	CurrentAttackRepeat = 0;

}

//==============================================================================================================
//мягко остановить атаку, или исчезнуть, или повернуть всяпять 
//==============================================================================================================
void AMyrPhyCreature::AttackGoBack()
{
	if(NoAttack()){	UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s AttackGoBack WTF no attack"), *GetName());	return;	}

	//если финальный этап, то просто исчезнуть атаку
	if(CurrentAttackPhase == EActionPhase::FINISH)
		AttackEnd();

	//отход назад
	else
	{	AdoptAttackPhase(EActionPhase::DESCEND);
		AdoptWholeBodyDynamicsModel(&GetCurrentAttackInfo()->DynModelsPerPhase[(int)CurrentAttackPhase], true);
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s AttackGoBack %s "),	*GetName(), *GetCurrentAttackName());
	}

}


//==============================================================================================================
// атаки - отменить начинаемое действие, если еще можно
// для ИИ - чтобы прерывать начатую атаку, например, при уходе цели из виду - в любое время до фазы STRIKE (ASCEND, READY, RUSH)
// для игрока - по достижении закладки в ролике анимации, когда игрок зажал кнопку, но долго думает с отпусканием (READY)
//==============================================================================================================
bool AMyrPhyCreature::AttackNotifyGiveUp()
{
	//смысл точки - решение здаться, когда истекает ожидание (READY), в остальных фазах закладка не имеет смысла
	if(CurrentAttackPhase == EActionPhase::READY)
	{	AdoptAttackPhase(EActionPhase::DESCEND);
		AdoptWholeBodyDynamicsModel(&GenePool->AttackActions[CurrentAttack]->DynModelsPerPhase[(int)CurrentAttackPhase], true);
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s attack %d give up"), *GetName(), (int)CurrentAttack);
		return true;
	}
	return false;
}

//==============================================================================================================
//когда между активно фазой и концом есть еще период безопасного закругления, и метка этой фазытолько получена
//==============================================================================================================
void AMyrPhyCreature::AttackNotifyFinish()
{
	//если до этого атака была в активной фазе, то
	if(CurrentAttackPhase == EActionPhase::STRIKE)
	{
		//если в атаке использовалось автонацеливание, завершить его, полностью передать управление игроку
		if(Daemon)
			if(Daemon->UseActDirFromAI)
				Daemon->UseActDirFromAI = false;

		//перевести ее в фазу финального закругления
		AdoptAttackPhase(EActionPhase::FINISH);
		AdoptWholeBodyDynamicsModel(&GenePool->AttackInfo(CurrentAttack)->DynModelsPerPhase[(int)CurrentAttackPhase], true);
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



//◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘
//==============================================================================================================
//начать самодействие (внимание, проверка применимости здесь не производится, надо раньше проверять)
//==============================================================================================================
void AMyrPhyCreature::SelfActionStart(int SlotNum)
{
	if(Attacking())
		if (GetCurrentAttackInfo()->ForbidSelfActions)
		{	UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s SelfActionStart NO - forbidden by current attack"), *GetName());
			return;
		}

	//если слот самодействия доступен
	if (CurrentSelfAction == 255)
	{
		//теперь можно стартовать
		CurrentSelfAction = SlotNum;
		AdoptWholeBodyDynamicsModel(&GetCurrentSelfActionInfo()->DynModelsPerPhase[SelfActionPhase], GetCurrentSelfActionInfo()->UseDynModels);

		//отправить символический звук выражения, чтобы очевидцы приняли во внимание
		if(GetCurrentSelfActionInfo()->Type == ECreatureAction::SELF_EXPRESS1)
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
		AdoptWholeBodyDynamicsModel(&GetCurrentSelfActionInfo()->DynModelsPerPhase[SelfActionPhase], GetCurrentSelfActionInfo()->UseDynModels);
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
		if (Mesh->DynModel->Sound)
		{
			//прервать этот звук
			Voice->Stop();
			CurrentSpelledSound = EPhoneme::S0;
		}

		//отнять силы, сообразно трудности действия, вне зависимости, закончилось оно или прервалось
		//это какое-то мутное дополнение к отъёму удельному по фазам, наверно нах
		//HealthChange(-GenePool->SelfActions[(int)CurrentSelfAction]->DeltaHealthAtFail);
		//StaminaChange(-GenePool->SelfActions[(int)CurrentSelfAction]->DeltaStaminaAtFail);

		//собственно, очистить переменную 
		CurrentSelfAction = 255;
		SelfActionPhase = 0;

		//применить новые настройки динамики движения/поддержки для частей тела поясов
		AdoptWholeBodyDynamicsModel(&BehaveCurrentData->WholeBodyDynamicsModel, true);
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s SelfActionCease"), *GetName());
	}
}

//==============================================================================================================
//найти и запустить (безусловно, как команда из когда) самодействие по его идентификатору темы
//==============================================================================================================
void AMyrPhyCreature::SelfActionFindStart(ECreatureAction Type, bool ExtCheckChance)
{
	EAttackAttemptResult AreWeAble = EAttackAttemptResult::OKAY_FOR_NOW;
	int WhatToStart = 255;

	//отыскать подходящее, полную оценку применимости производить кроме вероятности
	//так как та строчка кода, откуда это вызывается, по идее должна знать, почему вызывается именно это
	for(int i=0; i<GenePool->SelfActions.Num(); i++)
		if(GenePool->SelfActions[i]->Type == Type)
		{	
			//провести почти полную оценку применимости, первый раз - без оценки шанса, остальные - с оценкой
			//хотя правильнее искать оптимальные критерии, но это сложно и долго
			bool CheckForChance = (WhatToStart != 255) && ExtCheckChance;
			AreWeAble = GenePool->SelfActions[i]->IsActionFitting(this, CheckForChance);
			if(AreWeAble != EAttackAttemptResult::OKAY_FOR_NOW)
				continue;

			//предварительно найденный
			WhatToStart = i;
		}

	//стартовать то, что в итоге накапло
	if (WhatToStart >= GenePool->SelfActions.Num())
	{	UE_LOG(LogMyrPhyCreature, Error, TEXT("ACTOR %s SelfActionFindStart %s NotFound, Verdict %s"), *GetName(),
		*TXTENUM(ECreatureAction, Type), *TXTENUM(EAttackAttemptResult, AreWeAble));
	}
	else
	{	SelfActionStart(WhatToStart);
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s SelfActionFindStart %s OK"), *GetName(), *TXTENUM(ECreatureAction, Type));
	}

}

//==============================================================================================================
//найти список применимых в данный момент самодействий и внести их в массив
//==============================================================================================================
void AMyrPhyCreature::ActionFindList(bool RelaxNotSelf, TArray<uint8>& OutActionIndices, ECreatureAction Restrict)
{
	//подходящий массив нужный пользователю
	auto& Source = RelaxNotSelf ? GenePool->RelaxActions : GenePool->SelfActions;

	//отыскать подходящее, изнутри вызвав оценку применимости
	for(int i=0; i<Source.Num(); i++)

		//если ограничение на константу темы действия
		if(Restrict==ECreatureAction::NONE || Source[i]->Type == Restrict)

			//полная проверка всех критериев применимости, за исключение вероятности
			if(Source[i]->IsActionFitting(this, false) == EAttackAttemptResult::OKAY_FOR_NOW)

				//адресация по индексу, адресат должен иметь указатель на генофонд существа для получения списка
				OutActionIndices.Add(i);
		
}



//◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘
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
	AdoptWholeBodyDynamicsModel(&GetCurrentRelaxActionInfo()->DynModelsPerPhase[RelaxActionPhase], GetCurrentRelaxActionInfo()->UseDynModels);
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s RelaxActionStart %s"), *GetName(), *GetCurrentRelaxActionInfo()->GetName());

}
//==============================================================================================================
//найти, выбрать и начать анимацию отдыха - в ответ на нажатие кнопки отдыхать
//==============================================================================================================
bool AMyrPhyCreature::RelaxActionFindStart()
{
	//уже деется какое-то действие по отдыху
	if(!NoRelaxAction()) return false;

	EAttackAttemptResult res;
	for(int i=0; i<GenePool->RelaxActions.Num(); i++)
	{
		//отыскать подходящее, изнутри вызвав оценку применимости
		res = GenePool->RelaxActions[i] -> IsActionFitting(this,false);

		// защита от дурака, фаз в анимации отдыха должено быть строго 3: вход, сидение, выход
		if(res == EAttackAttemptResult::OKAY_FOR_NOW && GenePool->RelaxActions[i]->DynModelsPerPhase.Num() == 3 )
		{
			RelaxActionStart(i);
			UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s RelaxActionFindStart OK"), *GetName());
			return true;
		}
	}
	UE_LOG(LogMyrPhyCreature, Warning, TEXT("ACTOR %s RelaxActionFindStart %s"), *GetName(), *TXTENUM(EAttackAttemptResult, res));
	return false;
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
		AdoptWholeBodyDynamicsModel(&GetCurrentRelaxActionInfo()->DynModelsPerPhase[RelaxActionPhase], GetCurrentRelaxActionInfo()->UseDynModels);
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s RelaxActionReachPeace %s"), *GetName(), *GetCurrentRelaxActionInfo()->GetName());
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
		UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s RelaxActionStartGetOut %s"), *GetName(), *GetCurrentRelaxActionInfo()->GetName());
	}
}

//==============================================================================================================
//достичь конца и полного выхода (вызывается из закладки анимации MyrAnimNotify)
//==============================================================================================================
void AMyrPhyCreature::RelaxActionReachEnd()
{
	//если реально делаем жест отдыха в фазе установившегося действия - перейти к следующей фазе, закруглению
	if (CurrentRelaxAction != 255 && RelaxActionPhase == 2)
	{	RelaxActionPhase = 0;
		CurrentRelaxAction = 255;
		
		//вернуть настройки динамики уровня выше - уровня состояния поведения
		AdoptWholeBodyDynamicsModel(&BehaveCurrentData->WholeBodyDynamicsModel, true);
	}
}


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
	AttackEnd();

	//не смогли съесть, остальное значит не нужно
	if (!result) return false;

	//найти и создать объект останков
	AActor* Remains = nullptr;
	if (dFood->WhatRemains)
		Remains = GetWorld()->SpawnActor(*dFood->WhatRemains, &Food->GetComponentTransform());

	//перед деструкцией будет отвязан ИИ
	Food->GetOwner()->Destroy();
	UE_LOG(LogMyrPhyCreature, Log, TEXT("ACTOR %s Eat %s"), *GetName(), *Food->GetOwner()->GetName());
	return true;
}

//==============================================================================================================
//обнаружить, что мы пересекаем умный объём и передать ему, что мы совершаем, возможно, судьбоносное действие
//==============================================================================================================
void AMyrPhyCreature::SendApprovalToOverlapper()
{
	TSet<UPrimitiveComponent*> OC;
	GetOverlappingComponents(OC);
	if (OC.Num())
		for (auto C : OC)
			if (auto TV = Cast<UMyrTriggerComponent>(C))
				TV->ReceiveActiveApproval(this);
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
		if (GetCurrentAttackInfo()->SendApprovalToTriggerOverlapperOnStart)
			return true;
		if (GetCurrentAttackInfo()->SendApprovalToTriggerOverlapperOnStrike)
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
		if(!NoRelaxAction()) GetCurrentRelaxActionInfo()->MetabolismMult;
		if(!NoSelfAction()) GetCurrentSelfActionInfo()->MetabolismMult;
	}
	else Power = 0;
}

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
	if(Mesh->gThorax.StandHardness > 0.4)
	{	auto pg = GetMyrGameInst()->Surfaces.Find(Mesh->Thorax.Surface);
		if (pg) ThoraxStealthFactor = pg->StealthFactor;
	}
	if(Mesh->gPelvis.StandHardness > 0.4)
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
		if (auto AFS = GetCurrentAttackInfo()->WhatLimbsUsed.Find(eLimb))
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