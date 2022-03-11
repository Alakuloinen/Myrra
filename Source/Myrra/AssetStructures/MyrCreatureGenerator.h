#pragma once

#pragma once
#include "CoreMinimal.h"
#include "../Myrra.h"
#include "Curves/CurveVector.h"		//для высчета по кривым
#include "MyrCreatureGenerator.generated.h"

//###################################################################################################################
// ▄▀ внимание, фабрика по созданию ресурсов этого типа в режакторе реализуется в модуле MyrraEditor/asset_factories.h
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, hidecategories = (Object), meta = (BlueprintSpawnableComponent), Config = Game)
class MYRRA_API UMyrCreatureGenerator : public UObject
{
	GENERATED_BODY()
	friend class AMyrPhyCreature;
	friend class AMyrraCreatureGenActor;

	//набор вариантов существ, которых может порождать этот генератор
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Examples") TArray<TSubclassOf<AMyrPhyCreature>> CreaturesToSpawn;

	//интервал времени между генерациями существ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Examples") float AverageInterval = 10.0f;

	//интервал жизней акторов
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Examples") float SpawnedActorAge = 100.0f;

	//эмоции, которые должно испытывать существо к норе в течение своей жизни
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI") class UCurveLinearColor* EmotionsOfCreatureToMeByTime;

	//система частиц, рендеремая при чутье запаха
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Visual") class UParticleSystem* Smell;

	//канал, на котором "виден" запах существ этого класса
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Visual", config)  uint8 SmellChannel = 0;

	//ЛОДы меша, которым воплощён актор, переход к которым означает начало и конец генерации
	//сделано, чтобы лишний раз не ситать расстояния, а вместо этого использовать и так работающий алгоритм
	//ЛОД 255 означает, что меш скрыт совсем по настройкам максимальной дальности прорисовки
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Examples") uint8 MeshLodToStartGeneration = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Examples") uint8 MeshLodToEndGeneration = 255;

public:


};
