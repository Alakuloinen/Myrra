#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"
#include "SEnumCombobox.h"
#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "DetailWidgetRow.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Runtime/AppFramework/Public/Widgets/Colors/SColorPicker.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h" //для SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"


TSharedRef<IPropertyTypeCustomization> FEmotionTypeCustomization::MakeInstance()
{	return MakeShareable(new FEmotionTypeCustomization);	}

void FEmotionTypeCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	MainHandle = StructPropertyHandle;

	Typicals = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEmotio"), true);

	//сохранить указатель на это нечто, чтобы с помощью него перериосвывать при изменении значений
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//вроде как подвязка обработчика события изменения значения свойства
	auto D = FSimpleDelegate::CreateSP(SharedThis(this), &FEmotionTypeCustomization::OnChanged);
	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, R))->SetOnPropertyValueChanged(D);
	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, G))->SetOnPropertyValueChanged(D);
	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, B))->SetOnPropertyValueChanged(D);
	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, A))->SetOnPropertyValueChanged(D);

	//столбец заголовка (левый)
	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	//столбец значений (правый) - здесь ничего не будет, чтобы визуально не отвлекал от содержимого строк внутри
	.ValueContent().MaxDesiredWidth(1800.0f).HAlign(HAlign_Fill)
	[
		SNew(SHorizontalBox)

		//квадратик с цветом
		+ SHorizontalBox::Slot().MaxWidth(20)	[	SAssignNew(Me, SBorder).VAlign(VAlign_Center).HAlign(HAlign_Center).BorderImage(&EquiColorBrush)	]

		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[	SNew(STextBlock).Text(FText::FromString("Rage")) ]
		+ SHorizontalBox::Slot().MaxWidth(50)						[	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, R))->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[	SNew(STextBlock).Text(FText::FromString("Love"))]
		+ SHorizontalBox::Slot().MaxWidth(50)						[	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, G))->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[	SNew(STextBlock).Text(FText::FromString("Fear"))]
		+ SHorizontalBox::Slot().MaxWidth(50)						[	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, B))->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[	SNew(STextBlock).Text(FText::FromString("Sure"))]
		+ SHorizontalBox::Slot().MaxWidth(50)						[	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, A))->CreatePropertyValueWidget()		]

		+ SHorizontalBox::Slot().FillWidth(1).MaxWidth(50)
		[	
			SNew(SEnumComboBox, Typicals)
				.CurrentValue(this, &FEmotionTypeCustomization::GetEnumVal)
				.OnEnumSelectionChanged(SEnumComboBox::FOnEnumSelectionChanged::CreateSP(this, &FEmotionTypeCustomization::ChangeEnumVal))
		]
	];

}

void FEmotionTypeCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

FEmotionTypeCustomization::FEmotionTypeCustomization():EquiColorBrush(FLinearColor::White)
{
	Typicals = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEmotio"), true);
}

FLinearColor FEmotionTypeCustomization::GetColorForMe()
{	FLinearColor Re(0,0,0,1);
	MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, R))->GetValue(Re.R);
	MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, G))->GetValue(Re.G);
	MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEmotio, B))->GetValue(Re.B);
	return Re;
}

void FEmotionTypeCustomization::OnChanged()
{
	EquiColorBrush.TintColor = GetColorForMe();
	PropertyUtilities->RequestRefresh();
}
