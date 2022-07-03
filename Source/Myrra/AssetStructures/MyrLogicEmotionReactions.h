// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Myrra.h"
#include "MyrLogicEmotionReactions.generated.h"

UCLASS() class MYRRA_API UMyrLogicEmotionReactions : public UDataAsset
{
	GENERATED_BODY()

public:

	//комплексная реакция на разные элементарные действия
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
		TMap<EMyrLogicEvent, FMyrLogicEventData> MyrLogicEvents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FWholeBodyDynamicsModel> DynModelsPerPhase;


	//забить место под новый
	bool Add(FMyrLogicEventData*& Dst, EMyrLogicEvent E);

	//конструктырь
	UMyrLogicEmotionReactions();

};

//новая херня, оболочка для структуры, которую можно иметь отдельным файлом
UCLASS() class MYRRA_API UMyrEmoReactionList : public UDataAsset
{
	GENERATED_BODY()

public:

	//комплексная реакция на разные элементарные действия
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
		FEmoReactionList List;

	//добавлялка, чтоб не писать много
	void Add(EEmoCause C, EEmotio V, uint8 Strength, uint8 Duration)
	{   List.Map.Add(C, FEmoStimulus(FEmotio::As(V).ToFLinearColor(), Strength, Duration));		}
	void Add(EEmoCause C, uint8 R, uint8 G, uint8 B, uint8 Strength, uint8 Duration)
	{	List.Map.Add(C, FEmoStimulus(R, G, B, Strength, Duration));		}

	//конструктырь
	UMyrEmoReactionList();

};
