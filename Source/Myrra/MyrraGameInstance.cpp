// Fill out your copyright notice in the Description page of Project Settings.

#include "MyrraGameInstance.h"
#include "MyrraGameModeBase.h"
#include "MyrraSaveGame.h"
#include "HAL/PlatformFilemanager.h"
#include "Engine/Classes/Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "Control/MyrPlayerController.h"
#include "Creature/MyrPhyCreature.h"

#include "MyrQuest.h"									//врубать и продвигать квесты

#include "Artefact/SwitchableStaticMeshComponent.h"		//чтобы делать стартовые вариации
#include "Artefact/MyrraDoor.h"							//чтобы делать стартовые вариации
#include "Artefact/MyrLocation.h"						//для сохранения помещений
#include "Artefact/MyrTriggerComponent.h"				//для звызова реакций на триггер-объёме
#include "Components/SplineComponent.h"					//для замощения кабелями		
#include "Components/SplineMeshComponent.h"				//для замощения кабелями

#include "UI/MyrBioStatUserWidget.h"					// индикатор над головой

#include "Control/MyrPlayerController.h"				// чтобы сменять вижеты на глазах у игрока

#include "Sky/MyrKingdomOfHeaven.h"						// чтобы сохранять время дня и чило дней

#include "Runtime/MoviePlayer/Public/MoviePlayer.h"		// для экранов загрузки, пока неясно, как

#include "Materials/MaterialParameterCollectionInstance.h"	// для подводки материала неба

#include "ImageUtils.h"									// загрузка скриншотов как текстур

#include "MyrraGameUserSettings.h"						//  опции для меню настроек

#include "AudioDeviceManager.h"							// для манипуляции громкостями звуков из кода
#include "AudioDevice.h"								// для манипуляции громкостями звуков из кода

#define ADDREA(Cause, Emo, Name, Desc) EmoReactionWhatToDisplay.Add(Cause, FEmoReactionsUI(TEXT(Name), TEXT(Desc), Emo))
#define ADDREAC(What,Where,How,Why, Emo, Name, Desc) EmoReactionWhatToDisplay.Add(FInflu(EInfluWhat::##What, EInfluWhere::##Where, EInfluHow::##How, EInfluWhy::##Why), FEmoReactionsUI(TEXT(Name), TEXT(Desc), Emo))
//====================================================================================================
//====================================================================================================
UMyrraGameInstance::UMyrraGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/*for (int i = 0; i < (int)EWeAt::Latent; i++)
	{
		SimpleEmoStimuliNamesMe[i] = UEnum::GetDisplayValueAsText((EWeAt)i);
		SimpleEmoStimuliNamesYe[i] = UEnum::GetDisplayValueAsText((EWeAt)i);
	}*/

	//начальные шаблоны некоторых эмоциональных рефлексов и их названия для интерфейса
	//тут смешение с тем, что в ИИ отдельно написано, надо слить
	//ADDREA(FInflu(EInfluWhere::Moon), FPathia(Hope, +2)		"Me and the Moon", "My feeling At Shine + At Night");
	//ADDREA(YE4_(NotMe, Big, New, Seen),			"A sudden giant seen",		"Subject is new, seen and relatively big",			FPathia(Anxiety,	+3));
	//ADDREA(YE3_(NotMe, New, Seen),				"See a novice",				"Subject is recently noticed and seen now",			FPathia(Hope,		+5));
	//ADDREA(YE3_(NotMe, Big, Seen),				"See a giant",				"Subject is big and seen",							FPathia(Worry,		+3));
	//ADDREA(YE4_(NotMe, Big, ComingCloser, Seen),"See a giant approaching",	"Subject is big, seen and is coming closer",		FPathia(Fear,		+3));

	ADDREA(Moon,				FPathia(Hope, +2), "At Moonlight", "The Moon is seen on the sky");

	ADDREA(YouReBig,			FPathia( Fear,		+2), "You're Big",			"The size of this entity is 2 or more times greater than the mine");
	ADDREA(YouReNew,			FPathia( Care,		+5), "You're New Here",		"This has never been before, so it's astonishing");
	ADDREA(YouReSeenBig,		FPathia( Anxiety,	+3), "You're seen big",		"This never-been-before entity is 2 or more times bigger than me");
	ADDREA(YouReUnreachable,	FPathia( Pessimism,	+2), "You're unreachable",	"This goal cannot be reached by walking or climbing");
	ADDREA(YouReImportant,		FPathia( Mania,		+2), "You're important",	"This entity makes very special feelings apriori or by the plot");
	ADDREA(YouComeAround,		FPathia( Anxiety,	-2), "You're coming around","This creature is moving in direction perpendicular to our sight");
	ADDREA(YouComeCloser,		FPathia( Anxiety,	+3), "You're coming closer","This being is approaching");
	ADDREA(YouComeAway,			FPathia( Cheer,		+3), "You're coming away",	"This creature is moving out of our location");

	//ADDREA(YouPleasedMe,		FPathia( Care,				+5), "You pleased me",		"This creature has made good feelings to us");

}

//====================================================================================================
//при изменении свойств при редактированни класса потомка в редакторе
//====================================================================================================
#if WITH_EDITOR
void UMyrraGameInstance::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

//====================================================================================================
//совсем общая функция для начальной инициализации, хотя хз чем здесь лучше конструктора
//похоже, заполнять данные, которые планируются сохраняться в ассете - здесь нельзя
//только установка параметров на время игры
//====================================================================================================
void UMyrraGameInstance::Init() 
{
	//что-то и здесь делается, иначе зачем уникальная функция, коей нет в других уобъектах
	Super::Init();

	//чтобы каждый запуск игры рандом был разным
	FMath::SRandInit(FDateTime::Now().GetSecond());

	//ассигнуем оттенки красного для дымов запаха, пусть будут красные, сиреневые, в меньшей степени оранжевые
	//так как жёлтый ближе к зеленому, а зеленый кошка всё же видит вместо красного
	//здесь только цвет, не яркость, яркость отдельно, поэтому диапазон 0-1
	ColorsOfSmellChannels.SetNum(16);
	for (auto& C : ColorsOfSmellChannels)
		C = FLinearColor(1.0f, FMath::RandRange(0.0f, 0.15f), FMath::RandRange(0.0f, 0.2f));

	//подцеп событий начала загрузки уровня и результативного конца 
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UMyrraGameInstance::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UMyrraGameInstance::EndLoadingScreen);

	//подцепить все квесты - точки срабатывания
	InitializeQuests();

}

void UMyrraGameInstance::PostLoad()
{
	Super::PostLoad();
}


//====================================================================================================
//выдать список всех сохраненных игр - не мое, стырено - подправлено к массиву объектов-сохранений
//вызывается при загрузке экрана-меню сохранений, а также ghb pfuheprt gjcktlytuj
//====================================================================================================
TArray<UMyrraSaveGame*> UMyrraGameInstance::GetAllSaveGameSlots()
{
	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	//такой вот изврат - подкласс искателя файлов внутри функции
	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	class FMyrSavesVisitor : public FMyrFileVisitor
	{public:
		FMyrSavesVisitor() { Extension = TEXT("sav"); }
		TArray<UMyrraSaveGame*> SavesFound;
		virtual void MyVisit(FString Name, const FFileStatData& StatData)
		{	UMyrraSaveGame* yaSave = Cast<UMyrraSaveGame>(UGameplayStatics::CreateSaveGameObject(UMyrraSaveGame::StaticClass()));
			if (yaSave)
			{	yaSave->SaveSlotName = FPaths::GetBaseFilename(Name);
				yaSave->SavedDateTime = StatData.ModificationTime;
				SavesFound.Add(yaSave);
			}
		}
	};
	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	//директория, где лежат сохраненные игры - это стандартный путь, по умолчанию для подсистемы сохранения
	//а вообще здесь нужно что-то более вразумительное, например ini-свойство
	const FString SavesFolder = FPaths::ProjectSavedDir() + TEXT("SaveGames");

	//массив найденных имен файлов
	TArray<UMyrraSaveGame*> Saves;

	//если выше определенная папка с сохранениями не пуста
	if (!SavesFolder.IsEmpty())
	{
		//создаем системный искатель файлов (крайне криво, нарочно не догадаешься)
		FMyrSavesVisitor Visitor;
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryStat(*SavesFolder, Visitor);
		Saves = Visitor.SavesFound;
	}

	return Saves;
}

//====================================================================================================
//порыться в папке скриншотов и найти все
//====================================================================================================
TArray<FString> UMyrraGameInstance::GetAllScreenshots()
{
	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	//такой вот изврат - подкласс искателя файлов внутри функции
	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	class FMyrScreenshotsVisitor : public FMyrFileVisitor
	{
	public:
		FMyrScreenshotsVisitor() { Extension = TEXT("png"); }
		TArray<FString> ScreenshotFound;
		virtual void MyVisit(FString Path, const FFileStatData& StatData)
		{	ScreenshotFound.Add(Path);	}
	};

	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	const FString Folder = FPaths::ProjectSavedDir() + TEXT("Screenshots\\Windows");
	TArray<FString> ScreenshotFound;
	if (!Folder.IsEmpty())
	{
		//создаем системный искатель файлов (крайне криво, нарочно не догадаешься)
		FMyrScreenshotsVisitor Visitor;
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryStat(*Folder, Visitor);
		ScreenshotFound = Visitor.ScreenshotFound;
	}
	return ScreenshotFound;
}

//для меню настроек
void UMyrraGameInstance::LoadOptions()
{	
	//инициализировать свою сборку настроек, загрузить ее откуда-то хз откуда
	GEngine->GameUserSettingsClass = UMyrraGameUserSettings::StaticClass();
	Options = (UMyrraGameUserSettings*)GEngine->GetGameUserSettings();
	Options->ApplySettings(false);

	//отдельно применить загруженные настройки звука
	SetVol(EMyrOptions::SoundAmbient);
	SetVol(EMyrOptions::SoundSubjective);
	SetVol(EMyrOptions::SoundNoises);
	SetVol(EMyrOptions::SoundVoice);
	SetVol(EMyrOptions::SoundMusic);
}
void UMyrraGameInstance::SaveOptions()
{
	//применить и сохранить настройки куда-то в файл
	Options->ApplySettings(false);
	Options->SaveSettings();
}
void UMyrraGameInstance::DiscardOptions()
{
	//перезагрузить последние сохраненные настройки
	Options->LoadSettings();
	Options->ResetToCurrentSettings();

	//отдельно применить загруженные настройки звука
	SetVol(EMyrOptions::SoundAmbient);
	SetVol(EMyrOptions::SoundSubjective);
	SetVol(EMyrOptions::SoundNoises);
	SetVol(EMyrOptions::SoundVoice);
	SetVol(EMyrOptions::SoundMusic);
}
int32 UMyrraGameInstance::GetOption(const EMyrOptions O) const	{ return	Options->GetOption(O);		}
void UMyrraGameInstance::SetOption(EMyrOptions O, int V)		{			Options->SetOption(O, V);	}

//====================================================================================================
//изменить громкость класса звуков 
//====================================================================================================
void UMyrraGameInstance::SetVol(EMyrOptions Nu)
{
	//перевести константу из сквозной нумерации опций в номер именно адуио настройки
	int Ind = ((int)Nu) - (int)EMyrOptions::SoundAmbient;
	if (Ind < 0 || Ind > 4) return;

	//выкорчевать микс из каких-то глобальных настроек, надеюсь в релизе они останутся
	auto SoundMix = GetWorld()->GetAudioDevice()->GetDefaultBaseSoundMixModifier();
	UGameplayStatics::SetSoundMixClassOverride(
		GetWorld(), SoundMix,
		SoundMix->SoundClassEffects[Ind].SoundClassObject,
		(float)Options->GetSoundVolume(Ind) / 100.0f, 1.0f, 0.1f, true);
}



//====================================================================================================
// загрузить скриншот в память как текстуру, рандомный из заданного списка путей к файлу
//====================================================================================================
UTexture2D* UMyrraGameInstance::GetRandomScreenshot(const TArray<FString>& Paths)
{
	if(Paths.Num())
		return FImageUtils::ImportFileAsTexture2D(Paths[FMath::RandRange(0, Paths.Num()-1)]);
	else return nullptr;
}


//====================================================================================================
//начать совсем новую игру
//====================================================================================================
bool UMyrraGameInstance::NewGame ()
{
	//начать загрузку базового уровня
	//название должно быть переменным, чтобы можно было иметь разные игры / начала
	UGameplayStatics::OpenLevel(GetWorld(), TEXT("natura"), true, TEXT(""));
	return true;
}

//====================================================================================================
//загрузить последнее сохранение
//====================================================================================================
bool UMyrraGameInstance::LoadLastSave ()
{
	auto Saves = GetAllSaveGameSlots();
	if(Saves.Num() == 0) return false;
	UMyrraSaveGame* Latest = Saves[0];
	for(auto Slot : Saves)
	{
		if(Slot->SavedDateTime < Latest->SavedDateTime)
			Latest = Slot;
	}
	return Load(Latest);
}

//====================================================================================================
//быстрое сохранение - слот создаётся внутри
//====================================================================================================
void UMyrraGameInstance::QuickSave()
{
	//создание объекта сохраненной игры
	UMyrraSaveGame* QuickSlot = Cast<UMyrraSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UMyrraSaveGame::StaticClass()));
	if (QuickSlot)
	{
		//дозаполнение общих данных по сохранению
		QuickSlot->SaveSlotName = TEXT("QuickSave");

		//собственно сохранить и уходить
		Save(QuickSlot);
	}
}


//====================================================================================================
//действия при окончании игры
//====================================================================================================
void UMyrraGameInstance::PostGameOverActions()
{
	//пока здесь только убор с экрана виджетов при смерти протагониста
	TArray<AActor*> FoundCreatures;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyrPhyCreature::StaticClass(), FoundCreatures);
	for (auto Actor : FoundCreatures)
	{
		auto Myr = Cast<AMyrPhyCreature>(Actor);
		if (Myr->StatsWidget()) Myr->StatsWidget()->OnDeath();
		UE_LOG(LogTemp, Warning, TEXT("GameInstance, %s knows player death"), *Myr->GetName());
	}
}

//====================================================================================================
//взять актора, найти все компоненты с альтернативами и перемещать их
//====================================================================================================
void UMyrraGameInstance::ShuffleSwitchableComponents(AActor* ForWhom)
{
/*	for (auto& C : ForWhom->GetComponents())
	{
		if(auto Cc = Cast<USwitchableStaticMeshComponent>(C))
			Cc->SetRandomMesh();
		else if (auto Cd = Cast<UMyrDoorLeaf>(C))
			Cd->VaryOnStart();
	}*/
}

//====================================================================================================
//для всяких зданий, оплетать их кабелями
//====================================================================================================
void UMyrraGameInstance::FillSplineWithMeshes(AActor* Actor, USplineComponent* BaseSpline, UStaticMesh* MeshForSegment)
{
	float Len = BaseSpline->GetSplineLength();
	float SegLen = MeshForSegment->GetBounds().BoxExtent.Z*2;
	int NumSeg = Len / SegLen;
	float CurDist = 0;
	for (int i = 0; i < NumSeg; i++)
	{
		FVector Pos1 = BaseSpline->GetLocationAtDistanceAlongSpline(CurDist, ESplineCoordinateSpace::Local);
		FVector Tg1 = BaseSpline->GetTangentAtDistanceAlongSpline(CurDist, ESplineCoordinateSpace::Local);
		CurDist += SegLen;
		FVector Pos2 = BaseSpline->GetLocationAtDistanceAlongSpline(CurDist, ESplineCoordinateSpace::Local);
		FVector Tg2 = BaseSpline->GetTangentAtDistanceAlongSpline(CurDist, ESplineCoordinateSpace::Local);
		FString ForName = BaseSpline->GetName() + TEXT("Seg") + FString::FromInt(i);
		USplineMeshComponent* YaSeg = NewObject<USplineMeshComponent>(Actor, FName(*ForName));
		YaSeg->RegisterComponent();
		YaSeg->SetStaticMesh(MeshForSegment);
		YaSeg->AttachToComponent(BaseSpline, FAttachmentTransformRules::KeepWorldTransform);

		YaSeg->SetStartAndEnd(Pos1, Tg1, Pos2, Tg2);
		UE_LOG(LogTemp, Error, TEXT("GameInstance, %s add new cable %d at %s"), *Actor->GetName(), i, *Pos1.ToString());
	}
}


//====================================================================================================
// загрузка игры (объект загрузки уже должен быть инициализирован правильным именем и файлом)
//====================================================================================================
bool UMyrraGameInstance::Load(UMyrraSaveGame * Slot)
{
	//проверка на правильность, сюда ещё бы фактические параметры игры проверять
	//плюс саму возможность сохраняться в данный момент
	if(!Slot) return false;

	//собственно, загрузить все голые данные
	Slot = Cast<UMyrraSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot->SaveSlotName, 0));

	//статистика
	Statistics = Slot->Statistics;

	//начать загрузку базового уровня, в котором было это сохранение
	UGameplayStatics::OpenLevel(GetWorld(), Slot->PrimaryLevel, true, TEXT("listen"));

	//сохранить данные для EndLoadingScreen... если это сработает
	JustLoadedSlot = Slot;

	//загрузка времени
	if(GetMyrGameMode()->Sky)
	{	GetMyrGameMode()->Sky->TimeOfDay = Slot->TimePassed;
		GetMyrGameMode()->Sky->ChangeTimeOfDay();
	}

	//по всем сохраненным существам
	for (auto CSD : JustLoadedSlot->AllCreatures)
	{
		//если явно указан класс, это динамически высранное существо
		if (CSD.Value.CreatureClass)
			GetWorld()->SpawnActor<AMyrPhyCreature>(CSD.Value.CreatureClass, CSD.Value.Transform);
	}


	//а вот квесты загрузить сразу же
	StartedQuests.Reset();
	for(auto QSD : JustLoadedSlot->AllQuests)
	{
		//заново создаем объекты прогрессирующих квестов
		UMyrQuestProgress* QWork = NewObject<UMyrQuestProgress>();

		//зугружаем пройденные стадии, просто имена
		QWork->Load(QSD.Value);

		//теперь сложнее, надо подцепить реальные данные квеста, придётся искать по массиву
		for(auto Q : AllQuests)
			if(Q->GetFName() == QSD.Value.Name) { QWork->Quest = Q; break; }

		//сборка готова, запилить пару в карту
		StartedQuests.Add(QSD.Value.Name, QWork);
	}

	//здесь должны проводиться финальные штрихи по внедрению загруженных данных
	return true;
}


//====================================================================================================
// сохранение игры (объект загрузки уже должен быть инициализирован правильным именем и файлом)
//====================================================================================================
bool UMyrraGameInstance::Save(UMyrraSaveGame * Slot)
{
	//проверка
	if (!Slot) return false;

	//вписать дату и время
	Slot->SavedDateTime = FDateTime::Now();

	//сохранить имя уровня, в котором происходило действо
	Slot->PrimaryLevel = FName(*GetWorld()->GetName());

	//статистика
	Slot->Statistics = Statistics;

	//сохранение всех существ
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyrPhyCreature::StaticClass(), Found);
	for (auto Actor : Found)
	{	auto Myr = Cast<AMyrPhyCreature>(Actor);
		FCreatureSaveData CSD;
		Myr->Save(CSD);
		Slot->AllCreatures.Add(Myr->GetFName(), CSD);
	}

	//сохранение всех артефактов
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyrArtefact::StaticClass(), Found);
	for (auto Actor : Found)
	{	auto Ar = Cast<AMyrArtefact>(Actor);
		FArtefactSaveData ASD;
		Ar->Save(ASD);
		Slot->AllArtefacts.Add(Ar->GetFName(), ASD);
	}

	//сохранение всех локаций
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyrLocation::StaticClass(), Found);
	for (auto Actor : Found)
	{	auto Lo = Cast<AMyrLocation>(Actor);
		FLocationSaveData LSD;
		Lo->Save(LSD);
		Slot->AllLocations.Add(Lo->GetFName(), LSD);
	}

	//сохранение состояний всех квестов
	for(auto QuestP : StartedQuests)
	{	FQuestSaveData QSD;
		QuestP.Value->Save(QSD);
		Slot->AllQuests.Add(QuestP.Value->Quest->GetFName(), QSD);
	}

	//сохранение прошедшего времени
	if(GetMyrGameMode()->Sky)
		Slot->TimePassed = GetMyrGameMode()->Sky->TimeOfDay;

	//сохранение счётчиков статистики
	//вообще непонятно, как это делать для мультиплеера
	auto Ctr = GetWorld()->GetFirstPlayerController<AMyrPlayerController>();

	//собственно сохранить и уходить
	return UGameplayStatics::SaveGameToSlot(Slot, Slot->SaveSlotName, 0);
}

//====================================================================================================
// сохранить с именем файла
//====================================================================================================
bool UMyrraGameInstance::SaveByFileName(FString Name)
{
	//создание объекта сохраненной игры, он еще пуст, минимальный каркас уобъекта
	UMyrraSaveGame* Slot = Cast<UMyrraSaveGame>(UGameplayStatics::CreateSaveGameObject(UMyrraSaveGame::StaticClass()));
	Slot->SaveSlotName = Name;

	//внутри происходит наполнение объекта данными, он сильно расширяется, затем все это нахрен не нужно
	//к счастью, поскольку слот создается локально в этой функции, он и умирает здесь же
	return Save(Slot);
}

//====================================================================================================
// удалить сохранку по имени файла
//====================================================================================================
UFUNCTION(BlueprintCallable) bool UMyrraGameInstance::DeleteSave(FString Name)
{
	return UGameplayStatics::DeleteGameInSlot(Name, 0);
	
}


//====================================================================================================
//включение Экрана загрузки
//====================================================================================================
void UMyrraGameInstance::BeginLoadingScreen (const FString& MapName)
{
	if(!IsRunningDedicatedServer())
	{
		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
		LoadingScreen.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();
		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
	}
}

//====================================================================================================
//завершение экрана загрузки
//====================================================================================================
void UMyrraGameInstance::EndLoadingScreen (UWorld* InLoadedWorld)
{
	//если уровень загружен по сохранке
	if (JustLoadedSlot)
	{
		//непонятно, насколько этот момент отображает реальное окончание загрузки всех объектов
		JustLoadedSlot = nullptr;
	}
}

//====================================================================================================
//разобрать все квесты и внести их заманушные стадии в лист ожидания
//====================================================================================================
void UMyrraGameInstance::InitializeQuests()
{	
	for (auto& Q : AllQuests)
	{
		if (Q == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("InitializeQuests - WTF null quest container"));
		}
		else
		{
			for (auto& QS : Q->QuestStates)
				QS.Value.PutToWaitingList(MyrQuestWaitingList, Q, QS.Key);
		}
	}
}

//====================================================================================================
//большой обработчик различных реакций, в квестах и в триггер объёмах
//====================================================================================================
UFUNCTION(BlueprintCallable) bool UMyrraGameInstance::React(FTriggerReason Rtype, UObject* ContextObj, AMyrPhyCreature* C, bool Release)
{
	bool ItemTurnedOn = false;

	//если реакция завязана на триггер-объёме, проделать ее изнутри него
	auto Trigger = Cast<UMyrTriggerComponent>(ContextObj);
	if(Trigger) Trigger->ReactSingle(Rtype, C, nullptr, Release);
	
	//реакции более абстрактные, не завязанные на триггер объёме
	else switch (Rtype.Why)
	{
		//начать загрузку уровня, в котором было это сохранение
		case EWhyTrigger::TravelToLevel:
			UGameplayStatics::OpenLevel(GetWorld(), FName(Rtype.Value), true, TEXT("listen"));
			break;

		//переставить демона на новое существо
		case EWhyTrigger::ChangeProtagonist:
			GetMyrGameMode()->ChangeProtagonist(Rtype, C);
			break;

		//поставить маркер квеста (если место поставки не триггер-объём, то сработает здесь)
		case EWhyTrigger::PlaceMarker:
			if (C->Daemon)
			{
				if(auto A = Cast<AActor>(ContextObj))
					C->Daemon->PlaceMarker(A->GetRootComponent());
				else if(auto U = Cast<UPrimitiveComponent>(ContextObj))
					C->Daemon->PlaceMarker(U);
			}
			break;

		//прервать игру и дать возможность загрузиться с сохранения
		case EWhyTrigger::GameOver:
			if (C->Daemon)
			{
				//вывести меню не сразу, а спустя указанное время, за которое применить эффект
				C->Daemon->PreGameOverLimit = FCString::Atof(*Rtype.Value);
			}
			break;


	}
	return true;
}

//====================================================================================================
//создать для текущей функции локальную инстанцию коллекции параметров материала
//====================================================================================================
UMaterialParameterCollectionInstance* UMyrraGameInstance::MakeMPCInst()
{
	return GetWorld()->GetParameterCollectionInstance(GetMatParams());
}

/*void UMyrraGameInstance::EmoStimulusToMnemo(const FReflex& In, FText& Out) const
{	
	TArray<FText> Infli;
	bool Me = ((In.Condition & (1ull << (int)EYeAt::NotMe)) == 0);
	for(int i=1;i<(int)EYeAt::Latent;i++)
	{	if(In.Condition & (1ull<<i))
		{
			FText CuName = Me ? SimpleEmoStimuliNamesMe[i] : SimpleEmoStimuliNamesYe[i];
			if (CuName.IsEmpty())
				Infli.Add(FText::FromString(TXTENUM(EYeAt, (EYeAt)i)));
			else
				Infli.Add(CuName);
		}
	}
	Out = FText::Join(FText::FromString(TEXT(", ")), Infli);
}*/





//====================================================================================================
//передать инфу о событии на головной уровень - это может стать причиной продвижения истории и квеста
//====================================================================================================
void UMyrraGameInstance::MyrLogicEventCheckForStory(EMyrLogicEvent Event, class AMyrPhyCreature* Instigator, float Amount, UObject* Victim)
{
	//тут надо подергать список на предмет совпадения условий
	TArray<FMyrQuestTrigger*> Found;
	MyrQuestWaitingList.MultiFind(Event,Found);
	if(Found.Num()==0)
		UE_LOG(LogTemp, Log, TEXT("MyrLogicEventCheckForStory - No quests with such trigger %s"), *TXTENUM(EMyrLogicEvent, Event));
	for (auto Transition : Found)
	{
		//если не указано имя, то сравниваем классы зачинщика, если не совпадают, ищем другой
		if (!Transition->WhoCausedEvent.IsNone())
			if (Transition->WhoCausedEvent != Instigator->GetFName())
				if (Transition->WhoCausedEvent != Instigator->GetClass()->GetFName())
				{
					UE_LOG(LogTemp, Log, TEXT("MyrLogicEventCheckForStory quest %s unfound subj %s"),
						*Transition->OwningQuest->GetName(), *Transition->WhoCausedEvent.ToString());
					continue;
				}

		//объект, может быть имя самого, имя родителя, имя класса
		if (!Transition->WhatIsAffected.IsNone())
			if (Transition->WhatIsAffected != Victim->GetFName())
				if (Transition->WhatIsAffected != Victim->GetOuter()->GetFName())
					if (Transition->WhoCausedEvent != Victim->GetClass()->GetFName())
					{
						UE_LOG(LogTemp, Log, TEXT("MyrLogicEventCheckForStory quest %s unfound obj %s"),
							*Transition->OwningQuest->GetName(), *Transition->WhatIsAffected.ToString());
						continue;
					}

		//если квест, от которого поступила эта зацепка, находится в правильном состоянии
		if (!Transition->OwningQuest) continue;

		//найти, может этот квест уже стартовал, туда надо записать прогресс
		UMyrQuestProgress* WorkingQuest = GetWorkingQuest(Transition->OwningQuest);

		//если не нашли, значит, он еще не стартовал, и нужно его сконструировать
		if(!WorkingQuest)
		{	WorkingQuest = NewObject<UMyrQuestProgress>();
			WorkingQuest->Quest = Transition->OwningQuest;
			WorkingQuest->CurrentState = TEXT("Waiting");
			StartedQuests.Add(Transition->OwningQuest->GetFName(), WorkingQuest);
		}

		//проверка, что переход вызывается для правильного состояния
		if (Transition->QuestCurStateName != WorkingQuest->CurrentState)
		{	UE_LOG(LogTemp, Log, TEXT("MyrLogicEventCheckForStory quest %s wrong state %s, needed %s"),
				*Transition->OwningQuest->GetName(), *WorkingQuest->CurrentState.ToString(), *Transition->QuestCurStateName.ToString());
			continue;
		}

		//дальше надо выполнить переход к новой стадии квеста - там внутри всяческие внешние реакции
		WorkingQuest->DoTransition(Transition->QuestNextStateName, Instigator);
	}

}
