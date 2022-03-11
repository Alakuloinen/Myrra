// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../AssetStructures/MyrKingdomOfHeavenWeather.h"
#include "MyrKingdomOfHeaven.generated.h"

//###################################################################################################################
//###################################################################################################################
USTRUCT(BlueprintType) struct FAmbients
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<class UTextureCube*> Maps;
};


//###################################################################################################################
//сборка всего, что нужно для неба и погоды
//###################################################################################################################
UCLASS(HideCategories = (Mesh, Material, Shape, Physics, Navigation, Collision, Animation)) class MYRRA_API AMyrKingdomOfHeaven : public AActor
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

	//опорная дата действия (время дня надо ставить на нуль), для которой прекалькулируется оборот солнца (для оптимизации)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FDateTime DateOfPrecalculatedDays;

	//ссылка на коллекцию параметров материала для множества материалов типа позиция солнца и т.п.
	//эта общая коллекция для всей игры, и хорошо было бы запрятать ее во что-то глобальное, вроде gameinst, gamemode
	//но эти объекты существуют только при живой игре, и попытка обратиться к ним в редакторе = краш
	//приходится костылём дублировать эту штуку здесь для нужд токм этого класса
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class UMaterialParameterCollection* EnvMatParam;

	//место действия на земле
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float Latitude = 45.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float Longitude = -73.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float ZenithSunIntensity = 60;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float ZenithMoonIntensity = 1;

	//количество ступеней для прекалькуляции позиции солнца над горизонтов в виде кривой кусочно-линейной
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	uint8 SunRotationSamples = 10;

	//коэффициент астрономического времени, например 100 - это сто секунд в реальную секунду 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float GameTimeDilation = 100.0f;

	//скорость рассчёта погоды в стабильном режиме и в режиме смены погод (нафиг здесь, можно константами)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float TickIntervalStable = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float TickIntervalWeatherChange = 0.1f;


	//распределение цветов солнца и луны от времени дня
	//для упрощения, можно использовать физ-модель, но почему-то так быстрее
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UCurveLinearColor* SunIntensitiesPerZ;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UCurveLinearColor* MoonIntensitiesPerZ;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UCurveLinearColor* GroundColorsPerSunZ;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UCurveLinearColor* ZenithColorsPerSunZ;

	//это можно создавать, а можно и менять - координаты радиус вектора на солнце от времени суток 
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UCurveVector* SunPositionsPerDay;

	//посчитанные времена восхода, заката, полдня в долях суток - нужны для мнемонического определения времени (утро, вечер..)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SunriseFrac;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SunsetFrac;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float NoonFrac;

	//набор текстур для освещения общего, по времени суток
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	uint8 NumberOfCubemaps = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FAmbients SkyAmbients;



	//▄▀ - текущие параметры, меняются в реальном времени

	//время дня, или нескольких дней, которое реально изменяется в игре, солнце отсчитывается от DateOfPrecalculatedDays + TimeOfDay
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FTimespan TimeOfDay;

	//время суток в виде скаляра (0-1), например полдень=0.5, можно конечно вычислятьь, но как-то убого/длинно это делается
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float CurrentDayFraction;

	//погоды, текущая, начинающаяся, степень перехода, длительность
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	UMyrKingdomOfHeavenWeather* CurrentWeather = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	UMyrKingdomOfHeavenWeather* UpcomingWeather = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float WeatherTransition = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float WeatherRemainingTime = 0;		// сколько времени осталось до начала смены погоды (внимание, измеряется в реальных секундах!)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float RainAmount = 0;				// текущая сила дождя
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FLinearColor CurrentWeatherEmotion;	// текущее настроение, обусловленное погодой



	FTimerHandle Timer;

public:

	//конструктор
	AMyrKingdomOfHeaven();

	//после загрузки всего актора
	virtual void PostLoad() override;

	//для запуска перестройки структуры дерева после изменения параметров
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//-----------------------------------------------------
public:	//▄▀ - генерация текстур оружения для данного режима движения солнца, и кривых траекторий солнца
//-----------------------------------------------------

	//вычислить кривую положений солнца для определенного дня
	UFUNCTION(BlueprintCallable) void PrecalculateSunCycle();

	//создать текстуру из цели рендра
	UFUNCTION(BlueprintCallable) class UTextureCube* CreateTexture(class UTextureRenderTargetCube* Source);

	//сгенерировать все нужные текстуры для эмбиента
	UFUNCTION(BlueprintCallable) void GenerateCubemaps();
	UFUNCTION(BlueprintCallable) void GenerateCubemapsOnTimer();

//-----------------------------------------------------
public:	//▄▀ - общие процедуры
//-----------------------------------------------------

	//изменить наклон солнца
	void ChangeTimeOfDay();

	//пересчитать яркость и цвет солнца в зависимости от высоты
	void CorelateSunLigntToAngle();

	//для текущего времени получить индексы в массиве карт окружения и коэффициент смеси для них - чтобы далее передать сами карты в SkyLight->SetCubemapBlend
	void SelectCubemapsForCurrentTime(uint8& iMap1, uint8& iMap2, float& TransitCoef);

	//определить критерии для смены погоды и подготовить смену (выбрать новую погоду)
	void BewareWeathersChange(float DeltaTime);

	//запустить текущую погода на новый отрезок времени (в ассете длительность в игровых минутах, поэтому *60 в игровые секунды, затем в реальные секунды
	void ReloadWeather(UMyrKingdomOfHeavenWeather* W, float TimeVariation) { WeatherRemainingTime = (W->DurationMinutes + TimeVariation * W->DurationRange) * 60 / GameTimeDilation; }

	//применить/изменить погоду (согласно свежеизмененным CurrentWeather, UpcomingWeather, WeatherTransition)
	void ApplyWeathers(UMyrKingdomOfHeavenWeather* W1, UMyrKingdomOfHeavenWeather* W2 = nullptr, float A = 0.0f);

	//создать для текущей функции локальную инстанцию коллекции параметров материала
	class UMaterialParameterCollectionInstance* MakeMPCInst();

//-----------------------------------------------------
public:	//▄▀ - возвращуны
//-----------------------------------------------------

	//доступ к глобальным вещам
	UFUNCTION(BlueprintCallable) class UMyrraGameInstance* GetMyrGameInst() { return (UMyrraGameInstance*)GetGameInstance(); }
	UFUNCTION(BlueprintCallable) class AMyrraGameModeBase* GetMyrGameMode() { return (AMyrraGameModeBase*)GetWorld()->GetAuthGameMode(); }

	//направление ветра (теперь лежит в другом классе, посему доступ через функцию)
	FVector2D& WindDir();

	//определить время дня в вещественном предствлении
	FLinearColor MorningDayEveningNight();



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//после инициализации компонентов - важно, чтобы до любых BeginPlay на уровне
	virtual void PostInitializeComponents() override;

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//стырено @verycollective verycollective/Sundial - расчёт позиции солнца
	/////////////////////////////////////////////////////////////////////////////////////////////////////

public:

	/** Get the sun's position data based on position, date and time */
	UFUNCTION(BlueprintCallable) static void GetSunPosition(float Latitudo, float Longitudo, const FDateTime date, float& altitude, float& azimuth);

	UFUNCTION(BlueprintCallable) static void GetSunRiseSet(float Latitudo, float Longitudo, const FDateTime date, float& Rise, float& Set, float &Noon);


};
