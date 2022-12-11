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

//для сокращения, а то слишком многословоно
#define PROJ(V,N) V = FVector3f::VectorPlaneProject(V,N)
#define XY(V) FVector2f(V)
#define UP FVector3f::UpVector
#define L01(X) FMath::Clamp(X, 0.0f, 1.0f)
#define LIM0(X) FMath::Max(0.0f, X)
#define LIM1(X) FMath::Min(1.0f, X)

#define LRP(X, A, Y) X = FMath::Lerp(X, Y, A)

//статический, таблица поиска абсолютного идентификатора части тела по его относительному идентификатору ("лучу")
ELimb UMyrGirdle::GirdleRays[2][(int)EGirdleRay::MAXRAYS] =
{ {ELimb::PELVIS,		ELimb::L_LEG,	ELimb::R_LEG,	ELimb::LUMBUS,	ELimb::TAIL},
	{ELimb::THORAX,		ELimb::L_ARM,	ELimb::R_ARM,	ELimb::PECTUS,	ELimb::HEAD}
};

//выдать членик и его физ-тело в системе коориднат конкретно этого пояса
FLimb&			UMyrGirdle::GetLimb (EGirdleRay R)			{ return me() -> GetLimb		(GetELimb(R)); }
const FLimb&	UMyrGirdle::GetLimb (EGirdleRay R) const	{ return mec() -> GetLimb		(GetELimb(R)); }
FBodyInstance*  UMyrGirdle::GetBody (EGirdleRay R)			{ return me() -> GetMachineBody	(GetELimb(R)); }
FBodyInstance*  UMyrGirdle::GetBody (EGirdleRay R) const	{ return mec()-> GetMachineBody (GetELimb(R)); }



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
#define BDYLOC(R)	me()->GetMachineBody(GirdleRays[IsThorax][(int)EGirdleRay::R])->GetUnrealWorldTransform().GetLocation()
#define CBDY(R)		mec()->GetMachineBody(GirdleRays[IsThorax][(int)EGirdleRay::R])

#define LAXIS(L,V) me()->GetLimbAxis##V##(L)
#define BAXIS(B,V) me()->GetBodyAxis##V##(B)

//==============================================================================================================
//конструктор
//==============================================================================================================
UMyrGirdle::UMyrGirdle (const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//инициализация битовых полей
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
	LMB(Center).Floor.Normal = BAXIS(BDY(Center), Up);
	GuidedMoveDir = BAXIS(BDY(Center), Forth);

	//фрейм в мире самого членика = его локальные оси в его же локальных координатах, не меняются
	CI->PriAxis1 = (FVector)MyrAxes[(int)me()->MBoneAxes().Up];
	CI->SecAxis1 = (FVector)MyrAxes[(int)me()->MBoneAxes().Forth];
	CI->Pos1 = FVector(0,0,0);

	//фрейм в мире окружения = первичная ось=верх, вторичная=вперед, пока неясно, нужно ли смещать позицию от нуля
	CI->PriAxis2 = FVector(0,0,1);	//вверх, вертикаль
	CI->SecAxis2 = FVector(-1,0,0);	//вперед почему тут минус, ВАЩЕ ХЗ Я В АХУЕ, но без него при ограничении по этой оси существо переворачивается
	CI->Pos1 = FVector(0,0,0);

	//окончательно инициализировать привязь, возможно, фреймы больше не будут меняться
	CI->InitConstraint(BDY(Center), GetBodyInstance(), 1.0f, this);

	//привязь, удерживающая спину в заданном положении
	me()->SetFreedom(CI, TargetFeetLength, TargetFeetLength, TargetFeetLength, 180, 180, 180);
	CI->SetLinearPositionDrive(false, false, false);
	CI->SetLinearDriveParams(6000, 100, 10000);
	CI->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
	CI->SetAngularDriveParams(9000, 2000, 10000);
	CI->SetOrientationDriveTwistAndSwing(false, false);


	//////////////////////////////////////////////////
	Super::BeginPlay();

	//сразу все лимиты и привязи сбросить
	AllFlags() = 0;
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
	PackLegLine(LMB(Right), BAXIS(BDY(Center), Down) * TargetFeetLength);
	PackLegLine(LMB(Left), BAXIS(BDY(Center), Down) * TargetFeetLength);
}

//==============================================================================================================
//позиция откуда нога растёт, просто транслирует функцию из Mesh
//==============================================================================================================
FVector UMyrGirdle::GetFootRootLoc(ELimb eL)
{
	return me()->GetLimbShoulderHubPosition(eL);
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
FVector3f UMyrGirdle::UnpackLegLine(FVector3f LL)
{	return 
		BAXIS(BDY(Spine), Up)		* LL.X +
		BAXIS(BDY(Spine), Forth)	* LL.Y +
		BAXIS(BDY(Spine), Right)	* LL.Z;
}

//==============================================================================================================
//загрузить новый радиус вектор ноги из точки касания с опорой
//==============================================================================================================
FVector3f UMyrGirdle::PackLegLine(FVector3f AbsRay)
{	return FVector3f(
		(BAXIS(BDY(Spine), Up) | AbsRay),
		(BAXIS(BDY(Spine), Forth) | AbsRay),
		(BAXIS(BDY(Spine), Right) | AbsRay));
}

//==============================================================================================================
//позиция точки проекции ноги на твердь
//==============================================================================================================
FVector UMyrGirdle::GetFootVirtualLoc(FLimb& L)
{ return GetFootRootLoc(L.WhatAmI) + (FVector)UnpackLegLine(L); }

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
//скорее всего уже не нужно, мах ногами полностью виртуален
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
bool UMyrGirdle::AdoptDynModel(FGirdleDynModels& Models)
{
	//если не используется поданная модель,берется нижележащая по приоритету
	FGirdleDynModels* ExtIfNotUse = MyrOwner()->GetPriorityModel(IsThorax);

	//если текущая модель просто для порядка следования и не используется
	if (!Models.Use)
		CurrentDynModel = MyrOwner()->GetPriorityModel(IsThorax);

	//сохранить связку, так как в процессе понадобится уточнять
	else CurrentDynModel = &Models;

	//влить почленно
	me()->AdoptDynModelForLimb(LMB(Center), CurrentDynModel->Center, CurrentDynModel->CommonDampingIfNeeded);
	me()->AdoptDynModelForLimb(LMB(Spine), CurrentDynModel->Spine, CurrentDynModel->CommonDampingIfNeeded);
	me()->AdoptDynModelForLimb(LMB(Tail), CurrentDynModel->Tail, CurrentDynModel->CommonDampingIfNeeded);

	//внедрить непосредственно флаги ограничений в зависимости от приземленности
	AllFlags() = LMB(Center).Floor.IsValid() ? CurrentDynModel->AllFlagsFloor() : CurrentDynModel->AllFlagsAir();

	//запретить ногам махать вдоль спины - сомнительная надобность, опасно
	SetSpineLock(CurrentDynModel->LegsSwingLock);
	return CurrentDynModel->Use;
}


//==============================================================================================================
//трассировка лучом для установление опоры, критерий - наличие компонента и материала
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
	bool R = Hit.Component.IsValid() && Hit.PhysMaterial.IsValid();
	if (R)	
	{
		LINEWT(ELimbDebug::LineTrace, Start + (FVector)Dst * Hit.Time, FVector(0), 1, 0.5);
		LINEWT(ELimbDebug::LineTrace, Start, FVector(0), 1, 0.5);
	}
	else	LINEWT(ELimbDebug::LineTrace, Start, (FVector)Dst, 0.5, 0.5);
	return R;
}

//==============================================================================================================
//прощупать почву под положение ног
//==============================================================================================================
bool UMyrGirdle::SenseForAFloor(float DeltaTime)
{
	//сюда скидывать результаты
	FHitResult Hit(ForceInit);

	//противоположный пояс и направление на него - нужен при расчётё ведомого
	auto Opposite = MyrOwner()->GetAntiGirdle(this);
	FVector3f ToOpp = MyrOwner()->SpineVector * (IsThorax ? -1 : 1);
	FVector3f& MDir = MyrOwner()->MoveDirection;
	FVector3f& ADir = MyrOwner()->AttackDirection;

	//существо хочет и может двигаться (реакция на WASD, кроме случаев усталости и смерти)
	bool Mobile = (MyrOwner()->MoveGain > 0.01);

	//признак серьезного вписывания в стену, определяется в п.3
	bool BumpedIn = false;

	//в предыдущий кадр состояние всего тело определялось как в воздухе (летим, падаем)
	//нужно для детекции преземления у второй ноги, когда первая уже привели пояс в режим приземленности
	bool WasInAir = IS_MACRO(MyrOwner()->CurrentState, AIR);

	//стартовая точка трассировки
	FLimb& StartLimb = LMB(Center);

	//локальные оси центрального членика, тупо чтобы короче писать и чтобы не корчевать трансформацию
	FVector3f LocUp = LAXIS(StartLimb, Up);
	FVector3f LocFront = LAXIS(StartLimb, Forth);

	//вектора ориентации якоря, должны быть ортонормированы
	FVector3f DueUp = LocUp;
	FVector3f DueFront = LocFront;
	
	FVector RealLoc = BDYLOC(Center);			// реальный центр физического пояса лошарика, ерзает из-за физики, но помогает расслабить позвоночник реальой позой
	FVector StableLoc = GetComponentLocation(); // центр якоря, кинематический, положение, в котором тело не съезжает
	FVector DueLoc;								// сюда будет сливаться финальная позиция якоря

	// 1 =========== ПЕРЕРАСЧЁТ ТОГО, ЧТО НЕ ЗАВИСИТ ОТ ТРАССИРОВКИ ==================
	//################################################################################

	//текущий уровень прижатия к земле
	Crouch = FMath::Lerp(Crouch, CurrentDynModel->Crouch, LIM1(DeltaTime * 5));

	//устойчивость = обнулить с прошлого кадра + внести слабый вклад других частей, чтобы при лежании на боку было >0
	StandHardness = 
		FMath::Max(0, (int)LMB(Spine).Stepped - STEPPED_MAX + 20) + 
		FMath::Max(0, (int)LMB(Tail).Stepped - STEPPED_MAX + 15);

	//поверхность пригодна для движения = она пологая или по ней можно карабкаться, с предыдущего кадра, далее перерассчитается
	bool Steppable = false;

	//абсолютная скорость в поясе, для расчёта скорости относительной
	FVector3f RelativeSpeedAtPoint = me()->BodySpeed(LMB(Center));

	// 2 ============= ТРАССИРОВКА, ПРИНЯТИЕ ОПОРЫ ===================================
	//################################################################################

	//направление трассировки - по умолчанию по нормали (если в воздухе нормаль равно локальной оси таза)
	FVector3f TraceTo = Vertical ? FVector3f::DownVector : -StartLimb.Floor.Normal;

	//необходимое поднятие над землей, норма с учётом подогнутия, однако если карабкаемся, считаем по максимуму, чтобы не упасть от случайного недоставания
	float Elevation = GetTargetLegLength();

	//прощупать опору - в результате опора найдена
	Trace(StableLoc, TraceTo * Elevation * 1.2, Hit);

	//сборка пола из трассировки, здесь Floor.Normal уже сразу присваивается
	FFloor NewFloor(Hit);

	//сразу просканировать ноги для получения альтернативных выборок опоры, и заменить, если они лучше
	ProbeFloorByFoot(DeltaTime, LMB(Left),  2 * IsThorax,		NewFloor,	Hit.ImpactPoint);
	ProbeFloorByFoot(DeltaTime, LMB(Right), 2 * IsThorax + 1,	NewFloor,	Hit.ImpactPoint);

	//прощупать опору - в результате опора найдена
	if (NewFloor)
	{
		//сила удара, урон (скорость берется по оси нормали, чтобы избежать касательных быстрот, но общий модуль тоже учитывается)
		float FallSeverity = me()->ShockResult(StartLimb, (VelocityAgainstFloor | (-NewFloor.Normal)) + Speed * 0.1, NewFloor);
		if (FallSeverity > 0.8f)
			MyrOwner()->Hurt(StartLimb, FallSeverity * 0.1, Hit.ImpactPoint);

		//типа если до этого были в воздухе, а сейчас на земле, то упали, припружинить вниз
		if(!StartLimb.Floor) Crouch = 1;

		//определить нужду и возможность карабкаться
		Climbing = MyrOwner()->bClimb				// главное условие, извне идет запрос на карабканье
			&& NewFloor.IsClimbable()				// поверхность позволяет зацепиться
			&& (Lead() || Opposite->Climbing);		// ведомый пояс может цепляться только если ведущий уже зацеплен = сначала передними, потом задними

		//если поверхность крутая, не удержаться, однако помечена ступабельной на уровне компонента
		if (NewFloor.TooSteep() && NewFloor.CharCanStep())					
		{
			float SlideBy = MDir|NewFloor.Normal;	// насколько не вбок к крутой поверхности мы двигаемся
			if (SlideBy > 0.4 && SlideBy < -0.5)	// особый случай, когда мы сходим с уступа (нормаль близка к нашему курсу)
				Steppable = true;
			else if (SlideBy > 0.2)
				FixedOnFloor = false;//◘◘
			else Steppable = Climbing;				// если не спускаемся, а склон крутой, то все же можем по нему карабкаться
		}
		else Steppable = true;						// пологий склон ничего не мешает идти

		//исключить аномальные случаи, когда центр якоря уже погрузился в стену и трассирует оттуда - нормаль будет левая, надо его сначала вытащить
		Steppable &= !Hit.bStartPenetrating;

		//пригодная для движения поверхность - окончателно сохранить в тело
		if (Steppable)
		{
			AllFlags() = CurrentDynModel->AllFlagsFloor();
			StartLimb.AcceptFloor(NewFloor, 1.0f);
		}
		//новая опора непригодна для движения
		else
		{
			//сорваться/соскользнуть, если уже лезем по вертикали
			Climbing = false;
		}

		//если пол движется, внести его скорость в собственное представление о скорости
		if(NewFloor.IsMovable())
			RelativeSpeedAtPoint -= NewFloor.Speed(Hit.ImpactPoint);
	}
	//опора не найдена или найдена совсем уж плохая
	else
	{
		//удалить опору из пояса и обеих ног и снять флаги фиксаций
		StartLimb.EraseFloor();
		LMB(Right).EraseFloor();
		LMB(Left).EraseFloor();
		Climbing = false;
		Steppable = false;
		AllFlags() = CurrentDynModel->AllFlagsAir();

		//куда устрамлять ненужную нормаль: на нормаль соседнего пояса, на нормаль своего спинного членика или на собственный верх
		FVector3f DstNormal = LocUp;
		if (Opposite->Climbing) DstNormal = Opposite->GetCenter().Floor.Normal; else
		if (GetSpine().Floor && !GetSpine().Floor.TooSteep()) DstNormal = GetSpine().Floor.Normal;
		LRP(StartLimb.Floor.Normal, LIM1(DeltaTime*5), DstNormal);

		//норм. норм. а то накапливаются искажения
		StartLimb.Floor.Normal.Normalize();
	}

	// 3 ============= РАСЧЁТ ВЕКТОРА ДВИЖЕНИЯ ======================================
	//################################################################################

	//тело испытывает движение (только в движении вектор курса имеет большой смысл)
	if(Mobile)
	{
		//если имеется надёжная опора, а не просто когтеточка
		if(Steppable)
		{
			//поддержка движения вдоль ветки
			if (StartLimb.Floor.IsCapsule())
			{
				auto DirAlong = StartLimb.Floor.Dir (GuidedMoveDir);		// единичный вектор вдоль ветки
				auto MoveCoaxis = StartLimb.Floor.Coax(MDir);				// сонаправленность ранее посчитанного курса и линии ветки
				auto LookCoaxis = StartLimb.Floor.Coax(ADir);				// сонаправленность ранее посчитанного курса и линии ветки
				auto ThickAsMe = MyrOwner()->SpineLength * 0.25;			// толщина ветки с самым сильным ограничением
				float SupportVal = FMath::Abs(LookCoaxis);					// сила поддержки вдоль ветки
				float DimOnVertical = 1 - DirAlong.Z * DirAlong.Z;			// не сдерживать, если ползём по вертикали, чтоб уметь обползать вокруг ствола
				float BackUpForce = (1 - StartLimb.Floor.Normal.Z);			// сила тяги вверх, чтобы устремлять и удерживать на вершине ветки 

				//добавить к курсу вектор вдоль ветки, для поддержки, чтоб не упасть
				GuidedMoveDir += DirAlong * SupportVal * DimOnVertical;

				//добавить к курсу вектор вверх, чтобы вытянуть пояс на самый верх, на седло ветки
				GuidedMoveDir += UP * BackUpForce * DimOnVertical;
			}
		}

		//ведущий пояс, активное следование курсу
		if(Lead())
		{
			//прибавить курс к нулю (если предкоррекции не делали) или к уже добавленному движению вдоль ветки
			//видимо, в этот же MDir, при траекторном движении в воздухе, надо класть касательную параболы
			GuidedMoveDir += MDir;

			//переход к вознесению по стенке при упирании в нее
			if(StartLimb.Floor.Normal.Z < 0.5)
			{	float GoUpOrDown = FMath::Sign(ADir.Z + 0.7f);				// при взгляде сильно вниз, курс выворачивается вниз, а не вверх
				float SkipSmallBumps = Steppable ? 0.4 : 0.7;				// вычитаемое, что занулит мелкие и случайные тычки в стену
				float BumpInAmount = XY(-StartLimb.Floor.Normal)|XY(MDir);	// упёртость в крутизну с плоскости
				float RiseAmount = LIM0(BumpInAmount - SkipSmallBumps);		// коэффициент тяги вверх

				//выворот вектора курса на вертикаль
				GuidedMoveDir.Z += (Climbing ? 2 : 1) * RiseAmount * GoUpOrDown;	
			}
		}
		//ведомый - ориентацию на ведущего прибавить к нулю или к уже добавленному движению вдоль ветки
		else GuidedMoveDir += ToOpp;										
	}
	//вектор курса для wannabe неподвижного пояса
	else
	{
		if(!FixedOnFloor)
			GuidedMoveDir += 0.1*LocFront;
	}

	//окончательное формирования вектора курса, универсальное
	BumpedIn = me()->GuideAlongObstacle(GuidedMoveDir,	// уложить или отразить курс от стены, в которую уперлись
		Hit.bStartPenetrating);							// особенно если якорь проник туда полностью
	if (Steppable)
		PROJ(GuidedMoveDir, StartLimb.Floor.Normal);	// дополнительно уложить курс в плоскости опоры
	GuidedMoveDir.Normalize();							// нормализовать, чтобы можно было сравнивать единичные
	
	// 4 ============= РАЗРЕШЕНИЕ ПРОБЛЕМ ВЕДОМОГО ПОЯСА =============================
	//################################################################################
	if (!Lead())
	{
		//разрешение случая, когда ведомый в результате обтеканий направился против ведущего, что ведет к разрыву спины
		float UnSplit = Opposite->GuidedMoveDir | GuidedMoveDir;	// соосность ведущего и ведомого поясов, чем меньше, тем больше раскоряка
		float SeverMinLim = Climbing ? 0.0f : -0.5;					// самая худшая соосность, при которой насупает срыв ведомого пояса
		float SafeCoaxis = SeverMinLim + Speed * 0.002;				// порог несоосности курсов поясов, чем быстрее, тем раньше наступает срыв
		float ExtBumpInfl = me()->Bumper1().GetBumpCoaxis() * 0.4f;// при внешней упертости срыв наступает еще раньше

		//критерии срыва ведомого пояса с опоры на свободу в рамках физдвижка
		if (UnSplit <= (SafeCoaxis + ExtBumpInfl) || 			// несоосность курсов слишком велика ИЛИ
			(Opposite->Climbing && !Climbing))					// костыль, оторваться от земли, если верх уже зацепился за дерево, иначе распидорасит
				FixedOnFloor = false;//◘◘
		//else if(Opposite->GetLimb(EGirdleRay::Center).Floor.TooSteep())
		//	FixedOnFloor = false;//◘◘
	}

	// 5 ============= РАСЧЁТ ОСЕЙ ОРИЕНТАЦИИ ЯКОРЯ ==================================
	//################################################################################

	//если требуется поддержка вертикали, первичен вектор вверх, вперед расчитывается из него
	if(Vertical)
	{	DueUp = UP;
		DueFront = PROJ(GuidedMoveDir, DueUp);
		DueFront.Normalize();
	}
	//прочие позиции без вертикали, но с креплением к опоре
	else if(FixedOnFloor)
	{	DueFront = GuidedMoveDir;					// присваиваем перед=курс, верх=нормаль, 
		DueUp = StartLimb.Floor.Normal;				// они скорее всего уже ортогональны
		if (FMath::Square(DueUp|DueFront) > 0.01)	// но если они всё же не ортогональны - ортогонализировать
		{	FVector3f TempLat = DueUp ^ DueFront;	// вектор в сторону, честно ортогональный и нормали, и курсу
			TempLat.Normalize();					// если не ортогональны, этот вектор не единичен и его надо нормализовать
			if (CurrentDynModel->NormalAlign)		// если первична нормаль, то правим курс
			{	DueFront = DueUp ^ TempLat;
				if ((DueFront | GuidedMoveDir) < 0)
					DueFront = -DueFront;
			}
			else									// прочих случаях сохраняем точный курс и правим вектор вверх
			{	DueUp = DueFront ^ TempLat;
				if ((DueFront | StartLimb.Floor.Normal) < 0) DueUp = -DueUp;
			}
		}
	}
	//else: оси уже присвоены при объявлении переменных

	// 6 ============= РАСЧЁТ ПРИРАЩЕНИЯ КИНЕМАТИЧЕСКОГО ДВИЖЕНИЯ=====================
	//################################################################################

	//шаг движения: для управляемой тяги просто курс, иначе для самодействий, где курс движения - настройка из модели
	auto MoveShift = MyrOwner()->GetDesiredVelocity() * DeltaTime *
		(me()->DynModel->MoveWithNoExtGain ? me()->CalcOuterAxisForLimbDynModel(StartLimb) : GuidedMoveDir);

	//учёт относительного движения пола, то есть если пол движется, то и тело двигать с этой скоростью вне зависимости от тяги
	if (StartLimb.Floor)
		if (StartLimb.Floor.IsMovable())
			MoveShift -= StartLimb.Floor.Speed(Hit.ImpactPoint);

	// 7 ============= РАСЧЁТ НОВОГО ПОЛОЖЕНИЯ ЯКОРЯ =================================
	//################################################################################

	//сюда будет записываться отклонение физического позвонка от кинематического якоря - признак уткнутости/застрялости
	float PosErr =  (StableLoc - RealLoc).Size() / (TargetFeetLength * 0.2);

	//сложное позиционирование якоря только для режимов фиксации тела на якоре
	if (FixedOnFloor)
	{
		//для предсказуемой поверхности восстановить идеальное положение из результатов трассировки
		if (Steppable)
			DueLoc = StableLoc + (FVector)(TraceTo * (Hit.Distance - Elevation));
		else DueLoc = StableLoc;

		//применить вектор движения на отрезок, соответствующий скорости
		DueLoc += FVector(MoveShift);

		//коррекция кинематической позиции якоря по отклонению от физического членика (=натяжению зацепа)
		if (!Lead() || BumpedIn)

			//если слишком большое отклонение, вернуть якорь на место физического членика, чтоб его не унесло сквозь стены и т.п.
			DueLoc = FMath::Lerp(DueLoc, RealLoc, Lead() ? FMath::Min(PosErr, 0.7f) : FMath::Clamp(PosErr, 0.3f, 0.7f));
	}
	//не цепляемся за опору, возможно, вообще в воздухе, послушно ставим якорь в позицию физ-членика, который будет его тащить
	else DueLoc = RealLoc;

	// 8 ============= ОБНОВЛЕНИЕ ПРИВЯЗИ ТЕЛА К ЯКОРЮ ==============================
	//################################################################################

	//подстройка притяжения к зацепу
	auto CI = BDY(Center)->DOFConstraint;

	//кинематически переместить якорь в новое нужное место с нужным углом поворота
	SetWorldTransform(FTransform((FVector)DueFront, (FVector)(DueFront^ DueUp), (FVector)DueUp, DueLoc));

	//включение сил, линейно прижимающих спину к якорю, действует только если режим цепа к опоре
	CI->SetLinearPositionDrive(FixedOnFloor, FixedOnFloor, FixedOnFloor);

	//силы, ограничивающие углы по рысканью влево-вправо, и по кренам вперед/назад и по сторонам
	CI->SetOrientationDriveTwistAndSwing(TightenYaw, TightenLean);

	//жесткое ограничение по углам
	me()->SetAngles(CI, LockYaw ? 0 : 180, LockRoll ? 0 : 180, LockPitch ? 0 : 180);

	// 9 ============= ВСЯКОЕ ==============================
	//################################################################################

	//обновление вектора и скаляра скорости
	VelocityAgainstFloor = FMath::Lerp(VelocityAgainstFloor, RelativeSpeedAtPoint, 0.3);
	Speed = VelocityAgainstFloor.Size();

	//если мы только включили режим карабканья и что-то не получилось, запустить вспомогательные действиия
	if (MyrOwner()->bWannaClimb) MyrOwner()->ClimbTryActions();

	// 10 ============= ОТЛАДКА ЛИНИЯМИ  ==============================
	//################################################################################

	//отладка устойчивости - только после обработки ног
	LINELIMBWTC(ELimbDebug::GirdleStandHardness, StartLimb, (FVector)StartLimb.Floor.Normal * 15, 1, 0.03, StandHardness / 255.0f);

	//отладка напряженности центрального зацепа
	if (LDBG(ELimbDebug::CentralConstrForceLin) || LDBG(ELimbDebug::CentralConstrForceAng))
	{	FVector Lin, Ang;
		CI->GetConstraintForce(Lin, Ang);
		LINELIMBWT(ELimbDebug::CentralConstrForceAng, StartLimb, Ang * 105, 1, 0.04f);
		LINELIMBWT(ELimbDebug::CentralConstrForceLin, StartLimb, Lin * 105, 1, 0.04f);
	}

	LINELIMBWTC(ELimbDebug::PhyCenterDislocation, StartLimb, (FVector)(GetComponentLocation() - BDYLOC(Center)), 1, 0.04f, PosErr);

	//индикация направления
	LINELIMBWT(ELimbDebug::GirdleGuideDir, StartLimb, (FVector)GuidedMoveDir * 15, 1, 0.04f);
	LINELIMBWTC(ELimbDebug::GirdleCrouch, StartLimb, (FVector)StartLimb.Floor.Normal * 20, 1, 0.04f, Crouch);

	//сверху от туловище рисуем тестовые индикаторы
	if (LDBG(ELimbDebug::GirdleConstrainMode))
	{
		auto Ct = BDY(Center)->GetCOMPosition() + StartLimb.Floor.Normal * 15;
		auto Color = FColor(255, 0, 255, 10);
		float ang = CI->ProfileInstance.ConeLimit.Swing1LimitDegrees;
		float ang2 = CI->ProfileInstance.TwistLimit.TwistLimitDegrees;
		if (Steppable)		DrawDebugString(GetWorld(), (FVector)Ct, Hit.bStartPenetrating ? TEXT("P") : TEXT("S"), nullptr, FColor(255, 0, 255*Hit.bStartPenetrating), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (Climbing)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("_C"), nullptr, FColor(0, 255, 255), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (Vertical)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("__V"), nullptr, FColor(ang, 255, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (TightenYaw)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("____Ty"), nullptr, FColor(255, 0, ang2), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (TightenLean)	DrawDebugString(GetWorld(), (FVector)Ct, TEXT("______Tl"), nullptr, FColor(150, 200, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (FixedOnFloor)	DrawDebugString(GetWorld(), (FVector)Ct, TEXT("________F"), nullptr, FColor(255, 255, 255), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (LockPitch)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("__________p"), nullptr, FColor(255, 255, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (LockYaw)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("___________y"), nullptr, FColor(255, 255, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (LockRoll)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("____________r"), nullptr, FColor(255, 255, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (BumpedIn)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("_____________B ")+TXTENUM(ELimb, me()->Bumper1().WhatAmI), nullptr, FColor(0, 255 * me()->Bumper1().Colinea, 255), 0.02, false, Lead() ? 1.5f : 1.0f);
		//DrawDebugString(GetWorld(), (FVector)(Ct+StartLimb.Floor.Normal*15), FString::SanitizeFloat(PosErr), nullptr, FColor(255, 255, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
	}
	return false;
}

//==============================================================================================================
//прощупать трасировкой позицию ступни, если применимо
//==============================================================================================================
bool UMyrGirdle::ProbeFloorByFoot(float DeltaTime, FLimb& Foot, int ProperTurn, FFloor& MainFloor, FVector &AltHitPoint)
{
	//центральный членик, туловище
	FLimb& CNTR = LMB(Center);

	//такт выделенный на полный просчёт этой ноги
	bool MyTime = ((MyrOwner()->FrameCounter & 3) == ProperTurn);

	//момент приземления, возможно, больный
	bool JustLanded = !CNTR.Floor.IsValid() && MainFloor.IsValid();
	
	//критерий обновления желаемой позы и, возможно, подтверждения ее с помощью трассировки
	if (JustLanded || MyTime)
	{
		// 2 ============= ПЕРЕРАСЧЁТ ЖЕЛАЕМОЙ ЛИНИИ НОГИ ========================
		//################################################################################

		//для дальних существ трассировку нафиг
		bool CanEverTrace = (me()->GetPredictedLODLevel() < 2);

		//точка начала трассировки, уровень плечь или таза
		FVector Start = GetFootRootLoc(Foot.WhatAmI);

		// состояние движения
		bool Mobile = (MyrOwner()->MoveGain > 0);

		//желаемый размах ног и скорость его обновления
		float Swing = 0.0;
		float SwingAlpha = 0.1;

		//если нога стоит на опоре
		if (Foot.Floor)
		{
			FVector3f Lateral = LAXIS(CNTR, Right) * mRight(Foot);// вектор в сторону, для разных ног отличается знаком
			float FootUneven = Foot.Floor.Normal | Lateral;		// кривизна в ноге (+вып, -вог)
			float CenterUneven = CNTR.Floor.Normal | Lateral;	// крен в центре, для обеих ног идентичен до знака
			float FloorSlant = CNTR.Floor.Normal.Z;				// влияние наклона в центре, __сжать |расставить

			//поскольку вычисляем коэффициент раскоряки, берем все показатели сжатия с минусом и переводим в [0;+1]
			Swing = L01(0.5 - 0.5 * (FloorSlant + CenterUneven + FootUneven));

			//скорость приближения к желаемому отгибу ноги пропроциональна скорости
			SwingAlpha = LIM1(DeltaTime * 8 * Speed * Speed * 0.0001f);
		}
		//опоры нет, или не достали трассировкой, или с прошлого кадра продолжает быть опора
		else
		{
			Swing = LIM0(LAXIS(CNTR, Up).Z);		// на воздухе пусть будет вариация между размахами ног через уровень вертикальности
			SwingAlpha = LIM1(1.5 * DeltaTime);		// скорость сдвига/раздвига ног постоянная
		}

		//подготовить целевой вектор линии ноги, оименовать компоненты, чтоб было понятно
		FVector3f LocLineToBe;
		float& DesiredUpDown = LocLineToBe.X;
		float& DesiredForeBack = LocLineToBe.Y;
		float& DesiredLeftRight = LocLineToBe.Z;

		//компонента влево/вправо, определяется удобной расставкой ног, разные ноги отличаются знаком
		DesiredLeftRight = CurrentDynModel->GetLegSpread(Swing) * mRight(Foot);
		if (CNTR.Floor) if (CNTR.Floor.Thin()) DesiredLeftRight = mRight(Foot);

		//компонента вперед/назад, отражает либо растяжку ног вместе с центром, или строгую вертикаль
		DesiredForeBack = (Vertical ? -UP : LAXIS(CNTR, Down)) | LAXIS(LMB(Spine), Forth);

		//компонента вверх/вниз, для простоты строго единица, а вообще нужно извлекать из нормированности
		DesiredUpDown = (Vertical ? -UP : LAXIS(CNTR, Down)) | LAXIS(LMB(Spine), Up);

		//применить ко всем компонентам линии сразу ее нормальную длину
		LocLineToBe *= GetTargetLegLength();

		//представить реальную трассированную линию ноги в локальном виде - сохранить старое значение
		FVector3f LocLineReal = GetLegLineLocal(Foot);

		//внесение маленьких отклонений в сторону идеала, теперь эта линия может использоваться для трассировки
		GetLegLineLocal(Foot) = FVector3f(
			FMath::Lerp(LocLineReal.X, DesiredUpDown, Foot.Floor ? 1 : LIM1(5 * DeltaTime)),
			FMath::Lerp(LocLineReal.Y, DesiredForeBack, Foot.Floor ? 1 : LIM1(5 * DeltaTime)),
			FMath::Lerp(LocLineReal.Z, DesiredLeftRight, SwingAlpha));

		//отладка
		LINEWT(ELimbDebug::FeetShoulders, Start, (FVector)UnpackLegLine(Foot), 1, 0);

		// 3 ============= ТРАССИРОВКА, ПОЛУЧЕНИЕ РЕАЛЬНОЙ ПОЗЫ ========================
		//################################################################################

		// брюшной сенсор уже нашупал опору, то есть у ног в принципе есть шагс правильно лечь на ту же поверхность
		if (MainFloor.IsValid())						
		{
			//модель сблизи, вдали нах лишние трассировки
			if (CanEverTrace)
			{
				//непосредственно щупать по ранее посчитанному вектору ноги, но на бОльшую длину
				FHitResult Hit(ForceInit);
				if (Trace(Start, UnpackLegLine(Foot) * 3, Hit))
				{
					//пока не присваеваем опору, детектируем приземление и считаем возможные увечья конкретно этой ноге
					FFloor NewFloor(Hit);
					if (JustLanded || (NewFloor && !Foot.Floor))
					{
						auto Severity = me()->ShockResult(Foot, (VelocityAgainstFloor | (-NewFloor.Normal)) + Speed * 0.1, NewFloor);
						if (Severity > 0.1)
							MyrOwner()->Hurt(Foot, Severity, Hit.ImpactPoint);
					}

					//теперь можно сохранить опору, она однозначно принадлежит этой ноге
					Foot.Floor = NewFloor;

					//рокировка опорами, если эта нога нащупала более выгодную точку опоры, чем брюхо, тут же внутри поменять местами
					float FootWeight = Foot.Floor.Rating(MyrOwner()->AttackDirection, LAXIS(CNTR, Up), MyrOwner()->bClimb);
					float DefWeight = MainFloor.Rating(MyrOwner()->AttackDirection, LAXIS(CNTR, Up), MyrOwner()->bClimb);
					if (FootWeight > DefWeight)
					{	AltHitPoint = Hit.ImpactPoint;
						MainFloor = Foot.Floor;
						UE_LOG(LogMyrPhyCreature, Log, TEXT("%s ProbeFloorByFoot substitute floor by %s"),
							*GetOwner()->GetName(), *TXTENUM(ELimb, (ELimb)Foot.WhatAmI));
					}

					//теперь уже конкретно для ноги
					if (Hit.Distance > TargetFeetLength * 1.2)// если до опоры дотянулись лучом, но он длинее ноги
						Foot.EraseFloor();					// убрать опору как недостижимую
					else
					{
						// сохранить опору в ногу + упаковать скорректированную (по нижнему краю) линию ноги для показа в анимации
						Foot.Floor = Hit;					
						PackLegLine(Foot, FVector3f(Hit.ImpactPoint - Start));
					}
				}
				//трасиировка вообще не нашла опоры, помечаем ногу как без опоры
				else Foot.EraseFloor(); 
			}

			//быстрый способ позиционировать ноги
			else Foot.Floor = MainFloor;
		}
	}

	// подтвердить устойчивость
	if (Foot.Floor)	StandHardness += 100;				
	return Foot.Floor.IsValid();
}

//==============================================================================================================
//переместить в нужное место (центральный членик)
//==============================================================================================================
void UMyrGirdle::ForceMoveToBody()
{
	FVector3f DueFront = BAXIS(BDY(Center), Forth);
	FVector3f DueUp = BAXIS(BDY(Center), Up);
	FVector DueLoc = BDY(Center)->GetUnrealWorldTransform().GetLocation();
	SetWorldTransform(FTransform((FVector)DueFront, (FVector)(DueFront ^ DueUp), (FVector)DueUp, DueLoc));
}

//==============================================================================================================
//полностью разорвать привязь к якорю, для мертвых
//==============================================================================================================
void UMyrGirdle::DetachFromBody()
{
	//подстройка притяжения к зацепу
	auto CI = BDY(Center)->DOFConstraint;
	me()->SetFreedom(CI, -1, -1, -1, 180, 180, 180);

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
	if(CLimb.Floor.Normal.Z < 0.5)
	{ 	PROJ(HorDir, CLimb.Floor.Normal);
		HorDir.Normalize();
		VerDir = CLimb.Floor.Normal;
	}

	//насколько главное тело пояса лежит на боку - если сильно на боку, то прыгать трудно
	float SideLie = FMath::Abs(LAXIS(CLimb,Right) | CLimb.Floor.Normal);
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
