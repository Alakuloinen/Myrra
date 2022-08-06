// Fill out your copyright notice in the Description page of Project Settings.

#include "MyrPlayerController.h"
#include "../MyrraGameInstance.h"	//без этого вообще никуда
#include "../MyrraGameModeBase.h"	//без этого вообще никуда
#include "MyrraGameModeBase.h"
#include "Engine/Classes/Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "../Creature/MyrPhyCreature.h"	//чтоб спавгить демона сразу на привязи
#include "../UI/MyrBioStatUserWidget.h"	// модифицированный виджет со вложенными указателями на игрока, контроллер их переключает
#include "../UI/MyrMenuWidget.h"	// модифицированный виджет со вложенными указателями на игрока, контроллер их переключает
#include "Kismet/GameplayStatics.h"						// для вызова GetAllActorsOfClass
#include "Camera/CameraShakeBase.h"			// трясун камеры

//==============================================================================================================
// конструктор - здесь вроде как можно инициализировать виджеты
//==============================================================================================================
AMyrPlayerController::AMyrPlayerController()
{
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
	//получить доступ к gamemode - там ручками вписаны какие классы для интерфейса создавать
	auto GameMode = Cast<AMyrraGameModeBase>(GetWorld()->GetAuthGameMode());
	UE_LOG(LogTemp, Log, TEXT("AMyrPlayerController BeginPlay Create Widgets from %s"), *GameMode->GetName());

	//новый код только с 2 классами интерфейса - непосредственное создание виджетов
	if (GameMode->WidgetUI_Class && GameMode->WidgetHUD_Class)
	{
		WidgetUI = CreateWidget<UMyrMenuWidget>(this, GameMode->WidgetUI_Class);
		WidgetHUD = CreateWidget<UMyrBioStatUserWidget>(this, GameMode->WidgetHUD_Class);
		ChangeWidgetRefs(GetDaemonPawn()->GetOwnedCreature());
		ChangeWidgets();
	}

#if !WITH_EDITOR
	//если в системе еще не родился загруженный слот сохранений, это значит мы попали на уровень начальной загрузки
	if(!GetMyrGameInstance()->JustLoadedSlot)
		ChangeWidgets(EUIEntry::Start);
#endif

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
	InputComponent->BindAction ("Pause", IE_Pressed, this, &AMyrPlayerController::CmdPause);
	InputComponent->BindAction ("Saves", IE_Pressed, this, &AMyrPlayerController::CmdSaves);
	InputComponent->BindAction ("QuickSave", IE_Pressed, this, &AMyrPlayerController::CmdQuickSave);
	InputComponent->BindAction ("Stats", IE_Pressed, this, &AMyrPlayerController::CmdStats);
	InputComponent->BindAction ("Quests", IE_Pressed, this, &AMyrPlayerController::CmdQuests);
	InputComponent->BindAction ("Options", IE_Pressed, this, &AMyrPlayerController::CmdOptions);
	InputComponent->BindAction ("EmoStimuli", IE_Pressed, this, &AMyrPlayerController::CmdEmoStimuli);
	InputComponent->BindAction ("Phenes", IE_Pressed, this, &AMyrPlayerController::CmdPhenes);


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

void AMyrPlayerController::ChangeWidgetRefs(AMyrPhyCreature* PC)
{
	WidgetHUD->MyrOwner = PC;
	WidgetHUD->MyrAI = PC->MyrAI();
	WidgetUI->MyrOwner = PC;
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
//==============================================================================================================
//новая фигня - закрыть универсальный агрегатор меню вплоть до игрового худа
//==============================================================================================================
//==============================================================================================================
void AMyrPlayerController::ChangeWidgets(EUIEntry NewEntry)
{
	//достаём новый режим, найдя в карте из класса AMyrraGameModeBase
	auto GameMode = Cast<AMyrraGameModeBase>(GetWorld()->GetAuthGameMode());


	//переключение на меню конкретное
	if(NewEntry != EUIEntry::NONE)
	{
		//поискать в принципе такую команду в этом гейммоде, если нет, то вызов некорректный
		auto Data = GameMode->MenuSets.Find(NewEntry);
		if (!Data) return;

		//если замещается игровой интерфейс 
		if (!bUI)
		{
			//удалить виджет худа с экрана
			WidgetHUD->RemoveFromParent();

			//записать если нужно, новый набор меню
			WidgetUI->InitNewMenuSet(Data->AsMenu);

			//переключить внутренний набор на нужное окно
			WidgetUI->ChangeCurrentWidget(NewEntry);

			//сразу поместить на экран
			WidgetUI->AddToViewport();

			//настроить управление, характерное для меню, с курсором и кнопками
			FInputModeUIOnly InputModeData;
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::LockInFullscreen);
			InputModeData.SetWidgetToFocus(WidgetUI->TakeWidget()); //Because UMG wraps Slate
			WidgetUI->SetKeyboardFocus();

			//на всякий случай снова заложить ссылку на главного героя в виджет меню
			WidgetUI->MyrOwner = GetDaemonPawn()->OwnedCreature;

			//включить обесцвечивание подлежащей под меню сцены
			GetDaemonPawn()->SwitchToMenuPause(true);

			//это всё местные методы
			SetInputMode(InputModeData);
			bShowMouseCursor = true;
			SetTickableWhenPaused(true);
			UGameplayStatics::SetGamePaused(GetWorld(), true);
			bUI = true;
			return;
		}

		//на экране уже есть меню, но включен другой виджет
		else if (WidgetUI->CurrentWidgetId != NewEntry)
		{
			//переключить внутренний набор на нужное окно
			WidgetUI->ChangeCurrentWidget(NewEntry);
			bUI = true;
			return;
		}

		//если же меню раскрыто то же, что и запрашивается - сигнал к выключению меню до худа игры - ниже
	}
	//если функция до сих пор не закончилась, наступает черед смены режима с меню на игровой худ
	//убрать с экрана старый виджет
	WidgetUI->RemoveFromParent();

	//сразу поместить на экран
	WidgetHUD->AddToViewport();
	WidgetHUD->SetOwnerCreature(GetDaemonPawn()->OwnedCreature);
	WidgetHUD->MyrPlayerController = this;
	WidgetHUD->OnJustShowed();

	//настроить управление для нормального 3д-мира
	FInputModeGameOnly InputModeData;
	SetInputMode(InputModeData);
	bShowMouseCursor = false;
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	GetDaemonPawn()->SwitchToMenuPause(false);
	bUI = false;
}

//==============================================================================================================
//сообщение на экран об успешном фоновом сохранении
//==============================================================================================================
void AMyrPlayerController::CmdQuickSave()
{
	//собственно, процедура сохранения
	GetMyrGameInstance()->QuickSave();

}


