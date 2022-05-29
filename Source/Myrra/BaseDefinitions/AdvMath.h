#pragma once

#include "CoreMinimal.h"
#include "AdvMath.generated.h"

//ограничить нулем и единицей с насыщением (чтоб не писать длинные FMath::Clamp(...) )
float FORCEINLINE SAT01(float v) { return v>1.0f ? 1.0f : (v<0.0f ? 0.0f : v); }

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
//поиск позиции шестнадцатиричного разряда (4 бита) в 32-битном поле
//==============================================================================================================
uint32 FORCEINLINE GetHexDigit32 (uint32 Shifted)
{
	if(Shifted & 0xffff0000)
	{
		if(Shifted & 0xff000000)
		{
			if(Shifted & 0xf0000000)	return 7;
			else						return 6;
		}
		else
		{
			if(Shifted & 0x00f00000)	return 5;
			else						return 4;
		}
	}
	else
	{
		if(Shifted & 0x0000ff00)
		{
			if(Shifted & 0x0000f000)	return 3;
			else						return 2;
		}
		else
		{
			if(Shifted & 0x000000f0)	return 1;
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
extern FVector MyrAxes[8];

//совсем быстрый и ущербный по краям арксинус
float FORCEINLINE UFastArcSin(float X)
{ 
	return X * ( 1 + X*X*( 1/6 + X*X*(3/8 + 15/48*X*X))); 
}

