// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "GameFramework/Actor.h"
#include "MyrLocation.generated.h"

//###################################################################################################################
//здание, строение и прочая большая сборка разнородных компонентов (включая многоликие меши и триггеры)
//- обладает объёмом, можно находиться внутри или вне, за объём отвечает PrimaryLocationVolume
//- учитывает существ и артефакты, которые находятся в этом объёме, подаёт сигналы для квестов
//- оказывает эмоциональное влияние на находящихся внутри существ
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

	//какой компонент в составе этого актора сигналит своим высранным существам - чтобы они шли в его сторону
	//их может быть несколько, но включен только один
	class UMyrTriggerComponent* CurrentAISignalSource = nullptr;

	//навязываемая этой локацией эмоция
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FMyrLogicEventData EmotionalInducing;

	//настройки гашения шумов специально для этой локации
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class USoundAttenuation* SoundAttenuation;

	//обнаруженные в этой локации предметы
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSet<class AMyrPhyCreature*> Inhabitants;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSet<class AMyrArtefact*> Content;

	//если территория полностью исключает дождь
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool FullyCoversRain = true;

	//уменьшение количества вьющихся насекомых
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 FliesAmount = 0;

	
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

	//регистрация входящих существ и предметов
	void AddCreature(AMyrPhyCreature* New);
	void RemoveCreature(AMyrPhyCreature* New);
	void AddArtefact(AMyrArtefact* New);
	void RemoveArtefact(AMyrArtefact* New);

	//выдать вовне, для ИИ, адрес конкретного источника сигнала
	UMyrTriggerComponent* GetCurrentBeacon() {	return CurrentAISignalSource;	}
};
