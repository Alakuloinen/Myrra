#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h" //��� SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"


TSharedRef<IPropertyTypeCustomization> FRatioTypeCustomization::MakeInstance()
{
	return MakeShareable(new FRatioTypeCustomization);
}

//���������� ������ ���������
void FRatioTypeCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//��������������������������� ��� ������ �������
	MainHandle = StructPropertyHandle;

	//��� ����������� ����
	float Mi = 0.0f, Ma = 1.0f;
	TypeFactor = MainHandle->GetProperty()->GetCPPType()[1];
	if (TypeFactor == 'B')	Mi = -1.0f; 

	//��������� ��������� �� ��� �����, ����� � ������� ���� �������������� ��� ��������� ��������
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//����������� �� �������������
	HandleInt = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRatio, M));
	if(TypeFactor == 'B') HandleInt = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBipolar, M));

	//������� ��������� (�����)
	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	//������� �������� (������) - ����� ������ �� �����, ����� ��������� �� �������� �� ����������� ����� ������
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

//���������� ������ ��� ���������� ��� ���������, �� ��� �� �����, ��� � ��������� ���������
//���� ���� �������� ���, ������� ��� ����� ���������� ����� ������
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
