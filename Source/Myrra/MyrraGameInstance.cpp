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


//====================================================================================================
//====================================================================================================
UMyrraGameInstance::UMyrraGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

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
	EmoReactionWhatToDisplay.Add(EEmoCause::Pain,
		FEmoReactionsUI(TEXT("Pain"), TEXT("Emotional reaction to acute pain from damage"), EEmotio::Insanity, 10, 200));
	EmoReactionWhatToDisplay.Add(EEmoCause::MeJumped,
		FEmoReactionsUI(TEXT("Me Jumped"), TEXT("Feeling after a successful jump, scaled by jump accumulated force"), EEmotio::Pride, 1, 10));
	EmoReactionWhatToDisplay.Add(EEmoCause::VoiceOfRatio,
		FEmoReactionsUI(TEXT("Ratio"), TEXT("Tendency to balance emotions to rational sober state"), EEmotio::Peace, 2, 150));
	EmoReactionWhatToDisplay.Add(EEmoCause::Burnout,
		FEmoReactionsUI(TEXT("Burnout"), TEXT("Lightening all emotions to the void due to emotional overload"), EEmotio::Void, 3, 100));

	EmoReactionWhatToDisplay.Add(EEmoCause::TimeDay,
		FEmoReactionsUI(TEXT("Time: Day"), TEXT("Daytime induced vibe, no scale, can be rejected at overload"), FEmoStimulus(90, 120, 60, 1, 100)));
	EmoReactionWhatToDisplay.Add(EEmoCause::TimeEvening,
		FEmoReactionsUI(TEXT("Time: Evening"), TEXT("Evening induced vibe, no scale, can be rejected at overload"), FEmoStimulus(70, 100, 100, 1, 100)));
	EmoReactionWhatToDisplay.Add(EEmoCause::TimeMorning,
		FEmoReactionsUI(TEXT("Time: Morning"), TEXT("Morning induced vibe, no scale, can be rejected at overload"), FEmoStimulus(50, 120, 40, 1, 100)));
	EmoReactionWhatToDisplay.Add(EEmoCause::TimeNight,
		FEmoReactionsUI(TEXT("Time: Night"), TEXT("Nighttime induced vibe, no scale, can be rejected at overload"), FEmoStimulus(70, 120, 120, 1, 100)));
	EmoReactionWhatToDisplay.Add(EEmoCause::Moon,
		FEmoReactionsUI(TEXT("Moon"), TEXT("Emotion towards the visible moon, the brighter and upper the stronger"), FEmoStimulus(100, 90, 130, 2, 100)));

	EmoReactionWhatToDisplay.Add(EEmoCause::WeatherSunny,
		FEmoReactionsUI(TEXT("Sunny weather"), TEXT("Attitude towards sunshine, the stronger the less clouds and the higher the sun"), FEmoStimulus(90, 130, 20, 3, 100)));
	EmoReactionWhatToDisplay.Add(EEmoCause::WeatherCloudy,
		FEmoReactionsUI(TEXT("Cloudy weather"), TEXT("Attitude towards daily clouds and overcast, scaled by blue sky shortage"), EEmotio::Peace, 1, 30));
	EmoReactionWhatToDisplay.Add(EEmoCause::WeatherFoggy,
		FEmoReactionsUI(TEXT("Foggy weather"), TEXT("Attitude towards considerable amount of fog, scaled by fog density"), FEmoStimulus(130, 90, 150, 3, 100)));


	EmoReactionWhatToDisplay.Add(EEmoCause::LowHealth,
		FEmoReactionsUI(TEXT("Low Health"), TEXT("Emotional reaction to severe heath lowering, scaled by zero proximity"), EEmotio::Horror, 50, 100));
	EmoReactionWhatToDisplay.Add(EEmoCause::LowStamina,
		FEmoReactionsUI(TEXT("Low Stamina"), TEXT("Feeling physically exhausted, scaled by zero proximity"), EEmotio::Void, 1, 20));

	EmoReactionWhatToDisplay.Add(EEmoCause::DamagedArm,
		FEmoReactionsUI(TEXT("Damaged Arm"), TEXT("Emotion scaled by arm / hinder paw damage"), FEmoStimulus(150, 0, 150, 4, 30)));
	EmoReactionWhatToDisplay.Add(EEmoCause::DamagedLeg,
		FEmoReactionsUI(TEXT("Damaged Leg"), TEXT("Emotion scaled by leg / yonder paw damage"), FEmoStimulus(120, 0, 180, 3, 50)));
	EmoReactionWhatToDisplay.Add(EEmoCause::DamagedTail,
		FEmoReactionsUI(TEXT("Damaged Tail"), TEXT("Emotion scaled by tail physical damage"), FEmoStimulus(180, 0, 50, 1, 60)));
	EmoReactionWhatToDisplay.Add(EEmoCause::DamagedHead,
		FEmoReactionsUI(TEXT("Damaged Head"), TEXT("Emotion scaled by head physical damage"), EEmotio::Insanity, 4, 70));
	EmoReactionWhatToDisplay.Add(EEmoCause::DamagedCorpus,
		FEmoReactionsUI(TEXT("Damaged Body"), TEXT("Emotion scaled by chest, spine, belly physical damage"), FEmoStimulus(180, 20, 150, 1, 60)));

	EmoReactionWhatToDisplay.Add(EEmoCause::HearYouUnknown,
		FEmoReactionsUI(TEXT("Hearing Unknown Creature"), TEXT("Emotion of hearing a creature that was never met (perveived, attacked) before. Scaled by loudness, defined by a general attitude to that creature's species, if present"), EEmotio::Worry, 1, 100));
	EmoReactionWhatToDisplay.Add(EEmoCause::SeeYouUnknown,
		FEmoReactionsUI(TEXT("Seeing Unknown Creature"), TEXT("Emotion of visually perceiving a creature that was never met (perveived, attacked) before. Scaled by visibility, defined by a general attitude to that creature's species, if present"), EEmotio::Anxiety, 2, 10));
	EmoReactionWhatToDisplay.Add(EEmoCause::HearYouKnown,
		FEmoReactionsUI(TEXT("Hearing Known Creature"), TEXT("Emotion of hearing a creature that was already met before and stays defined by a unique attitude in our memory. Scaled by loudness."), EEmotio::Worry, 1, 10));
	EmoReactionWhatToDisplay.Add(EEmoCause::SeeYouKnown,
		FEmoReactionsUI(TEXT("Seeing Known Creature"), TEXT("Emotion of visually preceiving a creature that was already met before and stays defined by a unique attitude in our memory. Scaled by visibility."), EEmotio::Anxiety, 1, 1));

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
	class FFindSavesVisitor : public IPlatformFile::FDirectoryStatVisitor
	{
	public:

		//конструктор пуст, а зачем он нужен явно хз
		FFindSavesVisitor() {}

		//внутренний накопитель найденных имен файлов
		//непонятно, зачем нужно создавать УОбъекты, когда можно хранить просто список строк
		TArray<UMyrraSaveGame*> SavesFound;

		//переопределение функции ответа на нахождение файла или папки
		virtual bool Visit(const TCHAR* FilenameOrDirectory, const FFileStatData& StatData)
		{
			//если это папка
			if (!StatData.bIsDirectory)
			{
				FString FullFilePath(FilenameOrDirectory);
				if (FPaths::GetExtension(FullFilePath) == TEXT("sav"))
				{
					//создать болванку объекта-сохранения и записать туда имя
					UMyrraSaveGame* yaSave = Cast<UMyrraSaveGame>(UGameplayStatics::CreateSaveGameObject(UMyrraSaveGame::StaticClass()));
					if (yaSave)
					{
						yaSave->SaveSlotName = FPaths::GetBaseFilename(FullFilePath);
						yaSave->SavedDateTime = StatData.ModificationTime;
						SavesFound.Add(yaSave);
					}
				}
			}
			return true;
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
		FFindSavesVisitor Visitor;
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryStat(*SavesFolder, Visitor);
		Saves = Visitor.SavesFound;
	}

	return Saves;
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
//====================================================================================================
void UMyrraGameInstance::ToggleSaves()
{
	//получить контроллер первого (пока единственного) игрока
	//какой-то бред, важные функции распределны между GameInstance и PlayerController
	auto PC = Cast<AMyrPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	PC->ToggleSaves();
}

//====================================================================================================
//====================================================================================================
void UMyrraGameInstance::TogglePause()
{
	//получить контроллер первого (пока единственного) игрока
	//какой-то бред, важные функции распределны между GameInstance и PlayerController
	auto PC = Cast<AMyrPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	PC->TogglePause();
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
		for (auto& QS : Q->QuestStates)
			QS.Value.PutToWaitingList(MyrQuestWaitingList, Q, QS.Key);
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
