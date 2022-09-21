// Fill out your copyright notice in the Description page of Project Settings.


#include "MyrGirdle.h"
#include "../MyrraGameInstance.h"						// ядро игры
#include "MyrPhyCreature.h"								// само существо
#include "MyrPhyCreatureMesh.h"							// тушка
#include "DrawDebugHelpers.h"							// рисовать отладочные линии
#include "PhysicalMaterials/PhysicalMaterial.h"			// для выкорчевывания материала поверхности пола

//рисовалки отладжочных линий
#if WITH_EDITOR
#define LINE(C, A, AB) MyrOwner()->Line(C, A, AB)
#define LINEWT(C, A, AB,W,T) MyrOwner()->Line(C, A, AB,W,T)
#define LINEW(C, A, AB, W) MyrOwner()->Line(C, A, AB, W)
#define LINELIMB(C, L, AB) MyrOwner()->Line(C, me()->GetMachineBody(L)->GetCOMPosition(), AB)
#define LINELIMBW(C, L, AB, W) MyrOwner()->Line(C, me()->GetMachineBody(L)->GetCOMPosition(), AB, W)
#define LINELIMBWT(C, L, AB, W, T) MyrOwner()->Line(C, me()->GetMachineBody(L)->GetCOMPosition(), AB, W, T)
#define LINELIMBWTC(C, L, AB, W, T, t) MyrOwner()->Linet(C, me()->GetMachineBody(L)->GetCOMPosition(), AB, t, W, T)
#define LDBG(C) MyrOwner()->IsDebugged(C)
#else
#define LDBG(C) false
#define LINE(C, A, AB)
#define LINELIMB(C, L, AB)
#define LINELIMBW(C, L, AB, W)
#define LINELIMBWT(C, L, AB, W, T) 
#define LINELIMBWTC(C, L, AB, W, T, t)
#define LINEW(C, A, AB, W)
#endif

//меш этого существа
UMyrPhyCreatureMesh* UMyrGirdle::me() { return MyrOwner()->GetMesh(); }
UMyrPhyCreatureMesh* UMyrGirdle::mec() const  { return MyrOwner()->GetMesh(); }

//статический, таблица поиска абсолютного идентификатора части тела по его относительному идентификатору ("лучу")
ELimb UMyrGirdle::GirdleRays[2][(int)EGirdleRay::MAXRAYS] =
{ {ELimb::PELVIS,		ELimb::L_LEG,	ELimb::R_LEG,	ELimb::LUMBUS,	ELimb::TAIL},
	{ELimb::THORAX,		ELimb::L_ARM,	ELimb::R_ARM,	ELimb::PECTUS,	ELimb::HEAD}
};

//выдать членик и его физ-тело в системе коориднат конкретно этого пояса
FLimb&			UMyrGirdle::GetLimb (EGirdleRay R)			{ return me() -> GetLimb		(GetELimb(R)); }
const FLimb&	UMyrGirdle::GetLimb (EGirdleRay R) const	{ return mec() -> GetLimb		(GetELimb(R)); }
FBodyInstance*  UMyrGirdle::GetBody (EGirdleRay R)			{ return me() -> GetMachineBody	(GetELimb(R)); }

//констрайнт, которым спинная часть пояса подсоединяется к узловой (из-за единой иерархии в разных поясах это с разной стороны)
FConstraintInstance* UMyrGirdle::GetGirdleSpineConstraint() { return (IsThorax) ? me()->GetMachineConstraint(me()->Thorax) : me()->GetMachineConstraint(me()->Lumbus); }
FConstraintInstance* UMyrGirdle::GetGirdleArchSpineConstraint() { return (IsThorax) ? me()->GetArchMachineConstraint(me()->Thorax) : me()->GetArchMachineConstraint(me()->Lumbus); }

//режим попячки назад
bool UMyrGirdle::IsMovingBack() const { return MyrOwner()->bMoveBack; }




//то же для краткости и инлайновости
#define ELMB(R)							GirdleRays[IsThorax][(int)EGirdleRay::R]
#define LMB(R)		me()->GetLimb       (GirdleRays[IsThorax][(int)EGirdleRay::R])
#define CLMB(R)		mec()->GetLimb       (GirdleRays[IsThorax][(int)EGirdleRay::R])
#define BDY(R)		me()->GetMachineBody(GirdleRays[IsThorax][(int)EGirdleRay::R])
#define CBDY(R)		mec()->GetMachineBody(GirdleRays[IsThorax][(int)EGirdleRay::R])

#define LAXIS(L,V) me()->GetLimbAxis##V##(L)
#define BAXIS(B,V) me()->GetBodyAxis##V##(B)

//==============================================================================================================
//конструктор
//==============================================================================================================
UMyrGirdle::UMyrGirdle (const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//инициализация битовых полей
	FixedOnFloor = false;
	NoTurnAround = false;
	Vertical = false;
	bHiddenInGame = false;

	//чтобы таскать отдельно от остального тела и не менять фреймы зацепа
	SetUsingAbsoluteRotation(true);	
	SetUsingAbsoluteLocation(true);	

	//не нужна физика
	SetSimulatePhysics(false);
}

//==============================================================================================================
//при запуске игры
//==============================================================================================================
void UMyrGirdle::BeginPlay()
{
	//берем для примера правую конечность и выясняем, есть ли конечности вообще
	HasLegs = me()->HasPhy(ELMB(Right));

	//профиль столкновений особый, чтобы не отбивался от тела, а вообще колизии наверно не нужны
	SetCollisionProfileName(TEXT("PawnTransparent"));

	//переместить нас в центр пояса конечностей, чтобы все смещения были по нулям
	SetWorldLocation(BDY(Center)->GetUnrealWorldTransform().GetLocation());

	//настроить подопечные поясу части тела (указав для них максимальную скорость вращения)
	me()->InitLimbCollision(LMB(Center), 100);
	me()->InitLimbCollision(LMB(Spine), 100);
	me()->InitLimbCollision(LMB(Tail), 100);
		
	////////////////////////////////////////////////////////
	//инициализировать центральный зацеп
	BDY(Center)->DOFConstraint = me()->PreInit();
	auto CI = BDY(Center)->DOFConstraint;

	//стартоваые значения для нормалей ног, чтобы знать, в какую сторону трассировать
	LMB(Center).ImpactNormal = BAXIS(BDY(Center), Up);
	GuidedMoveDir = BAXIS(BDY(Center), Forth);

	//фрейм в мире самого членика = его локальные оси в его же локальных координатах, не меняются
	CI->PriAxis1 = (FVector)MyrAxes[(int)me()->MBoneAxes().Up];
	CI->SecAxis1 = (FVector)MyrAxes[(int)me()->MBoneAxes().Forth];
	CI->Pos1 = FVector(0,0,0);

	//фрейм в мире окружения = первичная ось=верх, вторичная=вперед, пока неясно, нужно ли смещать позицию от нуля
	CI->PriAxis2 = FVector(0,0,1);	//вверх, вертикаль
	CI->SecAxis2 = FVector(-1,0,0);	//почему тут минус, ВАЩЕ ХЗ Я В АХУЕ, но без него при ограничении по этой оси существо переворачивается
	CI->Pos1 = FVector(0,0,0);

	//окончательно инициализировать привязь, возможно, фреймы больше не будут меняться
	CI->InitConstraint(BDY(Center), GetBodyInstance(), 1.0f, this);

	//привязь, удерживающая спину в заданном положении
	me()->SetFreedom(CI, TargetFeetLength, TargetFeetLength, TargetFeetLength, 180, 180, 180);
	CI->SetLinearPositionDrive(false, false, false);
	CI->SetLinearDriveParams(5000, 100, 10000);
	CI->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
	CI->SetAngularDriveParams(5000, 200, 10000);
	CI->SetOrientationDriveTwistAndSwing(false, false);


	//////////////////////////////////////////////////
	Super::BeginPlay();
}

//==============================================================================================================
//пересчитать базис длины ног через расстояние от сокета-кончика для кости-плечевогосустава
//==============================================================================================================
void UMyrGirdle::UpdateLegLength()
{
	//извлечь значение
	TargetFeetLength = (TargetFeetLength == 0) ? FMath::Sqrt(GetFeetLengthSq()) : TargetFeetLength * GetComponentScale().Z;

	//привязь, удерживающая спину в заданном положении
	me()->SetFreedom(BDY(Center)->DOFConstraint, TargetFeetLength, TargetFeetLength, TargetFeetLength, 180, 180, 180);

	//начальные радиусы ног строго вниз по бокам тела от плеч
	SetFootRay(LMB(Right), BAXIS(BDY(Center), Down) * TargetFeetLength);
	SetFootRay(LMB(Left), BAXIS(BDY(Center), Down) * TargetFeetLength);
}

//==============================================================================================================
//вычислить ориентировочное абсолютное положение конца ноги по геометрии скелета
//==============================================================================================================
FVector UMyrGirdle::GetFootTipLoc(ELimb eL)
{
	//у конечности должен быть сокет, расположеный на самом кончике, он же используется для постановки следа и пыли
	FName TipName = me()->FleshGene(eL)->GrabSocket;

	//если сокет явно не задан, что плохо
	if (TipName.IsNone())
	{
		//ваще плохой случай
		return me()->GetComponentLocation();
	}
	//идеальный случай - точное положение косточки на конце
	else return me()->GetSocketLocation(TipName);
	 
}

//==============================================================================================================
//радиус вектор от плеча до ступни в абсолютных координатах
//==============================================================================================================
FVector3f UMyrGirdle::GetFootRay(FLimb& L)
{ 	return 
		BAXIS(BDY(Spine), Up) * L.RelFootRay().X +
		BAXIS(BDY(Spine), Forth) * L.RelFootRay().Y +
		BAXIS(BDY(Spine), Right) * L.RelFootRay().Z;
}

//==============================================================================================================
//загрузить новый радиус вектор ноги из точки касания с опорой
//==============================================================================================================
void UMyrGirdle::SetFootRay(FLimb& F, FVector3f AbsRay)
{ 	F.RelFootRay() = FVector3f(
		(BAXIS(BDY(Spine), Up) | AbsRay),
		(BAXIS(BDY(Spine), Forth) | AbsRay),
		(BAXIS(BDY(Spine), Right) | AbsRay)
	);
}

//==============================================================================================================
//позиция точки проекции ноги на твердь
//==============================================================================================================
FVector UMyrGirdle::GetFootVirtualLoc(FLimb& L)
{ return me()->GetLimbShoulderHubPosition(L.WhatAmI) + (FVector)GetFootRay(L); }

//==============================================================================================================
//пересчитать реальную длину ног
//==============================================================================================================
float UMyrGirdle::GetFeetLengthSq(ELimb eL)
{
	//если сокета нет придётся брать центр колса-ноги и приспускать по нормали вниз на радиус колеса
	FVector Tip = GetFootTipLoc(eL);

	//другой конец тоже неочевиден - обычно это конец "ключицы" или специальная кость, помечающая место вокруг которого конечность крутится
	FVector Root = me()->GetLimbShoulderHubPosition(eL);

	//для быстроты берется квадрат расстояния
	return FVector::DistSquared(Root, Tip);
}

float UMyrGirdle::GetFeetLengthSq(bool Right)
{	return GetFeetLengthSq(Right ? ELMB(Right) : ELMB(Left)); }


//==============================================================================================================
//включить или отключить проворот спинной части относительно ног (мах ногами вперед-назад)
//==============================================================================================================
void UMyrGirdle::SetSpineLock(bool Set)
{
	//если изначально был залочен, то и будет залочен
	auto CI = GetGirdleSpineConstraint();
	auto aCI = GetGirdleArchSpineConstraint();
	if (Set)	CI->ProfileInstance.TwistLimit.TwistMotion = EAngularConstraintMotion::ACM_Locked;
	else		CI->ProfileInstance.TwistLimit.TwistMotion = aCI->ProfileInstance.TwistLimit.TwistMotion;
	CI->UpdateAngularLimit();
	//UE_LOG(LogMyrPhyCreature, Log, TEXT("%s: SetGirdleSpineLock %d"), *GetOwner()->GetName(), Set);
}

//==============================================================================================================
//влить динамическую модель 
//==============================================================================================================
void UMyrGirdle::AdoptDynModel(FGirdleDynModels& Models)
{
	//сохранить связку, так как в процессе понадобится уточнять
	CurrentDynModel = &Models;

	//влить почленно
	me()->AdoptDynModelForLimb(LMB(Center), Models.Center, Models.CommonDampingIfNeeded);
	me()->AdoptDynModelForLimb(LMB(Spine), Models.Spine, Models.CommonDampingIfNeeded);
	me()->AdoptDynModelForLimb(LMB(Tail), Models.Tail, Models.CommonDampingIfNeeded);

	//запретить ногам махать вдоль спины - сомнительная надобность, опасно
	SetSpineLock(Models.LegsSwingLock);

}


//==============================================================================================================
//трассировка лучом для установление опоры
//==============================================================================================================
bool UMyrGirdle::Trace(FVector Start, FVector3f Dst, FHitResult& Hit)
{
	//преднастройки трассировки
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("Daemon_TraceForSteppedness")), false, MyrOwner());
	RV_TraceParams.bReturnPhysicalMaterial = true;
	FCollisionObjectQueryParams COQ;
	COQ.AddObjectTypesToQuery(ECC_WorldStatic);
	COQ.AddObjectTypesToQuery(ECC_PhysicsBody);
	GetWorld()->LineTraceSingleByObjectType(Hit, Start, Start + (FVector)Dst, COQ, RV_TraceParams);
	if (Hit.IsValidBlockingHit())	LINEWT(ELimbDebug::LineTrace, Start + (FVector)Dst * Hit.Time, FVector(0), 2, 0.5);
	else							LINEWT(ELimbDebug::LineTrace, Start, (FVector)Dst, 1, 0.5);
	return Hit.IsValidBlockingHit();
}

//==============================================================================================================
//прощупать почву под положение ног
//==============================================================================================================
bool UMyrGirdle::SenseForAFloor(float DeltaTime)
{
	//сюда скидывать результаты
	FHitResult Hit(ForceInit);

	//откуда
	FLimb& StartLimb = LMB(Center);
	const FVector Start = 0.5 * (me()->GetLimbShoulderHubPosition(ELMB(Left)) + me()->GetLimbShoulderHubPosition(ELMB(Right)));

	//направление - по умолчанию по оси таза вниз
	FVector3f LookAt = LAXIS(LMB(Center), Down);

	//если уже есть твердь, то можно уточнить, куда щупать
	if (StartLimb.Stepped)
	{
		//если жесткая вертикаль, то вне зависимости от крена таза щупать строго вниз
		if (Vertical) LookAt = FVector3f::DownVector;

		//если без вертикали, но на тверди, ориентроваться на нормаль
		else LookAt = -StartLimb.ImpactNormal;
	}

	//поднятие над землей, норма, от плеч до ступней
	float Elevation = GetTargetLegLength();

	//устойчивость = обнулить с прошлого кадра + внести слабый вклад других частей, чтобы при лежании на боку было >0
	StandHardness = 
		FMath::Max(0, (int)LMB(Spine).Stepped - STEPPED_MAX + 20) + 
		FMath::Max(0, (int)LMB(Tail).Stepped - STEPPED_MAX + 15);

	//на старте ограничения берем из предписаний модели, затем будем смотреть, можно ли их в ситуации использовать
	FixedOnFloor = CurrentDynModel->FixOnFloor;
	Vertical = CurrentDynModel->HardVertical;
	NoTurnAround = CurrentDynModel->HardCourseAlign;

	//насколько далеко искать опору
	float ReachMod = StartLimb.Stepped ? 1.3 : 1.0;

	//особый случай, если активная фаза прыжка, прервать фиксацию на опоре, что оторваться
	if (MyrOwner()->DoesAttackAction() && MyrOwner()->NowPhaseToJump())
	{ FixedOnFloor = false; ReachMod = 1.0; }

	//##############################################################################
	//покадровая трассировка опоры
	Trace(Start, LookAt * Elevation * ReachMod, Hit);
	//##############################################################################

	//абсолютная скорость в поясе, для расчёта скорости относительной
	FVector3f RelativeSpeedAtPoint = me()->BodySpeed(LMB(Center));

	//понять , куда надо поместить якорь, чтобы этовыглядело как стояние над землей
	FVector DueLoc = GetComponentLocation();
	FVector3f DueUp = FVector3f::UpVector;
	FVector3f DueFront = BAXIS(BDY(Center), Forth);

	//реальный центр координат центра, минимум натяга
	FVector RealLoc = BDY(Center)->GetUnrealWorldTransform().GetLocation();

	//текущий уровень прижатия к земле
	Crouch = FMath::Lerp(Crouch, CurrentDynModel->Crouch, DeltaTime*5);

	//положение, в котором тело не съезжает
	FVector StableLoc = GetComponentLocation();

	//касание при падении
	bool Fallen = false;
	auto FM = Hit.PhysMaterial;
	FBodyInstance* Floor = nullptr;
	if(Hit.Component.IsValid()) Floor = Hit.Component->GetBodyInstance(Hit.BoneName);

	//нашупали правильную опору
	if (Hit.bBlockingHit && Floor && FM.IsValid())
	{
		//считанная нормаль
		auto NewNormal = (FVector3f)Hit.ImpactNormal;

		//условия перехода к карабканью - желание пользователя зацепиться и подходящая поверхность
		Climbing =
			(MyrOwner()->bClimb == true) &&						// желание зацепиться
			FLimb::IsClimbable(FM->SurfaceType.GetValue()) &&	// подходящая поверхность
			(NewNormal | LAXIS(StartLimb, Up))>-0.5f;			// подходящий угол

		//вес новой точки опоры
		float Weight = 1.0f;

		//при соприкосновении с землей из полета нужно будет внеочередно проверить ноги
		if (StartLimb.Stepped<STEPPED_MAX && IS_MACRO(MyrOwner()->CurrentState, AIR)) Fallen = true;

		//нормальное приятие поверхности = она пологая или по ней можно карабкаться
		if(NewNormal.Z > 0.5 || Climbing)
		{
			//сохранить часть данных сразу в ногу
			StartLimb.Surface = (EMyrSurface)FM->SurfaceType.GetValue();
			StartLimb.Stepped = STEPPED_MAX;
			StartLimb.Floor = Floor;

			//при резком изменении нормали (наступили на желоб/яму) новая инфа о тверди недостоверна
			Weight = FMath::Max(0.1f, StartLimb.ImpactNormal | NewNormal);
		}
		//отвесная поверхность, на которую нельзя залезть
		else
		{
			//в дополнение к уже посчитанным сообщаем фатальный непреодолимый затык
			Weight = 0.0f;
			//MyrOwner()->Stuck = -1;
		}

		//грубая коррекция возвышения на ногах
		//на случай если приземлился на брюхо
		if (Hit.Distance < Elevation*0.9)
			StableLoc -= (FVector)LookAt * (Elevation - Hit.Distance)*DeltaTime;
		else if(Hit.Distance > Elevation * 1.1)
			StableLoc += (FVector)LookAt * Elevation * DeltaTime;

		//скорректировать относительную скорость 
		RelativeSpeedAtPoint -= (FVector3f)Floor->GetUnrealWorldVelocityAtPoint(Hit.ImpactPoint);

		//нормаль, пока неясно, как плавно ее брать для середины
		if (CurrentDynModel->NormalAlign) StartLimb.ImpactNormal = NewNormal; else
		StartLimb.ImpactNormal = FMath::Lerp(StartLimb.ImpactNormal, NewNormal, 0.5f * Weight);

		//окончательное приведение скорости
		VelocityAgainstFloor = FMath::Lerp(VelocityAgainstFloor, RelativeSpeedAtPoint, 0.5f * Weight);

		//для невертикальной стойки стойка подстраивается под текущую нормаль
		if (!Vertical) DueUp = NewNormal;
		else DueUp = FVector3f::UpVector;

		//пересчитать маршрут на ближайший кадр
		if (CurrentDynModel->Leading)
		{
			//основа направления - курс из контроллера
			GuidedMoveDir = MyrOwner()->MoveDirection;

			//коррекция курса, если уткнулись в препятствие
			if (Hit.ImpactNormal.Z < 0.5)
			{	
				//для лазибельных препятствий выворот посчитанной траектории ввер
				float BumpInAmount = (-NewNormal) | MyrOwner()->MoveDirection;
				if(Climbing && BumpInAmount > 0.5)
					GuidedMoveDir.Z += 2*(BumpInAmount - 0.5);
			}

			//если имеется сильная уткнутость, то обтекаем препятствие
			me()->GuideAlongObstacle(GuidedMoveDir);

		}
		//ось вперед для ведомого - по направлению на ведущий
		else
		{
			GuidedMoveDir = (FVector3f)(MyrOwner()->GetAntiGirdle(this)->GetComponentLocation() - GetComponentLocation());
		}

		//ортонормировать текущий курс движения по актуальной нормали
		GuidedMoveDir = FVector3f::VectorPlaneProject(GuidedMoveDir, StartLimb.ImpactNormal);
		GuidedMoveDir.Normalize();

		//ориентировать якорь по посчитанному
		DueFront = GuidedMoveDir;

		//корректировка поднятия над землей если какие-то ноги слишком вытянуты
		if(FMath::Max(LMB(Right).RelFootRay().X, LMB(Right).RelFootRay().X) > Elevation)
			Elevation = FMath::Min3(Elevation, LMB(Right).RelFootRay().X, LMB(Right).RelFootRay().X);

		//идеально положение якоря спины
		DueLoc = Hit.ImpactPoint + (FVector)DueUp * Elevation;

		//вектор отклонения в плоскости ползка
		FVector DisplacedInPlane = FVector::VectorPlaneProject(DueLoc - RealLoc, Hit.ImpactNormal);
		FVector DisplacedInHeight = (DueLoc - RealLoc).ProjectOnTo(Hit.ImpactNormal);
		LINEWT(ELimbDebug::GirdleConstrainOffsets, RealLoc, DisplacedInPlane, 1, 0);
		LINEWT(ELimbDebug::GirdleConstrainOffsets, RealLoc, DisplacedInHeight, 1, 0);
		LINELIMBWTC(ELimbDebug::GirdleStepWeight, StartLimb, (FVector)StartLimb.ImpactNormal*15, 1, 0.03, Weight);

	}
	else
	//нащупали неверную опору
	if (Hit.bBlockingHit)
	{
		//MyrOwner()->Stuck = -1;
		Hit.ImpactPoint = Start + (FVector)LookAt * Elevation;
	}
	//не нащупали опору
	else 
	{
		//если нет опоры, то не на что цепляться
		FixedOnFloor = false;
		Climbing = false;
		GuidedMoveDir = FMath::Lerp(GuidedMoveDir, MyrOwner()->MoveDirection, DeltaTime);

		//даже в воздухе если имеется сильная уткнутость, то обтекаем препятствие
		if(me()->GuideAlongObstacle(GuidedMoveDir)) GuidedMoveDir.Normalize();

		//ортогональна постановка фиктивной нормали вверх
		LMB(Center).ImpactNormal = GuidedMoveDir ^ (BAXIS(BDY(Center), Right));
		if ((LMB(Center).ImpactNormal | BAXIS(BDY(Center), Up)) < 0) LMB(Center).ImpactNormal *= -1;
		LMB(Center).ImpactNormal.Normalize();

		//фэйковая точка контакта для расчёта раскоряки ног SenseFootAtStep
		Hit.ImpactPoint = Start + (FVector)LookAt * Elevation;

		//окончательное приведение скорости - очень медленно, чтоб плавно соскочить с поверхности
		VelocityAgainstFloor = FMath::Lerp(VelocityAgainstFloor, RelativeSpeedAtPoint, 0.1f);

		//в свободном состоянии никакой особой правильной осанки нет, просто повторять за физикой тушки
		DueUp = BAXIS(BDY(Center), Up);
	}

	//вектор "правильный вперед", корректирующий pitch'и обеих частей туловища
	Forward = BAXIS(BDY(Center), Right) ^ LMB(Center).ImpactNormal;
	Forward.Normalize();

	//вычислить желаемую позицию для якоря, а также приращение
	if (FixedOnFloor)
	{
		//плавно подводим якорь спины в нужную точку
		DueLoc = FMath::Lerp(StableLoc,								//со статичного места 
			FMath::Lerp(DueLoc, RealLoc, 0.5f),						//плавно подводим реальную спину в якорь
			0.5f * MyrOwner()->MoveGain);							//плавно трогаемся с тягой местной

		//для случая, когда действие изнутри двигает тело
		if(me()->DynModel->MoveWithNoExtGain)
			 DueLoc += (FVector)me()->CalcOuterAxisForLimbDynModel(StartLimb) * MyrOwner()->GetDesiredVelocity() * DeltaTime; else

		//для режима управляемости - просто добавляем сдвиг в сторону желаемого курса
		 DueLoc += (FVector)GuidedMoveDir * MyrOwner()->GetDesiredVelocity() * DeltaTime;
	}
	//не цепляемся за поверхность, возможно, вообще в воздухе
	else
	{
		//поворот совпадает с реальным члеником, чтобы никак на него не влиять
		DueFront = BAXIS(BDY(Center), Forth);
		DueUp = BAXIS(BDY(Center), Up);
		DueLoc = RealLoc;
	}

	//кинематически переместить якорь в нужное место с нужным углом поворота
	SetWorldTransform(FTransform((FVector)DueFront, (FVector)(DueFront ^ DueUp), (FVector)DueUp, DueLoc));

	//подстройка притяжения к зацепу
	auto CI = BDY(Center)->DOFConstraint;

	//включение сил, прижимающих спину к якорю, действует только если режим цепа к опоре
	CI->SetLinearPositionDrive(FixedOnFloor, FixedOnFloor, FixedOnFloor);

	//включение сил, держащих спину по заданным углам
	CI->SetOrientationDriveTwistAndSwing(NoTurnAround, Vertical || CurrentDynModel->NormalAlign);

	//индикация направления
	LINELIMBWT(ELimbDebug::GirdleGuideDir, StartLimb, (FVector)GuidedMoveDir * 15, 1, 0.04f);
	LINELIMBWTC(ELimbDebug::GirdleCrouch, StartLimb, (FVector)Hit.Normal*20, 1, 0.04f, Crouch);

	//////////////////////////////////////////////////////////////////////////////
	//сверху от туловище рисуем тестовые индикаторы
	if (LDBG(ELimbDebug::GirdleConstrainMode))
	{
		auto Ct = BDY(Center)->GetCOMPosition() + StartLimb.ImpactNormal * 15;
		auto Color = FColor(255, 0, 255, 10);
		float ang = CI->ProfileInstance.ConeLimit.Swing1LimitDegrees;
		float ang2 = CI->ProfileInstance.TwistLimit.TwistLimitDegrees;
		if (Climbing)			DrawDebugString(GetWorld(), (FVector)Ct, TEXT("   C"), nullptr, FColor(0, 255, 255), 0.02, false, CurrentDynModel->Leading ? 1.5f : 1.0f);
		if (Vertical)			DrawDebugString(GetWorld(), (FVector)Ct, TEXT("     V"), nullptr, FColor(ang, 255, 0), 0.02, false, CurrentDynModel->Leading ? 1.5f : 1.0f);
		if (NoTurnAround)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("       Y"), nullptr, FColor(255, 0, ang2), 0.02, false, CurrentDynModel->Leading ? 1.5f : 1.0f);
		if (FixedOnFloor)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("         F"), nullptr, FColor(255, 255, 255), 0.02, false, CurrentDynModel->Leading ? 1.5f : 1.0f);
	}

	//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	// прощупь найденной поверхности виртуальными ногами, чтоб понять, как их ставить
	SenseFootAtStep (DeltaTime, LMB(Right), Hit.ImpactPoint, Fallen, 2 * IsThorax);
	SenseFootAtStep (DeltaTime, LMB(Left), Hit.ImpactPoint, Fallen, 2 * IsThorax + 1);
	//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	LINELIMBWTC(ELimbDebug::GirdleStandHardness, StartLimb, (FVector)StartLimb.ImpactNormal * 15, 1, 0.03, StandHardness/255.0f);

	//если мы только включили режим карабканья и что-то не получилось, запустить вспомогательные действиия
	if (MyrOwner()->bWannaClimb) MyrOwner()->ClimbTryActions();

	return false;
}

//==============================================================================================================
//побочная трассировка позици ноги,  коррекция раскорячивания ног
//==============================================================================================================
void UMyrGirdle::SenseFootAtStep(float DeltaTime, FLimb& Foot, FVector CentralImpact, bool Fallen, int ATurnToCompare)
{
	//старт трассировки,уровень плечь или таза
	const FVector Start = me()->GetLimbShoulderHubPosition(Foot.WhatAmI);


	//направление вовне в сторону
	FVector3f Lateral = Foot.IsLeft() ? LAXIS(LMB(Center), Left) : LAXIS(LMB(Center), Right);

	//распаковка в абсолютные координаты
	FVector3f CurFootRay = GetFootRay(Foot);

	//суррогатная точка контакта, если новой не считать, из центрального следа и в сторону
	FVector EndPoint = Start + (FVector)CurFootRay;

	//размах ноги в сторону - обратная связь для коррекции размаха
	float SwingAlpha = 0.1;

	//скорость обновления размаха ног
	float SpreadAlpha = 0.1;

	//пределы раскоряки для данного режима моторики
	float SwingMin = CurrentDynModel->LegsSpreadMin * TargetFeetLength;
	float SwingMax = CurrentDynModel->LegsSpreadMax * TargetFeetLength;

	//критерии полного просчёта ноги = 
	if (LMB(Center).Stepped &&											// брюшной сенсор уже нашупал опору
		me()->GetPredictedLODLevel() <2 &&								// модель сблизи, вдали нах сложности
		(Fallen || (MyrOwner()->FrameCounter & 3) == ATurnToCompare))	// своя очередь, для оптимизации считается раз в 4 такта
	{
		//глубина трассировки вниз
		float Depth = (1 + Crouch) * 1.2;

		//костыль, чтобы ноги всегда доставали, если достает брюхо
		if (LMB(Center).Stepped && !Foot.Stepped) Depth *= 1.3;

		//сюда скидывать результаты
		FHitResult Hit(ForceInit);
		if (Trace(Start, CurFootRay * Depth, Hit))
		{
			//сохранить часть данных сразу в ногу
			auto FM = Hit.PhysMaterial.Get();
			if (FM) Foot.Surface = (EMyrSurface)FM->SurfaceType.GetValue();

			//дополнение случая приземления через ногу
			if (IS_MACRO(MyrOwner()->CurrentState, AIR)) Fallen = true;

			//сохранить часть данных сразу в ногу
			Foot.Floor = Hit.Component.Get()->GetBodyInstance(Hit.BoneName);
			Foot.Stepped = STEPPED_MAX;
			EndPoint = Hit.ImpactPoint;
			StandHardness += 100;

			//вариация размаха ног только во время шага
			if (MyrOwner()->MoveGain > 0 || Fallen)
			{
				//насколько ногу надо отставить от сердеины (+) или вжать под себя(-)
				float CenterUneven = LMB(Center).ImpactNormal | Lateral;
				float FootUneven = (FVector3f)Hit.ImpactNormal | Lateral;
				SwingAlpha = FMath::Max(CenterUneven + FMath::Abs(CenterUneven - FootUneven), 0.0f);
				SpreadAlpha = DeltaTime * 4 * FMath::Min(FMath::Abs(SpeedAlongFront()) * 0.01f, 1.0f);
			}
			else SpreadAlpha = 0;

			//обновление вектора на ногу из посчитанных данных, реальная нога будет немного сдвинута по тренду раскоряки
			CurFootRay = (FVector3f)(Hit.ImpactPoint - Start);
		}
		//луч не нащупал опоры под этой ногой
		else
		{
			//дать шанс на один кадр еще побыть частью опоры
			if (Foot.Stepped > 1) Foot.Stepped = 1;

			//на воздухе пусть будет вариация между размахами ног через уровень вертикальности
			SwingAlpha = FMath::Max(LAXIS(LMB(Center), Up).Z, 0.0f);
			SpreadAlpha = 0.5;
		}
	}
	//просчёт по простому пути без трассировки
	else
	{
		//принять посчитанные данные из центра пояса
		if (LMB(Center).Stepped == STEPPED_MAX)
		{	StandHardness += 100;
			Foot.TransferFrom(LMB(Center));
		}
	}

	// если до этого нога была в воздухе, есть вероятность, что мы падали с высоты
	if (Fallen)
	{	float Severity = me()->ShockResult(Foot, me()->BodySpeed(LMB(Center)).Z, LMB(Center).Floor->OwnerComponent.Get());
		Foot.Damage += Severity;
		if (Severity > 0.0f)
			MyrOwner()->Hurt(Foot.WhatAmI, Severity, EndPoint, LMB(Center).ImpactNormal, Foot.Surface);
	}


	//самое желаемое положение ноги без оглядки на реальные точки земли
	FVector3f DesiredRay = (FVector3f)(CentralImpact + (FVector)Lateral * FMath::Lerp(SwingMin, SwingMax, SwingAlpha) - Start);

	//новый вектор (плечо - ступня) в абсолютных координатах
	CurFootRay = FMath::Lerp(CurFootRay, DesiredRay, SpreadAlpha);

	//костыль, чтоб не качались, при жесткой вертикали резко поддержать вертикальность в плоскости хода
	if (Vertical) CurFootRay = FVector3f::VectorPlaneProject(CurFootRay, GuidedMoveDir);

	//загиб ступни вперед/назад
	Foot.FootPitch() = 0.5 + ((FVector3f)(EndPoint - Start) | GuidedMoveDir) / 2 / TargetFeetLength;

	//запоковка в локальные координаты спины
	SetFootRay(Foot, CurFootRay);


}


//==============================================================================================================
//физически подпрыгнуть этим поясом конечностей
//==============================================================================================================
void UMyrGirdle::PhyPrance(FVector3f HorDir, float HorVel, float UpVel)
{
	//центральная часть тела пояса конечностей
	auto& CLimb = LMB(Center);

	//когда тело на сильно крутой поверхности, например карабкается, то привычное "горизонтальное" направление
	//получаемое из вектора атка/взгляда, неправильно, и его нужно сделать локальным к телу
	FVector3f VerDir(0,0,1);
	if(CLimb.ImpactNormal.Z < 0.5)
	{ 	HorDir = FVector3f::VectorPlaneProject(HorDir, CLimb.ImpactNormal);
		HorDir.Normalize();
		VerDir = CLimb.ImpactNormal;
	}

	//насколько главное тело пояса лежит на боку - если сильно на боку, то прыгать трудно
	float SideLie = FMath::Abs(LAXIS(CLimb,Right) | CLimb.ImpactNormal);
	float UnFall = 1.0 - SideLie;

	//суммарное направление прыжка
	FVector3f Dir = HorDir * HorVel + VerDir * UpVel;

	//резко отключить пригнутие, разогнуть ноги во всех частях тела
	Crouch = 0.0f;

	//применить импульсы к туловищу	// ослабление, связанное с лежанием, не применяется, чтобы мочь встать, извиваясь
	me()->PhyJumpLimb(LMB(Left), Dir);
	me()->PhyJumpLimb(LMB(Right), Dir);
	me()->PhyJumpLimb(LMB(Spine), Dir);
	me()->PhyJumpLimb(CLimb, Dir);
	UE_LOG(LogMyrPhyCreature, Log, TEXT("%s: PhyPrance horV=%g, upV=%g"), *GetOwner()->GetName(), HorVel, UpVel);

	//явно, не дожидаясь ProcessLimb, обнулить жесткость стояния, чтобы в автомате состояний поведения не переклинивало
	CLimb.EraseFloor();
	StandHardness = 0;
}

//==============================================================================================================
//выдать все уроны частей тела в одной связки - для блюпринта обновления худа
//==============================================================================================================
FLinearColor UMyrGirdle::GetDamage() const
{
	return FLinearColor 
		(	CLMB(Spine) .Damage + CLMB(Center).Damage,
			CLMB(Left)  .Damage,
			CLMB(Right) .Damage,
			CLMB(Tail)  .Damage
		);
}
