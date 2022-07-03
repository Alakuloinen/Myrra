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
#include "PropertyCustomizationHelpers.h" //для SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"


TSharedRef<IPropertyTypeCustomization> FEmotionTypeCustomization::MakeInstance()
{	return MakeShareable(new FEmotionTypeCustomization);	}

//расцветить строку заголовка
void FEmotionTypeCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//головнойдескрипторсохранить для других функций
	MainHandle = StructPropertyHandle;

	//объектперечислитель опорных эмоций
	Typicals = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEmotio"), true);

	//сохранить указатель на это нечто, чтобы с помощью него перериосвывать при изменении значений
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//вроде как подвязка обработчика события изменения значения свойства
	auto D = FSimpleDelegate::CreateSP(SharedThis(this), &FEmotionTypeCustomization::OnChanged);
	GetR()->SetOnPropertyValueChanged(D);
	GetG()->SetOnPropertyValueChanged(D);
	GetB()->SetOnPropertyValueChanged(D);
	GetA()->SetOnPropertyValueChanged(D);

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
		[	
			SNew(SEnumComboBox, Typicals)
			.CurrentValue(this, &FEmoStimulusTypeCustomization::GetEnumVal)
			.OnEnumSelectionChanged(SEnumComboBox::FOnEnumSelectionChanged::CreateSP(this, &FEmoStimulusTypeCustomization::ChangeEnumVal))
		]

		//в строчку разместить численные поля всех компонентов
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

//расцветить строки под заголовком при раскрытии, их тут не будет, все в заголовке умещается
void FEmotionTypeCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

//конструктор
FEmotionTypeCustomization::FEmotionTypeCustomization():EquiColorBrush(FLinearColor::White)
{
	Typicals = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEmotio"), true);
}

//универсальные возвращуны хэндлов компонентов
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


//получить данные компонентов в виде цвета
FLinearColor FEmotionTypeCustomization::GetColorForMe() const
{	FLinearColor Re(0,0,0,1);
	GetR()->GetValue(Re.R);
	GetG()->GetValue(Re.G);
	GetB()->GetValue(Re.B);
	UE_LOG(LogTemp, Log, TEXT("FEmotionTypeCustomization::GetColorForMe %g, %g %g"), Re.R, Re.G, Re.B);
	return Re;
}

//внедрить значение из внешней эмоции по хэндлам
void FEmotionTypeCustomization::SetValue(FEmotio E)
{	GetR()->SetValue(E.R);
	GetG()->SetValue(E.G);
	GetB()->SetValue(E.B);
	GetA()->SetValue(E.A);
	UE_LOG(LogTemp, Log, TEXT("FEmotionTypeCustomization::SetValue %g, %g %g"), E.R, E.G, E.B);

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
	SetValue(FEmotio::As((EEmotio)Nv));
}

//когда редактируется выбором из списка архетипов
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

 //универсальные возвращуны хэндлов компонентов
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

//внедрить значение из внешней эмоции по хэндлам
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
