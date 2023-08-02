// ◘Fill out your copyright notice in the Description page of Project Settings.


#include "MyrQuest.h"
#include "MyrraGameInstance.h"
#include "Creature/MyrPhyCreature.h"
#include "Control/MyrDaemon.h"
#include "UI/MyrBioStatUserWidget.h"

//====================================================================================================
//скинуть указатели на переходы этого состояния этого квеста в общюю карту для быстрого доступа
//====================================================================================================
void FMyrQuestState::PutToWaitingList(TMultiMap<EMyrLogicEvent, FMyrQuestTrigger*>& MyrQuestWaitingList, class UMyrQuest* Owner, FName ThisInMap)
{
	//все возможные переходы
	for (auto& QT : QuestTransitions)				// важно auto&, чтобы получать реальные адреса
	{
		QT.Value.OwningQuest = Owner;				//чтобы ссылаться извне при продвижении
		QT.Value.QuestNextStateName = QT.Key;		//в транзиции ключ - это имя результирующего состояния
		QT.Value.QuestCurStateName = ThisInMap;		//имя состояния - берется из внешней карты в классе квеста
		MyrQuestWaitingList.Add(QT.Value.Event, &QT.Value);
		UE_LOG(LogTemp, Log, TEXT("Quest %s PutToWaitingList - %s"), *Owner->GetName(), *UEnum::GetValueAsString(QT.Value.Event));
	}
}

//====================================================================================================
// осуществить переход в новую стадию квеста
//====================================================================================================
bool UMyrQuestProgress::DoTransition(FName NewState, class AMyrPhyCreature* Owner)
{
	if(!Quest.IsValid())
	{	UE_LOG(LogTemp, Error, TEXT("Quest DoTransition %s WTF no quest!"), *GetName());
		return false;
	}
	// запрашиваемое состояние существует
	if (auto NS = Quest->QuestStates.Find(NewState))
	{
		//сохранить предыдущее состояние как последнее пройденное
		StatesPassed.Add(CurrentState);

		//перейти в новое состояние
		CurrentState = NewState;

		//удалить старый маркер состояния если таковой имелся
		if(Owner->Daemon) Owner->Daemon->RemoveMarker();

		//прореагировать на смену состояния
		UObject* ContextObj = nullptr;
		for(auto R : NS->Reactions)
		{
			//найти контекст - реальный объект по тексту, может быть долго
			if(!R.ExactContextObj.IsNone())
			{	if (!ContextObj || ContextObj->GetFName() != R.ExactContextObj)
				{	ContextObj = FindObject<UObject>(ANY_PACKAGE, *R.ExactContextObj.ToString());
					if(!ContextObj)
						UE_LOG(LogTemp, Warning, TEXT("Quest DoTransition %s not found %s"), *GetName(), *R.ExactContextObj.ToString());
				}
			}
			//выполнить команды реакций на воцарение этого состояния
			Owner->GetMyrGameInst()->React(R, ContextObj, Owner);
		}

		//далее нужно обрабатывать реакции
		//если нет новых переходов - это конец квеста
		if (NS->QuestTransitions.Num() == 0)
		{
		}
		else
		{
		}


		//обобразить в интерфейсе факт продвижения
		if (Owner->IsUnderDaemon())
			Owner->Daemon->HUDOfThisPlayer()->OnQuestChanged(this);
		UE_LOG(LogTemp, Log, TEXT("Quest DoTransition %s changed to %s"), *GetName(), *NewState.ToString());
		return true;
	}

	//аварийный выход если почему-то не нашли такое состояние
	UE_LOG(LogTemp, Error, TEXT("Quest DoTransition %s WTF no state %s"), *GetName(), *NewState.ToString());
	return false;
}


