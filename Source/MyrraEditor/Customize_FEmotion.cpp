#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SEnumCombo.h"
#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "DetailWidgetRow.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Runtime/AppFramework/Public/Widgets/Colors/SColorPicker.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h" //��� SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

#define GETP(S) MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPathia, S))


TSharedRef<IPropertyTypeCustomization> FEmotionTypeCustomization::MakeInstance()
{	return MakeShareable(new FEmotionTypeCustomization);	}

//���������� ������ ���������
void FEmotionTypeCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//��������������������������� ��� ������ �������
	MainHandle = StructPropertyHandle;

	//������������������� ������� ������
	Typicals = FindObject<UEnum>(nullptr, TEXT("/Script/Myrra.EPathia"), true);

	//��������� ��������� �� ��� �����, ����� � ������� ���� �������������� ��� ��������� ��������
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//����� ��� �������� ����������� ������� ��������� �������� ��������
	auto D = FSimpleDelegate::CreateSP(SharedThis(this), &FEmotionTypeCustomization::OnChanged);
	GETP(Rage)->SetOnPropertyValueChanged(D);
	GETP(Love)->SetOnPropertyValueChanged(D);
	GETP(Fear)->SetOnPropertyValueChanged(D);
	GETP(Work)->SetOnPropertyValueChanged(D);

	//����� ���������� ���������� ����
	OnChanged();

	//������� ��������� (�����)
	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	//������� �������� (������) - ����� ������ �� �����, ����� ��������� �� �������� �� ����������� ����� ������
	.ValueContent().MaxDesiredWidth(1800.0f).HAlign(HAlign_Fill)
	[
		SNew(SHorizontalBox)

		//��������� � ������
		+ SHorizontalBox::Slot().MaxWidth(20)	[	SAssignNew(Me, SBorder).VAlign(VAlign_Center).HAlign(HAlign_Center).BorderImage(&EquiColorBrush)	]

		//����� �� ������ ���������
		+ SHorizontalBox::Slot().FillWidth(2).MaxWidth(100)
		[	SNew(SEnumComboBox, Typicals)
			.CurrentValue(this, &FEmotionTypeCustomization::GetEnumVal)
			.OnEnumSelectionChanged(SEnumComboBox::FOnEnumSelectionChanged::CreateSP(this, &FEmotionTypeCustomization::ChangeEnumVal))
		]

		//� ������� ���������� ��������� ���� ���� �����������
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)	[ SNew(STextBlock).Text(FText::FromString("|")).ColorAndOpacity(FLinearColor(1,0,0,1))]
		+ SHorizontalBox::Slot().MaxWidth(50).VAlign(VAlign_Center) [ GETP(Rage)->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)  [ SNew(STextBlock).Text(FText::FromString("|")).ColorAndOpacity(FLinearColor(0,1,0,1))]
		+ SHorizontalBox::Slot().MaxWidth(50).VAlign(VAlign_Center) [ GETP(Love)->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)  [ SNew(STextBlock).Text(FText::FromString("|")).ColorAndOpacity(FLinearColor(0,0,1,1))]
		+ SHorizontalBox::Slot().MaxWidth(50).VAlign(VAlign_Center) [ GETP(Fear)->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)  [ SNew(STextBlock).Text(FText::FromString("|")).ColorAndOpacity(FLinearColor(1,1,1,1))]
		+ SHorizontalBox::Slot().MaxWidth(50).VAlign(VAlign_Center) [ GETP(Work)->CreatePropertyValueWidget()		]

	];

}

//���������� ������ ��� ���������� ��� ���������, �� ��� �� �����, ��� � ��������� ���������
void FEmotionTypeCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

//�����������
FEmotionTypeCustomization::FEmotionTypeCustomization():EquiColorBrush(FLinearColor::White)
{
	Typicals = FindObject<UEnum>(nullptr, TEXT("/Script/Myrra.EEmotio"), true);


}

TSharedRef<SWidget> FEmotionTypeCustomization::WidgetForEmoList()
{
	return SNew(SEnumComboBox, Typicals)
		.CurrentValue(this, &FEmotionTypeCustomization::GetEnumVal)
		.OnEnumSelectionChanged(SEnumComboBox::FOnEnumSelectionChanged::CreateSP(this, &FEmotionTypeCustomization::ChangeEnumVal));
}


//�������� ������ ����������� � ���� �����
FLinearColor FEmotionTypeCustomization::GetColorForMe() const
{	FColor Re(0,0,0,255);
	GETP(Rage)->GetValue(Re.R);
	GETP(Love)->GetValue(Re.G);
	GETP(Fear)->GetValue(Re.B);
	UE_LOG(LogTemp, Log, TEXT("FEmotionTypeCustomization::GetColorForMe %u, %u %u"), Re.R, Re.G, Re.B);
	return FLinearColor(Re);
}

//�������� �������� �� ������� ������ �� �������
void FEmotionTypeCustomization::SetValue(FPathia E)
{	GETP(Rage)->SetValue(E.Rage);
	GETP(Love)->SetValue(E.Love);
	GETP(Fear)->SetValue(E.Fear);
	GETP(Work)->SetValue(E.Work);
	UE_LOG(LogTemp, Log, TEXT("FEmotionTypeCustomization::SetValue %g, %g %g"), E.Rage, E.Love, E.Fear);

}

//���������� ��������� ����� ��������������� ��������� ������������� �����
void FEmotionTypeCustomization::OnChanged()
{
	EquiColorBrush.TintColor = GetColorForMe();
	PropertyUtilities->RequestRefresh();
}

//����� ������������� ������� �� ������ ���������
void FEmotionTypeCustomization::ChangeEnumVal(int Nv, ESelectInfo::Type Hz)
{
	SetValue(FPathia((EPathia)Nv));
}

//����� ������������� ������� �� ������ ���������
int FEmotionTypeCustomization::GetEnumVal() const
{
	FPathia Mee;
	GETP(Rage)->GetValue(Mee.Rage);
	GETP(Love)->GetValue(Mee.Love);
	GETP(Fear)->GetValue(Mee.Fear);
	GETP(Work)->GetValue(Mee.Work);
	EPathia RightArch, SecArch;
	Mee.GetFullArchetype(RightArch, SecArch);
	return (int)RightArch;
}
 