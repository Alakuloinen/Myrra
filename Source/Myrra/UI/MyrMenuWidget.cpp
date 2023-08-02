// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/MyrMenuWidget.h"
#include "../Control/MyrPlayerController.h"			// контроллр игрока
#include "../Control/MyrDaemon.h"					// пешка-дух для управления существами, регистрируется как протагонист в Game Mode
#include "../Creature/MyrPhyCreature.h"				// существо, 
#include "../MyrraGameModeBase.h"					// базис загруженного уровня, нужен, чтобы откорчевать оттуда протагониста, 
#include "../MyrraGameInstance.h"					// ядро игры 
#include "MyrBioStatUserWidget.h"

#include "Components/WidgetSwitcher.h"				// чтобы ворочать виджетами из-под кода
#include "Components/ListView.h"					// чтобы ворочать виджетами из-под кода
#include "Components/Image.h"						// чтобы ворочать виджетами из-под кода
#include "Components/TextBlock.h"					// чтобы ворочать виджетами из-под кода

#include "GameFramework/InputSettings.h"
#include "Kismet/KismetSystemLibrary.h"

//==============================================================================================================
//во время виджета воспринимать клавиши чтобы закрыть его или переключаться между пунктами меню
//==============================================================================================================
void UMyrMenuWidget::ReactToKey(FKeyEvent InKeyEvent)
{
	//если нажали эскейп в меню, по любому его надо закрыть
	if(InKeyEvent.GetKey() == EKeys::Escape)
	{	GetMyrPlayerController()->ChangeWidgets(EUIEntry::NONE);
		return;
	}

	//найти в таблице ввода что означает данное нажатие
	TArray<FInputActionKeyMapping> OutMappings;
	auto InputSettings = UInputSettings::GetInputSettings();
	auto ActionMa = InputSettings->GetActionMappings();

	for (auto& UIE : GetMyrraGameMode()->MenuSets)
	{	
		//выдать все возможные сочетания клавиш для очередной записи команды меню (по имени)
		InputSettings->GetActionMappingByName(UIE.Value.ActionMapName, OutMappings);
		for (auto& AM : OutMappings)
			
			//если прилетевшее сочитание клавишь в точности соответствует одной из команд меню
			if (AM.Key == InKeyEvent.GetKey() && AM.bAlt == InKeyEvent.IsAltDown() && AM.bCtrl == InKeyEvent.IsControlDown() && AM.bShift == InKeyEvent.IsShiftDown())
			{
				//если нажали ту самую кнопку по которой открывать/закрывать текуще открытое окно = закрыть всё меню до худа
				if (UIE.Key == CurrentWidgetId)
					GetMyrPlayerController()->ChangeWidgets(EUIEntry::NONE);

				//если это другая клавиша, то возможно, по ней открывается другой виджет для этой сборки = внутренними средствами переключить
				else if (MenuItemsSet && (1 << (int)UIE.Key)) ChangeCurrentWidget(UIE.Key);

				//если в текущем наборе меню такого пункта нет, то переоткрыть виджет с новым наоборм меню
				else GetMyrPlayerController()->ChangeWidgets(UIE.Key);
			}
	}
}

//==============================================================================================================
//внутренняя реализация смены виджетов
//==============================================================================================================
void UMyrMenuWidget::MakeWidgetCurrent(EUIEntry NewWindowCode, bool FromMenu)
{
	if (!GetMyrraGameMode()) return;
	if (!TextBlockHeader) return;

	//если тык в эту кнопку подразумевает вывод виджета, то попытаться найти
	auto EntryData = GetMyrraGameMode()->MenuSets.Find(NewWindowCode);

	//если действительно для заданного уровня предусмотрен такой виджет
	if (EntryData)
	{
		//данное окно настроек может повлечь за собой переукрашивания всего экрана в новый фон
		if (EntryData->BackgroundMaterial)
		{
			//заменить "кисть" в виджете полотна фона
			ImageCanvas->SetBrushFromMaterial(EntryData->BackgroundMaterial);
		}
		else UE_LOG(LogTemp, Log, TEXT("%s.MakeWidgetCurrent for %s no specific background"), *GetName(), *TXTENUM(EUIEntry, NewWindowCode));

		//если предусмотрен сам виджет (задан в блюпринте его класс)
		if (EntryData->Widget.Get())
		{
			//поместить в заголовок предусмотренное для окна название
			TextBlockHeader->SetText(EntryData->HumanReadableName);

			//поскольку при заполнении стойла виджетов мы сохранили индексы, под которыми туда ложились виджеты
			//можно просто получить этот индекс из идентификатора опции
			WidgetSwitcher->SetActiveWidgetIndex(IndexInWidgSwit[(int)NewWindowCode]);
			CurrentWidgetId = NewWindowCode;

			//перекрасить выделенную кнопку меню, если окно вызвалось из игры или по кнопке
			if(!FromMenu) ListViewMenuButtons->SetSelectedIndex(IndexInListView[(int)NewWindowCode]);
		}
		else UE_LOG(LogTemp, Log, TEXT("%s.MakeWidgetCurrent for %s no widget!"), *GetName(), *TXTENUM(EUIEntry, NewWindowCode));

		//если команда меню вместо виджета или вместе с виджетом запрашивала какое-то глобальное действие:
		switch (NewWindowCode)
		{
			case EUIEntry::Continue:	GetMyrPlayerController()->ChangeWidgets();	break;
			case EUIEntry::NewGame:		GetMyrraGameInstance()->NewGame();			break;
			case EUIEntry::LoadLast:	GetMyrraGameInstance()->LoadLastSave();		break;
			case EUIEntry::QuickSave:	GetMyrraGameInstance()->QuickSave();		break; //тут нужно еще сообщение
			case EUIEntry::Quit:		GetMyrraGameInstance()->SaveOptions();		
				UKismetSystemLibrary::QuitGame(GetWorld(), GetMyrPlayerController(), EQuitPreference::Quit, false);
				break;
		}
	}
	else UE_LOG(LogTemp, Error, TEXT("%s.MakeWidgetCurrent no entry for menu comand %s"), *GetName(), *TXTENUM(EUIEntry, NewWindowCode));
}

//==============================================================================================================
//добавить внутри в нужное место новый пункт меню
//==============================================================================================================
void UMyrMenuWidget::AddMenuEntry(EUIEntry NewWindowCode)
{
	if (!GetMyrraGameMode()) return;
	if (!TextBlockHeader) return;

	//сначала предположить, что тык в эту кнопку меню подразумевает вывод виджета
	auto EntryData = GetMyrraGameMode()->MenuSets.Find(NewWindowCode);
	if (EntryData)
	{
		//здесь еще надо как-то добавлять кнопку меню через объект прослойку
		auto ItemObj = NewObject<UMyrMenuItem>(ListViewMenuButtons);
		ItemObj->CurrentWidgetId = NewWindowCode;
		ItemObj->EntryData = *EntryData;
		ListViewMenuButtons->AddItem(ItemObj);
		IndexInListView[(int)NewWindowCode] = ListViewMenuButtons->GetListItems().Num()-1;
		UE_LOG(LogTemp, Log, TEXT("%s.AddMenuEntry %s(%s)"), *GetName(), *TXTENUM(EUIEntry, NewWindowCode), *EntryData->HumanReadableName.ToString());

		if (EntryData->Widget)
		{

			//auto NewWidget = CreateWidget<UWidget>(this, EntryData->Widget);
			auto NewWidget = WidgetTree->ConstructWidget<UWidget>(EntryData->Widget);
			auto MyrWi = Cast<UMyrBioStatUserWidget>(NewWidget);
			if (MyrWi)
			{	MyrWi->MyrOwner = MyrOwner;
				MyrWi->MyrPlayerController = GetMyrPlayerController();
			}
			WidgetSwitcher->AddChild(NewWidget);
			IndexInWidgSwit[(int)NewWindowCode] = WidgetSwitcher->GetSlots().Num()-1;
			UE_LOG(LogTemp, Log, TEXT("%s.AddMenuEntry %s widget %s"), *GetName(), *TXTENUM(EUIEntry, NewWindowCode), *NewWidget->GetName());
		}
	}

}

//==============================================================================================================
//переразместить новый набор меню, то есть пунктов, которые отображаются вместе с данным виджетом
// и к которым можно перейти кликом или клавишей непосредственно из данного открытого окна
//===============================================================================================================
bool UMyrMenuWidget::InitNewMenuSet(int32 Set)
{	
	//если новый набор меню точно такой же как старый, то не надо ничего делать
	if(Set == MenuItemsSet) return false;
	else MenuItemsSet = Set;
	UE_LOG(LogTemp, Log, TEXT("%s.InitNewMenuSet %g"), *GetName(), Set);
	
	//внутренняя очистка лист-вью от старых меню
	ClearMenuItems();

	//разбор бит в поле и добавление новых пунктов (и виджетов к ним)
	for (int i = (int)EUIEntry::NONE+1; i < (int)EUIEntry::MAX; i++)
		if (MenuItemsSet & (1 << i)) AddMenuEntry((EUIEntry)i);

	return true;
}

//===============================================================================================================
// удалить весь список пунктов меню
//===============================================================================================================
void UMyrMenuWidget::ClearMenuItems()
{
	if (!ListViewMenuButtons) return;
	ListViewMenuButtons->ClearListItems();
	UE_LOG(LogTemp, Log, TEXT("%s.ClearMenuItems"), *GetName());
}
