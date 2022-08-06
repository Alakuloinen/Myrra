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

	//������ ��������� �� ������ �������� ������ - ��������� �� ���� ������� � ������������
	//���� ���������� ��� ��������, ��������� � ������ (����� � ������) ����� ���� ���������
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class AMyrPhyCreature* MyrOwner;

	//�� ����� ��� ������ ���� � ��� ����������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = EUIEntry)) int32 MenuItemsSet = 0;

	//������� ���� ��������� �� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	EUIEntry CurrentWidgetId = EUIEntry::NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	UUserWidget* CurrentWidget = nullptr;

	//�������� �� ��� ���� ������ ������� �� ���� ���� - ��� ��� �������� � ����, ��� ��������� ��� ����� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	bool Cancelable = true;


public:

	//����� ������������ ���������
	UFUNCTION(BlueprintCallable) class UMyrraGameInstance* GetMyrraGameInstance() const { return (UMyrraGameInstance*)GetGameInstance(); }
	UFUNCTION(BlueprintCallable) class AMyrraGameModeBase* GetMyrraGameMode() const { return (AMyrraGameModeBase*)GetWorld()->GetAuthGameMode(); }
	UFUNCTION(BlueprintCallable) class AMyrPlayerController* GetMyrPlayerController() const { return (AMyrPlayerController*)GetOwningPlayer<APlayerController>(); }

	// ����������� � ���������� ����� ������� = ������������ � ������ ������ � ����������� �����������
	UFUNCTION(BlueprintImplementableEvent)	void ChangeCurrentWidget(EUIEntry New);

	//�������� ������ � ������ ����� ����� ����� ����
	UFUNCTION(BlueprintImplementableEvent) void AddMenuEntry(EUIEntry New);

	//�������� ������ � ������ ����� ����� ����� ����
	UFUNCTION(BlueprintImplementableEvent) void ClearMenuItems();


	//�������������� ����� ����� ����, �� ���� �������, ������� ������������ ������ � �������� � � ������� ����� ������� ������ ��� �������� ��������������� �� ������� ��������� ����
	UFUNCTION(BlueprintCallable) bool InitNewMenuSet(int32 Set);

	//�� ����� ������� ������������ ������� ����� ������� ��� ��� ������������� ����� �������� ����
	UFUNCTION(BlueprintCallable) void ReactToKey(FKeyEvent InKeyEvent);


};
