#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h" //��� SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"


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
	//�������� ������ ������� �� �� ��������, ������� �� ����� ������ ����� � ������ ���������
	TSharedPtr<IPropertyHandle> Handle0 = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HardVertical));
	TSharedPtr<IPropertyHandle> Handle1 = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, FixOnFloor));
	TSharedPtr<IPropertyHandle> Handle2 = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, LegsSwingLock));
	TSharedPtr<IPropertyHandle> Handle3 = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HardCourseAlign));
	TSharedPtr<IPropertyHandle> Handle4 = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HeavyLegs));

	//��������� ������������ �����
	MainHandle = StructPropertyHandle;

	//����� ����������� ���-�� � �������� ���� ������������ 
	EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ELDY"), true);

	//������� ��������� (�����)
	HeaderRow.NameContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(10)
		[
			//����� �������� �����, thprax,pelvis
			SNew(STextBlock)
			.Text(StructPropertyHandle->GetPropertyDisplayName())
			.ColorAndOpacity(FLinearColor(255, 255, 0, 255))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		]
		+ SHorizontalBox::Slot().FillWidth(4)
		[
			//������� �������� "Leading"
			SNew(SProperty, StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Leading))).ShouldDisplayName(true)
		]

	]

	//������� �������� (������) - ����������� �� ����� ����
	.ValueContent()
		.MinDesiredWidth(1800.0f)
		.MaxDesiredWidth(1800.0f)
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
	HandleF[1] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, LegsSpread));
	HandleF[2] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, ForcesMultiplier));
	HandleF[3] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, FeetBrakeDamping));
	HandleF[4] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, CommonDampingIfNeeded));

	TSharedPtr<IPropertyHandle> HandleB[5];
	HandleB[0] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HardVertical));
	HandleB[1] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HardCourseAlign));
	HandleB[2] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, FixOnFloor));
	HandleB[3] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, LegsSwingLock));
	HandleB[4] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HeavyLegs));

	//��������� �� ������� ������� ����� ��� ������ ����
	for (int i = 0; i < 5; i++)
	{
		//��������, ��������� � ��������
		int32 Val;	HandleLimbs[i].Get()->GetValue(Val);
		DigestColor[i] = MakeDigestString(Val, Digest[i]);

		//����� ��� �������� ����������� ������� ��������� �������� ��������
		HandleLimbs[i].Get()->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FMyrGirdleModelTypeCustomization::OnChanged));

		//������ ��� ��������
		StructBuilder.AddCustomRow(HandleLimbs[i]->GetPropertyDisplayName())

			//����� ����� - ��� �������� � ��
			.NameContent()
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
					+ SHorizontalBox::Slot().MaxWidth(100)
					[
						HandleLimbs[i]->CreatePropertyNameWidget()
					]
					+ SHorizontalBox::Slot().MaxWidth(40)
					[
						HandleLimbs[i]->CreatePropertyValueWidget()
					]
				//���������� ������, ������������ �����, ������� ������ �����������
				+ SHorizontalBox::Slot().FillWidth(10)
					[
						SAssignNew(Mnemo[i], STextBlock).Text(FText::FromString(Digest[i])).ColorAndOpacity(DigestColor[i])
					]
				//������ ������� - �������� �������� �� ����������� � ����� ����� ����, �� ������ ���������� ������ �����
				+ SHorizontalBox::Slot().MaxWidth(200).FillWidth(2)
					[
						SNew(SProperty, HandleF[i]).ShouldDisplayName(true)
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
