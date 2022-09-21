#pragma once
#include "CoreMinimal.h"
#include "Factories\Factory.h"
#include "AssetTypeCategories.h"
#include "AssetTypeActions_Base.h"
#include "MyrraEditor.h"
#include "../Myrra/AssetStructures/MyrCreatureGenePool.h"
#include "../Myrra/AssetStructures/MyrCreatureAttackInfo.h"
#include "../Myrra/AssetStructures/MyrCreatureBehaveStateInfo.h"
#include "../Myrra/AssetStructures/MyrDendroInfo.h"
#include "../Myrra/AssetStructures/MyrArtefactInfo.h"
#include "asset_factories.generated.h"

//###################################################################################################################
// фабрика для создания и обслуживания генофондов
//###################################################################################################################
UCLASS(hidecategories = (Object))
class MYRRAEDITOR_API UMyrCreatureGenePoolFactory : public UFactory
{
	GENERATED_BODY()
public:
	UMyrCreatureGenePoolFactory() { bCreateNew = true; bEditAfterNew = true; SupportedClass = UMyrCreatureGenePool::StaticClass(); }
	virtual uint32 GetMenuCategories() const override { return EAssetTypeCategories::Blueprint; }
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override
	{	return NewObject<UMyrCreatureGenePool>(InParent, InClass, InName, Flags);	}
};

class MYRRAEDITOR_API FGenePoolActions : public FAssetTypeActions_Base
{
public:
	FGenePoolActions(EAssetTypeCategories::Type InAssetCategory) : MyAssetCategory(InAssetCategory) {}
	virtual FText GetName() const override { return FText::FromString(TEXT("Myrra Creature GenePool")); }
	virtual FColor GetTypeColor() const override { return FColor(10, 10, 10); }
	virtual UClass* GetSupportedClass() const override { return UMyrCreatureGenePool::StaticClass(); }
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
	virtual uint32 GetCategories() override { return MyAssetCategory; }
	EAssetTypeCategories::Type MyAssetCategory;
};



//###################################################################################################################
// фабрика для создания и обслуживания сборок инфы по состоянию поведения существа
//###################################################################################################################
UCLASS(hidecategories = (Object))
class MYRRAEDITOR_API UMyrCreatureBehaveStateInfoFactory : public UFactory
{
	GENERATED_BODY()
public:
	UMyrCreatureBehaveStateInfoFactory() { bCreateNew = true; bEditAfterNew = true; SupportedClass = UMyrCreatureBehaveStateInfo::StaticClass(); }
	virtual uint32 GetMenuCategories() const override { return EAssetTypeCategories::Blueprint; }
	virtual FName GetNewAssetThumbnailOverride() const { return TEXT("BehaveStateInfoBrush"); }
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override
	{
		return NewObject<UMyrCreatureBehaveStateInfo>(InParent, InClass, InName, Flags);
	}
};
class MYRRAEDITOR_API FBehaveStateInfoActions : public FAssetTypeActions_Base
{
public:
	FBehaveStateInfoActions(EAssetTypeCategories::Type InAssetCategory) : MyAssetCategory(InAssetCategory) {}
	virtual FText GetName() const override { return FText::FromString(TEXT("Myrra Creature Behave State Info")); }
	virtual FColor GetTypeColor() const override { return FColor(40, 30, 10); }
	virtual UClass* GetSupportedClass() const override { return UMyrCreatureBehaveStateInfo::StaticClass(); }
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
	virtual uint32 GetCategories() override { return MyAssetCategory; }
	EAssetTypeCategories::Type MyAssetCategory;
};

//###################################################################################################################
// фабрика для создания и обслуживания сборок инфы по артефактам - аналог генофонда существ
//###################################################################################################################
UCLASS(hidecategories = (Object))
class MYRRAEDITOR_API UMyrArtefactInfoFactory : public UFactory
{
	GENERATED_BODY()
public:
	UMyrArtefactInfoFactory() { bCreateNew = true; bEditAfterNew = true; SupportedClass = UMyrArtefactInfo::StaticClass(); }
	virtual uint32 GetMenuCategories() const override { return EAssetTypeCategories::Blueprint; }
	virtual FName GetNewAssetThumbnailOverride() const { return TEXT("ArtefactInfoBrush"); }
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override
	{
		return NewObject<UMyrArtefactInfo>(InParent, InClass, InName, Flags);
	}
};
class MYRRAEDITOR_API FArtefactInfoActions : public FAssetTypeActions_Base
{
public:
	FArtefactInfoActions(EAssetTypeCategories::Type InAssetCategory) : MyAssetCategory(InAssetCategory) {}
	virtual FText GetName() const override { return FText::FromString(TEXT("Myrra Artefact Info")); }
	virtual FColor GetTypeColor() const override { return FColor(40, 30, 10); }
	virtual UClass* GetSupportedClass() const override { return UMyrArtefactInfo::StaticClass(); }
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
	virtual uint32 GetCategories() override { return MyAssetCategory; }
	EAssetTypeCategories::Type MyAssetCategory;
};





//###################################################################################################################
// фабрика для создания и обслуживания одновалентных действий
//###################################################################################################################
UCLASS(hidecategories = (Object))
class MYRRAEDITOR_API UMyrCreatureActionInfoFactory : public UFactory
{
	GENERATED_BODY()
public:
	UMyrCreatureActionInfoFactory() { bCreateNew = true; bEditAfterNew = true; SupportedClass = UMyrActionInfo::StaticClass(); }
	virtual uint32 GetMenuCategories() const override { return EAssetTypeCategories::Blueprint; }
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override
	{
		return NewObject<UMyrActionInfo>(InParent, InClass, InName, Flags);
	}
};
class MYRRAEDITOR_API FActionInfoActions : public FAssetTypeActions_Base
{
public:

	FActionInfoActions(EAssetTypeCategories::Type InAssetCategory) : MyAssetCategory(InAssetCategory) {}
	virtual FText GetName() const override { return FText::FromString(TEXT("Myrra Creature Action Info")); }
	virtual FColor GetTypeColor() const override { return FColor(10, 10, 10); }
	virtual UClass* GetSupportedClass() const override { return UMyrActionInfo::StaticClass(); }
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
	virtual uint32 GetCategories() override { return MyAssetCategory; }
	EAssetTypeCategories::Type MyAssetCategory;
};

//###################################################################################################################
// фабрика для создания и обслуживания инфы по виду деревьев
//###################################################################################################################
UCLASS(hidecategories = (Object))
class MYRRAEDITOR_API UMyrDendroInfoFactory : public UFactory
{
	GENERATED_BODY()
public:
	UMyrDendroInfoFactory() { bCreateNew = true; bEditAfterNew = true; SupportedClass = UMyrDendroInfo::StaticClass(); }
	virtual uint32 GetMenuCategories() const override { return EAssetTypeCategories::Blueprint; }
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override
	{
		return NewObject<UMyrDendroInfo>(InParent, InClass, InName, Flags);
	}
};
class MYRRAEDITOR_API FDendroInfoActions : public FAssetTypeActions_Base
{
public:
	FDendroInfoActions(EAssetTypeCategories::Type InAssetCategory) : MyAssetCategory(InAssetCategory) {}
	virtual FText GetName() const override { return FText::FromString(TEXT("Myrra Creature Action Info")); }
	virtual FColor GetTypeColor() const override { return FColor(10, 50, 10); }
	virtual UClass* GetSupportedClass() const override { return UMyrDendroInfo::StaticClass(); }
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
	virtual uint32 GetCategories() override { return MyAssetCategory; }
	EAssetTypeCategories::Type MyAssetCategory;
};
