// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Myrra.h"
#include "GameFramework/SaveGame.h"
#include "MyrraSaveGame.generated.h"

//###################################################################################################################
// аггрегатор сохраняемых параметров игровой сессии. При сохранении этот объект создаётся и целиком сериализуется.
// Здесь только отобранные значащие плоские данные, никаких уобъектов, копий акторов и т.п., поэтому процесс учета
// этих данных в сцене (создание акторов, изменение их свойств) - отдельный алгоритм
//###################################################################################################################
UCLASS() class MYRRA_API UMyrraSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	//типа имя, но имя чего? конечного файла?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString SaveSlotName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FDateTime SavedDateTime;

	//перзистентный уровень, главный, на котором происзодила игра; остальные
	//уровни загружаются вокруг игрока по его положению либо автоматически (акторы-объёмы)
	//либо при загрузке главного по срабатыванию триггеров каких-то
	UPROPERTY(VisibleAnywhere) FName PrimaryLevel;

	//время дня, или нескольких дней, которое реально изменяется в игре, солнце отсчитывается от DateOfPrecalculatedDays + TimeOfDay
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FTimespan TimePassed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	uint8 SunrizeTimeFrac;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	uint8 SunsetTimeFrac;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	uint8 NoonTimeFrac;

	//массив данных по героям (и вообще всем существам в игре) и предметам
	UPROPERTY(VisibleAnywhere) TMap <FName, FCreatureSaveData> AllCreatures;
	UPROPERTY(VisibleAnywhere) TMap <FName, FArtefactSaveData> AllArtefacts;
	UPROPERTY(VisibleAnywhere) TMap <FName, FLocationSaveData> AllLocations;
	UPROPERTY(VisibleAnywhere) TMap <FName, FQuestSaveData>    AllQuests;

	//здесь должен быть ещё контейнер квестов, но он пока не придуман

	//явно освободить память
	void CleanUp()
	{
		AllCreatures.Empty();
		AllArtefacts.Empty();
		AllLocations.Empty();
		AllQuests.Empty();
	}
};

//###################################################################################################################
// ярлычок, добываемый из файла сохранки, содержащий минимум инфы, соответственно, не захламляющий память 
// а то полная сохранка может содержать тонны инфы по всем существам, предметам, квестам, а в меню они все загружаются
// хотя создание объекта и наполнение его из файла - два разных расхода памят
// а здесь всего 80 байт, а почему UObject - потому что ListView UMG требует для себя массива УОбъектов и никак иначе
//###################################################################################################################
UCLASS() class MYRRA_API UMyrSaveGameLabel : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString SaveSlotName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FDateTime SavedDateTime;
};