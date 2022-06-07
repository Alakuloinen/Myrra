// Fill out your copyright notice in the Description page of Project Settings.


#include "MyrraDoor.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"	// привязь на физдвижке
#include "Components/StaticMeshComponent.h"				// 
#include "Components/AudioComponent.h"					// звук
#include "DrawDebugHelpers.h"							// рисовать отладочные линии
#include "../Creature/MyrPhyCreature.h"					// когда замечает нас существо
#include "Kismet/GameplayStatics.h"						// для вызова SpawnEmitterAtLocation и GetAllActorsOfClass


//==============================================================================================================
//правильный конструктор
//==============================================================================================================
UMyrDoorLeaf::UMyrDoorLeaf(const FObjectInitializer& ObjectInitializer) :Super(ObjectInitializer)
{
	SetMobility(EComponentMobility::Movable);				//дверь подвижна
	SetSimulatePhysics(false);								//физику нах, говёно работает, симулировать кинематикой
	SetNotifyRigidBodyCollision(true);						//слать сообщения при ударе о дверь
	OnComponentHit.AddDynamic(this, &UMyrDoorLeaf::OnHit);	//куда слать
	PrimaryComponentTick.TickInterval = 1.0f;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

//==============================================================================================================
//при запуске игры
//==============================================================================================================
void UMyrDoorLeaf::BeginPlay()
{
	Super::BeginPlay();
	if (!Profile) return;

	//полагая, что дверь в начале игры открыта, сохраняем позицию закрытости
	StartAlongDir = DoorAlong();
	RotVelBaseP = FQuat(DoorUp(), Profile->MaxRotVelPositive);
	RotVelBaseN = FQuat(DoorUp(), -Profile->MaxRotVelNegative);

	//после фиксации нулоевого положения можно поставить дверь в открытое положение, если нужно
	if (Profile->StartAngle != 0 || Profile->SelfClosingForce != 0.0f)
	{
		SetOpenness(Profile->StartAngle);
		PrimaryComponentTick.TickInterval = 0.0f;
	}

	//начальная перетасовка
	//здесь нужно как-то различать тру рандомные и рандомные в начале игры и потом сохраняемые
	//возможно, сохранять только артефакты, а строения с этими объектами оставить магическим образом изменчивыми
	VaryOnStart();
}

//==============================================================================================================
//каждый кадр - когда движется
//==============================================================================================================
void UMyrDoorLeaf::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Profile) return; 

	//только если так покадрово (хотя нах проверять, просто обнулить дельту
	if (PrimaryComponentTick.TickInterval == 0.0f)
	{
		//затухание
		RotVel = RotVel * (1 - Profile->Friction);

		//самозакрывание (есди введена такая сила)
		if (Profile->SelfClosingForce > 0.0f)
		{
			RotVel = FMath::Lerp(RotVel,
				FMath::Clamp(50*(Profile->SelfClosingOpenness - GetSideOpenness()), -1.0f, 1.0f),
				Profile->SelfClosingForce);
		}

		//пересечение порога, события установки открытости и закрытости
		////////////////////////////////////////////////////////////////////////
		//дверь пересекает планку, вероятно, закрыта или закрылась только что
		if (GetClosedness() > 0.99)
		{
			//если включено автозакрывание по закрытию
			if (LockPending) Lock();

			//просто фиксация закрытия
			if (!Closed)
			{
				//вызывести флаг и прозвучать
				Closed = true;
				DoorSound(Profile->SoundAtClosing, RotVel);
			}
		}
		//открытие
		else if (GetClosedness() < 0.85)
		{
			//если до этого числилась закрытой
			if (Closed)
			{
				//сбросить, уже слишком большой угол, чтобы считаться закрытой
				Closed = false;

				//если последний толкатель (видимо, приведший к открытию) - существо
				if (auto Myr = Cast<AMyrPhyCreature>(LastMover))

					//записать этому существу в копилку событие, возможно, повлияющее на игровой сюжет
					Myr->CatchMyrLogicEvent(EMyrLogicEvent::ObjOpenDoor, FMath::Abs(GetSideOpenness()), this);
			}
		}

		//применение плюс торможение на краях интервала
		////////////////////////////////////////////////////////////////////////
		//текущая инерция открывает дверь в плюсовую сторону
		if (RotVel > 0)
		{
			//применение диапазона ограничений на косинус угла открытия в плюсовую сторону
			if (GetSideOpenness() >= Profile->PositiveLimit.GetLowerBoundValue())
			{	if (GetSideOpenness() < Profile->PositiveLimit.GetUpperBoundValue()) RotVel *= 0.9;	else RotVel = 0;
			}
			AddLocalRotation(FQuat::Slerp(FQuat::Identity, RotVelBaseP, RotVel), true);
		}
		//текущая инерция открывает дверь в минусовую сторону
		else
		{
			//применение диапазона ограничений на косинус угла открытия в минусовую сторону
			if (-GetSideOpenness() >= Profile->NegativeLimit.GetLowerBoundValue())
			{	if (-GetSideOpenness() < Profile->NegativeLimit.GetUpperBoundValue()) RotVel *= 0.9;	else RotVel = 0;
			}
			AddLocalRotation(FQuat::Slerp(FQuat::Identity, RotVelBaseN, -RotVel), true);
		}

		//отключение тика, когда дверь упокоилась
		if (FMath::Abs(RotVel) < 0.0001)
		{	PrimaryComponentTick.TickInterval = 1.0f;
			RotVel = 0;
		}

		FVector St = GetComponentLocation() + FVector(0, 0, 5);
		DrawDebugLine(GetOwner()->GetWorld(), St, St + DoorFront() * (100), FColor(255 * GetClosedness(), 255 * GetSideOpenness(), 255), false, 0.1f, 100, 1);
		DrawDebugLine(GetOwner()->GetWorld(), St, St + StartAlongDir * (100), FColor(255 * GetClosedness(), 255, 255), false, 0.1f, 100, 1);
		DrawDebugString(GetWorld(), GetComponentLocation(),
			FString::SanitizeFloat(GetSideOpenness(), 1) + TEXT("   Vel:") + FString::SanitizeFloat(RotVel, 1),
			nullptr, FColor(255, 255, 0, 255), 0.02, false, 1.0f);
	}
}

//==============================================================================================================
//реакция на касание 
//==============================================================================================================
void UMyrDoorLeaf::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!Profile) return;
	if (Locked) return;

	//сонаправленность точки приложения силы и полотна двери, обычно всегда 1 для толкания, но если касются торца, открывается туже
	float ByNormal = FVector::DotProduct(Hit.Normal, DoorFront());

	//насколько большая скорость была у токающего объекта - самая важная завизисмость
	float ByVel = FVector::DotProduct(OtherComp->GetPhysicsLinearVelocity(Hit.BoneName), -Hit.Normal);

	//плечо силы, насколько далеко от оси произошел удар, чем дальше, тем лучше
	float Shoulder = FMath::Abs(FVector::DotProduct(GetComponentLocation() - Hit.ImpactPoint, DoorAlong()) / Bounds.BoxExtent.X / 2);

	//каким-то образом коснулись, умудрившись отлипать от двери - никакого толчка
	if (ByVel < 0) return;

	//изменить текущий модуль скорости вращения
	RotVel = RotVel + Profile->Sensibility * ByNormal * (ByVel + 1.0f) * Shoulder;
	RotVel = FMath::Clamp(RotVel, -1.0f, 1.0f);
	UE_LOG(LogTemp, Log, TEXT("%s: OnHit, Shoulder %g? byvel=%g, bynormal=%gб RotVel=%g"), *GetName(), Shoulder, ByVel, ByNormal, RotVel);

	//сохранить последнего от/закрывателя
	LastMover = OtherActor;

	//сброс до минимального
	PrimaryComponentTick.TickInterval = 0.0f;
	//TickComponent(0.01f, ELevelTick::LEVELTICK_All, nullptr);
	SetComponentTickEnabled(true);


}

//==============================================================================================================
//явно установить открытость двери
//==============================================================================================================
void UMyrDoorLeaf::SetOpenness(float Angle)
{
	//почему минус хз, просто так работает и это придётся принять
	SetRelativeRotation(FQuat(DoorUp(), -Angle));
}

//==============================================================================================================
//звук разных моментов двери
//==============================================================================================================
void UMyrDoorLeaf::DoorSound(USoundBase* WhatTo, float Strength)
{
	//если в настройках присутствует образец звука
	if (WhatTo)
	{
		//мутная функция по созданию источника звука ad-hoc
		auto Noiser = UGameplayStatics::SpawnSoundAttached(
			WhatTo, this, NAME_None, this->GetComponentLocation(),
			FRotator(), EAttachLocation::KeepWorldPosition, true,
			1.0, 1.0f, 0.0f,
			nullptr, nullptr, true);

		//как ниюудь в редакторе сделать смесь звуков от силы удара
		Noiser->SetFloatParameter(TEXT("Strength"), Strength);
	}
}

//==============================================================================================================
//отпереть дверь, если она заперта и правильно позиционирована
//==============================================================================================================
void UMyrDoorLeaf::Unlock()
{
	if (Locked)
	{
		//перевести петли в состояние ограниченной, но свободы перемещения, ранее установленные лимиты не перезаписывать
		Locked = false;
		DoorSound(Profile->SoundAtUnLocking);

		//включить тик, дверь оживлена и может двигаться
		PrimaryComponentTick.SetTickFunctionEnable(true);

		//если включена сила самозакрытия или открытия - включить максимальный тик, чтобы сразу пошла двигаться
		if (Profile->SelfClosingForce > 0.0f)
			PrimaryComponentTick.TickInterval = 0.0;
		UE_LOG(LogTemp, Log, TEXT("%s: Unlock"), *GetName());
	}
	else UE_LOG(LogTemp, Log, TEXT("%s: Door already unlocked"), *GetName());

}

//==============================================================================================================
//запереть дверь
//==============================================================================================================
void UMyrDoorLeaf::Lock()
{
	LockPending = 0;	//отменить предвариловку
	RotVel = 0;			//обнулить скорость вращения
	SetOpenness(0);		//выровнять по нулю, чтобы красиво было
	Locked = true;		//взвести флаг

	//сделать тик тик, теперь не нужен
	PrimaryComponentTick.SetTickFunctionEnable(false);

	//проиграть характерный звук
	DoorSound(Profile->SoundAtLocking);
}

//==============================================================================================================
//внести вариацию состояний на старте, согласно ассету
//==============================================================================================================
void UMyrDoorLeaf::VaryOnStart()
{
	if (!Profile) return;
	uint8 Die = FMath::RandRange(0, 254);
	if (Die <= ProbabilityOfBeingAbsent) this->SetVisibility(false);
	if (Die <= ProbabilityOfBeingLocked) this->Lock();
}

