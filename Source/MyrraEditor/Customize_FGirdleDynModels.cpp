#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h" //��� SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

TSharedRef<IPropertyTypeCustomization> FMyrDynModelTypeCustomization::MakeInstance()
{	return MakeShareable(new FMyrDynModelTypeCustomization);}


//==============================================================================================================
// ���������������� ��, ��� ����� ���������� � ������ ��������� ��������-���������
//==============================================================================================================
void FMyrDynModelTypeCustomization::CustomizeHeader(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//�������� ������ ������� �� �� ��������, ������� �� ����� ������ ����� � ������ ���������
	TSharedPtr<IPropertyHandle> HandleAnimRate = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, AnimRate));
	TSharedPtr<IPropertyHandle> HandleHealthAdd = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, HealthAdd));
	TSharedPtr<IPropertyHandle> HandleStaminaAdd = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, StaminaAdd));
	TSharedPtr<IPropertyHandle> HandleMotionGain = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, MotionGain));
	TSharedPtr<IPropertyHandle> HandleJumpImp = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, JumpImpulse));
	TSharedPtr<IPropertyHandle> HandleAutoMove = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, MoveWithNoExtGain));
	TSharedPtr<IPropertyHandle> HandlePreJump = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, PreJump));
	TSharedPtr<IPropertyHandle> HandleFlyFixed = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, FlyFixed));
	TSharedPtr<IPropertyHandle> HandleSpineStiffness = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, SpineStiffness));

	TSharedPtr<IPropertyHandle> HandleSound = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Sound));

	HandleUse = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Use));
	HandleUse->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FMyrDynModelTypeCustomization::OnUsedChanged));

	//��� ��������� ����� ��� ����� ������� ���������� ����� � � ��� �������� ����������� ���� ����, ��� ���� �� ��� ���� ����������� ��� �����
	FString FullHead = StructPropertyHandle->GetPropertyDisplayName().ToString();
	if (FullHead == TEXT("0")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::ASCEND); }
	if (FullHead == TEXT("1")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::READY); }
	if (FullHead == TEXT("2")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::RUSH); }
	if (FullHead == TEXT("3")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::STRIKE); }
	if (FullHead == TEXT("4")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::FINISH); }
	if (FullHead == TEXT("5")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::DESCEND); }

	//������� ��������� (�����)
	HeaderRow.NameContent()
	[
		//����� � ����� �������� ������� ��������� �� ���������� �����
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(20)
		[
			//������ ������������ ������� ������� �����, ���� ������ �� ����� �������� - ����� ����� ����� ���� ��������� ������
			//� ������ ������ ��� ������ ����� �������� � �������
			SNew(STextBlock).Text(FText::FromString(FullHead))
			.ColorAndOpacity(FLinearColor(1, 0, 1, 1))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		]
		//��������� ������ - ��� �������� � ������ ������ ����� ��� ����������� ������ (���� ���-�� �������� ������)
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)
		[	HandleUse->CreatePropertyNameWidget()	]
		+ SHorizontalBox::Slot().FillWidth(2).HAlign(HAlign_Right)
			[HandleUse->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)
		[	HandleSound->CreatePropertyNameWidget()	]
		+ SHorizontalBox::Slot().FillWidth(10)
		[	SNew(SObjectPropertyEntryBox).PropertyHandle(HandleSound).DisplayThumbnail(false) ]
	]

	//������� �������� (������) - ����� ������ �� �����, ����� ��������� �� �������� �� ����������� ����� ������
	.ValueContent()
	.MinDesiredWidth(1200.0f)
	.MaxDesiredWidth(1800.0f)
	[
		//� ��������� 4 ������ ����� �������� ��������� �������� 4 float ��������, ���� �� ������
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(10).HAlign(HAlign_Right)	[	HandleAnimRate->CreatePropertyNameWidget()		]
		+ SHorizontalBox::Slot().FillWidth(5)						[	HandleAnimRate->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(13).HAlign(HAlign_Right)	[	HandleHealthAdd->CreatePropertyNameWidget()		]
		+ SHorizontalBox::Slot().FillWidth(5)						[	HandleHealthAdd->CreatePropertyValueWidget()	]
		+ SHorizontalBox::Slot().FillWidth(5)						[	HandleStaminaAdd->CreatePropertyValueWidget()	]
		+ SHorizontalBox::Slot().FillWidth(13).HAlign(HAlign_Right) [	HandleMotionGain->CreatePropertyNameWidget()	]
		+ SHorizontalBox::Slot().FillWidth(5)						[	HandleMotionGain->CreatePropertyValueWidget()	]
		+ SHorizontalBox::Slot().FillWidth(5)						[	HandleJumpImp->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(10).HAlign(HAlign_Right)	[	HandleSpineStiffness->CreatePropertyNameWidget()]
		+ SHorizontalBox::Slot().FillWidth(5)						[	HandleSpineStiffness->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[	HandleAutoMove->CreatePropertyNameWidget()		]
		+ SHorizontalBox::Slot().FillWidth(2)						[	HandleAutoMove->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[	HandlePreJump->CreatePropertyNameWidget()		]
		+ SHorizontalBox::Slot().FillWidth(2)						[	HandlePreJump->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[	HandleFlyFixed->CreatePropertyNameWidget()		]
		+ SHorizontalBox::Slot().FillWidth(2)						[	HandleFlyFixed->CreatePropertyValueWidget()		]

	];
}

//==============================================================================================================
// ���������������� ��, ��� ����� ���������� � ������� ���� ��������� ���������
//==============================================================================================================
void FMyrDynModelTypeCustomization::CustomizeChildren(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//��������� �� ��������, ������� �� ����� ������ � ������� ��� ����������
	TSharedPtr<IPropertyHandle> HandleThorax = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Thorax));
	TSharedPtr<IPropertyHandle> HandlePelvis = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Pelvis));

	//������ ��������� ������ � ���������� �� ��������� ��� ������� ���� �������
	IDetailPropertyRow& PropertyThoraxRow = StructBuilder.AddProperty(HandleThorax.ToSharedRef());
	IDetailPropertyRow& PropertyPelvisRow = StructBuilder.AddProperty(HandlePelvis.ToSharedRef());

	
	if(PropertyThoraxRow.CustomNameWidget())
		ThoN = PropertyThoraxRow.CustomNameWidget()->Widget;
	else PropertyThoraxRow.GetDefaultWidgets(ThoN, ThoV);

	if (PropertyThoraxRow.CustomValueWidget())
		ThoV = PropertyThoraxRow.CustomValueWidget()->Widget;
	else PropertyThoraxRow.GetDefaultWidgets(ThoV, ThoV);

	if (PropertyPelvisRow.CustomNameWidget())
		PelN = PropertyPelvisRow.CustomNameWidget()->Widget;
	else PropertyPelvisRow.GetDefaultWidgets(PelN, PelV);

	if (PropertyPelvisRow.CustomValueWidget())
		PelV = PropertyPelvisRow.CustomValueWidget()->Widget;
	else PropertyPelvisRow.GetDefaultWidgets(PelV, PelV);
}

//==============================================================================================================
//���������� �������
//==============================================================================================================
void FMyrDynModelTypeCustomization::OnUsedChanged()
{
	if (!HandleUse.IsValid()) return;
	bool Val; HandleUse->GetValue(Val);
	if (Val)
	{
		ThoN->SetVisibility(EVisibility::Visible);
		ThoV->SetVisibility(EVisibility::Visible);
	}
	else
	{
		ThoN->SetVisibility(EVisibility::Collapsed);
		ThoV->SetVisibility(EVisibility::Collapsed);

	}
	if (PropertyUtilities.IsValid())
		PropertyUtilities->RequestRefresh();
}

//��������������������������������������������������������������������������������������������������
//��������������������������������������������������������������������������������������������������
//��������������������������������������������������������������������������������������������������
//��������������������������������������������������������������������������������������������������


TSharedRef<IPropertyTypeCustomization> FMyrGirdleModelTypeCustomization::MakeInstance()
{
	return MakeShareable(new FMyrGirdleModelTypeCustomization);
}

//==============================================================================================================
// ���������������� ��, ��� ����� ���������� � ������ ��������� ��������-���������
//==============================================================================================================
void FMyrGirdleModelTypeCustomization::CustomizeHeader(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//��������� ������������ �����
	MainHandle = StructPropertyHandle;

	//����� ����������� ���-�� � �������� ���� ������������ 
	EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ELDY"), true);

	//��� �������� ���� ������� ��������� ����������� ���������
	HandleUsed = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Use));
	HandleUsed->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FMyrGirdleModelTypeCustomization::OnUsedChanged));


	//������� ��������� (�����)
	HeaderRow.NameContent()
		.HAlign(EHorizontalAlignment::HAlign_Fill)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			//����� �������� �����, thprax,pelvis
			SNew(STextBlock)
			.Text(StructPropertyHandle->GetPropertyDisplayName())
			.ColorAndOpacity(FLinearColor(255, 255, 0, 255))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		]
		+ SHorizontalBox::Slot().FillWidth(10).HAlign(EHorizontalAlignment::HAlign_Left)
		[
			//������� �������� "Used"
			SNew(SProperty, HandleUsed).ShouldDisplayName(true)
		]
		//������� �������� "Leading"
		+ SHorizontalBox::Slot().AutoWidth()
		[	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Leading))->CreatePropertyNameWidget()	]
		+ SHorizontalBox::Slot().AutoWidth()
		[	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Leading))->CreatePropertyValueWidget()		]

	]

	//������� �������� (������) - ����������� �� ����� ����
	.ValueContent()
		.MinDesiredWidth(2000.0f)
		.MaxDesiredWidth(2000.0f)
		[
			//�������, ����� �����-����������� ��-�� ����� ������ �������, �� ����� �� ��� ����������
			SNew(STextBlock)
			.Text(FText::FromString("================================================================================================================================="))
			.ColorAndOpacity(FLinearColor(255, 0, 255, 255))

		];
}

//==============================================================================================================
// ���������������� ��, ��� ����� ���������� � ������� ���� ��������� ���������
//==============================================================================================================
void FMyrGirdleModelTypeCustomization::CustomizeChildren(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//��������� ��������� �� ��� �����, ����� � ������� ���� �������������� ��� ��������� ��������
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//��������� �� ��������, ������� �� ����� ������ � ���� �������, ��� ����������
	HandleLimbs[0] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Center));
	HandleLimbs[1] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Left));
	HandleLimbs[2] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Right));
	HandleLimbs[3] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Spine));
	HandleLimbs[4] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Tail));

	TSharedPtr<IPropertyHandle> HandleF[5];
	HandleF[0] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Crouch));
	HandleF[1] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, LegsSpreadMin));
	HandleF[2] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, LegsSpreadMax));
	HandleF[3] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, ForcesMultiplier));
	HandleF[4] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, CommonDampingIfNeeded));

	TSharedPtr<IPropertyHandle> HandleB[5];
	HandleB[0] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HardVertical));
	HandleB[1] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HardCourseAlign));
	HandleB[2] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, FixOnFloor));
	HandleB[3] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, LegsSwingLock));
	HandleB[4] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, NormalAlign));

	//��������� �� ������� ������� ����� ��� ������ ����
	for (int i = 0; i < 5; i++)
	{
		//��������, ��������� � ��������
		int32 Val;	HandleLimbs[i].Get()->GetValue(Val);
		DigestColor[i] = MakeDigestString(Val, Digest[i]);

		//����� ��� �������� ����������� ������� ��������� �������� ��������
		HandleLimbs[i].Get()->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FMyrGirdleModelTypeCustomization::OnChanged));

		//����� ����� - ��� �������� � ��
		RowWidgets[i] = StructBuilder.AddCustomRow(HandleLimbs[i]->GetPropertyDisplayName()).NameContent()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("===")))
				.ColorAndOpacity(FLinearColor(0.5, 0.5, 0.5, 1))
			]
			//������ �����, ������ ���������� ����� ������ ���� ��� ��� ���� �����, ��� ��� ����� ����� 
			.ValueContent().HAlign(HAlign_Fill)
			[
					//� ����� ������ ����������� ������ ��������, ��������� � ������, �����, �� ������ - ������ �����������
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().MaxWidth(80)
					[
						HandleLimbs[i]->CreatePropertyNameWidget()
					]
					+ SHorizontalBox::Slot().MaxWidth(30)
					[
						HandleLimbs[i]->CreatePropertyValueWidget()
					]
				//���������� ������, ������������ �����, ������� ������ �����������
				+ SHorizontalBox::Slot().FillWidth(10)
					[
						SAssignNew(Mnemo[i], STextBlock).Text(FText::FromString(Digest[i])).ColorAndOpacity(DigestColor[i])
					]
				//������ ������� - �������� �������� �� ����������� � ����� ����� ����, �� ������ ���������� ������ �����
				+ SHorizontalBox::Slot().HAlign(EHorizontalAlignment::HAlign_Fill).FillWidth(2)
					[
						HandleF[i]->CreatePropertyNameWidget()
					]
				+ SHorizontalBox::Slot().MaxWidth(50)
					[
						HandleF[i]->CreatePropertyValueWidget()
					]
				//��� ������ �������� ��� ������ - ��������� �� ��������, ����� ������-������� �������� �� ������, ��� ��� ����
				+ SHorizontalBox::Slot()
					.MaxWidth(150)
					.FillWidth(2)
					.HAlign(HAlign_Right)
					[
						HandleB[i]->CreatePropertyNameWidget()
					]
				+ SHorizontalBox::Slot()
					.MaxWidth(20)
					[
						HandleB[i]->CreatePropertyValueWidget()
					]
			];
	}

}

//==============================================================================================================
//���������� ����� ����� � ������ ���������, ��� ������������ �������� - ������� ������ �����������
//==============================================================================================================
FLinearColor FMyrGirdleModelTypeCustomization::MakeDigestString(int32 Bits, FString& OutStr)
{
	//���������������, ��� ��������� �� ������������ ��� ��� �������� � ��������� ������������� �����
	if (!Bits)
	{
		OutStr = TEXT("Passive");
		return FLinearColor(0.3, 0.3, 0.3);
	}
	else
	{
		//�������� �� ����������
		OutStr = TEXT("");

		//���� ������ ��������� ��������� ���, �������� � ������ ��, ��� ���� ��� ������
		for (int i = 0; i < EnumPtr->GetMaxEnumValue(); i++)
			if (Bits & (1 << i))	OutStr += EnumPtr->GetDisplayNameTextByIndex(i).ToString() + " ";

		//����������, ������ �������, ������� ������ ������������ ��������� ����� ������
		if (Bits & LDY_GRAVITY)
		{
			if (Bits == LDY_GRAVITY) FLinearColor(0.4, 0.4, 0.4);
			else return FLinearColor(0.8, 0.8, 0.8);
		}
		else return FLinearColor(0.5, 0.5, 0.5);

	}
	return FLinearColor(0.5, 0.5, 0.5);
}

//==============================================================================================================
//���������� �������
//==============================================================================================================
void FMyrGirdleModelTypeCustomization::OnChanged()
{
	//������������ � ������
	for (int i = 0; i < 5; i++)
	{
		if (!HandleLimbs[i].IsValid()) continue;
		if (!Mnemo[i].IsValid()) continue;
		int32 Val;
		HandleLimbs[i].Get()->GetValue(Val);
		DigestColor[i] = MakeDigestString(Val, Digest[i]);
		Mnemo[i]->SetText(FText::FromString(Digest[i]));
		Mnemo[i]->SetColorAndOpacity(DigestColor[i]);
	}
	if (PropertyUtilities.IsValid())
		PropertyUtilities->RequestRefresh();
}

void FMyrGirdleModelTypeCustomization::OnUsedChanged()
{
	if(!HandleUsed.IsValid()) return;
	bool Val; HandleUsed->GetValue(Val);
	if(Val)
	{
		for (int i = 0; i < 5; i++)
		{
			RowWidgets[i].WholeRowContent().Widget->SetVisibility(EVisibility::Visible);
			RowWidgets[i].ValueWidget.Widget->SetVisibility(EVisibility::Visible);
			RowWidgets[i].NameWidget.Widget->SetVisibility(EVisibility::Visible);
		}
	}
	else
	{
		for (int i = 0; i < 5; i++)
		{
			RowWidgets[i].WholeRowContent().Widget->SetVisibility(EVisibility::Collapsed);
			RowWidgets[i].ValueWidget.Widget->SetVisibility(EVisibility::Collapsed);
			RowWidgets[i].NameWidget.Widget->SetVisibility(EVisibility::Collapsed);
		}
	}
	if (PropertyUtilities.IsValid())
		PropertyUtilities->RequestRefresh();
}
