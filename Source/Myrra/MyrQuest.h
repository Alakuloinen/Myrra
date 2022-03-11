// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "UObject/NoExportTypes.h"
#include "MyrQuest.generated.h"

//###################################################################################################################
//состояние конечного автомата квеста, стадия квеста, 
//соединяется с другими стадиями с помощью FMyrQuestTransition
//###################################################################################################################
USTRUCT(BlueprintType) struct FMyrQuestState
{
	GENERATED_USTRUCT_BODY()

	//описание для отображения игроку
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText TextForJournal;
};

//###################################################################################################################
//мгновенный переход, дуга конечного автомата квеста
//###################################################################################################################
USTRUCT(BlueprintType) struct FMyrQuestTransition
{
	GENERATED_USTRUCT_BODY()

	//описание для отображения игроку
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText TextForJournal;
};

//###################################################################################################################
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, hidecategories = (Object), meta = (BlueprintSpawnableComponent), Config = Game)
class MYRRA_API UMyrQuest : public UObject
{
	GENERATED_BODY()

public:

	//человекопонятное имя, которое можно отобразить на экране как пункт меню и переовдить
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText HumanReadableName;

	//состояния квеста
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FMyrQuestState> QuestStates;

	//переходы между состояниями квеста
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FMyrQuestTransition> QuestTransitions;

	//поместить новый триггер в лист ожидания
	void PutToTheWaitingList();

public:

	//осуществить обработку очередной цеплялки
	//указывается идентификатор внутреннего представления перехода между состояниями, из которого порождена цеплялка
	//возвращается истина, если по итогам цеплялку надо удалить из списка ожидания
	bool Process(int32 TransID) { return true; }

	UFUNCTION(BlueprintCallable) void MakeState1i1o(const FMyrQuestTransition& i1, FMyrQuestTransition& o1) {}

	//болванка для графического проектирования квеста в редакторе
	UFUNCTION(BlueprintImplementableEvent)	void ConstructQuestLogic(AActor* Obj, bool Can);

};
