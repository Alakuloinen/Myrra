// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "GameFramework/Actor.h"
#include "MyrLocation.generated.h"

//###################################################################################################################
//здание, строение и прочая большая сборка разнородных компонентов (включая многоликие меши и триггеры)
//в которой сузество может находиться и изменять, то есть его можно сохранять
//###################################################################################################################
UCLASS() class MYRRA_API AMyrLocation : public AActor
{
	GENERATED_BODY()

	//корень - какой-нибудь меш из строительного набоа, котоый лучше расположить в углу начала координат
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* RootMesh = nullptr;

	//корень актора - объём, покрывающий всю локацию, выдающий сообщение, что существо в нее вошло/вышло, также может быть изменяющий камеру
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMyrTriggerComponent* PrimaryLocationVolume = nullptr;

public:

	//какой компонент сигналит своим ввсранным существам - чтобы они шли в его сторону
	class UMyrTriggerComponent* CurrentAISignalSource = nullptr;

	//настройки гашения шумов специально для этой локации
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class USoundAttenuation* SoundAttenuation;
	
public:

	//охватить это строение при загрузке и сохранении игры
	UFUNCTION(BlueprintCallable) void Save(FLocationSaveData& Dst);
	UFUNCTION(BlueprintCallable) void Load(const FLocationSaveData& Src);

public:	
	// Sets default values for this actor's properties
	AMyrLocation();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:

	//выдать вовне, для ИИ, адрес конкретного источника сигнала
	UMyrTriggerComponent* GetCurrentBeacon() {	return CurrentAISignalSource;	}
};
