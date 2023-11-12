#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h" //для SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

//тут все как в прочих кустомизациях, только меньше, поэтому нах коментарии
#define GETP(S) MainHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInflu, S))
TSharedRef<IPropertyTypeCustomization> FInfluCustomization::MakeInstance() { return MakeShareable(new FInfluCustomization); }
void FInfluCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	MainHandle = StructPropertyHandle;
	HeaderRow
		.NameContent()[StructPropertyHandle->CreatePropertyNameWidget()]
		.ValueContent()
		.MaxDesiredWidth(200.0f)
		.HAlign(HAlign_Fill)
		.MinDesiredWidth(50)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1)[GETP(What)->CreatePropertyValueWidget()]
				+ SHorizontalBox::Slot().FillWidth(1)[GETP(Where)->CreatePropertyValueWidget()]
				+ SHorizontalBox::Slot().FillWidth(1)[GETP(How)->CreatePropertyValueWidget()]
				+ SHorizontalBox::Slot().FillWidth(1)[GETP(Why)->CreatePropertyValueWidget()]
		];
}

void FInfluCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}
