// Fill out your copyright notice in the Description page of Project Settings.


#include "MyrKingdomOfHeaven.h"
#include "../MyrraGameInstance.h"							// ядро игры
#include "../MyrraGameModeBase.h"							// ядро игры
#include "Components/StaticMeshComponent.h"					// для куполов неба, дисков солнца и луны
#include "Components/DirectionalLightComponent.h"			// свет солнца и луны
#include "Components/SkyAtmosphereComponent.h"				// новая голубизна неба
#include "Components/ExponentialHeightFogComponent.h"		// туман у земли
#include "Components/SceneCaptureComponentCube.h"			// для захвата текстуры океружения
#include "Components/VolumetricCloudComponent.h"			// компонент объёмные облака, для кучевых
#include "Engine/TextureRenderTargetCube.h"
//#include "Classes/Atmosphere/AtmosphericFogComponent.h"		// старая голубизна неба
#include "Components/SkyLightComponent.h"					// внешний свет неба, адаптируется под окружение
#include "Curves/CurveLinearColor.h"						// таблица цветов 
#include "Curves/CurveVector.h"								// таблица положений солнца
#include "Materials/MaterialInstanceDynamic.h"				// для подводки материала неба
#include "Materials/MaterialParameterCollectionInstance.h"	// для подводки материала неба

#include "../MyrraGameModeBase.h"							// объект-уровень

#include "Engine/TextureCube.h"

#include "Engine/Public/TimerManager.h"

#include "UObject/UObjectBaseUtility.h"

//==============================================================================================================
// конструктор
//==============================================================================================================
AMyrKingdomOfHeaven::AMyrKingdomOfHeaven()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.31f;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	//слои неба приходится сразу разделить, чтобы сортировать по порядку отрисовки
	Sphaera = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarsDome"));
	Sphaera->SetTranslucentSortPriority(0);
	RootComponent = Sphaera;

	//объемные облака
	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("VolumetricCloud"));
	Clouds->SetupAttachment(RootComponent);

	//пока неясно, юзать его постоянно или наплодить с помощью него статических текстур
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponentCube>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(RootComponent);
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->ShowFlags.DisableAdvancedFeatures();
	SceneCapture->ShowFlags.StaticMeshes = 0;
	SceneCapture->ShowFlags.SkeletalMeshes = 0;
	SceneCapture->ShowFlags.Atmosphere = 1;
	SceneCapture->ShowFlags.Landscape = 0;


	//свет неба - в корне, он не должен двигаться
	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);

	//солнце, вращается относительно корня
	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLight->SetupAttachment(RootComponent);

	//туман, важна высота
	ExponentialFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	ExponentialFog->SetupAttachment(RootComponent);
	ExponentialFog->SetRelativeLocation(FVector(0, 0, -500));
	ExponentialFog->SetFogDensity(0.02);
	ExponentialFog->SetFogHeightFalloff(0.5);

	//компоненты, зависящие от поворота солнца
	if (SunLight)
	{
		//собственно цвет неба
		SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
		SkyAtmosphere->SetupAttachment(SunLight);

		//луна, сцеплена с солнцем, поскольку земля вращается вокруг своей оси быстрее, чем луна перемещается
		MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
		MoonLight->SetupAttachment(SunLight);
	}

}

void AMyrKingdomOfHeaven::PostLoad()
{
	Super::PostLoad();
}

#if WITH_EDITOR
void AMyrKingdomOfHeaven::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	//SceneCapture->TextureTarget->Resource->TextureRHI;
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMyrKingdomOfHeaven, DateOfPrecalculatedDays) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMyrKingdomOfHeaven, SunPositionsPerDay))
	{
		PrecalculateSunCycle();
	}
	else if(PropertyName == GET_MEMBER_NAME_CHECKED(AMyrKingdomOfHeaven, NumberOfCubemaps))
	{
		GenerateCubemaps();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AMyrKingdomOfHeaven, CurrentWeather))
	{
		ApplyWeathers(CurrentWeather);
	}
	

	ChangeTimeOfDay();
	//SkyLight->RecaptureSky();
	//SceneCapture->CaptureScene();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif


//==============================================================================================================
// Called when the game starts or when spawned
//==============================================================================================================
void AMyrKingdomOfHeaven::BeginPlay()
{
	PrimaryActorTick.TickInterval = 0.31f;
	PrimaryActorTick.SetTickFunctionEnable(true);
	Super::BeginPlay();

	//взвести переменную таймер текущей погоды, variation позволяет играть с заданной в сборке погоды длительностью
	if(CurrentWeather) ReloadWeather(CurrentWeather, 0.0f);

	//применить параметры выбранной погоды (в материалы неба, в глобальные параметры, в дождь перед камерой)
	ApplyWeathers(CurrentWeather);

}

//==============================================================================================================
// Called every frame
//==============================================================================================================
void AMyrKingdomOfHeaven::Tick(float DeltaTime)
{
	//соответствующий промежуток времени внутри игры
	float InGameDelta = DeltaTime * GameTimeDilation;

	//посчитать новое время
	FTimespan NewTimeOfDay = TimeOfDay + FTimespan(0, 0, 0, InGameDelta, (InGameDelta - FMath::FloorToFloat(InGameDelta)) * 1000000);

	//сменить время
	TimeOfDay = NewTimeOfDay;

	//обработать погоду
	BewareWeathersChange(DeltaTime);

	//обновить астрономические параметры
	ChangeTimeOfDay();
	Super::Tick(DeltaTime);

}

//==============================================================================================================
//после инициализации компонентов - важно, чтобы до любых BeginPlay на уровне
//==============================================================================================================
void AMyrKingdomOfHeaven::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	//подцепиться для объекта уровня, чтобы все существа на уровне знали время и погоду
	if (GetWorld())
		if (GetMyrGameMode())
			GetMyrGameMode()->Sky = this;

}



//==============================================================================================================
//создать текстуру из цели рендра
//==============================================================================================================
UTextureCube* AMyrKingdomOfHeaven::CreateTexture(UTextureRenderTargetCube* Source)
{
	//получить точную дату/время, в которой береётся проба неба
	FDateTime CurT(DateOfPrecalculatedDays.GetTicks() + TimeOfDay.GetTicks());

	//составить имя для ассета текстуры, из имени цели рендера и конкретных часов и минут
	int h = CurT.GetHour();
	int m = CurT.GetMinute();
	FString NewTextureName = Source->GetName() + FString::Printf(TEXT("Tex_%02dh%02dm"), h, m);

		//эта магия порождает имя файла-ассета, из которого вышел этот объект, с условным путём /Game/....
	FString PathPackage = Source->GetOutermost()->GetName();

	//имя источника нам не нужно, его надо вырезать, заменить именем текстуры
	PathPackage = PathPackage.Replace(*Source->GetName(), *NewTextureName);

	//создать болванку для файла-упаковки для нового ассета
	UPackage* Package = CreatePackage(nullptr, *PathPackage);
	Package->FullyLoad();


	//собственно сгенерировать живой отпечаток в памяти, сразу связанный с коробчонкой-файлом
	UTextureCube* Texture = Source->ConstructTextureCube(Package, NewTextureName, RF_Public | RF_Standalone | RF_MarkAsRootSet);

	// Updating Texture & mark it as unsaved
	Texture->AddToRoot();
	Texture->UpdateResource();
	Package->MarkPackageDirty();

	//сохранение на диск
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PathPackage, FPackageName::GetAssetPackageExtension());
	UPackage::SavePackage(Package, Texture, RF_Public | RF_Standalone | RF_MarkAsRootSet, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);
	UE_LOG(LogTemp, Error, TEXT("SKY %s: SavePackage %s in %s for %s"), *GetName(), *PackageFileName, *PathPackage, *Texture->GetName());


	return Texture;
}

//==============================================================================================================
//начать генерировать все нужные текстуры для эмбиента - 
//==============================================================================================================
void AMyrKingdomOfHeaven::GenerateCubemaps()
{
	//удалить предыдущий набор, если был
	SkyAmbients.Maps.SetNum(0);

	//если введено нуль карт, на этом прекратить всё
	if (NumberOfCubemaps == 0) return;

	//если захват сцены отключен, ничего нельзя сделать
	if (!SceneCapture->IsVisible()) SceneCapture->SetVisibility(true);

	//создать таймер, чтобы в промежутках система работала и устанавливалась
	GetWorld()->GetTimerManager().SetTimer(Timer, this, &AMyrKingdomOfHeaven::GenerateCubemapsOnTimer, 0.5f, true);

	//начальная точка отсчёта времени
	TimeOfDay = FTimespan::FromSeconds(0);

	//выполнить процедуру изменения состояния неба для следующего кадра
	ChangeTimeOfDay();
}

//==============================================================================================================
//сгенерировать очередную текстуру окружения для очередного времени суток - по таймеру
//==============================================================================================================
void AMyrKingdomOfHeaven::GenerateCubemapsOnTimer()
{
	//обновить текстуру окружения
	SceneCapture->CaptureScene();
		
	//создать тексутру для текущего времени 
	auto T = CreateTexture(SceneCapture->TextureTarget);

	//добавить созданную текстуру в массив
	SkyAmbients.Maps.Add(T);

	//если достигли нужного количества текстур окружения, прекратить процедуру, убив таймер
	if (SkyAmbients.Maps.Num() >= NumberOfCubemaps)
	{	
		GetWorld()->GetTimerManager().ClearTimer(Timer);
		SceneCapture->SetVisibility(false);
		return;
	}

	//выставление очередного времени для новой освещенности неба
	TimeOfDay = FTimespan::FromSeconds(SkyAmbients.Maps.Num() * 86400.0 / NumberOfCubemaps);

	//выполнить процедуру изменения состояния неба для следующего кадра
	ChangeTimeOfDay();
}

//==============================================================================================================
//вычислить кривую для определенного дня
//==============================================================================================================
void AMyrKingdomOfHeaven::PrecalculateSunCycle()
{
	float MaxSunZ = 0, ZeroSunZ = 100;
	
	//вычислить пороговые значения положения солнца
	GetSunRiseSet(Latitude, Longitude, DateOfPrecalculatedDays, SunriseFrac, SunsetFrac, NoonFrac);

	//если файл кривых имеется
	if (SunPositionsPerDay)
	{
		//очистить кривую
		SunPositionsPerDay->ResetCurve();
		TArray<FRichCurveEditInfo> Curves = SunPositionsPerDay->GetCurves();
		UE_LOG(LogTemp, Error, TEXT("SKY %s: Recalc sun positions %d for %s"), *GetName(), SunRotationSamples, *DateOfPrecalculatedDays.ToString());

		//проход по суткам с увеличенным разрешением, для точной фиксации рассветов, закатов,
		//точки на кривой расставляются с заданным разрешением
		double sinv = 0.25 / SunRotationSamples;
		for (int i = 0; i < 4*SunRotationSamples; i++)
		{
			//вычислить полное время первого дня опорной даты
			double CurrentCycleFraction = (double)i * sinv;
			FDateTime CurT(DateOfPrecalculatedDays.GetTicks() + (double)ETimespan::TicksPerDay * CurrentCycleFraction);

			//вычислить позицию солнца в двух координатах: угол поднятия, и азимут, также угол
			float Alt = 0;		float Azi = 0;
			GetSunPosition(Latitude, Longitude, CurT, Alt, Azi);

			//определение приближенных точек полудня, рассвета, заката
			float SunZ = -FRotator(Alt, Azi, 0).Vector().Z;
			if (SunZ > MaxSunZ) { MaxSunZ = SunZ; NoonFrac = CurrentCycleFraction; }
			if (FMath::Abs(SunZ) < ZeroSunZ)
			{ 
				ZeroSunZ = FMath::Abs(SunZ);
				if(CurT.IsMorning()) SunriseFrac = CurrentCycleFraction;
				else SunsetFrac = CurrentCycleFraction;
			}

			//каждый четвертый раз расставлять точку на кривой
			if ((i & 3) == 0)
			{
				//дабавить на первую кривую вектора сэмпл высоты
				FKeyHandle KeyHandle = Curves[0].CurveToEdit->AddKey(CurrentCycleFraction, Alt);
				Curves[0].CurveToEdit->SetKeyInterpMode(KeyHandle, RCIM_Linear);

				//найти предыдущую точку на кривой, если проищошёл перегиб, сделать перегиб резкий
				FKeyHandle KeyHandlePrev = Curves[0].CurveToEdit->GetPreviousKey(KeyHandle);
				if(KeyHandlePrev != FKeyHandle::Invalid())
					if (FMath::Abs(Curves[0].CurveToEdit->GetKeyValue(KeyHandlePrev) - Alt) > 140)
					{
						Curves[0].CurveToEdit->SetKeyInterpMode(KeyHandlePrev, RCIM_Constant);
					}

				//добавить на вторую кривую вектора семпл азимута
				KeyHandle = Curves[1].CurveToEdit->AddKey(CurrentCycleFraction, Azi);
				Curves[1].CurveToEdit->SetKeyInterpMode(KeyHandle, RCIM_Linear);

				//найти предыдущую точку на кривой, если проищошёл перегиб, сделать перегиб резкий
				KeyHandlePrev = Curves[1].CurveToEdit->GetPreviousKey(KeyHandle);
				if (KeyHandlePrev != FKeyHandle::Invalid())
					if (FMath::Abs(Curves[1].CurveToEdit->GetKeyValue(KeyHandlePrev) - Azi) > 140)
					{
						Curves[1].CurveToEdit->SetKeyInterpMode(KeyHandlePrev, RCIM_Constant);
					}
				UE_LOG(LogTemp, Error, TEXT("SKY %s: - add key for %dh,%dm Alt=%g, Azi=%g"), *GetName(), CurT.GetHour(), CurT.GetMinute(), Alt, Azi);
			}

		}
		//это зачем-то нужно внутри
		SunPositionsPerDay->Modify(true);
		SunPositionsPerDay->MarkPackageDirty();
	}

}



//==============================================================================================================
//изменить наклон солнца
//==============================================================================================================
void AMyrKingdomOfHeaven::ChangeTimeOfDay()
{
	//выставление позиции солна по таблице
	if (SunPositionsPerDay)
	{
		//доля суток - по-разному вычислять можно
		CurrentDayFraction = (TimeOfDay.GetTicks() % ETimespan::TicksPerDay) / (float)ETimespan::TicksPerDay;
		//CurrentDayFraction = FMath::Frac((float)TimeOfDay.GetTicks() / (float)ETimespan::TicksPerDay);
		//CurrentDayFraction = ((int64)(TimeOfDay.GetTicks() % ETimespan::TicksPerDay)) / (double)ETimespan::TicksPerDay;

		//получить уклон солнца по таблице от доли суток
		auto Rot = SunPositionsPerDay->GetVectorValue(CurrentDayFraction);

		//перевести его в углы эйлера и повернуть объект "солнце на радиус-векторе"
		SunLight->SetWorldRotation(FRotator(Rot.X, Rot.Y, 0));
#if DEBUG_LOG_SKY
		UE_LOG(LogTemp, Error, TEXT("SKY %s: ChangeTimeOfDay dayfrac=%g, Alt=%g, Azi=%g"), *GetName(), CurrentDayFraction, Rot.X, Rot.Y);
#endif
	}
	SunLight->UpdateChildTransforms();

	//обновление шейдеров цвета неба
	CorelateSunLigntToAngle();
}

//==============================================================================================================
//пересчитать яркость и цвет солнца в зависимости от высоты (высота уже тем или иным образом определена)
//==============================================================================================================
void AMyrKingdomOfHeaven::CorelateSunLigntToAngle()
{
	//инстанция глобальных параметров материалов
	//нужна именно локальная переменная, поскольку новые значения отгружаются только в деструкторе
	auto MPC = MakeMPCInst();

	//если имеются предвычисленные карты окружения
	//и если эта функция вызывается не входе вычисления карт окружения
	if(SkyAmbients.Maps.Num() && NumberOfCubemaps == SkyAmbients.Maps.Num())
	{
		uint8 iAmb1;
		uint8 iAmb2;
		float Alpha;
		SelectCubemapsForCurrentTime(iAmb1, iAmb2, Alpha);
		SkyLight->SetCubemapBlend(SkyAmbients.Maps[iAmb1], SkyAmbients.Maps[iAmb2], Alpha);
	}

	//предопределенная кривая интенсивностей и цветов солнца
	if (SunIntensitiesPerZ)
	{
		auto Color = SunIntensitiesPerZ->GetUnadjustedLinearColorValue(-SunLight->GetDirection().Z);
		SunLight->SetIntensity(Color.A * ZenithSunIntensity);
		SunLight->SetLightColor(Color);
		//SkyLight->SetIntensity(Color.A);

		MPC->SetVectorParameterValue(TEXT("DirectionToSun"), -SunLight->GetDirection());
		MPC->SetVectorParameterValue(TEXT("SunColor"), Color);

		//таблица цветов луны
		if (MoonIntensitiesPerZ)
		{
			//цвет луны
			auto Color2 = MoonIntensitiesPerZ->GetUnadjustedLinearColorValue(-MoonLight->GetDirection().Z);

			//фаза луны, максимальная яркость, если вектора строго разнонаправлены, это полная луна
			float Phase = 0.5f + 0.5f*FVector::DotProduct(-SunLight->GetDirection(), MoonLight->GetDirection());

			//интенсивность сискусственно снижаем, если интенсивность солнца сейчас слишком высокая
			//это костыль, чтобы не мучиться настройками HDR, но пусть пока будет
			//а вот фаза - обязательная штука
			Color2.A = (Color2.A - Color.A) ;
			MoonLight->SetIntensity(Color2.A * Phase * ZenithMoonIntensity);
			MoonLight->SetLightColor(Color2);
			MPC->SetVectorParameterValue(TEXT("MoonColor"), Color2);
		}
	}
	if (ZenithColorsPerSunZ)
	{
		auto Color = ZenithColorsPerSunZ->GetUnadjustedLinearColorValue(-SunLight->GetDirection().Z);
		MPC->SetVectorParameterValue(TEXT("SkyColorZenith"), Color);
	}
	if (GroundColorsPerSunZ)
	{
		auto Color = GroundColorsPerSunZ->GetUnadjustedLinearColorValue(-SunLight->GetDirection().Z);
		MPC->SetVectorParameterValue(TEXT("SkyColorNadir"), Color);
	}
#if DEBUG_LOG_SKY
	UE_LOG(LogTemp, Error, TEXT("SKY %s: Sunlight recalc dir=%g"), *GetName(), SunLight->GetDirection().Z);
#endif
}

//==============================================================================================================
//для текущего времени (TimeOfDay) получить две ближайшие карты окружения (индексы в массиве) и коэффициент смеси для них
//==============================================================================================================
void AMyrKingdomOfHeaven::SelectCubemapsForCurrentTime(uint8& iMap1, uint8& iMap2, float& TransitCoef)
{
	//часть суток в диапазоне 0-1
	//float DayFraction = ((int64)(TimeOfDay.GetTicks() % ETimespan::TicksPerDay)) / (double)ETimespan::TicksPerDay;

	//часть суток в диапазоне количества имеющихся карта окружения
	float FayFractionInSamples = CurrentDayFraction * SkyAmbients.Maps.Num();

	//целочисленные границы = номера ближайших карт
	iMap1 = FMath::FloorToInt(FayFractionInSamples);
	iMap2 = FMath::CeilToInt(FayFractionInSamples);
	if (iMap1 == SkyAmbients.Maps.Num()) iMap1 = 0;
	if (iMap2 == SkyAmbients.Maps.Num()) iMap2 = 0;
	TransitCoef = FayFractionInSamples - (float)iMap1;
}

//==============================================================================================================
//определить критерии для смены погоды и подготовить смену (выбрать новую погоду)
// - выполняется равномерно в тике
//==============================================================================================================
void AMyrKingdomOfHeaven::BewareWeathersChange(float DeltaTime)
{
	//разделка суток по периодам
	auto TimesOfDay = MorningDayEveningNight();

	if (CurrentWeather)
	{
		//конкретный исход
		float DiceRoll = FMath::RandRange(0.0f, 1.0f);

		//имеется следующая погода
		if (UpcomingWeather)
		{
			//выполнение смеси погод с нужным коэффициентом
			WeatherTransition += DeltaTime * 0.1;

			//Если коэффициент смеси достиг 1, значит вторая погода полностью заменила первую
			if (WeatherTransition >= 1.0f)
			{
				//запустить вторую погоду как единственную, на случайно выбранное время
				CurrentWeather = UpcomingWeather;
				UpcomingWeather = nullptr;
				ReloadWeather(CurrentWeather, DiceRoll);
				PrimaryActorTick.TickInterval = TickIntervalStable;
			}

			//собственно применить графически изменения в виде погоды
			ApplyWeathers(CurrentWeather, UpcomingWeather, WeatherTransition);
		}
		//текущая погода стабильна, перехода нет
		else
		{
			//если время стабильного участка текущей погоды подошло к концу
			if (WeatherRemainingTime <= 0)
			{
				//у текущей погоды должны быть возможные переходы
				for (auto W2 : CurrentWeather->OrderedTransitions)
				{
					//вероятность применения данной новой погоды с учётом времени дня
					float Probability =
						TimesOfDay.R * CurrentWeather->ChanceMorning +
						TimesOfDay.G * CurrentWeather->ChanceMidday +
						TimesOfDay.B * CurrentWeather->ChanceEvening +
						TimesOfDay.A * CurrentWeather->ChanceMidnight;

					//шанс не упущен - новая погода выбрана
					if (Probability > DiceRoll)
					{
						//перейти в состояние с двумя погодами, реальный переход будет отрабатываться уже в следующий такт
						UpcomingWeather = W2;
						WeatherTransition = 0.0f;
						PrimaryActorTick.TickInterval = TickIntervalWeatherChange;
						break;
					}
				}
				//не нашли подходящего перехода - перезапуск текущей погоды (перевод из минут в секунды, далее из игровых в реальные секунды)
				if (!UpcomingWeather) ReloadWeather(CurrentWeather, DiceRoll);
			}
			//если время стабильной погоды ещё есть - просто обновить счётчик
			else WeatherRemainingTime -= DeltaTime;
		}
	}

	//проверка, так как при тике в редакторе это не сработает
	if (GetMyrGameMode())
	{
		//применить текущую плотность дождя (в протагонисте рассчитывается остаточная мокрота)
		GetMyrGameMode()->SetWeatherRainAmount(RainAmount);

		//изменить звуки/музыку в зависимости от погоды и времени суток
		GetMyrGameMode()->SetAmbientSoundParameters(TimesOfDay.G, RainAmount);
	}

}

//==============================================================================================================
//применить/изменить погоду (в материалах неба)
//сами доли перехода здесь не рассчитываются, вызывается только когда происходит изменение в погоде
// но уровень дождя здесь только считается, реально применяься он должен равномерно в тике
//==============================================================================================================
void AMyrKingdomOfHeaven::ApplyWeathers(UMyrKingdomOfHeavenWeather* W1, UMyrKingdomOfHeavenWeather* W2, float A)
{
	//если даже текущая погода не дана
	if (!W1) return;

	//инстанция глобальных параметров материалов
	//нужна именно локальная переменная, поскольку новые значения отгружаются только в деструкторе
	auto MPC = MakeMPCInst();
	if (!MPC) return;

	FVector2D WindDirL;

	//еимеется погода, с которой надо смешивать текущую
	if (W2 && W2!=W1)
	{
		ExponentialFog->SetFogDensity(												FMath::Lerp(W1->FogDensity, W2->FogDensity, A));
		ExponentialFog->SetFogHeightFalloff(										FMath::Lerp(W1->FogFalloff, W2->FogFalloff, A));
		ExponentialFog->SetWorldLocation(FVector(0,0,								FMath::Lerp(W1->FogZCoordinate, W2->FogZCoordinate, A)));
		ExponentialFog->SecondFogData.FogDensity =									FMath::Lerp(W1->SecondFogDensity, W2->SecondFogDensity, A);
		ExponentialFog->VolumetricFogScatteringDistribution =						FMath::Lerp(W1->VolumetricFogScatteringDistribution, W2->VolumetricFogScatteringDistribution, A);
		ExponentialFog->VolumetricFogExtinctionScale =								FMath::Lerp(W1->VolumetricFogExtinctionScale, W2->VolumetricFogExtinctionScale, A);
		WindDirL = FVector2D(FMath::Lerp(W1->Wind.X, W2->Wind.X, A), FMath::Lerp(W1->Wind.Y, W2->Wind.Y, A));
		RainAmount = FMath::Lerp(W1->RainAmount, W2->RainAmount, A);
		CurrentWeatherEmotion = FMath::Lerp(W1->EmotionColor, W2->EmotionColor, A);

		//по-новому, чтобы не выбирать отдельный материал
		MPC->SetScalarParameterValue(TEXT("CurrentWeatherCirrusAmount"), FMath::Lerp(W1->CirrusAmount, W2->CirrusAmount, A));
		MPC->SetScalarParameterValue(TEXT("CurrentWeatherCumulusAmount"), FMath::Lerp(W1->CumulusAmount, W2->CumulusAmount, A));

	}
	//погода стабильна, не меняется (вызывается один раз на весь период стабильности)
	else
	{
		ExponentialFog->SetFogDensity(												W1->FogDensity);
		ExponentialFog->SetFogHeightFalloff(										W1->FogFalloff);
		ExponentialFog->SetWorldLocation(FVector(0, 0,								W1->FogZCoordinate));
		ExponentialFog->SecondFogData.FogDensity =									W1->SecondFogDensity;
		ExponentialFog->VolumetricFogScatteringDistribution =						W1->VolumetricFogScatteringDistribution;
		ExponentialFog->VolumetricFogExtinctionScale =								W1->VolumetricFogExtinctionScale;
		WindDirL = FVector2D(W1->Wind.X, W1->Wind.Y);
		RainAmount = W1->RainAmount;
		CurrentWeatherEmotion = W1->EmotionColor;

		MPC->SetScalarParameterValue(TEXT("CurrentWeatherCirrusAmount"), W1->CirrusAmount);
		MPC->SetScalarParameterValue(TEXT("CurrentWeatherCumulusAmount"), W1->CumulusAmount);
	}

	if(GetMyrGameMode())
		if (GetMyrGameMode()->WindDir())
		{
			WindDir() = WindDirL; 
			MPC->SetVectorParameterValue(TEXT("WindDir"), FLinearColor(WindDirL.X, WindDirL.Y, 0, 0));
		}

	if (ExponentialFog->VolumetricFogExtinctionScale < 0.001) ExponentialFog->SetVolumetricFog(false);
	else ExponentialFog->SetVolumetricFog(true);
}

//направление ветра (теперь лежит в другом классе, посему доступ через функцию)
FVector2D& AMyrKingdomOfHeaven::WindDir()
{
	return *GetMyrGameMode()->WindDir();
}

//==============================================================================================================
//прямой способ расчёта времени дня в виде долей 
//==============================================================================================================
FLinearColor AMyrKingdomOfHeaven::MorningDayEveningNight()
{
	//очень эмпирически и неточно
	return FLinearColor
	(+
		4.0 * FMath::Max(0.0f, 0.25f - FMath::Abs(CurrentDayFraction - SunriseFrac - 0.1f)),	//утро
		2.0 * FMath::Max(0.0f, 0.5f  - FMath::Abs(CurrentDayFraction - NoonFrac)),				//день
		3.0 * FMath::Max(0.0f, 0.33f - FMath::Abs(CurrentDayFraction - SunsetFrac + 0.05f)),	//вечер
		FMath::Clamp(5 * SunLight->GetDirection().Z, 0.0f, 1.0f)								//ночь
	);
}

//====================================================================================================
//создать для текущей функции локальную инстанцию коллекции параметров материала
//====================================================================================================
UMaterialParameterCollectionInstance* AMyrKingdomOfHeaven::MakeMPCInst()
{
	return GetWorld()->GetParameterCollectionInstance(EnvMatParam);
}


UFUNCTION(BlueprintCallable) void AMyrKingdomOfHeaven::GetSunPosition(float Latitudo, float Longitudo, const FDateTime DateTime, float& altitude, float& azimuth)
{

	double LatitudeRad = FMath::DegreesToRadians(Latitudo);

	// Get the julian day (number of days since Jan 1st of the year 4713 BC)
	double JulianDay = DateTime.GetJulianDay() + (DateTime.GetTimeOfDay().GetTotalHours() /*- TimeOffset*/) / 24.0;
	double JulianCentury = (JulianDay - 2451545.0) / 36525.0;

	// Get the sun's mean longitude , referred to the mean equinox of julian date
	double GeomMeanLongSunDeg = FMath::Fmod(280.46646 + JulianCentury * (36000.76983 + JulianCentury * 0.0003032), 360.0f);
	double GeomMeanLongSunRad = FMath::DegreesToRadians(GeomMeanLongSunDeg);

	// Get the sun's mean anomaly
	double GeomMeanAnomSunDeg = 357.52911 + JulianCentury * (35999.05029 - 0.0001537 * JulianCentury);
	double GeomMeanAnomSunRad = FMath::DegreesToRadians(GeomMeanAnomSunDeg);

	// Get the earth's orbit eccentricity
	double EccentEarthOrbit = 0.016708634 - JulianCentury * (0.000042037 + 0.0000001267 * JulianCentury);

	// Get the sun's equation of the center
	double SunEqOfCtr = FMath::Sin(GeomMeanAnomSunRad) * (1.914602 - JulianCentury * (0.004817 + 0.000014 * JulianCentury))
		+ FMath::Sin(2.0 * GeomMeanAnomSunRad) * (0.019993 - 0.000101 * JulianCentury)
		+ FMath::Sin(3.0 * GeomMeanAnomSunRad) * 0.000289;

	// Get the sun's true longitude
	double SunTrueLongDeg = GeomMeanLongSunDeg + SunEqOfCtr;

	// Get the sun's true anomaly
	//	double SunTrueAnomDeg = GeomMeanAnomSunDeg + SunEqOfCtr;
	//	double SunTrueAnomRad = FMath::DegreesToRadians(SunTrueAnomDeg);

	// Get the earth's distance from the sun
	//	double SunRadVectorAUs = (1.000001018*(1.0 - EccentEarthOrbit*EccentEarthOrbit)) / (1.0 + EccentEarthOrbit*FMath::Cos(SunTrueAnomRad));

	// Get the sun's apparent longitude
	double SunAppLongDeg = SunTrueLongDeg - 0.00569 - 0.00478 * FMath::Sin(FMath::DegreesToRadians(125.04 - 1934.136 * JulianCentury));
	double SunAppLongRad = FMath::DegreesToRadians(SunAppLongDeg);

	// Get the earth's mean obliquity of the ecliptic
	double MeanObliqEclipticDeg = 23.0 + (26.0 + ((21.448 - JulianCentury * (46.815 + JulianCentury * (0.00059 - JulianCentury * 0.001813)))) / 60.0) / 60.0;

	// Get the oblique correction
	double ObliqCorrDeg = MeanObliqEclipticDeg + 0.00256 * FMath::Cos(FMath::DegreesToRadians(125.04 - 1934.136 * JulianCentury));
	double ObliqCorrRad = FMath::DegreesToRadians(ObliqCorrDeg);

	// Get the sun's right ascension
	double SunRtAscenRad = FMath::Atan2(FMath::Cos(ObliqCorrRad) * FMath::Sin(SunAppLongRad), FMath::Cos(SunAppLongRad));
	double SunRtAscenDeg = FMath::RadiansToDegrees(SunRtAscenRad);

	// Get the sun's declination
	double SunDeclinRad = FMath::Asin(FMath::Sin(ObliqCorrRad) * FMath::Sin(SunAppLongRad));
	double SunDeclinDeg = FMath::RadiansToDegrees(SunDeclinRad);

	double VarY = FMath::Pow(FMath::Tan(ObliqCorrRad / 2.0), 2.0);

	// Get the equation of time
	double EqOfTimeMinutes = 4.0 * FMath::RadiansToDegrees(VarY * FMath::Sin(2.0 * GeomMeanLongSunRad) - 2.0 * EccentEarthOrbit * FMath::Sin(GeomMeanAnomSunRad) + 4.0 * EccentEarthOrbit * VarY * FMath::Sin(GeomMeanAnomSunRad) * FMath::Cos(2.0 * GeomMeanLongSunRad) - 0.5 * VarY * VarY * FMath::Sin(4.0 * GeomMeanLongSunRad) - 1.25 * EccentEarthOrbit * EccentEarthOrbit * FMath::Sin(2.0 * GeomMeanAnomSunRad));

	// Get the hour angle of the sunrise
	double HASunriseDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Cos(FMath::DegreesToRadians(90.833)) / (FMath::Cos(LatitudeRad) * FMath::Cos(SunDeclinRad)) - FMath::Tan(LatitudeRad) * FMath::Tan(SunDeclinRad)));
	//	double SunlightDurationMinutes = 8.0 * HASunriseDeg;

	// Get the local time of the sun's rise and set
	double SolarNoonLST = (720.0 - 4.0 * Longitudo - EqOfTimeMinutes/* + TimeOffset * 60.0*/) / 1440.0;
	double SunriseTimeLST = SolarNoonLST - HASunriseDeg * 4.0 / 1440.0;
	double SunsetTimeLST = SolarNoonLST + HASunriseDeg * 4.0 / 1440.0;

	// Get the true solar time
	double TrueSolarTimeMinutes = FMath::Fmod(DateTime.GetTimeOfDay().GetTotalMinutes() + EqOfTimeMinutes + 4.0 * Longitudo/* - 60.0 * TimeOffset*/, 1440.0);

	// Get the hour angle of current time
	double HourAngleDeg = TrueSolarTimeMinutes < 0 ? TrueSolarTimeMinutes / 4.0 + 180 : TrueSolarTimeMinutes / 4.0 - 180.0;
	double HourAngleRad = FMath::DegreesToRadians(HourAngleDeg);

	// Get the solar zenith angle
	double SolarZenithAngleRad = FMath::Acos(FMath::Sin(LatitudeRad) * FMath::Sin(SunDeclinRad) + FMath::Cos(LatitudeRad) * FMath::Cos(SunDeclinRad) * FMath::Cos(HourAngleRad));
	double SolarZenithAngleDeg = FMath::RadiansToDegrees(SolarZenithAngleRad);

	// Get the sun elevation
	double SolarElevationAngleDeg = 90.0 - SolarZenithAngleDeg;
	double SolarElevationAngleRad = FMath::DegreesToRadians(SolarElevationAngleDeg);
	double TanOfSolarElevationAngle = FMath::Tan(SolarElevationAngleRad);

	// Get the approximated atmospheric refraction
	double ApproxAtmosphericRefractionDeg = 0.0;
	if (SolarElevationAngleDeg <= 85.0)
	{
		if (SolarElevationAngleDeg > 5.0)
		{
			ApproxAtmosphericRefractionDeg = 58.1 / TanOfSolarElevationAngle - 0.07 / FMath::Pow(TanOfSolarElevationAngle, 3) + 0.000086 / FMath::Pow(TanOfSolarElevationAngle, 5) / 3600.0;
		}
		else
		{
			if (SolarElevationAngleDeg > -0.575)
			{
				ApproxAtmosphericRefractionDeg = 1735.0 + SolarElevationAngleDeg * (-518.2 + SolarElevationAngleDeg * (103.4 + SolarElevationAngleDeg * (-12.79 + SolarElevationAngleDeg * 0.711)));
			}
			else
			{
				ApproxAtmosphericRefractionDeg = -20.772 / TanOfSolarElevationAngle;
			}
		}
		ApproxAtmosphericRefractionDeg /= 3600.0;
	}

	// Get the corrected solar elevation
	double SolarElevationcorrectedforatmrefractionDeg = SolarElevationAngleDeg + ApproxAtmosphericRefractionDeg;

	// Get the solar azimuth 
	double tmp = FMath::RadiansToDegrees(FMath::Acos(((FMath::Sin(LatitudeRad) * FMath::Cos(SolarZenithAngleRad)) - FMath::Sin(SunDeclinRad)) / (FMath::Cos(LatitudeRad) * FMath::Sin(SolarZenithAngleRad))));
	double SolarAzimuthAngleDegcwfromN = HourAngleDeg > 0.0f ? FMath::Fmod(tmp + 180.0f, 360.0f) : FMath::Fmod(540.0f - tmp, 360.0f);


	// offset elevation angle to fit with UE coords system
	altitude = 180.0f + SolarElevationcorrectedforatmrefractionDeg;
	azimuth = SolarAzimuthAngleDegcwfromN;
}

UFUNCTION(BlueprintCallable) void AMyrKingdomOfHeaven::GetSunRiseSet(float Latitudo, float Longitudo, const FDateTime DateTime, float& Rise, float& Set, float& Noon)
{

	double LatitudeRad = FMath::DegreesToRadians(Latitudo);

	// Get the julian day (number of days since Jan 1st of the year 4713 BC)
	double JulianDay = DateTime.GetJulianDay() + (DateTime.GetTimeOfDay().GetTotalHours() /*- TimeOffset*/) / 24.0;
	double JulianCentury = (JulianDay - 2451545.0) / 36525.0;

	// Get the sun's mean longitude , referred to the mean equinox of julian date
	double GeomMeanLongSunDeg = FMath::Fmod(280.46646 + JulianCentury * (36000.76983 + JulianCentury * 0.0003032), 360.0f);
	double GeomMeanLongSunRad = FMath::DegreesToRadians(GeomMeanLongSunDeg);

	// Get the sun's mean anomaly
	double GeomMeanAnomSunDeg = 357.52911 + JulianCentury * (35999.05029 - 0.0001537 * JulianCentury);
	double GeomMeanAnomSunRad = FMath::DegreesToRadians(GeomMeanAnomSunDeg);

	// Get the earth's orbit eccentricity
	double EccentEarthOrbit = 0.016708634 - JulianCentury * (0.000042037 + 0.0000001267 * JulianCentury);

	// Get the sun's equation of the center
	double SunEqOfCtr = FMath::Sin(GeomMeanAnomSunRad) * (1.914602 - JulianCentury * (0.004817 + 0.000014 * JulianCentury))
		+ FMath::Sin(2.0 * GeomMeanAnomSunRad) * (0.019993 - 0.000101 * JulianCentury)
		+ FMath::Sin(3.0 * GeomMeanAnomSunRad) * 0.000289;

	// Get the sun's true longitude
	double SunTrueLongDeg = GeomMeanLongSunDeg + SunEqOfCtr;

	// Get the sun's true anomaly
	//	double SunTrueAnomDeg = GeomMeanAnomSunDeg + SunEqOfCtr;
	//	double SunTrueAnomRad = FMath::DegreesToRadians(SunTrueAnomDeg);

	// Get the earth's distance from the sun
	//	double SunRadVectorAUs = (1.000001018*(1.0 - EccentEarthOrbit*EccentEarthOrbit)) / (1.0 + EccentEarthOrbit*FMath::Cos(SunTrueAnomRad));

	// Get the sun's apparent longitude
	double SunAppLongDeg = SunTrueLongDeg - 0.00569 - 0.00478 * FMath::Sin(FMath::DegreesToRadians(125.04 - 1934.136 * JulianCentury));
	double SunAppLongRad = FMath::DegreesToRadians(SunAppLongDeg);

	// Get the earth's mean obliquity of the ecliptic
	double MeanObliqEclipticDeg = 23.0 + (26.0 + ((21.448 - JulianCentury * (46.815 + JulianCentury * (0.00059 - JulianCentury * 0.001813)))) / 60.0) / 60.0;

	// Get the oblique correction
	double ObliqCorrDeg = MeanObliqEclipticDeg + 0.00256 * FMath::Cos(FMath::DegreesToRadians(125.04 - 1934.136 * JulianCentury));
	double ObliqCorrRad = FMath::DegreesToRadians(ObliqCorrDeg);

	// Get the sun's right ascension
	double SunRtAscenRad = FMath::Atan2(FMath::Cos(ObliqCorrRad) * FMath::Sin(SunAppLongRad), FMath::Cos(SunAppLongRad));
	double SunRtAscenDeg = FMath::RadiansToDegrees(SunRtAscenRad);

	// Get the sun's declination
	double SunDeclinRad = FMath::Asin(FMath::Sin(ObliqCorrRad) * FMath::Sin(SunAppLongRad));
	double SunDeclinDeg = FMath::RadiansToDegrees(SunDeclinRad);

	double VarY = FMath::Pow(FMath::Tan(ObliqCorrRad / 2.0), 2.0);

	// Get the equation of time
	double EqOfTimeMinutes = 4.0 * FMath::RadiansToDegrees(VarY * FMath::Sin(2.0 * GeomMeanLongSunRad) - 2.0 * EccentEarthOrbit * FMath::Sin(GeomMeanAnomSunRad) + 4.0 * EccentEarthOrbit * VarY * FMath::Sin(GeomMeanAnomSunRad) * FMath::Cos(2.0 * GeomMeanLongSunRad) - 0.5 * VarY * VarY * FMath::Sin(4.0 * GeomMeanLongSunRad) - 1.25 * EccentEarthOrbit * EccentEarthOrbit * FMath::Sin(2.0 * GeomMeanAnomSunRad));

	// Get the hour angle of the sunrise
	double HASunriseDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Cos(FMath::DegreesToRadians(90.833)) / (FMath::Cos(LatitudeRad) * FMath::Cos(SunDeclinRad)) - FMath::Tan(LatitudeRad) * FMath::Tan(SunDeclinRad)));
	//	double SunlightDurationMinutes = 8.0 * HASunriseDeg;

	// Get the local time of the sun's rise and set
	Noon = (720.0 - 4.0 * Longitudo - EqOfTimeMinutes/* + TimeOffset * 60.0*/) / 1440.0;
	Rise = Noon - HASunriseDeg * 4.0 / 1440.0;
	Set = Noon + HASunriseDeg * 4.0 / 1440.0;

}
