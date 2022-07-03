#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"
#include "Runtime/Launch/Resources/Version.h"
#if (ENGINE_MAJOR_VERSION >= 5)	
#include "SEnumCombo.h"
#endif
#if (ENGINE_MAJOR_VERSION == 4)	
#include "SEnumComboBox.h"
#endif
#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "DetailWidgetRow.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Runtime/AppFramework/Public/Widgets/Colors/SColorPicker.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h" //��� SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"


TSharedRef<IPropertyTypeCustomization> FEmotionTypeCustomization::MakeInstance()
{	return MakeShareable(new FEmotionTypeCustomization);	}

//���������� ������ ���������
void FEmotionTypeCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//��������������������������� ��� ������ �������
	MainHandle = StructPropertyHandle;

	//������������������� ������� ������
	Typicals = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEmotio"), true);

	//��������� ��������� �� ��� �����, ����� � ������� ���� �������������� ��� ��������� ��������
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//����� ��� �������� ����������� ������� ��������� �������� ��������
	auto D = FSimpleDelegate::CreateSP(SharedThis(this), &FEmotionTypeCustomization::OnChanged);
	GetR()->SetOnPropertyValueChanged(D);
	GetG()->SetOnPropertyValueChanged(D);
	GetB()->SetOnPropertyValueChanged(D);
	GetA()->SetOnPropertyValueChanged(D);

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
		[	
			SNew(SEnumComboBox, Typicals)
			.CurrentValue(this, &FEmoStimulusTypeCustomization::GetEnumVal)
			.OnEnumSelectionChanged(SEnumComboBox::FOnEnumSelectionChanged::CreateSP(this, &FEmoStimulusTypeCustomization::ChangeEnumVal))
		]

		//� ������� ���������� ��������� ���� ���� �����������
		+ SHorizontalBox::Slot().FillWidth(0.5).VAlign(VAlign_Center).HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString("Rage"))]
		+ SHorizontalBox::Slot().MaxWidth(50)						[ GetR()->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(0.5).VAlign(VAlign_Center).HAlign(HAlign_Right)[	SNew(STextBlock).Text(FText::FromString("Love"))]
		+ SHorizontalBox::Slot().MaxWidth(50)						[ GetG()->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(0.5).VAlign(VAlign_Center).HAlign(HAlign_Right)[	SNew(STextBlock).Text(FText::FromString("Fear"))]
		+ SHorizontalBox::Slot().MaxWidth(50)						[ GetB()->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(0.5).VAlign(VAlign_Center).HAlign(HAlign_Right)[	SNew(STextBlock).Text(FText::FromString("Sure"))]
		+ SHorizontalBox::Slot().MaxWidth(50)						[ GetA()->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(1.5)						[ Get5thWidget() ]

	];

}

//���������� ������ ��� ���������� ��� ���������, �� ��� �� �����, ��� � ��������� ���������
void FEmotionTypeCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

//�����������
FEmotionTypeCustomization::FEmotionTypeCustomization():EquiColorBrush(FLinearColor::White)
{
	Typicals = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEmotio"), true);
}

//������������� ���������� ������� �����������
TSharedPtr<class IPropertyHandle> FEmotionTypeCustomization::GetR() const
{	return MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, R));	}
TSharedPtr<class IPropertyHandle> FEmotionTypeCustomization::GetG() const
{	return MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, G));	}
TSharedPtr<class IPropertyHandle> FEmotionTypeCustomization::GetB() const
{	return MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, B));	}
TSharedPtr<class IPropertyHandle> FEmotionTypeCustomization::GetA() const
{	return MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, A));	}
TSharedRef<SWidget> FEmotionTypeCustomization::Get5thWidget()
{	return SNew(SBorder);	}

TSharedRef<SWidget> FEmotionTypeCustomization::WidgetForEmoList()
{
	return SNew(SEnumComboBox, Typicals)
		.CurrentValue(this, &FEmotionTypeCustomization::GetEnumVal)
		.OnEnumSelectionChanged(SEnumComboBox::FOnEnumSelectionChanged::CreateSP(this, &FEmotionTypeCustomization::ChangeEnumVal));
}


//�������� ������ ����������� � ���� �����
FLinearColor FEmotionTypeCustomization::GetColorForMe() const
{	FLinearColor Re(0,0,0,1);
	GetR()->GetValue(Re.R);
	GetG()->GetValue(Re.G);
	GetB()->GetValue(Re.B);
	UE_LOG(LogTemp, Log, TEXT("FEmotionTypeCustomization::GetColorForMe %g, %g %g"), Re.R, Re.G, Re.B);
	return Re;
}

//�������� �������� �� ������� ������ �� �������
void FEmotionTypeCustomization::SetValue(FEmotio E)
{	GetR()->SetValue(E.R);
	GetG()->SetValue(E.G);
	GetB()->SetValue(E.B);
	GetA()->SetValue(E.A);
	UE_LOG(LogTemp, Log, TEXT("FEmotionTypeCustomization::SetValue %g, %g %g"), E.R, E.G, E.B);

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
	SetValue(FEmotio::As((EEmotio)Nv));
}

//����� ������������� ������� �� ������ ���������
int FEmotionTypeCustomization::GetEnumVal() const
{
	FEmotio Mee(GetColorForMe());
	int RightArch = 0;
	Mee.GetArch((EEmotio*) & RightArch);
	return RightArch;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

 TSharedRef<IPropertyTypeCustomization> FEmoStimulusTypeCustomization::MakeInstance()
{	return MakeShareable(new FEmoStimulusTypeCustomization);	}

 //������������� ���������� ������� �����������
TSharedPtr<class IPropertyHandle> FEmoStimulusTypeCustomization::GetR() const
{	return MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmoStimulus, R));	}
TSharedPtr<class IPropertyHandle> FEmoStimulusTypeCustomization::GetG() const
{	return MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmoStimulus, G));	}
TSharedPtr<class IPropertyHandle> FEmoStimulusTypeCustomization::GetB() const
{	return MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmoStimulus, B));	}
TSharedPtr<class IPropertyHandle> FEmoStimulusTypeCustomization::GetA() const
{	return MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmoStimulus, A));	}
TSharedRef<SWidget> FEmoStimulusTypeCustomization::Get5thWidget()
{	return SNew(SProperty, MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmoStimulus, Aftershocks))).ShouldDisplayName(true); }

FLinearColor FEmoStimulusTypeCustomization::GetColorForMe() const
{	FLinearColor Co(0, 0, 0, 1);
	uint8 Ret = 0;
	GetR()->GetValue(Ret); Co.R = Ret / 255.0f;
	GetG()->GetValue(Ret); Co.G = Ret / 255.0f;
	GetB()->GetValue(Ret); Co.B = Ret / 255.0f;
	UE_LOG(LogTemp, Log, TEXT("FEmoStimulusTypeCustomization::GetColorForMe %g, %g %g"), Co.R, Co.G, Co.B);
	return Co;
}

//�������� �������� �� ������� ������ �� �������
void FEmoStimulusTypeCustomization::SetValue(FEmotio E)
{	GetR()->SetValue((uint8)(E.R * 255));
	GetG()->SetValue((uint8)(E.G * 255));
	GetB()->SetValue((uint8)(E.B * 255));
	GetA()->SetValue((uint8)(E.A * 255));
	UE_LOG(LogTemp, Log, TEXT("FEmoStimulusTypeCustomization::SetValue %g, %g %g"), E.R, E.G, E.B);
}

TSharedRef<SWidget> FEmoStimulusTypeCustomization::WidgetForEmoList()
{	return SNew(SEnumComboBox, Typicals)
		.CurrentValue(this, &FEmoStimulusTypeCustomization::GetEnumVal)
		.OnEnumSelectionChanged(SEnumComboBox::FOnEnumSelectionChanged::CreateSP(this, &FEmoStimulusTypeCustomization::ChangeEnumVal));
}
