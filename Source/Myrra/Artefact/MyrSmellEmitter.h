// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "Particles/ParticleSystemComponent.h"
#include "MyrSmellEmitter.generated.h"

//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//модифицированный источник частиц, сам откликающися на включение видения запахов
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYRRA_API UMyrSmellEmitter : public UParticleSystemComponent
{
	GENERATED_BODY()
	
public:

	//последнее время, когда объект потревожили - для отсчёта задержки
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 SmellChannel = 0;

	//яркость или плотноть дыма, домножает цвет в хдр
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFloatRange ParticleBrightness;

	//размер одного клуба запаха
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFloatRange ParticleSize;

	//размер одного клуба запаха
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFloatRange ParticleAmount;

	//время жизни одного клуба запаха
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFloatRange ParticleVelocity;


public:

	//правильный конструктор
	UMyrSmellEmitter(const FObjectInitializer& ObjectInitializer);

	//здесь привязка, потому что нужен загруженный мир, чтобы найти протагониста и его второй конец
	virtual void BeginPlay() override;

	//в ответ на событие переключение канала
	UFUNCTION() void OnSwitchToSmellChannel(int SmellChan);

	//обновить параметры генерации частиц из собственных переменных (указывается степень включённости запаха)
	UFUNCTION() void UpdateParams(const float alpha = 1.0f);

	//подправить в редакторе и сразу посмотреть
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{	UpdateParams();
		Super::PostEditChangeProperty(PropertyChangedEvent);
	}
#endif

	//позор, что такого нет, и приходтся ручками заворачивать эту длинючую строку
	static float LerpR(FFloatRange& R, float A) { return FMath::Lerp(R.GetLowerBoundValue(), R.GetUpperBoundValue(), A); }

};
