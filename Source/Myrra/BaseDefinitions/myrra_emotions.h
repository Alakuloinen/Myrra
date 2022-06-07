#pragma once
#include "CoreMinimal.h"
#include "AdvMath.h"
#include "myrra_emotions.generated.h"

//БЛЯДЬ КАК ЖЕ ВЫ ЗАДОЛБАЛИ С ТАКИМИ ДЛИННЫМИ ИМЕНАМИ ДЛЯ ТАКИХ ПРОСТЫХ ВЕЩЕЙ
#define M1 GetLowerBoundValue()
#define M2 GetUpperBoundValue()


UENUM(BlueprintType) enum class EEmotio : uint8
{
	Void,			//опустошенность
	Reverie,		//мечтательность, надежда
	Gloom,			//хмурость
	Worry,			//беспокойство

	Peace,			//уравновешенность, покой в бодрствовании, уравновешены все эмоции
	Hope,			//недежда на лучшее, но неуверенность
	Pessimism,		//пессимизм, мизантропия
	Carelessness,	//беспечность


	Enmity,			//враждебность, осознанная оппозиция с уважением и осторожностью
	Hatred,			//ненависть, враждеьность с осторожностью, но без уважения
	Anger,			//сердитость, обиженность, злоба на того, кто важен
	Fury,			//бешенство, безудержная неприязнь

	Grace,			//благосклонность, осознанная поддержка с критичностью и осторожностью
	Love,			//любовь, обожание с волнением, боязнь сделать хуже
	Patronage,		//покровительство, пренебрежительное вазвышение
	Adoration,		//обожание, без страха и критичности

	Fear,			//боязнь, осознанное принятие опасности
	Anxiety,		//предчувствие дурного? или сокрушенность? или стыд? удивленность? или очень осторожное любопытство
	Phobia,			//фобия, страх и ненависть
	Horror,			//ужас


	Mania,			//мания, одержимость
	Awe,			//благоговение, трясучесть от страха и обожания, паника
	Disgust,		//отвращение, активная неуемная ненависть, желание скрыться или уничтожить
	Insanity,		//безумие, 
	MAX
};
//R безвольность			бодрствование					ярость
//G равнодушие				учтивость						любовь
//B беспечность				осторожность					ужас
//A - 
// * уровень эмоций потребляет энергию, при низкой энергии и низком запасе сил накал эмоций проседает
// * уровень эмоций увеличивает сонность, чем возбужденнее, тем больше захочется спать
// * дисперсия эмоций потребляет запас сил, чем более резкие различия у эмоций, тем быстрее организм выдыхается

//пока неясно, добавлять ли сюда полную причину с указателем объекта, или оставить безликой лептой воздействия
USTRUCT(BlueprintType) struct MYRRA_API FEmoStimulus
{
	GENERATED_USTRUCT_BODY()

	//эмоция, в сторону которой действует
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 R;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 G;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 B;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 A;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 Aftershocks;

	//базовые компоненты: ярость(красный), любовь(зеленый), страх(синий) - 
	float Rage()		const { return (float)R/255.0f; }		//тяга приблизиться
	float Love()		const { return (float)G/255.0f; }		//тяга приблизиться 
	float Fear()		const { return (float)B/255.0f; }		//тяга отдалиться 
	float Sure()		const { return (float)A/255.0f; }

};

//возможно, сделать с шаблоном
USTRUCT(BlueprintType) struct MYRRA_API FEmotio
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0")) float R;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0")) float G;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0")) float B;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0")) float A;

	//базовые компоненты: ярость(красный), любовь(зеленый), страх(синий) - 
	float Rage()		const { return R; }		//тяга приблизиться
	float Love()		const { return G; }		//тяга приблизиться 
	float Fear()		const { return B; }		//тяга отдалиться 
	float Sure()		const { return A; }

	float PowerSquared()const { return R * R + B * B + G * G; }
	float PowerAxial ()	const { return FMath::Abs(R) + FMath::Abs(G) + FMath::Abs(B); }
	float Discordance() const { return FMath::Abs(R-G) + FMath::Abs(G-B) + FMath::Abs(R-B); }

	FLinearColor ToFLinearColor() { return *((FLinearColor*)this); }
	FVector4 ToFVector4() { return *((FVector4*)this); }
	FVector ToFVector() const { return *((FVector*)this); }
	FColor ToFColor() { return FColor(R*255, G*255, B*255, A*255); }

	FEmotio()				: R(0),G(0),B(0),A(1) {}
	FEmotio(FColor O)		: R((float)O.R / 255.0), G((float)O.G / 255.0), B((float)O.B / 255.0), A((float)O.A / 255.0) {}
	FEmotio(FLinearColor O) : R(O.R), G(O.G), B(O.B), A(O.A) {}
	FEmotio(float All)		: R(All), G(All), B(All), A(1) {}
	FEmotio(float nRage, float nLove, float nFear, float Sure = 1) : R(nRage), G(nLove), B(nFear), A(Sure) {}

	FEmotio operator*	(float M) { return FEmotio(R*M, G*M, B*M, A); }
	FEmotio Dim			(float M) { if(M<=1 && M>=0) return (*this)*M; else return *this; }
	FEmotio StepTo		(FEmotio New, float Step, bool SureToo=false)	{ return FEmotio ( ::StepTo(R,New.R,Step), ::StepTo(G,New.G,Step), ::StepTo(B,New.B,Step), SureToo?(::StepTo(A,New.A,Step)):A ); }
	FEmotio Combine		(FEmotio N, bool SureToo=false)					{ return FEmotio ( FMath::Lerp(R, N.R, N.A), FMath::Lerp(G, N.G, N.A), FMath::Lerp(B, N.B, N.A), SureToo?FMath::Lerp(A,N.A,N.A):A); }
	FEmotio Mix			(FEmotio N, float E, bool SureToo=false)		{ return FEmotio ( FMath::Lerp(R, N.R, E), FMath::Lerp(G, N.G, E), FMath::Lerp(B, N.B, E), SureToo?FMath::Lerp(A,N.A,E):A); }

	bool ApplyStimulus(FEmoStimulus& S)
	{	if(S.Aftershocks==0) return false;
		float a = S.Sure();
		R = FMath::Lerp(R, S.Rage(), a);
		G = FMath::Lerp(G, S.Love(), a);
		B = FMath::Lerp(B, S.Fear(), a);
		S.Aftershocks = FMath::Max(S.Aftershocks-1, 0);
		return (S.Aftershocks!=0);
	}		

	float DistSquared3D (FEmotio& O) const { return FVector::DistSquared(ToFVector(), O.ToFVector()); }
	float Dist3D (FEmotio& O)		const { return FVector::Dist(ToFVector(), O.ToFVector()); }
	float DistAxial3D (FEmotio& O)	const { return FMath::Abs(R - O.R) + FMath::Abs(G - O.G) + FMath::Abs(B - O.B); }

	float PrimaryMotionStimulus() const { return R + G - 2*B*B; }
	float PrimaryStealthStimulus() const { return 2*(B - FMath::Square(B*B)) - R - G*G; }

	static FEmotio& As(EEmotio Arch)
	{
		static FEmotio archetypes[(int)EEmotio::MAX] =
		{
			{0.0f, 0.0f, 0.0f, 1.0f},		//Void				0.0			опустошенность
			{0.0f, 0.5f, 0.0f, 1.0f},		//Reverie			0.5	|		мечтательность, надежда
			{0.5f, 0.0f, 0.0f, 1.0f},		//Gloom				0.5	|		хмурость
			{0.0f, 0.0f, 0.5f, 1.0f},		//Worry				0.5	|		беспокойство

			{0.5f, 0.5f, 0.5f, 1.0f},		//Peace				0.0			уравновешенность, покой в бодрствовании, уравновешены все эмоции
			{0.0f, 0.5f, 0.5f, 1.0f},		//Hope				1.0	||		недежда на лучшее, но неуверенность
			{0.5f, 0.0f, 0.5f, 1.0f},		//Pessimism			1.0	||		пессимизм, мизантропия
			{0.5f, 0.5f, 0.0f, 1.0f},		//Carelessness		1.0	||		беспечность


			{1.0f, 0.5f, 0.5f, 1.0f},		//Enmity			1.0	||		враждебность, осознанная оппозиция с уважением и осторожностью
			{1.0f, 0.0f, 0.5f, 1.0f},		//Hatred			2.0	||||	ненависть, враждеьность с осторожностью, но без уважения
			{1.0f, 0.5f, 0.0f, 1.0f},		//Anger				2.0	||||	сердитость, обиженность, злоба на того, кто важен
			{1.0f, 0.0f, 0.0f, 1.0f},		//Fury				2.0	||||	бешенство, безудержная неприязнь

			{0.5f, 1.0f, 0.5f, 1.0f},		//Grace				1.0	||		благосклонность, осознанная поддержка с критичностью и осторожностью
			{0.0f, 1.0f, 0.5f, 1.0f},		//Love				2.0	||||	любовь, обожание с волнением, боязнь сделать хуже
			{0.5f, 1.0f, 0.0f, 1.0f},		//Patronage			2.0	||||	покровительство, пренебрежительное вазвышение
			{0.0f, 1.0f, 0.0f, 1.0f},		//Adoration			2.0	||||	обожание, без страха и критичности

			{0.5f, 0.5f, 1.0f, 1.0f},		//Fear				1.0	||		боязнь, осознанное принятие опасности
			{0.0f, 0.5f, 1.0f, 1.0f},		//Anxiety			2.0	||||	предчувствие дурного? или сокрушенность? или стыд? удивленность? или очень осторожное любопытство
			{0.5f, 0.0f, 1.0f, 1.0f},		//Phobia			2.0	||||	фобия, страх и ненависть
			{0.0f, 0.0f, 1.0f, 1.0f},		//Horror			2.0	||||	ужас


			{1.0f, 1.0f, 0.0f, 1.0f},		//Mania				2.0	||||	мания, одержимость
			{0.0f, 1.0f, 1.0f, 1.0f},		//Awe				2.0	||||	благоговение, трясучесть от страха и обожания, паника
			{1.0f, 0.0f, 1.0f, 1.0f},		//Disgust			2.0	||||	отвращение, активная неуемная ненависть, желание скрыться или уничтожить
			{1.0f, 1.0f, 1.0f, 1.0f}		//Insanity			0.0			безумие, 

		};
		return archetypes[(int)Arch];
	}
};









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

