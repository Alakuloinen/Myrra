// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Myrra.h"
#include "MyrDaemon.h"
#include "GameFramework/PlayerController.h"
#include "MyrPlayerController.generated.h"

//###################################################################################################################
// контроллер игрока - до сих пор непонятно, зачем переопределять, но, судя по всему виджеты меню обычно делаются тут
// ныне используется для следующих фич:
// - хранение, создание, смена виджета на экран (меню паузы/сохранений/прочего VS худ)
// - включение (по кнопке, геймплею) функций с виджетами (меню, диалоговые окна) - остальные через GameInstance
// / ограничения на огляд при 1-ом лице, чтобы не смотреть внутрь тела, пока работает не вполне гладко (видимо, теперь не нужно)
// / ролевая система (возможно, вынести в отдельный компонент, но смысла нет, так как нужно только в одном месте) - ролевая теперь другая, надо переделывать
//###################################################################################################################
UCLASS() class MYRRA_API AMyrPlayerController : public APlayerController
{
	GENERATED_BODY()

	//виджеты различных режимов интерфейса, адресуемые именами для простоты и понятности, не выдумывать энумераторы
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TMap<FName, UUserWidget*> UIWidgets;

	//текущее состояние интерфейса поверх игры (да, упроперти только перове из них)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	FName CurrentUIModeName;			// имя текущего интерфейса - для удобства, универсальности и редактора
	FInterfaceMode* CurrentUIMode;		// аггрегатор реальных свойств режима интерфейса - управлние, ток времени

	// запоминалка для предыдущего режима, чтобы легче возвращаться
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FName ToggleRecentUIModeName;		

	//текст важной инфы
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		FText PlayHudMessage;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	//	class UMyrRPGComponent* RPGComponent = nullptr;

public:

	//прямой доступ к вектору ориентации тела (для шаманства с ограничениями обзора для первого лица)
	//сам вектор хранится в акторе-существе, которым управляет актор-демон, котором управляет этот контроллер
	FVector* BodyOrientation = nullptr;

	//включение режима ограничений на огляд, чтоб не смотреть себе внутрь
	//устанавливается через функцию SetFirstPerson, которая ныне приходит из актора AMyrDaemon
	bool FirstPersonLimits = false;

	//ограничение на поворот головы 
	float FirstPersonHeadYawLimit = 90;

//-------------------------------------------------------------
public:	//переопределения стандартных методов
//-------------------------------------------------------------

	//конструктор
	AMyrPlayerController();

	//непосредственно перед началом, здесь создаются виджеты по образцам
	virtual void BeginPlay() override;

	// определить ввод, клавиши, мышь и т.п.
	virtual void SetupInputComponent() override;

	//какой-то стандартный метод, сюда мы добавляем ограничения на вращение камеры по достижениям углов
	virtual void UpdateRotation(float DeltaTime) override;

//-------------------------------------------------------------
public:	//свои методы
//-------------------------------------------------------------

	//выдать управляемого актора в нужном формате
	class AMyrDaemon* GetDaemonPawn() { return Cast<AMyrDaemon>(GetPawn()); }

	//выдать управляемого актора в нужном формате
	class UMyrraGameInstance* GetMyrGameInstance() { return (UMyrraGameInstance*)GetGameInstance(); }
	class AMyrraGameModeBase* GetMyrGameMode() { return (AMyrraGameModeBase*)GetWorld()->GetAuthGameMode(); }

	//смена режима лица
	void SetFirstPerson(bool Set);

	//переключиться на другой интерфейс
	UFUNCTION(BlueprintCallable) void ChangeUIMode(FName ModeName);

	//текущий интерфейс
	UFUNCTION(BlueprintCallable) FName GetCurrentUIModeName() const { return CurrentUIModeName; };
	UFUNCTION(BlueprintCallable) FName GetRecentUIModeName() const { return ToggleRecentUIModeName; };
	FInterfaceMode* GetCurrentUIMode() const { return CurrentUIMode; };

	//выдать текущий виджет текущего режима интрфейса, может выдать нуль
	class UMyrBioStatUserWidget* CurrentWidget();

	//если мы находимся на уровне, который предполагает игру (а не только интерфейс) для этого должен присутствовать виджет худ
	UFUNCTION(BlueprintCallable) bool OnPlayLevel() const { return (bool)UIWidgets.Find(TEXT("Play"));  }

	//UMyrRPGComponent* GetRPGComponent() { return RPGComponent; }

//-------------------------------------------------------------
public:	//обработчики некоторых системных команд
//-------------------------------------------------------------

	//обработчики команд включения и выключения разных меню по кнопкам
	UFUNCTION(BlueprintCallable) void TogglePause();
	UFUNCTION(BlueprintCallable) void ToggleSaves();
	UFUNCTION(BlueprintCallable) void ToggleStats();
	UFUNCTION(BlueprintCallable) void ToggleQuests();
	UFUNCTION(BlueprintCallable) void QuickSave();

	//экран при окончании игры
	UFUNCTION(BlueprintCallable) void GameOverScreen();

};
