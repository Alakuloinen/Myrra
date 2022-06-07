// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyrArtefact.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "MyrraDoor.generated.h"

UENUM(BlueprintType) enum class EDoorState : uint8
{
	DeadLocked,	//заперт, невозможно открыть
	Locked,		//закрыта на ручку, поворот ручки поможет открыть
	Closed,		//открыта, но захлопнута
	Open		//открыта настежь
};

//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
UCLASS() class MYRRA_API UMyrDoorProfile : public UDataAsset
{
	GENERATED_BODY()

public:

	//восприимчивость двери к ударам, лёгкость открывания
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float Sensibility = 0.5f;

	//коэффициент затухания вращения двери
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float Friction = 0.01f;

	//насколько дверь сама пытается закрыться
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float SelfClosingForce = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float SelfClosingOpenness = 0.0f;

	//лимиты на открытие в разные стороны, если между интервалом, то просто туже движется
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") FFloatRange PositiveLimit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") FFloatRange NegativeLimit;

	//максимальные скорости, к сожалению, зависимость от DeltaTime послана, эти числа вводятся в граничные кватернионы и затем лерпится по безразмерному коэффициенту скорости
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float MaxRotVelPositive = 0.01f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float MaxRotVelNegative = 0.01f;

	//стартовый угол открытия двери, в радианах, плюс или минус
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") float StartAngle = 0.0f;

	//звук, или сборка звуков, при прохождении или закрывании двери
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") class USoundBase* SoundAtClosing;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") class USoundBase* SoundAtLocking;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Door") class USoundBase* SoundAtUnLocking;


	UMyrDoorProfile()
	{
		PositiveLimit = FFloatRange(0.5f, 0.6f);
		NegativeLimit = FFloatRange(0.5f, 0.6f);
	}

};


//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//новый вариант двери, теперь встраиваемый компонентов в любой класс в любом количестве
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYRRA_API UMyrDoorLeaf : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	// сохранить существо, которое последним открывало или закрывало дверь
	UPROPERTY(EditAnywhere, BlueprintReadWrite) AActor* LastMover;	

	//данные по типу артефакта
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UMyrDoorProfile* Profile;

	//вероятностные параметры для массового внедрения вариаций в сцену
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 ProbabilityOfBeingAbsent = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 ProbabilityOfBeingLocked = 0;

	//объект, который, будучи дотронутым или пересечённым, открывает или закрывает дверь
	//пока неясно, как сделать это универсально, возможно, придётся заводить особый класс, даже отдельного актора
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class USceneComponent* Trigger;

	//состояния
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Closed : 1;			// просто закрыто параллельно порогу
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Locked : 1;			// сейчас заперто или не заперто
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 LockPending : 1;		// сказали запереть, но реальный запор поризойдёт, когда дверь захлопнется

	//направление вдоль полотна двери в закрытом состоянии
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector StartAlongDir;

	// кватернионные эквиваленты вращения в положительную и отрицательную сторону - для лерпа
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FQuat RotVelBaseP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FQuat RotVelBaseN;

	//скалярный эквивалент скорости вращения, нормирован 0-1, для лерпа кватернионов
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RotVel = 0;

public:

	//правильный конструктор
	UMyrDoorLeaf(const FObjectInitializer& ObjectInitializer);

	//при запуске игры
	virtual void BeginPlay() override;

	//каждый кадр
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	//реакция на касание 
	UFUNCTION()	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

//процедуры
public:

	//явно установить открытость двери
	void SetOpenness(float Angle);

	//звук разных моментов двери
	void DoorSound(USoundBase* WhatTo, float Strength = 1.0f);

	//отпереть дверь, если она заперта и правильно позиционирована
	void Unlock();

	//запереть дверь
	void Lock();

	//внести вариацию состояний на старте, согласно ассету
	void VaryOnStart();

//возвращенцы
public:

	//ось вращения двери
	FVector DoorUp() const { return GetComponentTransform().GetUnitAxis(EAxis::Z); }

	//направление вдоль полотна двери
	FVector DoorAlong() const { return GetComponentTransform().GetUnitAxis(EAxis::X); }

	//направление движения двери
	FVector DoorFront() const { return GetComponentTransform().GetUnitAxis(EAxis::Y); }

	//степень (косинус угла) открытости двери
	float GetClosedness() { return FVector::DotProduct(StartAlongDir, DoorAlong()); }

	//открытость двери вперёд (+) или назад (-)
	float GetSideOpenness() { return FVector::DotProduct(-StartAlongDir, DoorFront()); }
};

