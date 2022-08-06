#pragma once
#include "CoreMinimal.h"
#include "AdvMath.h"
#include "myrra_emotio.generated.h"
//###################################################################################################################
// ���� ������� ��������� �� ������, ��������� �� MyrLogic ������������ �������, �������� ���������� �� ��������
// ���� ���-�� ��������� ������������ ������������� ������� ���������� �� ����������
//###################################################################################################################
UENUM(BlueprintType) enum class EEmoCause : uint8
{
	ZERO,				// ������ ����, ����� ������ ���� ����� ��� ����� �� � ������

	VoiceOfRatio,		// ��� ������, ������������ ���������� ������ �� ������ Peace, ����� ���� ����, �� ������ ���������
	Burnout,			// ���������, ���������� ������ � ����, ����� ������ ��� ���

	LowStamina,			// ������������� ���������� ��������� (����� �� ���������������� �� ������� �������?)
	LowHealth,			// ������������� ������������� � �������� ������
	LowEnergy,			// ������������� ������������ �������

	DamagedCorpus,		// ������������� ������������� �������
	DamagedLeg,			// ������������� ������������ ����
	DamagedArm,			// ������������� ������������ ���� / �������� ����
	DamagedTail,		// ������������� ������������� ������
	DamagedHead,		// ������������� ������������ ������
	Pain,				// ������������� ��������� ������ ����

	Wetness,			// �������, ����� ������ ��/����� �����
	WeatherCloudy,		// �������, ����� ������ �������� ������
	WeatherSunny,		// �������, ����� ���� ������ ������
	WeatherFoggy,		// �������, ����� �������� ������
	TimeMorning,		// ������������� ������ �� ��������� �������
	TimeEvening,		// ������������� ������ �� ��������� �������
	TimeDay,			// ������������� ������ �� �������
	TimeNight,			// ������������� ������ �� �������
	Moon,				// ������������� ������ �� ����� ����
	TooCold,			// ������� �����������
	TooHot,				// ������� �����������

	//�������� ��� ���� ��������� ����� (��� � ���� ��, ���� �� �������� �� ����)

	ObjKnownNear,		// ������ ��� ��������,								�� ����������									�� ������ ������
	ObjKnownClose,		// ���� ������ ������								�� ����������									�� ������ ������
	ObjKnownComeClose,	// ���� ������������,								�� �������� ��������� � ����������				�� ������ ������
	ObjKnownFlyClose,	// �������� ���������								�� ���������� � �������� (� �������?)			�� ������ ������

	ObjBigComeClose,	// ������ �������, ������������						�� ������� ����������� � �������				����������, �� ������ �� ������������ ������
	ObjBigComeAway,		// ������ �������, ���������						�� ������� �������� � �������					����������, �� ������ �� ������������ ������

	ObjBigInRageComeClose,// ������ �������, ����, ������������				�� ������� ����������� � �������				����������, �� ������ �� ������������ ������
	ObjBigInRageComeAway,// ������ �������, ����, ���������					�� ������� � �������							����������, �� ������ �� ������������ ������
	ObjBigInRageClose,	// ������ �������, ����, ������ ������				�� ���������� � �������							����������, �� ������ �� ������������ ������
	ObjBigInRageNear,	// ������ �������, ����, ������						�� ���������� � �������							����������, �� ������ �� ������������ ������
	ObjSmallInRageClose,// ������ ������, ����, ������ ������				�� ����������, �������	� ������				����������, �� ������ �� ������������ ������

	ObjInLoveComeClose,	// ������ �����������, ������������					�� ����� � ��������								����������, �� ������ �� ������������ ������
	ObjInLoveComeAway,	// ������ �����������, ����������					�� ����� � ��������								����������, �� ������ �� ������������ ������
	ObjInLoveClose,		// ������ �����������, �����						�� ����� � ����������							����������, �� ������ �� ������������ ������
	ObjInLoveNear,		// ������ �����������, �����						�� ����� � ����������							����������, �� ������ �� ������������ ������

	ObjInFearComeAway,	// ������ � ������, ���������						�� ������� �������� � �������					����������, �� ������ �� ������������ ������
	ObjBigInFearComeClose,// ������ ������� � ������, ������������			�� �������, ������ � �������					����������, �� ������ �� ������������ ������

	ObjSmallClose,		// ������ ������, ������ ������						�� ����������									����������, �� ������ �� ������������ ������
	ObjSmallNear,		// ������ ������, ������, �� �� ��������			�� ����������									����������, �� ������ �� ������������ ������

	ObjSmallUnawareNear,// ������ ������, �� �������� ���, ������			�� ����������, �������, �����������				����������, �� ������ �� ������������ ������
	ObjSmallUnawareClose,// ������ ������, �� �������� ���, ��������		�� ����������, �������, �����������				����������, �� ������ �� ������������ ������
	ObjBigUnawareNear,	// ������ �������, �� �������� ���, ������			�� ����������, �������, �����������				����������, �� ������ �� ������������ ������
	ObjBigUnawareComeClose,	// ������ �������, �� �������� ���, �������		�� ��������, �������, �����������				����������, �� ������ �� ������������ ������

	ObjUnknownHeard,	// �������� ����� �� ����������� ��������			�� ���������� � �������������					����������, �� ������ �� ������������ ������
	ObjUnknownSeen,		// �������� ������� ���������� �������� -			�� ��������� � �������������					����������, �� ������ �� ������������ ������
	ObjUnknownFlyClose,	// ���������� �������� ���������					�� ���������� � �������� (� �������?)			����������, �� ������ �� ������������ ������

	ObjOfFearUneen,		// ������, �������� ������, ������� �� ����			�� ������ � ���������							����������, �� ������ �� ������������ ������
	ObjOfRageUneen,		// ������, �������� ���������, ������� �� ����		�� ������ � ���������							����������, �� ������ �� ������������ ������
	ObjOfLoveUneen,		// ������, �������� �����, ������� �� ����			�� ����� � ���������							����������, �� ������ �� ������������ ������

	ObjOfRageLost,		// ������, �������� ���������, ��� �������			�� ������ � ���������							����������, �� ������ �� ������������ ������
	ObjOfLoveLost,		// ������, �������� �����, ��� �������				�� ����� � ���������							����������, �� ������ �� ������������ ������

	//�������� ��� ���� �����

	SubjBigInRageHit,	// ������ �������, ����, ������ ���					�� �������, ����� � �����						����������, �� ������ �� ������������ ������
	SubjBigInFearHit,	// ������ �������, ��������, ������ ���				�� �������, ������ � �����						����������, �� ������ �� ������������ ������
	SubjBigInLoveHit,	// ������ �������, �������, ������ ���				�� �������, ����� � �����						����������, �� ������ �� ������������ ������
	SubjSmallInRageHit,	// ������ ������, ����, ������ ���					�� �������, ����� � �����						����������, �� ������ �� ������������ ������
	SubjSmallInFearHit,	// ������ ������, ��������, ������ ���				�� �������, ������ � �����						����������, �� ������ �� ������������ ������
	SubjSmallInLoveHit,	// ������ ������, �������, ������ ���				�� �������, ����� � �����						����������, �� ������ �� ������������ ������

	SubjOfLoveHit,		// ������ ������� ����, ������ ���					�� ����� ����� � �����							����������, �� ������ �� ������������ ������
	SubjOfFearHit,		// ������ �������� ������, ������ ���				�� ������ ������ � �����						����������, �� ������ �� ������������ ������
	SubjOfRageHit,		// ������ �� �������� ������, ������ ���			�� ������ ����� � �����							����������, �� ������ �� ������������ ������

	//�������� � �������� ����� ����� 

	SubjOfLoveMishit,	// ������ ������� ����, �����������					�� ����� ����� � 								����������, �� ������ �� ������������ ������
	SubjOfFearMishit,	// ������ �������� ������, �����������				�� ������ ������ � 								����������, �� ������ �� ������������ ������
	SubjOfRageMishit,	// ������ �� �������� ������, �����������			�� ������ ����� � 								����������, �� ������ �� ������������ ������

	SubjOfRageAttack,	// ������ �������� ���������, ����� �����			�� ������ ����� � ���������������� ���������	����������, �� ������ �� ������������ ������
	SubjOfFearAttack,	// ������ �������� ������, ����� �����				�� ������ ������ � ���������������� ���������	����������, �� ������ �� ������������ ������
	SubjOfLoveAttack,	// ������ �������� �����, ����� �����				�� ����� ����� � ���������������� ���������		����������, �� ������ �� ������������ ������

	//�������������

	SubjHitObjOfLove,	// ������ ������ ����, ���� �� �����				// �� ����� �����, ��������� � �����
	ObjOfLoveHit,		// ������, ���� �� �����, ������� ����				// �� ����� �����, ��������� � �����
	SubjHitObjOfRage,	// ������ ������ ����, �� ���� �� � ������			// �� ����� ������, ��������� � �����
	SubjHitObjOfFear,	// ������ ������ ����, ���� �� ������				// �� ������ ������, ��������� � �����

	MeHurtFirst,		//� �������� ������ ���� ����-��
	MeHurtAgain,		//� ��� ������� ��� �������� ������� ���������
	MeHitFirst,			//� ��������� ������ ���� ���� ������ ���
	MeHitAgain,			//� ��������� ������ ���� ���� ������� ���
	MeMishitFirst,		//� ����������� ����������
	MeMishitAgain,		//� ����������� ��� ������� ���
	MeDidGrace,			//� ����� ���������������� ��������
	MeExpressed,		//� �������� ������������-��������� (������ �������� �� ���� - ������ ������������� ������)

	// ����������� ��� ��������

	MeAteHealthPlus,	//���� ����������������� ��������
	MeAteStaminaPlus,	//���� ����������������� ����� ���
	MeAteHealthMinus,	//���� �������� ��������
	MeAteStaminaMinus,	//���� �������� ����� ���

	MeJumped,			//������ �������			// �� ���� ������

	MeDidQuestStep,		//� ������ �����-�� ������� ������ (������ ������� � ���������� ������)
	MeDidAnyQuestStep,	//� ������ �����-�� ������� ������ - ����� ������ ��� ���� �������

	NONE				// ��������, ��� ���������� ����������� ���������
};


//###################################################################################################################
//������ ������, ���������, ������� ��������� ��������, ����� � ����� � ������������ �������� ������ �����, ����� ��������
//###################################################################################################################
USTRUCT(BlueprintType) struct MYRRA_API FEmoStimulus
{
	GENERATED_USTRUCT_BODY()

	//������, � ������� ������� ���������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 R;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 G;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 B;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 A;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255")) uint8 Aftershocks;

	//������� ����������: ������(�������), ������(�������), �����(�����) - 
	float Rage()		const { return (float)R / 255.0f; }		//���� ������������
	float Love()		const { return (float)G / 255.0f; }		//���� ������������ 
	float Fear()		const { return (float)B / 255.0f; }		//���� ���������� 
	float Sure()		const { return (float)A / 255.0f; }

	//��� ���� - ��������, ������ ������ �������� ���������, ��� ��� ������� ��������� ������, ���������� �������������
	FColor ToFColor() const { return FColor(R,G,B,A); }

	//������ ����� ����� � ����� ���-�� ���������
	bool Valid() const { return A > 0 && Aftershocks > 0;  }

	//�������� ������� �������������
	void Attenuate(float Coef)
	{ 	uint8 Reduct = ((float)A * Coef);
		if (Reduct > 0) A = Reduct; else Aftershocks = ((float)Aftershocks * Coef);
	}

	FEmoStimulus() :R(0), G(0), B(0), A(0), Aftershocks(0) {}
	FEmoStimulus(uint8 r, uint8 g, uint8 b, uint8 a = 0, uint8 s = 0) :R(r), G(g), B(b), A(a), Aftershocks(s) {}
	FEmoStimulus(FLinearColor C, uint8 strength, uint8 duration) :R(C.R * 255), G(C.G * 255), B(C.B * 255),  A(strength), Aftershocks(duration) {}

};

//###################################################################################################################
//��������� ������ � ������ ������ � ������
//###################################################################################################################
USTRUCT(BlueprintType) struct MYRRA_API FEmoEntry
{
	GENERATED_USTRUCT_BODY()

	//������� ������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EEmoCause Cause;

	//������, ���������� �� ������� � ����� ����������, ���������������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEmoStimulus Stimulus;

	//������, ������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TWeakObjectPtr<UObject> Whence;
	FEmoEntry(EEmoCause C, FEmoStimulus S, UObject* W) : Cause(C), Stimulus(S), Whence(W) {}
	FEmoEntry() : Cause(EEmoCause::NONE), Stimulus(), Whence() {}
};


//###################################################################################################################
//������� ������ ������������ ��������
//###################################################################################################################
UENUM(BlueprintType) enum class EEmotio : uint8
{
	Void,			//��������������
	Reverie,		//��������������, �������
	Gloom,			//��������
	Worry,			//������������

	Peace,			//����������������, ����� � �������������, ������������ ��� ������
	Hope,			//������� �� ������, �� �������������
	Pessimism,		//���������, �����������
	Carelessness,	//�����������


	Enmity,			//������������, ���������� ��������� � ��������� � �������������
	Hatred,			//���������, ������������ � �������������, �� ��� ��������
	Anger,			//����������, �����������, ����� �� ����, ��� �����
	Fury,			//���������, ����������� ���������

	Grace,			//���������������, ���������� ��������� � ������������ � �������������
	Love,			//������, �������� � ���������, ������ ������� ����
	Pride,			//��������, ���������������, ����������������� ����������
	Adoration,		//��������, ��� ������ � �����������

	Fear,			//������, ���������� �������� ���������
	Anxiety,		//������������ �������? ��� �������������? ��� ����? ������������? ��� ����� ���������� �����������
	Phobia,			//�����, ����� � ���������
	Horror,			//����


	Mania,			//�����, �����������
	Awe,			//������������, ���������� �� ������ � ��������, ������
	Disgust,		//����������, �������� �������� ���������, ������� �������� ��� ����������
	Insanity,		//�������, 
	MAX
};

//###################################################################################################################
// ������� ����������� ������
//###################################################################################################################
USTRUCT(BlueprintType) struct MYRRA_API FEmotio
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0")) float R;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0")) float G;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0")) float B;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0")) float A;

	//������� ����������: ������(�������), ������(�������), �����(�����) - 
	float Rage()		const { return R; }		//���� ������������
	float Love()		const { return G; }		//���� ������������ 
	float Fear()		const { return B; }		//���� ���������� 

	float &Resource()	{ return A; }			//����� ������������� ���, �������� ����������, ��������� ���

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
			{0.0f, 0.0f, 0.0f, 1.0f},		//Void				0.0			��������������
			{0.0f, 0.5f, 0.0f, 1.0f},		//Reverie			0.5	|		��������������, �������
			{0.5f, 0.0f, 0.0f, 1.0f},		//Gloom				0.5	|		��������
			{0.0f, 0.0f, 0.5f, 1.0f},		//Worry				0.5	|		������������

			{0.5f, 0.5f, 0.5f, 1.0f},		//Peace				0.0			����������������, ����� � �������������, ������������ ��� ������
			{0.0f, 0.5f, 0.5f, 1.0f},		//Hope				1.0	||		������� �� ������, �� �������������
			{0.5f, 0.0f, 0.5f, 1.0f},		//Pessimism			1.0	||		���������, �����������
			{0.5f, 0.5f, 0.0f, 1.0f},		//Carelessness		1.0	||		�����������


			{1.0f, 0.5f, 0.5f, 1.0f},		//Enmity			1.0	||		������������, ���������� ��������� � ��������� � �������������
			{1.0f, 0.0f, 0.5f, 1.0f},		//Hatred			2.0	||||	���������, ������������ � �������������, �� ��� ��������
			{1.0f, 0.5f, 0.0f, 1.0f},		//Anger				2.0	||||	����������, �����������, ����� �� ����, ��� �����
			{1.0f, 0.0f, 0.0f, 1.0f},		//Fury				2.0	||||	���������, ����������� ���������

			{0.5f, 1.0f, 0.5f, 1.0f},		//Grace				1.0	||		���������������, ���������� ��������� � ������������ � �������������
			{0.0f, 1.0f, 0.5f, 1.0f},		//Love				2.0	||||	������, �������� � ���������, ������ ������� ����
			{0.5f, 1.0f, 0.0f, 1.0f},		//Pride				2.0	||||	��������, ���������������, ����������������� ����������
			{0.0f, 1.0f, 0.0f, 1.0f},		//Adoration			2.0	||||	��������, ��� ������ � �����������

			{0.5f, 0.5f, 1.0f, 1.0f},		//Fear				1.0	||		������, ���������� �������� ���������
			{0.0f, 0.5f, 1.0f, 1.0f},		//Anxiety			2.0	||||	������������ �������? ��� �������������? ��� ����? ������������? ��� ����� ���������� �����������
			{0.5f, 0.0f, 1.0f, 1.0f},		//Phobia			2.0	||||	�����, ����� � ���������
			{0.0f, 0.0f, 1.0f, 1.0f},		//Horror			2.0	||||	����


			{1.0f, 1.0f, 0.0f, 1.0f},		//Mania				2.0	||||	�����, �����������
			{0.0f, 1.0f, 1.0f, 1.0f},		//Awe				2.0	||||	������������, ���������� �� ������ � ��������, ������
			{1.0f, 0.0f, 1.0f, 1.0f},		//Disgust			2.0	||||	����������, �������� �������� ���������, ������� �������� ��� ����������
			{1.0f, 1.0f, 1.0f, 1.0f}		//Insanity			0.0			�������, 

		};
		return archetypes[(int)Arch];
	}
};


//###################################################################################################################
// ������ ������������ ������������� ������ � ������������� �������
// ��� ������� ����������� ����� ��� ��� ������� � ����� ����������� ���� �������
//###################################################################################################################
USTRUCT(BlueprintType) struct MYRRA_API FEmoReactionList
{
	GENERATED_USTRUCT_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<EEmoCause, FEmoStimulus> Map;

};

//###################################################################################################################
//�������� ������
// ��������, ���� �� ������� ����� �������, � ������ �����-���������
// //� ������ �������, ������� ����� ������������ ����������, � ������ ����� ������ �������
//###################################################################################################################
USTRUCT(BlueprintType) struct MYRRA_API FEmoMemory
{
	GENERATED_USTRUCT_BODY()

	//�������������� ������� ������ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEmotio Emotion;

	//���� �������� ��������� �������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FEmoEntry> Factors;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//������ ���� ��
	void Tick()
	{
		//���������� �������� ���, ��� � ����� �������, ���� �� ����������� 
		if (Factors.Num() > 0)
			if (Factors[Factors.Num() - 1].Stimulus.Aftershocks == 0)
				Factors.RemoveAt(Factors.Num() - 1);

		//����� ��������� ������ � ����������� ����������
		for (int i = Factors.Num() - 1; i >= 0; i--)
			if (Factors[i].Stimulus.Aftershocks > 0)
				Emotion.ApplyStimulus(Factors[i].Stimulus);

		//���������, ��� ��������� ��������� ���� ������
		//��������, ��� ����� ������ Sleepiness, � ������ ����������������� ����
		Emotion.Resource() -= 0.0001 * (Emotion.PowerAxial());

	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//������ ����� �������� �� ������
	//��������, ����� �� ������ ������
	bool AddEmoFactor(EEmoCause Why, UObject* Whence, FEmoStimulus Stimulus = FEmoStimulus())
	{
		//��������� ��� ��������������
		int ProperPlace = -1;
		for (int i = Factors.Num() - 1; i >= 0; i--)
		{
			//��������� � ������ ��������� ����� �� ����� ������������� ���� �������
			if (Factors[i].Stimulus.Aftershocks == 0 && ProperPlace < 0)
				ProperPlace = i;

			//���� ���� � �� �� ������� � ������ ������� = �� ������� �����, � �������� ������
			else if (Factors[i].Cause == Why && Factors[i].Whence == Whence)
			{	Factors[i].Stimulus.Aftershocks = FMath::Min(255, Factors[i].Stimulus.Aftershocks + Stimulus.Aftershocks);
				return false;
			}
		}

		//���� �� ����� �����, ��������� � �����
		if (ProperPlace < 0)
			Factors.Add(FEmoEntry(Why, Stimulus, Whence));
		else Factors[ProperPlace] = FEmoEntry(Why, Stimulus, Whence);
		return true;

	}
};