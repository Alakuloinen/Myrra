#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"
#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "DetailWidgetRow.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Runtime/AppFramework/Public/Widgets/Colors/SColorPicker.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h" //для SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"



TSharedRef<IPropertyTypeCustomization> FMyrLogicEventDataTypeCustomization::MakeInstance()
{	return MakeShareable(new FMyrLogicEventDataTypeCustomization);}

//==============================================================================================================
// инициализировать то, что будет рисоваться в строке заголовка свойства-структуры
//==============================================================================================================
void FMyrLogicEventDataTypeCustomization::CustomizeHeader(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//сохранить указатель на это нечто, чтобы с помощью него перериосвывать при изменении значений
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//получить рычаги влияния на те свойства, которые мы хотим видеть прямо в строке заголовка
	HandleColorForMe =			StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyrLogicEventData, EmotionDeltaForMe));
	HandleColorForGoal =		StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyrLogicEventData, EmotionDeltaForGoal));
	TSharedPtr<IPropertyHandle> HandleTimesToUseAfterCatch =	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyrLogicEventData, TimesToUseAfterCatch));
	TSharedPtr<IPropertyHandle> HandleMultEmotionByInputScalar =StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyrLogicEventData, MultEmotionByInputScalar));

	//вроде как подвязка обработчика события изменения значения свойства
	auto D = FSimpleDelegate::CreateSP(SharedThis(this), &FMyrLogicEventDataTypeCustomization::OnChanged);
	HandleColorForGoal.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, R))->SetOnPropertyValueChanged(D);
	HandleColorForGoal.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, G))->SetOnPropertyValueChanged(D);
	HandleColorForGoal.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, B))->SetOnPropertyValueChanged(D);
	HandleColorForGoal.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, A))->SetOnPropertyValueChanged(D);
	HandleColorForMe.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, R))->SetOnPropertyValueChanged(D);
	HandleColorForMe.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, G))->SetOnPropertyValueChanged(D);
	HandleColorForMe.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, B))->SetOnPropertyValueChanged(D);
	HandleColorForMe.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, A))->SetOnPropertyValueChanged(D);

	OnChanged();

	//столбец заголовка (левый)
	HeaderRow.NameContent()
	[
		SNew(SHorizontalBox)
	]
	
	//столбец значений (правый) 
	.ValueContent()
	.MinDesiredWidth(1600.0f)
	.MaxDesiredWidth(1800.0f)
	[
		SNew(SHorizontalBox)

		//квадратик с цветом
		+ SHorizontalBox::Slot().FillWidth(1)						[SAssignNew(Me, SBorder).VAlign(VAlign_Center).HAlign(HAlign_Center).BorderImage(&bme)]

		//название эмоции
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)  [SNew(STextBlock).Text(FText::FromString("Emotion (Me)")).MinDesiredWidth(0)]

		//компонентв эмоции с подписями
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString("R"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[HandleColorForMe->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, R))->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString("L"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[HandleColorForMe->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, G))->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString("F"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[HandleColorForMe->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, B))->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString("*"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[HandleColorForMe->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, A))->CreatePropertyValueWidget()]

		+ SHorizontalBox::Slot().FillWidth(1)						[ SAssignNew(Goal, SBorder).VAlign(VAlign_Center).HAlign(HAlign_Center).BorderImage(&bgo)]

		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)  [ SNew(STextBlock).Text(FText::FromString("Emotion (Goal)")).MinDesiredWidth(0)]

		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[ SNew(STextBlock).Text(FText::FromString("R"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[ HandleColorForGoal->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, R))->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[ SNew(STextBlock).Text(FText::FromString("L"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[ HandleColorForGoal->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, G))->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[ SNew(STextBlock).Text(FText::FromString("F"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[ HandleColorForGoal->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, B))->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[ SNew(STextBlock).Text(FText::FromString("*"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[ HandleColorForGoal->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, A))->CreatePropertyValueWidget()]

		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[ SNew(STextBlock).Text(FText::FromString("Aftershocks:"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[	HandleTimesToUseAfterCatch->CreatePropertyValueWidget()	]
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[ SNew(STextBlock).Text(FText::FromString("Scale By Input:"))]
		+ SHorizontalBox::Slot().FillWidth(1)						[	HandleMultEmotionByInputScalar->CreatePropertyValueWidget()	]

	];

}

//==============================================================================================================
// инициализировать то, что будет рисоваться в строках ниже заголовка структуры
//==============================================================================================================
void FMyrLogicEventDataTypeCustomization::CustomizeChildren(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//подбираем те свойства, которые мы хотим видеть в таблице ПОД заголовком
	TSharedPtr<IPropertyHandle> XPE = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyrLogicEventData, ExperienceIncrements));

	//просто добавляем строки с содержимым по умолчанию для данного типа свойств
	StructBuilder.AddProperty(XPE.ToSharedRef());

}

void FMyrLogicEventDataTypeCustomization::OnChanged()
{
	bgo.TintColor = GetColorForGoal();
	bme.TintColor = GetColorForMe();
	PropertyUtilities->RequestRefresh();

}


