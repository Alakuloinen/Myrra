#include "MyrCreatureAttackInfo.h"
#include "..\Creature\MyrPhyCreature.h"
#include "..\Control\MyrAI.h"
#include "..\Myrra.h"
#include "Animation/AnimSequenceBase.h"			//для расчёта длины и закладок анимации
#include "..\Creature\Animation\MyrAnimNotify.h"

//==============================================================================================================
//подходит ли данное действие для выполнения данным существом
// параметр говорит о намеренности игрока совершить команду - в этом случае многие условия не действуют
// наоборот, дла ИИ вводятся дополнительные условия
//==============================================================================================================
EAttackAttemptResult FActionCondition::IsFitting (class AMyrPhyCreature* Owner, bool CheckByChance)
{
	// проверка по вероятности - в начале, так как быстро освобождает от более сложных условий
	// отключается для команд демона и особых событий в ИИ, включается для Idle-перебора-чо-бы-устроить
	if (CheckByChance)
	{
		uint8 WhatToTakeAsChance = 255;
		if (Owner->IsUnderDaemon())
			WhatToTakeAsChance = FMath::Lerp(ChanceForPlayer, Chance, Owner->MyrAI()->AIRuleWeight);
		else WhatToTakeAsChance = Chance;
		if (!Owner->MyrAI()->ChanceRandom(WhatToTakeAsChance))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s chance %d > %d"), *Owner->GetName(), Owner->MyrAI()->RandVar&255, WhatToTakeAsChance);
			return EAttackAttemptResult::OUT_OF_CHANCE;//◘◘>
		}

		//если общее, интегральное эмоциональное настроение не вписывается в заданный круг
		//эмоции - та же вероятность не хотеть, если приказывают строго выполнить, то они не рассматриваются
		if (!FeelToCause.Within(Owner->MyrAI()->GetIntegralEmotion()))
			return EAttackAttemptResult::OUT_OF_SENTIMENT;//◘◘>
	}

	// если нельзя при произнесении звуков ртом
	if (SkipDuringSpeaking && Owner->CurrentSpelledSound != (EPhoneme)0)			
		return EAttackAttemptResult::OUT_OF_STAYING_MUTE;//◘◘>

	// если нельзя во время релакс-действия
	if (SkipDuringRelax && Owner->CurrentRelaxAction != 255)
		return EAttackAttemptResult::FORBIDDEN_IN_RELAX;//◘◘>

	// если выполняется атака, а при атаке нельзя - не делать 
	if (SkipDuringAttack && !Owner->NoAttack())	
		return EAttackAttemptResult::WRONG_PHASE;//◘◘>

	///////////////////////////////////////////////////

	// попадает в диапазоны  усталости
	if (!Staminas.Contains(Owner->Stamina))
		return EAttackAttemptResult::OUT_OF_STAMINA;//◘◘>

	// попадает в диапазоны здоровья
	if (!Healthes.Contains(Owner->Health))
		return EAttackAttemptResult::OUT_OF_HEALTH;//◘◘>

	// попадает в диапазоны здоровья
	if (!Sleepinesses.Contains(Owner->Sleepiness))
		return EAttackAttemptResult::OUT_OF_SLEEPINESS;//◘◘>

	// попадает в диапазоны длительностей текущего состояния (хз зачем)
	if (!StateAges.Contains(Owner->StateTime))
		return EAttackAttemptResult::OUT_OF_HEALTH;//◘◘>

	// попадает в диапазоны скорости 
	// ура, теперь можно различать ход вперед и задний ход! 
	if (!Velocities.Contains(Owner->GetPelvis()->SpeedAlongFront()))
		return EAttackAttemptResult::OUT_OF_VELOCITY;//◘◘>

	//отсечение по дозволенным/запрещенным состояниям поведения существа
	if(States.Contains(Owner->GetBehaveCurrentState()) == StatesForbidden)
		return EAttackAttemptResult::OUT_OF_BEHAVE_STATE;//◘◘>;

	///////////////////////////////////////////////////////////////

	//если не убились об предыдущие условия, значит всё нормально
	return EAttackAttemptResult::OKAY_FOR_NOW;
}

//==============================================================================================================
//подходит ли данное данное существо на роль жертвы данной атаки
//==============================================================================================================
EAttackAttemptResult FVictimType::IsVictimFitting(AMyrPhyCreature* Owner, UPrimitiveComponent* VictimBody, bool CheckByChance, FGoal* Goal, bool ByAI)
{
	//наслучай если будет вызвана в отсутствие целей
	if (!VictimBody) return EAttackAttemptResult::WRONG_ACTOR;//◘◘>

	//если эта функция вызывается из ИИ - думать за ИИ о применимости в пространстве
	//для игрока вызывать атаку в любом случае, даже если это тупо об воздух
	if (ByAI)
	{	if (Goal)
		{	if (!CheckByRadius(Goal->LookAtDist))
				return EAttackAttemptResult::OUT_OF_RADIUS;//◘◘>
			if (!CheckByAngle(Goal->LookAlign))
				return EAttackAttemptResult::OUT_OF_ANGLE;//◘◘>
		}
		//тут ничего не делается, так как ИИ не атакует не-цели
		else { }
	}

	//если актор, на коего возложили сие действие, в списке особых 
	//это нужно только для жертвы
	if (Condition.Actors.Contains(Owner->StaticClass()) == Condition.ActorsForbidden)
		return EAttackAttemptResult::WRONG_ACTOR;//◘◘>


	//если в остальном проходим - придётся запустить проверку жертвы как отдельной сущности
	if (auto VictimAlive = Cast<AMyrPhyCreature>(VictimBody->GetOwner()))
		return Condition.IsFitting(VictimAlive, CheckByChance);
	else return Condition.IsFittingSimple(VictimBody->GetOwner());
}

//==============================================================================================================
//форма проверки для ИИ, стоит ли совершать данную атаку по отношению к данной цели
//==============================================================================================================
EAttackAttemptResult UMyrActionInfo::IsActionFitting(
	AMyrPhyCreature* Owner,
	UPrimitiveComponent* ExactVictim,
	FGoal* Goal, uint8& VictimType, bool ByAI, bool CheckByChance)
{
	//проверять ли себя = не нужно, если уже начал, и нужно решить о продолжении
	bool CheckMyself = true;
	EAttackAttemptResult R = EAttackAttemptResult::INCOMPLETE_DATA;

	//если имеются сборки атак
	if (VictimTypes.Num() > 0)
	{
		//если какая-то атака уже выполняется - заартачиться, даже если это мы выполняемся
		//ибо данную функцию следует запускать только при тестировании еще не начавшегося действия
		if (Owner->DoesAttackAction()) return EAttackAttemptResult::WRONG_PHASE;//◘◘>

		//по всем, хотя здесь скорее всего будет один элемент
		for (int i=0;i<VictimTypes.Num();i++)
		{
			//проверить применимость этой сборки к этой жертве (шанс не проверяем, потому что только для субъекта)
			//внимание, CheckByChance для жертвы не осуществляется, потому что если жертва = игрок, то функция будет думать, что 
			//совершает проверку для игрока, а для игрока почти все самопроизвольные действия запрещены
			//и вообще второй раз зачем вероятность считать
			R = VictimTypes[i].IsVictimFitting (Owner, ExactVictim, false, Goal, ByAI);
			if (ActOk(R))
			{
				//если жертва подходит - проверить себя и сразу выдать вердикт
				if (CheckMyself) R = Condition.IsFitting(Owner, CheckByChance);
				VictimType = i;
				return R; //◘◘>
			}
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
EAttackAttemptResult UMyrActionInfo::RetestForStrikeForAI(class AMyrPhyCreature* Owner, UPrimitiveComponent* ExactVictim, FGoal* Goal)
{
	//проверяем применимость к той жертве, которая была изначально вбита при начале атаки
	auto R = VictimTypes[Owner->CurrentAttackVictimType].IsVictimFitting (Owner, ExactVictim, true, Goal, true);

	//здесь шанс нехрена проверять, если начали, так уж и заканчивать
	if (ActOk(R)) R = Condition.IsFitting(Owner, false);
	return R; //◘◘>
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
bool UMyrActionInfo::QuickAimResult(AMyrPhyCreature* Owner, FGoal* Goal, float Accuracy)
{
	if(Owner->CurrentAttackVictimType >= VictimTypes.Num()) return false;//◘◘>
	auto VicTy = VictimTypes[Owner->CurrentAttackVictimType];

	//если жертва вне радиуса доставания атаки, не надо ее начинать
	if(	!VicTy.CheckByRadius (Goal->LookAtDist)) return false;//◘◘>

	//если жертва вне угла поражения
	if(	!VicTy.CheckByAngle (Goal->LookAlign)) return false;//◘◘>

	//если мы попадаем, но нацелены неточно (и помогать нам при такой точности - читерство)
	if(Goal->LookAlign < Accuracy)  return false;//◘◘>
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

		//если пользователь добавил сборку для жертвы из нуля хотя бы одну
		if (VictimTypes.Num() > 0 && DynModelsPerPhase.Num() < (int)EActionPhase::NUM)
		{
			DynModelsPerPhase.SetNum((int)EActionPhase::NUM);
			DynModelsPerPhase[(int)EActionPhase::READY].AnimRate = -0.1f;
			DynModelsPerPhase[(int)EActionPhase::DESCEND].AnimRate = -1.0f;
		}

	RecalcOverallStaminaFactor();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

//==============================================================================================================
//форма проверки для ИИ, стоит ли совершать даннуб атаку по отношению к данной цели
//==============================================================================================================
EAttackAttemptResult UMyrAttackInfo::ShouldIStartThisAttack (
	AMyrPhyCreature* Owner,		// существо, которое хочет атаковать этой атакой
	UPrimitiveComponent* ExactVictim,		// жертва существа, определенная ИИ, при игре игроком это может быть и 0
	FGoal* Goal,				// сборка цели ИИ, может быть и при атаке игроком, если включен режим помощи в нацеливании
	bool ByAI)					// дополнительный флаг, что совершается именно ИИ, хотя зачем он нужен?
{
	//проверять отдельно, подходит ли сам деятель (для оптимизации)
	//не нужно, если он уже начал, и нужно решить о продолжении
	bool CheckMyself = true;

	/*//если уже какая-то атака выполняется
	if(!Owner->NoAttack())
	{
		//именно эта атака сейчас выполняется
		if(Owner->GetCurrentAttackInfo() == this)
		{
			//в какой фазе уже находится наша атака
			switch(Owner->CurrentAttackPhase)
			{
				//в начальной фазе - хорошо, себя можно не проверять, только жертву
				case EActionPhase::ASCEND:	
				case EActionPhase::READY:	
				case EActionPhase::RUSH:
					CheckMyself = false;
					break;

				//если в этой атаке предусмотрено несколько быстрых повторов, и они еще не исчерпаны
				//то продолжить проверку, но не избегать проверки исполнителя, иначе досрочно прекратить
				case EActionPhase::STRIKE:
					if(NumInstantRepeats >= Owner->CurrentAttackRepeat)
						return EAttackAttemptResult::OUT_OF_REPEATS;//◘◘>
					else break;

				//фаза заката, всё, поезд ушёл, только дожидаться, когда атака кончится
				case EActionPhase::DESCEND: return EAttackAttemptResult::WRONG_PHASE;//◘◘>
			}
		}
		//выполняется какое-то другое действие, нельзя включить ЭТУ атаку, дождаться конца
		else return EAttackAttemptResult::WRONG_PHASE;//◘◘>
	}

	//исполнитель сам по отдельности не попадает под критерии (например, здоровья мало или двигается слишком быстро)
	if(CheckMyself)
	{	//###########################################################
		auto CheckRes = Condition.IsFitting (Owner);
		if(CheckRes != EAttackAttemptResult::OKAY_FOR_NOW) return CheckRes;//##>
	}

	//если атакует игрок, прервать дальнейшее - так как дальше типично ИИ-шные оптимизации, отсекающие
	//те случаи (слишком далеко, не так повёрнуты), когда игрок сам должен думать
	if(!ByAI) return EAttackAttemptResult::OKAY_FOR_NOW;//◘◘>
	
	//////////////////////////////////////////////////////////////////////////
	//если ИИ атакующего уже подсуетился и промерил жертву в качестве цели
	if(Goal)
	{
		//если жертва вне радиуса доставания атаки, не надо ее начинать
		if(	!CheckByRadius (Goal->LookAtDist))
			return EAttackAttemptResult::OUT_OF_RADIUS;//◘◘>

		//если жертва вне угла поражения
		if(	!CheckByAngle (Goal->LookAlign))
			return EAttackAttemptResult::OUT_OF_ANGLE;//◘◘>
	}

	//////////////////////////////////////////////////////////////////////////
	//если атака направляется на существо
	if(auto MyrVic = Cast<AMyrPhyCreature>(ExactVictim->GetOwner()))
	{
		//жертва по отдельности не попадают под критерии (например, здоровья мало или двигается слишком быстро)
		//###########################################################
		auto CheckRes = ConditionVictim.IsFitting (Owner);
		if(CheckRes != EAttackAttemptResult::OKAY_FOR_NOW) return CheckRes;//##>
	}
	*/
	//вожделенная строчка
	return EAttackAttemptResult::OKAY_FOR_NOW;
}

//==============================================================================================================
//сравнить дву атаки для текущего существа, чтобы выбрать более подходящую
//==============================================================================================================
bool UMyrAttackInfo::IsItBetterThanTheOther (AMyrPhyCreature* Performer, UMyrAttackInfo *TheOther)
{
	auto MyRes = Condition.IsFitting(Performer);
	if (MyRes != EAttackAttemptResult::OKAY_FOR_NOW) return false;

	auto MyRes2 = TheOther->Condition.IsFitting(Performer);
	if (MyRes2 != EAttackAttemptResult::OKAY_FOR_NOW) return true;

	if(Performer->Stamina < 0.5) 
	{
		//тут надо по другому
//		if(DeltaStaminaAtSuccess < TheOther->DeltaStaminaAtSuccess) return false;
	}
	else
	{
		if (TactileDamage < TheOther->TactileDamage) return false;
	}
	return true;
}

//==============================================================================================================
//проверка возможности попадения в цель, для автонацеливания
//==============================================================================================================
bool UMyrAttackInfo::QuickAimResult(AMyrPhyCreature* Owner, FGoal* Goal, float Accuracy)
{
	//если жертва вне радиуса доставания атаки, не надо ее начинать
	if(	!CheckByRadius (Goal->LookAtDist)) return false;//◘◘>

	//если жертва вне угла поражения
	if(	!CheckByAngle (Goal->LookAlign)) return false;//◘◘>

	//если мы попадаем, но нацелены неточно (и помогать нам при такой точности - читерство)
	if(Goal->LookAlign < Accuracy)  return false;//◘◘>
	return true;//◘◘>
}


