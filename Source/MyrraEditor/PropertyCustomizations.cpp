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

TSharedRef<IPropertyTypeCustomization> FMyrDynModelTypeCustomization::MakeInstance()
{	return MakeShareable(new FMyrDynModelTypeCustomization);}


//==============================================================================================================
// инициализировать то, что будет рисоваться в строке заголовка свойства-структуры
//==============================================================================================================
void FMyrDynModelTypeCustomization::CustomizeHeader(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//получить рычаги влияния на те свойства, которые мы хотим видеть прямо в строке заголовка
	TSharedPtr<IPropertyHandle> HandleAnimRate = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, AnimRate));
	TSharedPtr<IPropertyHandle> HandleHealthAdd = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, HealthAdd));
	TSharedPtr<IPropertyHandle> HandleStaminaAdd = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, StaminaAdd));
	TSharedPtr<IPropertyHandle> HandleMotionGain = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, MotionGain));
	TSharedPtr<IPropertyHandle> HandleSound = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Sound));

	//для заголовка таким вот тупым образом распознать цифры и к ним добавить эквиваленты этих цифр, как если бы они были константами фаз атаки
	FString FullHead = StructPropertyHandle->GetPropertyDisplayName().ToString();
	if (FullHead == TEXT("0")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::ASCEND); }
	if (FullHead == TEXT("1")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::READY); }
	if (FullHead == TEXT("2")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::RUSH); }
	if (FullHead == TEXT("3")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::STRIKE); }
	if (FullHead == TEXT("4")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::FINISH); }
	if (FullHead == TEXT("5")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::DESCEND); }

	//столбец заголовка (левый)
	HeaderRow.NameContent()
	[
		//прямо в левой половине заводим контейнер из нескольких ячеек
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(20)
		[
			//вместо стандартного виджета создать текст, взяв строку из имени свойства - чтобы текст можно было покрасить цветом
			//в данном случае это просто номер элемента в массиве
			SNew(STextBlock).Text(FText::FromString(FullHead))
			.ColorAndOpacity(FLinearColor(1, 0, 1, 1))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		]
		//последние ячейка - под название и виджет ассета звука без громоздкого ярлыка (надо как-то добавить фильтр)
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)
		[	HandleSound->CreatePropertyNameWidget()	]
		+ SHorizontalBox::Slot().FillWidth(10)
		[	SNew(SObjectPropertyEntryBox).PropertyHandle(HandleSound).DisplayThumbnail(false) ]
	]

	//столбец значений (правый) - здесь ничего не будет, чтобы визуально не отвлекал от содержимого строк внутри
	.ValueContent()
	.MinDesiredWidth(1200.0f)
	.MaxDesiredWidth(1800.0f)
	[
		//в остальные 4 ячейки левой половины заголовка засунуть 4 float свойства, друг за другом
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(10)	[	SNew(SProperty, HandleAnimRate).ShouldDisplayName(true)		]
		+ SHorizontalBox::Slot().FillWidth(10)	[	SNew(SProperty, HandleHealthAdd).ShouldDisplayName(true)	]
		+ SHorizontalBox::Slot().FillWidth(10)	[	SNew(SProperty, HandleStaminaAdd).ShouldDisplayName(true)	]
		+ SHorizontalBox::Slot().FillWidth(10)	[	SNew(SProperty, HandleMotionGain).ShouldDisplayName(true)	]

	];
}

//==============================================================================================================
// инициализировать то, что будет рисоваться в строках ниже заголовка структуры
//==============================================================================================================
void FMyrDynModelTypeCustomization::CustomizeChildren(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//подбираем те свойства, которые мы хотим видеть в таблице ПОД заголовком
	TSharedPtr<IPropertyHandle> HandleThorax = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Thorax));
	TSharedPtr<IPropertyHandle> HandlePelvis = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Pelvis));

	//просто добавляем строки с содержимым по умолчанию для данного типа свойств
	IDetailPropertyRow& PropertyThoraxRow = StructBuilder.AddProperty(HandleThorax.ToSharedRef());
	IDetailPropertyRow& PropertyPelvisRow = StructBuilder.AddProperty(HandlePelvis.ToSharedRef());
}


//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№



//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№

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


