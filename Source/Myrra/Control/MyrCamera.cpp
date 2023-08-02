// Fill out your copyright notice in the Description page of Project Settings.


#include "Control/MyrCamera.h"
#include "Control/MyrDaemon.h"
#include "Control/MyrPlayerController.h"
#include "Creature/MyrPhyCreature.h"

//� �� ������� ������
#define LOC(c) c->GetComponentLocation() 

//==============================================================================================================
//������� � ������� ������ ��������� �������� 
//==============================================================================================================
void UMyrCamera::AdoptEyeVisuals(const FEyeVisuals& EV)
{
	//�������������� ��������� � ��������� ������ � ���������� ���������� ���������
	PostProcessSettings = EV.OphtalmoEffects;
	FieldOfView = EV.FieldOfView;
	GNearClippingPlane = EV.NearClipPlane;

	//��������� ��������� ��� ������������
	//�� ������� �� �����, ��� ��� �� ��������, � ��� ������ ��� � ������� �� ����� ���� ������, 
	//�� ������ �� ��� "�������" ��������� �������� � ��������� PostProcessSettings � ������� WeightedBlendables
	if (PostProcessSettings.WeightedBlendables.Array.Num() > 0)
	{
		//����� "�������" ��������, ������� ������������ ������. ���������, ����� �� ��������� ������ 
		auto* mat = Cast<UMaterialInterface>(PostProcessSettings.WeightedBlendables.Array[0].Object);
		HealthReactionScreen = UMaterialInstanceDynamic::Create(mat, this);

		//�������� � ��� �� ����� ����������� �� ������������
		PostProcessSettings.WeightedBlendables.Array[0].Object = HealthReactionScreen;

		//����� �� ��������� ��������
		HealthReactionScreen->SetScalarParameterValue(TEXT("LowHealthAmount"), FMath::Min(1.0f - myBody()->Health, 1.0f));
		HealthReactionScreen->SetScalarParameterValue(TEXT("Psychedelic"), 0);
	}
}

//==============================================================================================================
//������� ������������� ���
//==============================================================================================================
void UMyrCamera::SetPerson(int P, USceneComponent* ExternalSeer)
{
	if (P == 33)
	{	Sight = me()->GetRootComponent();
		if(DistanceModifier > 0.1) HappenPerson(33);
		DistanceModifier = 1.0f;
		Seer = nullptr;
	}
	else if(P == 11)
	{ 
		DistanceModifier = 0.0f;
		Seer = nullptr;
	}
	else if (P == 44 && ExternalSeer)
	{
		Seer = ExternalSeer;
		if(!Sight) Sight = me()->GetRootComponent();
	}
}

//==============================================================================================================
//������� ������������ �������� � ������������� ���� - �������� ������
//==============================================================================================================
void UMyrCamera::HappenPerson(int P)
{
	if (P == 33)		Sight = me()->GetRootComponent();
	else if (P == 11)	Sight = nullptr;
	if (auto E = me()->GetEyeVisuals(P)) AdoptEyeVisuals(*E);
}

//==============================================================================================================
//��������� ���������� ������
//==============================================================================================================
void UMyrCamera::ProcessDistance(float DeltaTime, FVector ctrlDir)
{
	if (myCtrl()->IsNowUIPause()) return;

	//����������� � ���������� �� ������ �� �����, ����� ��������� �����
	FVector Dir = ctrlDir;
	float	DueDist = DistanceBasis * DistanceModifier;

	//�������, ��������, ��������� �������� ��� ������� ������, ������ � ����� - ��� ����� ��������� ������������� � ��������� �������� ������ 
	FVector SeerPos  = Seer  ? LOC(Seer)	: (Sight ? (LOC(Sight) - Dir * DueDist) : LOC(me()->GetRootComponent()));
	FVector SightPos = Sight ? LOC(Sight)	: SeerPos + Dir * DueDist;

	//��������������� ������� ����������� � ������ ������ 
	SeerPos += GetComponentTransform().GetUnitAxis(EAxis::Y)*SeerOffset/* + GetComponentTransform().GetUnitAxis(EAxis::Z) * SeerOffset.Y*/;
	SeerOffset = FMath::Lerp(SeerOffset, SeerOffsetTarget, 0.1);

	//������� ��������� ������ � ������ ��������� �� �����������
	FVector SeerPosDistort = SeerPos;

	//�������� �������� ������ � ������ �������
	float DollySpeed = DeltaTime;
	float OrbitSpeed = Seer ? 0.1f : 1.0f;


	//������� ������ ������ ����� ������ �� ������ �� ����
	FVector CurCamDir = SightPos - LOC(this);
	FVector NeededCamDir = SightPos - SeerPos;

	//����������� ������������ �������� ����� ������
	float CurCamDist2 = CurCamDir.SizeSquared();
	if ((CurCamDist2 < RadiusHard * RadiusHard) == (bool)Sight)
		HappenPerson((Sight == nullptr || DistanceModifier > 0) ? 33 : 11);

	//������� ���� ������, ��� ����� 3 ���� ��� �������������������
	if (Sight)
	{
		//�������� �������� �� ���������� � ��������� ������
		float Dist = DueDist;
		NeededCamDir.ToDirectionAndLength(Dir, Dist);

		//�������� ������� ����������� ����� - �� ���� ������� � ������ � ��� "�� �������" �� ������ ������ ����� �����������
		FVector OverStart = SightPos - Dir*(Dist + RadiusSoft);

		//����������� 
		FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("MyrCameraTrace")), false, me());
		RV_TraceParams.AddIgnoredActor(Sight->GetOwner());
		RV_TraceParams.AddIgnoredActor(myBody());
		FHitResult Hit(ForceInit);
		GetWorld()->LineTraceSingleByChannel(Hit, SightPos, OverStart, ECC_WorldStatic, RV_TraceParams);

		//���� ����������� �������� �����������
		if (Hit.Component.IsValid() && Hit.Distance > 0 && 
			Hit.Component->GetCollisionResponseToChannel(ECollisionChannel::ECC_Camera) != ECollisionResponse::ECR_Ignore)
		{
			//������� ������������� � ������ ����� ������ ������
			float Penetration = Hit.Distance - Dist;

			//����������� ����� �������� � ������� ����� ������ ������ � �������� ��� ���������� �����
			if (Penetration < RadiusHard)
			{
				//����������� ����� � ������� ����������, ��� � ������ - ���� ����� ��������
				auto ObjDim = Hit.Component->Bounds.GetBox().GetSize();
				float ObjSize = FMath::Max(ObjDim.X, ObjDim.Y);
				FVector ObjCenter = Hit.Component->Bounds.GetBox().GetCenter();
				FVector ObjRay = Hit.ImpactPoint - ObjCenter;
				float NoSideWay = ObjRay | Dir;
				if (Hit.Distance < Dist/2 && ObjSize < Dist / 2)
				{
					FVector DirProjNormalOnLookAt = ObjRay - Dir * NoSideWay;
					FVector PointDislocFromHit = Hit.ImpactPoint + (ObjSize + RadiusHard) * DirProjNormalOnLookAt.GetSafeNormal();
					FVector DirToDisloc = SightPos - PointDislocFromHit;
					SeerPosDistort = SightPos + 2 * DirToDisloc;
				}
				//����������� �������� �������, �������� ����� ���������� ������
				else
				{	
					//����� ������������, ��� ������ ����������
					SeerPosDistort = SightPos - Dir * (Hit.Distance - RadiusHard);
					if(Hit.Component->Mobility == EComponentMobility::Static)
						DollySpeed = 1;
					else DollySpeed = 10*DeltaTime;
				}
			}
			//����������� � "������ ����", ������ �� ����� ������ ����� �����
			else
			{
				//����� ������ ������������ �� ������� 
				SeerPosDistort = SightPos - Dir * (Hit.Distance - RadiusHard);
				DollySpeed = DeltaTime * Penetration / RadiusSoft;
			}
		}
		//����������� �� ���������� � ������� ��������, �������� ����������� ������ � ����� ������� 
		else
		{
			SeerPosDistort = SeerPos;
			DollySpeed = 0.1;
		}

		FVector WayToWish = SeerPosDistort - LOC(this);
		FVector WayCloseUp = Dir * (WayToWish | Dir);
		FVector WayAround = WayToWish - WayCloseUp;
		FVector NewCamPos = LOC(this) + (WayAround * OrbitSpeed) + (WayCloseUp * DollySpeed);

		//��������� ������� ��� ������ ����� ������ - ������ ������� � ������ �������
		//FVector NewCamPos = FMath::Lerp(LOC(this), SeerPosDistort, MatchingSpeed);
		Dir = (SightPos - NewCamPos).GetUnsafeNormal();
		auto Ro = FRotationMatrix::MakeFromXZ(Dir, FVector::UpVector).ToQuat();
		SetWorldLocationAndRotation(NewCamPos, Ro);

	}
	else SetRelativeLocationAndRotation(FVector(0, 0, 0), me()->GetControlRotation());

}

//==============================================================================================================
//��������� ����������� ����������� ��������
//==============================================================================================================
void UMyrCamera::SetFeelings(float Psycho, float Pain, float Dying)
{
	//���� � �����-����� ��� ��������� ���������� ��� ��� ��������, ����� ������� ����������� � ���������
	PostProcessSettings.ColorSaturation.W *= (myCtrl()->IsNowUIPause() || Dying > 0.0f)
		? 0.8f : (1.2 - PostProcessSettings.ColorSaturation.W);

	//������� �������
	HealthReactionScreen->SetScalarParameterValue(TEXT("Psychedelic"), FMath::Min(FMath::Max(Psycho, Dying), 1.0f));
	HealthReactionScreen->SetScalarParameterValue(TEXT("Desaturate"), FMath::Min(Dying, 1.0f));

	//������ �������
	Pain = FMath::Clamp(Pain - 0.9f, 0.0f, 1.0f);
	PostProcessSettings.SceneFringeIntensity = Pain;			//�������� ������� � ������ ��� ������ ����
	myCtrl()->GetCameraShake()->ShakeScale = Psycho;			//��������� ������, ��������� � �����������
	myCtrl()->GetPainCameraShake()->ShakeScale = Pain;			//������� ������, ��������� � �����


}

void UMyrCamera::SetFeelingsSlow(float Health, float Sleepy, float Psycho, float Dying)
{
	//�������� � �������� ������������ ������� ���-������� + ������� ���������� + ������� ��������������
	PostProcessSettings.MotionBlurAmount = me()->GetCurMotionBlur() + 20 * Dying + 10 * Psycho;
	PostProcessSettings.bOverride_MotionBlurAmount = (PostProcessSettings.MotionBlurAmount > 0.01);

	//����������, ��������� � ���������
	PostProcessSettings.ColorContrastMidtones.W = 1 - Sleepy / 2;	

	//���� �� ����� ��� �������
	HealthReactionScreen->SetScalarParameterValue(TEXT("LowHealthAmount"), FMath::Clamp(1.2f - 1.1f * Health, 0.0f, 1.0f));
}

//==============================================================================================================
//�����������
//==============================================================================================================
UMyrCamera::UMyrCamera(const FObjectInitializer& ObjectInitializer)
{
	bUsePawnControlRotation = false;
}

//��� ������� ����
void UMyrCamera::BeginPlay()
{
	Super::BeginPlay();

	//��������� ������ ������, ��� ���������� � ������������ �����������
	if (me()->Shake.Get())		myCtrl()->AddCameraShake(me()->Shake);
	if (me()->PainShake.Get())	myCtrl()->AddPainCameraShake(me()->PainShake);
	
	//�������������� ��������� ��������� = ��� ���������� ��������
	PostProcessSettings.bOverride_ColorContrastMidtones = true;

	//�������� ������� �������������� ������
	PostProcessSettings.bOverride_ColorSaturation = true;	

}
