// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "UObject/NoExportTypes.h"
#include "MyrQuest.generated.h"


//###################################################################################################################
//состояние конечного автомата квеста, стадия квеста, 
//соединяется с другими стадиями с помощью FMyrQuestTransition
//нулевая стадия квеста всегда загружена - чтобы его начать, последняя стадия не имеет переходов
//###################################################################################################################
USTRUCT(BlueprintType) struct FMyrQuestState
{
	GENERATED_USTRUCT_BODY()

	//имя чтобы ссылаться в редакторе при составлении
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText TextCaption;

	//подробное описание для журнала
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText TextForJournal;

	//реакции на достижение состояния
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FTriggerReason> Reactions;

	//переходы между из этого состояния в другие состояния
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FName, FMyrQuestTrigger> QuestTransitions;

	//поместить все условные переходы в централизованный лист цеплялок для быстрого доступа (список извне из инстанции)
	void PutToWaitingList(TMultiMap<EMyrLogicEvent, FMyrQuestTrigger*>& MyrQuestWaitingList, class UMyrQuest* Owner, FName ThisInMap);

};



//###################################################################################################################
//квест - чисто статическая структура данных безотносительно реального прогресса
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, hidecategories = (Object), meta = (BlueprintSpawnableComponent), Config = Game)
class MYRRA_API UMyrQuest : public UDataAsset
{
	GENERATED_BODY()

public:

	//человекопонятное имя, которое можно отобразить на экране как пункт меню и переовдить
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText HumanReadableName;

	//состояния квеста
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FName, FMyrQuestState> QuestStates;

	//показывать ли в журнале - некоторые "обслуживающие" квесты лучше делать исподтишка 
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool ShowInJournal = true;

	//нужно ли сохранять при сохранении игры, или он казуально-бесконечный
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Save = true;


};

//###################################################################################################################
//динамическое воплощение квеста, ослеживание прохождения
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, hidecategories = (Object), meta = (BlueprintSpawnableComponent), Config = Game)
class MYRRA_API UMyrQuestProgress : public UObject
{
	GENERATED_BODY()

public: 

	//нарративный источник
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TWeakObjectPtr<UMyrQuest> Quest;

	//цепочка прогресса, имена стадий в соответствующем квесте
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> StatesPassed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName CurrentState;

	//квест начат = пройдено хоть одно состояние
	UFUNCTION(BlueprintCallable) bool IsStarted() const { return (StatesPassed.Num() > 0); }

	//квест закончен = имеется пройденные состояния, текущее же сброшено в нуль (это делается явно при встече с состояние без переходов)
	UFUNCTION(BlueprintCallable) bool IsFinished() const { return StatesPassed.Num() > 0 && CurrentState == NAME_None; }

	//совершить переход на новую стадию, указывается протагонист (хотя как быть если квест подвигает непись?)
	UFUNCTION(BlueprintCallable) bool DoTransition(FName NewState, class AMyrPhyCreature* Owner);

	//загрузка и сохранение
	UFUNCTION(BlueprintCallable) void Load(FQuestSaveData& Data) { StatesPassed = Data.PassedStates; CurrentState = Data.PassedStates.Last(0); StatesPassed.SetNum(StatesPassed.Num()-1); }
	UFUNCTION(BlueprintCallable) void Save(FQuestSaveData& Data) { Data.Name = GetFName(); Data.PassedStates = StatesPassed; Data.PassedStates.Add(CurrentState); }


};