// Fill out your copyright notice in the Description page of Project Settings.


#include "Control/MyrAI.h"
#include "../Creature/MyrPhyCreature.h"						// управляемое существо
#include "../Artefact/MyrLocation.h"						// локация - для нацеливания на компоненты маячки в составе целиковой локации
#include "../Artefact/MyrArtefact.h"						// артефакт - тоже может быть целью
#include "../Dendro/MyrDendroMesh.h"						// для обработки вариантов поведения от сидения на дереве
#include "../MyrraGameInstance.h"							// глобальный мир игры
#include "../MyrraGameModeBase.h"							// уровень - брать протагониста
#include "../AssetStructures/MyrLogicEmotionReactions.h"	// эмоции по событиям

#include "Curves/CurveLinearColor.h"				//для высчета скоростей по кривым

#include "AIModule/Classes/Perception/AIPerceptionComponent.h"
#include "AIModule/Classes/Perception/AISenseConfig_Hearing.h"

#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"						// для вызова GetAllActorsOfClass

#include "DrawDebugHelpers.h"						// для рисования

uint32 AMyrAI::RandVar = 1;

//свой лог
DEFINE_LOG_CATEGORY(LogMyrAI);

#define TRA(br, cmd, gain, cond)       if(cond) { Drive.Gain = gain; Drive.DoThis = ECreatureAction::##cmd; MoveBranch=br; return;  }
#define BEH1(b1)      (me()->CurrentState == EBehaveState::##b1)
#define BEH2(b1, b2)  (me()->CurrentState == EBehaveState::##b1 || me()->CurrentState == EBehaveState::##b2)

//==============================================================================================================
// конструктор
//==============================================================================================================
AMyrAI::AMyrAI(const FObjectInitializer & ObjInit)
{
	// тик ИИ - редко, два раза в секунду, вообще это число надо динамически подстраивать
	PrimaryActorTick.TickInterval = 0.5f;
	PrimaryActorTick.bCanEverTick = true;

	//модуль перцепции
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception Component"));
	SetPerceptionComponent(*AIPerception);
	AIPerception->bEditableWhenInherited = true;

	//настройки слуха, предварительные - они должны меняться при подключении к цели
	ConfigHearing = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Myrra Hearing"));
	if (ConfigHearing)
	{	ConfigHearing->HearingRange = 1000.f;
		ConfigHearing->DetectionByAffiliation.bDetectEnemies = true;
		ConfigHearing->DetectionByAffiliation.bDetectNeutrals = true;
		ConfigHearing->DetectionByAffiliation.bDetectFriendlies = true;
		AIPerception->ConfigureSense(*ConfigHearing);
	}
	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	//привзять обработчики событий
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AMyrAI::OnTargetPerceptionUpdated);
}

//==============================================================================================================
//событие восприятия - на данный момент, когда ловим звук или спец-сообщение от другого существа
//==============================================================================================================
void AMyrAI::OnTargetPerceptionUpdated (AActor * Actor, FAIStimulus Stimulus)
{
	//это вообще бред, почему себя нельзя исключить
	if (Actor == me()) return;

	//выделить из актора конкретный компонент-источник-цель, единого рецепта нет, придётся разбирать
	UPrimitiveComponent* ExactObj = (UPrimitiveComponent*)Actor->GetRootComponent();
	if (auto L = Cast<AMyrLocation>(Actor))
		ExactObj = (UPrimitiveComponent*)L->GetCurrentBeacon();
	else if (auto M = Cast<AMyrPhyCreature>(Actor))
		ExactObj = M->GetMesh();

	//слух и другие мгнровенные воздействия без пространства
	EHowSensed HowSensed = EHowSensed::HEARD;
	float Strength = Stimulus.Strength;
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && Stimulus.WasSuccessfullySensed())
	{
		//спецсигналы - расщепляем "силу" на целый идентификатор и дробную силу
		if(Stimulus.Tag == TEXT("HeardSpecial"))
		{	HowSensed = (EHowSensed)((int)Stimulus.Strength);
			Strength = Strength - (int)Stimulus.Strength;
		}


		//проталкиваем стимул глубже - на уровень замечания и рассовывания по ячейкам целей
		//UE_LOG(LogTemp, Error, TEXT("%s AI Notices %s result %s"), *me()->GetName(), *Actor->GetName(), *TXTENUM(EGoalAcceptResult, NotRes));
		auto NotRes = Notice (ExactObj, HowSensed, Strength, nullptr);

	}
}

//==============================================================================================================
//обработчик занятия объекта (при его появлении или в начале игры/уровня
//==============================================================================================================
void AMyrAI::OnPossess(APawn * InPawn)
{
	Super::OnPossess(InPawn);

	//переименовать, чтобы в отладке в редакторе отображалось имя, ассоциированное с подопечным существом
	FString NewName = InPawn->GetName() + TEXT("-") + GetName();
	Rename(*NewName);
	UpdateSenses();
}

//==============================================================================================================
// начать игру / ввести экземпляр контроллера в игру
//==============================================================================================================
void AMyrAI::BeginPlay()
{
	//интервал тика можно динамически менять - например при неходьбе нах каждый кадр?
	PrimaryActorTick.TickInterval = 0.5f;
	Super::BeginPlay();
	if (me())
	{	if (me()->IsUnderDaemon()) AIRuleWeight = 0.0f;
		UAIPerceptionSystem::RegisterPerceptionStimuliSource(ME(), UAISense_Hearing::StaticClass(), ME());
	}
}

//==============================================================================================================
// рутинные действия
//==============================================================================================================
void AMyrAI::Tick(float DeltaTime)
{
	//часть медленных не нужных часто процедур самого тела, дабы не плодить тиков и делителей, вынесена сюда
	ME()->RareTick(DeltaTime);

	//внутря
	Super::Tick(DeltaTime);

	//возможно, рандом тяжёл, пусть он случается реже основных расчётов
	RandVar = FMath::Rand();
	if (RandVar == 0) RandVar = 1;

	//аналоговые тяги пригнуться и развернуться (последняя скорее всего нах)
	float FacingAmount = 0;
	float StealthAmount = 0;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	//пока неясно, как ИИ обрабатывать смерть, возможно просто остановить тик
	//но на всякий случай просто тик укорачивается
	if (me()->Dead())
	{
		PrimaryActorTick.TickInterval = 1.6;
		AttemptSuicideAsPlayerGone();
		return;//◘◘>
	}else


	//LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
	//#~~#~~# если нет никаких целей - искусственным образом притянуть за уши какие-то цели
	//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	if(Goals[PrimaryGoal].Object == Goals[SecondaryGoal].Object)
	{
		Drive.MoveDir = me()->GetAxisForth();		//самозавод по позиции тела
		PrimaryActorTick.TickInterval = 0.6;		//редкий такт
		if(AttemptSuicideAsPlayerGone())			//если высран, убрать когда игрок далеко
			return;//◘◘>
	}else

	//если есть первичная цель, значит хотя бы одна цель есть, и есть, что обсчитывать
	if (Goals[PrimaryGoal].Object)
	{
		//обнулить курс и скорость перед аккумуляцией новых
		CleanMotionParameters(0);

		//LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
		//#◘◘#◘◘# есть и вторичная цель внимания, значит 2 цели, и нужно раскорячивать внимание между ними
		//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		if(Goals[SecondaryGoal].Object)
		{
			//контейнеры для результатов трассировк
			FHitResult Trace1(ForceInit);
			FHitResult Trace2(ForceInit);

			//полностью пересчитать все парметры целей
			MeasureGoal (Goals[PrimaryGoal],   nullptr, DeltaTime, false, EHowSensed::ROUTINE, &Trace1);
			MeasureGoal (Goals[SecondaryGoal], nullptr, DeltaTime, false, EHowSensed::ROUTINE, &Trace2);

			//если разность весов изменилась, и первичная цель уже выглядит как не первичная
			if (Goals[PrimaryGoal].Weight < Goals[SecondaryGoal].Weight)
			{
				//поменять местами индексы цели (не меняя целей)
				PrimaryGoal = SecondaryGoal;
				SecondaryGoal = 1 - PrimaryGoal;

				//если атака в разгаре - мягко прервать, обратив движение
				if (me()->DoesAttackAction())
					ME()->NewAttackGoBack();
			}

			//при экстремальном превышении веса исключить вторичную и просчитать тягу для одной первичной
			if (Goals[PrimaryGoal].Weight > 10 * Goals[SecondaryGoal].Weight)
			{	Goals[PrimaryGoal].Weight = 1.0f;
				Goals[SecondaryGoal].Weight = 0.0f;

				//сразу задать глобальноую аналоговую тягу, потому что других целей движения нет
				Drive.Gain = CalcAdditiveGain(Goals[PrimaryGoal], StealthAmount, FacingAmount);

				//проложить путь к единственной цели
				Goals[PrimaryGoal].RouteResult = Route(Goals[PrimaryGoal], Remember(Goals[PrimaryGoal].Object), &Trace1);
				Drive.MoveDir = Goals[PrimaryGoal].MoveToDir;
			}
			//здоровая конкуренция между двумя целями
			else
			{
				//нормализация весов, чтобы можно было взвешенно суммиировать воздействия каждой цели
				const float Normalizer = 1.0 / (Goals[PrimaryGoal].Weight + Goals[SecondaryGoal].Weight);
				Goals[PrimaryGoal].Weight *= Normalizer;
				Goals[SecondaryGoal].Weight *= Normalizer;
			
				//степень раскоряки между 2 целями, если близко к 1.0 - значит по пути, -1.0 = в разных сторонах
				float MindSplit = (Goals[PrimaryGoal].LookAtDir | Goals[SecondaryGoal].LookAtDir);

				//цели примерно в одном напавлении + главная значительно важнее вторичной = дискретно делить полномочия
				bool Look2Follow1 = (MindSplit > 0.0f && Goals[PrimaryGoal].Weight > 0.6 * Normalizer);

				//отдельно вычисляем тяги к целям
				float Gain1 = CalcAdditiveGain(Goals[PrimaryGoal],		StealthAmount, FacingAmount);
				float Gain2 = CalcAdditiveGain(Goals[SecondaryGoal],	StealthAmount, FacingAmount);
				Drive.Gain = Gain1 + Gain2 * MindSplit;

				//режим "смотрю на вторую цель, иду к первой", направление взгляда не должно искажеть направление атаки
				if (Look2Follow1)
				{	
					//маршрут рассчитать только для первой цели
					Goals[PrimaryGoal].RouteResult = Route(Goals[PrimaryGoal], Remember(Goals[PrimaryGoal].Object), &Trace1);
					Drive.MoveDir = Goals[PrimaryGoal].MoveToDir;

					//смотреть на вторую цель только если нет атаки (иначе внимание концентрируется самой атакой на жертву)
					if(me()->NoAttack()) Drive.ActDir = Goals[SecondaryGoal].LookAtDir;
				}
				//цели по разные стороны от нас и примерно равны по важности
				//режим раскоряки с отвлекателем (прокладывается средний маршрут)
				else
				{
					//отдельно просчитать удобные пути к двум целям
					Goals[PrimaryGoal].  RouteResult =  Route (Goals[PrimaryGoal],   Remember(Goals[PrimaryGoal].Object),   &Trace1);
					Goals[SecondaryGoal].RouteResult =  Route (Goals[SecondaryGoal], Remember(Goals[SecondaryGoal].Object), &Trace2);

					//выбрать смешанный, средний путь
					Drive.MoveDir = FMath::Lerp(Goals[PrimaryGoal].MoveToDir,Goals[SecondaryGoal].MoveToDir, Goals[SecondaryGoal].Weight);

					//куда смотреть: то на одну, то на другую
					if(ChancePeriod(Goals[PrimaryGoal].Weight)) 
						Drive.ActDir = Goals[PrimaryGoal].LookAtDir;
					else Drive.ActDir = Goals[SecondaryGoal].LookAtDir;
				}
			}
		}
		//LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
		//#◘◘#~~# первичная цель только одна
		//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		else
		{
			//полностью пересчитать все парметры цели
			FHitResult Trace1(ForceInit);
			MeasureGoal (Goals[PrimaryGoal], nullptr, DeltaTime, true, EHowSensed::ROUTINE, &Trace1);
			Drive.Gain = CalcAdditiveGain(Goals[PrimaryGoal], StealthAmount, FacingAmount);

			
			//проложить путь к единственной цели
			Goals[PrimaryGoal].RouteResult = Route(Goals[PrimaryGoal], Remember(Goals[PrimaryGoal].Object), &Trace1);
			Drive.MoveDir = Goals[PrimaryGoal].MoveToDir;
			Drive.ActDir = Goals[PrimaryGoal].LookAtDir;

			//сдвиг в группе смыслов движения: первая группа - к цели, вторая от цели, внутри группы разные способы
			//if(Goals[PrimaryGoal].RouteResult == ERouteResult::Away_Directly)
			//	MoveMnemo = EMoveResult::Walk_From_Goal;
			//else MoveMnemo = EMoveResult::Walk_To_Goal;
		}

		/////////////////////////////////////////////////////////////
		//если есть хотя бы 1 цель и уже выполняется атака - мож помочь ей довыполнить
		//ВНИМАНИЕ! - это только для чистого ИИ, иначе будет вмешиваться и не давать завершать
		if (AIRuleWeight > 0.5 && me()->Attacking())
		{	
			//быстро перепроверить применимость атаки
			auto R = me()->GetAttackAction()->RetestForStrikeForAI (me(), Goal_1().Object, &Goal_1());

			//если ранее начатая к подготовке атака еще актуальна для удара
			if(ActOk(R))	
			{	PrimaryActorTick.TickInterval = 0.15;				// быстрое обновление 
				Drive.ActDir = Goals[PrimaryGoal].LookAtDir;		// смотреть строго на цель атаки
				R = me()->NewAttackStrike();						// попытаться запустить напрямую
				UE_LOG(LogMyrAI, Warning, TEXT("AI %s NewAttackStrike cause %s"), *me()->GetName(), *TXTENUM(EAttackAttemptResult, R));
			}
			//если атаку нельзя продолжить по материальным причинам, а не потому что уже фаза не та
			else
			{	PrimaryActorTick.TickInterval = 0.25;				// успокоить метаболизм
				me()->NewAttackEnd();
				UE_LOG(LogMyrAI, Warning, TEXT("AI %s NewAttackEnd cause %s"), *me()->GetName(), *TXTENUM(EAttackAttemptResult,R));
			}
		}
	}

	////////////////////////////////////////////////////
	//досужий перебор всей палитры действий, чтобы, если повезет и подходит, рандомно что-то начинать
	//выполняется как для игрока так и для непися, просто ненужные, своевольные действия должны иметь ChancrForPlayer=0
	for(int i=0; i<me()->GetGenePool()->Actions.Num(); i++)
	{
		uint8 VictimType = 255;
		auto A = me()->GetGenePool()->Actions[i];
		auto R = A -> IsActionFitting (me(), Goal_1().Object, &Goal_1(), VictimType, true, true);
		//UE_LOG(LogMyrAI, Warning, TEXT("AI %s Trying %s cause %s"), *me()->GetName(), *A->HumanReadableName.ToString(), *TXTENUM(EAttackAttemptResult, R));
		if(ActOk(R))
		{
			//если нашли атаку
			if(A->IsAttack())
			{
				//направить ее на обидчика и стартануть
				Drive.ActDir = Goal_1().LookAtDir;
				ME()->NewAttackStart(i, VictimType);
			}

			//самодействие
			else ME()->SelfActionStart(i);
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	//влияние общей памяти о пережитом событии на эмоцию 
	MyrLogicTick(EventMemory, false);

	//новый способ отражения эмоционального метаболизма
	EmoMemory.Tick();
	
	//пересчитать интегральную эмоцию, чтобы следующие кадры использовать для анимации
	RecalcIntegralEmotion();

	//отсечение ненужной возни, если ИИ в теле игрока мало используется
	if (AIRuleWeight < 0.1) return; //◘◘>

	////////////////////////////////////////////////////////////////////////////////////
	//небольшие корректировки аналоговой тяги перед принятием решения

	//когда здоровье достигло 1/2 - тяга ослабляется только если изначально низка, сильные же рывки не затрагиваются
	if(me()->Health < 0.5)
		if(Drive.Gain < 0.9 - 0.3*me()->Health)
			Drive.Gain *= 2 * me()->Health;

	//когда усталость достигла 1/5 - просто домножается так, чтобы меньше 1/5 ослабляло тягу с 1 до 0
	if(me()->Stamina < 0.2)
		Drive.Gain *= 5 * me()->Stamina;

	//когда боль достигла 1/2 - просто домножается так, чтобы меньше 1/2 ослабляло тягу с 1 до 0
	if(me()->Pain > 0.5)
		if(Drive.Gain < 0.9 - 0.3*me()->Pain)
			Drive.Gain *= 2 * me()->Pain;

	//для аддитивного установления признака
	EMoveResult TempMoveMnemo = EMoveResult::NONE;

	////////////////////////////////////////////////////////////////////////////////////
	// из аналоговых намерений перейти посредством конечного автомата к дискретным
	// решением относительно двигательного поведения
	// пока неясно, заводить ли отдельный энум, или базировать на BehaveState, как 
	// это уже делается на уровне PhyCreature, то есть де-факто делать надстройку
	// над BehaveState-конечным автоматом
	////////////////////////////////////////////////////////////////////////////////////

	//если вошли в триггер-объём с установленными векторами движения - отклонить ранее посчитанный вектор в соответствии
	if(me()->ModifyMoveDirByOverlap(Drive.MoveDir, EWhoCame::Creature))
	{	
		UE_LOG(LogMyrAI, Warning, TEXT("AI %s ModifyMoveDirByOverlap %s"), *me()->GetName(), *Drive.MoveDir.ToString());
		Drive.MoveDir.Normalize();
	}


	//степень соосности желаемого курса и тела, насколько передом вперед
	float KeepDir = FMath::Max(0.0f, Drive.MoveDir | me()->GetThorax()->Forward);
	bool OnAir = !me()->GotLandedAny();

	//определение высоты над землёй - пока неясно, нужно ли это для нелетающих
	if(OnAir)
	{	FHitResult Hit;
		FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("AI_TraceForAltitude")), false, me());
		RV_TraceParams.AddIgnoredActor(me());			// чтобы не застили части нас
		GetWorld()->LineTraceSingleByChannel (Hit, me()->GetHeadLocation(), me()->GetHeadLocation() + FVector::DownVector*10000, ECC_WorldStatic, RV_TraceParams);
		FlyHeight = Hit.Distance;
	}

	//признак того, что хотим двигаться прочь от цели
	bool Away = ((int)Goals[PrimaryGoal].RouteResult >= (int)ERouteResult::Away_Directly);

	//отдельный случай для летающего существа
	if (me()->CanFly())
	{
		//поднялись достаточно высоко, можно опускаться
		float TerminalHeight = me()->GetBodyLength() * 10;
		bool FlewHigh = (FlyHeight > TerminalHeight);

		//достаточно далеко от цели
		bool TooFar = (Goal_1().LookAtDist > me()->GetBodyLength() * 20);

		//включение механик полета
		TRA(3, TOGGLE_SOAR, Drive.Gain, (TooFar || Away) && (!FlewHigh));								// взлетать если надо улепетывать или птица достаточно далеко от цели (пока не надо пикировать в нее)
		TRA(2, TOGGLE_FLY, Drive.Gain, FlewHigh);										// переходить с подъёма на нормальный полёт, когда очень высоко 
		TRA(1, TOGGLE_WALK, Drive.Gain, OnAir && Drive.Gain > 1 && !Away && !TooFar);	// пикировать, когда цель близко, тяга к ней высока 

	}

	//очень высокая тяга
	if(Drive.Gain > 1.0f)
	{
		//стартовая небольшая вариация скорости (вокруг единицы) - для большой тяги способ стабилизировать
		float AltGain = 0.8f + 0.2f * Drive.Gain;
		TRA ( 2, TOGGLE_CROUCH,		AltGain,		StealthAmount > 0.7 + 0.6 * Paranoia);		// при высокой тяге паранойя сильно отвращает от скрадывания, и вообще хочется бежать а не ползти
		TRA ( 4, TOGGLE_WALK,		0.5 + 0.3*KeepDir,	KeepDir < 0.5);								// в бегу перейти на шаг, если нажо резко развернуться
		TRA ( 5, TOGGLE_HIGHSPEED,	AltGain,		true);										// перейти на бег - если всё прочее не сработало, то важна только тяга
	}
	//очень низкая тяга
	else if(Drive.Gain < 0.4f)
	{
		//стартовый базис скорости - периодичность
		float AltGain = Period(0, 1, 2*Drive.Gain);
		TRA ( 7, TOGGLE_CROUCH,		AltGain,		StealthAmount > 0.8);						// очень исльная тяга скрадывания - чередовать скрадывание и застывание
		TRA ( 8, TOGGLE_CROUCH,		Drive.Gain,		StealthAmount > 0.4 + 0.2 * Paranoia);		// очень сильная тяга скрыться - чередовать стойку и скрадывание
		TRA ( 9, TOGGLE_WALK,		AltGain,		true);										// по умолчанию прерывистый шаг, так как медленный шаг некрасив, если в полёте, то парение вниз
	}
	//нормальная тяга
	else
	{	TRA (10, TOGGLE_CROUCH,		Drive.Gain,		StealthAmount > 0.6+0.3*Paranoia);			// скрадывание
		TRA (11, TOGGLE_WALK,		Drive.Gain,		true);										// стандартное хождение по земле от или на
		TRA ( 3, TOGGLE_SOAR,		Drive.Gain,		me()->CanFly() && Away);					// при средней тяге взлёт с места только если от цели улепетываем
	}

}

//==============================================================================================================
//нужно, когда существо исчезает в норе или на дистанции
//==============================================================================================================
void AMyrAI::OnUnPossess()
{
	//по какому-то маразму разработчиков для одного ИИ контроллера это событие вызывается 2 раза  
	//сначала до отделения Pawn, потому после, когда эта переменная уже 0
	if (!me()) return;
	UE_LOG(LogMyrAI, Warning, TEXT("AI %s is being destroyed"), *me()->GetName());

	//практика показывает, что удалять все ссылки в ИИ других персонажей надо незамедлительно и в лоб
	//иначе ИИ растягивает обновление до времени после удаления объекта, что приводит к ошибкам
	TArray<AActor*> FoundCreatures;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyrPhyCreature::StaticClass(), FoundCreatures);
	for (auto& Actor : FoundCreatures)
	{
		//вот таким черезжопным способом исключить себя
		if (Actor != this)
		{
			//для каждого потенциального свидетеля вызвать спец-функцию
			//которая удаляет ссылку на нас
			auto Myr = Cast<AMyrPhyCreature>(Actor);
			if (Myr->MyrAI())
				Myr->MyrAI()->SeeThatObjectDisappeared(me()->GetMesh());
		}
	}

	//остановить тик, потому что за каким-то хером он между утерей подопечного и самодестроем продолжает тичить
	PrimaryActorTick.SetTickFunctionEnable(false);

	//остановить дополнительную тик-функцию, а то после отъёма объекта управления она почему-то еще работает
	//CineTickFunc.SetTickFunctionEnable(false);
	Super::OnUnPossess();
}

//==============================================================================================================
//прозвучать "виртуально", чтобы перцепция других существ могла нас услышать
//==============================================================================================================
void AMyrAI::LetOthersNotice (EHowSensed Event, float Strength)
{	UAISense_Hearing::ReportNoiseEvent(me(), me()->GetHeadLocation(), (float)Event + FMath::Max(Strength*0.1f, 0.9f), me(), 0, TEXT("HeardSpecial"));	}



//==============================================================================================================
//уведомить систему о прохождении звука ( вызывается из шага, из MyrAnimNotify.cpp / MakeStep)
//==============================================================================================================
void AMyrAI::NotifyNoise(FVector Location, float Strength)
{
	//прозвучать "виртуально", чтобы перцепция других существ могла нас услышать
	UAISense_Hearing::ReportNoiseEvent(me(), Location, Strength, me());
}

//==============================================================================================================
//вспомнить или запомнить встреченного актера - возвратить индекс во внутреннем массиве для быстрой адресации
//==============================================================================================================
FGestaltRelation* AMyrAI::Remember (UPrimitiveComponent* NewObj)
{
	//получить индекс в контейнере
	auto PRel = Memory.Find(NewObj);
	if (PRel) return PRel;

	//первый раз увидели - залить эмоцию из архетипичных для класса
	auto& NeuGestalt = Memory.Add(NewObj);
	NeuGestalt.NeverBefore = 1;
	NeuGestalt.FromFColor ( ClassRelationToClass(NewObj->GetOwner()) );
	return &NeuGestalt;
}


//==============================================================================================================
//если мы прокачались в чувствительности, изменить радиусы зрения, слуха...
//==============================================================================================================
void AMyrAI::UpdateSenses()
{
	//контроллер не висит в пустоте - обновить 
	if(me())
	{	ConfigHearing->HearingRange	= me()->GetGenePool()->DistanceHearing;
		AIPerception->RequestStimuliListenerUpdate();
	}
}

//==============================================================================================================
//выполнить трассировку к цели чтобы явно пересчитать видимость цели или второстепенного объекта
//==============================================================================================================
bool AMyrAI::TraceForVisibility (UPrimitiveComponent *Object, FGestaltRelation* Gestalt, FVector ExactRadius, FHitResult* Hit)
{
	//подготовить всякую хрень для трассировки, исключить себя и цель
	FVector Start = me()->GetHeadLocation();		// трассировать из глаз
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("AI_TraceForVisibility")), false, me());
	RV_TraceParams.bReturnPhysicalMaterial = true;	// может, и не нужно
	RV_TraceParams.AddIgnoredActor(me());			// чтобы не застили части нас

	//собсьвенно, провести трассировку
	GetWorld()->LineTraceSingleByChannel (*Hit, Start, Start + ExactRadius, ECC_WorldStatic, RV_TraceParams);
	
	//собственно, обновление глобального флага видимости объекта
	Gestalt->NowSeen = (Hit->Component.Get() == Object); 
	return Gestalt->NowSeen;
}


//==============================================================================================================
//забрать образ в текущую операционную часть ИИ - и вернуть указатель на ячейку, где вся подноготная варится
//==============================================================================================================
EGoalAcceptResult AMyrAI::Notice (UPrimitiveComponent* NewGoalObj, EHowSensed HowSensed, float Strength, FGoal** WhatNoticed)
{
	EGoalAcceptResult WhatToReturn;								// что возвращатать (чтобы еще пошаманить с результатом до выхода)
	auto Gestalt = Remember(NewGoalObj);						// образ, созданный только что или уже 
	int GoalToAddIndex = Gestalt->InGoal0 ? 0 : (Gestalt->InGoal1 ? 1 : 2);	// номер ячейки цели, в которую добавляется этот образ

	//МОДИФИКАЦИЯ, если образ ранее был запомнен как находящийся в одной из ячеек
	if(GoalToAddIndex < 2)
	{
		//разместить новые данные (но метрики не пересчитывать для оптимизации, дождаться тика)
		PraePerceive(Goals[GoalToAddIndex], HowSensed, Strength, NewGoalObj, Gestalt);
		WhatToReturn = (EGoalAcceptResult)((int)EGoalAcceptResult::ModifyFirst + GoalToAddIndex); //◘◘>
		PostPerceive(Goals[GoalToAddIndex], HowSensed, Strength, Gestalt, WhatToReturn);
	}
	//если образ не находится ни в одной из ячеек
	else
	{
		//номер ячейки цели на основе свободности ячейки 0
		GoalToAddIndex = (bool)(Goals[0].Object);

		//если в полученной ячейке пусто, то разместить стимул в ней
		if(!Goals[GoalToAddIndex].Object)
		{
			//разместить новые данные на пустой ячейке - значит, полностью пересчитать (MeasureGoal)
			PraePerceive(Goals[GoalToAddIndex], HowSensed, Strength, NewGoalObj, Gestalt);
			MeasureGoal (Goals[GoalToAddIndex], Gestalt, 0, true, HowSensed);
			WhatToReturn = (EGoalAcceptResult)((int)EGoalAcceptResult::AdoptFirst + GoalToAddIndex); //◘◘>
			PostPerceive(Goals[GoalToAddIndex], HowSensed, Strength, Gestalt, WhatToReturn);
		}
		//если вторая ячейка занята, значит все ячейки заняты
		else
		{	
			//создаём 3ью псевдоцель, оцениваем ее вес, но PostPerceive пока рано
			FGoal CandidateGoal;
			PraePerceive(CandidateGoal, HowSensed, Strength, NewGoalObj, Gestalt);
			MeasureGoal(CandidateGoal, Gestalt, 0, true, HowSensed);

			//если новая цель более весомая, чем вторичная (а вторичная априори бессмысленней первичной)
			if (RecalcGoalWeight(Goals[SecondaryGoal]) < CandidateGoal.Weight)
			{
				//заменить ячейку (не забыв выгрузить старую) и пометить адрес в гешатльте
				Forget(Goals[SecondaryGoal], Gestalt);
				Goals[SecondaryGoal] = CandidateGoal; 
				if(&Goals[SecondaryGoal] == &Goals[0]) Gestalt->InGoal0 = true;	else Gestalt->InGoal1 = true;
				WhatToReturn = EGoalAcceptResult::ReplaceSecond; //◘◘>

				//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
				//остальные MyrLogicEvent вызываются в PostPerceive, но это событие видно лишь отсюда
				ME()->CatchMyrLogicEvent(EMyrLogicEvent::ObjDroppedOtherForYou, 1.0f, CandidateGoal.Object);//##>

				//угнездить воспринятый объект теперь уже не во временной, а в постоянной ячейке цели
				PostPerceive(Goals[SecondaryGoal], HowSensed, Strength, Gestalt, WhatToReturn);
			}
			//отбросить цель - локальная переменная CandidateGoal погибнет напрасно, PostPerceive не вызывается
			else WhatToReturn = EGoalAcceptResult::Discard; //◘◘>
		}
	}
	return WhatToReturn;
}

//==============================================================================================================
//забыть из операционной памяти, пересохранив в вдолговременную память
//==============================================================================================================
void AMyrAI::Forget(FGoal& Goal, FGestaltRelation* Gestalt)
{
	//изменяем глубинное отношение к цели по текущей эмоции настолько, насколько уверенны, что узнали цель
	Gestalt->SaveEmotionToLongTermMem( Goal.EventMemory );

	//убрать флаг "никогда раньше" потому что теперь гешатльт считается изменён живывм опытом
	Gestalt->NeverBefore = 0;

	//удалить из ячейки упоминание о гештальте
	if (&Goal == &Goals[0]) Gestalt->InGoal0 = 0; else Gestalt->InGoal1 = 0;
	UE_LOG(LogMyrAI, Warning, TEXT(" - AI %s forgets %s"), *me()->GetName(), *Goal.Object->GetName());
	Goal = FGoal();
}

//==============================================================================================================
// внести образ в ячейку цели или поторомшить имеющуюся цель сенсорным сигналом
// вызывается только в акте перцепции, когда определена ячейка FGoal (даже если она временная и ненастоящяя)
//==============================================================================================================
void AMyrAI::PraePerceive (
	FGoal& Goal,					// оперативная цель, объект которой был вновь замечен (или вновь создаваемая для этого объекта)
	EHowSensed HowSensed,			// подробный тип стимула
	float Strength,					// сила стимула - изнутри ИИ системы
	UPrimitiveComponent* Obj,		// объект-ключ, с которым свящана эта цель внимания
	FGestaltRelation* NeuGestalt)	// образ в долговременной памяти с архетипами
{
	//замеченный образ впервые заливается в пустую ячейку цели внимания
	if(!Goal.Object)
	{
		//занять пустую ячейку цели
		Goal.Object = Obj;

		// предварительное значение слышимости (хз зачем здесь)
		Goal.Audibility = FMath::Min (Strength, 0.1f);	

		//застолбить в гешатльте, в какую ячейку мы поместили цель
		//осторожно! эта функция может быть вызвана для кандидата на цели, и тогда адрес будет левым, поэтому 2 проверки
		if(&Goal == &Goals[0]) NeuGestalt->InGoal0 = true; else
		if(&Goal == &Goals[1]) NeuGestalt->InGoal1 = true;
	}
}


//==============================================================================================================
// среагировать на перцепцию (нового или очередного) объекта
// вызывается только в событиях перцепции новых объектов, после выполнения минимальной части Measure.Goal
// здесь же происходит обработка слуха о специальных событиях (атака, смерть другого существа)
//==============================================================================================================
void AMyrAI::PostPerceive(FGoal& Goal, EHowSensed HowSensed, float Strength, FGestaltRelation* Gestalt, EGoalAcceptResult Result)
{
	//разбор служебных сообщений, посланных через систему слуха
	switch (HowSensed)
	{
		//стимул звука - обновить уровень слышимости цели (для этого расстояния уже должны быть посчитаны)
		case EHowSensed::HEARD:
			RecalcGoalAudibility(Goal, Strength, Gestalt);
			break;

		//услышанный зверь (уже измеренный и упакованный в Goal) кого-то атаковал, может, нас
		case EHowSensed::ATTACKSTRIKE:
		case EHowSensed::ATTACKSTART:
		{	auto Res = BewareAttack(Goal.Object, &Goal, Gestalt, HowSensed == EHowSensed::ATTACKSTRIKE);
			RecalcGoalAudibility(Goal, Strength, Gestalt);
			UE_LOG(LogTemp, Error, TEXT("%s BewareAttack(%s) = %s"), *me()->GetName(),
				*TXTENUM(EHowSensed, HowSensed), *TXTENUM(EAttackEscapeResult, Res));
			break;
		}

		//принят сигнал о смерти этой цели (но сама цель всё ещё валяется трупом)
		//сброс уверенности в узнавании, чтобы впитать в себя архетипическое отношения к мертвым данного класса
		case EHowSensed::DIED:
			Goal.Sure() = 0.0f;
			break;

		//принятие экспрессивного самодействия, типа клича угрозы, тут пока неясно, как
		case EHowSensed::EXPRESSED:

			//напрямую вписать в память событие впечатленности самодействием, напрямую взяв из самодействия данные для него
			if (me()->DoesSelfAction())
				ME()->CatchMyrLogicEvent(EMyrLogicEvent::MePatient_AffectByExpression, 1.0, Goal.Object, &me()->GetSelfAction()->EmotionalInducing);
			else UE_LOG(LogTemp, Error, TEXT("%s WTF Expressed, but no self action"), *me()->GetName());


			//так как это типа слух, пометить что мы услышали и увеличить / вернуть слышимость
			RecalcGoalAudibility(Goal, Strength, Gestalt);
			break;

		//зов неживого аттрактора: сначала как слух для накрутки веса
		case EHowSensed::CALL_OF_HOME:
			RecalcGoalAudibility(Goal, 3.0f, Gestalt);
			break;
	}

	//обнуление счётчика времени без событий - ведь произошло событие
	//неясно, нужно ли это теперь, когда эмоции считаются другим образом
	Goal.TicksNoEvents = 0;

	//первое восприятие этого существа вообще в жизни - эмоции ориентируются на узнавание класса существ
	if (Gestalt->NeverBefore)
	{	
		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
		ME()->CatchMyrLogicEvent(EMyrLogicEvent::ObjNoticedFirstTime, 1.0f, Goal.Object);
		Goal.EventMemory.Emotion = me()->ClassRelationToClass(Goal.Object->GetOwner());
		Gestalt->NeverBefore = false;
	}

}




//==============================================================================================================
//найти/перерассчитать расстояние, направление, а из них - слышимость, видимость, уверенность, абсолютный вес
//самая тяжёлая функция, вызывается в медленном тике, не в событиях слышки (которые могут прилетать с любой частотой)
//==============================================================================================================
void AMyrAI::MeasureGoal (FGoal& Goal,	//ячейка цели
	FGestaltRelation* Gestalt,			//внецелевой образ
	float DeltaTime,					//время для синхронизации с реальным временем
	bool Sole,							//признак того, что цель одна и вес пересчитывать не надо
	EHowSensed HowSensed,				//событие, приведшее к замечанию цели - для ветвления/оптимизации
	FHitResult* OutVisTrace)			//сборка результатов трассировки видисости - нужно дальше, для поиска пути, когда будет известно, на цель или от цели
{
	//если введенная цель не соответствует адресам жестких ячеек - это режим оценки новой цели и текущий Goal - временная переменная
	bool TestNewGoal = (&Goal != &Goals[0])&&(&Goal != &Goals[1]);

	//долгопамятный образ
	if(!Gestalt) Gestalt = Remember(Goal.Object);

	/////////////////////////////////////////////////////////////////////////////////
	//предварительно рассчитать направление на цель и расстояние до цели
	//подготовить переменные: квадрат расстояния и цель как существо
	float d2 = 1000; AMyrPhyCreature* GoalMyr = Cast<AMyrPhyCreature>(Goal.Object->GetOwner());
	Goal.Relativity = EYouAndMe::Unrelated;
	Goal.RouteResult = ERouteResult::Towards_Directly;
	FHitResult VisHit(ForceInit);
	
	//если цель - сущство, имеется шанс более подробно
	if(GoalMyr)
	{
		//глядим на произвольно выбранную часть тела
		//это пока предварительный вектор, ненормированный
		VisHit.ImpactPoint = GoalMyr->GetVisEndPoint();

		//предварительный полный радиус-вектор, чтоб прикинуть, нужны ли более сложные вычисления
		FVector3f TempLookAtDir = (FVector3f)(VisHit.ImpactPoint - me()->GetHeadLocation());

		//также предварительно ракурс и расстояние
		Goal.LookAlign = TempLookAtDir | me()->GetLookingVector();
		d2 = TempLookAtDir.SizeSquared();

		//если соперник перед лицом (а не за задом) и если соперник на расстоянии меньшем трёх наших длин тел
		if (Goal.LookAlign > 0 && d2 < FMath::Square(2 * me()->GetBodyLength()))
		{
			//значит он предельно близко, и нужно вычислять расстояния до ближайшего членика его тела
			VisHit.ImpactPoint = GoalMyr->GetClosestBodyLocation(me()->GetHeadLocation(), VisHit.ImpactPoint, d2);

			//на таком малом расстоянии перевычисление ближайших точек приводит к резкому дерганию головой
			//чтобы его сгладить, берется предыдущий полный радиус вектор
			Goal.LookAtDir = FMath::Lerp(
				Goal.LookAtDir*Goal.LookAtDist,
				(FVector3f)(VisHit.ImpactPoint - me()->GetHeadLocation()),
				0.3f);

			//перерасчёт предвариительного суждение спереди/сзади
			Goal.LookAlign = Goal.LookAtDir | me()->GetLookingVector();

			//на таком близком расстоянии пора бы проверить наличие непосредственного контакта
			if (me()->IsTouchingThisComponent(Goal.Object))
				Goal.Relativity = EYouAndMe::Touching;
		}

		//далеко от цели, предудщее значение не нужно, можно его переписать
		else Goal.LookAtDir = TempLookAtDir;
	}
	//цель - произвольный объект, 
	else
	{
		//смотрим в центр координат габаритов компонента
		VisHit.ImpactPoint = Goal.Object->Bounds.GetSphere().Center;
		Goal.LookAtDir = (FVector3f)(VisHit.ImpactPoint - me()->GetHeadLocation());

		//расположение цели по отношению к нашему полю зрения
		//если цель за затылком, нет смысла вычислять точно
		//ВНИМАНИЕ, это предвариетнльное ненормированное значение
		Goal.LookAlign = Goal.LookAtDir | me()->GetLookingVector();

	}
	
	//расстояние по прямой до объекта - временный флаг, чтобы распознать случай заполнения
	Goal.LookAtDist = -1;

	/////////////////////////////////////////////////////////////////////////////////
	//уточнить расположение цели трассировкой - типа зрение, видимость, что застит
	//сюда сохранится увиденное препятствие

	//трассировку (зрение) делать только если объект впереди нас
	//и если не касается нашего тела
	if(Goal.LookAlign > 0 && Goal.Relativity != EYouAndMe::Touching)
	{
		//если в результате трассировки объект виден (нет загораживателей)
		if(TraceForVisibility(Goal.Object, Gestalt, (FVector)Goal.LookAtDir, &VisHit))
		{	
			//взять расстояние уже посчитанное из трассировки
			if(VisHit.Distance>0)
			{	Goal.LookAtDist = VisHit.Distance;
				Goal.LookAtDistInv = 1/VisHit.Distance;
				d2 = Goal.LookAtDist * Goal.LookAtDist;//для теста
			}
		}
	}

	//если не произошло заполнения расстояния через трассировку, расчёт классический
	if(Goal.LookAtDist<0)
	{	d2 = Goal.LookAtDir.SizeSquared();
		Goal.LookAtDistInv = FMath::InvSqrt(d2);
		Goal.LookAtDist = FMath::Sqrt(d2);
	}
	//нормализация через умножение на обратную норму
	Goal.LookAtDir *= Goal.LookAtDistInv;

	//здесь мы брали скалярное произведение одного единичного и одного длинного, теперь также надо избавиться от нормы
	Goal.LookAlign *= Goal.LookAtDistInv;

	//отладочная информация
	me()->Line(ELimbDebug::AILookDir, me()->GetHeadLocation(), (FVector)Goal.LookAtDir * Goal.LookAtDist, FColor(Goal.Visibility * 255, Gestalt->NowSeen ? 255 : 0, 255, 255), 1, 0.5);

/*#if DEBUG_LINE_AI
	UE_LOG(LogMyrAI, Log, TEXT("AI %s MeasureGoal %s, dist %g %s"), *me()->GetName(), *Goal.Object->GetOwner()->GetName(), FMath::Sqrt(d2), *VisHit.Location.ToString());
	if(!me()->IsUnderDaemon() && HowSensed == EHowSensed::ROUTINE)
		DrawDebugLine(GetWorld(), me()->GetHeadLocation(), me()->GetHeadLocation() + Goal.LookAtDir * Goal.LookAtDist,
			FColor(Goal.Visibility*255, Gestalt->NowSeen ? 255 : 0, 255, 255), false, 0.5, 100, 1);
#endif*/


	/////////////////////////////////////////////////////////////////////////////////
	//рассчитать уровни видимости, слышимости, уверенности, что опознали цель
	//полное сложное обновление, ибо события перцепции - только когда открывается и скрывается из виду
	RecalcGoalVisibility (Goal, GoalMyr, Gestalt);

	//насколько мы слышим цель - здесь, вне событий звука просто постепенно гасится
	Goal.Audibility = FMath::Max(0.0f,
		Goal.Audibility -								// предыдущее значение (далее - то, что вычитается)
			DeltaTime * 0.1f *							// чтобы звон в ушах слабел равномерно по времени
			(2.2f - Goal.Weight - Goal.Visibility));	// чем весомее цель, тем дольше верится, что звука от нее нет

	//Sure - память "я опознал тебя", для стелса, если меньше 1, использовать видимость/слышимость для уверения
	if (Goal.Sure() < 0.9f)
	{
		//скорость опознавания, связанная с тем, первый ли раз мы вообще видим эту цель, и нашим самочувствием
		//здесь пока неясно, смотреть все события или только последнее
		float AdvRecogMult = (Gestalt->NeverBefore) ? 0.1 : 0.3;
		if(Goal.EventMemory.Last() == EMyrLogicEvent::ObjDroppedOtherForYou) AdvRecogMult *= 2;
		AdvRecogMult *= me()->Health * me()->Stamina;
		
		//квадрат видимости, слышимости минус убытие, чтобы без стимулов (если хорошо затаиться) постепенно задаваться вопросом "а был ли мальчик?"
		float NewSure = Goal.Sure() + AdvRecogMult * ( FMath::Square(Goal.Visibility) * 0.7f + FMath::Square(Goal.Audibility) * 0.3f) - 0.01f;
		if (NewSure > 0.0f) NewSure = 0.0f;

		//применение новой степени уверенности и отлов события полной уверенности
		//вообще это неправильно, событие полного узнавание должно быть только один раз, а не после каждого склероза
		//но ради этого вводить лишний флаг в гештальт - пока неясно стоит ли
		if (NewSure >= 1.0f)
		{	
			//№№№№№№№№№№№№№№№№№№№№№№№№№
			ME()->CatchMyrLogicEvent(EMyrLogicEvent::ObjRecognizedFirstTime, 1.0f, Goal.Object);
			Gestalt->NeverBefore = 0;	//завершенный акт узнавания сбрасывает факт "первый раз встретил"
			Goal.Sure() = 1.0f;
		} else Goal.Sure() = NewSure;
	}
	//мы абсолютно уверенны, что знаем эту цель, но так сучно и со временем уверенность должна немного падать, типа склероз и вообще философия
	else if (Goal.Visibility + Goal.Audibility < 0.2 && Goal.TicksNoEvents>128) Goal.Sure() -= 0.01;
	

	//если мы тестируем новую цель, то сразу же определяем вес и уходим, не надо тут трассировки
	//может, даже стоит еще раньше сокращенно это все вычислить, или ввести более быструю функцию прикидку веса
	if(TestNewGoal)
	{	Goal.Weight = RecalcGoalWeight(Goal);
		return;//◘◘>
	}

	//кэширование веса на ближайший такт, чтоб не пересчитывать это нагромождение влияний
	//однако, если известно, что цель одна, избегать этих вычислений и сразу ставить единицу
	Goal.Weight = Sole ? 1.0f : RecalcGoalWeight(Goal);

	//влияние памяти о пережитом событии на эмоцию 
	MyrLogicTick(Goal.EventMemory, true);

	//если нужно выдать подробные результаты трассировки в пространстве (там содержатся объекты
	if (OutVisTrace) *OutVisTrace = VisHit;

	//счётчик скукоты, сбрасывается актами замечания, атаки
	if(Goal.TicksNoEvents < 255) Goal.TicksNoEvents++;

	//если скукота достигла предела, цель может быть уже не нужна
}

//==============================================================================================================
//получить (из кривых от расстояния до цели) аналоговые, биполярные прикидки для тяги (вызращается)
//также поцельно аккумулировать некоторые параметры, которые собираются для всего ИИ каждый такт
//==============================================================================================================
float AMyrAI::CalcAdditiveGain(FGoal& Goal, float& StealthAmount, float& FacingAmount)
{
	//аргумент для кривых - приведенная обратная дальность [0-1] = [бесконечность-касание]
	float DistFactor;

	//если мы касаемся противника - установить расстояние в единицу, чтобы не люфтило, ибо особый случай
	if (Goal.Relativity == EYouAndMe::Touching) DistFactor = 1.0;
	else
	{
		//проеобразовать цель в существо, если это возможно
		auto MyrGoal = Cast<AMyrPhyCreature>(Goal.Object->GetOwner());

		//если объект цели - другое существо
		if (MyrGoal)
		{
			//домножить на средний размер меня и вас, чтобы точка касания примерно соответствовала единице, а не, например, 1/30см
			DistFactor = (me()->GetBodyLength() + MyrGoal->GetBodyLength()) * 0.5;

			//подсчёт общей заметности уже НАС для цели, которая могут быть агрессивными или которые нас боятся
			//реально существо не может точно знать, что его видят, оно само должно хорошо видеть и/или слышать тех, чью зоркость оценивает
			//к тому же существо должно быть уверенно, что оно отслеживает правильных оппонентов, для этого пока служит множитель Sure
			if (Goal.EventMemory.GetRage() > 0.1f || Goal.EventMemory.GetFear() > 0.1f)
				Paranoia += HowClearlyYouPerceiveMe(MyrGoal) * Goal.Sure();
		}
		else DistFactor = me()->GetBodyLength();

		//собственно обратное расстояние, чтобы бесконечность читалась конечной точкой 
		DistFactor *= Goal.LookAtDistInv;

	}

	//цвет эмоции домножается на уверенность, чтобы действовал стелс
	// пока враг не уверен, что его преследуем мы, к которому он плохо относится, он не делает резких определенных движений
	// возможно, следует помещать уверенность в альфа-канал, чтобы поведение при неизвестном объекте было разным
	auto myEmotion = Goal.EventMemory.Emotion * Goal.Sure();

	//▄ получаем вектор аналоговых тяг для пограничных эмоций (положительный или отрицательный)
	//* это отдельный параметр для цели, выбор движения осуществляется на его основе дискретным образом
	const auto GainsForDist = me()->GetGenePool()->MotionGainForEmotions->GetUnadjustedLinearColorValue(DistFactor);
	float RealGain = DotProduct(GainsForDist, myEmotion);

	//▄ получаем условный аналоговый коэффициент желания быть незаметным
	//* это аддитивный параметр, который аккумулируется по всем целям
	const auto StealthForDist = me()->GetGenePool()->StealthGainForEmotions->GetUnadjustedLinearColorValue(DistFactor);
	StealthAmount += DotProduct(StealthForDist, myEmotion) * Goal.Weight;

	//▄ получаем условный аналоговый коэффициент надобности поворота к цели определенной частью тела
	//* это аддитивный параметр, который аккумулируется по всем целям
	const auto FacingForDist = me()->GetGenePool()->StealthGainForEmotions->GetUnadjustedLinearColorValue(DistFactor);
	FacingAmount += DotProduct(FacingForDist, myEmotion) * Goal.Weight;

	return RealGain * Goal.Weight;
}

//==============================================================================================================
//сложная трассировка (цель получает значение MoveToDir и, возможно, ClosestPoint)
//==============================================================================================================
ERouteResult AMyrAI::Route(FGoal& MainGoal, FGestaltRelation* Gestalt, FHitResult* WhatWeSaw)
{
	//если по чувствам нас скорее тянет в сторону цели
	if(Drive.Gain > 0)
	{
		//трассировка до цели обнаружила препятствие
		if(WhatWeSaw->bBlockingHit)
		{
			//если препятствие - сам объект цели, то есть мы его успешно увидели
			if(MainGoal.Object == WhatWeSaw->Component.Get())
			{
				//простейший случай - направить бег напрямую к видимой цели
				MainGoal.MoveToDir = MainGoal.LookAtDir;
				return ERouteResult::Towards_Directly;//◘◘>
			}

			//если препятствие - иной объект, который загораживает цель
			else return DecideOnObstacle (MainGoal.LookAtDir, MainGoal, Gestalt, WhatWeSaw, MainGoal.MoveToDir);//◘◘>

		} 
		//луч трассировки не дотянулся ни до цеди, ни до препятствия - вероятно, когда трассировка даже не проводилась
		else
		{
			//идём вперед напрямую
			MainGoal.MoveToDir = MainGoal.LookAtDir;//◘◘>
			return ERouteResult::Towards_Directly;
		}
	} 

	//если нас тянет прочь от цели, а цель при это почему-то видима
	else
	{
		//для начала просто переворачивает векторы, делаем тягу положительной
		MainGoal.MoveToDir = -MainGoal.LookAtDir;
		Drive.Gain = -Drive.Gain;

		//пытаемся посмотреть назад от цели (вопрос пока на какой радиус вдаль рассчитывать)
		FHitResult Hit2; FVector3f LookBckRadius = -MainGoal.LookAtDir * 100;
		TraceForVisibility(MainGoal.Object, Gestalt, (FVector)LookBckRadius, &Hit2);

		//если нашли препятствие, обходим
		if(Hit2.bBlockingHit)
			 return DecideOnObstacle(-MainGoal.LookAtDir, MainGoal, Gestalt, &Hit2, MainGoal.MoveToDir);//◘◘>
		else
			return ERouteResult::Away_Directly;//◘◘>
	}
}

//==============================================================================================================
//сложный алгоритм, что делать, если точно изместно, что перед целью препятствие
//==============================================================================================================
ERouteResult AMyrAI::DecideOnObstacle(FVector3f LookDir, FGoal& Goal, FGestaltRelation* Gestalt, FHitResult* Hit, FVector3f& OutMoveDir)
{
	//если направление на объект достаточно пологое, то есть не надо лезть или взлетать
	if(LookDir.Z < 0.5f)
	{
		//просто обойти препятствие по большой дуге
		return SimpleWalkAroundObstacle (LookDir, Hit, OutMoveDir);//◘◘>
	}
	//цель находится высоко
	//особый режим, тут как-то сложно надо ориентироваться - или запрыгивать, или залезать, или считать недоступным
	else
	{
		//если точно известно, что наша цель - существо
		if(auto Myr = Cast<AMyrPhyCreature>(Goal.Object->GetOwner()))
		{
			//если это существо именно что сидит на объекте выше нас
			if(Myr->IsStandingOnThisActor(Hit->Component->GetOwner()))
			{
				//пока пробуем подойти к предмету, а вообще тут надо выбирать, каким боком объодить, как прыгнуть
				OutMoveDir = (FVector3f)(Hit->Component->GetOwner()->GetActorLocation() - me()->GetHeadLocation());
				OutMoveDir.Normalize();
				return ERouteResult::Towards_Base;//◘◘>
			}
			//если существо не касается препятствия
			else
			{
				//просто обойти препятствие по большой дуге
				return SimpleWalkAroundObstacle (LookDir, Hit, OutMoveDir);//◘◘>
			}
		}
		
		//если не существо потеря надежды достигнуть цели при ее физической видимости
		else MyrLogicChangeReachable(Goal, Gestalt, false);
		OutMoveDir = -LookDir;
		return ERouteResult::GiveUp_For_Unreachable;//◘◘>
	}
}

//==============================================================================================================
//обойти препятствие, вычислив его размеры
//==============================================================================================================
ERouteResult AMyrAI::SimpleWalkAroundObstacle(FVector3f LookDir, FHitResult* Hit, FVector3f& OutMoveDir)
{
	//примерная толщина компонента, который надо бы обойти
	float Thickness = Hit->Component.Get()->Bounds.BoxExtent.X + Hit->Component.Get()->Bounds.BoxExtent.Y;

	//оценить высоту препятствия (самая высокая точка в мировых координатах)
	float Altitude = Hit->Component.Get()->Bounds.Origin.Z + Hit->Component.Get()->Bounds.BoxExtent.Z;

	//вектор обхода в сторону - по идее всё равно, идём мы к цели или от цели
	FVector3f RadicalByPass = LookDir^me()->GetUpVector();

	//попробовать загодя обойти объект, который мешается
	FVector3f ClosestPoint = Hit->Location + RadicalByPass * Thickness;
	FVector3f NewDir = ClosestPoint - (FVector3f)me()->GetHeadLocation();
	NewDir.Normalize();
	UE_LOG(LogTemp, Log, TEXT("AI %s SimpleWalkAroundObstacle %s"), *me()->GetName(), *Hit->Component->GetOwner()->GetName());
	return ERouteResult::Simple_Walkaround;
}



//==============================================================================================================
//раздобыть данные по конкретному эмоуиональному событию
//==============================================================================================================
FMyrLogicEventData* AMyrAI::MyrLogicGetData(EMyrLogicEvent What)
{
	if (!me()->GetGenePool()->MyrLogicReactions) return nullptr;
	return me()->GetGenePool()->MyrLogicReactions->MyrLogicEvents.Find(What);
}

//==============================================================================================================
//новая функция по добавлению эмоционального стимула 
//==============================================================================================================
void AMyrAI::AddEmotionStimulus(FEmoStimulEntry New)
{
}

//==============================================================================================================
//зарегистрировать событие, влияющее на жизнь существа - в рамках ИИ
//==============================================================================================================
void AMyrAI::RecordMyrLogicEvent(EMyrLogicEvent Event, float Mult, UPrimitiveComponent* Obj, FMyrLogicEventData* EventInfo)
{
	//если событие относится к объекту-цели (не только к нам самим)
	//то записать его в отдельную память внутри модели этой цели, если, конечно, модель цели для данного объекта присутствует
	if (Obj)
	{	FGoal* GoalWithThisObj = FindAmongGoals(Obj);
		if (GoalWithThisObj)
			GoalWithThisObj->EventMemory.AddEvent (Event, EventInfo, Mult, true);
	}

	//вне зависимости от наличиея активной цели то же самое событие вписать и в общую память "о себе"
	//оно может каким-то образом влиять на общее самоощущение
	EventMemory.AddEvent (Event, EventInfo, Mult, false);
}

//==============================================================================================================
//уяснить для себя и запомнить событие, когда цель становится недоступной или наоборот доступной
//==============================================================================================================
void AMyrAI::MyrLogicChangeReachable(FGoal& Goal, FGestaltRelation* Gestalt, bool Reachable)
{
	if(Reachable)
	{
		//отсечь спам событий, когда он уже признан доступным
		if(!Gestalt->Unreachable) return;
		auto EventInfo = MyrLogicGetData(EMyrLogicEvent::ObjNowUnreachable);
		if (EventInfo) RecordMyrLogicEvent(EMyrLogicEvent::ObjNowReachable, 0, Goal.Object, EventInfo);
		Gestalt->Unreachable = false;
	}
	else
	{	//отсечь спам событий, когда он уже признан недоступным
		if(Gestalt->Unreachable) return;
		auto EventInfo = MyrLogicGetData(EMyrLogicEvent::ObjNowUnreachable);
		if (EventInfo) RecordMyrLogicEvent(EMyrLogicEvent::ObjNowUnreachable, 0, Goal.Object, EventInfo);
		Gestalt->Unreachable = true;
	}
}

//==============================================================================================================
//рутинный процесс варения данной памяти эмоциональных откликов
//==============================================================================================================
void AMyrAI::MyrLogicTick(FEmotionMemory& AnEventMemory, bool ForGoal)
{
	//внутри лерпить шлейф отголоска последней эмоции
	//когда шлейф кончился... автоперевложить новую эмоцию
	if (!AnEventMemory.Tick(false))
	{
		//по умолчанию исчерпание эмоциональных влияний = скука
		auto autoEv = EMyrLogicEvent::SelfStableAndBoring;
		float Amplitude = 1.0f;
		switch (AnEventMemory.Last())
		{
			//скука длится давно - привыкание
			case EMyrLogicEvent::SelfStableAndBoring:
				Amplitude = AnEventMemory.LastMultReal() * 0.8;
				break;

			//осознание, что нами манипулировали - генерация особого события
			case EMyrLogicEvent::MePatient_AffectByExpression:
				autoEv = EMyrLogicEvent::MePatient_GuessDeceivedByExpression;
				break;
		}
		//применить автозаполнитель (пока это либо скука, либо аналитическое прозрение обманутости, но мог бы быть более сложный самоанализ
		EventMemory.AddEvent(autoEv, MyrLogicGetData(autoEv), Amplitude, ForGoal);
	}
}

//для более простой комбинаторики при выставлении события
EMyrLogicEvent operator+(const EMyrLogicEvent& o1, const EMyrLogicEvent& o2) { return (EMyrLogicEvent)((uint8)o1 + (uint8)o2); }
EMyrLogicEvent operator+(const EMyrLogicEvent& o1, const int& o2) { return (EMyrLogicEvent)((uint8)o1 + o2); }

//==============================================================================================================
//проанализировать соотношение нас и агрессора и выбрать подходящий эмоциональный отклик и геймлпейное событие
//надо как-то автоматически определять обратный актант
//==============================================================================================================
void AMyrAI::DesideHowToReactOnAggression(float Amount, AMyrPhyCreature* Motherfucker)
{

	// найти противника в памяти
	auto SeinGestaltInMir = Memory.Find(Motherfucker->GetMesh());
	
	//агрессия свершается первый раз или нет = почти все эвенты имеют пары отстоящие на 1 (первый не первый)
	bool FirstTime = false;
	if(!SeinGestaltInMir->ItDamagedUs)
		SeinGestaltInMir->ItDamagedUs = FirstTime = true;

	//по умолчанию считаем, что нас задели случайно (презумпция невиновности)
	EMyrLogicEvent ExactEventForPatient = EMyrLogicEvent::MePatient_CasualHurt + (FirstTime?1:0);//▄
	EMyrLogicEvent ExactEventForAgent = EMyrLogicEvent::MeAgent_CasualHurt + (FirstTime?1:0);//▄

	//если агрессор сам среди наших целей, то есть не неожиданность
	auto Goal = FindAmongGoals(Motherfucker->GetMesh());

	//для краткости - эмоция, которую мы фактически сейчас испытываем к агрессору
	auto Feel4You = Goal ? Goal->EventMemory.Emotion : FEmotion::Peace();

	//однако если точно известно, что объект нас атакует
	if (Motherfucker->Attacking() && Motherfucker->MyrAI()->Goal_1().Object == me()->GetMesh())
	{
		//по умолчанию = простой намеренный урон
		ExactEventForPatient = EMyrLogicEvent::MePatient_PlannedHurt + (FirstTime?1:0);//▄

		//если объект нам как друг
		if (Feel4You.Love() > 0.8 &&
			Feel4You.Rage() < 0.2 &&
			Feel4You.Fear() < 0.05)
			ExactEventForPatient = EMyrLogicEvent::MePatient_FirstPlannedHurtByFriend + (FirstTime?1:0);//▄
			
		//если нас наказывает хозяин (внимание, если уровень злобы на него перейдет порог, то хрен он, а не хозяин)
		if(SeinGestaltInMir->Authority &&
			Feel4You.Love() > 0.1 &&
			Feel4You.Rage() < 0.9 &&
			Feel4You.Fear() < 0.95)
			ExactEventForPatient = EMyrLogicEvent::MePatient_FirstPlannedHurtByAuthority + (FirstTime?1:0);//▄
			
		//наоборот мы хозяин, и нас почему-то бьют наши подчиненные
		if(auto MeinGestaltInDir = Motherfucker->MyrAI()->Memory.Find(me()->GetMesh()))
			if(MeinGestaltInDir->Authority)
				ExactEventForPatient = EMyrLogicEvent::MePatient_FirstPlannedHurtTowardsAuthority + (FirstTime?1:0);//▄

		//если атакующий очень маленький или атакующий очень большой (здесь пара первый не первый не введена ибо задолбало)
		if(me()->GetMesh()->Bounds.SphereRadius > Motherfucker->GetMesh()->Bounds.SphereRadius * 2.5)
			ExactEventForPatient = EMyrLogicEvent::MePatient_PlannedHurtByLittlePoor;//▄
		if(me()->GetMesh()->Bounds.SphereRadius < Motherfucker->GetMesh()->Bounds.SphereRadius / 2.5)
			ExactEventForPatient = EMyrLogicEvent::MePatient_PlannedHurtByBigFearful;//▄

		//если мы и так еле дышим, а он всё равно смертельно атакует
		if(me()->Health < 0.3 && Amount > 0.3)
			ExactEventForPatient = EMyrLogicEvent::MePatient_PlannedHurtTillMyDeath;//▄
		
	}
	//по всм признакам случайно задел нас - тут меньше опций
	else
	{
		//случайно задел
		ExactEventForPatient = EMyrLogicEvent::MePatient_FirstCasualHurt + (FirstTime?1:0);//▄

		//если на нас случайно поднимает руку друг
		if (Feel4You.Love() > 0.8 &&
			Feel4You.Rage() < 0.2 &&
			Feel4You.Fear() < 0.05)
			ExactEventForPatient = EMyrLogicEvent::MePatient_CasualHurtByFriend + (FirstTime?1:0);//▄

		//наоборот мы хозяин, и нас почему-то бьют наши подчиненные
		if(auto MeinGestaltInDir = Motherfucker->MyrAI()->Memory.Find(me()->GetMesh()))
			if(MeinGestaltInDir->Authority)
				ExactEventForPatient = EMyrLogicEvent::MePatient_CasualHurtTowardsAuthority + (FirstTime?1:0);//▄

	}

	//исходя из стого, что столбцы констрант объявлены одинаково/последовательно, инвертировать актант можно через арифметику
	ExactEventForAgent = EMyrLogicEvent((int)ExactEventForPatient + ((int)EMyrLogicEvent::MeAgent_CasualHurt - (int)EMyrLogicEvent::MePatient_CasualHurt));

	//вызвать, наконец, функцию регистрации отысканного события
	ME()->			CatchMyrLogicEvent (ExactEventForPatient,	Amount, Motherfucker -> GetMesh());
	Motherfucker->	CatchMyrLogicEvent (ExactEventForAgent,		Amount, me()         -> GetMesh());

	//вынести факт повреждения нас на уровень всего слышимого сообщества, чтобы, например, друзья заступились
	if(Amount > 0.2) LetOthersNotice (EHowSensed::PERCUTED, Amount);
}

//==============================================================================================================
//проанализировать наши отношения и решить как реагировать на ласку этого существа
//==============================================================================================================
void AMyrAI::DesideHowToReactOnGrace(float Amount, AMyrPhyCreature* Sweetheart)
{
	//по умолчанию считаем, что нас задели случайно (презумпция невиновности)
	EMyrLogicEvent ExactEventForPatient = EMyrLogicEvent::MePatient_CasualGrace;//▄
	EMyrLogicEvent ExactEventForAgent = EMyrLogicEvent::MeAgent_CasualGrace;//▄

	// найти противника в памяти
	auto SeinGestaltInMir = Memory.Find(Sweetheart->GetMesh());

	//однако если точно известно, что объект нас атакует
	if (Sweetheart->Attacking() && Sweetheart->MyrAI()->Goal_1().Object == me()->GetMesh())
	{
		//по умолчанию
		ExactEventForPatient = EMyrLogicEvent::MePatient_PlannedGrace;//▄

		//если на нас намеренно ласкает враг
		auto Goal = FindAmongGoals(Sweetheart->GetMesh());
		if (Goal) if (Goal->EventMemory.GetRage() > 0.8)
			ExactEventForPatient = EMyrLogicEvent::MePatient_PlannedGraceByEnemy;//▄

		//если нас наказывает хозяин
		if(SeinGestaltInMir->Authority)
			ExactEventForPatient = EMyrLogicEvent::MePatient_PlannedGraceByAuthority;//▄
	}
	else
	{
		//по умолчанию
		ExactEventForPatient = EMyrLogicEvent::MePatient_CasualGrace;//▄

		//если на нас случайно ласкает враг
		auto Goal = FindAmongGoals(Sweetheart->GetMesh());
		if (Goal) if (Goal->EventMemory.GetRage() > 0.8)
			ExactEventForPatient = EMyrLogicEvent::MePatient_CasualGraceByEnemy;//▄
	}

	//исходя из стого, что столбцы констрант объявлены одинаково/последовательно, инвертировать актант можно через арифметику
	ExactEventForAgent = EMyrLogicEvent((int)ExactEventForPatient + ((int)EMyrLogicEvent::MeAgent_CasualGrace - (int)EMyrLogicEvent::MePatient_CasualGrace));

	//вызвать, наконец, функцию регистрации отысканного события
	ME()->			CatchMyrLogicEvent (ExactEventForPatient, Amount, Sweetheart->GetMesh());
	Sweetheart->	CatchMyrLogicEvent (ExactEventForAgent, Amount, me()->GetMesh());
}



//==============================================================================================================
//реакция на чью-то атаку, "услышанную" в радиусе слуха
//==============================================================================================================
EAttackEscapeResult AMyrAI::BewareAttack (UPrimitiveComponent* Suspect, FGoal* SuspectAsMyGoal, FGestaltRelation* Gestalt, bool YouMadeStrike)
{
	//парировать атаки разрешено только с высоким ИИ
	if (AITooWeak()) return EAttackEscapeResult::NO_WILL; //◘◘>

	//вызвать событие "атакую" может только другое существо, так что доп. проверок не нужно
	auto You = (AMyrPhyCreature*)Suspect->GetOwner();

	//аномальная ситуация, когда атака сообщается, а атакующий почему-то не реагирует
	if (You->NoAttack()) return EAttackEscapeResult::NO_RIVAL; //◘◘>

	//атакующий измерен (эта функция вызывается после MeasureGoal) и запакован в GoalIfPresent
	if(SuspectAsMyGoal)
	{
		//сборка данных по всем аспектам атаки
		auto AInfo = You->GetAttackAction();
		auto& Attack = You->GetAttackActionVictim();

		//если атака априори наносит вред (это не атака-добродетель и не атака без контакта)
		if(Attack.TactileDamage > 0)
		{
			//степень попадения в поле действия атаки существа Suspect
			float RealDanger = Attack.IntegralAmount (SuspectAsMyGoal->LookAtDist, SuspectAsMyGoal->LookAlign);
			if(RealDanger > 0)
			{
				//если мы в этот момент (кого-либо) атакуем, мож даже контратакой, запущенной ранее в этой же функции
				if (me()->Attacking())
				{
					//текущая атака безнадёжна, поскольку не может парировать такой урон = завершаем ее, не тратим силы
					if(ME()->GetAttackAction()->MaxDamageWeCounterByThis < RealDanger)
						ME()->NewAttackEnd();
					
					//текущая атака небезнадёжна - может, запущена в предыдущий вызов этой функции
					//здесь не перепроверяется применимость, хз насколько это упущение
					else if (me()->PreStrike() && YouMadeStrike)
						return CounterStrike(RealDanger, You, SuspectAsMyGoal->LookAtDir);//◘◘>
				}

				//если едёт самодействие, оно или защитное (продолжить, у самодействий нет ударной фазы) или лишнее (отменить)
				//сюда бы еще ввести оценку оставшегося времени самодействия и если коротко, то всё равно отменять
				if(me()->DoesSelfAction())
				{	if(ME()->GetSelfAction()->MaxDamageWeCounterByThis < RealDanger)
						ME()->SelfActionCease();
					else return EAttackEscapeResult::WE_KEEP_GOOD_SELFACTION;//◘◘>
				}

				////////////////////////////////////////////////////////////////////////

				//по всем ваще атакам, самодеййствиям, релаксам
				for(int i=0; i<me()->GetGenePool()->Actions.Num(); i++)
				{
					//простая проверка по выдерживаемой силе, ищем в принципе годные контратаки
					auto A = me()->GetGenePool()->Actions[i];
					if(RealDanger < A->MaxDamageWeCounterByThis)
					{
						//если найдется атака, сюда номер жертвенной сборки, там варианты анимации и т.п.
						uint8 VictimType = 0;

						//комплексная проверка применимости действия с учётом субьъекта, объекта и нацеленности ИИ
						auto R = A->IsActionFitting (me(), Suspect, SuspectAsMyGoal, VictimType, true, true);

						//действие оказалось применимым по всем статьям
						if(ActOk(R))
						{
							//проверенное действие оказалось полноценной атакой
							if(A->IsAttack())
							{	
								//направить ее на обидчика и стартануть
								ME()->AttackDirection = SuspectAsMyGoal->LookAtDir;
								ME()->NewAttackStart(i, VictimType);

								//если начало атаки удалось
								if (me()->Attacking())
								{	
									//если противник уже бьёт, то тут же перейти самим в боевую фазу
									if(YouMadeStrike) return CounterStrike (RealDanger, You, SuspectAsMyGoal->LookAtDir);//◘◘>

									//иначе время еще есть, выйти из функции и ждать на изводе, когда он вдарит
									else
									{ 
										if(me()->CanFly())
										{
											//для летающих вспархивать при первой замашке
											Drive.DoThis = ECreatureAction::TOGGLE_SOAR;
											return EAttackEscapeResult::WE_GONNA_RUN;//◘◘>
										}
										//для нелетающих время еще есть
										else return EAttackEscapeResult::WE_STARTED_PARRY;//◘◘>
									}
								}
							}
							//проверенное действие оказалось самодействием
							else 
							{ ME()->SelfActionStart(i);
								return EAttackEscapeResult::WE_PARRIED_BY_SELFACTION;//◘◘>
							}
						}
					}
				}

				//последняя надежда (когда не нашли атаку или не смогли запустить) резко развернуться и убежать
				Drive.MoveDir = -SuspectAsMyGoal->LookAtDir;
				Drive.Release = false;
				Drive.Gain = 1.0f;
				if(me()->CanFly()) Drive.DoThis = ECreatureAction::TOGGLE_SOAR;
				else Drive.DoThis = ECreatureAction::TOGGLE_HIGHSPEED;
				return EAttackEscapeResult::WE_GONNA_RUN;//◘◘>

			}

			//ложная тревога, ничего не делать
			else
			{
				//летающие взлетают даже если им ничего не угрожает
				//вообще-то здесь должен быть настраиваемый коэффициент превышенной дальности
				if (SuspectAsMyGoal->LookAtDist < 10 * me()->GetBodyLength())
				{
					if (me()->CanFly())
					{	Drive.DoThis = ECreatureAction::TOGGLE_SOAR;
						Drive.Gain = 1.0f;
						return EAttackEscapeResult::WE_GONNA_RUN;//◘◘>
					}
					//для сильного страха также по ложной тревоге срываться на бег
					else if (Goal_1().EventMemory.GetFear() > 0.7)
					{	Drive.DoThis = ECreatureAction::TOGGLE_HIGHSPEED;
						Drive.Gain = 1.0f;
						return EAttackEscapeResult::WE_GONNA_RUN;//◘◘>
					}
					else return EAttackEscapeResult::NO_DANGER; //◘◘>
				}else return EAttackEscapeResult::NO_DANGER; //◘◘>
			}
		}
		//если атака априори приносит добро
		else
		{
			//пока ничего не сделать, а так надо плавно подвигаться к "атакующему"
			//но это стоит делать через эмоциональное отношение
			return EAttackEscapeResult::TOO_POSITIVE; //◘◘>
		}
	}
	//имеется объект, но на него не заполнена структура цели
	//неясно, как быть, перевычислять цель или вообще не допускать таких случаев
	//пока их и не может быть
	else{}
	
	return EAttackEscapeResult::NO_RIVAL; //◘◘>
}

//==============================================================================================================
//непосредственно активировать нашу ранее запущенную атаку в ответ на чужую атакуc
//==============================================================================================================
EAttackEscapeResult AMyrAI::CounterStrike(float RealDanger, AMyrPhyCreature* You, FVector3f DirToYou)
{
	//коэффициент силы атаки, пока действует только на силу прыжка
	//почему такой коэффициент, неясно, уж очень неуниверсально
	ME()->AttackForceAccum = 0.3 + RealDanger;
	
	//установить или обновить текущее направление атки
	ME()->AttackDirection = DirToYou;

	//запустить немедленный переход к активной фазе (сама фаза, может, наступит не сразу)
	auto R = ME()->NewAttackStrike ();
	UE_LOG(LogTemp, Warning, TEXT("AI %s CounterStrike AttackStrike %s, accum=%d"), *me()->GetName(), *TXTENUM(EAttackAttemptResult, R), ME()->AttackForceAccum);
	
	//если в том или ином виде удалось запустить активную фазу
	if(StrOk(R))
	{
		//отметить для агрессора и жертвы логическую и эмоциональную веху
		ME()->CatchMyrLogicEvent (EMyrLogicEvent::MePatient_HurtParried,	RealDanger,		You->GetMesh());
		You ->CatchMyrLogicEvent (EMyrLogicEvent::MeAgent_HurtParried,		RealDanger,		me()->GetMesh());
		return EAttackEscapeResult::WE_PARRIED;
	}
	else return EAttackEscapeResult::WE_GONNA_SUFFER;
}

//==============================================================================================================
// степень видимости цели, для стелса, нормирована, нельзя использовать как вес, многие цели одинаково видны
//==============================================================================================================
float AMyrAI::RecalcGoalVisibility(FGoal& Goal, AMyrPhyCreature* MyrAim, const FGestaltRelation* Gestalt)
{
	//если формально видно
	if (Gestalt->NowSeen)											
	{	
		//если цель - существо
		if(MyrAim)								
		{	
			//грань, ближе которой базис обратного расстояния равен 1 = полная видимость, а дальше - уже даже в упор не совсем видно
			float DistFullVision = 10.0f * (me()->GetBodyLength() + MyrAim->GetBodyLength());

			//расширение дальности полной видимости, если цель движется (для разнообразя берется то одна, то другая часть тела
			if (Rand0_1())	DistFullVision += MyrAim->GetThorax()->SpeedAlongFront() * 0.1f;
			else			DistFullVision += MyrAim->GetPelvis()->SpeedAlongFront() * 0.1f;

			//обратное расстояние, начиная с порога видимость даже при прямом незасящем обзоре падает
			Goal.Visibility = FMath::Min(1.0f, Goal.LookAtDistInv * DistFullVision);

			//добавить модификаторы стелса цели
			Goal.Visibility *=
				  MyrAim->GetBehaveCurrentData()->StealthPenalty			// снижение общей заметности за счёт особого режима поведения цели
				* MyrAim->GetCurrentSurfaceStealthFactor()					// коэффициент, модифицирующий заметность из-за свойств поверхности, на которой сидит цель
				* FMath::Clamp(0.5f + Goal.LookAlign, 0.0f, 1.0f);			// впереди полная видимость, сзади сильно падает независимо от расстояния и навыка стелса у цели
		}
		else															//если цель - произвольный объект
		{	Goal.Visibility = Goal.Weight * 0.1f									// абстрактный вес (посчитанный на прошлом этапе) влияет 
				+ FMath::Min(Goal.LookAtDistInv * me()->GetBodyLength() * 10, 1.0f)	// расстояние в 10 длин тела - это 100% ведимость
				* FMath::Min(1.0f + Goal.LookAlign, 1.0f);								// если цель находится сзади, видимость снижается из-за невозможности даже повернуться настолько			
		}
	}
	//если цель не видна - плавно рекуррентно снижать видимость, то есть веру/память об увиденном
	else Goal.Visibility = Goal.Visibility * 0.9;
	return Goal.Visibility;
}

//==============================================================================================================
// степень слышимости цели, для стелса, нормирована, нельзя использовать как вес, многие цели одинаково видны
// вызывается непосредственно в миг звука, в PostPerceive
//==============================================================================================================
float AMyrAI::RecalcGoalAudibility(FGoal& Goal, const float Strength, const FGestaltRelation* Gestalt)
{
	Goal.Audibility = FMath::Min(
		Goal.Audibility							// исходное значение
		+ Strength								// исходная сила стимула
		* Goal.LookAtDistInv					// мощное затухание с расстоянием (возможно, сделать линейным, ибо слишком резко)
		* ConfigHearing->HearingRange * 0.1f	// ближе 1/10 максимального радиуса слышимости расстояние почти не играет роли - и так хорошо слышно
		* (Gestalt->NowSeen ? 1.0f : 0.8f),		// если цель также видна, слышимость чутьбольше, ибо не перекрыто стенками
		1.0f);									// ограничение на диапазон (делать оглушение пока сложно)

	return Goal.Audibility;
}

//==============================================================================================================
// рассчитать степень важности цели (как новой, так и текущей), не нормирована, только для сравнений
//==============================================================================================================
float AMyrAI::RecalcGoalWeight(const FGoal& Goal) const
{
	return 0.001f	// константа, чтобы не получить в знаменателе ноль

		// взвешенная сила эмоции к оппоненту, максимальная эмоция - страх
		+ Goal.EventMemory.GetAlertPower()

		// коэффициент актуальности силы эмоций его к нам
		* (0.7f + 0.3f * WhatYouFeelOrRememberAboutMe (Goal.Object).Power()) 

		//насколько мы лицом к цели (вообще это входит в видимость/слышимость, так что нжно ли,)
		* (0.6f + 0.4f*Goal.LookAlign)
		
		// уровень видимости и слышимости (видимость более весома)
		* ( 0.6f * Goal.Visibility + 0.4f * Goal.Audibility)	

		// если цель атакует, неважно кого, она привлекает внимание
		* (me()->NoAttack() ? 1.0f : 2.0f);		
}

//==============================================================================================================
//рассчитать интегральную эмоцию всего существа
//==============================================================================================================
void AMyrAI::RecalcIntegralEmotion()
{
	//сначала берётся эмоция по отношению к себе самому
	FEmotion Compos = EventMemory.Emotion;

	//затем, если есть цели, к композиции прибавляется каждая эмоция по отношению к цели, но ОСЛАБЕННАЯ неким коэффициентом общей чувственной заметности цели
	//этот манер смеси больше нигде не используется, множители видимости и слышимости нестандартны, обычно видимость вносит больший вклад в уровень осознанности 
	//но здесь не сознание, а эмоции, к тому же слышимость более плавно убывает, так что она чуть ли не главнее видимости
	if(Goal_1().Object)
	{	float Coef = (0.5 * Goal_1().Visibility + 0.5 * Goal_1().Audibility);
		Compos = Compos + (Goal_1().EventMemory.Emotion * Coef);
	}
	if(Goal_2().Object)
	{	float Coef = (0.5 * Goal_2().Visibility + 0.5 * Goal_2().Audibility);
		Compos = Compos + (Goal_2().EventMemory.Emotion * Coef);
	}
	
	//перевод в удобный формат для воспроизведения анимации (типа Lab-цвета, но пока свой)
	IntegralEmotionPower = Compos.Normalize();	//общая сила эмоций, степень неспокойствия, смесь спокойной стойки и неспокойной композиции стоек
	IntegralEmotionRage = Compos.Rage();
	IntegralEmotionFear = Compos.Fear();
}

FLinearColor AMyrAI::GetIntegralEmotion() const
{	return FEmotion(IntegralEmotionRage, FMath::Sqrt(IntegralEmotionPower*IntegralEmotionPower - IntegralEmotionFear*IntegralEmotionFear - IntegralEmotionRage*IntegralEmotionRage), IntegralEmotionFear);
}



//==============================================================================================================
//рассчитать микс из воздействий звух управляющих существом сущностей
//здесь пока ничего не написано, так, заглушка
//==============================================================================================================
FCreatureDrive AMyrAI::MixWithOtherDrive(FCreatureDrive* Other)
{
	//временный
	FCreatureDrive D;

	//направление атаки берется из преимущественно внешнего
	D.ActDir = Other->ActDir;

	//направление движения сложнее
	D.MoveDir = FMath::Lerp(Other->MoveDir, Drive.MoveDir, AIRuleWeight);
	return D;//
}

//==============================================================================================================
//автонацеливание на найденного противника - вызывается из AMyrPhyCreature, управляемого игроком
//смысл - если мы почти точно целимся в цель ИИ, то ИИ корректирует наш вектор прицела в точности на объект
//==============================================================================================================
bool AMyrAI::AimBetter (FVector3f& OutAttackDir, float Accuracy)
{
	//закрыто на ремонт в связи с рефакторингом атак
	if(Goal_1().Object)
	{	if(me()->GetAttackAction()->QuickAimResult (me(), &Goal_1(), Accuracy))
		{	OutAttackDir = Goal_1().LookAtDir;
			UE_LOG(LogTemp, Log, TEXT("ACTOR %s AimBetter %g"), *me()->GetName(), Goal_1().LookAlign);
			return true;
		}
	}
	if(Goal_2().Object)
	{	if(me()->GetAttackAction()->QuickAimResult (me(), &Goal_2(), Accuracy))
		{	OutAttackDir = Goal_2().LookAtDir;
			UE_LOG(LogTemp, Log, TEXT("ACTOR %s AimBetter %g"), *me()->GetName(), Goal_2().LookAlign);
			return true;
		}
	}
	return false;
}

//==============================================================================================================
//действия, если актор исчез - удалить из долговременной памяти, и из целей
//==============================================================================================================
void AMyrAI::SeeThatObjectDisappeared(UPrimitiveComponent* ExactObj)
{
	//удаление из первоочередных целей, если они есть
	if (Goal_1().Object == ExactObj)
	{	Forget(Goal_1(), Remember(ExactObj));
		PrimaryGoal = SecondaryGoal;
	}
	else if (Goal_2().Object == ExactObj)
	{	Forget(Goal_2(), Remember(ExactObj));
	}

	//удаление из долговременной памяти
	Memory.Remove(ExactObj);
}

//==============================================================================================================
//запустить самоуничтожение, если игрок далеко  отошел от этого существа
//==============================================================================================================
bool AMyrAI::AttemptSuicideAsPlayerGone()
{
	//небывалый случай но все же если вызвано не из уровневого актора
	if(!GetMyrGameMode()) return false;

	//отсыскать единственного протагониста на уровне
	AMyrPhyCreature *PC = GetMyrGameMode()->GetProtagonist();

	//ели прогагонист мы то никакого сицида
	if(PC == me()) return false;

	//если нет протагониста хрень какая-то
	if (!PC)
	{	UE_LOG(LogMyrAI, Error, TEXT("AI %s AttemptSuicideAsPlayerGone WTF no Protagonist"), *me()->GetName());
		return false;
	}

	//подгадываем момент, когда протагонист не смотрит на нас и вообщенаходится вдалеке
	FVector3f LookAtDir = (FVector3f)(PC->GetHeadLocation() - me()->GetHeadLocation());
	float LookAlign = LookAtDir | me()->GetLookingVector();
	if(LookAlign < -ConfigHearing->HearingRange)
	{	
		//проверяем что мы не были изначально установлены на карте - тогда мы будем тут всегда
		if (!ME()->bNetStartup)
		{
			//собственно дестрой, сам ИИ по идее должен сам следом
			UE_LOG(LogMyrAI, Log, TEXT("ACTOR %s AttemptSuicideAsPlayerGone On Player %s Gone up to %g"), *me()->GetName(), *PC->GetName(), LookAlign);
			ME()->Destroy();
			return true;
		}
	}
	return false;
}



//==============================================================================================================
//межклассовое взаимоотношение - для ИИ, чтобы сформировать отношение при первой встрече с представителем другого класса
//==============================================================================================================
FColor AMyrAI::ClassRelationToClass (AActor* obj)
{
	//сам себя - начальное отношение полная любовь
	if (obj == me()) return FColor::Green;

	//найти сборку типовых отношений в генофонде, если не нашли - к неизвестному отношение "никак"
	FAttitudeTo* res = me()->GetGenePool()->AttitudeToOthers.Find(obj->GetClass());
	if (!res) return FColor::Black;

	//если нашли, зависит от того, живое или мёртвое
	else if (auto MyrAim = Cast<AMyrPhyCreature>(obj))
	{	if (MyrAim->Health <= 0)
			return res->EmotionToDeadColor;
	}
	return res->EmotionToLivingColor;
}

//==============================================================================================================
//выдать полные данные по нам как цели, которую имеет существо, находящееся у нас в целях
//==============================================================================================================
FGoal* AMyrAI::MeAsGoalOfThis(const AMyrPhyCreature* MyrAim) const
{
	const auto gn1 = MyrAim->MyrAI()->PrimaryGoal;
	FGoal* G = &MyrAim->MyrAI()->Goals[gn1];
	if (G->Object == me()->GetMesh()) return G;

	const auto gn2 = MyrAim->MyrAI()->SecondaryGoal;
	G = &MyrAim->MyrAI()->Goals[gn2];
	if (G->Object == me()->GetMesh()) return G;

	return nullptr;
}
//==============================================================================================================
//каковы эмоции у вот этого объекта по отношению к нам. Если не живой, то PEACE.
//если он нас не преследует, то дополнительно покопаться в его памяти, как он в принципе к нам относится
//==============================================================================================================
FEmotion AMyrAI::WhatYouFeelOrRememberAboutMe(UPrimitiveComponent* You) const
{
	if (auto NewMyrAim = Cast<AMyrPhyCreature>(You->GetOwner()))//если вы тоже существо, ты вы и правда можете иметь ко мне чувства
	{		if (FGoal* MeAsGoal = MeAsGoalOfThis(NewMyrAim))			//если имеется в целях
				return MeAsGoal->EventMemory.Emotion;//◘◘>					// выдать эмоцию его к нам
		auto Gestalt = Memory.Find(You);									// долговременная память противника
		if (Gestalt) return FEmotion(Gestalt->ToFColor());//◘◘>				// если там не нашли, то никакое отношение
	}
	return FEmotion::Peace();//◘◘>										//предметы не имеют чувств
}

//==============================================================================================================
//насколько это существо нас воспринимает 
//==============================================================================================================
float AMyrAI::HowClearlyYouPerceiveMe(AMyrPhyCreature* You) const
{
	//если имеется в целях, то берем уже посчитанную его видимость/слышимость нас
	if (FGoal* MeAsGoal = MeAsGoalOfThis(You))
		return 0.7 * MeAsGoal->Visibility + 0.3 * MeAsGoal->Audibility;//◘◘>

	//если нет в целях, то насколько он к нам лицом повернут
	return FMath::Clamp((FVector3f)(me()->GetHeadLocation() - You->GetHeadLocation()) | You->GetLookingVector(), 0.0f, 1.0f);//◘◘>
}

//==============================================================================================================
//насколько вот этот объект теоретически опасен (0 - безопасен, 1 - также опасен, как и мы, 10 - в десять раз более опасен)
//==============================================================================================================
float AMyrAI::HowDangerousAreYou(UPrimitiveComponent* You) const
{
	//если вы тоже существо, ты вы и правда можете иметь ко мне чувства
	if (auto NewMyrAim = Cast<AMyrPhyCreature>(You->GetOwner()))					
	{
		//отношение размеров играет роль, хотя еще нужно отношение сил
		return NewMyrAim->GetMesh()->Bounds.SphereRadius
			/ me()->GetMesh()->Bounds.SphereRadius;
	}
	else
	{	//если объект подвижный, он может задавить, важен его размер
		if(You->Mobility == EComponentMobility::Movable)
			return You->Bounds.SphereRadius
				/ me()->GetMesh()->Bounds.SphereRadius;

		//если объект статичный, он физически не опасен
		else return 0.0f;
	}
}

//==============================================================================================================
//найти цель которая в направлении взгляда, если есть - для подписи внизу экрана
//==============================================================================================================
int AMyrAI::FindGoalInView(FVector3f View, const float Devia2D, const float Devia3D, const bool ExactGoal2, AActor*& Result)
{
	Result = nullptr;
	auto& G = Goals[(int)ExactGoal2];
	if(G.Object != nullptr)
		if((FVector2f(G.LookAtDir)|FVector2f(View)) > 1.0f - Devia2D)
			if(FMath::Abs(G.LookAtDir.Z - View.Z) < Devia3D)
			{	if(Cast<AMyrPhyCreature>(G.Object->GetOwner()))
				{	Result = G.Object->GetOwner();
					return 1;
				}
				if(Cast<AMyrArtefact>(G.Object->GetOwner()))
				{	Result = G.Object->GetOwner();
					return 2;
				}
			}
	return 0;
}
