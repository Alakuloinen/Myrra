// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/MyrMenuWidget.h"
#include "../Control/MyrPlayerController.h"			// ��������� ������
#include "../Control/MyrDaemon.h"					// �����-��� ��� ���������� ����������, �������������� ��� ����������� � Game Mode
#include "../Creature/MyrPhyCreature.h"				// ��������, 
#include "../MyrraGameModeBase.h"					// ����� ������������ ������, �����, ����� ����������� ������ ������������, 
#include "../MyrraGameInstance.h"					// ���� ���� 
#include "GameFramework/InputSettings.h"

//�� ����� ������� ������������ ������� ����� ������� ��� ��� ������������� ����� �������� ����
void UMyrMenuWidget::ReactToKey(FKeyEvent InKeyEvent)
{
	//����� � ������� ����� ��� �������� ������ �������
	TArray<FInputActionKeyMapping> OutMappings;
	auto InputSettings = UInputSettings::GetInputSettings();
	auto ActionMa = InputSettings->GetActionMappings();

	//�� ���� ������ ������� � �������
	for (auto& ActionMap : ActionMa)
	{
		//���� � �����-�� ��������� ������ ���������� ������ ����� ������, ������� ������ ������
		if (ActionMap.Key == InKeyEvent.GetKey())
		{
			//���������, ��� ��� �������� ������������� ������ �������, ����� �� ���� �������
			UEnum *Enum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EUIEntry"));
			if(!Enum) return;
			auto MenuId = (EUIEntry)Enum->GetIndexByName(ActionMap.ActionName);

			//���� ������ �� ����� ������ �� ������� ���������/��������� ������ �������� ���� = ������� �� ���� �� ����
			if (MenuId == CurrentWidgetId)
				GetMyrPlayerController()->ChangeWidgets(EUIEntry::NONE);

			//���� ��� ������ �������, �� ��������, �� ��� ����������� ������ ������ ��� ���� ������ = ����������� ���������� �����������
			else if(MenuItemsSet && (1<<(int)MenuId)) ChangeCurrentWidget(MenuId);

			//���� � ������� ������ ���� ������ ������ ���, �� ����������� ������ � ����� ������� ����
			else GetMyrPlayerController()->ChangeWidgets(MenuId);
		}
	}
}


//�������������� ����� ����� ����, �� ���� �������, ������� ������������ ������ � �������� � � ������� ����� ������� ������ ��� �������� ��������������� �� ������� ��������� ����
bool UMyrMenuWidget::InitNewMenuSet(int32 Set)
{	
	//���� ����� ����� ���� ����� ����� �� ��� ������, �� �� ���� ������ ������
	if(Set == MenuItemsSet) return false;
	else MenuItemsSet = Set;
	
	//���������� ������� ����-��� �� ������ ����
	ClearMenuItems();

	//������ ��� � ���� � ���������� ����� ������� (� �������� � ���)
	for (int i = (int)EUIEntry::NONE+1; i < (int)EUIEntry::MAX; i++)
		if (MenuItemsSet & (1 << i)) AddMenuEntry((EUIEntry)i);

	return true;
}
