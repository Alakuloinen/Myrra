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
	Sleepiness,
	Pain,
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Reserve = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RateOfConsume = 0.0f;

	//������ ���������� ���������� �������� - ������ �� ����, ��� ��� ���������
	float& MaxReserve() { return RateOfConsume; }

	//��� ��� ������, �� �����������
	bool StillFull() const { return FMath::Abs(RateOfConsume) < FMath::Abs(Reserve); }

	//��������, ��� ��� ������ �����������
	void MarkAsBitten() { MaxReserve() = Reserve; }

	//��� ������, ��������� ������ ��� ����������� - ����������� ������ � ������� ����� ������ �����, ������ ������ ���� � "�����"
	bool Empty() { return (MaxReserve() * Reserve <= 0 );	}


	//�������� �� �������� �� ���� �� ������ ����� ������ �������, ��� = �������� ��������, ���� ��� �������
	bool Transfer(FDigestivePotion& Other, float NewPart)
	{
		//��������, ����� ����� ���� - ����� ����� �����, � ���������� RateOfConsume �������� ���������� ������ � �������
		//� �� � ������ ������������� �������; ��� ��� �������� ������ ������ ���� ������ �� ������, ��� ��������� �����
		//�� ����� ������� ���� � ��� ���������� �������� ������, ������������ �������� �������, � ����� ��� ���������� �����
		//������ ������, ��� ��� ������ (�� ������) - ���� ����� ��������� ������ � ������� ����� ���
		if (Other.StillFull())
		{
			//�������� ���������� ���� ���������� ���� ��� ������ ���� ���� ���
			RateOfConsume = FMath::Max(RateOfConsume, Other.RateOfConsume);

			//�������� ��� �����������, ������ ����� ��������� �� ��������������
			Other.MarkAsBitten();

			//���� �� ���� �����, ������ ��� ������������, � ����� ����� ���� ��������������� �� ����
			//����� ����������� �� ������ ���� ������������ ���� ���� ������ �������
			Reserve += FMath::Min(Other.Reserve, Other.Reserve * NewPart);
			Other.Reserve -= Other.Reserve * NewPart;

			//���� ������ ���� ������ ��� ����� �������, �� ��� ��� ��� ����� ����������
			return Other.Empty();
		}
		//�� ����� ������ ��� ������ �������� �����-�� �����
		else
		{
			//��� ���, �� �� ��� ����� ���� ������, ����������� ��������
			if (Other.Empty()) return true;

			//��� ������ ����������� �� ���������� ����, ��������� �� ����������� ������� ���� ���
			//������ ����� ���� ����� ��� �� ��������, � ���� ������ ��������� ���� ��, ��� ��������
			Reserve += FMath::Min(Other.MaxReserve() * NewPart, Other.Reserve);

			//�� ���� �� ����� ����������� ������ ���, ����� ����� � �����
			Other.Reserve -= Other.MaxReserve() * NewPart;
			return Other.Empty();
		}
	}

	//��������� ����� ������� ������� �� �������� �������������� (������������ ������� �����)
	bool SpendToParam(float* Param, float DeltaTime)
	{
		//����� ��� ������ ��������������, �������� � ������
		if (Param)
		{
			float Quant = FMath::Sign(Reserve) * RateOfConsume * DeltaTime;
			*Param += Quant;
			if (*Param > 1) *Param = 1;
			else if (*Param < 0) *Param = 0;
			Reserve -= Quant;
			return (Reserve * Quant < 0);
		}

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
				Other->Effects.RemoveAt(i);
		return Empty();
	}
};