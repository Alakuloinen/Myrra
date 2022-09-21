// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/MyrMenuWidget.h"
#include "../Control/MyrPlayerController.h"			// контроллр игрока
#include "../Control/MyrDaemon.h"					// пешка-дух для управления существами, регистрируется как протагонист в Game Mode
#include "../Creature/MyrPhyCreature.h"				// существо, 
#include "../MyrraGameModeBase.h"					// базис загруженного уровня, нужен, чтобы откорчевать оттуда протагониста, 
#include "../MyrraGameInstance.h"					// ядро игры 
#include "GameFramework/InputSettings.h"

//во время виджета воспринимать клавиши чтобы закрыть его или переключаться между пунктами меню
void UMyrMenuWidget::ReactToKey(FKeyEvent InKeyEvent)
{
	//найти в таблице ввода что означает данное нажатие
	TArray<FInputActionKeyMapping> OutMappings;
	auto InputSettings = UInputSettings::GetInputSettings();
	auto ActionMa = InputSettings->GetActionMappings();

	//по всем вообще записям о кнопках
	for (auto& ActionMap : ActionMa)
	{
		//если в какой-то очередной записи содержится именно такая кнопка, которая сейчас нажата
		if (ActionMap.Key == InKeyEvent.GetKey())
		{
			//убедиться, что имя действия соответствует списку менюшек, найти ид этой менюшки
			UEnum *Enum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EUIEntry"));
			if(!Enum) return;
			auto MenuId = (EUIEntry)Enum->GetIndexByName(ActionMap.ActionName);

			//если нажали ту самую кнопку по которой открывать/закрывать текуще открытое окно = закрыть всё меню до худа
			if (MenuId == CurrentWidgetId)
				GetMyrPlayerController()->ChangeWidgets(EUIEntry::NONE);

			//если это другая клавиша, то возможно, по ней открывается другой виджет для этой сборки = внутренними средствами переключить
			else if(MenuItemsSet && (1<<(int)MenuId)) ChangeCurrentWidget(MenuId);

			//если в текущем наборе меню такого пункта нет, то переоткрыть виджет с новым наоборм меню
			else GetMyrPlayerController()->ChangeWidgets(MenuId);
		}
	}
}


//переразместить новый набор меню, то есть пунктов, которые отображаются вместе с виджетом и к которым можно перейти кликом или клавишей непосредственно из данного открытого окна
bool UMyrMenuWidget::InitNewMenuSet(int32 Set)
{	
	//если новый набор меню точно такой же как старый, то не надо ничего делать
	if(Set == MenuItemsSet) return false;
	else MenuItemsSet = Set;
	
	//внутренняя очистка лист-вью от старых меню
	ClearMenuItems();

	//разбор бит в поле и добавление новых пунктов (и виджетов к ним)
	for (int i = (int)EUIEntry::NONE+1; i < (int)EUIEntry::MAX; i++)
		if (MenuItemsSet & (1 << i)) AddMenuEntry((EUIEntry)i);

	return true;
}
