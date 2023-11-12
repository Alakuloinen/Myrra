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
#include "PropertyCustomizationHelpers.h" //для SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

#define GETP(S) MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPathia, S))


TSharedRef<IPropertyTypeCustomization> FEmotionTypeCustomization::MakeInstance()
{	return MakeShareable(new FEmotionTypeCustomization);	}

//расцветить строку заголовка
void FEmotionTypeCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//головнойдескрипторсохранить для других функций
	MainHandle = StructPropertyHandle;

	//объектперечислитель опорных эмоций
	Typicals = FindObject<UEnum>(nullptr, TEXT("/Script/Myrra.EPathia"), true);

	//сохранить указатель на это нечто, чтобы с помощью него перериосвывать при изменении значений
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//вроде как подвязка обработчика события изменения значения свойства
	auto D = FSimpleDelegate::CreateSP(SharedThis(this), &FEmotionTypeCustomization::OnChanged);
	GETP(Rage)->SetOnPropertyValueChanged(D);
	GETP(Love)->SetOnPropertyValueChanged(D);
	GETP(Fear)->SetOnPropertyValueChanged(D);
	GETP(Work)->SetOnPropertyValueChanged(D);

	//сразу отобразить правильный цвет
	OnChanged();

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

		//выбор из списка архетипов
		+ SHorizontalBox::Slot().FillWidth(2).MaxWidth(100)
		[	SNew(SEnumComboBox, Typicals)
			.CurrentValue(this, &FEmotionTypeCustomization::GetEnumVal)
			.OnEnumSelectionChanged(SEnumComboBox::FOnEnumSelectionChanged::CreateSP(this, &FEmotionTypeCustomization::ChangeEnumVal))
		]

		//в строчку разместить численные поля всех компонентов
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

//расцветить строки под заголовком при раскрытии, их тут не будет, все в заголовке умещается
void FEmotionTypeCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

//конструктор
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


//получить данные компонентов в виде цвета
FLinearColor FEmotionTypeCustomization::GetColorForMe() const
{	FColor Re(0,0,0,255);
	GETP(Rage)->GetValue(Re.R);
	GETP(Love)->GetValue(Re.G);
	GETP(Fear)->GetValue(Re.B);
	UE_LOG(LogTemp, Log, TEXT("FEmotionTypeCustomization::GetColorForMe %u, %u %u"), Re.R, Re.G, Re.B);
	return FLinearColor(Re);
}

//внедрить значение из внешней эмоции по хэндлам
void FEmotionTypeCustomization::SetValue(FPathia E)
{	GETP(Rage)->SetValue(E.Rage);
	GETP(Love)->SetValue(E.Love);
	GETP(Fear)->SetValue(E.Fear);
	GETP(Work)->SetValue(E.Work);
	UE_LOG(LogTemp, Log, TEXT("FEmotionTypeCustomization::SetValue %g, %g %g"), E.Rage, E.Love, E.Fear);

}

//обработчик изменения чисел перекрашивающий квадратик получившегося цвета
void FEmotionTypeCustomization::OnChanged()
{
	EquiColorBrush.TintColor = GetColorForMe();
	PropertyUtilities->RequestRefresh();
}

//когда редактируется выбором из списка архетипов
void FEmotionTypeCustomization::ChangeEnumVal(int Nv, ESelectInfo::Type Hz)
{
	SetValue(FPathia((EPathia)Nv));
}

//когда редактируется выбором из списка архетипов
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
 