#pragma once
#include "CoreMinimal.h"
#include "../Myrra.h"
#include "Curves/CurveVector.h"		//для высчета по кривым
#include "MyrKingdomOfHeavenWeather.generated.h"

UENUM(BlueprintType) enum class EWeatherLocalContext : uint8
{
	Normal
};


//###################################################################################################################
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, hidecategories = (Object), meta = (BlueprintSpawnableComponent), Config = Game)
class MYRRA_API UMyrKingdomOfHeavenWeather : public UObject
{
	GENERATED_BODY()

public: 

	//время, которое эта погода в среднем держится, во внутренних минутах
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")	float DurationMinutes = 120.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")	float DurationRange = 30.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")	float UpcomingSpeed = 30.0f;

	//вероятности перехода в эту погоду в разные части суток
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probability")	float ChanceMorning = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probability")	float ChanceMidday = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probability")	float ChanceEvening = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probability")	float ChanceMidnight = 1.0f;


	//поднятие над землей компонента тумана, уровень высоты тумана, изначально он опущен, чтобы закаты не выцветали
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")	float FogZCoordinate = -500;

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

	//количество кучевых облаков
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")	float CumulusAmount = 0.5f;

	//количество перистых облаков
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")	float CirrusAmount = 0.5f;

	//Направление и сила ветра
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")	FVector2D Wind;

	//сколько дождя
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")	float RainAmount = 0.0f;

	//список возможных переходов для этой погоды
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transitions")	TArray<UMyrKingdomOfHeavenWeather*> OrderedTransitions;

	//эмоция, вызываемая этой погодой, модифицирует настроение героя, влияет на выбор фоновой музыки
	//очень небольшой вклад, так как разные герои по разному воспринимают погоду - в целом, скорее отношение "человека"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion")	FLinearColor EmotionColor;

};

