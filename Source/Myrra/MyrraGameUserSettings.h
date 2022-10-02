// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "GameFramework/GameUserSettings.h"
#include "MyrraGameUserSettings.generated.h"

//DefaultEngine.ini 
//[/Script/Engine.Engine]
//NearClipPlane = 10.000000
//GameUserSettingsClassName = / Script / Myrra.MyrraGameUserSettings

UCLASS()
class MYRRA_API UMyrraGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

protected:

	//громкости звуков в целых процентах
	UPROPERTY(config) uint8 SoundAmbient = 50;
	UPROPERTY(config) uint8 SoundSubjective = 50;
	UPROPERTY(config) uint8 SoundNoises = 70;
	UPROPERTY(config) uint8 SoundVoice = 80;
	UPROPERTY(config) uint8 SoundMusic = 40;


public:

	//выдать сохраненную опцию
	UFUNCTION(BlueprintCallable) int32 GetOption(const EMyrOptions O) const;

	//установить опцию
	UFUNCTION(BlueprintCallable) void  SetOption(EMyrOptions O, int V);

	//выдать громкость по индексу, внимание, опасная игра в указатели!
	int32 GetSoundVolume(int32 Numer) const { return (int32)((&SoundAmbient)[Numer]); }

};
