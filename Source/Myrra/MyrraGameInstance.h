// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Myrra.h"
#include "Engine/GameInstance.h" 
#include "Containers/Map.h"
#include "MyrQuest.h"
#include "Paper2D/Classes/PaperSprite.h"
#include "MyrraGameInstance.generated.h"

//структура для вынесения эмоциональных реакций в интерфейс, иконка, описание
USTRUCT(BlueprintType) struct MYRRA_API FEmoReactionsUI
{
	GENERATED_USTRUCT_BODY()

	//иконка из нарезки, показывающая суть эмоционального испытания
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FSlateBrush Icon;

	//человековаримый текст, как называется это событие
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Name;

	//дополнительный текст, разъясняющий, когда и как событие происходит
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Description;

	//само значение стимула, сюда будет подсасываться реальное значение при открытии экрана
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FPathia DefaultReaction;

	FEmoReactionsUI() {}
	FEmoReactionsUI(FText inName, FText inDesc, FPathia Arch = Peace) : Name(inName), Description(inDesc), DefaultReaction(Arch) {}
	FEmoReactionsUI(FString inName, FString inDesc, FPathia Arch = Peace) : Name(FText::FromString(inName)), Description(FText::FromString(inDesc)), DefaultReaction(Arch) {}

};

//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//подкласс искателя файлов
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
class FMyrFileVisitor : public IPlatformFile::FDirectoryStatVisitor
{
public:
	//расширение для поиска конкретных типов файла
	FString Extension;

	//своя функция по сгребыванию файлов
	virtual void MyVisit(FString Name, const FFileStatData& StatData) {}

	//переопределение функции ответа на нахождение файла или папки
	virtual bool Visit(const TCHAR* FilenameOrDirectory, const FFileStatData& StatData)
	{
		//если это папка
		if (!StatData.bIsDirectory)
		{	FString FullFilePath(FilenameOrDirectory);
			if (FPaths::GetExtension(FullFilePath) == Extension)
				MyVisit(FullFilePath, StatData);
		}
		return true;
	}

};


class URoleParameter;
//###################################################################################################################
// самый главный класс в игре, больше ничего и не нужно. Стоит всю игру, виден из много откуда, содержит общие
// для всей игры данные
// - набор квество
// - функции загрузки и сохранения
// - ролевые параметры
// - цвета дебаговых линий для разных каналов
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

	//настройки для меню
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class UMyrraGameUserSettings* Options;

	//настройки отображения эмоциональных реакций в меню самой игры
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ForceInlineRow), Category = "Emotion")  TMap<FReflex, FEmoReactionsUI> EmoReactionWhatToDisplay;
	UPROPERTY(EditAnywhere, Category = "Emotion")  FText SimpleEmoStimuliNamesMe[EYeAt::MAX];
	UPROPERTY(EditAnywhere, Category = "Emotion")  FText SimpleEmoStimuliNamesYe[EYeAt::MAX];

	//общие цвета интерфейса
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  FLinearColor ButtonBackgroundSelected;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  FLinearColor ButtonBackgroundUnSelected;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  FLinearColor ButtonBackgroundSelectedHovered;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  FLinearColor ButtonBackgroundUnSelectedHovered;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  FLinearColor ButtonTextUnSelected;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  FLinearColor ButtonTextSelected;

	//статистика текущей ветки игры
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  FGameStats Statistics;


//--------------------------------------------------------------------------------
public: // квесты
//--------------------------------------------------------------------------------

	//каждый акт нахождения в этой карте нужного события что-то запускает, и, возможно, меняет
	//само содержимое этой карты. слишком сложно, проще указатели на сами квесты сохранять
	TMultiMap<EMyrLogicEvent, FMyrQuestTrigger*> MyrQuestWaitingList;

	//это нужно в редакторе заполнить вообще все
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	TArray<UMyrQuest*> AllQuests;

	//квесты, которые уже начались или закончились, быстрый доступ по имени
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TMap<FName, UMyrQuestProgress*> StartedQuests;

//цвета для рисования линий отладки
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)	TMap<ELimbDebug, FColor> DebugColors;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FRatio Ratio;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FGestalt Gestalt;

//--------------------------------------------------------------------------------
public: // стандартные методы и переопределения виртуальных
//--------------------------------------------------------------------------------

	//это типа BeginPlay
	virtual void Init() override;

	//после загрузки
	virtual void PostLoad() override;

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

	//действия при окончании игры
	UFUNCTION(BlueprintCallable) void PostGameOverActions();

	//взять актора, найти все компоненты с альтернативами и перемещать их
	UFUNCTION(BlueprintCallable) void ShuffleSwitchableComponents(AActor* ForWhom);

	//для всяких зданий, оплетать их кабелями - пока не юзается ибо бажно
	UFUNCTION(BlueprintCallable) void FillSplineWithMeshes(class AActor* Actor, class USplineComponent* Base, class UStaticMesh* MeshForSegment);

	//загрузка и сохранение игры - вызываются из виждета-меню для конкретных файлов
	UFUNCTION(BlueprintCallable) bool Load (UMyrraSaveGame* Slot);
	UFUNCTION(BlueprintCallable) bool Save (UMyrraSaveGame* Slot);
	UFUNCTION(BlueprintCallable) bool SaveByFileName (FString Name);
	UFUNCTION(BlueprintCallable) bool DeleteSave (FString Name);

	//включение и завершение экрана загрузки
	UFUNCTION(BlueprintCallable) virtual void BeginLoadingScreen (const FString& MapName);
	UFUNCTION(BlueprintCallable) virtual void EndLoadingScreen (UWorld* InLoadedWorld);

	//разобрать все квесты и внести их заманушные стадии в лист ожидания
	UFUNCTION(BlueprintCallable) void InitializeQuests();

	//большой обработчик различных реакций, в квестах и в триггер объёмах
	UFUNCTION(BlueprintCallable) bool React(FTriggerReason Rtype, UObject* ContextObj, class AMyrPhyCreature* Owner, bool ReleaseStep = true);

	//передать инфу о событии на головной уровень - это может стать причиной продвижения истории и квеста
	//вызывается из существа
	void MyrLogicEventCheckForStory(EMyrLogicEvent Event, class AMyrPhyCreature* Instigator, float Amount, UObject* Victim);

	//для меню настроек
	UFUNCTION(BlueprintCallable) void  LoadOptions();
	UFUNCTION(BlueprintCallable) void  SaveOptions();
	UFUNCTION(BlueprintCallable) void  DiscardOptions();
	UFUNCTION(BlueprintCallable) int32 GetOption(const EMyrOptions O) const;
	UFUNCTION(BlueprintCallable) void  SetOption(EMyrOptions O, int V);
	UFUNCTION(BlueprintCallable) void  SetVol(EMyrOptions Nu);

//--------------------------------------------------------------------------------
public: //возвращуны
//--------------------------------------------------------------------------------

	//данные по текущему загруженному уровню
	class AMyrraGameModeBase* GetMyrGameMode() { return (AMyrraGameModeBase*)GetWorld()->GetAuthGameMode(); }

	//выдать оперативную часть квеста
	UFUNCTION(BlueprintCallable) UMyrQuestProgress* GetWorkingQuest(UMyrQuest* Source) const { auto R = StartedQuests.Find(Source->GetFName()); if (R) return *R; else return nullptr; }

	//выдать список имен всех сохраненных игр
	UFUNCTION(BlueprintCallable) TArray<UMyrraSaveGame*> GetAllSaveGameSlots();
	
	//найти все скриншоты и сделать из них текстуры
	UFUNCTION(BlueprintCallable) TArray<FString> GetAllScreenshots();
	UFUNCTION(BlueprintCallable) UTexture2D* GetRandomScreenshot(const TArray<FString>& Paths);

	//глобальная коллекция глобальных параметров материала
	class UMaterialParameterCollection* GetMatParams() const { return EnvMatParam; }

	//создать для текущей функции локальную инстанцию коллекции параметров материала
	class UMaterialParameterCollectionInstance* MakeMPCInst();

	//выдать ближайшую эмоцию словесную для интерфейса
	UFUNCTION(BlueprintCallable) float EmotionToMnemo(const FPathia In, EPathia& Out1, EPathia& Out2) const { return In.GetFullArchetype(Out1, Out2); }
	UFUNCTION(BlueprintCallable) float EmotionToMnemoText(const FPathia In, FText& Out1, FText& Out2) const
	{ 
		EPathia O1; EPathia O2;
		float Wei = In.GetFullArchetype(O1, O2);
		Out1 = UEnum::GetDisplayValueAsText(O1);
		Out2 = UEnum::GetDisplayValueAsText(O2);
		return Wei;
	}
	UFUNCTION(BlueprintCallable) FLinearColor EmoReflexToColor(const FReflex& In) const { return (FLinearColor)In.Emotion; }
	UFUNCTION(BlueprintCallable) void EmoStimulusToMnemo  (const FReflex& In, FText &Out) const;
	UFUNCTION(BlueprintCallable) bool IsLatent(FReflex R) const { return R.Emotion.IsLatent(); }

};