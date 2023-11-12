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
	AIPerception->bEditableWhenInherited = true;

	//настройки слуха, предварительные - они должны меняться при подключении к цели
	ConfigHearing = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Myrra Hearing"));
	if (ConfigHearing)
	{	ConfigHearing->HearingRange = 10000.f;
		auto& DbA = ConfigHearing->DetectionByAffiliation;
		DbA.bDetectEnemies = DbA.bDetectNeutrals = DbA.bDetectFriendlies = true;
		AIPerception->ConfigureSense(*ConfigHearing);
	}
	SetPerceptionComponent(*AIPerception);

	//исключить болванки, не являющиеся реальными объектами
	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	//базовые комплексные эмоции, состоящие из несколькоих элементарных стимулов, которые должны быть у всех
	/*ComplexEmotions.Add(FReflex(Moon, FPathia(Hope, +2)));
	ComplexEmotions.Add(FReflex(YouReBig,			FPathia( Fear,				+2) ));
	ComplexEmotions.Add(FReflex(YouReNewHere,		FPathia( Care,				+5) ));
	ComplexEmotions.Add(FReflex(YouReBigAndNew,		FPathia( Anxiety,			+3) ));
	ComplexEmotions.Add(FReflex(YouReUnreachable,	FPathia( Pessimism,			+2) ));
	ComplexEmotions.Add(FReflex(YouReImportant,		FPathia( Mania,				+2) ));
	ComplexEmotions.Add(FReflex(YouPleasedMe,		FPathia( Care,				+5) ));*/

	//привзять обработчик событий прилёта слуха и навешанных на него событий
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AMyrAIController::OnTargetPerceptionUpdated);
}


//==============================================================================================================
//обработчик занятия объекта (при его появлении или в начале игры/уровня
//==============================================================================================================
void AMyrAIController::OnPossess(APawn * InPawn)
{
	//переименовать, чтобы в отладке в редакторе отображалось имя, ассоциированное с подопечным существом
	FString NewName = InPawn->GetName() + TEXT("-") + GetName();
	Rename(*NewName);
	Super::OnPossess(InPawn);
	UE_LOG(LogMyrAIC, Warning, TEXT("%s possessed"), *GetName());

	//загрузить предопределенные комплексные рефлексы (надо будет ввести в ГеймИнст профили для разных характеров животных и загружать не все
	if (auto GI = GetMyrGameInst())
		for (auto& A : GI->EmoReactionWhatToDisplay)
			ComplexEmotions.Add(FReflex(A.Key, A.Value.DefaultReaction));

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
	if (me())
	{	AIRuleWeight = me()->IsUnderDaemon() ? 0.0f : 1.0f;

		//включить это существо в атмосферу слуха
		UAIPerceptionSystem::RegisterPerceptionStimuliSource(ME(), UAISense_Hearing::StaticClass(), ME());
	}
	Super::BeginPlay();
}


//==============================================================================================================
//событие восприятия - на данный момент, когда ловим звук или спец-сообщение от другого существа
//==============================================================================================================
void AMyrAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;							// на всякий
	if (Actor == me()) return;					// это вообще бред, почему себя нельзя исключить
	if (Actor == me()->Daemon) return;			// это вообще бред, почему себя нельзя исключить
	auto MyrYou = Cast<AMyrPhyCreature>(Actor);

	auto ExactObj = Actor->GetRootComponent();	// выделить из актора конкретный компонент-источник-цель, по умолчанию корневой компонент актора
	if (auto L = Cast<AMyrLocation>(Actor))		// если это локация, то там может быть много манящих объектов
		ExactObj = (USceneComponent*)L->GetCurrentBeacon();		// выдать текущий


	//корректность, если не WasSuccessfullySensed, это, возможно, утеря
	if (Stimulus.WasSuccessfullySensed())
	{
		// радиус-вектор от нас до объекта, по умолчанию до точки начала координат компонента-цели
		auto YouMinusMe = GetMeToYouVector(ExactObj);
		UE_LOG(LogMyrAIC, Log, TEXT("%s AI notices %s"), *me()->GetName(), *Actor->GetName());

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
		auto OldInfluences = MeinGestalt->Influences;

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
				case EHowSensed::ATTACKSTRIKE:	Strike = true;
				case EHowSensed::ATTACKSTART:

					//хз почему, но иногда срабатывает когда атаки нет
					if(MyrYou->CurrentAttack != 255)
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
							MeinGestalt->Influences.Add(EInfluWhy::Attack);
					}
					break;

				case EHowSensed::ATTACKEND:
					MeinGestalt->Influences.Del(EInfluWhy::Attack);
					break;

				case EHowSensed::DIED:
					MeinGestalt->MarkDead();
					break;

				case EHowSensed::REMOVE:
					Memory.Remove(*MeinGestalt);
					break;

				//сигнал подает агрессор, ударивший какое-то существо, возможно нас или нашего друга
				case EHowSensed::PERCUTER:
					{
						//участники событий
						auto YouAI = MyrYou->MyrAIController();	
						auto Victim = Cast<AMyrPhyCreature>(YouAI->ExactVictim->GetOwner());

						//если жертва мы, изменить отношение к агрессору ("Pain" для само-гештальта обрабатывается в тике, а для цели дефакто "сколько раз ты бил меня")
						if(Victim == me())	MeinGestalt->ReactToDamage(MeinGestalt->Chance(Strength, 0.8));

						//если жертва не мы, но мы знаем это существо
						else if(auto pGestalt = MeInYourMemory(Victim))
						{
							//если существо наш давний друг (iFriendliness = 0..1024), зафиксировать воздействие
							if(pGestalt->iFriendliness() + 200 + Strength > (FUrGestalt::RandVar & 1023))
								MeinGestalt->Influences.Add(EInfluWhy::HurtFriend);
						}
					}
					break;

				//когда сигнал подает жертва, имеет смысл когда мы агрессор
				case EHowSensed::PERCUTED:
					break;
				
				//кто-то ударился по собственной тупости
				case EHowSensed::HURT:
					break;
				
				//кто-то выразил конкретную эмоцию
 				case EHowSensed::EXPRESSED:
					break;

				case EHowSensed::CALL_OF_HOME:
					break;
			}
		}
		//обновить эмоции и их энергоценность в отношении объекта-сигнализатора - только если за эту функцию что-то изменилось
		if(OldInfluences.AsNumber() != MeinGestalt->Influences.AsNumber())
			UpdateEmotions(*MeinGestalt, MeinGestalt->GetWeight(), OldInfluences);

	} //else UE_LOG(LogMyrAIC, Log, TEXT("%s AI ignores %s"), *me()->GetName(), *Actor->GetName());
	
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

	//сбросить старую цель в фокусе, чтоб она не висла в случае неполадок
	ME()->SetObjectAtFocus(nullptr);

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
		//сохранить пребыбущий комплекс воздействий с прошлого такта, перед тем как будут зафиксированы новые, нужно для детекции изменений
		auto OldInfluences = Gestalt.Influences;

		//приближение и удаление живого противника
		if (Gestalt.IsCreature())
		{	auto MyrU = Gestalt.Creature();

			//влияние здоровья и усталости этой цели внимания
			Gestalt.Influences.Grade(EInfluHow::InjuredMost,	Gestalt.Chance(0.9 * FMath::Square(0.5f - MyrU->Health), 0.8));
			Gestalt.Influences.Grade(EInfluHow::TiredMost,		Gestalt.Chance(0.9 * FMath::Square(0.5f - MyrU->Stamina), 0.8));

			//насколько мы видны для существа-цели
			auto MeAsYourGestalt = MeInYourMemory(MyrU);
			if(MeAsYourGestalt) Gestalt.Influences.Set(EInfluWhat::YouNoticeMe, MeAsYourGestalt->PerceptAmount());

			//если существо явно двигается
			if (Gestalt.Creature()->GetThorax()->Speed > 10 )
			{
				//определение движения цели относительно нас
				float Coax = -MyrU->GetThorax()->VelDir | Gestalt.LookDir();	// вектор скорости в проекции на вектор нашего взгляда,
					  Coax *= (0.001 + Gestalt.HowClose());						// учет дальности, когда цель далеко, непонятно, бежит она к нам или от нас
					  Coax *= MyrU->GetThorax()->Speed * 0.01;					// учет скорости цели, более быстрая цель даже издалеко понятна в своем направлении
				Gestalt.Influences.Set(EInfluWhat::YouComingCloser, Coax * 3);	// Coax больше  0.333 округляется до единицы, тру, ставится флаг
				Gestalt.Influences.Set(EInfluWhat::YouComingAway, - Coax * 3);	// Coax меньше -0.333 округляется до единицы, тру, ставится флаг
			}
		}

		//рассчет объекта в фокусе для помощи в нацеливании (нахождение максимального соноправления)
		if(Gestalt.VisCoaxis > MaxCoax) { ME()->SetObjectAtFocus(&Gestalt); MaxCoax = Gestalt.VisCoaxis; }

		//если запись памяти указывает на реальный объект
		if(Gestalt.Obj.IsValid())
		{
			//найти самую влекущую и самую противную цель по уровню тяги
			//вместе с внесением пересчитать локацию кандидата в цели на текущий такт
			auto Gain = Gestalt.GetAnalogGain();
			GoalMaxLure.SeizeIf(Gestalt, Gain, true);
			GoalMinLure.SeizeIf(Gestalt, Gain, false);
		}

		//перевалить отношение к образу за очередной такт
		UpdateEmotions(Gestalt, Gestalt.GetWeight(), OldInfluences);
		Extraversy += Gestalt.Emotion.UniPower() * Gestalt.GetWeight();
	}

	//сохранить старые воздействия перед массовым внесением новых
	auto OldInfluences = Mich.Influences;

	//накатить поводы для внутренних эмоций
	Mich.Influences.Grade(EInfluHow::PainMost,		Mich.Chance(FMath::Min(1.0f, me()->Pain * 0.5), 0.9));
	Mich.Influences.Grade(EInfluHow::InjuredMost,	Mich.Chance(0.9 * FMath::Square(0.5f - me()->Health), 0.8));
	Mich.Influences.Grade(EInfluHow::TiredMost,		Mich.Chance(0.9 * FMath::Square(0.5f - me()->Stamina), 0.8));
	Mich.Influences.Set(EInfluWhat::MeHurtHead,		Mich.Chance(me()->GetDamage(ELimb::HEAD), 0.8));
	Mich.Influences.Set(EInfluWhat::MeHurtTorso,	Mich.Chance(me()->GetDamageTorso(), 0.8));
	Mich.Influences.Set(EInfluWhat::MeHurtLeg,		Mich.Chance(me()->GetDamageLegs(), 0.8));

	if(me()->CurrentState == EBehaveState::fall)
		Mich.Influences.Set(EInfluWhy::Falling,		me()->GetThorax()->VelocityAtFloor().Z * 0.01);

	//скоростной характер движения (тут бы не цифры, а какую-то комфортную скорость для данного классса существ)
	Mich.Influences.Set(EInfluWhat::MeMovingFast,	me()->GetThorax()->Speed > 150);
	Mich.Influences.Set(EInfluWhat::MeMovingSlow,	me()->GetThorax()->Speed < 50);
	Mich.Influences.Set(EInfluWhat::MeMovingBack,	me()->GetThorax()->Speed > 10 && (me()->GetThorax()->VelDir | me()->GetAxisForth()) < 0 );

	//самые общие для всех воздействия + финальная обработка их, дрейф значений эмоции
	UpdateEmotions(Mich, 1 - FMath::Min(Extraversy, 1.0f), OldInfluences);

	//если самая передняя цель не очень спереди, то передних целей нет вообще
	if(MaxCoax < 0.8) ME()->SetObjectAtFocus(nullptr);

	//пока неясно как плавно или остро отсекать по силе ИИ, но пока для протагониста все остальное не нужно
	if (AIRuleWeight == 0) return;

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

		//просто бежать от цели
		else GoalMaxLure.WhatToDo = FreeMoveAway(GoalMaxLure);

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
	LastRouteResult = PriGoal.WhatToDo == ERouteResult::NoGoal ? SecGoal.WhatToDo : PriGoal.WhatToDo;

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
	//проходим счетчиком дальше
	ActionToTryNow++;
	if (ActionToTryNow >= me()->GetGenePool()->Actions.Num()) ActionToTryNow = 0;

	//выудили реальное действие
	auto A = me()->GetGenePool()->Actions[ActionToTryNow];

	//тестируем применимость
	auto R = A->IsActionFitting(me(), Goal.pGestalt->Obj.Get(), Goal.Dist(), Goal.LookAtDir() | me()->AttackDirection, true, true);

	//если применимо, запускаем
	if(ActOk(R)) me()->ActionStart(ActionToTryNow);
	else UE_LOG(LogMyrAIC, Log, TEXT("%s ProposeAction %s towards %s result %s"), *me()->GetName(), *A->GetName(), *Goal.pGestalt->Obj->GetOwner()->GetName(), *TXTENUM(EResult, R));

}

//==============================================================================================================
//рутинная обработка в тике одного конкретного гештальта
//==============================================================================================================
void AMyrAIController::UpdateEmotions(FUrGestalt& Gestalt, float Weight, int32 OldInfluences)
{
	if(Weight < 0.01) return;					// совсем пассивный гештальт не должен обновляться
	uint8 Limit = Weight * 16;					// гештальты со слабым весом меньше обрабатывают эмоции (ограничить 0-16)

	//обновить стимулы внешней среды, одинаковые как для себя, так и для внешних гештальтов
	Gestalt.Influences.Set(EInfluWhere::Night,	Gestalt.Chance(0.5 * GetMyrGameMode()->NightAmount(), 0.8));
	Gestalt.Influences.Set(EInfluWhere::Shine,	Gestalt.Chance(0.5 * (GetMyrGameMode()->MoonIntensity() + GetMyrGameMode()->SunIntensity()), 0.8));
	Gestalt.Influences.Set(EInfluWhere::Rain,	Gestalt.Chance(0.9 * (me()->Daemon ? me()->Daemon->RainAmount : GetMyrGameMode()->RainAmount()), 0.8));
	Gestalt.Influences.Set(EInfluWhere::Fog,	Gestalt.Chance(0.5 * GetMyrGameMode()->FogAmount(), 0.8));
	Gestalt.Influences.Set(EInfluWhere::Cold,	Gestalt.Chance(0.5 * FMath::Square(1 - GetMyrGameMode()->Temperature()), 0.8));
	Gestalt.Influences.Set(EInfluWhere::Indoor, (bool)me()->IsInLocation());

	//если вообще никаких воздействий, то ничего не нужно считать, но можно скатиться в душевную пустоту
	if (Gestalt.Influences == 0)
	{	RegModEmotion(Gestalt.Emotion.Affect(FPathia(Void), 1));//◘◘>
		return;
	}

	//попытаться найти в памяти именно строго такое сочетание воздействий
	FReflex* pReflex		= ComplexEmotions.Find (Gestalt.Influences);
	FReflex* pReflexLatent  = ComplexEmotions.Find (Gestalt.Influences.With(EInfluWhy::Latent));

	//если рефлекс уже разблокирован, применить именно комплексный рефлекс
	if (pReflex)
		RegModEmotion(Gestalt.Emotion.Affect(pReflex->Emotion, Limit));//◘◘>

	//если полный рефлекс не найден, но найдена его латентная версия
	else if (pReflexLatent)
	{
		//если произошло реальное изменение с прошлого кадра
		if (OldInfluences != Gestalt.Influences)	
		{
			// усилить его, скоро можно будет раздлокировать
			pReflexLatent->Emotion.Work++;						
																
			// ну а пока он латентен, время подгадать, с какой эмоцией он выйдет в свет, начав с самых похожих на него
			auto ClosestR = FindClosest(Gestalt.Influences);
			
			// если ближайший найден и он сам не латентный
			if (ClosestR && !ClosestR->IsLatent())				
			{
				//сначала подмешать к текущему латентному щепотку самого похожего
				pReflexLatent->Emotion.Affect(ClosestR->Emotion, 4);
				RegModEmotion(Gestalt.Emotion.Affect(ClosestR->Emotion, Limit));//◘◘>
			}
			//вообще не найдено похожих, что странно, видимо, множество как-то оказалось пустым
			else
			{}
		}

		//если счетчик случаев достиг порога, пора разблокировать новый рефлекс
		if (pReflexLatent->Emotion.Work > 230)
		{
			pReflexLatent->Emotion.Work = 128;		// убрать значение счетчика, теперь он не нужен, поставить начальную работу среднюю по диапазону, чтобы и туда и сюда
			FReflex UnLatent(						// создать новую болванку, теперь уже нелатентного
				Gestalt.Influences,					// влияния изначальные, без бита латентности
				pReflexLatent->Emotion);			// эмоция выстраданная пока рефлекс был латентным
			ComplexEmotions.Add(UnLatent);			// добавить во множество
			ComplexEmotions.Remove(*pReflexLatent);	// теперь латентная болванка не нужна, удалить ее
		}

	}
	//если такой рефлекс не найден в памяти, это первый опыт данного воздействия 
	else
	{
		//перебрать и применить все элементарные воздействия поочередно
		uint8 NumInfls = 0;
		for(auto E : ComplexEmotions)
			if(Gestalt.Influences.Contains(E.Condition))
			{	NumInfls++;
				RegModEmotion(
					Gestalt.Emotion.Affect(E.Emotion, Limit));//◘◘>
			}

		//создаем для него болванку "латентную", которая не будет действовать на эмоции,
		//пока не встречена в жизни достаточное число раз, она же будет перенимать отклики реальных, похожих на нее сочитаний
		ComplexEmotions.Add(FReflex(Gestalt.Influences.With(EInfluWhy::Latent), Gestalt.Emotion));
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
	auto Alive = Cast<AMyrPhyCreature>(C->GetOwner());
	auto DestPoint = (Alive ? Alive->GetVisEndPoint() : C->GetComponentLocation());
	return VF ( DestPoint - me()->GetHeadLocation() );
}

//==============================================================================================================
//посмотреть на объект, определить его видимость, либо, если объект не указан, посмотреть в опр. место и поискать прерпятствия
//==============================================================================================================
bool AMyrAIController::See(FHitResult& VisHit, FVector ExplicitPos, USceneComponent* NeededObj)
{
	FCollisionQueryParams RV_TraceParams(TEXT("AI_Watch"), false, me());	// трассировать без полигонов и без нас
	RV_TraceParams.AddIgnoredActor(this);									// если к демону привязано что-то трассируемое, игнорировать 
	if(NeededObj)
	{	auto YouCre = Cast<AMyrPhyCreature>(NeededObj->GetOwner());			// если объект существо
		ExplicitPos = YouCre ? YouCre->GetVisEndPoint()						// из головы глянуть на случайную часть тела
				: NeededObj->GetComponentLocation();						// иначе просто в центр координат объекта
		RV_TraceParams.AddIgnoredComponent((UPrimitiveComponent*)NeededObj);// сам целевой объект тоже игнорировать, чтобы виделось 
	}
	RV_TraceParams.bReturnPhysicalMaterial = true;							// не факт что нужно
	
	//запуск
	GetWorld()->LineTraceSingleByChannel (VisHit, me()->GetHeadLocation(), ExplicitPos, ECC_WorldStatic, RV_TraceParams);
	if (VisHit.bBlockingHit) ExplicitPos = VisHit.ImpactPoint;
	LINEWT(AILookDir, me()->GetHeadLocation(), ExplicitPos - me()->GetHeadLocation(), 1, VisHit.bBlockingHit ? 1.0f : 0.5f);

	//"НЕ" потому что, если что-то увидел, значит это препятствие на пути к цели или на пути к точке, если не наткнулся - значит видит
	return !VisHit.bBlockingHit;
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

	//нормаль в точке препятствия в плоскости горизонта
	FVector3f ObstNormal = VF(Hit->ImpactNormal) * FVector3f(1, 1, 0);

	FVector3f ByPassDir;
	if (Hit->Distance < me()->GetBodyLength())				// если почти уткнулись в стену и надо срочно отходить
	{	if ((TempLookDir | ObstNormal) > -0.5)				// если в стену мы смотрим довольно косо, под острым углом, есть возможность мирно обойти по кромке
		{	ByPassDir = ObstNormal ^ me()->GetUpVector();	// вдоль стены
			if ((ByPassDir | TempLookDir) < 0)				// если вдоль стены от целевого вектора
				ByPassDir = -ByPassDir;						// развернуть, чтоб в оль стены прибилижало, а не удаляло от цели
		}
		else ByPassDir = ObstNormal;						//если в стену утыкаемся почти под прямым углом, то надо идти назад по нормали
	}
	else													//если до стены еще далеко
	{
		ByPassDir = TempLookDir ^ me()->GetUpVector();		// радикальный вектор обхода в сторону - тупо вбок от вектора куда смотрим
		if ((ByPassDir|ObstNormal) < 0)						// если вектор вбок навстречу нормали, то это, видимо, курс на стык с препятствием, просто с другой стороны, поэтому инвертировать
			ByPassDir = -ByPassDir;							// просто с другой стороны, поэтому инвертировать
	}

	//идти в сторону от курса на расстояние ширины препятствия и там зафиксировать точку цели
	FVector ClosestPoint = Hit->Location + ByPassDir * Thickness;

	//новый курс как вектор из лица в точку
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
	FHitResult HitSeen(ForceInit);
	bool Seen = G.pGestalt->VisCoaxis > 0 ? See(HitSeen, FVector(0), G.pGestalt->Obj.Get()) : false;
	G.pGestalt->Influences.Set(EInfluWhat::YouSeen, Seen);
	G.pGestalt->Visibility += 0.5f;
	
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
		G.pGestalt->Influences.Add(EInfluWhat::YouUnreachable);
		return ERouteResult::GiveUp_For_Unreachable;//◘◘>
	}
	//если цель в физическом доступе, но не видна, обогнуть препятствие, которое его загораживает
	else if(HitSeen.Component.IsValid()) return SimpleWalkAroundObstacle(G.MoveToDir, &HitSeen, G);//◘◘>

	//если цель не высоко или не существо, идем прямо на него
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
				InfluentGoal->LookAtDir()	* InfluentGoal->AnalogGain;
	}
	Away *= MainGoal.Dist()*3;

	//провести визуальную оценку, 
	FHitResult HitSaw(ForceInit);
	bool Saw = false;

	//если смотрим на угрозу, то бежим назад не глядя, 
	if(MainGoal.pGestalt->VisCoaxis > 0.4)
	{	Saw = See(HitSaw, FVector(0), MainGoal.pGestalt->Obj.Get());
		MainGoal.pGestalt->Influences.Set(EInfluWhat::YouSeen, Saw);
	}

	//вне зависимости от результатов оглядывания на цель визуально детектируем препятсвие
	HitSaw = FHitResult(ForceInit);
	Saw = See(HitSaw, me()->GetHeadLocation() + (FVector)Away);
	
	//если мы смотрели прочь от цели и увидели препятствие, постараться его обойти
	if(HitSaw.Component.IsValid())
		return SimpleWalkAroundObstacle(Away, &HitSaw, MainGoal);//◘◘>

	//просто устремиться по выбранному вектору прочь от угрозы
	//впереди на много метров никаких препятствий
	else Away.ToDirectionAndLength(MainGoal.MoveToDir, MainGoal.FirstDist);//◘◘>
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




