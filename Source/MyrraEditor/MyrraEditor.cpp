// Fill out your copyright notice in the Description page of Project Settings.

#include "MyrraEditor.h"
#include "PropertyEditorModule.h"	//говорят, это нужно включать в самом верху файла
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "asset_factories.h"
#include "PropertyCustomizations.h"

#define LOCTEXT_NAMESPACE "MyrraEditor"

//нужно для лога, кажется, напрямую связано с DECLARE_LOG_CATEGORY_EXTERN в хедере
DEFINE_LOG_CATEGORY(MyrraEditor)

//==============================================================================================================
// видимо, когда включается модуль
//==============================================================================================================
void FMyrraEditorModule::StartupModule()
{

	//тут ещё обычно что-то регистрируют
	UE_LOG(MyrraEditor, Warning, TEXT("TutorialEditor: Log Started"));

	//это, видимо, стартануть модуль / диспетчер показа свойств в редакторе
	auto& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >("PropertyEditor");

	//зарегистрировать расцветитель для класса MyrActionInfo
	//PropertyModule.RegisterCustomClassLayout("MyrActionInfo", FOnGetDetailCustomizationInstance::CreateStatic(&FMyrDynModelCustomization::MakeInstance));
	
	//зарегистрировать расцветитель для структуры WholeBodyDynamicsModel
	PropertyModule.RegisterCustomPropertyTypeLayout("GirdleFlags",				FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGirdleFlagsCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout("WholeBodyDynamicsModel",	FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMyrDynModelTypeCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout("GirdleDynModels",			FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMyrGirdleModelTypeCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout("MyrLogicEventData",		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMyrLogicEventDataTypeCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout("Pathia",					FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FEmotionTypeCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout("Ratio",					FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FRatioTypeCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout("Bipolar",					FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FRatioTypeCustomization::MakeInstance));

	//это вообще непонятно зачем
	PropertyModule.NotifyCustomizationModuleChanged();


	//вызов модуля связанного с ресурсами проекта, для добавления новых типов и категорий ассетов
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	auto MyrCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Myrra")), FText::FromString("Myrra"));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FGenePoolActions(MyrCategory)));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FActionInfoActions(MyrCategory)));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FBehaveStateInfoActions(MyrCategory)));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FDendroInfoActions(MyrCategory)));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FArtefactInfoActions(MyrCategory)));
}

//==============================================================================================================
// видимо, когда отключается модуль
//==============================================================================================================
void FMyrraEditorModule::ShutdownModule()
{
	UE_LOG(MyrraEditor, Warning, TEXT("TutorialEditor: Log Ended"));

	auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout("WholeBodyDynamicsModel");
	PropertyModule.UnregisterCustomClassLayout("GirdleDynModels");
	PropertyModule.UnregisterCustomClassLayout("MyrLogicEventData");
	PropertyModule.UnregisterCustomClassLayout("Pathia");
	PropertyModule.UnregisterCustomClassLayout("Ratio");
}

//самая важная макрооболочка... кажется, должна быть в самом конце
IMPLEMENT_GAME_MODULE(FMyrraEditorModule, MyrraEditor);
