#include "MyrActionInfo.h"
#include "..\Creature\MyrPhyCreature.h"
#include "..\Control\MyrAIController.h"
#include "..\Control\MyrDaemon.h"
#include "..\Myrra.h"
#include "Animation/AnimSequenceBase.h"			//для расчёта длины и закладок анимации
#include "..\Creature\Animation\MyrAnimNotify.h"

//==============================================================================================================
//подходит ли данное действие для выполнения данным существом
// параметр говорит о намеренности игрока совершить команду - в этом случае многие условия не действуют
// наоборот, дла ИИ вводятся дополнительные условия
//==============================================================================================================
EResult FActionCondition::IsFitting (AActor* AnyOwner, bool CheckByChance)
{
	auto Owner = Cast<AMyrPhyCreature>(AnyOwner);

	//если действие вызывается не над существом, проверка применимости идет по упрощенному стценрию
	if (!Owner)
	{
		if (Actors.Contains(AnyOwner->StaticClass()))
			if (StatesForbidden) return EResult::WRONG_ACTOR;//◘◘>
		if (!Velocities.Contains(AnyOwner->GetVelocity().Size()))
			return EResult::OUT_OF_VELOCITY;//◘◘>
		return EResult::OKAY_FOR_NOW;//◘◘>
	}

	// проверка по вероятности - в начале, так как быстро освобождает от более сложных условий
	// отключается для команд демона и особых событий в ИИ, включается для Idle-перебора-чо-бы-устроить
	if (CheckByChance)
	{
		uint8 WhatToTakeAsChance = 255;
		if (Owner->IsUnderDaemon())
			WhatToTakeAsChance = FMath::Lerp(ChanceForPlayer, Chance, Owner->MyrAIController()->AIRuleWeight);
		else WhatToTakeAsChance = Chance;
		if (!Owner->MyrAIController()->ChanceRandom(WhatToTakeAsChance))
		{
			//UE_LOG(LogTemp, Warning, TEXT("%s chance %d > %d"), *Owner->GetName(), Owner->MyrAI()->RandVar&255, WhatToTakeAsChance);
			return EResult::OUT_OF_CHANCE;//◘◘>
		}

		//если общее, интегральное эмоциональное настроение не вписывается в заданный круг
		//эмоции - та же вероятность не хотеть, если приказывают строго выполнить, то они не рассматриваются
		if(Owner->MyrAIController()->IntegralEmotion.Within(EmotionLim1, EmotionLim2))
			return EResult::OUT_OF_SENTIMENT;//◘◘>
	}

	// попадает в диапазоны 
	if(Owner->Daemon)
		if (!Sleepinesses.Contains(Owner->Daemon->Sleepiness))
			return EResult::OUT_OF_SLEEPINESS;//◘◘>


	// если нельзя при произнесении звуков ртом
	if (SkipDuringSpeaking && Owner->CurrentSpelledSound != (EPhoneme)0)			
		return EResult::OUT_OF_STAYING_MUTE;//◘◘>

	// если нельзя во время релакс-действия
	if (SkipDuringRelax && Owner->CurrentRelaxAction != 255)
		return EResult::FORBIDDEN_IN_RELAX;//◘◘>

	// если выполняется атака, а при атаке нельзя - не делать 
	if (SkipDuringAttack && !Owner->NoAttack())	
		return EResult::WRONG_PHASE;//◘◘>

	//это действие с низким приоритетом хочет вытолкнуть более приоритетоное
	if(Owner->DoesSelfAction())
		if(Priority <= Owner->GetSelfAction()->Condition.Priority)
			return EResult::LOW_PRIORITY;//◘◘>

	///////////////////////////////////////////////////

	// попадает в диапазоны  усталости
	if (!Staminas.Contains(Owner->Stamina))
		return EResult::OUT_OF_STAMINA;//◘◘>

	// попадает в диапазоны здоровья
	if (!Healthes.Contains(Owner->Health))
		return EResult::OUT_OF_HEALTH;//◘◘>

	// попадает в диапазоны длительностей текущего состояния (хз зачем)
	if (!StateAges.Contains(Owner->StateTime))
		return EResult::OUT_OF_HEALTH;//◘◘>

	// попадает в диапазоны скорости 
	// ура, теперь можно различать ход вперед и задний ход! 
	if (!Velocities.Contains(Owner->GetPelvis()->SpeedAlongFront()))
		return EResult::OUT_OF_VELOCITY;//◘◘>

	//отсечение по дозволенным/запрещенным состояниям поведения существа
	if(States.Contains(Owner->GetBehaveCurrentState()) == StatesForbidden)
		return EResult::OUT_OF_BEHAVE_STATE;//◘◘>;

	///////////////////////////////////////////////////////////////

	//если не убились об предыдущие условия, значит всё нормально
	return EResult::OKAY_FOR_NOW;
}



//==============================================================================================================
//форма проверки для ИИ, стоит ли совершать данную атаку по отношению к данной цели
//==============================================================================================================
EResult UMyrActionInfo::IsActionFitting(
	AMyrPhyCreature* Owner,
	USceneComponent* ExactVictim,
	float LookAtDist, float LookAlign, bool ByAI, bool CheckByChance)
{
	//проверять ли себя = не нужно, если уже начал, и нужно решить о продолжении
	bool CheckMyself = true;
	EResult R = EResult::INCOMPLETE_DATA;

	if(Owner->MyrAIController()->AIRuleWeight > MinAIWeight)
		return EResult::TOO_WEAK_AI;//◘◘>

	//если имеются сборки атак
	if (VictimTypes.Num() > 0)
	{
		//если какая-то атака уже выполняется - заартачиться, даже если это мы выполняемся
		//ибо данную функцию следует запускать только при тестировании еще не начавшегося действия
		if (Owner->DoesAttackAction()) return EResult::WRONG_PHASE;//◘◘>

		//проверить применимость этой сборки к этой жертве (шанс не проверяем, потому что только для субъекта)
		//внимание, CheckByChance для жертвы не осуществляется, потому что если жертва = игрок, то функция будет думать, что 
		//совершает проверку для игрока, а для игрока почти все самопроизвольные действия запрещены
		//и вообще второй раз зачем вероятность считать
		R = VictimTypes[0].IsVictimFitting (Owner, ExactVictim, LookAtDist, LookAlign, false, ByAI);
		if (ActOk(R))
		{
			//если жертва подходит - проверить себя и сразу выдать вердикт
			if (CheckMyself) R = Condition.IsFitting(Owner, CheckByChance);
			return R; //◘◘>
		}

		//не нашли подходящую сборку - выдать последнюю причину неподходящести
		return R;//◘◘>
	}

	//для самодействия все просто - проверка только себя
	return Condition.IsFitting(Owner, CheckByChance);//◘◘>
}

//==============================================================================================================
//упрощенный вариант перетестирования для ИИ перед страйком
//==============================================================================================================
EResult UMyrActionInfo::RetestForStrikeForAI(class AMyrPhyCreature* Owner, UPrimitiveComponent* ExactVictim, float Dist, float Ang)
{
	//проверяем применимость к той жертве, которая была изначально вбита при начале атаки
	auto R = VictimTypes[0].IsVictimFitting (Owner, ExactVictim, Dist, Ang, true, true);

	//здесь шанс нехрена проверять, если начали, так уж и заканчивать
	if (ActOk(R)) R = Condition.IsFitting(Owner, false);
	return R; //◘◘>
}

//==============================================================================================================
//рассчитать время, которое займёт прокат ролика между данными фазами
//==============================================================================================================
float UMyrActionInfo::PhaseTimes(TArray<float>& OutTimes)
{
	if (!DynModelsPerPhase.Num()) return -1;
	if (Motion->Notifies.Num() < DynModelsPerPhase.Num() - 1) return -1;

	OutTimes.SetNum(DynModelsPerPhase.Num());

	float TimeAcc = 0.0f;
	float LngAcc = 0.0f;

	for (int i = 0; i < DynModelsPerPhase.Num(); i++)
	{
		float CurLng = Motion->Notifies.Num() > i ? Motion->Notifies[i].GetTime() : Motion->GetPlayLength();
		OutTimes[i] = (CurLng - TimeAcc) * DynModelsPerPhase[i].AnimRate;
		LngAcc += OutTimes[i];
		TimeAcc = CurLng;
	}
	return LngAcc;
}


//==============================================================================================================
//пересчитать энергоемкость действия
//==============================================================================================================
void UMyrActionInfo::RecalcOverallStaminaFactor()
{
	OverallStaminaFactor = 0;
	float TimeAccum = 0.0f;
	if (DynModelsPerPhase.Num() == 0) return;

	//случай атаки с чётким числом фаз
	else if (VictimTypes.Num() > 0)
	{
		//тут непонятно, как сортированы закладки поэтому нах такую сложность
		for (int i = 0; i < Motion->Notifies.Num(); i++)
			OverallStaminaFactor += DynModelsPerPhase[i].StaminaAdd;

	}
	else if (Motion->Notifies.Num() >= DynModelsPerPhase.Num())
	{
		//считаем, что закладки соответствуют фазам
		for (int i = 0; i < Motion->Notifies.Num(); i++)
		{
			//считаем только правильные закладки
			if (auto MN = Cast<UMyrAnimNotify>(Motion->Notifies[i].Notify))
			{
				//удельный приток стамины в секунду на длины текущего участка в секундах = оценочное изменение стамины за это время
				OverallStaminaFactor += DynModelsPerPhase[i].StaminaAdd * (Motion->Notifies[i].GetTime() - TimeAccum);
				TimeAccum = Motion->Notifies[i].GetTime();
			}
		}
	}
}


//==============================================================================================================
//проверка возможности попадения в цель, для автонацеливания
//==============================================================================================================
bool UMyrActionInfo::QuickAimResult(AMyrPhyCreature* Owner, FGestalt Gestalt, float Accuracy)
{
	auto VicTy = VictimTypes[0];

	//если жертва вне радиуса доставания атаки, не надо ее начинать
	if(	!VicTy.CheckByRadius (Gestalt.Location.DecodeDist())) return false;//◘◘>

	//если жертва вне угла поражения
	if(	!VicTy.CheckByAngle (Gestalt.VisCoaxis)) return false;//◘◘>

	//если мы попадаем, но нацелены неточно (и помогать нам при такой точности - читерство)
	if(Gestalt.VisCoaxis < Accuracy)  return false;//◘◘>
	return true;//◘◘>
}


//==============================================================================================================
//реакция на добавление жертвы - превращение действия из самодействия в атаку
//==============================================================================================================
#if WITH_EDITOR
void UMyrActionInfo::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UMyrActionInfo, VictimTypes))
	{
		//если пользователь добавил сборку для жертвы из нуля хотя бы одну
		if (VictimTypes.Num() > 0 && DynModelsPerPhase.Num() < (int)EActionPhase::NUM)
		{
			DynModelsPerPhase.SetNum((int)EActionPhase::NUM);
			DynModelsPerPhase[(int)EActionPhase::READY].AnimRate = -0.1f;
			DynModelsPerPhase[(int)EActionPhase::DESCEND].AnimRate = -1.0f;
		}
	}

	//проверить, что для атаки фазы предусматривающие откат анимации поданы с отрицательной скоростью
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UMyrActionInfo, DynModelsPerPhase) && IsAttack())
	{
		for (int i = 0; i < DynModelsPerPhase.Num(); i++)
		{
			if ((EActionPhase)i == EActionPhase::DESCEND && DynModelsPerPhase[i].AnimRate > 0)
				DynModelsPerPhase[i].AnimRate = -1.0f;
			if ((EActionPhase)i == EActionPhase::READY && DynModelsPerPhase[i].AnimRate > 0)
				DynModelsPerPhase[i].AnimRate = -0.1f;
		}
	}

	//пересчитать энергетическую стоимость действия
	RecalcOverallStaminaFactor();

	//пересчитать обороноспособность действия
	RecalcMaxDamage();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

