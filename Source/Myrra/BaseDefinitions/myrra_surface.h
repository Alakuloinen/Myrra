#pragma once
#include "CoreMinimal.h"
#include "myrra_surface.generated.h"

//###################################################################################################################
//специфические типы поверхностей (имена в редакторе должны соответствовать)
//###################################################################################################################
UENUM(BlueprintType) enum class EMyrSurface : uint8
{
	Surface_0	/*		= (uint8)EPhysicalSurface::SurfaceType_Default*/,
	Surface_WoodRaw = (uint8)EPhysicalSurface::SurfaceType1,
	Surface_Fabric = (uint8)EPhysicalSurface::SurfaceType2,
	Surface_Flesh = (uint8)EPhysicalSurface::SurfaceType3,
	Surface_Ground = (uint8)EPhysicalSurface::SurfaceType4,
	Surface_HeavyMetal = (uint8)EPhysicalSurface::SurfaceType5,
	Surface_SpeedMetal = (uint8)EPhysicalSurface::SurfaceType6,
	Surface_Sand = (uint8)EPhysicalSurface::SurfaceType7,
	Surface_Wood = (uint8)EPhysicalSurface::SurfaceType8,
	Surface_Grass = (uint8)EPhysicalSurface::SurfaceType9,
	Surface_DeepGrass = (uint8)EPhysicalSurface::SurfaceType10,
	Surface_Moss = (uint8)EPhysicalSurface::SurfaceType11,
	Surface_Dirt = (uint8)EPhysicalSurface::SurfaceType12,
	Surface_Gravel = (uint8)EPhysicalSurface::SurfaceType13,
	Surface_Asphalt = (uint8)EPhysicalSurface::SurfaceType14,
	Surface_HeavyStone = (uint8)EPhysicalSurface::SurfaceType15,
	Surface_Glass = (uint8)EPhysicalSurface::SurfaceType16,
	Surface_Water = (uint8)EPhysicalSurface::SurfaceType17,
	Surface_Sting = (uint8)EPhysicalSurface::SurfaceType18,
	Surface_Burn = (uint8)EPhysicalSurface::SurfaceType19
};

//###################################################################################################################
//параметры поверхности, влияющие на действия, совершаемые на ней
//###################################################################################################################
USTRUCT(BlueprintType) struct FSurfaceInfluence
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float StealthFactor = 1.0;				// насколько, стоя на этой поверхности, удобно прятаться
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MotionVelocityFactor = 1.0;		// насколько эта поверхность помогает двигаться или сбавляет скорость
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HealthDamage = 0.0;				// некоторые поверхности болезненны, отнимают здоровье, а некоторые прибавляют
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Amortisation = 0.0;				// амортизация - отнятие урона (1.0 - сберегает единичную полную дозу здоровья)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UNiagaraSystem* ImpactBurstHit;	// всплеск частиц при наступании на поверхность
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UNiagaraSystem* ImpactBurstRaise;	// всплеск частиц при отрыве от поверхности
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float SoftnessForFootprint = 0.0;		// насколько мягка поверхность для того, чтоб оставлять вдавленные следы
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class USoundBase* SoundAtImpact;		// звук/сборка звуков, проигрываемая в момент удара по поверхности
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class USoundBase* SoundAtRaise;			// звук/сборка звуков, проигрываемая в момент отрыва от поверхности
};

//типы механического взаимодействия тела с поверхностью
//помогают выбрать звук, точнее ветвь в SoundCue
UENUM() enum class EBodyImpact : uint8
{
	StepHit,	// шаг ногой, жахнули-забыли, поверхность определяется отдельно из trace
	StepRaise,	// шаг ногой, жахнули-забыли, поверхность определяется отдельно из trace
	Bump,		// задели верхней частью тела (преверяется Z нормали и скорость) - жахнули-забыли
	Slide		// скольжение по поверхности - продолжительный, зацикленный
};