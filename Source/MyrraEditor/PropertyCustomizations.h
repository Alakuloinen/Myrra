#pragma once
#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Brushes/SlateColorBrush.h"

//#######################################################################################################
//вроде как это только для структур
//#######################################################################################################
class FMyrDynModelTypeCustomization : public IPropertyTypeCustomization
{
public:
	//создать экземпляр
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    //видимо, здесь происходит украшение заголовка и его составляющих
    virtual void CustomizeHeader   (TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow,				IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren (TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder,	IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
private:
    TSharedPtr<IPropertyHandle> DynModelPropertyHandle;


};

//#######################################################################################################
//вроде как это только для структур
//#######################################################################################################
class FMyrGirdleModelTypeCustomization : public IPropertyTypeCustomization
{
public:
    //создать экземпляр
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    //видимо, здесь происходит украшение заголовка и его составляющих
    virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	//обработчик события
	void OnChanged();

	//превратить набор битов в строку подсказку, что
	FLinearColor MakeDigestString (int32 Bits, FString& OutStr);

private:
	const UEnum* EnumPtr;
    TSharedPtr<IPropertyHandle> MainHandle;
    TSharedPtr<IPropertyHandle> HandleLimbs[5];
	FString Digest[5];
    FLinearColor DigestColor[5];
    TSharedPtr<IPropertyUtilities> PropertyUtilities;
    TSharedPtr<STextBlock> Mnemo[5];
};

//#######################################################################################################
//для структуры FMyrLogicEventData из файла myrra_gameplay.h
//#######################################################################################################
class FMyrLogicEventDataTypeCustomization : public IPropertyTypeCustomization
{
    FSlateColorBrush bme;
    FSlateColorBrush bgo;
public:
	//создать экземпляр
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    //видимо, здесь происходит украшение заголовка и его составляющих
    virtual void CustomizeHeader   (TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow,				IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren (TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder,	IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

    FMyrLogicEventDataTypeCustomization():bme(FLinearColor::Red), bgo(FLinearColor::White) {}
private:
    TSharedPtr<IPropertyHandle> HandleColorForMe;
    TSharedPtr<IPropertyHandle> HandleColorForGoal;
    TSharedPtr<IPropertyUtilities> PropertyUtilities;
    TSharedPtr<SBorder> Me;
    TSharedPtr<SBorder> Goal;

    //обработчик события
    void OnChanged();

    FLinearColor GetColorForMe()
	{ 	FColor R;
		HandleColorForMe->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, R))->GetValue(R.R);
		HandleColorForMe->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, G))->GetValue(R.G);
		HandleColorForMe->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, B))->GetValue(R.B);
		//HandleColorForMe->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, A))->GetValue(R.A);
        R.A = 255;
        return FLinearColor(R);
    }
    FLinearColor GetColorForGoal()
	{ 	FColor R;
		HandleColorForGoal->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, R))->GetValue(R.R);
		HandleColorForGoal->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, G))->GetValue(R.G);
		HandleColorForGoal->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, B))->GetValue(R.B);
		//HandleColorForGoal->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, A))->GetValue(R.A);
        R.A = 255;
        return FLinearColor(R);
	}


};