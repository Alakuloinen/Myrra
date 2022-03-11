// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "..\Myrra.h"
#include "UObject/NoExportTypes.h"
#include "MyrArtefactInfo.generated.h"

//###################################################################################################################
// ▄▀ внимание, фабрика по созданию ресурсов этого типа в режакторе реализуется в модуле MyrraEditor/asset_factories.h
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, hidecategories = (Object), meta = (BlueprintSpawnableComponent), Config = Game)
class MYRRA_API UMyrArtefactInfo : public UObject
{
	GENERATED_BODY()

public:

	//канал, на котором "виден" запах существ этого класса
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  uint8 SmellChannel = 0;

	//связанное с возможностью съесть этот артефакт
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Item") FDigestivity Digestivity;

	//восприимчивость двери к ударам, лёгкость открывания
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float DoorSensibility = 0.5f;

	//коэффициент затухания вращения двери
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float DoorFriction = 0.01f;

	//насколько дверь сама пытается закрыться
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float DoorSelfClosingForce = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float DoorSelfClosingOpenness = 0.0f;

	//лимиты на открытие в разные стороны, если между интервалом, то просто туже движется
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") FFloatRange DoorPositiveLimit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") FFloatRange DoorNegativeLimit;

	//максимальные скорости, к сожалению, зависимость от DeltaTime послана, эти числа вводятся в граничные кватернионы и затем лерпится по безразмерному коэффициенту скорости
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float DoorMaxRotVelPositive = 0.01f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float DoorMaxRotVelNegative = 0.01f;

	//стартовый угол открытия двери, в радианах, плюс или минус
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float StartAngle = 0.0f;

	//вероятностные параметры для массового внедрения вариаций в сцену
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") uint8 ProbabilityOfBeingAbsent = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") uint8 ProbabilityOfBeingLocked = 0;

	//звук, или сборка звуков, при прохождении или закрывании двери
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") class USoundBase* SoundAtClosing;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") class USoundBase* SoundAtLocking;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") class USoundBase* SoundAtUnLocking;


	UMyrArtefactInfo()
	{
		DoorPositiveLimit = FFloatRange(0.5f, 0.6f);
		DoorNegativeLimit = FFloatRange(0.5f, 0.6f);
	}
};
