// Fill out your copyright notice in the Description page of Project Settings.


#include "Artefact/MyrSmellEmitter.h"
#include "Control/MyrDaemon.h"
#include "MyrraGameModeBase.h"
#include "MyrraGameInstance.h"

//==============================================================================================================
//правильный конструктор
//==============================================================================================================
UMyrSmellEmitter::UMyrSmellEmitter(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
	ParticleSize = FFloatRange(50.0f, 50.0f);
	ParticleAmount = FFloatRange(0.1f, 1.0f);		//количество частиц в основном определяет силу запаха
	ParticleVelocity = FFloatRange(10.0f, 10.0f);
	ParticleBrightness = FFloatRange(0.1f, 10.0f);	//яркость также хорошо отображает силу запаха
}

//==============================================================================================================
// при появлении в игре
//==============================================================================================================
void UMyrSmellEmitter::BeginPlay()
{
	//типая процедура ручной подвязки к делегату
	Super::BeginPlay();
	if (GetWorld())
	{	auto GM = GetWorld()->GetAuthGameMode<AMyrraGameModeBase>();
		if (GM)
		{	if (GM->Protagonist)
			{	GM->Protagonist->SwitchToSmellChannel.AddUObject(this, &UMyrSmellEmitter::OnSwitchToSmellChannel);
				GM->Protagonist->UpdateSmellVision();
			}
		}
	}

	//первый раз залить параметры
	UpdateParams(1);
}

//==============================================================================================================
//обновить параметры генерации частиц из собственных переменных
//==============================================================================================================
void UMyrSmellEmitter::UpdateParams(const float alpha)
{
	//отсюда брать цвет
	if (!GetOwner()) return;
	if (!GetWorld()) return;
	auto GI = (UMyrraGameInstance*)GetOwner()->GetGameInstance();

	//цвет и яркость
	FLinearColor Co = GI ? GI->ColorsOfSmellChannels[SmellChannel] : FLinearColor(1.0f, FMath::RandRange(0.0f, 0.15f), FMath::RandRange(0.0f, 0.2f));
	SetColorParameter(TEXT("Color"), Co * LerpR(ParticleBrightness, alpha));

	//размер клуба
	SetFloatParameter(TEXT("SmellSize"), LerpR(ParticleSize, alpha));


	//густота в количестве частиц
	SetFloatParameter(TEXT("SmellAmount"), LerpR(ParticleAmount, alpha));

	//скорость = определяет высоту конуса
	SetFloatParameter(TEXT("SmellVelocity"), LerpR(ParticleVelocity, alpha));

}


//==============================================================================================================
//в ответ на событие переключение канала
//==============================================================================================================
void UMyrSmellEmitter::OnSwitchToSmellChannel(int SmellChann)
{
	if (SmellChann == SmellChannel)
		SetVisibility(true);
	else SetVisibility(false);
}
