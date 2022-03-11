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
