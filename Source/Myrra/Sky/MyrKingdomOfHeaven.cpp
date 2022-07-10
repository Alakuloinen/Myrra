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
#include "Engine/TextureRenderTargetCube.h"					// для рендера карт окружения
#include "Engine/TextureCube.h"
#include "Components/SkyLightComponent.h"					// внешний свет неба, адаптируется под окружение
#include "Curves/CurveLinearColor.h"						// таблица цветов 
#include "Curves/CurveVector.h"								// таблица положений солнца
#include "Materials/MaterialInstanceDynamic.h"				// для подводки материала неба
#include "Materials/MaterialParameterCollectionInstance.h"	// для подводки материала неба

#include "Rendering/Texture2DResource.h"					// для извлечения текселей карты погоды

#include "../MyrraGameModeBase.h"							// объект-уровень
#include "../Control/MyrDaemon.h"							// наблюдатель и первое лицо


#include "Engine/Public/TimerManager.h"

#include "UObject/UObjectBaseUtility.h"

#define MUT(x) (PropertyName == GET_MEMBER_NAME_CHECKED(AMyrKingdomOfHeaven, x))
//==============================================================================================================
// конструктор
//==============================================================================================================
AMyrKingdomOfHeaven::AMyrKingdomOfHeaven()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.31f;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	//свет неба - в корне, он не должен двигаться
	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetMobility(EComponentMobility::Stationary);
	RootComponent = SkyLight;

	//объемные облака
	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("VolumetricCloud"));
	Clouds->SetMobility(EComponentMobility::Static);
	Clouds->SetupAttachment(RootComponent);

	//небесная сфера, содержит шейдером перистые облака, звёзды и луну
	//должна вращаться от времени суток
	Sphaera = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarsDome"));
	Sphaera->SetTranslucentSortPriority(0);
	Sphaera->SetMobility(EComponentMobility::Movable);
	Sphaera->SetupAttachment(RootComponent);


	//пока неясно, юзать его постоянно или наплодить с помощью него статических текстур
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponentCube>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(RootComponent);
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->ShowFlags.DisableAdvancedFeatures();
	SceneCapture->ShowFlags.StaticMeshes = 0;
	SceneCapture->ShowFlags.SkeletalMeshes = 0;
	SceneCapture->ShowFlags.Atmosphere = 1;
	SceneCapture->ShowFlags.Landscape = 0;



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
		MoonLight->SetupAttachment(Sphaera);
	}

}

//==============================================================================================================
// после загрузки, например, редактора
//==============================================================================================================
void AMyrKingdomOfHeaven::PostLoad() {
	Super::PostLoad();

	//переконвертить дату начала действия в тики
	Inception = FDateTime(Year, Month, Day);

	//эфемерис: записать место на земле
	Ephemeris::setLocationOnEarth(Latitude, Longitude);
	InitUnits();

}

//==============================================================================================================
// в редакторе привили свойства = обновить модель
//==============================================================================================================
#if WITH_EDITOR
void AMyrKingdomOfHeaven::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	//SceneCapture->TextureTarget->Resource->TextureRHI;
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	//дата начала действия
	if (MUT(Year) || MUT(Month) || MUT(Day)) ResetEphemerides();

	//новую россыпь карт окружения для времен
	else if(MUT(NumberOfCubemaps))	GenerateCubemaps();

	//изменили время дня - обновить позиции светил
	else if (MUT(TimeOfDay))
	{	ResetEphemerides();
		ChangeTimeOfDay();
	}

	//двигаем позицию воздушной массы
	else if(MUT(AirMassPosition)) PerFrameRoutine(0);

	//изменили демона протагониста - взаимная регистрация протагониста и неба
	else if(MUT(Protagonist)) if(Protagonist) Protagonist->Sky = this;

	//направление ветра
	else if(MUT(WindDirTarget)) VeryRareAdjustWindDir(0);

	//смена позиции на земле
	else if(MUT(Longitude) || MUT(Latitude)) Ephemeris::setLocationOnEarth(Latitude, Longitude);

	//правка размерных коэффициентов - передать новые коэффициенты в шейдера
	else if(MUT(MultCloudPosition) || MUT(MultCloudsVelocity) || MUT(MultWeatherPosition))	InitUnits();
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

	//эфемерис: записать место на земле
	Ephemeris::setLocationOnEarth(Latitude, Longitude);

	//применить коэффициенты
	InitUnits();

}

//==============================================================================================================
// Called every frame
//==============================================================================================================
void AMyrKingdomOfHeaven::Tick(float DeltaTime)
{
	//посчитать новое время
	uint32 InGameDeltaTicks = (DeltaTime * GameTimeDilation) * (float)ETimespan::TicksPerSecond;
	TimeOfDay = TimeOfDay + InGameDeltaTicks;
	RecalcDayFraction();

	//плавный поворот направления ветра к целевому вектору, нах нормировка, точность не нужна
	WindDir.X = StepTo (WindDir.X, WindDirTarget.X, DeltaTime * 0.1f);
	WindDir.Y = StepTo (WindDir.Y, WindDirTarget.Y, DeltaTime * 0.1f);


	//если определена текстура погоды
	if (GlobalWeatherMap)
	{
		//как-то через жопу процесс добычи данных пикселей
		//возможно, ну нах, в начале просто скопировать в массив и радоваться
		auto PD = GlobalWeatherMap->GetRunningPlatformData();
		if (!PD) return;
		FColor* Texels = static_cast<FColor*>((*PD)->Mips[0].BulkData.Lock(LOCK_READ_ONLY));
		if (Texels)
		{
			//для сокращенности
			const uint16 MX = (*PD)->Mips[0].SizeX;
			const uint16 MY = (*PD)->Mips[0].SizeY;

			//координаты на текстуре погоды в диапазоне 0-1
			//вначале в нуле координат плюс сдвиг массы с коэффициентом как в шейдере
			FVector2D WeatherCanvasCoords(WeatherMapPosition[0], WeatherMapPosition[1]);

			//затем если известно положение камеры, берется 2Д позиция зенита над телом
			if (Protagonist)
				WeatherCanvasCoords += FVector2D(Protagonist->GetActorLocation()) * (MultCloudPosition * MultWeatherPosition);

			//отрубается период, остается только смещение
			WeatherCanvasCoords = FVector2D(FMath::Frac(WeatherCanvasCoords.X), FMath::Frac(WeatherCanvasCoords.Y));

			//координаты пикселя на текстуре
			FVector2D Tc = WeatherCanvasCoords * FVector2D((*PD)->Mips[0].SizeX, (*PD)->Mips[0].SizeY);

			//отклонения от границы пикселя = коэффициенты смеси между соседними пикселями для фильтрации
			FVector2D Ta = FVector2D( FMath::Frac(Tc.X), FMath::Frac(Tc.Y) ); 

			//расчёт целочисленных координат для билинейной фильтрации
			uint16 Px0 = Tc.X, Py0 = Tc.Y;
			uint16 Px1 = Tc.X + 1, Py1 = Tc.Y + 1;
			if(Px1 >= (*PD)->Mips[0].SizeX) Px1 = 0;
			if(Py1 >= (*PD)->Mips[0].SizeY) Py1 = 0;

			//билинейно-фильтрованная выборка текущей погоды
			FLinearColor FinalWeather = FMath::BiLerp(
				FLinearColor ( Texels [ (*PD)->Mips[0].SizeX * Py0	+ Px0	]),
				FLinearColor ( Texels [ (*PD)->Mips[0].SizeX * Py0	+ Px1	]),
				FLinearColor ( Texels [ (*PD)->Mips[0].SizeX * Py1	+ Px1	]),
				FLinearColor ( Texels [ (*PD)->Mips[0].SizeX * Py1	+ Px0	]),
				Ta.X, Ta.Y);

			//сразу открыть доступ, чтобы в других потоках не задерживалось
			(*PD)->Mips[0].BulkData.Unlock();

			//получение базовых параметров погоды и тут же расчёт первой производной харки = скорости ветра
			WeatherDerived.UpdateWindSpeed ( 
				WeatherBase.UpdateFromMap_GetWindSpeed ( (FVector4)FinalWeather, 0.3f * DeltaTime ),
				0.5f*DeltaTime );

			//обновить производные параметры погоды, которые не требуют покадровости
			WeatherDerived.UpdateRare(WeatherBase, DeltaTime, -SunDir().Z);

			//ветер, вектор направления и сила
			auto MPC = MakeMPCInst();
			if(MPC) MPC->SetVectorParameterValue(TEXT("WindDir"), FLinearColor(WindDir.X, WindDir.Y, 0.0f, WeatherDerived.WindSpeed));


		}
		//открыть доступ
		else (*PD)->Mips[0].BulkData.Unlock();

	}

	//особенно редко провести корректировку направления ветра, чтоб не забить координаты большими числами
	FrameCunter++;
	if ((FrameCunter & 15) == 0) VeryRareAdjustWindDir(DeltaTime * 16);

	//обновить позиции светил и неба
	ChangeTimeOfDay();

	//всё
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
		{
			//зарегистрировать себя на уровне
			GetMyrGameMode()->Sky = this;

			//добавить прямую ссылку на протагониста
			if (GetMyrGameMode()->Protagonist)
				Protagonist = GetMyrGameMode()->Protagonist;
		}

}



//==============================================================================================================
//создать текстуру из цели рендра
//==============================================================================================================
UTextureCube* AMyrKingdomOfHeaven::CreateTexture(UTextureRenderTargetCube* Source)
{
	//получить локальную точную дату/время, в которой береётся проба неба
	FDateTime CurT(Inception + TimeOfDay);

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
	GetWorld()->GetTimerManager().ClearTimer(Timer);
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
//применить новые коэффициенты размеров облаков
//==============================================================================================================
void AMyrKingdomOfHeaven::InitUnits()
{
	//инстанция глобальных параметров материалов
	//нужна именно локальная переменная, поскольку новые значения отгружаются только в деструкторе
	if (GetWorld())
	{
		auto MPC = MakeMPCInst();
		if (MPC)
		{

			//инициализировать множители перевода координат для облаков и неба: они требуются как минимум в 2 материалах: облаков и сферы, поэтому коллекция
			MPC->SetVectorParameterValue(TEXT("SkyCanvasMultipliers"), FLinearColor(MultCloudPosition, MultCloudsVelocity, MultWeatherPosition, MultCloudPosition * MultCloudsVelocity));

			//отправка нового значения в коллекцию параметров
			MPC->SetVectorParameterValue(TEXT("AirMassPosition"),
				FLinearColor(
					AirMassPosition.X, AirMassPosition.Y,				//натуральная позиция в метрических единицах
					WeatherMapPosition[0], WeatherMapPosition[1]));		//приведенная позиция для полотна облаков и погоды

			//ветер более глобален чем погода и доступ к нему через гейммод
			MPC->SetVectorParameterValue(TEXT("WindDir"), FLinearColor(WindDir.X, WindDir.Y, 0, WeatherDerived.WindSpeed));
		}

	}

	//подготовить стартовые опорные точки для интерполяции
	ResetEphemerides();
}

//==============================================================================================================
//полностью пересчитать небесную механику для определенных даты и времени
//==============================================================================================================
void AMyrKingdomOfHeaven::EvaluateSkyForTime(double TargetDayFraction, FQuat& Sun, FQuat& Moon, FQuat& Sky)
{
	//текущее время в тиках, с точностью до полуночи + часть дня явно выводимая из плавающей доли суток
	FDateTime CurrentDateTime((Inception + FTimespan(TimeOfDay.GetDays(),0,0,0)) + TargetDayFraction*ETimespan::TicksPerDay);

	//для расчёта через библиотеку
	Ephemeris::setLocationOnEarth(Latitude, Longitude);

	//перевести местное время в UTC, ибо юлианский день отсчитывается именно от него
	FDateTime CurrentDateTimeInLondon = CurrentDateTime.GetTicks() - ((int)TimeZone - 12) * ETimespan::TicksPerHour;

	//вообще где-то сие зовут юлианский век, где-то просто Т
	double JuCe = JulianCentury(CurrentDateTimeInLondon);

	//среднее звездное время
	float GreenwitchAngle = GreenwitchMeanSideralAngle(CurrentDateTimeInLondon);
	float LocalAngle = LocalSideralAngleFromGMST(GreenwitchAngle);

	//выплюнуть поворот для небесной сферы
	Sky = FRotator(Latitude, -LocalAngle, 0.0f).Quaternion();

	//расчёт всякой хуйни, зависящей только от времени и места, но не от конкретного светила
	TemporalDerivatives TD = Ephemeris::CalcTemporalDerivatives(JuCe, GreenwitchAngle);
	double H = (TD.ApparentSideralTimeH - TD.GeographicLongitudeH) * 15;

	//расчёт координат солнца
	auto SunCoords = Ephemeris::equatorialCoordinatesForSunAtJC(JuCe, &TD, nullptr);
	auto SunCoordsH = Ephemeris::equatorialToHorizontal(H - SunCoords.ra * 15, SunCoords.dec, Latitude);
	Sun = FRotator(-SunCoordsH.alt, -SunCoordsH.azi, 0).Quaternion();

	//расчёт координат луны
	auto MoonCoords = Ephemeris::equatorialCoordinatesForEarthsMoonAtJC(JuCe, &TD, nullptr);
	auto MoonCoordsH = Ephemeris::equatorialToHorizontal(H - MoonCoords.ra * 15, MoonCoords.dec, Latitude);
	Moon = FRotator(-MoonCoordsH.alt, -MoonCoordsH.azi, 0).Quaternion();

	UE_LOG(LogTemp, Log, TEXT("SKY %s: EvaluateSkyForTime dayfrac=%g"), *GetName(), TargetDayFraction); 

}


//==============================================================================================================
//изменить наклон солнца
//==============================================================================================================
void AMyrKingdomOfHeaven::ChangeTimeOfDay()
{

	//численная оценка времени полудня
	float OldSunZ = SunZ();
	if(OldSunZ > MaxSunZ) { MaxSunZ = OldSunZ, NoonFrac = CurrentDayFraction; }

	//коэффициент смеси угла между точно посчитанными значениями
	float Mix = Ephemerides.MixAmount(CurrentDayFraction);

	//проверка выхода за пределы интерполируемого участка
	if(Mix > 1)
	{	
		//сдвинуть вторую точку на первую, а для второй пересчитать эфемериды
		Ephemerides.ShiftAll();
		EvaluateSkyForTime(Ephemerides.Time0 + DayFractionQuantumForSampling, Ephemerides.F(0), Ephemerides.F(1), Ephemerides.F(2));
		Mix -= 1;
	}
	else if (Ephemerides.DiffInv == 0)	ResetEphemerides();

	UE_LOG(LogTemp, Log, TEXT("SKY %s: ChangeTimeOfDay dayfrac=%g, time0=%g a=%g"), *GetName(), CurrentDayFraction, Ephemerides.Time0, Mix);

	//крутить небесные тела быстро, по интерполяции между посчитанными точками
	SunLight->SetWorldRotation(Ephemerides.Current(0, Mix));
	MoonLight->SetWorldRotation(Ephemerides.Current(1, Mix));
	Sphaera->SetWorldRotation(Ephemerides.Current(2, Mix));

	//численная оценка времени заката и восхода
	if(OldSunZ > 0 && SunZ() < 0) SunsetFrac = CurrentDayFraction; else
	if(OldSunZ < 0 && SunZ() > 0) SunriseFrac = CurrentDayFraction; else
	
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
	if (!MPC) return;

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
		auto Color = SunIntensitiesPerZ->GetUnadjustedLinearColorValue(-SunDir().Z);
		SunLight->SetIntensity(Color.A * ZenithSunIntensity);
		SunLight->SetLightColor(Color);
		MPC->SetVectorParameterValue(TEXT("DirectionToSun"), -SunDir());
		MPC->SetVectorParameterValue(TEXT("SunColor"), Color);

		//таблица цветов луны - вот это нужно, ибо модель плохо справляется
		if (MoonIntensitiesPerZ)
		{
			//цвет луны
			auto MoonColor = MoonIntensitiesPerZ->GetUnadjustedLinearColorValue(-MoonDir().Z);

			//интенсивность сискусственно снижаем, если интенсивность солнца сейчас слишком высокая
			//это костыль, чтобы не мучиться настройками HDR, но пусть пока будет
			//а вот фаза - обязательная штука
			MoonColor.A = FMath::Max(0.0f, MoonColor.A - Color.A) ;
			MoonLight->SetIntensity(MoonColor.A * MoonPhase() * ZenithMoonIntensity);
			MoonLight->SetLightColor(MoonColor);
			MPC->SetVectorParameterValue(TEXT("MoonColor"), MoonColor);
		}
	}

	//кривая цветов зенита неба, добавляется к модели чтобы получить нормальный цвет, особенно ночью
	if (ZenithColorsPerSunZ)
	{
		auto Color = ZenithColorsPerSunZ->GetUnadjustedLinearColorValue(-SunDir().Z);
		MPC->SetVectorParameterValue(TEXT("SkyColorZenith"), Color);
	}

	//цвет земли, неясно, нужно ли
	if (GroundColorsPerSunZ)
	{
		auto Color = GroundColorsPerSunZ->GetUnadjustedLinearColorValue(-SunDir().Z);
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
	float DayFractionInSamples = CurrentDayFraction * SkyAmbients.Maps.Num();

	//целочисленные границы = номера ближайших карт
	iMap1 = FMath::FloorToInt(DayFractionInSamples);
	iMap2 = FMath::CeilToInt(DayFractionInSamples);
	if (iMap1 == SkyAmbients.Maps.Num()) iMap1 = 0;
	if (iMap2 == SkyAmbients.Maps.Num()) iMap2 = 0;
	TransitCoef = DayFractionInSamples - (float)iMap1;
}

//==============================================================================================================
// применить погоду к компонентам тумана
//==============================================================================================================
void AMyrKingdomOfHeaven::ApplyWeather()
{
	//внедрение новых значений куда надо
	ExponentialFog->SetFogDensity(WeatherDerived.FogDensity);
	ExponentialFog->SetFogHeightFalloff(WeatherDerived.FogFalloff);
	ExponentialFog->SetWorldLocation(FVector(0, 0, WeatherDerived.FogHeight));
	ExponentialFog->SecondFogData.FogDensity = WeatherDerived.SecondFogDensity;
	ExponentialFog->VolumetricFogScatteringDistribution = WeatherDerived.VolumetricFogScatteringDistribution;
	ExponentialFog->VolumetricFogExtinctionScale = WeatherDerived.VolumetricFogExtinctionScale;
	//MPC->SetScalarParameterValue(TEXT("CurrentWeatherCirrusAmount"), WeatherNow.CirrusAmount);
	//MPC->SetScalarParameterValue(TEXT("CurrentWeatherCumulusAmount"), WeatherNow.CumulusAmount);

	//выключить если ноль - хз, стоит ли свеч такая оптимизация
	if (ExponentialFog->VolumetricFogExtinctionScale < 0.001) ExponentialFog->SetVolumetricFog(false);
	else ExponentialFog->SetVolumetricFog(true);

}

//направления света светил, в другую сторону - направления на светила, откуда можно извлекать высоту, азимут
FVector AMyrKingdomOfHeaven::SunDir() const { return SunLight->GetDirection(); }
FVector AMyrKingdomOfHeaven::MoonDir() const { return MoonLight->GetDirection(); }


//====================================================================================================
//создать для текущей функции локальную инстанцию коллекции параметров материала
//====================================================================================================
UMaterialParameterCollectionInstance* AMyrKingdomOfHeaven::MakeMPCInst()
{
	return GetWorld()->GetParameterCollectionInstance(EnvMatParam);
}

//====================================================================================================
//вычисление, требующие реального времени - переносятся в тик другого актора
//====================================================================================================
void AMyrKingdomOfHeaven::PerFrameRoutine(float DeltaTime)
{
	//инстанция глобальных параметров материалов
	//нужна именно локальная переменная, поскольку новые значения отгружаются только в деструкторе
	auto MPC = MakeMPCInst();
	if (!MPC) return;

	//приращение перемещения массы воздуха
	double WindPath = DeltaTime;

	//собственно, подвижка массы воздуха, реальная шкала в сантиметрах
	AirMassPosition += WindDir * WindPath;

	//перевести в сдвиг текстуры погоды, вот здесь очень мелкие дельты, оттого дупель
	WindPath = MultCloudPosition * MultCloudsVelocity;

	//специальные координаты для подвижки полотна погоды, сразу примененные множители, чтобы километр был медленный
	WeatherMapPosition[0] = AirMassPosition.X * WindPath;
	WeatherMapPosition[1] = AirMassPosition.Y * WindPath;

	//отправка нового значения в коллекцию параметров
	MPC->SetVectorParameterValue(TEXT("AirMassPosition"), 
		FLinearColor(
			AirMassPosition.X, AirMassPosition.Y,				//натуральная позиция в метрических единицах
			WeatherMapPosition[0], WeatherMapPosition[1]));		//приведенная позиция для полотна облаков и погоды

	//обновить часть слагаемых погоды
	WeatherDerived.UpdateFrequent(WeatherBase, DeltaTime);

	//применить погоду
	ApplyWeather();

}

//====================================================================================================
//изредка менять направление ветра и проверять, далеко ли ушла текстура
//====================================================================================================
void AMyrKingdomOfHeaven::VeryRareAdjustWindDir(float DeltaTime)
{
	if (WindDirTarget.X > 0)
		if (WeatherMapPosition[0] > 1)
			WindDirTarget.X -= 0.3;

	if (WindDirTarget.X < 0)
		if (WeatherMapPosition[0] < 0)
			WindDirTarget.X += 0.3;

	if (WindDirTarget.Y > 0)
		if (WeatherMapPosition[1] > 1)
			WindDirTarget.Y -= 0.3;

	if (WindDirTarget.Y < 0)
		if (WeatherMapPosition[1] < 0)
			WindDirTarget.Y += 0.3;

	WindDirTarget.Normalize();
}
