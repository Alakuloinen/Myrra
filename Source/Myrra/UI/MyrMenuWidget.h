// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "Blueprint/UserWidget.h"
#include "MyrMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class MYRRA_API UMyrMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	//единый указатель на объект источник данных - остальное по идее берется в обработчиках
	//хотя переменные для здоровья, усталости и эмоций (будут в релизе) пусть таки останутся
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class AMyrPhyCreature* MyrOwner;

	//то какие еще строки меню в нем содержатся
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = EUIEntry)) int32 MenuItemsSet = 0;

	//текущее окно выбранное из меню
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	EUIEntry CurrentWidgetId = EUIEntry::NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	UUserWidget* CurrentWidget = nullptr;

	//возможно ли это меню вообще закрыть до худа игры - или оно тупиково в себе, как начальное или конец игры
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	bool Cancelable = true;


public:

	//вызов божественных сущностей
	UFUNCTION(BlueprintCallable) class UMyrraGameInstance* GetMyrraGameInstance() const { return (UMyrraGameInstance*)GetGameInstance(); }
	UFUNCTION(BlueprintCallable) class AMyrraGameModeBase* GetMyrraGameMode() const { return (AMyrraGameModeBase*)GetWorld()->GetAuthGameMode(); }
	UFUNCTION(BlueprintCallable) class AMyrPlayerController* GetMyrPlayerController() const { return (AMyrPlayerController*)GetOwningPlayer<APlayerController>(); }

	// реализовать в блюпринете смену виджета = прикрепление к нужной панели и открепление предыдущего
	UFUNCTION(BlueprintImplementableEvent)	void ChangeCurrentWidget(EUIEntry New);

	//добавить внутри в нужное место новый пункт меню
	UFUNCTION(BlueprintImplementableEvent) void AddMenuEntry(EUIEntry New);

	//добавить внутри в нужное место новый пункт меню
	UFUNCTION(BlueprintImplementableEvent) void ClearMenuItems();


	//переразместить новый набор меню, то есть пунктов, которые отображаются вместе с виджетом и к которым можно перейти кликом или клавишей непосредственно из данного открытого окна
	UFUNCTION(BlueprintCallable) bool InitNewMenuSet(int32 Set);

	//во время виджета воспринимать клавиши чтобы закрыть его или переключаться между пунктами меню
	UFUNCTION(BlueprintCallable) void ReactToKey(FKeyEvent InKeyEvent);


};
