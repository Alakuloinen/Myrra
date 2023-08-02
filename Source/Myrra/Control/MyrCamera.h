// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "Myrra.h"
#include "MyrCamera.generated.h"

//режим положение камеры, определяет также эффекты движения, постэффекты и т.п.
UENUM(BlueprintType) enum class EWhoSees : uint8
{
	NONE,					//0000

	Creature = 5,			//01 01
	CreatureToPlayer = 6,	//01 10
	CreatureToWatcher = 7,	//01 11

	PlayerToCreature = 9,	//10 01
	Player = 10,			//10 10
	PlayerToWatcher = 11,	//10 11

	WatcherToCreature = 13,	//11 01
	WatcherToPlayer = 14,	//11 10
	Watcher = 15,			//11 11
};


//###################################################################################################################
// живая камера с огибом
// 1 - позиционирование (вставить во внешний тик)
// 2 - огиб препятствий
// 3 - модификация расстояния (доделать)
// 4 - пост-эффекты (вводить извне в тике)
//###################################################################################################################
UCLASS() class MYRRA_API UMyrCamera : public UCameraComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)		USceneComponent* Seer = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		USceneComponent* Sight = nullptr;

	//базис расстояния камеры в реальных сантиметрах, определяется или явно, или размерами тушки подопечного существа
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float DistanceBasis = 150;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float DistanceModifier = 1.0f;

	//радиус орешка вокруг камеры от 3 лица, вглубь которого твёрдым предметам не разрешено проникать
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float RadiusHard = 10;

	//радиус сферы вокруг камеры 3 лица, от которой начинается упругость и отталкивание от стен
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float RadiusSoft = 100;

	//отклонение точки наблюдения от контроллерной позиции в плоскости перпендикулярной линии взгляда
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float SeerOffsetTarget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float SeerOffset;

public: 

	//пост-процесс материал для экрана ухудшения здоровья, достаётся из настроек камеры
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	UMaterialInstanceDynamic* HealthReactionScreen = nullptr;

	//тряски камеры - это не объект, это в редакторе подвязать класс
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		TSubclassOf<UCameraShakeBase> Shake;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		TSubclassOf<UCameraShakeBase> PainShake;

public:

	//загнать в текущую камеру настройки эффектов 
	void AdoptEyeVisuals(const FEyeVisuals& EV);

	//базовый переключатель лиц
	void SetPerson(int P, USceneComponent* ExternalSeer = nullptr);
	void HappenPerson(int P);

	//полная процедура позиционирования, с трассировкой, каждый кадр
	void ProcessDistance(float DeltaTime, FVector ctrlDir);

	//применить графическое отображение ощущений
	void SetFeelings(float Psycho, float Pain, float Dying);
	void SetFeelingsSlow(float Health, float Sleepy, float Psycho, float Dying);

public:

	class AMyrDaemon*				me()			{ return (AMyrDaemon*)GetOwner(); }
	class AMyrDaemon*				me() const		{ return const_cast<UMyrCamera*>(this)->me(); }
	class AMyrPlayerController*		myCtrl()		{ return (AMyrPlayerController*)(((APawn*)GetOwner())->GetController()); }
	class AMyrPlayerController*		myCtrl() const	{ return const_cast<UMyrCamera*>(this)->myCtrl(); }
	class AMyrPhyCreature*			myBody()		{ return (AMyrPhyCreature*)GetOwner()->GetAttachParentActor(); }
	class AMyrPhyCreature*			myBody() const	{ return const_cast<UMyrCamera*>(this)->myBody(); }

	int GetPerson() const
	{ 	if (DistanceModifier == 0)
		{	if (!Sight) return 11;		//незыблемое первое лицо
			else if (Seer) return 41;	//переход с четвертого (наблюдателя) на первое
			else return 31;				//переход с третьего на первое
		}else
		{	if (Seer) return 44; else	//стабильное четвертое
			if (Sight) return 33; else	//стабильное третье
			return 13;						
		}
	}


//стандартные методы и переопределения
public:

	//конструктор
	UMyrCamera(const FObjectInitializer& ObjectInitializer);

	//при запуске игры
	virtual void BeginPlay() override;

};
