#pragma once
#include "CoreMinimal.h"
#include "AdvMath.h"
#include "myrra_psyche.generated.h"

//###################################################################################################################
//безумно компактная форма психологической локации для внешнего объекта
//###################################################################################################################
USTRUCT(BlueprintType) struct FPsyLoc
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FBipolar X = 0.5f;				// вектор единичного направления на цель
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FBipolar Y = 0.5f;				// вектор единичного направления на цель
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FBipolar Z = 0.5f;				// вектор единичного направления на цель
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRatio D = 1.0f;				// квадратный корень расстояния до цели

	bool IsValid() const { return !(X == 0.0f && Y == 0.0f && Z == 0.0f); }
	operator bool() const { return IsValid(); }
	void SetInvalid() { X = 0.0f; Y = 0.0f; Z = 0.0f; D = 1.0f; }

	void EncodeDist(float Dist)		{ D = (int)FMath::Sqrt(Dist);	}

	void EncodeDir(FVector3f Dir)	{ X = Dir.X; Y = Dir.Y; Z = Dir.Z; }
	void EncodeDir(FVector Dir)		{ EncodeDir((FVector3f)Dir); }

	float Encode(FVector3f Radius)	{ float iD = FMath::InvSqrt(Radius.SizeSquared()); Radius *= iD; EncodeDir(Radius); iD = FMath::InvSqrtEst(iD); D = (int)iD; return iD*iD; }
	float EncodePrecise(FVector3f R){ float iD = FMath::InvSqrt(R.SizeSquared()); R *= iD; EncodeDir(R); D = FMath::InvSqrt(iD); return 1/iD; }
	float Encode(FVector Radius)	{ return Encode((FVector3f)Radius); }

	float DecodeDist() const		{ return D*(float)D; }
	FVector3f DecodeDir() const		{ return FVector3f(X, Y, Z); }
	FVector2f DecodeDir2d() const	{ return FVector2f(X, Y); }
};

//###################################################################################################################
//опорные эмоции мнемонически понятные
//###################################################################################################################

#define __I	 64
#define _II	128
#define III	192

#define EMPATH(R,L,F) (((R/64)<<4) + ((L/64)<<2) + ((F/64)))

//из константы выдрать 2-битные коды каналов
#define OUTSURE(P) ((((uint8)P)>>6)&3u)
#define OUTRAGE(P) ((((uint8)P)>>4)&3u)
#define OUTLOVE(P) ((((uint8)P)>>2)&3u)
#define OUTFEAR(P) ((((uint8)P)>>0)&3u)

//из сокращенных кодов в реальные опорные значения каналов
#define GETRAGE(P) (OUTRAGE(P)*64)
#define GETLOVE(P) (OUTLOVE(P)*64)
#define GETFEAR(P) (OUTFEAR(P)*64)

//,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
enum PATHIA
{
	Void		= EMPATH(  0,  0,  0),
	//-------------------------------------------------------------------------------------
	Wane		= EMPATH(__I,__I,__I),			//опустошенность
	Reverie		= EMPATH(__I,_II,__I),			//мечтательность, надежда
	Gloom		= EMPATH(_II,__I,__I),			//хмурость
	Worry		= EMPATH(__I,__I,_II),			//беспокойство
	//-------------------------------------------------------------------------------------
	Peace		= EMPATH(_II,_II,_II),			//уравновешенность, покой в бодрствовании, уравновешены все эмоции
	Hope		= EMPATH(__I,_II,_II),			//недежда на лучшее, но неуверенность
	Pessimism	= EMPATH(_II,__I,_II),			//пессимизм, мизантропия
	Cheer		= EMPATH(_II,_II,__I),			//беспечность
	//=====================================================================================
	Enmity		= EMPATH(III,_II,_II),			//враждебность, осознанная оппозиция с уважением и осторожностью
	Hatred		= EMPATH(III,__I,_II),			//ненависть, враждеьность с осторожностью, но без уважения
	Anger		= EMPATH(III,_II,__I),			//сердитость, обиженность, злоба на того, кто важен
	Fury		= EMPATH(III,__I,__I),			//бешенство, безудержная неприязнь
	//-------------------------------------------------------------------------------------
	Grace		= EMPATH(_II,III,_II),			//благосклонность, осознанная поддержка с критичностью и осторожностью
	Care		= EMPATH(__I,III,_II),			//любовь, обожание с волнением, боязнь сделать хуже
	Pride		= EMPATH(_II,III,__I),			//гордость, покровительство, пренебрежительное вазвышение
	Adoration	= EMPATH(__I,III,__I),			//обожание, без страха и критичности
	//-------------------------------------------------------------------------------------
	Fear		= EMPATH(_II,_II,III),			//боязнь, осознанное принятие опасности
	Anxiety		= EMPATH(__I,_II,III),			//предчувствие дурного? или сокрушенность? или стыд? удивленность? или очень осторожное любопытство
	Phobia		= EMPATH(_II,__I,III),			//фобия, страх и ненависть
	Horror		= EMPATH(__I,__I,III),			//ужас
	//=====================================================================================
	Mania		= EMPATH(III,III,__I),			//мания, одержимость
	Awe			= EMPATH(__I,III,III),			//благоговение, трясучесть от страха и обожания, паника
	Disgust		= EMPATH(III,__I,III),			//отвращение, активная неуемная ненависть, желание скрыться или уничтожить
	Insanity	= EMPATH(III,III,III)			//безумие, 
}; 

//очередной ёбаный пиздец анрила, он не переваривает макросы в определениях перечислителей,
//поэтому приходится заводить ещё один чистый энум, и его уже ссыпать в основной
UENUM(BlueprintType) enum class EPathia : uint8
{
	Void = ::Void,
	Wane = ::Wane,
	Worry = ::Worry,
	Horror = ::Horror,
	Reverie = ::Reverie,
	Hope = ::Hope,
	Anxiety = ::Anxiety,	
	Adoration = ::Adoration,
	Care = ::Care,		
	Awe = ::Awe,				
	Gloom = ::Gloom,
	Pessimism = ::Pessimism,
	Phobia = ::Phobia,	
	Cheer = ::Cheer,
	Peace = ::Peace,	
	Fear = ::Fear,	
	Pride = ::Pride,
	Grace = ::Grace,
	Fury = ::Fury,	
	Hatred = ::Hatred,
	Disgust = ::Disgust,
	Anger = ::Anger,
	Enmity = ::Enmity,								
	Mania = ::Mania,
	Insanity = ::Insanity
};

ENUM_RANGE_BY_FIRST_AND_LAST(EPathia, EPathia::Void, EPathia::Insanity);

//###################################################################################################################
//эмоциональное отношение к чему бы то ни было
//###################################################################################################################
USTRUCT(BlueprintType) struct FPathia
{	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 Rage = _II;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 Love = _II;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 Fear = _II;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 Work = _II;

	//конструктора
	FPathia()									: Rage(_II),Love(_II),Fear(_II),Work(_II) {}
	FPathia(uint8 R, uint8 L, uint8 F, uint8 W) : Rage(R),Love(L),Fear(F),Work(W) {}
	FPathia(uint8 R, uint8 L, uint8 F, int W)	: Rage(R),Love(L),Fear(F),Work(_II+W) {}
	FPathia(float R, float L, float F)			: Rage(R),Love(L),Fear(F),Work(_II) {}
	FPathia(float All) 							: Rage(All),Love(All),Fear(All),Work(_II) {}
	FPathia(FLinearColor O)						: Rage(O.R*255),Love(O.G*255),Fear(O.B*255),Work(O.A*255) {}		//
	FPathia(PATHIA A)							: Rage(GETRAGE(A)),Love(GETLOVE(A)),Fear(GETFEAR(A)),Work(_II){}	// из константы опорного значения
	FPathia(EPathia A)							: FPathia((PATHIA)A) {}												// из константы опорного значения
	FPathia(char l)								: Rage(255),Love(255),Fear(255),Work(0) {}							// латентная, Work - счётчик до рождения
	FPathia(PATHIA A, int8 R, int8 L, int8 F, int8 W)
		: Rage(GETRAGE(A)+R), Love(GETLOVE(A)+L), Fear(GETFEAR(A)+F), Work(_II+W) {}	// из константы опорного значения со сдвигом
	FPathia(PATHIA A, int8 W)
		: Rage(GETRAGE(A)), Love(GETLOVE(A)), Fear(GETFEAR(A)), Work(_II+W) {}			// из константы опорного значения со сдвигом
	FPathia(EPathia A, int8 R, int8 L, int8 F, int8 W) : FPathia((PATHIA)A,R,L,F,W) {}

	//формы с плавающей запятой
	float fRage() const { return (float)Rage / 255.0; }
	float fLove() const { return (float)Love / 255.0; }
	float fFear() const { return (float)Fear / 255.0; }
	float rRage() const { return (float)Rage / 127.0 - 0.5f; }
	float rLove() const { return (float)Love / 127.0 - 0.5f; }
	float rFear() const { return (float)Fear / 127.0 - 0.5f; }

	//мания, используется для учёта осторожности к жертвам ради еды,
	float fMania() const { return fRage()*fLove()*(1 - FMath::Abs(fRage() - fLove())); }

	//представить как цвет, чтоб вовне показать
	operator FLinearColor() { return FLinearColor(fRage(), fLove(), fFear(), 1.0f); }
	operator FVector() { return FVector(fRage(), fLove(), fFear()); }
	operator FVector3f() { return FVector3f(fRage(), fLove(), fFear()); }
	operator FVector3f() const { return FVector3f(fRage(), fLove(), fFear()); }

		//простая форма расстояния по сумме осей
	float DistAxial(const FPathia O) const { return FMath::Abs(fRage() - O.fRage()) + FMath::Abs(fLove() - O.fLove()) + FMath::Abs(fFear() - O.fFear()); }

	//векторная форма по скалярному произведению
	float DistSquare(const FPathia O) const { return FMath::Square(fRage() - O.fRage()) + FMath::Square(fLove() - O.fLove()) + FMath::Square(fFear() - O.fFear()); }

	//разность со знаком, степень неуравновешенности
	int16 DiffAxial(const FPathia O) const { return (int16)Rage-O.Rage+Love-O.Love+Fear-O.Fear; }

	//расстояние между эмоциями в целочисленной форме
	uint16 uiDistAxial(const FPathia O) const { return FMath::Abs(Rage - O.Rage) + FMath::Abs(Love - O.Love) + FMath::Abs(Fear - O.Fear); }

	//простой кэффициент желания приближаться или отдаляться при этой эмоции
	float DriveGain(float Vi, bool Se) const { return fLove() - fFear() + ((Rage>_II) ? fRage() : -fRage()); }

	//величина эмоции в целочисленной форме относительно пустоты и нормы
	uint16 BiSize() const { return DiffAxial(FPathia(Peace)); }
	uint16 UniSize() const { return DiffAxial(FPathia(Void)); }

	//сила эмоции относительно пустоты и нормы в диапазоне 0-1
	float BiPower() const { return DistSquare (FPathia(Peace)); }
	float UniPower() const { return DistSquare (FPathia(Void)); }

	//правильная форма дистанции как между векторами
	float DistReal(FPathia& O) const { return FVector3f::Distance((FVector3f)*this, (FVector3f)O); }
	float DistReal2(FPathia& O) const { return FVector3f::DistSquared((FVector3f)*this, (FVector3f)O); }

	static uint8 Mean(uint8 A, uint8 B) { return ((uint16)A + B)/2+1; }
	static uint8 Add(uint8 A, uint8 B) { return FMath::Min(255, (uint16)A + B); }
	static uint8 Sub(uint8 A, uint8 B) { return FMath::Max(0, (int16)A - B); }
	static bool Within(uint8 O, uint8 A, uint8 B) { return O >= FMath::Min(A,B) && O <= FMath::Max(A,B); }

	//приращение эмоции в сторону целевой от разницы потенциалов между текущей и целевой
	uint8 Speed(uint8 V)
	{	static uint8 lut[] = {1,2,3,4,5,6,3,5,4,3,2,3,2,1,5,8};
		return lut[(V+15)>>4];
	}

	void AffectChan (uint8& Destin, uint8 Source, uint8 ExternalLimit)
	{	if(Work == 0) return;			// если энергии не хватает на любое приращение
		if(ExternalLimit == 0) return;	// если внешний лимит запрещает любое приращение 
		int16 D = Destin - Source;		// разница между текущим и целью,+/-, "напряжение" стимула
		uint16 Da = FMath::Abs(D);		// величина разницы по модулю, насколько разнятся уровни
		auto dD = Speed(Da);			// реальное приращение, как бы ток от напряжения
		if(dD > Work) dD = Work;		// обрезать приращение по запасам энергии
		if(dD > ExternalLimit) dD = Work;// обрезать приращение по внешнему литиму
		if(D>0) Destin -= dD; else Destin += dD;
		Work -= dD;						// потратить энергию
	}

	//принять действие внешней эмоции
	int Affect(const FPathia Dst, uint8 ExtLim = 255)
	{	uint8 OldWork = Work;				// сохранить работу до воздействия
		AffectChan(Fear, Dst.Fear, ExtLim);	// каждое изменение эмоции отнимает энергию
		AffectChan(Rage, Dst.Rage, ExtLim);	// страх самое неотложное, потом злоба, потом любовь
		AffectChan(Love, Dst.Love, ExtLim);	// если энергия иссякла, эмоция не меняется
		int nWork = Work + Dst.Work - 127;  // восплонить из целевой эмоции утраченное (Dst>127) или отсосать остатки (Dst<127)
		Work = FMath::Clamp(nWork,0,255);	// может отсосать до нуля или наполнить до предела 255
		return (int)Work - OldWork;			// вернуть потраченную работу  (изменения в большую или меньшую сторону)
	}

	//просто смешать
	void Interpo(const FPathia Dst, float Alpha)
	{	Rage = 255 * FMath::Lerp(fRage(), Dst.fRage(), Alpha);
		Love = 255 * FMath::Lerp(fLove(), Dst.fLove(), Alpha);
		Fear = 255 * FMath::Lerp(fFear(), Dst.fFear(), Alpha);
	}

	//умножение эмоций здесь будет комбинация до уровня среднего
	FPathia operator*(const FPathia O) const { return FPathia(Mean(Rage,O.Rage), Mean(Love,O.Love), Mean(Fear,O.Fear), Mean(Work, O.Work)); }

	//сложение эмоций здесь будет просто сумма компонентов с насыщением
	FPathia operator+(const FPathia O) const { return FPathia(Add(Rage,O.Rage), Add(Love,O.Love), Add(Fear,O.Fear), Add(Work, O.Work)); }

	//савнение эмоций - точное равенство по всем аффектным компонентам
	bool operator==(const FPathia O) const { return (Love==O.Love && Rage==O.Rage && Fear==O.Fear); }
	bool operator!=(const FPathia O) const { return (Love != O.Love || Rage != O.Rage || Fear != O.Fear); }
	bool operator!=(const EPathia O) const { return (*this != FPathia(O)); }

	//"меньше" - это не меньше, а примерное равенство на уровне мнемонических констант, то есть практическая близость эмоций
	bool operator<(const FPathia O) const { return (EPathia)EMPATH(Rage, Love, Fear) == (EPathia)EMPATH(O.Rage, O.Love, O.Fear); }

	//"больше" - это не больше, сильное неравенство на уровне мнемонических констант, то есть практическая отдаленность эмоций
	bool operator>(const FPathia O) const { return (EPathia)EMPATH(Rage, Love, Fear) != (EPathia)EMPATH(O.Rage, O.Love, O.Fear); }

	//латентная форма для рефлекса, не участвует в чувствах, используется для учёта частоты использования
	bool IsLatent() const { return (Love==255 && Rage==255 && Fear==255); }

	//проверка вхождения в множество, заданное кубом из двух опорных эмоций 
	bool Within(FPathia L1, FPathia L2) const { return Within(Love, L1.Love, L2.Love) && Within(Rage, L1.Rage, L2.Rage) && Within(Fear, L1.Fear, L2.Fear); }

	//скалярное произведение в дробной форме
	float operator|(FPathia& O) const { return fRage()*O.fRage() + fLove()*O.fLove() + fFear()*O.fFear(); }
	float Dot(float R, float L, float F) const { return fRage()*R + fLove()*L + fFear()*F; }

	//из численного представления получить наиболее близкий мнемонический архетип, выдать расстояние до него, то есть точность определения
	EPathia GetArchetype() const { return (EPathia)EMPATH(Rage, Love, Fear); }
	float GetFullArchetype(EPathia& Closest, EPathia& SecondClosest) const 
	{	Closest = GetArchetype();
		float Dist1 = DistAxial(Closest), Dist2 = 1000;
		for (EPathia i : TEnumRange<EPathia>())
		{
			if ((EPathia)i == Closest) continue;
			float nuDist = DistAxial(FPathia((EPathia)i));
			if(nuDist < Dist2) { Dist2 = nuDist; SecondClosest = (EPathia)i; }
		}
		return Dist1 / (Dist1 + Dist2);
	}
};

//номера битов элементарных компонентов отношения к объекту
UENUM(BlueprintType, meta = (Bitflags)) enum class EYeAt : uint8
{
	NotMe, Big, New, Important, Unreachable, Seen,				
	Shine, Rain, Cold, Night, Fog, Indoor,				

	Injured,			//0 - здоровый или не имеющий здоровья, 1 - явно раненный
	Dying,				//0 - здоровый или не имеющий здоровья, 1 - умирающий, с низким уровнем здоровья

	Tired,				//0 - полный сил или не имеющий сил, 1 - явно уставший
	Relaxing,			//0 - полный сил или не имеющий сил, 1 - явно уставший

	Jumping,		//
	Flying,			//

	NoticingMe,		//◘

	AimingMe,		//◘

	Attacking,		//

	HurtMe,			//◘
	HurtMeMore,		//◘

	HurtFriend,		//◘
	HurtEnemy,		//◘

	PleasedMe,		//◘

	PleasedFriend,	//◘

	ComingCloser,	//◘
	ComingAway,		//◘

	a29,
	a30,
	MAX
};


UENUM(BlueprintType, meta = (Bitflags)) enum class EMeAt : uint8
{
	NotMe, HurtHead, New, HurtTorso, HurtLeg, Moving,					
	Shine, Rain, Cold, Night, Fog, Indoor,

	Injured =			(uint8)EYeAt::Injured,
	InjuredMore =		(uint8)EYeAt::Dying,

	Tired =				(uint8)EYeAt::Tired,
	TiredMore =			(uint8)EYeAt::Relaxing,

	YoureJumping,			//
	YoureFlying,			//

	YoureNoticingMe,		//◘

	Falling =			(uint8)EYeAt::AimingMe,

	YoureAttacking,			//

	Pain =				(uint8)EYeAt::HurtMe,
	PainMore =			(uint8)EYeAt::HurtMeMore,

	YouDidHarmToFriend,		//◘
	YouDidHarmToEnemy,		//◘

	YouDidGraceToMe,		//◘

	YouDidGraceToFriend,	//◘

	YoureComingCloser,		//◘
	YoureComingAway,		//◘

	a29,
	a30,

	MAX

};


inline int32 MeBe(EMeAt O1) { return 1 << (int)O1; }
inline int32 YeBe(EYeAt O1) { return 1 << (int)O1; }

#define ME_(A) MeBe(EMeAt::##A)
#define ME2_(A, B) (MeBe(EMeAt::##A)|MeBe(EMeAt::##B))
#define ME3_(A, B, C) (MeBe(EMeAt::##A)|MeBe(EMeAt::##B)|MeBe(EMeAt::##C))
#define ME2B_(A) (MeBe(EMeAt::##A) | MeBe((EMeAt)((int)(EMeAt::##A)+1)))

#define YE_(A) YeBe(EYeAt::##A)
#define YE2_(A, B) YeBe(EYeAt::##A)|YeBe(EYeAt::##B)
#define YE3_(A, B, C) (YeBe(EYeAt::##A)|YeBe(EYeAt::##B)|YeBe(EYeAt::##C))
#define YE4_(A, B, C, D) (YeBe(EYeAt::##A)|YeBe(EYeAt::##B)|YeBe(EYeAt::##C)|YeBe(EYeAt::##D))

//###################################################################################################################
//эмоциональный стмул, направление изменения эмоции
//###################################################################################################################

//костыль, из-за того, что в аргументах TMap нельзя уточнять, как будет показываться свойство
USTRUCT(BlueprintType) struct FReflex
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = EYeAt))	int32 Condition;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)											FPathia Emotion;

	FReflex() : Condition(0), Emotion(Peace) {}
	FReflex(int32 C) : Condition(C), Emotion(Peace) {}
	FReflex(int32 C, FPathia E) : Condition(C), Emotion(E) {}
	bool operator==(const FReflex& O) const { return (this->Condition == O.Condition); }
};

uint32 FORCEINLINE GetTypeHash(FReflex O) { return GetTypeHash(O.Condition); }

//###################################################################################################################
// психологический образ кого бы то ни было
//###################################################################################################################
USTRUCT(BlueprintType) struct FUrGestalt
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRatio TimeStable = 0;		// время стабильности характеристик
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EPathia PrevEmotion;		// прошлая эмоция, в компактной форме
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FPathia Emotion;			// текущая эмоция, отношение сугубо к этому объекту
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (Bitmask, BitmaskEnum = EYeAt)) int32 Influences = 0;		// биты эмоциональных стимулов

	static int32 RandVar;													// Myrra.cpp

	//добавление влияний, капает в ИИ в ответ на изменения в окружающем мире
	void AddCause(uint32 S) { Influences |= S; }
	void DelCause(uint32 S) { Influences &= ~S; }
	void SetCause(uint32 S, bool V) { if (V) AddCause(S); else DelCause(S); }
	void SetCauseChance(uint32 S, float P) { if (RandVar < P * RAND_MAX) AddCause(S); else DelCause(S); }
	bool HasCause(uint32 S) const { return (Influences & S) == S; }
	void SetCause2bit(uint32 Mask, float Ratio)
	{	Influences &= ~Mask;
		if (Ratio < 0.33) Influences |= ((Mask >> 1) | Mask); else		// 00[01]10
		if (Ratio < 0.66) Influences |= ((Mask << 1) | Mask); else		// 01[10]00
			Influences |= (Mask);										// 00[11]00
	}

	FUrGestalt() : Emotion(Peace) {}

	void UrTick(int Change = 0)
	{
		//применение коэффициента скукоты
		TimeStable = TimeStable + (1 - FastLog2((uint32)Change));
	}
};

USTRUCT(BlueprintType) struct FGestalt : public FUrGestalt
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRatio VisCoaxis = 0;		// потенциальная видимость
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRatio Audibility = 0;		// уровень слышимости, обновляется сигналами ИИ, расстоянием, видимостью
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRatio Visibility = 0;		// видимость, уверенность, что вижу, кэш с запаздыванием
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRatio TimeKnown = 0;		// время знакомости, в начале растет быстро, потом медленнее, при невидении почти не растёт
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FPsyLoc Location;			// психологически скомканные данные о пространственном расположении объекта относительно нас
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
						TWeakObjectPtr<USceneComponent> Obj = nullptr;		// сам объект
	
	FGestalt(FUrGestalt U) : FUrGestalt(U) {}
	FGestalt() : FUrGestalt() {}
	FGestalt(USceneComponent* C) : FUrGestalt(), Obj(C)  {}

	FVector3f LookDir() const {	return Location.DecodeDir(); }

	//сравнение только по объектам, нужно для упаковки гештальтов во множество
	bool operator==(const FGestalt& O) const { return (this->Obj == O.Obj); }

	//добавление влияний, капает в ИИ в ответ на изменения в окружающем мире
	void AddInfluence(EYeAt S) { Influences |= 1 << (int)S; }
	void DelInfluence(EYeAt S) { Influences &= ~(1 << (int)S); }
	void SetInfluence(EYeAt S, bool V) { if (V) AddInfluence(S); else DelInfluence(S); }
	void SetInfluenceProbability(EYeAt S, int32 Random,  float V) { if (Random < V * RAND_MAX) AddInfluence(S); else DelInfluence(S); }
	bool HasInfluence(EYeAt S) const { return (Influences & (1 << (int)S)) > 0;	}

	//двухбитная степень обида на существо за побои
	uint8 GetBeaten() { return HasInfluence(EYeAt::HurtMe) + 2 * HasInfluence(EYeAt::HurtMeMore); }
	void SetBeaten(uint8 O) { SetInfluence(EYeAt::HurtMe, O&1); SetInfluence(EYeAt::HurtMeMore, (O>>1)&1); }

	//дабы не плодить биты, состояние "мертв" кодируется парадоксальной комбинацией НЕ НЕ Я и НЕДОСТУПЕН
	void MarkDead() { DelCause(YE_(Injured)|YE_(Dying)); 	}
	bool IsMarkedDead() { return HasCause(YE_(Injured)|YE_(Dying)); }

	//степень дружественности учитывает историю отношений
	bool IsFriend() const { return (GETLOVE(PrevEmotion) == III && GETLOVE(Emotion.GetArchetype()) >= _II); }
	uint16 iFriendliness() const { return (5*Emotion.Love + 3*GETLOVE(PrevEmotion)); }
	float Friendliness() const { return iFriendliness()/1024.0f; }

	//степень враждебности учитывает историю отношений
	bool IsEnemy() const { return (GETRAGE(PrevEmotion) == III && GETRAGE(Emotion.GetArchetype()) >= _II); }
	uint16 iEnmity() const { return (3*Emotion.Rage + 5*GETRAGE(PrevEmotion)); }
	float Enmity() const { return iEnmity()/1024.0f; }

	//близость, с разной степенью абстарктности, в оснвном в диапазоне 0-1
	float Proximity() const { return 1.0f/Location.DecodeDist(); }
	float HowClose() const { return 1.0f - Location.D; }

	//быстро проверить на живость... пока непонятно как быстрее, не таща лишние заголовки
	bool IsCreature() const { return Obj.IsValid() && Obj->GetFName() == TEXT("Mesh"); }
	class AMyrPhyCreature* Creature() { return (AMyrPhyCreature*)Obj->GetOwner(); }

	//интегральная заметность
	float PerceptAmount() const { return 0.6*Visibility + 0.4*Audibility; }

	//базовое желание скрыться, без учета, видит ли нас цель, и эпизодов охоты
	float FearStealth() const { return (Emotion.fFear() - Emotion.fRage()) * HowClose(); }

	//обновить данные по пространственному расположению цели
	float FixLoc(FVector3f YouMinusMe, FVector3f View) { return Location.Encode(YouMinusMe); VisCoaxis = (Location.DecodeDir() | View); }

	//инициализация для внешнего существа
	//==============================================================================================================
	void InitRival(float MySize, float YourSize)
	{
		AddCause(YE_(New));
		if(MySize*2 > YourSize) AddCause(YE_(Big));
	}

	//элементарный асинхронный акт замечания, также выдаёт свежепосчитанное расстояние до цели
	//==============================================================================================================
	float Hear(FVector3f YouMinusMe, FVector3f View)
	{	float Dist = Location.EncodePrecise(YouMinusMe);
		VisCoaxis = (FVector2f(Location.DecodeDir()) | FVector2f(View));
		if(VisCoaxis > 0.8) VisCoaxis = (Location.DecodeDir() | View);
		Audibility = Audibility + 100 * HowClose();
		return Dist;
	}

	//сила желания приближаться или удаляться
	//==============================================================================================================
	float GetAnalogGain() const
	{	return	Emotion.rLove() * ( 0.5f + 0.4f * Visibility + 0.1 * HowClose()) +
				Emotion.fFear() * (-0.5f - 0.4f * Visibility - 0.1 * HowClose() + 2.0f * int(HowClose() > 0.9)) +
				Emotion.rRage() * ( 0.2f + 0.4f * HowClose() + 0.4 * VisCoaxis);
	}
	//вес объекта, кандидатство в цели
	float GetWeight() const	{ return FMath::Abs(GetAnalogGain()); }

	//принципиальная реакция на побои
	//==============================================================================================================
	void ReactToDamage(float Strength, float Rand01)
	{
		uint8 Beaten = GetBeaten();
		float Attitude = (1.0f + Enmity()) * Strength * Rand01 * (4 - Beaten)/2;
		if(Attitude > 0.5f) Beaten++;
		if(Beaten > 3) Beaten = 3;
		SetBeaten(Beaten);
	}

	//такт образа, для перебора в цикле, извне дается сигнал, что прошло много времени и изменение эмоции за этот такт
	//==============================================================================================================
	void Tick(bool Tock, int Change = 0)
	{
		//слышимость при отсутствии сигналов сама пропадает
		Audibility--;

		//падение чувства видимости
		if(HasInfluence(EYeAt::Seen))
			Visibility = Visibility + 50 * Proximity();
		else Visibility = Visibility - 20 * Proximity();

		//"так" - редкое событие для подсчёта субъективного времени знания этого объекта и умещения в 1 байте
		if(Tock &&  Audibility>0.5)
		{	PrevEmotion = Emotion.GetArchetype();
			TimeKnown++;
		}

		//распрощаться со статусом "новый" если эмоция впервые значительно изменилась
		if(PrevEmotion != Emotion.GetArchetype()) DelInfluence(EYeAt::New);

		//из родителя
		UrTick(Tock);
	}
};

uint32 FORCEINLINE GetTypeHash(FGestalt O) { return GetTypeHash(O.Obj); }
