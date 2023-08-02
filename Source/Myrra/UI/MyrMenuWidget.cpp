// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/MyrMenuWidget.h"
#include "../Control/MyrPlayerController.h"			// ��������� ������
#include "../Control/MyrDaemon.h"					// �����-��� ��� ���������� ����������, �������������� ��� ����������� � Game Mode
#include "../Creature/MyrPhyCreature.h"				// ��������, 
#include "../MyrraGameModeBase.h"					// ����� ������������ ������, �����, ����� ����������� ������ ������������, 
#include "../MyrraGameInstance.h"					// ���� ���� 
#include "MyrBioStatUserWidget.h"

#include "Components/WidgetSwitcher.h"				// ����� �������� ��������� ��-��� ����
#include "Components/ListView.h"					// ����� �������� ��������� ��-��� ����
#include "Components/Image.h"						// ����� �������� ��������� ��-��� ����
#include "Components/TextBlock.h"					// ����� �������� ��������� ��-��� ����

#include "GameFramework/InputSettings.h"
#include "Kismet/KismetSystemLibrary.h"

//==============================================================================================================
//�� ����� ������� ������������ ������� ����� ������� ��� ��� ������������� ����� �������� ����
//==============================================================================================================
void UMyrMenuWidget::ReactToKey(FKeyEvent InKeyEvent)
{
	//���� ������ ������ � ����, �� ������ ��� ���� �������
	if(InKeyEvent.GetKey() == EKeys::Escape)
	{	GetMyrPlayerController()->ChangeWidgets(EUIEntry::NONE);
		return;
	}

	//����� � ������� ����� ��� �������� ������ �������
	TArray<FInputActionKeyMapping> OutMappings;
	auto InputSettings = UInputSettings::GetInputSettings();
	auto ActionMa = InputSettings->GetActionMappings();

	for (auto& UIE : GetMyrraGameMode()->MenuSets)
	{	
		//������ ��� ��������� ��������� ������ ��� ��������� ������ ������� ���� (�� �����)
		InputSettings->GetActionMappingByName(UIE.Value.ActionMapName, OutMappings);
		for (auto& AM : OutMappings)
			
			//���� ����������� ��������� ������� � �������� ������������� ����� �� ������ ����
			if (AM.Key == InKeyEvent.GetKey() && AM.bAlt == InKeyEvent.IsAltDown() && AM.bCtrl == InKeyEvent.IsControlDown() && AM.bShift == InKeyEvent.IsShiftDown())
			{
				//���� ������ �� ����� ������ �� ������� ���������/��������� ������ �������� ���� = ������� �� ���� �� ����
				if (UIE.Key == CurrentWidgetId)
					GetMyrPlayerController()->ChangeWidgets(EUIEntry::NONE);

				//���� ��� ������ �������, �� ��������, �� ��� ����������� ������ ������ ��� ���� ������ = ����������� ���������� �����������
				else if (MenuItemsSet && (1 << (int)UIE.Key)) ChangeCurrentWidget(UIE.Key);

				//���� � ������� ������ ���� ������ ������ ���, �� ����������� ������ � ����� ������� ����
				else GetMyrPlayerController()->ChangeWidgets(UIE.Key);
			}
	}
}

//==============================================================================================================
//���������� ���������� ����� ��������
//==============================================================================================================
void UMyrMenuWidget::MakeWidgetCurrent(EUIEntry NewWindowCode, bool FromMenu)
{
	if (!GetMyrraGameMode()) return;
	if (!TextBlockHeader) return;

	//���� ��� � ��� ������ ������������� ����� �������, �� ���������� �����
	auto EntryData = GetMyrraGameMode()->MenuSets.Find(NewWindowCode);

	//���� ������������� ��� ��������� ������ ������������ ����� ������
	if (EntryData)
	{
		//������ ���� �������� ����� ������� �� ����� ��������������� ����� ������ � ����� ���
		if (EntryData->BackgroundMaterial)
		{
			//�������� "�����" � ������� ������� ����
			ImageCanvas->SetBrushFromMaterial(EntryData->BackgroundMaterial);
		}
		else UE_LOG(LogTemp, Log, TEXT("%s.MakeWidgetCurrent for %s no specific background"), *GetName(), *TXTENUM(EUIEntry, NewWindowCode));

		//���� ������������ ��� ������ (����� � ��������� ��� �����)
		if (EntryData->Widget.Get())
		{
			//��������� � ��������� ��������������� ��� ���� ��������
			TextBlockHeader->SetText(EntryData->HumanReadableName);

			//��������� ��� ���������� ������ �������� �� ��������� �������, ��� �������� ���� �������� �������
			//����� ������ �������� ���� ������ �� �������������� �����
			WidgetSwitcher->SetActiveWidgetIndex(IndexInWidgSwit[(int)NewWindowCode]);
			CurrentWidgetId = NewWindowCode;

			//����������� ���������� ������ ����, ���� ���� ��������� �� ���� ��� �� ������
			if(!FromMenu) ListViewMenuButtons->SetSelectedIndex(IndexInListView[(int)NewWindowCode]);
		}
		else UE_LOG(LogTemp, Log, TEXT("%s.MakeWidgetCurrent for %s no widget!"), *GetName(), *TXTENUM(EUIEntry, NewWindowCode));

		//���� ������� ���� ������ ������� ��� ������ � �������� ����������� �����-�� ���������� ��������:
		switch (NewWindowCode)
		{
			case EUIEntry::Continue:	GetMyrPlayerController()->ChangeWidgets();	break;
			case EUIEntry::NewGame:		GetMyrraGameInstance()->NewGame();			break;
			case EUIEntry::LoadLast:	GetMyrraGameInstance()->LoadLastSave();		break;
			case EUIEntry::QuickSave:	GetMyrraGameInstance()->QuickSave();		break; //��� ����� ��� ���������
			case EUIEntry::Quit:		GetMyrraGameInstance()->SaveOptions();		
				UKismetSystemLibrary::QuitGame(GetWorld(), GetMyrPlayerController(), EQuitPreference::Quit, false);
				break;
		}
	}
	else UE_LOG(LogTemp, Error, TEXT("%s.MakeWidgetCurrent no entry for menu comand %s"), *GetName(), *TXTENUM(EUIEntry, NewWindowCode));
}

//==============================================================================================================
//�������� ������ � ������ ����� ����� ����� ����
//==============================================================================================================
void UMyrMenuWidget::AddMenuEntry(EUIEntry NewWindowCode)
{
	if (!GetMyrraGameMode()) return;
	if (!TextBlockHeader) return;

	//������� ������������, ��� ��� � ��� ������ ���� ������������� ����� �������
	auto EntryData = GetMyrraGameMode()->MenuSets.Find(NewWindowCode);
	if (EntryData)
	{
		//����� ��� ���� ���-�� ��������� ������ ���� ����� ������ ���������
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
//�������������� ����� ����� ����, �� ���� �������, ������� ������������ ������ � ������ ��������
// � � ������� ����� ������� ������ ��� �������� ��������������� �� ������� ��������� ����
//===============================================================================================================
bool UMyrMenuWidget::InitNewMenuSet(int32 Set)
{	
	//���� ����� ����� ���� ����� ����� �� ��� ������, �� �� ���� ������ ������
	if(Set == MenuItemsSet) return false;
	else MenuItemsSet = Set;
	UE_LOG(LogTemp, Log, TEXT("%s.InitNewMenuSet %g"), *GetName(), Set);
	
	//���������� ������� ����-��� �� ������ ����
	ClearMenuItems();

	//������ ��� � ���� � ���������� ����� ������� (� �������� � ���)
	for (int i = (int)EUIEntry::NONE+1; i < (int)EUIEntry::MAX; i++)
		if (MenuItemsSet & (1 << i)) AddMenuEntry((EUIEntry)i);

	return true;
}

//===============================================================================================================
// ������� ���� ������ ������� ����
//===============================================================================================================
void UMyrMenuWidget::ClearMenuItems()
{
	if (!ListViewMenuButtons) return;
	ListViewMenuButtons->ClearListItems();
	UE_LOG(LogTemp, Log, TEXT("%s.ClearMenuItems"), *GetName());
}
