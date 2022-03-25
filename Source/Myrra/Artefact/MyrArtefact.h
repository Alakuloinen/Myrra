// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Myrra.h"
#include "AssetStructures/MyrArtefactInfo.h"
#include "MyrArtefact.generated.h"

//###################################################################################################################
//ныне почти пустой по умолчанию актор куда можно набирать компонентов типа 
//###################################################################################################################
UCLASS() class MYRRA_API AMyrArtefact : public AActor
{
	GENERATED_BODY()

public:

	//источник эффектов частиц - в первую очередь для эффекта запаха (внимание, не создается в конструкторе, сюда цепляется то, что создано в редакторе)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UMyrSmellEmitter* SmellEmitter = nullptr;

public:	

	//имя
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText HumanReadableName;

	//данные по типу артефакта
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UMyrArtefactInfo* Archetype;

	//артефакт можно есть по кусочкам
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  TArray<FDigestivity> WhatItDoesIfEaten;


public:	

	//конструктор - создание компонента физ-привязи
	AMyrArtefact();

	//после загрузки
	virtual void PostLoad() override;

	//на старте или в вмомент появления в сцене
	virtual void BeginPlay() override;

	//хз, не нужно наверно пока
	virtual void Tick (float DeltaTime) override;

	//для запуска перестройки структуры после изменения параметров
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	

	//охватить этот артефакт при загрузке и сохранении игры
	UFUNCTION(BlueprintCallable) void Save(FArtefactSaveData& Dst);
	UFUNCTION(BlueprintCallable) void Load(const FArtefactSaveData& Src);

	//сгенерировать имя для высранного артефакта этого вида - реализуется прямо в блупринтах, так как у каждого всё по разному
	UFUNCTION(BlueprintImplementableEvent) FText GenerateHumanReadableName();

};
