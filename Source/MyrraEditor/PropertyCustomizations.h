#pragma once
#include "CoreMinimal.h"
#include "Myrra.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Brushes/SlateColorBrush.h"
#include "DetailWidgetRow.h"


//#######################################################################################################
//вроде как это только для структур
//#######################################################################################################
class FEmotionTypeCustomization : public IPropertyTypeCustomization
{
public:
    //создать экземпляр
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    //для подкраски результирующего цветового эквивалента
    FSlateColorBrush EquiColorBrush;

    //видимо, здесь происходит украшение заголовка и его составляющих
    virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

    FEmotionTypeCustomization();

	//когда редактируется цветом, и из цвета выбирается список архетипов
    int GetEnumVal() const;

    //когда редактируется выбором из списка архетипов
    virtual void ChangeEnumVal(int Nv, ESelectInfo::Type Hz);

protected:

    virtual TSharedRef<SWidget> WidgetForEmoList();
    virtual FLinearColor GetColorForMe() const;
    virtual void SetValue(FPathia E);

    TSharedPtr<IPropertyHandle> MainHandle;
    TSharedPtr<IPropertyUtilities> PropertyUtilities;
    TSharedPtr<SBorder> Me;
    UEnum* Typicals;

    //обработчик события
    void OnChanged();

};


//#######################################################################################################
// для структуры FWholeBodyDynamicsModel из файла myrra_anatomy.h
//#######################################################################################################
class FGirdleFlagsCustomization : public IPropertyTypeCustomization
{
public:
    //создать экземпляр
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    //видимо, здесь происходит украшение заголовка и его составляющих
    virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
};

class FMyrDynModelTypeCustomization : public IPropertyTypeCustomization
{
public:
	//создать экземпляр
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    //видимо, здесь происходит украшение заголовка и его составляющих
    virtual void CustomizeHeader   (TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow,				IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren (TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder,	IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

    //обработчик события
    void OnUsedChanged();

private:
    TSharedPtr<IPropertyHandle> HandleUse;
    TSharedPtr<SWidget> ThoN;
    TSharedPtr<SWidget> ThoV;
    TSharedPtr<SWidget> PelN;
    TSharedPtr<SWidget> PelV;
    TSharedPtr<IPropertyHandle> DynModelPropertyHandle;
    TSharedPtr<IPropertyUtilities> PropertyUtilities;
};


//#######################################################################################################
// для структуры FGirdleDynModels из файла myrra_anatomy.h
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
    void OnUsedChanged();

	//превратить набор битов в строку подсказку, что
	FLinearColor MakeDigestString (int32 Bits, FString& OutStr, char TypeCode);

private:
    
   
    UEnum* EnumDyMoPtr;		// указатель на перечислитель бит дин-модели члеников, чтобы брать оттуда строки названий
    TSharedPtr<IPropertyHandle> MainHandle;
    TSharedPtr<IPropertyHandle> HandleUsed;
    TSharedPtr<IPropertyHandle> HandleLimbs[3];
    TSharedPtr<IPropertyHandle> HandleFlags[3];
    FDetailWidgetRow RowWidgets[5];
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

//#######################################################################################################
//для структуры FRatio из файла AdvMath.h
//#######################################################################################################
class FRatioTypeCustomization : public IPropertyTypeCustomization
{
public:
    //создать экземпляр
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    //видимо, здесь происходит украшение заголовка и его составляющих
    virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

    //обработчик события
    float GetValue() const;
    void OnValueCommitted(float Val, ETextCommit::Type Tc);
    void OnValueChanged(float Val);

private:
    float Buffer;
    char TypeFactor;
    TSharedPtr<IPropertyHandle> MainHandle;
    TSharedPtr<IPropertyHandle> HandleInt;
    TSharedPtr<IPropertyUtilities> PropertyUtilities;

};

//#######################################################################################################
//для структуры FInflu из файла psyche.h
//#######################################################################################################
class FInfluCustomization : public IPropertyTypeCustomization
{
public:
    //создать экземпляр
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    //видимо, здесь происходит украшение заголовка и его составляющих
    virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
    TSharedPtr<IPropertyHandle> MainHandle;
    TSharedPtr<IPropertyUtilities> PropertyUtilities;

};