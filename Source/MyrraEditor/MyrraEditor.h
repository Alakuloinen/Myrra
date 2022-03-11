// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "UnrealEd.h"
//#include "asset_factories.h"

//==============================================================================================================
//говорят, чтобы отделить лог модуля в случае финальной сборки без редактора
//==============================================================================================================
DECLARE_LOG_CATEGORY_EXTERN(MyrraEditor, All, All)


class FMyrraEditorModule : public IModuleInterface
{

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};
