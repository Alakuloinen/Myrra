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

    virtual TSharedPtr<class IPropertyHandle> GetR() const;
    virtual TSharedPtr<class IPropertyHandle> GetG() const;
    virtual TSharedPtr<class IPropertyHandle> GetB() const;
    virtual TSharedPtr<class IPropertyHandle> GetA() const;
    virtual TSharedRef<SWidget> Get5thWidget();
    virtual TSharedRef<SWidget> WidgetForEmoList();
    virtual FLinearColor GetColorForMe() const;
    virtual void SetValue(FEmotio E);

    TSharedPtr<IPropertyHandle> MainHandle;
    TSharedPtr<IPropertyUtilities> PropertyUtilities;
    TSharedPtr<SBorder> Me;
    UEnum* Typicals;

    //обработчик события
    void OnChanged();

};

class FEmoStimulusTypeCustomization : public FEmotionTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();
protected:
    virtual TSharedPtr<class IPropertyHandle> GetR() const;
    virtual TSharedPtr<class IPropertyHandle> GetG() const;
    virtual TSharedPtr<class IPropertyHandle> GetB() const;
    virtual TSharedPtr<class IPropertyHandle> GetA() const;
    virtual TSharedRef<SWidget> Get5thWidget();
    virtual TSharedRef<SWidget> WidgetForEmoList();
    virtual FLinearColor GetColorForMe() const;
    virtual void SetValue(FEmotio E);

};

//#######################################################################################################
// для структуры FWholeBodyDynamicsModel из файла myrra_anatomy.h
//#######################################################################################################
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
    TSharedPtr<IPropertyHandle> HandleLimbs[5];
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