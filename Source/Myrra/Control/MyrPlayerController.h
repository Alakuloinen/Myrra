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

	//текст важной инфы
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		FText PlayHudMessage;

	//новые варианты оформления - два виджета-комбайна, внутри которых меняются менюшки и окна
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	class UMyrMenuWidget* WidgetUI = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	class UMyrBioStatUserWidget* WidgetHUD = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	bool bUI = false;


public:

	//прямой доступ к вектору ориентации тела (для шаманства с ограничениями обзора для первого лица)
	//сам вектор хранится в акторе-существе, которым управляет актор-демон, котором управляет этот контроллер
	FVector* BodyOrientation = nullptr;

	//включение режима ограничений на огляд, чтоб не смотреть себе внутрь
	//устанавливается через функцию SetFirstPerson, которая ныне приходит из актора AMyrDaemon
	bool FirstPersonLimits = false;

	//ограничение на поворот головы 
	float FirstPersonHeadYawLimit = 90;

	//трясуны камеры в случае упоротости и в случае боли
	class UCameraShakeBase* Shake = nullptr;
	class UCameraShakeBase* PainShake = nullptr;

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

	//возвращуны текущих виджетов
	UMyrBioStatUserWidget* GetWidgetHUD() { return WidgetHUD; }
	UMyrMenuWidget* GetWidgetUI() { return WidgetUI; }
	bool IsNowUIPause() const { return bUI; }

	//впендюрить трясунов
	void AddCameraShake(TSubclassOf<UCameraShakeBase> s) { Shake = PlayerCameraManager->StartCameraShake(s); Shake->ShakeScale = 0; }
	void AddPainCameraShake(TSubclassOf<UCameraShakeBase> s) { PainShake = PlayerCameraManager->StartCameraShake(s);  PainShake->ShakeScale = 0; }
	UCameraShakeBase* GetCameraShake() { return Shake; }
	UCameraShakeBase* GetPainCameraShake() { return PainShake; }

	//ввести в виджеты ссылки на текущего главного героя (в начале и при обновлении)
	void ChangeWidgetRefs(class AMyrPhyCreature* PC);

	//смена режима лица
	void SetFirstPerson(bool Set);


//-------------------------------------------------------------
public:	//обработчики некоторых системных команд
//-------------------------------------------------------------


	//новая фигня - закрыть универсальный агрегатор меню вплоть до игрового худа
	UFUNCTION(BlueprintCallable) void ChangeWidgets(EUIEntry NewEntry = EUIEntry::NONE);

	//команды интерфейса
	UFUNCTION(BlueprintCallable) void CmdPause()		{ ChangeWidgets(EUIEntry::Pause); };
	UFUNCTION(BlueprintCallable) void CmdGameOver()		{ ChangeWidgets(EUIEntry::GameOver); };
	UFUNCTION(BlueprintCallable) void CmdQuests()		{ ChangeWidgets(EUIEntry::Quests); };
	UFUNCTION(BlueprintCallable) void CmdEmoStimuli()	{ ChangeWidgets(EUIEntry::EmoStimuli); };
	UFUNCTION(BlueprintCallable) void CmdKnown()		{ ChangeWidgets(EUIEntry::Known); };
	UFUNCTION(BlueprintCallable) void CmdSaves()		{ ChangeWidgets(EUIEntry::Saves); };
	UFUNCTION(BlueprintCallable) void CmdOptions()		{ ChangeWidgets(EUIEntry::Options); };
	UFUNCTION(BlueprintCallable) void CmdPhenes()		{ ChangeWidgets(EUIEntry::Phenes); };
	UFUNCTION(BlueprintCallable) void CmdStats()		{ ChangeWidgets(EUIEntry::Stats); };

	//это даже не меню, а вызов процедуры сохранения и закрытия (закрытия или оставить?) текущего меню
	UFUNCTION(BlueprintCallable) void CmdQuickSave();


};
