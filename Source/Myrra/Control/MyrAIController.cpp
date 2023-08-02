// Fill out your copyright notice in the Description page of Project Settings.


#include "Control/MyrAIController.h"
#include "Control/MyrDaemon.h"
#include "Creature/MyrPhyCreature.h"

#include "../Artefact/MyrLocation.h"						// локация - для нацеливания на компоненты маячки в составе целиковой локации
#include "../Artefact/MyrArtefact.h"						// артефакт - тоже может быть целью
#include "../MyrraGameInstance.h"							// глобальный мир игры
#include "../MyrraGameModeBase.h"							// уровень - брать протагониста

#include "AIModule/Classes/Perception/AIPerceptionComponent.h"
#include "AIModule/Classes/Perception/AISenseConfig_Hearing.h"

#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"							// для вызова GetAllActorsOfClass

#include "DrawDebugHelpers.h"								// для рисования

//свой лог
DEFINE_LOG_CATEGORY(LogMyrAIC);

//==============================================================================================================
// конструктор
//==============================================================================================================
AMyrAIController::AMyrAIController(const FObjectInitializer & ObjInit)
{
	// тик ИИ - редко, два раза в секунду, вообще это число надо динамически подстраивать
	PrimaryActorTick.TickInterval = 0.5f;
	PrimaryActorTick.bCanEverTick = true;

	//модуль перцепции
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception Component"));
	SetPerceptionComponent(*AIPerception);
	AIPerception->bEditableWhenInherited = true;

	//настройки слуха, предварительные - они должны меняться при подключении к цели
	ConfigHearing = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Myrra Hearing"));
	if (ConfigHearing)
	{	ConfigHearing->HearingRange = 1000.f;
		AIPerception->ConfigureSense(*ConfigHearing);
	}
	//исключить болванки, не являющиеся реальными объектами
	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	ComplexEmotions.Add(FReflex(YE_(Shine)|YE_(Night),			FPathia( Hope,				+2)));
	ComplexEmotions.Add(FReflex(YE_(NotMe)|YE_(Big),			FPathia( Fear,				+2)));
	ComplexEmotions.Add(FReflex(YE_(NotMe)|YE_(New),			FPathia( Care,				+5)));
	ComplexEmotions.Add(FReflex(YE_(NotMe)|YE_(New)|YE_(Big),	FPathia( Anxiety,			+3)));
	ComplexEmotions.Add(FReflex(YE_(NotMe)|YE_(Unreachable),	FPathia( Pessimism,			+2)));
	ComplexEmotions.Add(FReflex(YE_(NotMe)|YE_(Important),		FPathia( Mania,				+2)));
	ComplexEmotions.Add(FReflex(YE_(NotMe)|YE_(PleasedMe),		FPathia( Care,				+5)));


	//привзять обработчик событий прилёта слуха и навешанных на него событий
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AMyrAIController::OnTargetPerceptionUpdated);
}


//==============================================================================================================
//обработчик занятия объекта (при его появлении или в начале игры/уровня
//==============================================================================================================
void AMyrAIController::OnPossess(APawn * InPawn)
{
	//переименовать, чтобы в отладке в редакторе отображалось имя, ассоциированное с подопечным существом
	Super::OnPossess(InPawn);
	FString NewName = InPawn->GetName() + TEXT("-") + GetName();
	Rename(*NewName);

	//добавить самый главный гештальт - самого себя
	Mich = FGestalt(meC());

	//если существо управляется игроком, то внести также отношение к игроку
	if(me()->Daemon) Memory.Add (FGestalt(me()->Daemon->GetCamera()));

	//всё
	UpdateSenses();
}

//==============================================================================================================
//нужно, когда существо исчезает в норе или на дистанции
//==============================================================================================================
void AMyrAIController::OnUnPossess()
{
	//по какому-то маразму разработчиков для одного ИИ контроллера это событие вызывается 2 раза  
	//сначала до отделения Pawn, потому после, когда эта переменная уже 0
	if (!me()) return;
	UE_LOG(LogMyrAIC, Warning, TEXT("AI %s is being destroyed"), *me()->GetName());
	Memory.Remove(me()->GetMesh());

	//остановить тик, потому что за каким-то хером он между утерей подопечного и самодестроем продолжает тичить
	PrimaryActorTick.SetTickFunctionEnable(false);

	//остановить дополнительную тик-функцию, а то после отъёма объекта управления она почему-то еще работает
	Super::OnUnPossess();
}

//==============================================================================================================
// начать игру / ввести экземпляр контроллера в игру
//==============================================================================================================
void AMyrAIController::BeginPlay()
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
//событие восприятия - на данный момент, когда ловим звук или спец-сообщение от другого существа
//==============================================================================================================
void AMyrAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Actor == me()) return;					// это вообще бред, почему себя нельзя исключить
	if (Actor == me()->Daemon) return;			// это вообще бред, почему себя нельзя исключить
	auto MyrYou = Cast<AMyrPhyCreature>(Actor);

	auto ExactObj = Actor->GetRootComponent();	// выделить из актора конкретный компонент-источник-цель, по умолчанию корневой компонент актора
	if (auto L = Cast<AMyrLocation>(Actor))		// если это локация, то там может быть много манящих объектов
		ExactObj = (USceneComponent*)L->GetCurrentBeacon();		// выдать текущий

	// радиус-вектор от нас до объекта, по умолчанию до точки начала координат компонента-цели
	auto YouMinusMe = GetMeToYouVector(ExactObj);

	//корректность, если не WasSuccessfullySensed, это, возможно, утеря
	if (Stimulus.WasSuccessfullySensed())
	{
		//слух и другие мгнровенные воздействия без пространства
		EHowSensed HowSensed = EHowSensed::HEARD;
		float Strength = Stimulus.Strength;

		//попробовать вспомнить замеченный объект или если не знаем
		//сформировать совсем новый гештальт, задать начальне характеристики
		auto MeinGestalt = Memory.Find(ExactObj);
		if (!MeinGestalt)
		{	FGestalt NeuGestalt(ExactObj);
			MeinGestalt = &Memory[Memory.Add(NeuGestalt)];
			MeinGestalt->InitRival(me()->SpineLength, MyrYou ? MyrYou->SpineLength : 0);	
		}

		//сразу асинхронно определить положение и воспринять акт услышания, и получить расстояние до объекта
		float DistTo = MeinGestalt->Hear(YouMinusMe, me()->AttackDirection);

		//спецсигналы - расщепляем "силу" на целый идентификатор и дробную силу
		if (Stimulus.Tag == TEXT("HeardSpecial"))
		{	HowSensed = (EHowSensed)((int)Stimulus.Strength);
			Strength = Strength - (int)Stimulus.Strength;
			bool Strike = false;

			//анализ служебных сигналов
			switch(HowSensed)
			{
				//начало или свершение атаки
				case EHowSensed::ATTACKSTRIKE:
					Strike = true;
				case EHowSensed::ATTACKSTART:
					{
						//вывалить все основные переменные
						auto YouAI = MyrYou->MyrAIController();	
						auto AInfo = MyrYou->GetAttackAction();
						auto& Attack = MyrYou->GetAttackActionVictim();
						float HowAffectsMe = Attack.IntegralAmount ( DistTo, MeinGestalt->VisCoaxis);
						uint8 iCounterAttack = 255;

						//если атака несет урон и мы хотя бы потенциально попадаем в область поражения
						if(HowAffectsMe * Attack.TactileDamage >= 0)
						{
							//как бы то ни было, а нацелиться на виновника надо сразу
							Drive.ActDir = MeinGestalt->Location.DecodeDir();

							//если мы тоже выполняем какую-то атаку или самодействие, но оно слишком слабое, чтобы противостоять урону, прервать его
							if(me()->DoesAttackAction())	if (HowAffectsMe > me()->GetAttackDynModel()->DamageLim)		me()->AttackEnd();
							if(me()->DoesSelfAction())		if (HowAffectsMe > me()->GetSelfDynModel()->DamageLim)			me()->SelfActionCease();
							if(me()->DoesRelaxAction())		if (HowAffectsMe > me()->GetRelaxDynModel()->DamageLim)			me()->RelaxActionStartGetOut();

							//если мы не делали атаку или только что прервали безнадежную атаку, то значит теперь мы можем найти контр-действие
							if(!me()->DoesAttackAction())
							{
								//найти подходящую контратаку или контр-самодействие, и запустить
								iCounterAttack = me()->ActionFindBest(ExactObj, HowAffectsMe, DistTo, (float)MeinGestalt->VisCoaxis, true, true);
								if(iCounterAttack != 255)
								{	auto A = me()->GetGenePool()->Actions[iCounterAttack];
									if(A->IsAttack())	ME()->AttackActionStart(iCounterAttack, MyrYou->GetMesh());
									else				ME()->SelfActionStart(iCounterAttack);
								}
							}

							//при ударе врага попытка немедленно активизировать свою атаку
							if(me()->DoesAttackAction() && me()->PreStrike())
							{	
								//если враг уже начинает реально бить, или если не начинает, но мы можем сильно пострадать - ударить тоже
								if(Strike || HowAffectsMe > 0.5)
									if(ActOk(me()->GetAttackAction()->RetestForStrikeForAI(me(), MyrYou->GetMesh(), DistTo, (float)MeinGestalt->VisCoaxis)))
									if (!StrOk(ME()->AttackActionStrike()))
										ME()->AttackEnd();
							}

							//если все слабые действия были прерваны, а сильные не найдены, то путь один - бегство
							if(!me()->DoesAnyAction())
							{	Drive.ActDir = -MeinGestalt->Location.DecodeDir();
								Drive.MoveDir = Drive.ActDir;
								Drive.Gain = 1.0f;
								if(me()->CanFly()) Drive.DoThis = EAction::Soar;	//для летающих вспорхнуть
								else Drive.DoThis = EAction::Run;
							}
						}

						//если мы видим и слышим атакующего
						if(MeinGestalt->PerceptAmount() > 0.2 )
							MeinGestalt->AddInfluence(EYeAt::Attacking);
					}
					break;

				case EHowSensed::ATTACKEND:
					MeinGestalt->DelInfluence(EYeAt::Attacking);
					break;

				case EHowSensed::DIED:
					MeinGestalt->MarkDead();
					break;

				case EHowSensed::REMOVE:
					Memory.Remove(*MeinGestalt);
					break;

				//эпизод удара атакой этого существа по другому существу, возможно, по нам
				case EHowSensed::HITTER:
					{
						//участники событий
						auto YouAI = MyrYou->MyrAIController();	
						auto Victim = Cast<AMyrPhyCreature>(YouAI->ExactVictim->GetOwner());

						//если жертва формально мы
						if(Victim == me())
						{
							//повысить нашу мнемоническую враждебность
							MeinGestalt->ReactToDamage(Strength, fRand());
						}

						//если жертва не мы, но мы знаем это существо
						else if(auto pGestalt = MeInYourMemory(Victim))
						{
							//если существо наш давний друг, зафиксировать воздействие
							if(pGestalt->iFriendliness() + 200 + Strength > (FUrGestalt::RandVar & 1023))
								MeinGestalt->AddInfluence(EYeAt::HurtFriend);
						}
					}
					break;


				case EHowSensed::EXPRESSED:
					break;

				case EHowSensed::CALL_OF_HOME:
					break;
			}
		}
		//проталкиваем стимул глубже - на уровень замечания и рассовывания по ячейкам целей
		//UE_LOG(LogTemp, Error, TEXT("%s AI Notices %s result %s"), *me()->GetName(), *Actor->GetName(), *TXTENUM(EGoalAcceptResult, NotRes));
	}
}

//==============================================================================================================
// рутинные действия, внимание, если нет подвязанного существа, то тик должен быть выключен
//==============================================================================================================
void AMyrAIController::Tick(float DeltaTime)
{
	
	Super::Tick(DeltaTime);			// внутря
	Counter++;						// счетчик для периодических поведений

	//возможно, рандом тяжёл, пусть он случается реже основных расчётов
	FUrGestalt::RandVar = FMath::Rand();		
	if (FUrGestalt::RandVar == 0) FUrGestalt::RandVar = 1;

	//часть медленных не нужных часто процедур самого тела, дабы не плодить тиков и делителей, вынесена сюда
	ME()->RareTick(DeltaTime);

	FAIGoal GoalMaxLure;		// цель, к которой максимальное влечение
	FAIGoal GoalMinLure;		// цель, от которой максимальное отторжение
	EmoChangeAccum = 0;			// суммарное изменение в запасе сил, может быть отрицательным (истощение) и положительным (восполнение)
	EmoBalanceAccum = 0;		// суммарное изменение в запасе сил, может быть отрицательным (истощение) и положительным (восполнение)
	EmoWorkAccum = 0;			// сумма текущих сил всех эмоций, способность воспринимать новые эмоции
	EmoEnergyAccum = 0;			// сумма мощности всех эмоций, общие затраты энергии на поддержания горения эмоций
	float MaxCoax = -1;
	float Extraversy = 0;

	////////////////////////////////////////////////////////
	//возможно, стоит разбить этот цикл по тактам
	for(auto& Gestalt : Memory)
	{
		//выставить влияния, характерные исключительно для внешнего объекта
		Gestalt.SetCauseChance(YE_(Injured),	0.9 * FMath::Square(0.5f - me()->Health));
		Gestalt.SetCauseChance(YE_(Dying),		0.9 * FMath::Square(0.5f - me()->Health));
		Gestalt.SetCauseChance(YE_(Tired),		0.9 * FMath::Square(0.5f - me()->Stamina));
		Gestalt.SetCauseChance(YE_(NoticingMe), Gestalt.PerceptAmount());

		//перевалить отношение к образу за очередной такт
		UpdateEmotions(Gestalt, Gestalt.GetWeight());
		Extraversy += Gestalt.Emotion.UniPower() * Gestalt.GetWeight();

		//приближение и удаление живого противника
		if (Gestalt.IsCreature())
			if (Gestalt.Creature()->GetThorax()->Speed > 50)
			{
				//вектор скорости в проекции на вектор направления, вблизи одна сотая (меньше порог срабатывания) вдали одна двухсотая (нужно сильно разогнаться)
				float Coaxis = -Gestalt.Creature()->GetThorax()->VelocityAgainstFloor | Gestalt.LookDir() * (0.005 + 0.005 * Gestalt.HowClose());

				//рассыпание признаков "приближается" или удаляется
				if (Coaxis > 0.5)				Gestalt.SetCauseChance(YE_(ComingCloser),	Coaxis - 0.5);
				else if (Coaxis < -0.5)			Gestalt.SetCauseChance(YE_(ComingAway),		0.5 - Coaxis);
			}

		//осознанная близость
		Gestalt.SetCauseChance(YE_(ComingCloser) | YE_(ComingAway), FMath::Square(Gestalt.HowClose()));

		//рассчет объекта в фокусе для помощи в нацеливании
		if(Gestalt.VisCoaxis > MaxCoax) { AtFocus = &Gestalt; MaxCoax = Gestalt.VisCoaxis; }

		//если запись памяти указывает на реальный объект
		if(Gestalt.Obj.IsValid())
		{
			//найти самую влекущую и самую противную цель по уровню тяги
			//вместе с внесением пересчитать локацию кандидата в цели на текущий такт
			auto Gain = Gestalt.GetAnalogGain();
			GoalMaxLure.SeizeIf(Gestalt, Gain, true);
			GoalMinLure.SeizeIf(Gestalt, Gain, false);
		}
	}

	//поводы для внутренних эмоций
	Mich.SetCause2bit(ME2B_(Pain),			FMath::Min(1.0f, me()->Pain * 0.5));
	Mich.SetCause2bit(ME2B_(Injured),		1 - me()->Health);
	Mich.SetCause2bit(ME2B_(Tired), 		1 - me()->Stamina);
	Mich.SetCauseChance(ME_(HurtHead),		me()->GetDamage(ELimb::HEAD));
	Mich.SetCauseChance(ME_(HurtTorso),		me()->GetDamage(ELimb::PECTUS) + me()->GetDamage(ELimb::LUMBUS));
	Mich.SetCauseChance(ME_(Falling),		me()->CurrentState == EBehaveState::fall ? -me()->GetThorax()->VelocityAgainstFloor.Z*0.01 : 0.0);

	//переварить сосотояния образа себя
	UpdateEmotions(Mich, 1 - FMath::Min(Extraversy, 1.0f));

	//если самая передняя цель не очень спереди, то передних целей нет вообще
	if(MaxCoax < 0.7) AtFocus = nullptr;

	////////////////////////////////////////////////////////
	//есть две разные цели
	if(GoalMaxLure.pGestalt != GoalMinLure.pGestalt)
	{
		//степень раскоряки между 2 целями, если мменьше нуля, цели по разные сторон, за обеими не погонишься
		float Along = GoalMinLure.LookAtDir() | GoalMaxLure.LookAtDir();

		//если стремления разного знака, то минус, иначе обе хочется поймать или избечь
		float Bipol = GoalMinLure.AnalogGain * GoalMaxLure.AnalogGain;

		//к одной цели хочется стремиться, от другой убежать
		if(Bipol < 0)
		{
			//если цель и опасность по одну сторону, причем опасность ближе
			if(Along > 0 && GoalMinLure.Dist() < GoalMaxLure.Dist())
			{
				//найти путь обхода
				GoalMinLure.WhatToDo = FreeMoveAway(GoalMinLure, &GoalMaxLure);
				GoalMaxLure.TransferRoute(GoalMinLure, ERouteResult::Away_Hamper);
			}
			//если цели в разнх сторонах, проще всего
			else
			{
				//просто бежать к цели
				GoalMaxLure.WhatToDo = SimpleMoveToGoal(GoalMaxLure);
				GoalMinLure.WhatToDo = ERouteResult::Away_Reckless;
			}
		}
		//к обеим целям хочется стремиться
		else if(GoalMaxLure.AnalogGain > 0)
		{
			//если менее привлекательная цель намного хуже в этом плане
			if(GoalMinLure.AnalogGain < GoalMinLure.AnalogGain*0.5)
			{
				//игнорировать вторую цель начисто
				GoalMaxLure.WhatToDo = SimpleMoveToGoal(GoalMaxLure);
				GoalMinLure.WhatToDo = ERouteResult::Ignore;
			}
			//цели с одной стороны
			else if(Along > 0)
			{
				//какое-то время бежать к одой или к средней точке
				GoalMaxLure.WhatToDo = SimpleMoveToGoal(GoalMaxLure);
				GoalMinLure.WhatToDo = ERouteResult::Towards_Glance;
			}
			//цели по разные стороны
			else
			{
				//раскорячиться, застопориться
				GoalMaxLure.WhatToDo = SimpleMoveToGoal(GoalMaxLure);
				GoalMinLure.WhatToDo = SimpleMoveToGoal(GoalMinLure);
			}
		}
		//от обеих целей хочется бежать
		else
		{
			//вычислить направление бегства исходя из дальности угроз
			GoalMinLure.WhatToDo = FreeMoveAway(GoalMinLure, &GoalMaxLure);
			GoalMaxLure.TransferRoute(GoalMinLure, ERouteResult::Away_Assist);
		}
	}
	//есть только одна цель GoalMaxLure == GoalMinLure
	else if(GoalMaxLure.pGestalt)	
	{
		//просто бежать к цели
		if(GoalMaxLure.AnalogGain > 0)
			GoalMaxLure.WhatToDo = SimpleMoveToGoal(GoalMaxLure);
		else GoalMinLure.WhatToDo = FreeMoveAway(GoalMinLure);

	}
	//нет целей вообще
	else							
	{
		//лечь и отдыхать, а вообще на уровне не должно быть совсем без гештальтов
		return;
	}

	//желание скрываться - суммарно от обеих целей
	CalcStealth(GoalMinLure);
	CalcStealth(GoalMaxLure);
	float Stealth = GoalMinLure.AnalogStealth + GoalMaxLure.AnalogStealth;

	////////////////////////////////////////////////////////
	//выбор направления движения

	//из целей по притягательности формируем цели по силе влияния
	auto& PriGoal = GoalMaxLure.UGain() >= GoalMinLure.UGain() ? GoalMaxLure : GoalMinLure;
	auto& SecGoal = GoalMaxLure.UGain() < GoalMinLure.UGain() ? GoalMaxLure : GoalMinLure;
	float Along = PriGoal.MoveToDir | SecGoal.MoveToDir;
	float Proport = PriGoal.UGain() / (SecGoal.UGain() + PriGoal.UGain());

	//если более влиятельная цель намного более влиятельная, чем менее влиятельная
	if(&PriGoal == &SecGoal || Proport > 0.8)
	{	Drive.MoveDir = PriGoal.MoveToDir;
		Drive.Gain = PriGoal.AnalogGain;
		Drive.ActDir = PriGoal.LookAtDir();
	}
	//средний уровень различий
	else if(Proport > 0.4)
	{	Drive.MoveDir = PriGoal.MoveToDir;
		Drive.Gain = PriGoal.AnalogGain;
		Drive.ActDir = (Along < 0.5 || Period(63, Proport))
			? PriGoal.LookAtDir() : SecGoal.LookAtDir();
	}
	//цели почти равны по притягательности, и расположены примерно в одной стороне
	else if(Along > 0.5)
	{	Drive.MoveDir = PriGoal.CombineMove(SecGoal);
		Drive.Gain = 0.8 * FMath::Lerp(PriGoal.AnalogGain, SecGoal.AnalogGain, Proport);
		Drive.ActDir = Period(63, Proport) ?
			 PriGoal.LookAtDir() : SecGoal.LookAtDir();
	}
	//цели почти равносильны, и расположены по разные стороны, и выпал период выбора первой цели
	else 
	{	auto& CurGoal = Period(63, Proport) ? PriGoal : SecGoal;
		Drive.MoveDir = CurGoal.MoveToDir;
		Drive.Gain = CurGoal.AnalogGain * 0.8;
		 Drive.ActDir = Period(31, Proport) ?
			 PriGoal.LookAtDir() : SecGoal.LookAtDir();
	}

	////////////////////////////////////////////////////////
	//выработка способа движения
	//очень высокая тяга к цели, бег
	if(Drive.Gain > 1)
	{	Drive.Set (EAction::Run,		0.8f + 0.3*Drive.Gain,		Drive.Gain < 1.5);
		Drive.Set (EAction::Sprint,		1.0f + 0.1*Drive.Gain,		Drive.Gain > 1.5);
	}
	//средняя тяга к цели, шаг
	else if(Drive.Gain > 0.4)
	{	Drive.Set (EAction::Walk,		0.7f + 0.5*Drive.Gain,		Stealth < 0.5);
		Drive.Set (EAction::Crouch,		0.2f + 0.8*Drive.Gain,		Stealth > 0.5);
	}
	//слабая тяга к цели, неуверенность
	else if(Drive.Gain > 0)
	{	Drive.Set (EAction::Walk,		(float)Period(63),			Stealth < 0.5);
		Drive.Set (EAction::Crouch,		0.2f + 0.8*Drive.Gain,		Stealth > 0.5);
	}
	//тяга даже на самой приятной цели ниже нуля, переходим к самой противной
	//уровень неприязни оченб большой, бегство
	else if(Drive.Gain < -1)
	{	Drive.Set (EAction::Run,		0.8f - 0.3*Drive.Gain,		Drive.Gain < -1.5);
		Drive.Set (EAction::Sprint,		1.0f - 0.1*Drive.Gain,		Drive.Gain > -1.5);
	}
	//уровень неприязни средний, шаг
	else if(Drive.Gain < -0.4)
	{	Drive.Set (EAction::Walk,		0.7f - 0.5*Drive.Gain,		Stealth < 0.3);
		Drive.Set (EAction::Crouch,		0.2f - 0.8*Drive.Gain,		Stealth > 0.3);
	}
	//слабая неприязнь
	else
	{	Drive.Set (EAction::Walk,		(float)Period(63),			Stealth < 0.3);
		Drive.Set (EAction::Crouch,		0.2f - 0.8*Drive.Gain,		Stealth > 0.3);
	}

	////////////////////////////////////////////////////////
	//воля на выполнение своих действий, пока перебор 2 элементов массива за такт
	ProposeAction(PriGoal);
	ProposeAction(PriGoal);
}

//==============================================================================================================
//попробовать в отношении текущей цели какое-нибудь действие, продвинуть счетчик
//==============================================================================================================
void AMyrAIController::ProposeAction(FAIGoal& Goal)
{
	ActionToTryNow++;
	if (ActionToTryNow >= me()->GetGenePool()->Actions.Num())
	 ActionToTryNow = 0;
	auto A = me()->GetGenePool()->Actions[ActionToTryNow];
	auto R = A->IsActionFitting(me(), Goal.pGestalt->Obj.Get(), Goal.Dist(), Goal.LookAtDir() | me()->AttackDirection, true, true);
	if(ActOk(R)) me()->ActionStart(ActionToTryNow);

}

//==============================================================================================================
//рутинная обработка в тике одного конкретного гештальта
//==============================================================================================================
void AMyrAIController::UpdateEmotions(FUrGestalt& Gestalt, float Weight)
{
	if(Weight < 0.01) return;					// совсем пассивный гештальт не должен обновляться
	uint8 Limit = Weight * 16;					// гештальты со слабым весом меньше обрабатывают эмоции (ограничить 0-16)
	int32 OldInfluences = Gestalt.Influences;	// сохранить влияния вокруг с предыдущего такта

	//обновить стимулы внешней среды, одинаковые как для себя, так и для внешних гештальтов
	Gestalt.SetCauseChance(YE_(Night),		0.5 * GetMyrGameMode()->NightAmount());
	Gestalt.SetCauseChance(YE_(Shine),		0.5 * (GetMyrGameMode()->MoonIntensity() + GetMyrGameMode()->SunIntensity()));
	Gestalt.SetCauseChance(YE_(Rain),		0.9 * (me()->Daemon ? me()->Daemon->RainAmount : GetMyrGameMode()->RainAmount()));
	Gestalt.SetCauseChance(YE_(Fog),		0.5 * GetMyrGameMode()->FogAmount());
	Gestalt.SetCauseChance(YE_(Cold),		0.5 * FMath::Square(1 - GetMyrGameMode()->Temperature()));
	Gestalt.SetCause(YE_(Indoor),			(bool)me()->IsInLocation());

	//----------------------------------------------------------------
	//применение эмоциональных воздействий
	int ChangeInEmotion = 0;
	auto pReflex = ComplexEmotions.Find(Gestalt.Influences);

	//сначала перебор комплексных симулов на точное совпадение
	if (pReflex && !pReflex->Emotion.IsLatent())
		RegModEmotion(Gestalt.Emotion.Affect(pReflex->Emotion, Limit));//◘◘>
	else
	{
		//попытаться найти ближайший по битам рефлекс и повлиять им
		auto CloR = FindClosest(Gestalt.Influences);
		if(CloR)
			RegModEmotion(Gestalt.Emotion.Affect(CloR->Emotion, Limit));//◘◘>

		//если вообще никакой ближайший не найден
		else
		{
			//суперпозиция элементарных рефлексов
			for (int i = 0; i < 32; i++)
				if ((Gestalt.Influences & (1 << i)) != 0)
					RegModEmotion(Gestalt.Emotion.Affect(me()->GetGenePool()->ElementaryEmotionsYe[i], Limit));//◘◘>
		}



		//если запись рефлекса все же найдена, то это "латентный рефлекс" считающий разы, частотность события
		if(pReflex)
		{
			//увеличить счетчик случаев с данной комбинацией условий, если она вновь появилась или достаточно длится
			if(OldInfluences != Gestalt.Influences)
				pReflex->Emotion.Work++;

			//если достигнут предел латентности, значит событий такого рода уже достаточно накапло, чтоб создать новый рефлекс
			if(pReflex->Emotion.Work > 200)
			{	
				//если мы успешно находили ближайший рефлекс, начальным значением нового рефлекса сделать эмоцию от ближайшего неравного 
				if(CloR) pReflex->Emotion = CloR->Emotion;

				//если ближайшего рефлекса не находили, то тупо собрать значение по сумме элементарных воздействий
				else
				{	FPathia Compound(Peace);
					for (int i = 0; i < 32; i++)
						if ((Gestalt.Influences & (1 << i)) != 0)
							Compound = Compound * me()->GetGenePool()->ElementaryEmotionsYe[i];
					pReflex->Emotion = Compound;
				}
			}
		}
		//если запись не найдена - это первый раз такая комбинация, создать для нее латентный рефлекс, чтобы считать количество
		else ComplexEmotions.Add ( FReflex(Gestalt.Influences, FPathia('l')) );
	}
	//----------------------------------------------------------------
	EmoWorkAccum += Gestalt.Emotion.Work;					//◘>
	EmoEnergyAccum += Gestalt.Emotion.uiDistAxial(Peace);	//◘>

	//выразить общую внешнюю эмоцию через текущие отношения к целям внимания
	IntegralEmotion.Interpo(Gestalt.Emotion, FMath::Min(1.0f, FMath::Abs(Weight*0.5f)));
}





//==============================================================================================================
//если мы прокачались в чувствительности, изменить радиусы зрения, слуха...
//==============================================================================================================
void AMyrAIController::UpdateSenses()
{
	//контроллер не висит в пустоте - обновить 
	if(me())
	{	ConfigHearing->HearingRange	= me()->GetGenePool()->DistanceHearing;
		AIPerception->RequestStimuliListenerUpdate();
	}
}

//==============================================================================================================
//если этот образ - существо, то оно, возможно, имеет нас в памяти
//==============================================================================================================
FGestalt* AMyrAIController::MeInYourMemory(AMyrPhyCreature* Myr)
{
	return Myr ? Myr->MyrAIController()->Memory.Find(meC()) : nullptr;
}

//==============================================================================================================
//найти точный вектор от нас на данный объект
//==============================================================================================================
FVector3f AMyrAIController::GetMeToYouVector(USceneComponent* C)
{
	auto M = Cast<AMyrPhyCreature>(C->GetOwner());					// если объект существо
	return FVector3f ( me()->GetHeadLocation()						// из головы глянуть на случайную часть тела
		- (M ? M->GetVisEndPoint() : C->GetComponentLocation()) );	// иначе просто в центр координат объекта
}

//==============================================================================================================
//посмотреть на объект, определить его видимость
//==============================================================================================================
FHitResult AMyrAIController::See(FVector ExplicitPos, USceneComponent* NeededObj)
{
	if(NeededObj)
	{	auto YouCre = Cast<AMyrPhyCreature>(NeededObj->GetOwner());			// если объект существо
		ExplicitPos = YouCre ? YouCre->GetVisEndPoint()						// из головы глянуть на случайную часть тела
				: NeededObj->GetComponentLocation();						// иначе просто в центр координат объекта
	}
	FHitResult VisHit(ForceInit);
	FCollisionQueryParams RV_TraceParams(TEXT("AI_Watch"), false, me());	// трассировать без полигонов и без нас
	RV_TraceParams.bReturnPhysicalMaterial = true;							// не факт что нужно
	
	//запуск
	GetWorld()->LineTraceSingleByChannel (VisHit, me()->GetHeadLocation(), ExplicitPos, ECC_WorldStatic, RV_TraceParams);
	return VisHit;
}

//==============================================================================================================
//просто обойти препятствие, вычислив его размеры
//==============================================================================================================
ERouteResult AMyrAIController::SimpleWalkAroundObstacle(FVector3f TempLookDir, FHitResult* Hit, FAIGoal& G)
{
	//примерная толщина компонента, который надо бы обойти
	float Thickness = Hit->Component.Get()->Bounds.BoxExtent.X + Hit->Component.Get()->Bounds.BoxExtent.Y;

	//оценить высоту препятствия (самая высокая точка в мировых координатах)
	float Altitude = Hit->Component.Get()->Bounds.Origin.Z + Hit->Component.Get()->Bounds.BoxExtent.Z;

	//вектор обхода в сторону - по идее всё равно, идём мы к цели или от цели
	FVector3f RadicalByPass = TempLookDir ^ me()->GetUpVector();

	//попробовать загодя обойти объект, который мешается
	FVector ClosestPoint = Hit->Location + RadicalByPass * Thickness;
	G.MoveToDir = FVector3f(ClosestPoint - me()->GetHeadLocation());
	G.MoveToDir.ToDirectionAndLength(G.MoveToDir, G.FirstDist);
	UE_LOG(LogTemp, Log, TEXT("AI %s SimpleWalkAroundObstacle %s"), *me()->GetName(), *Hit->Component->GetOwner()->GetName());
	if(G.AnalogGain >= 0) return ERouteResult::Towards_Walkaround;//◘◘>
	else return ERouteResult::Away_Walkaround;//◘◘>
}

//==============================================================================================================
//различные варианты движения к найденной цели
//==============================================================================================================
ERouteResult AMyrAIController::SimpleMoveToGoal(FAIGoal& G)
{
	//посмотреть на цель, проверить видимость
	FHitResult HitSeen = G.pGestalt->VisCoaxis > 0 ? See(FVector(0), G.pGestalt->Obj.Get()) : FHitResult(ForceInit);
	bool Seen = (HitSeen.Component.Get() == G.pGestalt->Obj || !HitSeen.Component.IsValid());
	G.pGestalt->SetInfluence (EYeAt::Seen, Seen);
	
	//если цель высоко, и мы не умеем летать
	if (G.LookAtDir().Z > 0.5 && !me()->CanFly())
	{
		//если цель существо или артефакт, мы можем найти предмет, на котором она физически лежит/сидит/стоит
		auto Cre = Cast<AMyrPhyCreature>(G.Owner());
		auto Art = Cast<AMyrArtefact>(G.Owner());
		auto CompoBase = Cre ? Cre->GetObjectIStandOn() : (Art ? Art->Floor.Component() : nullptr);

		//если цель сидит на каком-то предмете
		if (CompoBase)
		{	
			//рассмотрим вариант движения к этому предмету
			auto ToBase = ((FVector3f)(CompoBase->GetComponentLocation() - me()->GetHeadLocation())).GetSafeNormal();
			ToBase.ToDirectionAndLength(G.MoveToDir, G.FirstDist);

			//имеет смысл идти к предмету только если расстояние до него не намного больше расстояния до цели по прямой
			if (G.FirstDist < G.Dist()*2)
				return ERouteResult::Towards_Base;//◘◘>
		}

		//если дошли до этого место, значит мы не можем с нашей локомоцией достичь цели, разворачиваемся 
		G.MoveToDir = -G.LookAtDir();
		G.FirstDist = G.Dist();
		G.pGestalt->AddInfluence(EYeAt::Unreachable);
		return ERouteResult::GiveUp_For_Unreachable;//◘◘>
	}
	//если цель в физическом доступе, но не видна, обогнуть препятствие, которое его загораживает
	else if(!Seen) return SimpleWalkAroundObstacle(G.MoveToDir, &HitSeen, G);//◘◘>

	//если цель не высоко или не существо
	G.MoveToDir = G.LookAtDir();
	return ERouteResult::Towards_Directly;//◘◘>
}

//==============================================================================================================
//бежать прочь от цели, без конкретной ориентации, рассчитать вектор бегства с учётом второй цели
//==============================================================================================================
ERouteResult AMyrAIController::FreeMoveAway(FAIGoal& MainGoal, FAIGoal* InfluentGoal)
{

	//вектор прочь не нацеленный ни на что, просто в противоположном направлении
	FVector3f Away = -MainGoal.LookAtDir();
	if(InfluentGoal)
	{	Away =	MainGoal.LookAtDir() 		* MainGoal.AnalogGain +
				InfluentGoal->LookAtDir()	* InfluentGoal->AnalogGain;}
	Away *= MainGoal.Dist();

	//провести визуальную оценку, 
	FHitResult HitSeen;
	bool Seen = false;

	//если смотрим на угрозу, то бежим назад не глядя, 
	if(MainGoal.pGestalt->VisCoaxis > 0)
	{	HitSeen = See(FVector(0), MainGoal.pGestalt->Obj.Get());
		Seen = (HitSeen.Component.Get() == MainGoal.pGestalt->Obj.Get() || !HitSeen.Component.IsValid());
		MainGoal.pGestalt->SetInfluence (EYeAt::Seen, Seen);
	}
	//если отвеёрнуты от угрозы, то визуально детектируем препятсвие
	else HitSeen = See(me()->GetHeadLocation() + (FVector)Away);
	
	//если мы смотрели прочь от цели и увидели препятствие, постараться его обойти
	if(HitSeen.Component.IsValid() && !Seen)
		return SimpleWalkAroundObstacle(Away, &HitSeen, MainGoal);//◘◘>

	//просто устремиться по выбранному вектору прочь от угрозы
	//впереди на много метров никаких препятствий
	Away.ToDirectionAndLength(MainGoal.MoveToDir, MainGoal.FirstDist);//◘◘>
		return ERouteResult::Away_Directly;
}

//==============================================================================================================
//рассчитать желаемый уровень скрытности для этой цели
//==============================================================================================================
void AMyrAIController::CalcStealth (FAIGoal& Goal)
{
	//стирание предыдущего
	Goal.AnalogStealth = 0.0f;
	if(!Goal.pGestalt) return;

	//прятаться имеет смысл только от живых существ, если это оно...
	auto& U = *Goal.pGestalt;
	auto Crea = Cast<AMyrPhyCreature>(U.Obj.Get());
	if(Crea)
	{
		//базовая украдкость зависит от дальности и уровня страха к цели
		Goal.AnalogStealth = U.FearStealth();

		//если мы намеренно идём к цели и обладаем манией - включается скрытность как охотничье поведение
		if(Goal.AnalogGain > 0.5) Goal.AnalogStealth += U.Emotion.fMania() * U.HowClose();

		//если мы достаточно хорошо видим цель, мы можем понять, видит ли она нас
		if(U.PerceptAmount() > 0.5f)
		{
			//получаем образ нас в мозгах цели, если получили, то
			auto MeInYou = MeInYourMemory(Crea);
			if(MeInYou)
			{
				//расслабляемся, если цель далеко и нас не видит
				if(MeInYou->Visibility < 0.1)
					Goal.AnalogStealth *= (1 - MeInYou->Visibility * MeInYou->Location.D); else

				// сдаемся до 0, если цель явно нас видит, и мы тоже хорошо это видим
				if(MeInYou->Visibility > 0.7)
					Goal.AnalogStealth *= 0.5*(1 - U.Visibility);
			}
		}
	}
}

//==============================================================================================================
//найти еомплексный рефлекс наиболее близкий к введенному по 
//==============================================================================================================
FReflex* AMyrAIController::FindClosest(int32 InfluBits) const
{
	//минимальное расстояние хэмминга по битам
	FReflex* Found = nullptr;
	uint8 MinSetBits = 200;
	for(auto& R : ComplexEmotions)
	{
		uint32 X = InfluBits ^ (uint32)R.Condition;
		uint8 SetBits = 0;
		while(X>0) {SetBits += X&1; X>>=1;}
		if(SetBits < MinSetBits) { Found = (FReflex*)&R; SetBits = MinSetBits; }
	}
	return Found;
}


//прозвучать "виртуально", чтобы перцепция других существ могла нас услышать
void AMyrAIController::LetOthersNotice (EHowSensed Event, float Strength)
{	UAISense_Hearing::ReportNoiseEvent(me(), me()->GetHeadLocation(), (float)Event + FMath::Max(Strength*0.1f, 0.9f), me(), 0, TEXT("HeardSpecial"));	}

//уведомить систему о прохождении звука ( вызывается из шага, из MyrAnimNotify.cpp / MakeStep)
void AMyrAIController::NotifyNoise(FVector Location, float Strength)
{	UAISense_Hearing::ReportNoiseEvent(me(), Location, Strength, me());		}




