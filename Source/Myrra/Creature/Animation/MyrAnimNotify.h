// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Myrra.h"
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "MyrAnimNotify.generated.h"

//смысл закладки в анимации
UENUM(BlueprintType) enum class EMyrNotifyCause : uint8
{
	RelaxStart,
	RelaxStop,

	StepFrontLeft,
	StepFrontRight,
	StepBackLeft,
	StepBackRight,
	StepBackBoth,
	StepFrontBoth,

	RaiseFrontLeft,
	RaiseFrontRight,
	RaiseBackLeft,
	RaiseBackRight,
	RaiseBackBoth,
	RaiseFrontBoth,

	AttackBegin,		// самое начало ролика
	AttackGetReady,		// момент перед началом опасной фазы 
	AttackGiveUp,		// атака очень медленно идёт вспять и достигает этой точки выдыхания
	AttackComplete,		// момент конца опасной фазы
	AttackEnd,			// конец ролика

	NewPhase,			// сменить фазу самодействия
	SelfActionEnd		// конец ролика
};

//смысл закладки-интервала в анимации
UENUM(BlueprintType) enum class EMyrNotifyStateCause : uint8
{
	AttackDangerous,	// период, когда атака вредить (нужно ли вот так?)
	PelvisVertical,		// зафиксировать таз вертикально, если  позволяет поза
	ThoraxVertical		// зафиксировать плечи вертикально, если позволяет поза
};



//###################################################################################################################
// для обратной связи - единичная закладка в анимационный ролик и как на нее реагировать
// MyrAnimInst_Notify.cpp
//###################################################################################################################
UCLASS(BlueprintType) class MYRRA_API UMyrAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

	//вызывается, когда анимация достигает закладки
	virtual void Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Anim) override;

	public:

	//переменные можно заполнять после добавления закладки на шкалу времени ролика
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	EMyrNotifyCause Cause;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:

	//говорят, это позволяет отображать пользовательское имя закладки
	virtual FString GetNotifyName_Implementation() const override;
};

//###################################################################################################################
//для обратной связи - промежуток времени в анимационном ролике и как реагировать в течение этого промежутка
//###################################################################################################################
UCLASS(BlueprintType) class MYRRA_API UMyrAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	//переменные можно заполнять после добавления закладки на шкалу времени ролика
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	EMyrNotifyStateCause Cause;

	//стандартные
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

};