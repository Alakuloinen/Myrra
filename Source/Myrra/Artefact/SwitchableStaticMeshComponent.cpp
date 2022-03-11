// Fill out your copyright notice in the Description page of Project Settings.


#include "Artefact/SwitchableStaticMeshComponent.h"
#include "Artefact/MyrSmellEmitter.h"
#include "Components/PointLightComponent.h"				//чтобы вместе с вариантом включать и выключать свет
#include "MyrTriggerComponent.h"						//чтобы вместе с вариантом менять фокус камеры

//==============================================================================================================
//◘при запуске игры
//==============================================================================================================
void USwitchableStaticMeshComponent::BeginPlay()
{
	Super::BeginPlay();
	//начальная перетасовка
	//здесь нужно как-то различать тру рандомные и рандомные в начале игры и потом сохраняемые
	//возможно, сохранять только артефакты, а строения с этими объектами оставить магическим образом изменчивыми
	SetRandomMesh();
}

//==============================================================================================================
//явным образом включить облик под номером нум
//==============================================================================================================
void USwitchableStaticMeshComponent::SetMesh(int Num)
{
	if (Num >= 0 && Num < Variants.Num())
	{ 
		//собственно, установить новый меш
		if(Variants[Num].Mesh != GetStaticMesh()) SetStaticMesh(Variants[Num].Mesh);
		Current = Num;

		//рандомизировать данные для вариации цвета, прозрачности, сдвига координат
		for (int i = 0; i < PrimitiveDataRanges.Num(); i++)
		{	float NewPD = FMath::RandRange(PrimitiveDataRanges[i].GetLowerBoundValue(), PrimitiveDataRanges[i].GetUpperBoundValue());
			SetCustomPrimitiveDataFloat(i, NewPD);
		}

		//если шанс представлен не числами 0 и 255 -  растасовка шанса включить свет
		bool On = Variants[Num].ChanceToBeOn==255 ? true :
			     (Variants[Num].ChanceToBeOn==0   ? false : ((uint8)(FMath::Rand()) <= Variants[Num].ChanceToBeOn));
		TurnOnOrOff(On);
	}
}

//==============================================================================================================
//включить (не меняя меша, а воздействуя параметрами и на окружение)
//==============================================================================================================
void USwitchableStaticMeshComponent::TurnOnOrOff(bool On)
{

	//пока неясно, стоит ли делать такое универсальным или только для некоторых типов объекта и окружения
	SetCustomPrimitiveDataFloat(0, On ? 1.0f : 0.0f);

	//при комбинации с источником света жестко задано, что канал 0 примитив-данных - это горячесть материала в согласовании со светом
	if (auto L = Cast<UPointLightComponent>(GetAttachParent()))
	{	L->SetVisibility(On);
	}

	//если эта хрень прикреплена к источнику запаха менно в таком порядке
	else if (auto S = Cast<UMyrSmellEmitter>(GetAttachParent()))
	{	S->UpdateParams(On ? 1.0f : 0.0f);
	}

	//внешний запоминатель
	TurnedOn = On;
	UE_LOG(LogTemp, Log, TEXT("%s: Turned to %d"), *GetName(), On);
}


//==============================================================================================================
//включить произвольный из доступных - пока без весов, 
//==============================================================================================================
UFUNCTION(BlueprintCallable) void USwitchableStaticMeshComponent::SetRandomMesh()
{
	//взвешенное случайное - геморная хня через новый массив 
	TArray<uint16> WeightLadder;
	WeightLadder.SetNum(Variants.Num());
	int Sum = 0;
	for (int i = 0; i < Variants.Num(); i++)
	{	Sum += Variants[i].Chance;
		WeightLadder[i] = Sum;
	}
	int R = FMath::RandRange(0, Sum);
	for (int i = 0; i < WeightLadder.Num(); i++)
	{	if (R < WeightLadder[i])
		{	SetMesh(i);
			return;
		}
	}
}
