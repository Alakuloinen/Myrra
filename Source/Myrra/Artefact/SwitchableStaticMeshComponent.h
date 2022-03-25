// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "SwitchableStaticMeshComponent.generated.h"

//###################################################################################################################
//для компонента триггера
//###################################################################################################################
USTRUCT(BlueprintType) struct FMeshVariant
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticMesh* Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Chance = 255;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 ChanceToBeOn = 255;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TWeakObjectPtr<USoundBase> SoundWhenSwitched;
};

//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//позор, что такого нет стандартного
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYRRA_API USwitchableStaticMeshComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	// карта возможных обличий, число - это вероятность наступления данного обличия в случае
	// набирается в редакторе, можно создавать подклассы с предопределенными мешами
	// вообще это бы вынести в какой-нибудь одиночный ассет
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FMeshVariant> Variants;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Current = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool TurnedOn = false;

	//если труЪ, то перетасовываетя только один раз на старте новой игры, а затем сохраняется и однозначно восстанавливается из сохраненной
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool OnceAndThenSaved = false;

	//для рандомизации примитивдэйта - там чуть темнее, или координаты сдвинуть...
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FFloatRange> PrimitiveDataRanges;

public:

	//при запуске игры
	virtual void BeginPlay() override;


	//явным образом включить облик под номером нум
	UFUNCTION(BlueprintCallable) void SetMesh(int Num);

	//включить произвольный из доступных - пока без весов, 
	UFUNCTION(BlueprintCallable) void SetRandomMesh();

	//включить (не меняя меша, а воздействуя параметрами и на окружение)
	UFUNCTION(BlueprintCallable) void TurnOnOrOff(bool On);

public:

	//состояние абстрактной включекнности/выключенности (результат зависит от сути объекта и окружения)
	UFUNCTION(BlueprintCallable) bool IsOn() const { return TurnedOn; }

	//номер текущего меша
	UFUNCTION(BlueprintCallable) uint8 GetCurrent() const { return Current==-1 ? 0 : Current; }

	//оброботчики судьбоносных событий
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
		if (PropertyName == GET_MEMBER_NAME_CHECKED(USwitchableStaticMeshComponent, Current))
		{	if (Current >= Variants.Num()) Current = Variants.Num() - 1;
			else SetMesh(Current);
		}else
		if (PropertyName == GET_MEMBER_NAME_CHECKED(USwitchableStaticMeshComponent, TurnedOn))
			TurnOnOrOff(TurnedOn);
		Super::PostEditChangeProperty(PropertyChangedEvent);
	}
#endif

};
