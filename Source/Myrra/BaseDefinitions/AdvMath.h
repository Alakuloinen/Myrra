#pragma once

#include "CoreMinimal.h"
#include "AdvMath.generated.h"

#define VF(x) ((FVector3f)(x))
#define VD(x) ((FVector)(x))

//###################################################################################################################
// представление байта как нормированного дробного
//###################################################################################################################
USTRUCT(BlueprintType) struct FRatio
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	uint8 M;

	uint8 iAdd(const int O) const { return FMath::Clamp((int)M + (int)O, 0, 255); }
	uint8 Intp(const uint8 O) const { int A = (int)M-O; return M + (A<0)?1:(A>0)?-1:0; }
	uint8 Diff(const uint8 O) const { FMath::Abs((int)M - O); }

	uint8 fAdd(const float O) const { return iAdd(O*255); }
	operator float() { return (float)M / 255; }
	operator float() const { return (float)M / 255; }

	FRatio() :M(0) {}
	FRatio(float In) { M = 0; M = fAdd(In); }
	FRatio(int In) { M = 0; M = iAdd(In); }

	FRatio operator+(const int &O) { return FRatio(iAdd(O)); }
	FRatio operator-(const int &O) { return FRatio(iAdd(-O)); }
	FRatio operator++(int nahui) { return M = iAdd(1); }
	FRatio operator--(int nahui) { return M = iAdd(-1); }
	FRatio operator+=(const int& O) { return M = iAdd(O); }
	FRatio operator-=(const int& O) { return M = iAdd(-O); }
	FRatio operator+=(const float& O) { return M = fAdd(O); }
	FRatio operator-=(const float& O) { return M = fAdd(-O); }
	FRatio operator+(const float& O) { return FRatio(fAdd(O)); }
	FRatio operator-(const float& O) { return FRatio(fAdd(-O)); }

	FRatio operator+(const FRatio& O) { return *this + (int)(O.M); }
	FRatio operator-(const FRatio& O) { return *this - (int)(O.M); }
	FRatio operator|(const FRatio& O) const { return Diff(O.M); }	// разница, дельта
	FRatio operator>>(const FRatio& O) { return Intp(O.M); }		// интерполяция линейная
};

USTRUCT(BlueprintType) struct FBipolar : public FRatio
{
	GENERATED_BODY()

	operator float() { return (float)M / 127.0f - 1.0f; }
	operator float() const { return (float)M / 127.0f - 1.0f; }
	uint8 fAdd(const float O) const { return iAdd((0.5*O+0.5f)*255); }

	FBipolar() :FRatio(0) {}
	FBipolar(float In):FRatio(0) { M = fAdd(In); }
	FBipolar(int In) :FRatio(0) { M = iAdd(In); }

	FBipolar operator+(const int &O) { return FBipolar(iAdd(O)); }
	FBipolar operator-(const int &O) { return FBipolar(iAdd(-O)); }
	FBipolar operator++(int nahui) { return FBipolar(iAdd(1)); }
	FBipolar operator--(int nahui) { return FBipolar(iAdd(-1)); }
	FBipolar operator+(const float& O) { return FBipolar(fAdd(O)); }
	FBipolar operator-(const float& O) { return FBipolar(fAdd(-O)); }

	FBipolar operator+(const FBipolar& O) { return *this + (int)(O.M); }
	FBipolar operator-(const FBipolar& O) { return *this - (int)(O.M); }
	FBipolar operator|(const FBipolar& O) const { return Diff(O.M); }	// разница, дельта
	FBipolar operator>>(const FBipolar& O) { return Intp(O.M); }		// интерполяция линейная
};


inline FVector3f operator*(FVector A, FVector3f B) { return (FVector3f)(A * (FVector)B); }
//inline FVector3f operator+(FVector A, FVector3f B) { return (FVector3f)(A + (FVector)B); }
inline FVector3f operator-(FVector A, FVector3f B) { return (FVector3f)(A - (FVector)B); }
inline FVector3f operator-(FVector3f A, FVector B) { return (FVector3f)((FVector)A - B); }
inline FVector   operator+(FVector A, FVector3f B) { return (FVector)(A + (FVector)B); }

//среднее по интервалу, странно, что такой встроенной функции нет
inline float Lerp(FFloatRange FR, float A) { return FMath::Lerp(FR.GetLowerBoundValue(), FR.GetUpperBoundValue(), A); }


//ограничить нулем и единицей с насыщением (чтоб не писать длинные FMath::Clamp(...) )
float FORCEINLINE SAT01(float v) { return v>1.0f ? 1.0f : (v<0.0f ? 0.0f : v); }

#define VMID (FVector3f(0.5f, 0.5f, 0.5f))

//==============================================================================================================
//линейная интерполяция в прямом смысле, с константным шагом, никаких кривых
//==============================================================================================================
float FORCEINLINE StepTo( float OldVal, float NewVal, float Step)
{
	float Diffe = NewVal - OldVal;
	if(Diffe > Step)	return OldVal + Step; else
	if(Diffe < -Step)	return OldVal - Step; else
						return NewVal;
}

uint8 FORCEINLINE To(const uint8 Me, const uint8 You)
{
	if (You > Me) return Me + 1; else if (You < Me) return Me - 1; else return Me;
}


//==============================================================================================================
//плавное изменение единичного вектора
//==============================================================================================================
FVector FORCEINLINE SmoothDistortion(const FVector& PrevValue, const FVector NewValue, float Coeff)
{
	return PrevValue + (NewValue - PrevValue)*Coeff*(0.1 + 0.9*FMath::Abs(FVector::DotProduct(NewValue, PrevValue)));
}

//==============================================================================================================
//плавное изменение величины, ограниченной [0 1]
//==============================================================================================================
float FORCEINLINE SmoothDistortion01(const float& PrevValue, const float NewValue, float Coeff)
{
	return PrevValue + (NewValue - PrevValue)*Coeff*(1.0 - 0.6*FMath::Abs(NewValue - PrevValue));
}

//==============================================================================================================
//плавное изменение величины, ограниченной [0 1]
//==============================================================================================================
float FORCEINLINE SmoothDistortion01uni(const float& PrevValue, const float NewValue, float Coeff)
{
	return PrevValue + (NewValue - PrevValue)*Coeff*(1.0 - 0.9*FMath::Abs(NewValue - PrevValue));
}

//==============================================================================================================
//функция колокол
//==============================================================================================================
float FORCEINLINE Bell(float X)
{
	if (X > 1 || X < 0) return 0;
	auto x = X * 2 - 1;
	auto xx = x * x;
	return 1 + 2 * FMath::Abs(x * xx) - 3 * xx;
}
float FORCEINLINE HardBell(float X)
{
	if (X > 1 || X < 0) return 0;
	return 4 * X * (1 - X);
}

//==============================================================================================================
//поиск позиции шестнадцатиричного разряда (4 бита) в 32-битном поле
//==============================================================================================================
uint32 FORCEINLINE GetHexDigit32 (uint32 Shifted)
{
	if(Shifted & 0xffff0000)
	{	if(Shifted & 0xff000000)
		{	if(Shifted & 0xf0000000)	return 7;
			else						return 6;
		}
		else
		{	if(Shifted & 0x00f00000)	return 5;
			else						return 4;
		}
	}
	else
	{	if(Shifted & 0x0000ff00)
		{	if(Shifted & 0x0000f000)	return 3;
			else						return 2;
		}
		else
		{	if(Shifted & 0x000000f0)	return 1;
			else						return 0;
		}
	}
}

//==============================================================================================================
// бес коментариев. позор, что такого нет в стандартной блилиотеке
//==============================================================================================================
float FORCEINLINE DotProduct(FLinearColor C1, FLinearColor C2)
{
	return C1.R*C2.R + C1.G*C2.G + C1.B*C2.B + C1.A*C2.A;
}

#define BITBIT(x,start,mask) ( (((unsigned long)(x))>>(start)) & (mask))
#define BITBIT2FLOAT(x,start,mask) ((float)( (((unsigned long)(x))>>(start)) & (mask)) / ((float)(mask)))

//расширенное понятие оси для определения отрицательных значений
UENUM(BlueprintType) enum class EMyrAxis : uint8
{
	None = 0 UMETA(Hidden),					//000
	Xp = 1 UMETA(DisplayName = "+X"),		//001
	Yp = 2 UMETA(DisplayName = "+Y"),		//010
	Zp = 3 UMETA(DisplayName = "+Z"),		//011
	Minus = 4 UMETA(Hidden),				//100
	Xn = 5 UMETA(DisplayName = "-X"),		//101
	Yn = 6 UMETA(DisplayName = "-Y"),		//110
	Zn = 7 UMETA(DisplayName = "-Z")		//111
};

UENUM(BlueprintType) enum class EMyrRelAxis : uint8
{
	Front = 0,
	Right = 1,
	Up = 2,
	_MaxNonInv = 3,
	Back = 4,
	Left = 5,
	Down = 6,
	NUM
};


//по константе оси получить константу противоположной оси
EMyrAxis FORCEINLINE AntiAxis(EMyrAxis Axis)
{
	//исключающее ИЛИ - на бите минуса есть единица, значит введенный бит будет меняться на противоположное значение
	return (EMyrAxis)(((uint8)Axis) ^ ((uint8)EMyrAxis::Minus));
}

//оси соответствующие перечислителям EMyrAxis - для быстроты (покоятся в Myrra.cpp)
extern FVector3f MyrAxes[8];

//совсем быстрый и ущербный по краям арксинус
float FORCEINLINE UFastArcSin(float X)
{ 
	return X * ( 1 + X*X*( 1/6 + X*X*(3/8 + 15/48*X*X))); 
}

uint8 FORCEINLINE FastLog2(uint64 A)
{
	const uint8 tab64[64] = {
    63,  0, 58,  1, 59, 47, 53,  2,
    60, 39, 48, 27, 54, 33, 42,  3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22,  4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16,  9, 12,
    44, 24, 15,  8, 23,  7,  6,  5};
    A |= A >> 1;
    A |= A >> 2;
    A |= A >> 4;
    A |= A >> 8;
    A |= A >> 16;
    A |= A >> 32;
    return tab64[((uint64_t)((A - (A >> 1))*0x07EDD5E59A4E28C2)) >> 58];
}

uint8 FORCEINLINE FastLog2(uint32 A)
{
	const uint8 tab32[32] = {
     0,  9,  1, 10, 13, 21,  2, 29,
    11, 14, 16, 18, 22, 25,  3, 30,
     8, 12, 20, 28, 15, 17, 24,  7,
    19, 27, 23,  6, 26,  5,  4, 31};
    A |= A >> 1;
    A |= A >> 2;
    A |= A >> 4;
    A |= A >> 8;
    A |= A >> 16;
    return tab32[(uint32_t)(A*0x07C4ACDD) >> 27];
}

//==============================================================================================================
// оценка скорости для достижения цели по баллистической траектории
//==============================================================================================================

	//TargetPos = MyPos + V*Time + a*Time*Time/2
	//MyPos - TargetPos + V*Time + a*Time*Time/2 = 0
	//(a/2)t^2 + (V)t + (-R) = 0

//скорость начальная баллистической траектории при заданных начальной и конечной точке, времени полёта
FVector3f FORCEINLINE BallisticStartVel(FVector3f YouMinusMe, float Time)
{ return (YouMinusMe - 0.5 * FVector3f(0,0,-981) * Time * Time) / Time; }

//оценка времени для пролёта по вектору с заданной скоростью
float FORCEINLINE BallisticMinTime(FVector3f YouMinusMe, float MaxVz, float MaxVxy)
{ return FMath::Sqrt((FMath::Abs(YouMinusMe.X*YouMinusMe.Y) + YouMinusMe.Z*YouMinusMe.Z)/(MaxVz*MaxVz + MaxVxy*MaxVxy)); }

//два корня квадратного уровненения, два времени пути для достижения заданной точки
FVector2f FORCEINLINE BallisticFlightTimes(FVector3f YouMinusMe, FVector3f V)
{	FVector3f g(0,0,-981);											// ускорение
	auto D = V.Z * V.Z - 4 * (g.Z/2) * (-YouMinusMe.Z);				// дискриминант b^2 - 4ac
	if (D >= 0)														// имеются корни, то есть траектория существует (а может здесь быть иначе?)
	{	float rD = FMath::Sqrt(D);									
		float Divi = 1 / (2 * (g.Z / 2));
		return FVector2f (-V.Z + rD, -V.Z - rD) * Divi;				// вычисление двух корней (-b+-rD)/2a
	}else return FVector2f (-1, -1);								// неверные времена
}

FVector3f FORCEINLINE FindBallisticStartVelocity(FVector TargetPos, FVector MyPos, float MaxVz, float MaxVxy, float& Time)
{
	float MaxTime = Time;										// сохранить максимально отведенное для прыжка время
	FVector3f R = FVector3f(TargetPos - MyPos);					// радиус-вектор, прямая дорога до цели
	Time = BallisticMinTime(R, MaxVz, MaxVxy);					// оценка времени на преодоление расстояния
	FVector3f V = BallisticStartVel(R, Time);					// первое приближение для скорости
	
	// если скорость получается больше, чем можно выдюжить
	while (FMath::Abs(V.Z) > MaxVz || V.SizeSquared2D() > MaxVxy * MaxVxy)
	{	Time *= 1.1;
		V = BallisticStartVel(R, Time);
		if (Time > MaxTime) return FVector3f(0);
	}
	return V;
}
//extern FVector3f FindBallisticStartVelocity(FVector TargetPos, FVector MyPos, float MaxVz, float MaxVxy, float& Time);

//просто точку на параболе с нужным сдвигом времени
FVector FORCEINLINE PosOnBallisticTrajectory(FVector3f V, FVector StartPos, float T)
{
	return StartPos + FVector(V)*T + FVector(0,0,-981)*T*T/2;
}
