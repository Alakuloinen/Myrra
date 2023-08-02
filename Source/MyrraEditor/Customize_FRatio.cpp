#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h" //дл€ SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"


TSharedRef<IPropertyTypeCustomization> FRatioTypeCustomization::MakeInstance()
{
	return MakeShareable(new FRatioTypeCustomization);
}

//расцветить строку заголовка
void FRatioTypeCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//головнойдескрипторсохранить дл€ других функций
	MainHandle = StructPropertyHandle;

	//дл€ бипол€рного типа
	float Mi = 0.0f, Ma = 1.0f;
	TypeFactor = MainHandle->GetProperty()->GetCPPType()[1];
	if (TypeFactor == 'B')	Mi = -1.0f; 

	//сохранить указатель на это нечто, чтобы с помощью него перериосвывать при изменении значений
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//достучатьс€ до внутренностей
	HandleInt = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRatio, M));
	if(TypeFactor == 'B') HandleInt = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBipolar, M));

	//столбец заголовка (левый)
	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	//столбец значений (правый) - здесь ничего не будет, чтобы визуально не отвлекал от содержимого строк внутри
	.ValueContent().MaxDesiredWidth(200.0f).HAlign(HAlign_Fill).MinDesiredWidth(50)
		[
			SNew(SSpinBox<float>)
			.MinSliderValue(Mi)
			.MaxSliderValue(Ma)
			.MinDesiredWidth(20)
			.MaxFractionalDigits(3)
			.MinFractionalDigits(3)
			.OnValueChanged(FOnFloatValueChanged::CreateSP(SharedThis(this), &FRatioTypeCustomization::OnValueChanged))
			.OnValueCommitted(FOnFloatValueCommitted::CreateSP(SharedThis(this), &FRatioTypeCustomization::OnValueCommitted))
			.Value(this, &FRatioTypeCustomization::GetValue)
		];
}

//расцветить строки под заголовком при раскрытии, их тут не будет, все в заголовке умещаетс€
//даже если столбцов нет, функцию все равно приходитс€ иметь пустою
void FRatioTypeCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

float FRatioTypeCustomization::GetValue() const
{	uint8 Val; HandleInt->GetValue(Val);
	if(TypeFactor == 'B')
		return (float)(Val / 255.0f)*2.0f - 1.0f;
	else
		return (float)Val / 255.0f;
}

void FRatioTypeCustomization::OnValueCommitted(float InVal, ETextCommit::Type Tc)
{
	OnValueChanged(InVal);
}

void FRatioTypeCustomization::OnValueChanged(float InVal)
{	uint8 Val = InVal * 255;
	if(TypeFactor == 'B') Val = ((InVal+1.0f)*0.5f) * 255;
	HandleInt->SetValue(Val);
}
