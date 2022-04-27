#pragma once
#include "CoreMinimal.h"
#include "myrra_food.generated.h"


//��� - ��������, ��������� � ��������� ����

//###################################################################################################################
// ����� ������� ���� �� ��������������, ��� �� ����� ������
//###################################################################################################################
UENUM(BlueprintType) enum class EDigestiveTarget : uint8
{
	Health,
	Stamina,
	Energy,
	Psychedelic,
	NONE
};

//###################################################################################################################
// ������ ������ ������� ������ ����, ����� ������������ �� ��� � ����� � ��������� ������ �� ���������
//###################################################################################################################
USTRUCT(BlueprintType) struct FDigestivePotion
{
	GENERATED_BODY()

		//����� �������, ���������� ����� �������� ����������� ��� ���������
		UPROPERTY(EditAnywhere, BlueprintReadWrite) EDigestiveTarget Target;

	//������������ ����������� � ��������, ��������, ��� ����������� ������� ������ �������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Seconds = 1;

	//����� �������, ���� ������������ 1, �� �������� �������/����� � �������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Reserve = 0;

	//�������� �� �������� �� ���� �� ������ ����� ������ �������, ��� = �������� ��������, ���� ��� �������
	bool Transfer(FDigestivePotion& Other, float Part)
	{
		Reserve += Other.Reserve * Part;
		Seconds += Other.Seconds * Part;
		Other.Reserve *= 1 - Part;
		Other.Seconds *= 1 - Part;
		return (Other.Reserve <= 0.001);
	}

	//��������� ����� ������� ������� �� �������� �������������� (������������ ������� �����)
	bool SpendToParam(float* Param, float DeltaTime)
	{
		//���� ������� �������� �������/�����, �������� ���� ������ ��������� ������
		float Q = Reserve * DeltaTime / (float)Seconds;
		Reserve -= Q;

		//�������������� �������� ���� � ��������������� ������� - ��������, ������
		//����� ��� ������ ��������������, �������� � ������
		if (Param)
		{
			*Param += Q;
			if (*Param > 1) *Param -= Q;
			else if (*Param < 0) *Param += Q;
		}

		//���� ������ ����� ����� �������, �� ���� ����� �����������, �������� ����
		if (Reserve * Q <= 0) { Reserve = 0; return true; }
		else return false;
	}
};

//###################################################################################################################
//��������� �������� ������ ����, �������������, �������� ��� � ���, ��� � � �������
//###################################################################################################################
USTRUCT(BlueprintType) struct FDigestiveEffects
{
	GENERATED_BODY()

		//���� ������, ���� �� � ����� ��������, �� ��� ������� ���������
		UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FDigestivePotion> Effects;

	//���� ����� �������� ��������
	bool Empty() const { return (Effects.Num() == 0); }

	//�������� �� �������� ��������� ���� ���� ����� ��������, ��� = ��������, � ��������� ���� �������
	bool TransferSingle(FDigestivePotion& Src, float Part)
	{
		FDigestivePotion* Found = nullptr;
		for (auto& D : Effects)	if (D.Target == Src.Target) Found = &D;
		if (!Found) {
			Found = &Effects.AddDefaulted_GetRef();
			Found->Target = Src.Target;
		}
		return Found->Transfer(Src, Part);
	}

	//�������� �� (��� �������� �����) ���������� �� ��������� � ������� ��������
	bool Transfer(FDigestiveEffects* Other, float Part)
	{
		//� ����� ������� ������������
		for (int i = Other->Effects.Num() - 1; i >= 0; i--)
			if (TransferSingle(Other->Effects[i], Part))
				Effects.RemoveAt(i);
		return Empty();
	}
};