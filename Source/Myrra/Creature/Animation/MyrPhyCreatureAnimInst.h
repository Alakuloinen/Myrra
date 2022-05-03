// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "Animation/AnimInstance.h"
#include "MyrPhyCreatureAnimInst.generated.h"

//###################################################################################################################
// сборка производных параметров анимации для целого шарика
// MyrAnimInst_BoySection.cpp
//###################################################################################################################
USTRUCT(BlueprintType) struct FAGirdle
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float GainDirect;		// проекция курса на позвоночник
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float GainLateral;		// проекция курса на поперёк
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float Stands;			// устойчивость
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float LegsSpread;		// расставить/сомкнуть ноги
};

/**
 * 
 */
UCLASS()
class MYRRA_API UMyrPhyCreatureAnimInst : public UAnimInstance
{
	GENERATED_BODY()

	//свойства, видимые в анимационном блыпринте
public:

	//глобальное состояние, полная сборка
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Main)	class AMyrPhyCreature* Creature;

	//глобальное состояние, полная сборка
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mode)	EBehaveState CurrentState;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mode)	float StateTime = 0.0f;

	//скорость обмена веществ - определяет play rate для idle-анимаций
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mode)	float Metabolism = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mode)	float EmotionRage = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mode)	float EmotionFear = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mode)	float EmotionPower = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mode)	float EmotionAmount = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mode)	float Lighting = 0.0f;

	//все хозяйство, связанное с атаками (действиями от себя на цель)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		class UAimOffsetBlendSpace* AttackAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		class UBlendSpace* AttackPreciseAnimation;	//новое, экспериментальное
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		UAnimSequenceBase* AttackCurvesAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float UpperBodyMaskWeight = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		uint8 CurrentAttack = 255;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		EActionPhase CurrentAttackPhase;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float CurrentAttackPlayRate = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float AttackDirRawLeftRight = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float AttackDirRawUpDown = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float AttackDirAdvLeftRight = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float AttackDirAdvUpDown = 0.0f;


	//все хозяйство, связанное с самодействиями (самовызываемыми действиями на себя)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		UAnimSequenceBase* SelfAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		uint8 CurrentSelfAction = 255;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float SelfActionPlayRate = 0.0f;

	//все хозяйство, связанное с действиями отдыха (на себя, но вызываемыми игроком и с четким набором фаз)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		UAnimSequenceBase* RelaxMotion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		uint8 CurrentRelaxAction = 255;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float RelaxActionPlayRate = 0.0f;

	//звук, который сейчас произносится, для выбора позы рта для lipsync
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	EPhoneme CurrentSpelledSound = EPhoneme::S0;

	//сборки для передней и задней частей тела (степень скорости по Х, по У (Aim Offset координаты), множитель частоты)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	FAGirdle Thorax;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	FAGirdle Pelvis;


	//если получится контролировать физику через анимацию - этим контролировать выгиб спины и головы при движении
	//через выбор позы (анимации), специфичной для существа - далее из анимации будут подстроены сдвиги моторов,
	//далее по мере деформации каркаса по нему будет ориентирован остальной скелет
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Machine)	float MachineSpineLeftOrRight = 0.0f;

	//уклон всего тела вверх или вниз
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Machine)	float WholeBodyUpDown = 0.0f;


	//поворот и уклон тела (углы, радианы)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float SpineLeftOrRight = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float SpineUpOrDown = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float SpineTwist = 0.5f;

	//поворот, уклон головы и атаки (углы, радианы)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float HeadUpOrDown = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float HeadLeftOrRight = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float HeadTwist = 0.5f;

	//направление глаз (отсчитывается как направление атаки/взгляда в системе головы
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Machine)		float EyesLeftOrRight = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Machine)		float EyesUpOrDown = 0.0f;

	//поворот, уклон хвоста (углы, радианы)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float TailUpOrDown = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float TailLeftOrRight = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float TailTipUpOrDown = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float TailTipLeftOrRight = 0.0f;

	//выворот ног и рук взад-вниз
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float LegsRotation = 0.0f;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float ArmsRotation = 0.0f;

	//смещения конечностей относительно нормальных значений - без структур, так как в блюпринтах это косно
	//по умолчанию на середине интервала 0-1, так как часть их может реализовываться осбю времени, которая нормируется
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float RArmUpDown = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float RArmFrontBack = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float RArmLeftRight = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float RArmNormalFrontBack = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float LArmUpDown = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float LArmFrontBack = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float LArmLeftRight = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float LArmNormalFrontBack = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float RLegUpDown = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float RLegFrontBack = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float RLegLeftRight = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float RLegNormalFrontBack = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float LLegUpDown = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float LLegFrontBack = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float LLegLeftRight = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Limbs)	float LLegNormalFrontBack = 0.5;

//стандартные функции
public: 

	//конструктор
	UMyrPhyCreatureAnimInst() { /*Proxy = FMyrAnimProxy(this);*/ }

	//инициализация 
	virtual void NativeInitializeAnimation() override;

	//пересчёт (здесь в основном всё в лоб)
	virtual void NativeUpdateAnimation(float DeltaTimeX) override;

	//Executed when begin play is called on the owning component
	virtual void NativeBeginPlay() override;

public:

	//обновить анимионные сборки по отдельному поясу конечностей
	void UpdateGirdle(FAGirdle& AnimGirdle, class UMyrGirdle* PhyGirdle, float* LimbChunk, bool AllowMoreCalc);

	//рассчитать для данного кадра скорость анимации атаки, если таковая имеется
//	void UpdateAttackPlayRates(float strikeRate);

	//получить букет трансформаций конечности в правильном формате из физ-модели существа
	void SetLimbTransform(const FLimb& Limb, const FLimb& HubLimb, const FLimb& SpineLimb, float* LimbChunk, const float WheelRadius, FAGirdle& AGirdle, const float Handness);

	//получить численный параметр угла отклонения между сегментами тела по заданным осям
	void DriveBodyRotation(float& Dest, bool En, ELimb LLo, EMyrRelAxis LoA, ELimb LHi, EMyrRelAxis HiA, bool Asin = true);
};
