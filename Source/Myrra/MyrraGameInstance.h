// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Myrra.h"
#include "Engine/GameInstance.h" 
#include "Containers/Map.h"
//#include "Camera/CameraShake.h"
#include "MyrraGameInstance.generated.h"

class URoleParameter;
//###################################################################################################################
// самый главный класс в игре, больше ничего и не нужно. Стоит всю игру, виден из много откуда, содержит общие
// для всей игры данные
//###################################################################################################################
UCLASS(BlueprintType, Config=Game) class UMyrraGameInstance : public UGameInstance
{
	GENERATED_BODY()

//--------------------------------------------------------------------------------
public: // супер глобальные свойства
//--------------------------------------------------------------------------------
	//новая, глобальная сборка свойств поверхностей, локальные везде подчистить
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap < EMyrSurface, FSurfaceInfluence > Surfaces;

	//последний слот использованный для сохранения - костыль, чтобы дождаться загрузки уровня и только потом помещать существ
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) 
		UMyrraSaveGame* JustLoadedSlot = nullptr;

	//ссылка на коллекцию параметров материала для множества материалов типа позиция солнца и т.п.
	//эту штуку задать в редакторе
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class UMaterialParameterCollection* EnvMatParam;

	//шаблоны атрибутов существа и буфер дононра для операций копирования
	//это нужно только для экрана-виджета-меню распределения очков и навыков
	//может, переместить в сам виджет?
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<URoleParameter*> RoleParameterBases;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  EPhene DonorRoleAxis = EPhene::NUM;

	//цвета, которыми отображаются запахи разных каналов
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  TArray<FLinearColor> ColorsOfSmellChannels;

	//цель рендера используемая для фиксации истории следов
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class UTextureRenderTarget2D* TrailsTargetPersistent;

	//материал (видимо тупой простой) для копирования следов (забить в редакторе конкретный материал)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class UMaterialInterface* MaterialToHistorifyTrails;

	//эффект дрожи камеры - сюда надо в релакторе подцепить реальный блюпринт-ассет
	//UPROPERTY(EditDefaultsOnly, Category = Effects)	TSubclassOf<UCameraShake> PainCameraShake;

	//каждый акт нахождения в этой карте нужного события что-то запускает, и, возможно, меняет
	//само содержимое этой карты
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	TMap<EMyrLogicEvent, FMyrQuestsToStart> MyrQuestWaitingList;

//цвета для рисования линий отладки
#if WITH_EDITOR
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbAxisX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbAxisY;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbAxisZ;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbForces;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbTorques;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugGirdleGuideDir;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugGirdleStepped;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugMainDirs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbNormals;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbConstrPri;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugFeetShoulders;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbConstrForceLin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbConstrForceAng;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugCentralConstrForceLin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugCentralConstrForceAng;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLineTrace;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugFeetBrakeDamping;
	FColor DebugLineChannel(ELimbDebug Ch) { return ((FColor*)(&DebugLimbAxisX))[(int)Ch]; }
#endif

//--------------------------------------------------------------------------------
public: // стандартные методы и переопределения виртуальных
//--------------------------------------------------------------------------------

	//совсем общая функция для начальной инициализации
	virtual void Init() override;


	//при изменении свойств при редактированни класса потомка в редакторе
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//конструктор с местной спецификой
	UMyrraGameInstance(const FObjectInitializer& ObjectInitializer);

//--------------------------------------------------------------------------------
public: //свои методы
//--------------------------------------------------------------------------------

	//начать совсем новую игру (здесь прямо загружается уровень)
	UFUNCTION(BlueprintCallable) bool NewGame ();

	//загрузить последнее сохранение
	UFUNCTION(BlueprintCallable) bool LoadLastSave();
	UFUNCTION(BlueprintCallable) void QuickSave();

	//режимы интерфейса, которые включаются и отключаются по одной команде
	UFUNCTION(BlueprintCallable) void ToggleSaves();
	UFUNCTION(BlueprintCallable) void TogglePause();

	//действия при окончании игры
	UFUNCTION(BlueprintCallable) void PostGameOverActions();

	//взять актора, найти все компоненты с альтернативами и перемещать их
	UFUNCTION(BlueprintCallable) void ShuffleSwitchableComponents(AActor* ForWhom);

	//для всяких зданий, оплетать их кабелями
	UFUNCTION(BlueprintCallable) void FillSplineWithMeshes(class AActor* Actor, class USplineComponent* Base, class UStaticMesh* MeshForSegment);


	//загрузка и сохранение игры - вызываются из виждета-меню для конкретных файлов
	UFUNCTION(BlueprintCallable) bool Load (UMyrraSaveGame* Slot);
	UFUNCTION(BlueprintCallable) bool Save (UMyrraSaveGame* Slot);

	UFUNCTION(BlueprintCallable) bool SaveByFileName (FString Name);
	UFUNCTION(BlueprintCallable) bool DeleteSave (FString Name);

	//включение и завершение экрана загрузки
	UFUNCTION(BlueprintCallable) virtual void BeginLoadingScreen (const FString& MapName);
	UFUNCTION(BlueprintCallable) virtual void EndLoadingScreen (UWorld* InLoadedWorld);

	//передать инфу о событии на головной уровень - это может стать причиной продвижения истории и квеста
	//вызывается из существа
	void MyrLogicEventCheckForStory(EMyrLogicEvent Event, class AMyrPhyCreature* Instigator, float Amount, UObject* Victim);

//--------------------------------------------------------------------------------
public: //возвращуны
//--------------------------------------------------------------------------------

	//выдать список имен всех сохраненных игр
	UFUNCTION(BlueprintCallable) TArray<UMyrraSaveGame*> GetAllSaveGameSlots();

	//глобальная коллекция глобальных параметров материала
	class UMaterialParameterCollection* GetMatParams() const { return EnvMatParam; }

	//создать для текущей функции локальную инстанцию коллекции параметров материала
	class UMaterialParameterCollectionInstance* MakeMPCInst();

};