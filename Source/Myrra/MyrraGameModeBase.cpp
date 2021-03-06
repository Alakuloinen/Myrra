// Fill out your copyright notice in the Description page of Project Settings.

#include "MyrraGameModeBase.h"
#include "MyrraGameInstance.h"
#include "Control/MyrDaemon.h"
#include "Creature/MyrPhyCreature.h"
#include "Control/MyrPlayerController.h"
#include "Components/AudioComponent.h"					// звук
#include "Sky/MyrKingdomOfHeaven.h"					// небо
#include "Materials/MaterialParameterCollectionInstance.h"	// для подводки материала неба
#include "Kismet/GameplayStatics.h"						// для вызова GetAllActorsOfClass
#include "AssetStructures/MyrLogicEmotionReactions.h"					// для выкоревывания эмоциональных стимулов

//вызывается, когда очередной трек музыки подходит к концу
void AMyrraGameModeBase::OnAudioFinished()
{
	UE_LOG(LogTemp, Log, TEXT("%s: OnAudioFinished"), *GetName());
}

//==============================================================================================================
//конструктор
//==============================================================================================================
AMyrraGameModeBase::AMyrraGameModeBase() : Super()
{
	//непонятно пока, на что это влияет, но пусть будет так
	PlayerControllerClass = AMyrPlayerController::StaticClass();
	ProtagonistClass = AMyrDaemon::StaticClass();

	//источник звука музыки
	Music = CreateDefaultSubobject<UAudioComponent>(TEXT("Music"));
	RootComponent = Music;
	Music->OnAudioFinished.AddDynamic(this, &AMyrraGameModeBase::OnAudioFinished);

}

//==============================================================================================================
//при загрузке уровня
//==============================================================================================================
void AMyrraGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	PrimaryActorTick.SetTickFunctionEnable(true);
	Super::InitGame(MapName, Options, ErrorMessage);
}



//==============================================================================================================
// начать игру
//==============================================================================================================
void AMyrraGameModeBase::BeginPlay()
{
	Music->Play();
	Super::BeginPlay();

	//загрузка сохранения
	if (GetMyrGameInst()->JustLoadedSlot)
	{

	}
	//загрузка начисто, возможно, такого не будет 
	else
	{
	}


	//если переменная протагониста не заполнена
	//а она скорее всего заполнена - в самом демоне в PostInitializeComponents
	//он отыскивает данный объект и впендюривает к нам себя сам
	if (!Protagonist)
	{
		//попытаться разыскать его среди объектов стандартного класса, указанного в настройках 
		//по идее он найдет либо помещенного в сцену, либо динамчески спавненного вот только что
		FTransform tr;
		TArray<AActor*> Creatures;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ProtagonistClass, Creatures);
		if (Creatures.Num() > 0)
			Protagonist = (AMyrDaemon*)Creatures[0];
		else Protagonist = (AMyrDaemon*)GetWorld()->SpawnActor(ProtagonistClass, &tr);
	}

	//если почему-то не проспавгился контроллер, создать его
	if(!Protagonist->Controller)
		Protagonist->SpawnDefaultController();

	//если павн-демон проспавнился вот прямо сейчас (а в сцене не было)
	//то к нему не подвязано существо, поэтому найти первое подходящяя существо и подвязять
	if (!Protagonist->OwnedCreature)
	{	TArray<AActor*> Creatures;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyrPhyCreature::StaticClass(), Creatures);
		if (Creatures.Num() > 0)
			Protagonist->ClingToCreature(Creatures[0]);
	}

	//если небо себя добавило или уже было довалено
	if (Sky)
	{
		//связать напрямую игрока и небо
		if (!Sky->Protagonist) Sky->Protagonist = Protagonist;
		if (!Protagonist->Sky) Protagonist->Sky = Sky;
	}
	//ВНИМАНИЕ, это включает отображение всех форм коллизий, наглядно но медленно
	//GetWorld()->Exec(GetWorld(), TEXT("pxvis collision"));
}



//==============================================================================================================
//установить видимый уровень дождя
//==============================================================================================================
void AMyrraGameModeBase::SetWeatherRainAmount(float Amount)
{
	if (Protagonist)
		Protagonist->SetWeatherRainAmount(Amount);
	else UE_LOG(LogTemp, Error, TEXT("%s: No Protagonist!"), *GetName());
}

//==============================================================================================================
//модифицировать параметры звука фона и музыки, от времени дня, наличия дождя
//==============================================================================================================
void AMyrraGameModeBase::SetAmbientSoundParameters(float DayFraction, float Rain)
{
	Music->SetFloatParameter(TEXT("RainAmount"), Rain);
	Music->SetIntParameter(TEXT("Hour"), DayFraction * 24);
}

//==============================================================================================================
// модифицировать параметры звука по части призвука плохого самочувствия
//==============================================================================================================
void AMyrraGameModeBase::SetSoundLowHealth(float Health, float Metabolism, float Pain)
{
	Music->SetFloatParameter(TEXT("Health"), Health);
	Music->SetFloatParameter(TEXT("Metabolism"), Metabolism);
	Music->SetFloatParameter(TEXT("Pain"), Pain);
}

//==============================================================================================================
//сменить на этом уровне протагониста
//==============================================================================================================
void AMyrraGameModeBase::ChangeProtagonist(FTriggerReason R, AMyrPhyCreature* Former)
{
	if (!Former->IsUnderDaemon()) return;
	Protagonist->ReleaseCreature();
	TArray<AActor*> FoundCreatures;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyrPhyCreature::StaticClass(), FoundCreatures);
	for (auto& Actor : FoundCreatures)
	{
		if (Actor->GetName() == R.Value)
		{
			Protagonist->ClingToCreature(Actor);
		}
	}
}



//==============================================================================================================
//протагониста в виде конесного существа
//==============================================================================================================
UFUNCTION(BlueprintCallable)AMyrPhyCreature* AMyrraGameModeBase::GetProtagonist()
{
	if (Protagonist) return Protagonist->GetOwnedCreature();
	else return nullptr;
}

//==============================================================================================================
//тип восприятия протагониста - нормальное, чутьё на одних, чутьё на других
//==============================================================================================================
int AMyrraGameModeBase::ProtagonistSenseMode() const
{
	return Protagonist->IsFirstPerson();
}

//==============================================================================================================
// от первого лица ли
//==============================================================================================================
bool AMyrraGameModeBase::ProtagonistFirstPerson() const
{
	return (Protagonist->MyrCameraMode == EMyrCameraMode::FirstPerson);
}

//====================================================================================================
//для списка воздействий в интерфейсе выдать вот это вот воздействие для главного героя
//====================================================================================================
FEmoStimulus AMyrraGameModeBase::GetMyStimulus(EEmoCause Cause) const
{
	if (Protagonist)
		if (Protagonist->OwnedCreature)
		{
			if (Protagonist->OwnedCreature->EmoReactions.Map.Num() > 0)
			{
				if (auto R = Protagonist->OwnedCreature->EmoReactions.Map.Find(Cause))
					return *R;
			}
			else
			{
				if (Protagonist->OwnedCreature->GetGenePool())
					if (Protagonist->OwnedCreature->GetGenePool()->EmoReactions)
						if (auto R = Protagonist->OwnedCreature->GetGenePool()->EmoReactions->List.Map.Find(Cause))
							return *R;
			}
		}
	return FEmoStimulus();
}
//направление ветра (теперь лежит в другом классе, посему доступ через функцию)
FVector2D* AMyrraGameModeBase::WindDir()
{	return Sky ? &Sky->WindDir : nullptr; }

//абсолютная позиция массы воздуха, для сдвига карты вета, облаков и погоды
FVector2D* AMyrraGameModeBase::AirMassPosition()
{	return Sky ? &Sky->AirMassPosition : nullptr; }

//абсолютная позиция массы воздуха, для сдвига карты вета, облаков и погоды
double* AMyrraGameModeBase::WeatherMapPosition()
{	return Sky ? Sky->WeatherMapPosition : nullptr; }

