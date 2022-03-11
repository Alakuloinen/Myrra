// Fill out your copyright notice in the Description page of Project Settings.

#include "MyrPlayerController.h"
#include "../MyrraGameInstance.h"	//без этого вообще никуда
#include "../MyrraGameModeBase.h"	//без этого вообще никуда
#include "MyrraGameModeBase.h"
#include "Engine/Classes/Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "../Creature/MyrPhyCreature.h"	//чтоб спавгить демона сразу на привязи
#include "../UI/MyrBioStatUserWidget.h"	// модифицированный виджет со вложенными указателями на игрока, контроллер их переключает
#include "Kismet/GameplayStatics.h"						// для вызова GetAllActorsOfClass

//==============================================================================================================
// конструктор - здесь вроде как можно инициализировать виджеты
//==============================================================================================================
AMyrPlayerController::AMyrPlayerController()
{
}

//==============================================================================================================
// сменить наэкранный интерфейс
//==============================================================================================================
void AMyrPlayerController::ChangeUIMode (FName ModeName)
{
	UE_LOG(LogTemp, Warning, TEXT("Change UI: %s - %s"),
		*CurrentUIModeName.ToString(),
		*ModeName.ToString());

	AMyrDaemon* Me = GetDaemonPawn();
	if (!Me)
	{	UE_LOG(LogTemp, Error, TEXT("ChangeUIMode WTF no daemon pawn!"));
		return;
	}

	//проверка на существование старого виджета
	if(CurrentUIMode)
	{
		//виджет, который текущий
		UUserWidget** pCurWi = UIWidgets.Find(CurrentUIModeName);
		if (!pCurWi || !*pCurWi) { UE_LOG(LogTemp, Error, TEXT("No Widget 1 %s"), *CurrentUIModeName.ToString());	}
		else
		{
			//сохранить предыдущий, чтобы можно было вернуться на одну позицию
			ToggleRecentUIModeName = CurrentUIModeName;

			//убрать старый виджет с экрана
			UUserWidget* CurWi = *pCurWi;
			CurWi->RemoveFromParent();
		}
	}

	//достаём новый режим, найдя в карте из класса AMyrraGameModeBase
	auto GameMode = Cast<AMyrraGameModeBase>(GetWorld()->GetAuthGameMode());
	CurrentUIMode = GameMode->InterfaceModes.Find(ModeName);

	//если режим есть (иначе выдаст нуль, и система впадёт в пустой не HUDовый режим)
	if(CurrentUIMode)
	{
		//виджет, который текущий
		UUserWidget** pCurWi = UIWidgets.Find (ModeName);
		if (!pCurWi || !*pCurWi) { UE_LOG(LogTemp, Error, TEXT("No Widget 2 %s"), *ModeName.ToString());	return;	}
		UUserWidget* CurWi = *pCurWi;
		CurrentUIModeName = ModeName;

		//сразу поместить на экран
		CurWi->AddToViewport();

		// типы взаимодействия с пользователем - тыкабельное меню или нетыкабельный худ.
		if(CurrentUIMode->UI_notHUD == 1)
		{
			//настроить управление, характерное для меню, с курсором и кнопками
			FInputModeUIOnly InputModeData;
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::LockInFullscreen);
			InputModeData.SetWidgetToFocus(CurWi->TakeWidget()); //Because UMG wraps Slate
			CurWi->SetKeyboardFocus();

			//включить обесцвечивание подлежащей под меню сцены
			GetDaemonPawn()->SwitchToMenuPause(true);

			//это всё местные методы
			SetInputMode(InputModeData);
			bShowMouseCursor = true;
			SetTickableWhenPaused(true);
		}
		else
		{
			//выключить обесцвечивание подлежащей под меню сцены
			GetDaemonPawn()->SwitchToMenuPause(false);

			//настроить управление для нормального 3д-мира
			FInputModeGameOnly InputModeData;
			SetInputMode(InputModeData);
			bShowMouseCursor = false;
		}

		//если виджет определен как местный класс, инициализируем дополнительные поля
		if (auto BioCurWi = Cast<UMyrBioStatUserWidget>(CurWi))
		{	BioCurWi->MyrPlayerController = this;
			BioCurWi->MyrOwner = GetDaemonPawn()->GetOwnedCreature();
			BioCurWi->OnJustShowed();
		}

		//настроить ток времени / паузу по настройкам свежевыставленного режима
		if (CurrentUIMode->TimeFlowMod == 0)
		{	UGameplayStatics::SetGamePaused(GetWorld(), true);		}
		else
		{
			UGameplayStatics::SetGamePaused(GetWorld(), false);
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0);
		}
	}
	else { UE_LOG(LogTemp, Error, TEXT("No Widget pointer %s"), *ModeName.ToString());	return; }
}

//==============================================================================================================
//выдать текущий виджет текущего режима интрфейса, может выдать нуль
//==============================================================================================================
UMyrBioStatUserWidget* AMyrPlayerController::CurrentWidget()
{
	return  Cast<UMyrBioStatUserWidget>(*UIWidgets.Find(CurrentUIModeName)); 
}





//==============================================================================================================
// напосредственно перед началом игры, когда на уровне появляется носитель этого контроллера - это происходит рано
// однако Level Blueprint выполняется ещё раньше, и здесь всё затирается. Поэтому лучше захардкодить здесь
// все различия между типами уровней и выбор способа наэкранных свистелок на старте уровня
// увы, создание виджетов раньше не проходит
//==============================================================================================================
void AMyrPlayerController::BeginPlay()
{
	Super::BeginPlay();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//инициализация набора рельных виджетов по образцам из GameMode-потомка, определенного для данного уровня
	auto GameMode = Cast<AMyrraGameModeBase>(GetWorld()->GetAuthGameMode());
	UE_LOG(LogTemp, Log, TEXT("AMyrPlayerController BeginPlay Create Widgets from %s"), *GameMode->GetName());
	for (auto Wi : GameMode->InterfaceModes)
	{
		//создать на основе виджет-блюпринта собственно виджет
		UUserWidget* lw = CreateWidget<UUserWidget>(this, Wi.Value.Widget);
		UIWidgets.Add(Wi.Key, lw);
		UE_LOG(LogTemp, Log, TEXT("AMyrPlayerController --- add Widget %s (%s)"), *Wi.Key.ToString(), *Wi.Value.Widget->GetName());
	}

#if !WITH_EDITOR
	//если в системе еще не родился загруженный слот сохранений, это значит мы попали на уровень начальной загрузки
	if(!GetMyrGameInstance()->JustLoadedSlot)
		ChangeUIMode(TEXT("Start")); 
#endif


	//если на текущем уровне разрешён режим Play - это игровой уровень, и когда он загружен - сразу начинать с игры
	if(UIWidgets.Find(TEXT("Play"))) ChangeUIMode(TEXT("Play"));

	//сразу выставить изначальный режим интерфейса
	//для теста сразу переход к игре, а вообще здесь нужно стартовое меню
	ToggleRecentUIModeName = CurrentUIModeName; 

}

//==============================================================================================================
// здесь может производиться подключение клаиватуры и кнопок мыши к конкретным процедурам
//==============================================================================================================
void AMyrPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	//пока неясно, как делить действия в этом поле между контроллером и пешкой
	//пока чисто для разукрупнения здесь будут содержаться действия связанные с меню и интерфейсом,
	//то есть, не относящиеся к геймплею как к таковому
	InputComponent->BindAction ("Pause", IE_Pressed, this, &AMyrPlayerController::TogglePause);
	InputComponent->BindAction ("Saves", IE_Pressed, this, &AMyrPlayerController::ToggleSaves);
	InputComponent->BindAction("QuickSave", IE_Pressed, this, &AMyrPlayerController::QuickSave);
	InputComponent->BindAction("Stats", IE_Pressed, this, &AMyrPlayerController::ToggleStats);

}

//==============================================================================================================
//какой-то стандартный метод, сюда мы добавляем ограничения на вращение камеры по достижениям углов
//==============================================================================================================
void AMyrPlayerController::UpdateRotation(float DeltaTime)
{
	APlayerController::UpdateRotation(DeltaTime);
	/*if (FirstPersonLimits)
	{
		float BaseYaw = BodyOrientation->Rotation().Yaw;
		PlayerCameraManager->ViewYawMin = BaseYaw - 120;
		PlayerCameraManager->ViewYawMax = BaseYaw + 120;
	}*/
}

//==============================================================================================================
//смена режима лица
//==============================================================================================================
void AMyrPlayerController::SetFirstPerson(bool Set)
{
	if (Set) FirstPersonLimits = true;
	else
	{
		FirstPersonLimits = false;
		PlayerCameraManager->ViewYawMin = 0.f;
		PlayerCameraManager->ViewYawMax = 359.999f;
	}
}

//==============================================================================================================
//обработчики-обертки для команд включения и выключения разных меню
//здесь никаких сложных действий, просто остановка игры и отображение гуя поверх нее
//==============================================================================================================
void AMyrPlayerController::TogglePause()
{
	if(CurrentUIModeName == TEXT("Pause")) ChangeUIMode(TEXT("Play"));
	else ChangeUIMode(TEXT("Pause"));
}
void AMyrPlayerController::ToggleSaves()
{
	if(CurrentUIModeName == TEXT("Saves")) ChangeUIMode(ToggleRecentUIModeName);
	else ChangeUIMode(TEXT("Saves"));
}
void AMyrPlayerController::ToggleStats()
{
	if (CurrentUIModeName == TEXT("Stats"))	{	ChangeUIMode(ToggleRecentUIModeName);	}
	else ChangeUIMode(TEXT("Stats"));
}

//==============================================================================================================
//экран при окончании игры
//==============================================================================================================
void AMyrPlayerController::GameOverScreen()
{
	//надеть экран гейм овера, если до этого был экран игры
	if (CurrentUIModeName == TEXT("Play")) ChangeUIMode(TEXT("GameOver"));
	GetMyrGameInstance()->PostGameOverActions();
}

//==============================================================================================================
//сообщение на экран об успешном фоновом сохранении
//==============================================================================================================
void AMyrPlayerController::QuickSave()
{
	//собственно, процедура сохранения
	GetMyrGameInstance()->QuickSave();

	//если виджет на экране модифицированный, выдать ему сообщение - мож, напечатает строчку "сохранено"
	if (auto BioCurWi = Cast<UMyrBioStatUserWidget>(*UIWidgets.Find(CurrentUIModeName)))
		BioCurWi->OnSaved();
	UE_LOG(LogTemp, Log, TEXT("AMyrPlayerController QuickSave"));
}

