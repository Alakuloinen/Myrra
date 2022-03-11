// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyrDendroAnimInst.generated.h"

//###################################################################################################################
//###################################################################################################################
USTRUCT(BlueprintType) struct FDENDROFORM
{
	GENERATED_USTRUCT_BODY()

	//уклон вверх
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Shape, meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0"))
		float RaiseUpDown = 0;

	//изгиб вверх
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Shape, meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0"))
		float CurlUpDown = 0;

	//распушение по сторонам
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Shape, meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0"))
		float SpreadCondense = 0;

	//уклон влево или вправо
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Shape, meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0"))
		float SlopeRightLeft = 0;

};



//###################################################################################################################
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType) class MYRRA_API UMyrDendroAnimInst : public UAnimInstance
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Shape)
		FDENDROFORM DendroForm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Shape)
		class UPoseAsset *BasePoses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Shape)
		class UAnimSequence *RandomPoses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Shape)
		 float CurrentRandomPose;

	//----------------------------------------------------------------
	// вспомогательные функции для вызова из блюпринтов
	//----------------------------------------------------------------

	//разбить биполярное значение на два положительных униполяра
	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe, CompactNodeTitle = "+ -"))
		void split_sign(float in, float& outp, float &outn) const	{	if (in > 0) { outp = in; outn = 0.0f; }		else { outp = 0.0f; outn = -in; }	}

//---------------------------------------------------------------------------------------
public:	// переопределения родительских методов - обработчики внешних событий
//---------------------------------------------------------------------------------------

	//вызывается при инициализации - здесь начально присваиваются переменные
	virtual void NativeInitializeAnimation() override;

	//собственно тик функция анимации
	virtual void NativeUpdateAnimation(float DeltaTimeX) override;

};
