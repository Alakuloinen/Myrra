// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ephemeris.hpp"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
//#include "../AssetStructures/MyrKingdomOfHeavenWeather.h"

#include "Math/Vector4.h"
#include "MyrKingdomOfHeaven.generated.h"

//###################################################################################################################
// массив текстур окружения для разных частей суток
//###################################################################################################################
USTRUCT(BlueprintType) struct FAmbients
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<class UTextureCube*> Maps;
};

//###################################################################################################################
// часовой пояс - позор, что изначально такой не определено
//###################################################################################################################
UENUM(BlueprintType) enum class ETimeZone : uint8
{
	UTCm12 UMETA(DisplayName = "UTC-12:00"),
	UTCm11 UMETA(DisplayName = "UTC-11:00"),
	UTCm10 UMETA(DisplayName = "UTC-10:00"),
	UTCm9 UMETA(DisplayName = "UTC-9:00"),
	UTCm8 UMETA(DisplayName = "UTC-8:00"),
	UTCm7 UMETA(DisplayName = "UTC-7:00"),
	UTCm6 UMETA(DisplayName = "UTC-6:00"),
	UTCm5 UMETA(DisplayName = "UTC-5:00"),
	UTCm4 UMETA(DisplayName = "UTC-4:00"),
	UTCm3 UMETA(DisplayName = "UTC-3:00"),
	UTCm2 UMETA(DisplayName = "UTC-2:00"),
	UTCm1 UMETA(DisplayName = "UTC-1:00"),
	UTC,
	UTCp1 UMETA(DisplayName = "UTC+1:00"),
	UTCp2 UMETA(DisplayName = "UTC+2:00"),
	UTCp3 UMETA(DisplayName = "UTC+3:00"),
	UTCp4 UMETA(DisplayName = "UTC+4:00"),
	UTCp5 UMETA(DisplayName = "UTC+5:00"),
	UTCp6 UMETA(DisplayName = "UTC+6:00"),
	UTCp7 UMETA(DisplayName = "UTC+7:00"),
	UTCp8 UMETA(DisplayName = "UTC+8:00"),
	UTCp9 UMETA(DisplayName = "UTC+9:00"),
	UTCp10 UMETA(DisplayName = "UTC+10:00"),
	UTCp11 UMETA(DisplayName = "UTC+11:00"),
	UTCp12 UMETA(DisplayName = "UTC+12:00"),
	UTCp13 UMETA(DisplayName = "UTC+13:00"),
	UTCp14 UMETA(DisplayName = "UTC+14:00"),
};

//###################################################################################################################
// сборка для интерполяции поворота неба и светила
//###################################################################################################################
USTRUCT(BlueprintType) struct FCelestRotInterp
{
	GENERATED_USTRUCT_BODY()

	float Time0 = 0.0f;
	float DiffInv = 0.0f;
	FQuat Rotations[6];

	FQuat& B(int C) { return Rotations[C]; }		// минувший сэмпл
	FQuat& F(int C) { return Rotations[C+3]; }		//следующий сэмпл

	//здесь учитывается, что часть дня посреди интерполяции может перегнуться с 0.99 -> 0.01, в этом случае просто
	//прибавляется единица и делается, что часть дня 0.99 -> 1.01, на следующем Time0 перешьется в 0.01, и все снова будет правильно
	float MixAmount(float DayFrac) const { return DayFrac >= Time0 ? (DayFrac - Time0)*DiffInv : (DayFrac + 1 - Time0)*DiffInv; }

	FQuat Current(int Ch, float A) {  return FQuat::Slerp(B(Ch), F(Ch), A);   }
	FQuat Get(int Ch, float DayFra) { return Current(Ch, MixAmount(DayFra)); }

	void AddTime() { Time0 += 1/DiffInv; }
	void LoadTimeAndInterval(float T0, float Interval)  { Time0 = T0; DiffInv = 1/Interval; }
	void Load(int Ch, FQuat Back, FQuat Fore) { B(Ch) = Back; F(Ch) = Fore; }

	void Shift(int Ch) { B(Ch) = F(Ch); }
	void ShiftAll() { Time0 += 1 / DiffInv; if (Time0 > 1) Time0 -= 1; Shift(0); Shift(1); Shift(2); }
	void LoadNextAndShift(int Ch, FQuat Next) { B(Ch) = F(Ch); F(Ch) = Next; }
	void LoadNextAndShift(FQuat q1, FQuat q2, FQuat q3) { LoadNextAndShift(0,q1); LoadNextAndShift(1,q2); LoadNextAndShift(2,q3); Time0 += 1/DiffInv; }
};

//###################################################################################################################
// "геном" погоды, который считывается с карты, пока неясно, что в альфу и насколько привязывать этот базис к
// реальной физике атмосферы, типа давление, влажность и т.п.
//###################################################################################################################
USTRUCT(BlueprintType) struct FWeatherBase
{
	GENERATED_USTRUCT_BODY()
		
	//количество кучевых облаков
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float CumulusAmount = 0.5f;

	//количество перистых облаков
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float CirrusAmount = 0.5f;

	//уровень низкого давления
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Depression = 0.0f;

	//уровень низкого давления
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HuiniaVDoviesoc = 0.0f;


	//представить как полный цвет
	FLinearColor& AsColor() { return *((FLinearColor*)this); }
	FVector3f& AsVector() { return *((FVector3f*)this); }
	FVector3f& AsVector() const { return *((FVector3f*)this); }

	//уровень недождевого тумана
	float FogAmount(float tC) const { return FMath::Max(0.0f, Depression * (1.0f - tC) - 2.0f * CumulusAmount); }

	//общая степень облачности
	float Cloudiness(float Fog) const { return FMath::Min(1.0f, CumulusAmount + CirrusAmount * 0.25f + Fog); }

	//сила ветра для разнообразия берется как взвешенная разность не только давлений, но и непосредственно облаков
	float WindSpeed(const FLinearColor& New) const 
	{	auto D = (AsVector() - FVector3f(New));
		return D | (D * D * FVector3f(2,1,4));
	}

	//уровень дождя
	float RainAmount() const { return FMath::Max(0.0f, (CumulusAmount - 0.8f))*Depression*5.0f; }

	//загрузить данные из карты с плавным приближением, тут же разностно посчитать силу ветра
	float UpdateFromMap_GetWindSpeed (const FLinearColor& NewRaw, float MixAlpha, const FWeatherBase& ConstAdd)
	{	float W = WindSpeed(NewRaw);
		AsVector() = AsVector()*(1 - MixAlpha) + (ConstAdd.AsVector() + FVector3f(NewRaw))* MixAlpha;
		//CumulusAmount         =		FMath::Lerp (CumulusAmount,		ConstAdd.CumulusAmount + NewRaw.X, MixAlpha);
		//CirrusAmount =		FMath::Lerp (CirrusAmount,		ConstAdd.CirrusAmount + NewRaw.Y, MixAlpha);
		//Depression =		FMath::Lerp (Depression,		ConstAdd.Depression + NewRaw.Z, MixAlpha);
		return W;
	}

	//добавить другой
	void Add(FWeatherBase O) { AsVector() = AsVector() + O.AsVector(); }

};


//###################################################################################################################
// сборка вторичных параметров погоды, вычисляемых из базовых, кэшируемых чтобы плавно подводились
//###################################################################################################################
USTRUCT(BlueprintType) struct FWeatherDerived
{
	GENERATED_USTRUCT_BODY()

	//общий коэффициент тумана в отсутствие дождя, по нему считаются остальные
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")	float DryFog = 0.0f;

	//поднятие над землей компонента тумана, уровень границы тумана
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")	float FogHeight = -500;

	//густота тумана
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")	float FogDensity = 0.02f;

	//степень смены тумана в высоте
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")	float FogFalloff = 0.5f;

	//густота тумана
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")	float SecondFogDensity = 0.0f;

	//объемный туман, чем больше, темвыделеннее световой столб и ореол
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")	float VolumetricFogScatteringDistribution = 0.0f;

	//объемный туман, фактически плотность
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")	float VolumetricFogExtinctionScale = 0.0f;

	//Направление и сила ветра
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")	float WindSpeed = 2.0f;

	//сколько дождя
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")	float RainAmount = 0.0f;

	//температура, в диапазоне 0 - 1, для простоты выведения сигналов эмоциям от порогов
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")	float Temperature = 0.5f;

	//вклад ветра с насыщением
	float WindLimit(float MaxSpeed) const { return FMath::Min(1.0f, WindSpeed/MaxSpeed); }

	//обновить с лерпом скорость ветра из внешнего источника
	void UpdateWindSpeed (float N, float A) { WindSpeed = FMath::Lerp(WindSpeed, N, A); }

	//пересчитать силу дождя
	void UpdateRainAmount (float N, float A) { RainAmount = FMath::Lerp(RainAmount, N, A); }

	//пересчитать силу тумана
	void UpdateDryFog (float N, float A) { DryFog = FMath::Lerp(DryFog, N, A); }

	//пересчитать температуру
	void UpdateTemperature (float SunZ, float Cumulus, float Cirrus, float Alpha)
	{	Temperature = FMath::Lerp ( Temperature,				// температура, плавное ведение в сторону целевой
				0.333f *										// приведение к диапазону 0-1
				((2.0f+SunZ)									// высота солнца, первоочередной источник тепла, здесь выходит базовый диапазон 1.0 - 3.0
					- 0.25f * Cumulus * FMath::Min(0.0f,SunZ)	// уровень кучевой облачности, только днём, ночью пофиг, на пике сдвигает диапазон до 0.75 - 2.75
					- 0.04f * Cirrus							// уровень перистых облаков, очень слабо понижает температуру, сдвигая на пике до 0.96 - 2.96
					- 0.23f * RainAmount * (1 + WindLimit(3))	// совместное действие дождя и ветра, выше 5м/с - терминальный, на пике сдвигает (не считая полных облаков) до 0.54 - 2.54
					- 0.25f * WindLimit(20) ),					// изолированное действие ветра, на пике 20м/с сдвигает до 0.75 - 2.75
			Alpha);												// должно быть очень маленьким, ибо температура очень инертна
	}

	//пересчитать частные показатели тумана
	float UpdateFogDensity(float Alpha, float DryFogVal) {	FogDensity = FMath::Lerp( FogDensity, 0.2f * DryFogVal + 0.1f * RainAmount,	Alpha);	}
	float UpdateFogFalloff(float Alpha, float DryFogVal) {	FogFalloff = FMath::Lerp( FogFalloff, 4.0f * DryFogVal + 0.1f * RainAmount, Alpha ); }
	float UpdateFogHeight (float Alpha )				 {	FogHeight  = FMath::Lerp( FogHeight,  -100 * RainAmount, Alpha ); }

	float UpdateFogScatteringDistribution(float Alpha, float DryFogVal)
	{	VolumetricFogScatteringDistribution = FMath::Lerp( VolumetricFogScatteringDistribution, 0.9f * DryFogVal, Alpha);	}

	float FogExtinctionScale(float Alpha, float DryFogVal)
	{	VolumetricFogExtinctionScale = FMath::Lerp( VolumetricFogExtinctionScale, 9.0f * DryFogVal, Alpha);	}


	//процесс обновления тех характеристик, которые не требуют покадровости
	void UpdateRare(FWeatherBase& Source, float DeltaTime, float SunZ)
	{	UpdateRainAmount(Source.RainAmount(), DeltaTime);
		UpdateTemperature(SunZ, Source.CumulusAmount, Source.CirrusAmount, 0.1f * DeltaTime);
		DryFog = Source.FogAmount(Temperature); 
	}

	//обновление параметров, которые налицо каждый кадр, в основном туман
	void UpdateFrequent(FWeatherBase& Source, float DeltaTime)
	{	FogDensity = FMath::Lerp( FogDensity, 0.2f * DryFog + 0.1f * RainAmount, DeltaTime );
		FogFalloff = FMath::Lerp( FogFalloff, 4.0f * DryFog + 0.1f * RainAmount, DeltaTime );
		FogHeight  = FMath::Lerp( FogHeight,  -100 * RainAmount, DeltaTime);
		VolumetricFogScatteringDistribution		= FMath::Lerp( VolumetricFogScatteringDistribution, 0.9f * DryFog, DeltaTime);
		VolumetricFogExtinctionScale			= FMath::Lerp( VolumetricFogExtinctionScale, 9.0f * DryFog, DeltaTime);
	}


};

//###################################################################################################################
//внешняя сборка погоды - пока не нужно, возможно, ждя катсцен, чтобы получить определенную погоду
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType) class MYRRA_API UWeatherAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	//основные параметры
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FWeatherBase Core;

	//время, которое эта погода в среднем держится, во внутренних минутах
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")	float DurationMinutes = 120.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")	float DurationRange = 30.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")	float UpcomingSpeed = 30.0f;

	//вероятности перехода в эту погоду в разные части суток
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probability")	float ChanceMorning = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probability")	float ChanceMidday = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probability")	float ChanceEvening = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probability")	float ChanceMidnight = 1.0f;

	//список возможных переходов для этой погоды
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transitions")	TArray<UWeatherAsset*> OrderedTransitions;

	//вероятности всем скопом
	FLinearColor& Chances() { return *((FLinearColor*)(&ChanceMorning)); }

};


//###################################################################################################################
//сборка всего, что нужно для неба и погоды
//###################################################################################################################
UCLASS(HideCategories = (Mesh, Material, Shape, Physics, Navigation, Collision, Animation))
class MYRRA_API AMyrKingdomOfHeaven : public AActor
{
	GENERATED_BODY()

	//▄▀ - обязательные компоненты - 

	//небесная сфера, ссюда проецируются облака, неподвижна, возможно, двигать вместе с игроком
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UStaticMeshComponent* Sphaera = nullptr;

	//новая фигня, объёмные облака
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UVolumetricCloudComponent* Clouds;

	//солнечный свет, трансформация определяет положение солнца
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UDirectionalLightComponent* SunLight;

	//солнечный свет, трансформация определяет положение солнца
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UDirectionalLightComponent* MoonLight;

	//туман 
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UExponentialHeightFogComponent* ExponentialFog;

	//небо, туман имитирующий атмосферу и рассеивание солнца, трансформация определяет положение солнца
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class USkyAtmosphereComponent* SkyAtmosphere;

	// рассеянный свет от всего купола неба
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class USkyLightComponent* SkyLight;

	//считыватель освещенности для объектов по цвету неба
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class USceneCaptureComponentCube* SceneCapture;

public:

	//▄▀ - общие свойства (настройки, предпросчёты, не меняется в игре)

	//включать и отключать ток времени и смену суток в редакторе
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	bool TickTimeInEditor = false;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly) UWorld* EditorWorld = nullptr;
#endif

	//место действия на земле
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float Latitude = 53.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float Longitude = 37.0f;

	//время старта действия на земле
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	ETimeZone TimeZone = ETimeZone::UTCp3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "31", UIMin = "1", UIMax = "31"))				uint8 Day = 16;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "12", UIMin = "1", UIMax = "12"))				uint8 Month = 06;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "-4712", ClampMax = "4712", UIMin = "-4712", UIMax = "4712"))	int32 Year = 2007;

	//начальная дата действия во внутреннем формате, сюда скидываются отдельные поля свыше
	UPROPERTY()	FDateTime Inception;

	//▄▀ - погода в реальном времени

	//время дня, или нескольких дней, которое реально изменяется в игре, солнце отсчитывается от DateOfPrecalculatedDays + TimeOfDay
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FTimespan TimeOfDay;

	//время суток в виде скаляра (0-1), например полдень=0.5, можно конечно вычислятьь, но как-то убого/длинно это делается
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float CurrentDayFraction;
	//абсолютная позиция точки отсчёта всей массы воздуха
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D AirMassPosition;
	UPROPERTY() double WeatherMapPosition[2];

	//направление ветра (теперь лежит в другом классе, посему доступ через функцию)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D WindDir;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D WindDirTarget;

	//явный модификатор погоды, аддитивный, для перекрытия естественного течения
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FWeatherBase WeatherExplicitAdd;

	//текущие базовые характеристики погоды, рендерятся из карты, с лерпом
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FWeatherBase WeatherBase;

	//производные показатели погоды, рассчитываются однозначно (до лерпа) из базовых 
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FWeatherDerived WeatherDerived;

	//новый способ определения погоды, глобальная бесшовная текстура облачности/ясности, 
	//на ЦПУ берется тексель текущем месте для определения погоды, в ГПУ модифицирует облачность 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	UTexture* GlobalWeatherMap = nullptr;

	//коэффициент, превращающий координаты массы воздуха в сантиметрах в координаты текстуры облаков
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float MultCloudPosition = 0.000003;

	//коэффициент ускорения для высотного ветра, чтобы облака плыли быстрее наземного ветерка
	//возможно, сделать его изменяемым от погоды
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float MultCloudsVelocity = 1000;

	//кэффициент растяжения карты погоды относительно карты облаков
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float MultWeatherPosition = 0.01;

	//диапазон температур в градусах цельсия, мин = долго ночь+ливень+ветер, макс = долго день+ясно
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	uint8 MinTemperatureC = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	uint8 MaxTemperatureC = 30;

	//▄▀ - ресурсы и глубокие настройки

	//ссылка на коллекцию параметров материала для множества материалов типа позиция солнца и т.п.
	//эта общая коллекция для всей игры, и хорошо было бы запрятать ее во что-то глобальное, вроде gameinst, gamemode
	//но эти объекты существуют только при живой игре, и попытка обратиться к ним в редакторе = краш
	//приходится костылём дублировать эту штуку здесь для нужд токм этого класса
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class UMaterialParameterCollection* EnvMatParam;

	//интенсивности звезд, возможно, уже не нужны
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float ZenithSunIntensity = 60;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float ZenithMoonIntensity = 1;

	//коэффициент астрономического времени, например 100 - это сто секунд в реальную секунду 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float GameTimeDilation = 100.0f;

	//скорость рассчёта погоды в стабильном режиме и в режиме смены погод (нафиг здесь, можно константами)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float TickIntervalStable = 0.5f;


	//распределение цветов солнца и луны от времени дня
	//для упрощения, можно использовать физ-модель, но почему-то так быстрее
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UCurveLinearColor* SunIntensitiesPerZ;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UCurveLinearColor* MoonIntensitiesPerZ;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UCurveLinearColor* GroundColorsPerSunZ;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UCurveLinearColor* ZenithColorsPerSunZ;


	//посчитанные времена восхода, заката, полдня в долях суток - нужны для мнемонического определения времени (утро, вечер..)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SunriseFrac = 0.3;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SunsetFrac = 0.7;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float NoonFrac = 0.5;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MaxSunZ = 0.5;

	//кэш опорных точек вращения светил, для быстрой интерполяции
	UPROPERTY() FCelestRotInterp Ephemerides;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float DayFractionQuantumForSampling = 0.01;

	//набор текстур для освещения общего, по времени суток
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	uint8 NumberOfCubemaps = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FAmbients SkyAmbients;

	//протагонист на этой стороне нужен, чтоб брать позицию
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class AMyrDaemon* Protagonist = nullptr;


	//таймер в редакторе используется для генерации карт окружения не отрываясь от процесса
	FTimerHandle Timer;
	uint16 FrameCunter = 0;

public:

	//конструктор
	AMyrKingdomOfHeaven();

	//после загрузки всего актора
	virtual void PostLoad() override;

	//для запуска перестройки структуры дерева после изменения параметров
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** If true, actor is ticked even if TickType==LEVELTICK_ViewportsOnly */
	virtual bool ShouldTickIfViewportsOnly() const override { return TickTimeInEditor; }

#endif

//-----------------------------------------------------
public:	//▄▀ - генерация текстур оружения для данного режима движения солнца, и кривых траекторий солнца
//-----------------------------------------------------


	//создать текстуру из цели рендра
	UFUNCTION(BlueprintCallable) class UTextureCube* CreateTexture(class UTextureRenderTargetCube* Source);

	//сгенерировать все нужные текстуры для эмбиента
	UFUNCTION(BlueprintCallable) void GenerateCubemaps();
	UFUNCTION(BlueprintCallable) void GenerateCubemapsOnTimer();

//-----------------------------------------------------
public:	//▄▀ - общие процедуры
//-----------------------------------------------------

	//посчитать время суток в виде дроби из текущей даты/времени
	void RecalcDayFraction() { CurrentDayFraction = ((Inception + TimeOfDay).GetTicks() % ETimespan::TicksPerDay) / (double)ETimespan::TicksPerDay; }

	//сбросить и заново наполнить интерполяторы позиций светил
	void ResetEphemerides()
	{	Inception = FDateTime(Year, Month, Day);
		RecalcDayFraction();
		Ephemerides.LoadTimeAndInterval(CurrentDayFraction, DayFractionQuantumForSampling);
		EvaluateSkyForTime(CurrentDayFraction + DayFractionQuantumForSampling * 0, Ephemerides.B(0), Ephemerides.B(1), Ephemerides.B(2));
		EvaluateSkyForTime(CurrentDayFraction + DayFractionQuantumForSampling * 1, Ephemerides.F(0), Ephemerides.F(1), Ephemerides.F(2));
	}

	//полностью пересчитать небесную механику для определенных даты и времени
	void EvaluateSkyForTime(double JulianDate, FQuat& Sun, FQuat& Moon, FQuat& Sky);

	//изменить наклон солнца
	void ChangeTimeOfDay();

	//пересчитать яркость и цвет солнца в зависимости от высоты
	void CorelateSunLigntToAngle();

	//для текущего времени получить индексы в массиве карт окружения и коэффициент смеси для них - чтобы далее передать сами карты в SkyLight->SetCubemapBlend
	void SelectCubemapsForCurrentTime(uint8& iMap1, uint8& iMap2, float& TransitCoef);

	//применить новые коэффициенты размеров облаков
	void InitUnits();

	//новый способ получить данные для текущей погоды
	void ApplyWeather();

	//создать для текущей функции локальную инстанцию коллекции параметров материала
	class UMaterialParameterCollectionInstance* MakeMPCInst();

	//вычисление, требующие реального времени
	void PerFrameRoutine(float DeltaTime, float& WetnessToEvaporate);

	//изредка менять направление ветра и проверять, далеко ли ушла текстура
	void VeryRareAdjustWindDir(float DeltaTime);

//-----------------------------------------------------
public:	//▄▀ - возвращуны
//-----------------------------------------------------

	//доступ к глобальным вещам
	UFUNCTION(BlueprintCallable) class UMyrraGameInstance* GetMyrGameInst() { return (UMyrraGameInstance*)GetGameInstance(); }
	UFUNCTION(BlueprintCallable) class AMyrraGameModeBase* GetMyrGameMode() { return (AMyrraGameModeBase*)GetWorld()->GetAuthGameMode(); }

	//направление света
	FVector SunDir() const;
	float SunZ() const { return -SunDir().Z; }
	FVector MoonDir() const;

	//единица - разгар данной части дня, ноль - сейчас какая-то другая часть дня
	float MorningAmount() const { return 4.0 * FMath::Max(0.0f, 0.25f - FMath::Abs(CurrentDayFraction - SunriseFrac - 0.1f)); }
	float NoonAmount() const { return 2.0 * FMath::Max(0.0f, 0.5f - FMath::Abs(CurrentDayFraction - NoonFrac)); }
	float EveningAmount() const { return 3.0 * FMath::Max(0.0f, 0.33f - FMath::Abs(CurrentDayFraction - SunsetFrac + 0.05f)); }
	float NightAmount() const { return FMath::Clamp(5 * SunDir().Z, 0.0f, 1.0f); }

	//определить время дня в вещественном предствлении
	FLinearColor MorningDayEveningNight() const { return FLinearColor(MorningAmount(), NoonAmount(), EveningAmount(), NightAmount()); }

	//униполярная фаза луны, если вектора строго разнонаправлены, это полная луна, нуль это нет луны, растущая/стареющая не различаются ибо нах
	float MoonPhase() const { return 0.5f * FVector::DotProduct(-SunDir(), MoonDir()); }

	//текущий уровень облачности
	float Cloudiness() const { return WeatherBase.Cloudiness(WeatherDerived.DryFog); }

	//интенсивность, сила вклада луны
	float MoonIntensity() const { return (1 - Cloudiness()) * MoonPhase() * NightAmount() * (-MoonDir().Z); }

	//интенсивность, сила вклада луны
	float SunIntensity() const { return (1 - Cloudiness()) * (-SunDir().Z); }


	// астрономия
	//-----------------------------------------------------

	//очень часто требуется для расчётов
	double JulianCentury(FDateTime DT) const { return (DT.GetJulianDay() - 2451545.0) / 36525.0; }

	//странный способ привести угол к одному обороту
	double ReduceAngleTo360(double A) { while (A < 0.f || A > 360.f) A = A - FMath::Sign(A) * 360.f; return A; }

	//среднее звездное время в гринвиче в градусах
	double GreenwitchMeanSideralAngle(FDateTime UTC)
	{	const double NowMinusJ2000 = (UTC.GetTicks() - FDateTime::FromUnixTimestamp(946728000).GetTicks()) / static_cast<double>(ETimespan::TicksPerDay);
		return ReduceAngleTo360 (280.46061837 + 360.98564736629 * NowMinusJ2000);
	}
	//среднее звездное время на текущей долготе в градусах
	double LocalSideralAngleFromGMST(double GMST) {	return ReduceAngleTo360 (GMST + Longitude); }


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//после инициализации компонентов - важно, чтобы до любых BeginPlay на уровне
	virtual void PostInitializeComponents() override;


};
