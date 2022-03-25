// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Myrra.h"
#include "GameFramework/GameModeBase.h"
#include "MyrraGameModeBase.generated.h"

//###################################################################################################################
// этот актырь существует в единственном экземпляре на текущий постоянный УРОВЕНЬ, но может сменяться
// при смене перзистентного уровня. Таким образом, в каждый момент времени есть только один объект этого класса
// здесь можно регулировать множество актеров на уровне, досоздавать дополнительных актеров
// - здесь компонентом проигрыватель фоновой МУЗЫКИ, 
// - здесь запоминается протагонист, это позволяет любым другим объектам подстраиваться под наблюдения протагониста
// - - протагонист обплёвывается эффектом дождя, поэтому дождь устанавливается через этот класс
//###################################################################################################################
UCLASS(BlueprintType) class MYRRA_API AMyrraGameModeBase : public AGameModeBase
{
	GENERATED_BODY()


public:

	//контейнер визуальных режимов и виждетов для них, заполняется в редакторе
	//реальные виджеты создаются в объекте PlayerController по образцам сиим
	//разные уровни могут иметь разные наборы виджетов / меню
	//в редакторе создается потомок со всеми заполненностями - при смене уровней просто надо переуказать
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap <FName, FInterfaceMode> InterfaceModes;

	//настройки для обычной камеры от третьего лица, отсюда протагонист берет настройки, когда переключается с 1-ого лица обратно
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEyeVisuals ThirdPersonVisuals;


	//указатель на актора-управлятеля игроком
	//заполняться по идее должен не в методах этого класса, а при загрузке игры
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class AMyrDaemon* Protagonist = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AMyrDaemon> ProtagonistClass;

	//указатель на объект "небо", единственный на уровне - оттуда существа берут данные о времени суток, погоде
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class AMyrKingdomOfHeaven* Sky = nullptr;

	//музыка в игре - сюда программно помещаются разные куи в зависимости от времени, места, настроения, сюжета
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) UAudioComponent* Music = nullptr;

	//настройки гашения шумов специально для всего уровня (исключая отдельные локации со своими настройками)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class USoundAttenuation* SoundAttenuation;

protected:

	//вызывается, когда очередной трек музыки подходит к концу
	UFUNCTION()	void OnAudioFinished();

public://стандартные

	//конструктор
	AMyrraGameModeBase();

	//при загрузке уровня
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	//в начале игры - очевидно, при загрузке уровня
	virtual void BeginPlay() override;


public: //свои уникальные методы

	//установить видимый уровень дождя
	//вызывается из класа неба, применяется в классе протагониста (MyrraDaemon)
	//этот класс посредник, так как знает протагониста и виден отовсюду на уровне
	void SetWeatherRainAmount(float Amount);

	//модифицировать параметры звука фона и музыки, от времени дня, наличия дождя
	void SetAmbientSoundParameters(float DayFraction, float Rain);

	//модифицировать параметры звука по части призвука плохого самочувствия
	void SetSoundLowHealth(float Health, float Metabolism, float Pain);

	//что внутри пока неясно, оставлять на блюпринт или набирать списком данных
	void GameplayTriggerEvent(AActor* Subject, AActor *Object, EMyrLogicEvent WhatHappened);

public: //возвращуны

	//инстанция игры со своим типом
	class UMyrraGameInstance* GetMyrGameInst() { return (UMyrraGameInstance*)GetGameInstance(); }

	//возможно ли для данного уровня для данного режима сохранять на этом уровне игру - пока заглушка
	UFUNCTION(BlueprintCallable) bool IsSavingAllowed() const { return true; }

	//протагониста в виде конесного существа
	UFUNCTION(BlueprintCallable) class AMyrPhyCreature* GetProtagonist();

	//тип восприятия протагониста - нормальное, чутьё на одних, чутьё на других
	//от этого зависит, что рисовать на экране
	//эта функция раздаётся всем остальным классам
	int ProtagonistSenseMode() const;
	bool ProtagonistFirstPerson() const;

	//направление ветра (теперь лежит в другом классе, посему доступ через функцию)
	FVector2D* WindDir();

};
