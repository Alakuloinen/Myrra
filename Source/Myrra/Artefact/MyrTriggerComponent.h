// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Union.h"
#include "MyrTriggerComponent.generated.h"


//###################################################################################################################
//полный набор данных для высирания (спавна) объектов
//###################################################################################################################
USTRUCT(BlueprintType) struct FSpawnableData
{
	GENERATED_USTRUCT_BODY()

	//тип объекта, который высирается по объекту этой структуры
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AActor> WhatToSpawn;

	//при высере брать координаты на плоскости, а Z трассировать вниз, пока не достигнется земля - для посадки растений
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool TraceForGround = false;

	//вероятность (для взвешенной суммы) что будет высран именно этот
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Chance = 255;

	//время выдержки, если просят высрать раньше, он не высрется
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MinSecondsBetween = 10;

	//максимальное количество высранных акторов за всё время игры
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 MaxNumberOfEverSpawned = 255;

	//максимальное количество одновременно живых высранных акторов
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 MaxNumberOfPresent = 255;

	//если высираем существ, то спустя столько секунд сжирать их этим же объёмом, когда они вернутся
	//если отрицательное, то не сжирать никогда
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float KillAtReturnAfterSeconds = -1.0f;

	//эмоция, которую внушает этот дом высранным существам
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UCurveLinearColor* InducedEmotionByLifeTime;

	//сюда записывается время последнего высера, чтобы ограничить частоту
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FDateTime LastlySpawned;

	//количество высранных акторов за всё время игры
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 NumberOfEverSpawned = 0;

	//множество высранных и на данный момент живых акторов данного типа
	//пока неясно как отслеживать и удалять трупы
	TSet<AActor*> Spawned;
};

//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//компонент меш произвольной формы, при пересечении которого что-то происходит
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYRRA_API UMyrTriggerComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	//последнее время, когда объект потревожили - для отсчёта задержки
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FDateTime LastOverlapEndTime;

	//количество секунд выдержки, если минус, то ваще одноразовый 
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float SecondsToHold = 0;

	//количество секунд выдержки, если минус, то ваще одноразовый 
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float SecondsToRest = 1;

	//выполнять действие только если существо явно нажало какую-то кнопку
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool PerformOnlyByApprovalFromCreature = false;

	//генерировать судьбоносное для игры сообщение на входе из объёма или на выходе
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool GenerateMyrLogicMsgOnIn = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool GenerateMyrLogicMsgOnOut = false;

	//список того, чем можно срать, если только приемный триггер, то список, естественно, пуст
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FSpawnableData> Spawnables;

	//список реакций, можно навесить несколько
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FTriggerReason> Reactions;

	//используется как маяк для привлечения существ, тик включен даже когда никто не пересекает
	bool Beacon;


public:

	//внутрь в функции выше - проверить, можно ли, или временно залочено
	bool CheckTimeInterval(float& MyParam, FString Msg);

	//принять от существа подтверждение, что нажата кнопка и действие совершено
	void ReceiveActiveApproval(class AMyrPhyCreature* Sender);

	//действия по изменению расстояния камеры
	bool ReactionCameraDist(class AMyrDaemon *D, FTriggerReason& R, bool Release);
	float GetCameraDistIfPresent() { for(auto R : Reactions) if(R.Why == EWhyTrigger::CameraDist) return FCString::Atof(*R.Value); return 1.0f;  }

	//реакция на установку векшней позиции камеры
	bool ReactCameraExtPoser(class AMyrDaemon* D, bool Release);

	//действия по открытию двери кнопкой
	void ReactionOpenDoor(class AMyrPhyCreature* C, FTriggerReason& R, bool Release);

	//действия по съеданию
	bool ReactionEat(class AMyrPhyCreature* C, bool *EndChain, FTriggerReason& R);

	//реакция - высер нового объекта
	bool ReactSpawn(FTriggerReason& R, bool Release);

	//реакция - убийство попавшего в объём существа
	bool ReactDestroy(class AMyrPhyCreature* C, class AMyrArtefact* A, FTriggerReason& R);

	//реакция - немедленное уничтожение всего того, что высрано
	bool ReactDestroySpawned(FTriggerReason& R);

	//залезть в коробку умиротворения
	bool ReactQuiet(class AMyrPhyCreature* C, bool Release);

	//мгновенно переместить себя или предмет в другое место
	bool ReactTeleport(FTriggerReason& R, class AMyrPhyCreature* C, class AMyrArtefact* A, bool Deferred);

	//обработать случай входа и выхода из локации
	bool ReactEnterLocation(class AMyrPhyCreature* C, class AMyrArtefact* A, bool Enter); 

	//сформировать вектор дрейфа по векторному полю, вызывается не отсюда, а из ИИ
	FVector ReactVectorFieldMove(FTriggerReason& R, class AMyrPhyCreature* C);

	//сформировать вектор тяги в пределы зоны
	FVector ReactGravityPitMove(FTriggerReason& R, class AMyrPhyCreature* C);

	//вывести наэкран некую инфу относительно содержимого актора, к которому приадлежит этот триггер объём
	bool ReactNotify(FTriggerReason& R, class AMyrPhyCreature* C);

	//найти в этом триггере нужную реакцию
	FTriggerReason* HasReaction(EWhyTrigger T)									{ for (auto& R : Reactions) if (R.Why == T) return &R; return nullptr; }
	FTriggerReason* HasReaction(EWhyTrigger T, EWhyTrigger T2, EWhyTrigger T3)	{ for (auto& R : Reactions) if (R.Why == T || R.Why == T2 || R.Why == T3) return &R; return nullptr; }
	FTriggerReason* HasReaction(EWhyTrigger Tmin, EWhyTrigger Tmax)				{ for (auto& R : Reactions) if ((int)R.Why >= (int)Tmin && (int)R.Why <= (int)Tmax) return &R; return nullptr; }

	///////////////////////////////////////

	//прореагировать одну строку реакции
	bool ReactSingle(FTriggerReason& R, class AMyrPhyCreature* C, class AMyrArtefact* A, bool Release, bool* EndChain = nullptr);

	//прореагировать все имеющиеся в этом компоненте реакции (может быть вызвано извне как реакция сюжета)
	void React(class AMyrPhyCreature* C, class AMyrArtefact* A, class AMyrDaemon* D, bool Release, EWhoCame Who);

	//зонтик для входов и выходов из пересечения
	void OverlapEvent(AActor* OtherActor, UPrimitiveComponent* OverlappedComp, bool ComingOut);

	//вышестоящие инстанции
	UFUNCTION(BlueprintCallable) class UMyrraGameInstance* GetMyrGameInst() { return (UMyrraGameInstance*)GetOwner()->GetGameInstance(); }
	UFUNCTION(BlueprintCallable) class AMyrraGameModeBase* GetMyrGameMode();

public:

	//свершить генерацию объекта согласно внутренним правилам
	UFUNCTION(BlueprintCallable) bool DoSpawn(int ind = -1);

	//высрать вполне определенный объект
	bool SpawnIt(FSpawnableData& What);


public:

	//правильный конструктор
	UMyrTriggerComponent(const FObjectInitializer& ObjectInitializer);

	//изредка тикать, если надо подзывать к себе высранных существ
	virtual void TickComponent(float DeltaTime,	enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//перед самим началом игры
	virtual void BeginPlay() override;

	//начало пересечения с внешним объектом 
	UFUNCTION() void OverlapBegin(
		class UPrimitiveComponent* OverlappedComp,
		class AActor* OtherActor,
		class UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	//конец пересечения с внешним объектом
	UFUNCTION()	void OverlapEnd(
		class UPrimitiveComponent* OverlappedComp,
		class AActor* OtherActor,
		class UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	//вызывается, когда один из высранных акторов умирает
	UFUNCTION() void OnSpawnedEndPlay(AActor* Actor, EEndPlayReason::Type Why);

	
};
