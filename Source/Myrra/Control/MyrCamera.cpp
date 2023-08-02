// Fill out your copyright notice in the Description page of Project Settings.


#include "Control/MyrCamera.h"
#include "Control/MyrDaemon.h"
#include "Control/MyrPlayerController.h"
#include "Creature/MyrPhyCreature.h"

//а то слишком длинно
#define LOC(c) c->GetComponentLocation() 

//==============================================================================================================
//загнать в текущую камеру настройки эффектов 
//==============================================================================================================
void UMyrCamera::AdoptEyeVisuals(const FEyeVisuals& EV)
{
	//переприсвоение структуры с эффектами камеры и глобальный расстояний отсечения
	PostProcessSettings = EV.OphtalmoEffects;
	FieldOfView = EV.FieldOfView;
	GNearClippingPlane = EV.NearClipPlane;

	//установка материала для постэффектов
	//мы заранее не знаем, что это за материал, и для разных лиц и существ он может быть разный, 
	//но ссылка на его "неживой" экземпляр хранится в структуре PostProcessSettings в массиве WeightedBlendables
	if (PostProcessSettings.WeightedBlendables.Array.Num() > 0)
	{
		//нужно "оживить" материал, сделать динамическую ссылку. непонятно, будет ли удаляться старая 
		auto* mat = Cast<UMaterialInterface>(PostProcessSettings.WeightedBlendables.Array[0].Object);
		HealthReactionScreen = UMaterialInstanceDynamic::Create(mat, this);

		//заменить в том же слоте статическую на динамическую
		PostProcessSettings.WeightedBlendables.Array[0].Object = HealthReactionScreen;

		//сразу же настроить параметр
		HealthReactionScreen->SetScalarParameterValue(TEXT("LowHealthAmount"), FMath::Min(1.0f - myBody()->Health, 1.0f));
		HealthReactionScreen->SetScalarParameterValue(TEXT("Psychedelic"), 0);
	}
}

//==============================================================================================================
//базовый переключатель лиц
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
//событие законченного перехода к определенному лицу - изменить шейдер
//==============================================================================================================
void UMyrCamera::HappenPerson(int P)
{
	if (P == 33)		Sight = me()->GetRootComponent();
	else if (P == 11)	Sight = nullptr;
	if (auto E = me()->GetEyeVisuals(P)) AdoptEyeVisuals(*E);
}

//==============================================================================================================
//обработка расстояния камеры
//==============================================================================================================
void UMyrCamera::ProcessDistance(float DeltaTime, FVector ctrlDir)
{
	if (myCtrl()->IsNowUIPause()) return;

	//направление и расстояние от камеры до точки, будет считаться далее
	FVector Dir = ctrlDir;
	float	DueDist = DistanceBasis * DistanceModifier;

	//целевые, желаемые, идеальные значения для позиции камеры, начало и конец - без учёта искажений препятствиями и положения реальной камеры 
	FVector SeerPos  = Seer  ? LOC(Seer)	: (Sight ? (LOC(Sight) - Dir * DueDist) : LOC(me()->GetRootComponent()));
	FVector SightPos = Sight ? LOC(Sight)	: SeerPos + Dir * DueDist;

	//скорректировать позицию наблюдателя с учетом сдвига 
	SeerPos += GetComponentTransform().GetUnitAxis(EAxis::Y)*SeerOffset/* + GetComponentTransform().GetUnitAxis(EAxis::Z) * SeerOffset.Y*/;
	SeerOffset = FMath::Lerp(SeerOffset, SeerOffsetTarget, 0.1);

	//целевое положение камеры с учётом искажений от препятствий
	FVector SeerPosDistort = SeerPos;

	//скорость перехода камеры в нужную позицию
	float DollySpeed = DeltaTime;
	float OrbitSpeed = Seer ? 0.1f : 1.0f;


	//текущий полный вектор линии зрения от камеры до цели
	FVector CurCamDir = SightPos - LOC(this);
	FVector NeededCamDir = SightPos - SeerPos;

	//обнаружение завершенного перехода между лицами
	float CurCamDist2 = CurCamDir.SizeSquared();
	if ((CurCamDist2 < RadiusHard * RadiusHard) == (bool)Sight)
		HappenPerson((Sight == nullptr || DistanceModifier > 0) ? 33 : 11);

	//имеется цель камеры, это режим 3 лица или кинематографический
	if (Sight)
	{
		//доделать разбивку на расстояние и единичный вектор
		float Dist = DueDist;
		NeededCamDir.ToDirectionAndLength(Dir, Dist);

		//конечная позиция трассировки назад - от цели видения в голову и ещё "за затылок" на радиус мягкой сферы ограничения
		FVector OverStart = SightPos - Dir*(Dist + RadiusSoft);

		//трассировка 
		FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("MyrCameraTrace")), false, me());
		RV_TraceParams.AddIgnoredActor(Sight->GetOwner());
		RV_TraceParams.AddIgnoredActor(myBody());
		FHitResult Hit(ForceInit);
		GetWorld()->LineTraceSingleByChannel(Hit, SightPos, OverStart, ECC_WorldStatic, RV_TraceParams);

		//если трассировка нащупала препятствие
		if (Hit.Component.IsValid() && Hit.Distance > 0 && 
			Hit.Component->GetCollisionResponseToChannel(ECollisionChannel::ECC_Camera) != ECollisionResponse::ECR_Ignore)
		{
			//глубина проникновения в мягкую сферу вокруг камеры
			float Penetration = Hit.Distance - Dist;

			//препятствие сзади проникло в жесткую сферу вокруг камеры и возможно уже загородило обзор
			if (Penetration < RadiusHard)
			{
				//препятствие ближе к объекту наблюдения, чем к камере - есть смысл обогнуть
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
				//препятствие неудобно огибать, придется резко приближать камеру
				else
				{	
					//резко приблизиться, без всяких плавностей
					SeerPosDistort = SightPos - Dir * (Hit.Distance - RadiusHard);
					if(Hit.Component->Mobility == EComponentMobility::Static)
						DollySpeed = 1;
					else DollySpeed = 10*DeltaTime;
				}
			}
			//препятствие в "мягкой зоне", влияет на место камеры очень мягко
			else
			{
				//очень плавно приблизиться по вектору 
				SeerPosDistort = SightPos - Dir * (Hit.Distance - RadiusHard);
				DollySpeed = DeltaTime * Penetration / RadiusSoft;
			}
		}
		//препятствие не обнаружено в опасной близости, свободно переместить камеру в новую позицию 
		else
		{
			SeerPosDistort = SeerPos;
			DollySpeed = 0.1;
		}

		FVector WayToWish = SeerPosDistort - LOC(this);
		FVector WayCloseUp = Dir * (WayToWish | Dir);
		FVector WayAround = WayToWish - WayCloseUp;
		FVector NewCamPos = LOC(this) + (WayAround * OrbitSpeed) + (WayCloseUp * DollySpeed);

		//финальное решение для нового места камеры - плавно сгоняем в нужную сторону
		//FVector NewCamPos = FMath::Lerp(LOC(this), SeerPosDistort, MatchingSpeed);
		Dir = (SightPos - NewCamPos).GetUnsafeNormal();
		auto Ro = FRotationMatrix::MakeFromXZ(Dir, FVector::UpVector).ToQuat();
		SetWorldLocationAndRotation(NewCamPos, Ro);

	}
	else SetRelativeLocationAndRotation(FVector(0, 0, 0), me()->GetControlRotation());

}

//==============================================================================================================
//применить графическое отображение ощущений
//==============================================================================================================
void UMyrCamera::SetFeelings(float Psycho, float Pain, float Dying)
{
	//уход в чёрно-белое при включении интерфейса или при умирании, иначе плавное возвращение в цветность
	PostProcessSettings.ColorSaturation.W *= (myCtrl()->IsNowUIPause() || Dying > 0.0f)
		? 0.8f : (1.2 - PostProcessSettings.ColorSaturation.W);

	//усилить слепоту
	HealthReactionScreen->SetScalarParameterValue(TEXT("Psychedelic"), FMath::Min(FMath::Max(Psycho, Dying), 1.0f));
	HealthReactionScreen->SetScalarParameterValue(TEXT("Desaturate"), FMath::Min(Dying, 1.0f));

	//прочие эффекты
	Pain = FMath::Clamp(Pain - 0.9f, 0.0f, 1.0f);
	PostProcessSettings.SceneFringeIntensity = Pain;			//усиление двоения в глазах при резкой боли
	myCtrl()->GetCameraShake()->ShakeScale = Psycho;			//медленная тряска, связанная с упоротостью
	myCtrl()->GetPainCameraShake()->ShakeScale = Pain;			//быстрая тряска, связанная с болью


}

void UMyrCamera::SetFeelingsSlow(float Health, float Sleepy, float Psycho, float Dying)
{
	//размытие в движении определяется текущей дин-моделью + уровнем упоротости + уровнем предсмертности
	PostProcessSettings.MotionBlurAmount = me()->GetCurMotionBlur() + 20 * Dying + 10 * Psycho;
	PostProcessSettings.bOverride_MotionBlurAmount = (PostProcessSettings.MotionBlurAmount > 0.01);

	//помутнение, связанное с сонностью
	PostProcessSettings.ColorContrastMidtones.W = 1 - Sleepy / 2;	

	//тьма по краям при увечьях
	HealthReactionScreen->SetScalarParameterValue(TEXT("LowHealthAmount"), FMath::Clamp(1.2f - 1.1f * Health, 0.0f, 1.0f));
}

//==============================================================================================================
//конструктор
//==============================================================================================================
UMyrCamera::UMyrCamera(const FObjectInitializer& ObjectInitializer)
{
	bUsePawnControlRotation = false;
}

//при запуске игры
void UMyrCamera::BeginPlay()
{
	Super::BeginPlay();

	//запустить тряску камеры, она бесконечна и регулируется параметрами
	if (me()->Shake.Get())		myCtrl()->AddCameraShake(me()->Shake);
	if (me()->PainShake.Get())	myCtrl()->AddPainCameraShake(me()->PainShake);
	
	//разблокировать настройку контраста = его регулирует сонность
	PostProcessSettings.bOverride_ColorContrastMidtones = true;

	//включить функцию обесцвечивания экрана
	PostProcessSettings.bOverride_ColorSaturation = true;	

}
