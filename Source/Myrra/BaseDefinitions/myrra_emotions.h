#pragma once
#include "CoreMinimal.h"
#include "AdvMath.h"
#include "myrra_emotions.generated.h"

//БЛЯДЬ КАК ЖЕ ВЫ ЗАДОЛБАЛИ С ТАКИМИ ДЛИННЫМИ ИМЕНАМИ ДЛЯ ТАКИХ ПРОСТЫХ ВЕЩЕЙ
#define M1 GetLowerBoundValue()
#define M2 GetUpperBoundValue()


//надо вычищать этот класс, слишком много хни, особенно значений альфы
USTRUCT(BlueprintType) struct FEmotion
{
	GENERATED_USTRUCT_BODY()

	//представление в виде цвета
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor EquiColor;

	//базовые компоненты: ярость(красный), любовь(зеленый), страх(синий) - 
	float Rage()		const { return EquiColor.R; }
	float Love()		const { return EquiColor.G; }
	float Fear()		const { return EquiColor.B; }

	//или вот лучше сюда перенести параметр уверенности
	float& Sure() { return EquiColor.A; }


	//двухмерная карта (добро-зло; стремление-избегание)
	float GetPolarity() const { return 1.0*Love() - 1.0*Rage(); }
	float GetActivity() const { return 1.0*Rage() + 1.0*Love() - 1.0*Fear(); }

	//расстояние в квадрате от сего до введенного
	float Dist2(const FEmotion& o) const { return FVector::DistSquared(FVector(EquiColor), FVector(o.EquiColor)); }

	//яркость/мощь/активность эмоции - берется квадрат для скорости
	float Power() const { return FVector(EquiColor).SizeSquared(); }

	//перевзвешенная мощность чувств, чтобы те, кого мы боимся, больше побуждали к действиям, чем те, кого мы любим
	float AlertPower() const { return Rage() * 0.2f + Love() * 0.2f + Fear() * 0.6f; }

	//апетитность (для съедобных) - степень ровности 
	//float Appetit() const { return FMath::Clamp(GetActivity() - FMath::Abs(GetPolarity()), 0.0f, 1.0f); }



	//конструктора
	FEmotion() :EquiColor(FLinearColor::Black) {}
	FEmotion(FColor def) { EquiColor.R = 0.00392*(float)def.R; EquiColor.G = 0.00392*(float)def.G; EquiColor.B = 0.00392*(float)def.B; EquiColor.A = 0.00392*(float)def.A; }
	FEmotion(FLinearColor def) :EquiColor(def) {}
	FEmotion(float rage, float love, float fear, float s = 1.0) { EquiColor.R = rage; EquiColor.G = love; EquiColor.B = fear; EquiColor.A = s; }

	//перевод в узкий цвет как надо, а не по местным стандартам - для кодирования гештальтов в долговременную память
	operator FColor() const { return FColor(EquiColor.R * 255, EquiColor.G * 255, EquiColor.B * 255, FMath::Clamp(EquiColor.A, 0.0f, 1.0f) * 255); }

	//для вычисления взвешенной суммы
	FEmotion operator+(const FEmotion& o) { return FEmotion(Rage()+o.Rage(), Love()+o.Love(), Fear()+o.Fear(), EquiColor.A+o.EquiColor.A); }
	FEmotion operator*(float o) { return FEmotion(Rage()*o, Love()*o, Fear()*o); }

	//балансированный модификатор эмоции с терниями по краям диапазона
	//внимание, простое применение этого модификатора не гарантирует соблюдения здорового диапазона 0-1
	void ChangeValue(float& V, float delta)
	{	if (delta > 0.0f)
			V += delta * (1.0f - V);
		else V += delta * V;
	}

	//подмешать новую эмоцию с весом
	void Mix(FEmotion& NewComponent, float Weight)
	{
		EquiColor = FMath::Lerp(EquiColor, NewComponent.EquiColor, Weight);
	}

	//взять из узкого цвета (из долговременной памяти) с ослаблением, для симуляции неуверенности
	void MixWithArchetypeEmotion ( FColor Src, float Att)
	{	EquiColor.R = FMath::Lerp (EquiColor.R, 0.00392f*(float)Src.R, Att);
		EquiColor.G = FMath::Lerp (EquiColor.G, 0.00392f*(float)Src.G, Att);
		EquiColor.B = FMath::Lerp (EquiColor.B, 0.00392f*(float)Src.B, Att);
		if (Power() > 1.0f)
			ChangeFear(0);
	}

	void ChangeAll ( FLinearColor Dst, float Alpha)
	{	EquiColor.R = FMath::Lerp (EquiColor.R, Dst.R, Alpha);
		EquiColor.G = FMath::Lerp (EquiColor.G, Dst.G, Alpha);
		EquiColor.B = FMath::Lerp (EquiColor.B, Dst.B, Alpha);	}

	void Reset() { EquiColor = FLinearColor::Black; }

	//конкретная модификация эмоции
	void ChangeFear(float delta) { ChangeValue(EquiColor.B, delta); }
	void ChangeLove(float delta) { ChangeValue(EquiColor.G, delta); }
	void ChangeRage(float delta) { ChangeValue(EquiColor.R, delta); }
	void ChangeStoicity(float delta) { ChangeValue(EquiColor.A, delta); }
	void Inflate(float dm) { ChangeRage(dm); ChangeLove(dm); ChangeFear(dm); }

	//константы типичных эмоций
	static const FEmotion Peace() { return FLinearColor::Black; }
	static const FEmotion Anger() { return FLinearColor::Red; }
	static const FEmotion Adore() { return FLinearColor::Green; }
	static const FEmotion Horror() { return FLinearColor::Blue; }
	static const FEmotion Curiosity() { return FLinearColor(0.3f, 0.3f, 0.0f); }
	static const FEmotion Mania() { return FLinearColor::Yellow; }

	float Normalize()
	{	float N = Rage()*Rage() + Love()*Love() + Fear()*Fear();
		EquiColor.A = N;
		if(N>1.0)
		{	N = FMath::InvSqrt(N);
			EquiColor.R *= N;
			EquiColor.G *= N;
			EquiColor.B *= N;
			return 1.0f;
		}
		else return N;
	}

	FEmotion MixMeWithThis (const FEmotion& NewEmo, float Weight)
	{	return FEmotion(FMath::Lerp(EquiColor, NewEmo.EquiColor, Weight));	}


};


//диапазон эмоций - прямоугольник на карте эмоций (убрать)
/*USTRUCT(BlueprintType) struct FEmotionRange
{
	GENERATED_USTRUCT_BODY()

		//просто две эмоции
		UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor limit1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor limit2;

	FEmotion E1() const { return FEmotion(limit1); }
	FEmotion E2() const { return FEmotion(limit2); }

	//введенная эмоция S лежит в заданном пределе (по теореме пифагора)
	bool Within(const FEmotion& S) const { return E1().Dist2(S) + E2().Dist2(S) <= E1().Dist2(E2()); }

	FEmotionRange() : limit1(FColor::Black), limit2(FColor::White) {}

};*/

//###################################################################################################################
//диапазон эмоций - радиус, проще, заменить выше описанное
//###################################################################################################################
USTRUCT(BlueprintType) struct FEmotionRadius
{
	GENERATED_USTRUCT_BODY()

	//RGB цвета используем собственно для эмоции, альфу используем как радиус вокруг точки
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor EmotionRGBRadiusAlpha;

	//проверки - двоичная и степень подходящести
	bool Within(const FEmotion& MyEmo) const { return FEmotion(EmotionRGBRadiusAlpha).Dist2(MyEmo) < (float)EmotionRGBRadiusAlpha.A / 255.0f; }
	float Fitness(const FEmotion& MyEmo) const { const float res = FEmotion(EmotionRGBRadiusAlpha).Dist2(MyEmo) - (float)EmotionRGBRadiusAlpha.A / 255.0f; return res > 0.0f ? res : 0.0; }
};




//###################################################################################################################
// структура ад-хок, только для одного место, зато компактно и наглядно в редакторе - отношение к живому и мертвому
//###################################################################################################################
USTRUCT(BlueprintType) struct FAttitudeTo
{
	GENERATED_USTRUCT_BODY()

	//начальное отношение к живому представителю какой-то расы/вида
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FColor EmotionToLivingColor;

	//отношение к трупу
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FColor EmotionToDeadColor;
};

