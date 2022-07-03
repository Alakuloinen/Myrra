#pragma once
#include "CoreMinimal.h"
#include "AdvMath.h"
#include "myrra_emotio.generated.h"
//###################################################################################################################
// суть стимула влияющего на эмоцию, отдельное от MyrLogic пространство событий, напрямую завязанное на чувствах
// надо как-то подбирать элементарные эмоциональные события отвязанные от реальности
//###################################################################################################################
UENUM(BlueprintType) enum class EEmoCause : uint8
{
	ZERO,				// просто ноль, чтобы список этой хуйни был похож на С строку

	VoiceOfRatio,		// зов разума, способствует смазыванию эмоций до уровня Peace, когда есть силы, но эмоции расшатаны
	Burnout,			// выгорание, стремление эмоций к нулю, когда совсем нет сил

	LowStamina,			// психосоматика физической усталости (стоит ли распараллеливать по текущим эмоциям?)
	LowHealth,			// психосоматика болезненности и близости смерти
	LowEnergy,			// психосоматика истощенности голодом

	DamagedCorpus,		// психосоматика поврежденного корпуса
	DamagedLeg,			// психосоматика поврежденной ноги
	DamagedArm,			// психосоматика поврежденной руки / передней ноги
	DamagedTail,		// психосоматика поврежденного хвоста
	DamagedHead,		// психосоматика поврежденной головы
	Pain,				// психосоматика внезапной резкой боли

	Wetness,			// чувство, когда моткро от/после дождя
	WeatherCloudy,		// чувство, когда совсем облачная погода
	WeatherSunny,		// чувство, когда ярко светит солнце
	WeatherFoggy,		// чувство, когда туманная погода
	TimeMorning,		// эмоциональный отклик от утреннего времени
	TimeEvening,		// эмоциональный отклик от вечернего времени
	TimeDay,			// эмоциональный отклик от времени
	TimeNight,			// эмоциональный отклик от времени
	Moon,				// эмоциональный отклик от яркой луны
	TooCold,			// влияние температуры
	TooHot,				// влияние температуры

	HearYouUnknown,		// услышали звуки от незнакомого существа
	SeeYouUnknown,		// внезапно увидели незнакомое существо

	GoalBig,			// цель в досяшаемости больше нас, какие это вызывает чувства
	GoalSmall,			// цель в досягаемости меньше нас, какие это вызывает чувства
	GoalFlies,			// цель в досягаемости летающая, какие это вызывает чувства

	GoalAttackFirst,	// цель в досягаемости уже ранее атаковала нас опасной атакой
	GoalAttackAgain,	// цель в досягаемости уже ранее атаковала нас опасной атакой
	GoalHurtFirst,		// цель в досягаемости уже ранее атаковала нас с болезненными последствиями
	GoalHurtAgain,		// цель в досягаемости уже ранее атаковала нас с болезненными последствиями
	GoalGraceFirst,		// цель в досягаемости уже ранее атаковала нас позитивным действием
	GoalGraceAgain,		// цель в досягаемости уже ранее атаковала нас позитивным действием

	GoalRageComingAway,	// враждебная цель изменила движение с приближения на избегание
	GoalRageComingClose,// враждебная цель изменила движение с избегания на приближение
	GoalFearComingAway,	// страшная цель изменила движение с приближения на избегание
	GoalFearComingClose,// страшная цель изменила движение с избегания на приближение
	GoalLoveComingAway,	// любимая цель изменила движение с приближения на избегание
	GoalLoveComingClose,// любимая цель изменила движение с избегания на приближение

	MeHurtFirst,		//я случайно сделал урон кому-то
	MeHurtAgain,		//я уже который раз нечаянно увечнул невинного
	MeHitFirst,			//я намеренно сделал урон цели первый раз
	MeHitAgain,			//я намеренно сделал урон цели который раз
	MeMishitFirst,		//я промахнулся неожиданно
	MeMishitAgain,		//я промахнулся уже который раз
	MeDidGrace,			//я начал доброжелательное действие
	MeExpressed,		//я совершил самодействие-выражение (эмоция задается из него - способ самокоррекции эмоции)

	MeAteHealthPlus,	//я съел еду восстанавливающую здоровье
	MeAteStaminaPlus,	//я съел еду восстанавливающую запас сил
	MeAteHealthMinus,	//я съел еду крадущую здоровье
	MeAteStaminaMinus,	//я съел еду крадущую запас сил

	MeJumped,			//просто прыгнул

	MeDidQuestStep,		//я сделал какую-то ступень квеста (стимул задаётся в параметрах квеста)
	MeDidAnyQuestStep,	//я сделал какую-то ступень квеста - общий стимул для всех квестов

	NONE				// максимум, или количество осмысленных вариантов
};


//###################################################################################################################
//стимул эмоции, слагаемое, которое заносится событием, сидит в стеке и воздействует наэмоцию нужное время, потом исчезает
//###################################################################################################################
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
	float Rage()		const { return (float)R / 255.0f; }		//тяга приблизиться
	float Love()		const { return (float)G / 255.0f; }		//тяга приблизиться 
	float Fear()		const { return (float)B / 255.0f; }		//тяга отдалиться 
	float Sure()		const { return (float)A / 255.0f; }

	//как цвет - внимание, нельзя просто выдавать указатель, так как порядок компонент другой, приходится пересоздавать
	FColor ToFColor() const { return FColor(R,G,B,A); }

	//стимул имеет смысл и может что-то привнести
	bool Valid() const { return A > 0 && Aftershocks > 0;  }

	//ослабить внешним коэффициентом
	void Attenuate(float Coef)
	{ 	uint8 Reduct = ((float)A * Coef);
		if (Reduct > 0) A = Reduct; else Aftershocks = ((float)Aftershocks * Coef);
	}

	FEmoStimulus() :R(0), G(0), B(0), A(0), Aftershocks(0) {}
	FEmoStimulus(uint8 r, uint8 g, uint8 b, uint8 a = 0, uint8 s = 0) :R(r), G(g), B(b), A(a), Aftershocks(s) {}
	FEmoStimulus(FLinearColor C, uint8 strength, uint8 duration) :R(C.R * 255), G(C.G * 255), B(C.B * 255),  A(strength), Aftershocks(duration) {}

};

//###################################################################################################################
//одинарная запись в памяти эмоций с сылкой
//###################################################################################################################
USTRUCT(BlueprintType) struct MYRRA_API FEmoEntry
{
	GENERATED_USTRUCT_BODY()

	//причина эмоции
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EEmoCause Cause;

	//стимул, копируется из таблицы и далее зименяется, декрементируясь
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEmoStimulus Stimulus;

	//объект, выслав
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TWeakObjectPtr<UObject> Whence;
	FEmoEntry(EEmoCause C, FEmoStimulus S, UObject* W) : Cause(C), Stimulus(S), Whence(W) {}
	FEmoEntry() : Cause(EEmoCause::NONE), Stimulus(), Whence() {}
};


//###################################################################################################################
//опорные эмоции мнемонически понятные
//###################################################################################################################
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
	Pride,			//гордость, покровительство, пренебрежительное вазвышение
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

//###################################################################################################################
// текущая оперативная эмоция
//###################################################################################################################
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

	float &Resource()	{ return A; }			//запас эмоциональных сил, иссякает постепенно, придумать как

	float PowerSquared()const { return R * R + B * B + G * G; }
	float PowerAxial()	const { return FMath::Abs(R) + FMath::Abs(G) + FMath::Abs(B); }
	float Discordance() const { return FMath::Abs(R - G) + FMath::Abs(G - B) + FMath::Abs(R - B); }

	FLinearColor ToFLinearColor() { return *((FLinearColor*)this); }
	FVector4 ToFVector4() { return *((FVector4*)this); }
	FVector ToFVector() const { return *((FVector*)this); }
	FColor ToFColor() { return FColor(R * 255, G * 255, B * 255, A * 255); }

	FEmotio() : R(0), G(0), B(0), A(1) {}
	FEmotio(FColor O) : R((float)O.R / 255.0), G((float)O.G / 255.0), B((float)O.B / 255.0), A((float)O.A / 255.0) {}
	FEmotio(FLinearColor O) : R(O.R), G(O.G), B(O.B), A(O.A) {}
	FEmotio(float All) : R(All), G(All), B(All), A(1) {}
	FEmotio(float nRage, float nLove, float nFear, float Sure = 1) : R(nRage), G(nLove), B(nFear), A(Sure) {}
	FEmotio(FEmoStimulus O) : R((float)O.R / 255.0), G((float)O.G / 255.0), B((float)O.B / 255.0), A((float)O.A / 255.0) {}

	FEmotio operator*	(float M) { return FEmotio(R * M, G * M, B * M, A); }
	FEmotio Dim(float M) { if (M <= 1 && M >= 0) return (*this) * M; else return *this; }
	FEmotio StepTo(FEmotio New, float Step, bool SureToo = false) { return FEmotio(::StepTo(R, New.R, Step), ::StepTo(G, New.G, Step), ::StepTo(B, New.B, Step), SureToo ? (::StepTo(A, New.A, Step)) : A); }
	FEmotio Combine(FEmotio N, bool SureToo = false) { return FEmotio(FMath::Lerp(R, N.R, N.A), FMath::Lerp(G, N.G, N.A), FMath::Lerp(B, N.B, N.A), SureToo ? FMath::Lerp(A, N.A, N.A) : A); }
	FEmotio Mix(FEmotio N, float E, bool SureToo = false) { return FEmotio(FMath::Lerp(R, N.R, E), FMath::Lerp(G, N.G, E), FMath::Lerp(B, N.B, E), SureToo ? FMath::Lerp(A, N.A, E) : A); }
	
	float GetArch(EEmotio* Closest, EEmotio* SecondClosest = nullptr) const 
	{	int RightArch = 0, SecondArch = 0;
		float RightDist = 10;
		for (int i = 0; i < (int)EEmotio::MAX; i++)
		{
			float NuDist = DistAxial3D(FEmotio::As((EEmotio)i));
			if (NuDist < RightDist)
			{
				SecondArch = RightArch;
				RightDist = NuDist;
				RightArch = i;
			}
		}
		if (Closest) *Closest = (EEmotio)RightArch;
		if (SecondClosest) *SecondClosest = (EEmotio)SecondArch;
		return RightDist;
	}

	bool ApplyStimulus(FEmoStimulus& S)
	{	if (S.Aftershocks == 0) return false;
		float strength = S.Sure();
		R = FMath::Lerp(R, S.Rage(), strength);
		G = FMath::Lerp(G, S.Love(), strength);
		B = FMath::Lerp(B, S.Fear(), strength);
		S.Aftershocks = FMath::Max(S.Aftershocks - 1, 0);
		return (S.Aftershocks != 0);
	}

	float DistSquared3D(FEmotio& O) const { return FVector::DistSquared(ToFVector(), O.ToFVector()); }
	float Dist3D(FEmotio& O)		const { return FVector::Dist(ToFVector(), O.ToFVector()); }
	float DistAxial3D(FEmotio& O)	const { return FMath::Abs(R - O.R) + FMath::Abs(G - O.G) + FMath::Abs(B - O.B); }

	float PrimaryMotionStimulus() const { return R + G - 2 * B * B; }
	float PrimaryStealthStimulus() const { return 2 * (B - FMath::Square(B * B)) - R - G * G; }

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
			{0.5f, 1.0f, 0.0f, 1.0f},		//Pride				2.0	||||	гордость, покровительство, пренебрежительное вазвышение
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


//###################################################################################################################
// список соответствий эмоциональных причин и эмоциональных реакций
// как минимум протагонист носит его как таблицу и может прокачивать свои реакции
//###################################################################################################################
USTRUCT(BlueprintType) struct MYRRA_API FEmoReactionList
{
	GENERATED_USTRUCT_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<EEmoCause, FEmoStimulus> Map;

};

//###################################################################################################################
//механизм памяти
// возможно, ваще не хранить здесь стимулы, а только мнемо-константы
// //с другой стороны, стимулы можно генерировать процедурно, а список вести только опорных
//###################################################################################################################
USTRUCT(BlueprintType) struct MYRRA_API FEmoMemory
{
	GENERATED_USTRUCT_BODY()

	//результирующая текущая эмоция 
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEmotio Emotion;

	//стек факторов влиятелей текущих
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FEmoEntry> Factors;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//каждый такт ИИ
	void Tick()
	{
		//оказионное удаление тех, кто в конце массива, чтоб не передвигать 
		if (Factors.Num() > 0)
			if (Factors[Factors.Num() - 1].Stimulus.Aftershocks == 0)
				Factors.RemoveAt(Factors.Num() - 1);

		//обход рабранных стимул и постепенное исчерпание
		for (int i = Factors.Num() - 1; i >= 0; i--)
			if (Factors[i].Stimulus.Aftershocks > 0)
				Emotion.ApplyStimulus(Factors[i].Stimulus);

		//придумать, как правильно уменьшать этот ресурс
		//возможно, это будет вместо Sleepiness, и эмоции восстанавливаются сном
		Emotion.Resource() -= 0.0001 * (Emotion.PowerAxial());

	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//внести новый влиятель на эмоцию
	//возможно, здесь же делать фильтр
	bool AddEmoFactor(EEmoCause Why, UObject* Whence, FEmoStimulus Stimulus = FEmoStimulus())
	{
		//перебрать все присутствующие
		int ProperPlace = -1;
		for (int i = Factors.Num() - 1; i >= 0; i--)
		{
			//поместить в первую доступную дырку на месте отработавшего свое стимула
			if (Factors[i].Stimulus.Aftershocks == 0 && ProperPlace < 0)
				ProperPlace = i;

			//если одна и та же причина и объект виовник = не создаем новый, а изменяем старый
			else if (Factors[i].Cause == Why && Factors[i].Whence == Whence)
			{	Factors[i].Stimulus.Aftershocks = FMath::Min(255, Factors[i].Stimulus.Aftershocks + Stimulus.Aftershocks);
				return false;
			}
		}

		//если не нашли дырку, добавляем в конец
		if (ProperPlace < 0)
			Factors.Add(FEmoEntry(Why, Stimulus, Whence));
		else Factors[ProperPlace] = FEmoEntry(Why, Stimulus, Whence);
		return true;

	}
};