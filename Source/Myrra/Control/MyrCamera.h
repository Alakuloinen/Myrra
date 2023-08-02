// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "Myrra.h"
#include "MyrCamera.generated.h"

//����� ��������� ������, ���������� ����� ������� ��������, ����������� � �.�.
UENUM(BlueprintType) enum class EWhoSees : uint8
{
	NONE,					//0000

	Creature = 5,			//01 01
	CreatureToPlayer = 6,	//01 10
	CreatureToWatcher = 7,	//01 11

	PlayerToCreature = 9,	//10 01
	Player = 10,			//10 10
	PlayerToWatcher = 11,	//10 11

	WatcherToCreature = 13,	//11 01
	WatcherToPlayer = 14,	//11 10
	Watcher = 15,			//11 11
};


//###################################################################################################################
// ����� ������ � ������
// 1 - ���������������� (�������� �� ������� ���)
// 2 - ���� �����������
// 3 - ����������� ���������� (��������)
// 4 - ����-������� (������� ����� � ����)
//###################################################################################################################
UCLASS() class MYRRA_API UMyrCamera : public UCameraComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)		USceneComponent* Seer = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		USceneComponent* Sight = nullptr;

	//����� ���������� ������ � �������� �����������, ������������ ��� ����, ��� ��������� ����� ����������� ��������
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float DistanceBasis = 150;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float DistanceModifier = 1.0f;

	//������ ������ ������ ������ �� 3 ����, ������ �������� ������ ��������� �� ��������� ���������
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float RadiusHard = 10;

	//������ ����� ������ ������ 3 ����, �� ������� ���������� ��������� � ������������ �� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float RadiusSoft = 100;

	//���������� ����� ���������� �� ������������� ������� � ��������� ���������������� ����� �������
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float SeerOffsetTarget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		float SeerOffset;

public: 

	//����-������� �������� ��� ������ ��������� ��������, �������� �� �������� ������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	UMaterialInstanceDynamic* HealthReactionScreen = nullptr;

	//������ ������ - ��� �� ������, ��� � ��������� ��������� �����
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		TSubclassOf<UCameraShakeBase> Shake;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)		TSubclassOf<UCameraShakeBase> PainShake;

public:

	//������� � ������� ������ ��������� �������� 
	void AdoptEyeVisuals(const FEyeVisuals& EV);

	//������� ������������� ���
	void SetPerson(int P, USceneComponent* ExternalSeer = nullptr);
	void HappenPerson(int P);

	//������ ��������� ����������������, � ������������, ������ ����
	void ProcessDistance(float DeltaTime, FVector ctrlDir);

	//��������� ����������� ����������� ��������
	void SetFeelings(float Psycho, float Pain, float Dying);
	void SetFeelingsSlow(float Health, float Sleepy, float Psycho, float Dying);

public:

	class AMyrDaemon*				me()			{ return (AMyrDaemon*)GetOwner(); }
	class AMyrDaemon*				me() const		{ return const_cast<UMyrCamera*>(this)->me(); }
	class AMyrPlayerController*		myCtrl()		{ return (AMyrPlayerController*)(((APawn*)GetOwner())->GetController()); }
	class AMyrPlayerController*		myCtrl() const	{ return const_cast<UMyrCamera*>(this)->myCtrl(); }
	class AMyrPhyCreature*			myBody()		{ return (AMyrPhyCreature*)GetOwner()->GetAttachParentActor(); }
	class AMyrPhyCreature*			myBody() const	{ return const_cast<UMyrCamera*>(this)->myBody(); }

	int GetPerson() const
	{ 	if (DistanceModifier == 0)
		{	if (!Sight) return 11;		//���������� ������ ����
			else if (Seer) return 41;	//������� � ���������� (�����������) �� ������
			else return 31;				//������� � �������� �� ������
		}else
		{	if (Seer) return 44; else	//���������� ���������
			if (Sight) return 33; else	//���������� ������
			return 13;						
		}
	}


//����������� ������ � ���������������
public:

	//�����������
	UMyrCamera(const FObjectInitializer& ObjectInitializer);

	//��� ������� ����
	virtual void BeginPlay() override;

};
