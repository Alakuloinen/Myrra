#pragma once
#include "CoreMinimal.h"
#include "myrra_roleparams.generated.h"

UENUM(BlueprintType) enum class ELevelShift : uint8
{
	Exactly = 0,				// 0 значение точно соответствует целочисленному значению уровня
	TinyMinus = 255,			//-1 значение отошло от целого уровня в минус, но считается тем же уровнем 
	TinyPlus = 1,				// 1 значение перевалило грань уровня в плюс, но считается тем же уровнем
	FatalMinus = 254,			//-2 значение ушло вниз от порога уровня настолько, что пора скачком привести его к более низкому уровню
	FatalPlus = 2,				// 2 значение вышло за планку уровня настолько, что пора скачком привести его до уровня
	FatalMinusLocked = 253,		//-3 значение пробило дно уже давно, новые не принимаются
	FatalPlusLocked = 3			// 3 значение пробило дно уже давно, новые не принимаются
};


//67 миллионов квантов - всего 32 или 64 (для беззнакового) уровня
#define LVL_BITS 26

//2 миллиона квантов до порога, а потом резко в 32 раза больше
#define THRE_BITS 21

//константы максимального опыта до границы уровня и до порога повышения уровня
#define LVL_SIZE_IN_XP	(1<<LVL_BITS)
#define LVL_HALF_IN_XP	(1<<(LVL_BITS-1))
#define MAX_XP_IN_LVL	(LVL_SIZE_IN_XP-1)
#define MAX_XP_UPLVL	((1<<THRE_BITS)-1)
#define PROGRESS_HIDER  (~MAX_XP_IN_LVL)
#define MAX_XP			(1<<31)
#define MAX_LEVEL		(1<<(31-LVL_BITS))
#define MAX_LEVEL_EXP	(1<<(1<<(31-LVL_BITS)))

//битовая сборка признака выхода за границу
#define THRE_OVER ((int)ELevelShift::FatalMinus & (int)ELevelShift::FatalPlus)

UENUM(BlueprintType) enum class EPhene : uint8
{	Strength,		// сила - урон от атак ВНИМАНИЕ, этот параметр придётся сделать очень в большом диапазоне для того, чтоб была разность между существами
	Agility,		// скорость - коэффициенты скоростей движения, анимации атак
	Stamina,		// запас энергии - выносливость, сопротивление усталости, атакам и возможность восстанавливаться
	Care,			// сознательность - точность атак, уровень скрытности, степень контроля за эмоциями
	Charm,			// привлекательность, уменьшает ненависть других, увеличивает любовь
	NUM
};

//выбор кривой роста навыка от роса уровня
enum PHENE_CURVE { pLIN, pACC, pDEC, pSGM, PHENE_CURVE_MAX };			

//новая попытка
USTRUCT(BlueprintType) struct FPhene
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 XP;					//представление в виде квантов опыта
	
	static float LUT[PHENE_CURVE_MAX][MAX_LEVEL+2];							//таблица подготовленных значений для целых уровней

	int8  BareLevel()	const { return (XP >> LVL_BITS); }					// "голый" уровень, просто старшие биты, без учёта деградации
	int32 BareProgress()const { return (XP & MAX_XP_IN_LVL); }				// дробная часть от нижней границы голого уровня
	int32 BareLevelXP()	const { return (XP & PROGRESS_HIDER); }				// количество квантов, соответствующее круглому уровню без прогресса дальше
	bool Regress()		const { return (BareProgress() > LVL_HALF_IN_XP); }	// прогресса больше половины, тяготеет к следующей планке уровня, рассматривается как регресс от следующей планки вниз
	int32 Level()		const { return (XP+LVL_HALF_IN_XP) >> LVL_BITS; }	// практический уровень, если к голому прогрессу прибавить еще 1/2 уровневой порции, то в битах уровня будет на 1 больше
	int32 LevelXP()		const { return (XP+LVL_HALF_IN_XP)&PROGRESS_HIDER; }// количество опыта соответствующее круглому уровню, но уже практическому, а не "голому"
	int32 Progress()	const { return XP - LevelXP(); }					// практический прогресс/регресс, просто разность текущего опыта и круглого опыта соответствующего посчитанному практическому уровню
	bool Overflow()		const { return FMath::Abs(Progress()) > MAX_XP_UPLVL; } //если практический прогресс или регресс выходит за порог пассивного накопления
	bool OverflowUp()	const { return Progress() > MAX_XP_UPLVL; }			//если практический прогресс выходит за порог пассивного накопления в плюс = хочет поднять уровень
	bool OverflowDn()	const { return Progress() < -MAX_XP_UPLVL; }		//если практический регресс выходит за порог пассивного накопления в плюс = хочет поднять уровень

	float R_Progress()	const { return Progress() / (float)MAX_XP_UPLVL; }
	float R_ProgAbs()	const { return FMath::Abs(R_Progress()); }
	float R_WholeXP()	const { return XP / (double)MAX_XP;  }
	float R(int Cu)		const { return LUT[Cu][Level()]; }					//произвольный ряд

	int32 SetBareLevel(uint8 nL)		{ int R = Level(); XP = nL * LVL_SIZE_IN_XP; return Level() - R; }
	int32 ChangeBareLevel(int Offset)	{ return SetBareLevel(BareLevel() + Offset); }
	bool EraseProgress()				{ ChangeBareLevel(0); return true; }
	bool AddExperience(int32 NewXP)		{ if(!Overflow()) {XP += NewXP; return true; } else return false;	}
	int32 Uplevel(bool Don = false)		{ if(Overflow()||Don) return ChangeBareLevel((Progress()>0) + Don); else return 0; }
	bool Donate()						{ return (OverflowUp() && EraseProgress()); }
	void SetRandom(int L, int Rng=-1)	{ SetBareLevel(L); if(Rng<0)Rng=MAX_XP_UPLVL; XP += FMath::RandRange(-Rng, Rng); }

	//статус заполненности, что делать с этим парметром - для гуя
	ELevelShift GetStatus() const
	{	if(BareProgress() == 0)	return ELevelShift::Exactly;
		if(Overflow())
		{	if(OverflowUp())	return ELevelShift::FatalPlus;
			else				return ELevelShift::FatalMinus;
		}
		else if(Progress()>0)	return ELevelShift::TinyPlus;
		else 					return ELevelShift::FatalMinus;
	}
};

//сборка всех прокачиваемых характеристик персонажа + даватель по ним производных коэффициентов для геймплея
USTRUCT(BlueprintType) struct FPhenotype
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FPhene Strength;		
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FPhene Agility;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FPhene Stamina;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FPhene Care;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FPhene Charm;
	float AttackDamageFactor()  { return  0.8 + 0.4 * ( 0.8 * Strength.R(pLIN) + 0.2 * Agility.R(pLIN)); }
	float AttackShieldFactor()  { return  0.6 + 0.8 * ( 0.4 * Strength.R(pLIN) + 0.4 * Stamina.R(pLIN) + 0.2 * Agility.R(pLIN)); }
	float BumpShockResistance() { return  0.7 + 0.6 * ( 0.6 * Stamina.R(pLIN)  + 0.4 * Strength.R(pLIN)); }
	float HearingRangeFactor()  { return  0.6 + 0.8 * ( 0.7 * Care.R(pLIN)     - 0.2 * Agility.R(pLIN) - 0.1 * Strength.R(pLIN) + 0.3); }
	float JumpForceFactor()     { return  0.7 + 0.6 * ( 0.6 * Strength.R(pLIN) + 0.4 * Agility.R(pLIN)); }
	float JumpBackForceFactor() { return  0.7 + 0.6 * ( 0.6 * Agility.R(pLIN)  + 0.4 * Strength.R(pLIN)); }
	float MoveSpeedFactor()     { return  0.8 + 0.4 * ( 0.8 * Agility.R(pLIN)  + 0.2 * Strength.R(pLIN)); }

	FPhene& ByEnum(EPhene P)	{ return ((FPhene*)this)[(int)P]; }
	FPhene& ByInt(int I)		{ return ((FPhene*)this)[I]; }
};

