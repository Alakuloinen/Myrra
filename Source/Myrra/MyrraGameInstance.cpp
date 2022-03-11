// Fill out your copyright notice in the Description page of Project Settings.

#include "MyrraGameInstance.h"
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
#include "Components/SplineComponent.h"					//для замощения кабелями		
#include "Components/SplineMeshComponent.h"				//для замощения кабелями

#include "UI/MyrBioStatUserWidget.h"					// индикатор над головой

#include "Control/MyrPlayerController.h"				// чтобы сменять вижеты на глазах у игрока

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
	TArray<AActor*> FoundCreatures;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyrPhyCreature::StaticClass(), FoundCreatures);
	for (auto Actor : FoundCreatures)
	{
		auto Myr = Cast<AMyrPhyCreature>(Actor);
		FCreatureSaveData CSD;
		Myr->Save(CSD);
		Slot->AllCreatures.Add(Myr->GetFName(), CSD);
	}

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
	auto PreResult = MyrQuestWaitingList.Find(Event);
	if(PreResult)
	{
		//перебор всех имеющихся возможностей
		for(auto& Q : PreResult->AvailableQuestTriggers)
		{
			//если не указано имя, то сравниваем классы зачинщика, если не совпадают, ищем другой
			if(Q.Instigator.IsNone() && Q.InstigatorType != Instigator->GetClass()) continue;

			//если имя указано, ищем полное совпадение, если нет, переходим к дркгому
			else if(Q.Instigator != Instigator->GetFName()) continue;

			//если не указано имя объекта но сам объект подан, то сравниваем классы, если не совпадают, ищем другой
			if(Q.Destination.IsNone())
			{	if(Victim) if(Victim->GetClass() != Q.DestinationType) continue;}
			
			//если имя указано, ищем полное совпадение, если нет, переходим к дркгому
			else if(Q.Destination != Victim->GetFName()) continue;

			//теперь исполнение
			auto Result = Q.OwningQuest->Process(Q.QuestTransID);

			//если исполнение показало, что надо теперь убрать эту цеплялку
			if(Result)
			{
				//помечаем как пустое, чтоб потом удалить
				Q.OwningQuest = nullptr;
			}
		}
	}
}
