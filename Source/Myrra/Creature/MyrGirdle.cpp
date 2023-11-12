// Fill out your copyright notice in the Description page of Project Settings.


#include "MyrGirdle.h"
#include "../MyrraGameInstance.h"						// ядро игры
#include "MyrPhyCreature.h"								// само существо
#include "MyrPhyCreatureMesh.h"							// тушка
#include "DrawDebugHelpers.h"							// рисовать отладочные линии
#include "PhysicalMaterials/PhysicalMaterial.h"			// для выкорчевывания материала поверхности пола
#include "Curves/CurveLinearColor.h"					// возможно, понадобится для прохода формы шага
#include "Engine/CurveTable.h"							// для прохода формы шага
#include "Curves/SimpleCurve.h"							// для прохода формы шага

//рисовалки отладжочных линий
#if WITH_EDITOR
#define LINE(C, A, AB) MyrOwner()->Line(C, A, AB)
#define LINEWT(C, A, AB,W,T) MyrOwner()->Line(C, A, AB,W,T)
#define LINEWTC(C, A, AB,W,T, tint) MyrOwner()->Linet(C, A, AB,tint, W,T)
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

//динамическая модель для этого пояса (часто общей дин-модели, которая указателем сохраняется в меше)
FGirdleDynModels* UMyrGirdle::DynModel() { return IsThorax ? &me()->DynModel->Thorax : &me()->DynModel->Pelvis; }

//для сокращения, а то слишком многословоно
#define PROJ(V,N) V = FVector3f::VectorPlaneProject(V,N)
#define PRJN(V,N) FVector3f::VectorPlaneProject(V,N)
#define XY(V) FVector2f(V)
#define UP FVector3f::UpVector
#define L01(X) FMath::Clamp(X, 0.0f, 1.0f)
#define LIM0(X) FMath::Max(0.0f, X)
#define LIM1(X) FMath::Min(1.0f, X)

#define LRP(X, A, Y) X = FMath::Lerp(X, Y, A)

#define FRAME (MyrOwner()->FrameCounter)

#define SPEED_TOBE (MyrOwner()->GetDesiredVelocity())

float MaxDeltaPhase = 0;
float MinDeltaPhase = 0;


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

//то же для краткости и инлайновости
#define ELMB(R)							GirdleRays[IsThorax][(int)EGirdleRay::R]
#define LMB(R)		me()->GetLimb       (GirdleRays[IsThorax][(int)EGirdleRay::R])
#define CLMB(R)		mec()->GetLimb       (GirdleRays[IsThorax][(int)EGirdleRay::R])
#define BDY(R)		me()->GetMachineBody(GirdleRays[IsThorax][(int)EGirdleRay::R])
#define BDYLOC(R)	me()->GetMachineBody(GirdleRays[IsThorax][(int)EGirdleRay::R])->GetUnrealWorldTransform().GetLocation()
#define BDYX(R)		me()->GetMachineBody(GirdleRays[IsThorax][(int)EGirdleRay::R])->GetUnrealWorldTransform().GetUnitAxis(EAxis::X)
#define BDYY(R)		me()->GetMachineBody(GirdleRays[IsThorax][(int)EGirdleRay::R])->GetUnrealWorldTransform().GetUnitAxis(EAxis::Y)
#define BDYZ(R)		me()->GetMachineBody(GirdleRays[IsThorax][(int)EGirdleRay::R])->GetUnrealWorldTransform().GetUnitAxis(EAxis::Z)
#define CBDY(R)		mec()->GetMachineBody(GirdleRays[IsThorax][(int)EGirdleRay::R])

#define LAXIS(L,V) me()->GetLimbAxis##V##(L)
#define BAXIS(B,V) me()->GetBodyAxis##V##(B)

//доступен механизм шагов
bool UMyrGirdle::HasSteps() const { return mec()->HasPhy(ELMB(Right)); }

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

	FairSteps = 0;
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
	MaxDeltaPhase = 0;
	MinDeltaPhase = 0;

}

//==============================================================================================================
//пересчитать базис длины ног через расстояние от сокета-кончика для кости-плечевого сустава
//вызывается извне перед игрой, пока тело еще в начальной позе, но уже с масштабом
//==============================================================================================================
void UMyrGirdle::UpdateLegLength()
{
	//извлечь значение
	TargetFeetLength = (TargetFeetLength == 0) ? (GetRefLegLength()) : TargetFeetLength * GetComponentScale().Z;
	//TargetFeetLength = (TargetFeetLength == 0) ? FMath::Sqrt(GetFeetLengthSq()) : TargetFeetLength * GetComponentScale().Z;

	//привязь, удерживающая спину в заданном положении
	me()->SetFreedom(BDY(Center)->DOFConstraint, TargetFeetLength, TargetFeetLength, TargetFeetLength, 180, 180, 180);
	 
	//начальные радиусы ног строго вниз по бокам тела от плеч
	PackLegLine(LMB(Right), BAXIS(BDY(Center), Down) * TargetFeetLength, TargetFeetLength);
	PackLegLine(LMB(Left), BAXIS(BDY(Center), Down) * TargetFeetLength, TargetFeetLength);
}

//==============================================================================================================
//позиция откуда нога растёт, просто транслирует функцию из Mesh
//==============================================================================================================
FVector UMyrGirdle::GetFootRootLoc(ELimb eL)
{
	return me()->GetLimbShoulderHubPosition(eL);
}

//==============================================================================================================
//ориентировочное абсолютное положение конца ноги (сокет-кончик/боёк должен быть внедрен в скелет и в генофонд!)
//используется только чтобы вычислить базовую длину ноги перед игрой
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
FVector3f UMyrGirdle::UnpackLegLine(FLegPos LL)
{	return 
		BAXIS(BDY(Center), Up)		* LL.DownUp +
		BAXIS(BDY(Center), Forth)	* LL.BackFront +
		BAXIS(BDY(Center), Right)	* LL.LeftRight;
}

//==============================================================================================================
//загрузить новый радиус вектор ноги из точки касания с опорой
//==============================================================================================================
FLegPos UMyrGirdle::PackLegLine(FVector3f AbsRay, float Lng)
{	FLegPos wtr; 
	wtr.BackFront = (BAXIS(BDY(Center), Forth)	| AbsRay);
	wtr.DownUp =	(BAXIS(BDY(Center), Up)		| AbsRay);
	wtr.LeftRight = (BAXIS(BDY(Center), Right)	| AbsRay);
	wtr.Pitch = (Lng > 0) ? Lng : AbsRay.Length();
	return wtr;
}


//==============================================================================================================
//идеальное расположение ноги для текущей позы тела, для данного пояса
//==============================================================================================================
FLegPos UMyrGirdle::idealLegLine(FLimb& Foot)
{	FLegPos LocLineReal = GetLegLine(Foot);
	return PackLegLine(IdealAbsLegLine(Foot, LocLineReal.Length()));
}

FVector3f UMyrGirdle::IdealAbsLegLine(FLimb& Foot, float Length)
{
	float Swing = 0.0;										// желаемый размах ног
	auto& CeL = LMB(Center);								// центральный членик

	//скорость подстройки - если нога стоит на опоре (получено с прошлого вызова этой функции)
	if (Foot.Floor)
	{
		FVector3f Lateral = LAXIS(CeL, Right) * mRight(Foot);// вектор в сторону, для разных ног отличается знаком
		float FootUneven = Foot.Floor.Normal | Lateral;		// кривизна в ноге (+вып, -вог)
		float CenterUneven = CeL.Floor.Normal | Lateral;	// крен в центре, для обеих ног идентичен до знака
		float FloorSlant = CeL.Floor.Normal.Z;				// влияние наклона в центре, __сжать |расставить

		//поскольку вычисляем коэффициент раскоряки, берем все показатели сжатия с минусом и переводим в [0;+1]
		Swing = L01(0.5 - 0.5 * (FloorSlant + CenterUneven + FootUneven));
	}
	//скорость подстройки - если опоры нет, или не достали трассировкой, или с прошлого кадра продолжает быть опора
	else Swing = LIM0(LAXIS(CeL, Up).Z);					// на воздухе пусть будет вариация между размахами ног через уровень вертикальности

	//сначала вычисляется абсолютная позиция, потом уже конвертируется в "спинное" пространство
	FVector3f LocLineToBe =
		LAXIS(CeL, Down) +								// базис линии ноги, единичная ось вниз центр-членика (он сам может качаться тангажом)
		LAXIS(CeL, Right) * mRight(Foot)				// вектор в сторону, для раскоряки ног
		* DynModel()->GetLegSpread(Swing); 				// уровень раскоряки, среднее между предельными долями единицы из настроек модели
	return LocLineToBe * Length;
}

//==============================================================================================================
//позиция точки проекции ноги на твердь
//==============================================================================================================
FVector UMyrGirdle::GetFootVirtualLoc(FLimb& L)
{ return GetFootRootLoc(L.WhatAmI) + (FVector)UnpackLegLine(L); }

//==============================================================================================================
//эффективная раскоряка ноги в направлении хотьбы, нужно для оценки фазы шага
//==============================================================================================================
float UMyrGirdle::LegStretch(FLimb & Foot)
{	auto& Le = GetLegLine(Foot);
	auto& CNTR = LMB(Center);
	return		0.9 * Le.BackFront * FMath::Sign(LAXIS(CNTR, Forth) | GuidedMoveDir) +
				0.3 * Le.LeftRight * FMath::Sign(LAXIS(CNTR, Right) | GuidedMoveDir);
}

//==============================================================================================================
//пересчитать реальную длину ног
//==============================================================================================================
float UMyrGirdle::GetFeetLengthSq(ELimb eL)
{	FVector Tip = GetFootTipLoc(eL);
	FVector Root = me()->GetLimbShoulderHubPosition(eL);
	return FVector::DistSquared(Root, Tip);
}
float UMyrGirdle::GetFeetLengthSq(bool Right)
{	return GetFeetLengthSq(Right ? ELMB(Right) : ELMB(Left)); }

//==============================================================================================================
//найти изначальную базисную длину ноги по базовой позе скелета. Здесь не учитывается масштаб
// и вообще это не правильно, потому что может особая поза через анимацию
//==============================================================================================================
float UMyrGirdle::GetRefLegLength()
{
	ELimb eL = ELMB(Right);
	auto ReSke = me()->GetSkeletalMeshAsset()->RefSkeleton;
	auto Tip =	me()->GetBoneTransformInRef(ReSke, ReSke.FindBoneIndex(me()->FleshGene(eL)->GrabSocket))			.GetLocation();
	auto Root = me()->GetBoneTransformInRef(ReSke, ReSke.FindBoneIndex(me()->MachineGene(eL)->AxisReferenceBone))	.GetLocation();
	//auto Tip = me()->GetBoneTransformRel(ReSke, me()->FleshGene(eL)->GrabSocket).GetLocation();
	//auto Root = me()->GetBoneTransformRel(ReSke, me()->MachineGene(eL)->AxisReferenceBone).GetLocation();
	return FVector::Distance(Tip, Root);
}

//==============================================================================================================
//включить или отключить проворот спинной части относительно ног (мах ногами вперед-назад)
//скорее всего уже не нужно, мах ногами полностью виртуален
//==============================================================================================================
void UMyrGirdle::SetSpineLock(bool Set)
{
	//если изначально был залочен, то и будет залочен
	auto& CurPa = GetGirdleSpineConstraint()->ProfileInstance.TwistLimit.TwistMotion;
	auto OldPa = CurPa;
	if (Set)	CurPa = EAngularConstraintMotion::ACM_Locked;
	else		CurPa = GetGirdleArchSpineConstraint()->ProfileInstance.TwistLimit.TwistMotion;
	if(CurPa != OldPa) GetGirdleSpineConstraint()->UpdateAngularLimit();
	//UE_LOG(LogMyrPhyCreature, Log, TEXT("%s: SetGirdleSpineLock %d"), *GetOwner()->GetName(), Set);
}

//==============================================================================================================
//установить силу, с которой махи вперед-назад двумя лапами будут сдерживаться шарниром между спиной и поясом
//==============================================================================================================
void UMyrGirdle::SetJunctionStiffness(float Stiffness)
{
	if (Stiffness == 0.0f)
		GetGirdleSpineConstraint()->SetOrientationDriveTwistAndSwing(false, false);
	else
	{
		GetGirdleSpineConstraint()->SetOrientationDriveTwistAndSwing(true, false);
		auto& ArchDv = GetGirdleArchSpineConstraint()->ProfileInstance.AngularDrive.TwistDrive;
		GetGirdleSpineConstraint()->SetAngularDriveParams(ArchDv.Stiffness * Stiffness, ArchDv.Damping * Stiffness, ArchDv.MaxForce);
	}
}

//==============================================================================================================
//обнаружить призмеление и применить боль если нужно - на любом членике, хоть ноге, хоть туловище
//==============================================================================================================
FFloor UMyrGirdle::LandingHurt(FLimb& Limb, FHitResult Hit)
{
	//временная опора
	FFloor NewFloor(Hit);

	//критерий приземления - в членике опоры не было и вот извне // возможно, добавить еще как-то внешний критерий
	if (NewFloor.IsValid() && !Limb.Floor.IsValid())
	{	
		//сила удара, урон (скорость берется по оси нормали, чтобы избежать касательных быстрот, но общий модуль тоже учитывается)
		float FallSeverity = me()->ShockResult(Limb, (VelocityAtFloor() | (-NewFloor.Normal)) + Speed * 0.1, NewFloor);

		//порог в принципе мало-мальски ощутимого удара
		if (FallSeverity > 0.1)
		{
			//пождать ноги
			Crouch = FMath::Min(1.0f, Crouch + FallSeverity*3);

			//порог увечья по важности членика для стояния, дабы не заводить отдельной переменной (0ю8 для брюха, 0.1 для ног)
			if (FallSeverity > 0.6f - me()->MachineGene(Limb.WhatAmI)->TouchImportance)
				MyrOwner()->Hurt(Limb, FallSeverity * 0.1, Hit.ImpactPoint);
		}
	}
	return NewFloor;
}


//==============================================================================================================
//влить динамическую модель (те параметры, которые не надо пересматривать каждый кадр)
//==============================================================================================================
bool UMyrGirdle::AdoptDynModel(FGirdleDynModels& Models)
{
	//влить почленно
	me()->AdoptDynModelForLimb(LMB(Center),	CensorDynModel(DynModel()->Center),	DynModel()->CommonDampingIfNeeded);
	me()->AdoptDynModelForLimb(LMB(Spine),	CensorDynModel(DynModel()->Spine),	DynModel()->CommonDampingIfNeeded);
	me()->AdoptDynModelForLimb(LMB(Tail),					DynModel()->Tail,	DynModel()->CommonDampingIfNeeded);

	//жесткость плеч
	SetJunctionStiffness(DynModel()->JunctionStiffness);

	//внедрить непосредственно флаги ограничений в зависимости от приземленности
	Flags = LMB(Center).Floor.IsValid() ? DynModel()->OnGnd : DynModel()->InAir;
	return DynModel()->Use;
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
		LINEWT(ELimbDebug::LineTrace, Start, FVector(0), 1, 0.2);
	}
	else LINEWT(ELimbDebug::LineTrace, Start, (FVector)Dst, 0.5, 0.5);
	return R;
}

//==============================================================================================================
//прощупать почву под положение ног
//==============================================================================================================
bool UMyrGirdle::SenseForAFloor(float DeltaTime)
{
	//всякие локальные переменные, нужные по всей длине функции
	FLimb& StartLimb = LMB(Center);			// стартовая точка трассировки
	float &Gain = MyrOwner()->MoveGain;		// существо хочет и может двигаться (реакция на WASD, кроме случаев усталости и смерти)
	FHitResult Hit(ForceInit);				// сюда скидывать результаты пробы общей опоры для центра пояса
	bool Steppable = false;					// поверхность пологая ИЛИ по ней можно карабкаться, определяется в п.2
	bool BumpedIn = false;					// признак серьезного вписывания в стену, определяется в п.3

	//преднастройки шага для этого пояса и для этого состояния поведения
	FStepGene& StG = MyrOwner()->GetBehaveCurrentData()->StepGene(IsThorax);

	//противоположный пояс и направление на него - нужен при расчётё ведомого
	auto Opposite = MyrOwner()->GetAntiGirdle(this);
	FVector3f ToOpp = MyrOwner()->SpineVector * (IsThorax ? -1 : 1);

	//сокращенные имена всетельных направлений
	FVector3f& MDir = MyrOwner()->MoveDirection;
	FVector3f& ADir = MyrOwner()->AttackDirection;

	//вектора ориентации якоря, должны быть ортонормированы, начальное значение - из локальных осей центрального членика
	FVector3f DueUp		= LAXIS(StartLimb, Up);
	FVector3f DueFront	= LAXIS(StartLimb, Forth);
	
	//позиции якоря 
	FVector RealLoc = BDYLOC(Center);			// реальный центр физического пояса лошарика, ерзает из-за физики, но помогает расслабить позвоночник реальой позой
	FVector StableLoc = GetComponentLocation(); // центр якоря, кинематический, положение, в котором тело не съезжает
	FVector DueLoc;								// сюда будет сливаться финальная позиция якоря

	//возможно, чисто для отладки - режим быстрого забирания по стене без карабканья
	Ascending = false;
	Descending = false;

	//получение инфы из кривых по длине шага и по высоте подъема
	float StrideNow = IdealStepSizeX(Speed), StrideToBe = IdealStepSizeX(SPEED_TOBE);
	float ElevMod = 0;
	if (StG.Shapes)
	{
		if(auto Kurwa = StG.Shapes->GetRowMap().Find(TEXT("ElevationByPhase")))
			ElevMod = FMath::Max((*Kurwa)->Eval(GetStep(LMB(Left)).Phase), (*Kurwa)->Eval(GetStep(LMB(Right)).Phase));
		if (auto Kurwa = StG.Shapes->GetRowMap().Find(TEXT("ElevationBySpeed")))
			ElevMod *= (*Kurwa)->Eval(Speed * 0.01);
		if(auto Kurwa = StG.Shapes->GetRowMap().Find(TEXT("StrideBySpeed")))
		{	float BaseLength = TargetFeetLength * StG.LengthMod;
			StrideNow =  BaseLength * (*Kurwa)->Eval(0.01 * Speed);
			StrideToBe = BaseLength * (*Kurwa)->Eval(0.01 * MyrOwner()->GetCurBaseVel());
		}
	} 

	// 1 =========== ПЕРЕРАСЧЁТ ТОГО, ЧТО НЕ ЗАВИСИТ ОТ ТРАССИРОВКИ ==================
	//################################################################################

	//текущий уровень прижатия к земле
	Crouch = FMath::Lerp(Crouch, DynModel()->Crouch, LIM1(DeltaTime * 5));

	//устойчивость = обнулить с прошлого кадра + внести слабый вклад других частей, чтобы при лежании на боку было >0
	StandHardness = 
		FMath::Max(0, (int)LMB(Spine).Stepped - STEPPED_MAX + 20) + 
		FMath::Max(0, (int)LMB(Tail).Stepped - STEPPED_MAX + 15);

	//абсолютная скорость в поясе, для расчёта скорости относительной
	FVector3f RelativeSpeedAtPoint = me()->BodySpeed(LMB(Center));

	// 2 ============= ТРАССИРОВКА, ПРИНЯТИЕ ОПОРЫ ===================================
	//################################################################################

	//направление трассировки - по умолчанию по нормали (если в воздухе нормаль равно локальной оси таза)
	FVector3f TraceTo = Flags.Vertical ? FVector3f::DownVector : -StartLimb.Floor.Normal;

	//необходимое поднятие над землей, норма с учётом подогнутия, однако если карабкаемся, считаем по максимуму, чтобы не упасть от случайного недоставания
	float Elevation = GetTargetLegLength() * (1 + ElevMod*StG.RisingMod);

	//прощупать опору - в результате опора найдена
	Trace(StableLoc, TraceTo * Elevation * MyrOwner()->GetBehaveCurrentData()->TraceFeetDepthMult, Hit);

	//сборка пола из трассировки, здесь Floor.Normal уже сразу присваивается
	FFloor NewFloor = LandingHurt(StartLimb, Hit);

	//альтернативная точка опоры, предложенная одной из ног - для изменения траектории в пользу более удобной опоры
	FVector AltHitPoint = Hit.ImpactPoint;
	bool AltFloorFound = false;

	//сразу просканировать ноги 
	ProcessSteps(LMB(Left), DeltaTime, StrideNow, StrideToBe, Elevation, 2 * IsThorax);
	ProcessSteps(LMB(Right), DeltaTime, StrideNow, StrideToBe, Elevation, 2 * IsThorax + 1);

	//коэффициент бодания в нормаль к возможной опоре или наоборот скатывания
	//в случае одобренной опоры сюда запишется не ноль, нужно для обнружения взбирания, скатывания в разных поведениях
	float BumpAlongNormal = 0.0;

	//прощупать опору - в результате опора найдена
	if (NewFloor)
	{
		//оценка упертости (ввобзе так неправильно, поскольку ХУ могут быть сильно неединичными)
		BumpAlongNormal = XY(MDir) | XY(-NewFloor.Normal);

		// 2.1 ============ ОЦЕНКА ПРИГОДНОСТИ ОПОРЫ ДЛЯ ХОДЬБЫ ==========================
		//################################################################################

		//определить нужду и возможность карабкаться
		Climbing = MyrOwner()->bClimb				// главное условие, извне идет запрос на карабканье
			&& NewFloor.IsClimbable()				// поверхность позволяет зацепиться
			&& (Lead() || Opposite->Climbing);		// ведомый пояс может цепляться только если ведущий уже зацеплен = сначала передними, потом задними

		//режим карабканья, крутизна не имеет значения, хоть за потолок когтями
		if(Climbing) Steppable = true; else

		//не зацепиться, поверхность крутая, однако помечена на уровне компонента "можно наступать"
		if (NewFloor.TooSteep())					
		{	if (NewFloor.CharCanStep())
			{	if(MyrOwner()->MoveGain > 0.5)
				{	Ascending = BumpAlongNormal > 0.5;
					Descending = BumpAlongNormal < -0.5; }
				Steppable = Climbing || Ascending || Descending;
			} else Steppable = Climbing;		// не ступабельная опора - на устойчивость опоры влияет только лазание				
		}else Steppable = true;					// полоский склон, ничто не мешает идти			

		//исключить аномальные случаи, когда центр якоря уже погрузился в стену и трассирует оттуда - нормаль будет левая, надо его сначала вытащить
		if(Hit.bStartPenetrating) Steppable = false;

		//пригодная для движения поверхность - окончателно сохранить в тело
		if (Steppable)
		{	Flags = DynModel()->OnGnd;
			StartLimb.AcceptFloor(NewFloor, 1.0f);
		}
		//новая опора непригодна для движения // сорваться/соскользнуть, если уже лезем по вертикали
		else Climbing = false;

		//если пол движется, внести его скорость в собственное представление о скорости
		if(NewFloor.IsMovable()) RelativeSpeedAtPoint -= NewFloor.Speed(Hit.ImpactPoint);
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
		Ascending = false;
		Flags = DynModel()->InAir;

		//фиксация без опоры = фикасация на траектории, возможна только если есть цель
		if(!MyrOwner()->JumpTarget) Flags.FixOn = false;//◘◘

		//куда устрамлять ненужную нормаль: на нормаль соседнего пояса, на нормаль своего спинного членика или на собственный верх
		FVector3f DstNormal = LAXIS(StartLimb, Up);
		if (Opposite->Climbing) DstNormal = Opposite->GetCenter().Floor.Normal; else
		if (GetSpine().Floor && !GetSpine().Floor.TooSteep()) DstNormal = GetSpine().Floor.Normal;
		LRP(StartLimb.Floor.Normal, LIM1(DeltaTime*5), DstNormal);

		//норм. норм. а то накапливаются искажения
		StartLimb.Floor.Normal.Normalize();
	}

	//запретить ногам махать вдоль спины
	SetSpineLock(Flags.LockLegsSwing);

	// 3 ============= РАСЧЁТ ВЕКТОРА ДВИЖЕНИЯ ======================================
	//################################################################################

	//режим кинематического перемещения по параболе - в вектор движения уже заолжен нормированный вектор скорости, касательная к параболе
	if (MyrOwner()->CurrentState == EBehaveState::project)
		GuidedMoveDir = MDir;
	 else

	//тело испытывает движение (только в движении вектор курса имеет большой смысл)
	if(Gain > 0.01)
	{
		//если имеется надёжная опора, а не просто когтеточка
		if(Steppable)
		{
			//чуточку повернуться в сторону ноги, встретившей более удобную опору, если таковой нет, тут будет просто ноль
			if(AltFloorFound) GuidedMoveDir += 0.2*FVector3f(AltHitPoint - Hit.ImpactPoint);

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
			//направление по вертикали никакое, если по земле (его определить нормаль) и равное вектору взгляда, если мы спускаемся или в воздухе)
			GuidedMoveDir += FVector3f(MDir.X, MDir.Y, (Descending || IsInAir()) ? ADir.Z : 0);

			//переход к вознесению по стенке при упирании в нее
			if(StartLimb.Floor.Normal.Z < 0.5)
			{	float GoUpOrDown = FMath::Sign(ADir.Z + 0.7f);				// при взгляде сильно вниз, курс выворачивается вниз, а не вверх
				float SkipSmallBumps = Steppable ? 0.4 : 0.7;				// вычитаемое, что занулит мелкие и случайные тычки в стену
				float RiseAmount = LIM0(BumpAlongNormal - SkipSmallBumps);		// коэффициент тяги вверх

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
		GuidedMoveDir += 0.5 * LAXIS(StartLimb, Forth);
	}

	//окончательное формирования вектора курса, универсальное
	BumpedIn = me()->GuideAlongObstacle(GuidedMoveDir,	// уложить или отразить курс от стены, в которую уперлись
		Hit.bStartPenetrating);							// особенно если якорь проник туда полностью
	//if (Steppable)
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
				Flags.FixOn = false;//◘◘
		//else if(Opposite->GetLimb(EGirdleRay::Center).Floor.TooSteep())
		//	FixedOnFloor = false;//◘◘
	}

	// 5 ============= РАСЧЁТ ОСЕЙ ОРИЕНТАЦИИ ЯКОРЯ ==================================
	//################################################################################

	//если требуется поддержка вертикали, первичен вектор вверх, вперед расчитывается из него
	if(Flags.Vertical)
	{	DueUp = UP;
		DueFront = PRJN(GuidedMoveDir, DueUp);
		DueFront.Normalize();
	}
	//прочие позиции без вертикали, но с креплением к опоре
	else if(Flags.FixOn)
	{	DueFront = GuidedMoveDir;					// присваиваем перед=курс, верх=нормаль, 
		DueUp = StartLimb.Floor.Normal;				// они скорее всего уже ортогональны
		if (FMath::Square(DueUp|DueFront) > 0.01)	// но если они всё же не ортогональны - ортогонализировать
		{	FVector3f TempLat = DueUp ^ DueFront;	// вектор в сторону, честно ортогональный и нормали, и курсу
			TempLat.Normalize();					// если не ортогональны, этот вектор не единичен и его надо нормализовать
			if (1)									// если первична нормаль, то правим курс
			{	DueFront = DueUp ^ TempLat;
				if ((DueFront | GuidedMoveDir) < 0)
					DueFront = -DueFront;
			}
			else									// прочих случаях сохраняем точный курс и правим вектор вверх
			{	DueUp = DueFront ^ TempLat;
				if ((DueFront | StartLimb.Floor.Normal) < 0)
					DueUp = -DueUp;
			}
		}
	}
	else {} //оси уже присвоены при объявлении переменных

	// 6 ============= РАСЧЁТ ПРИРАЩЕНИЯ КИНЕМАТИЧЕСКОГО ДВИЖЕНИЯ=====================
	//################################################################################

	//сюда будет записываться отклонение физического позвонка от кинематического якоря - признак уткнутости/застрялости
	float PosErr = (StableLoc - RealLoc).Size() / (TargetFeetLength * 0.2);

	//сложное позиционирование якоря только для режимов фиксации тела на якоре
	if (Flags.FixOn)
	{
		//в воздухе в режиме кинематичекого движения по праболе 
		if (MyrOwner()->CurrentState == EBehaveState::project)

			//сразу считается финальная позиция - аналитически из сохраненных стартовых условий прыжка
			DueLoc = MyrOwner()->GetJumpTrajAtNow(CachedPreJumpPos());

		else
		{
			//в анимации может быть кривая, которая делает общее движение рывками
			auto AnimDrivenVelFactor = MyrOwner()->GetMesh()->GetAnimInstance()->GetCurveValue(TEXT("Speed"));

			//шаг движения: для управляемой тяги просто курс, иначе для самодействий, где курс движения - настройка из модели
			auto MoveShift = MyrOwner()->GetDesiredVelocity() * DeltaTime *
				(me()->DynModel->MoveWithNoExtGain ? me()->CalcOuterAxisForLimbDynModel(StartLimb) : GuidedMoveDir);

			//учёт относительного движения пола, то есть если пол движется, то и тело двигать с этой скоростью вне зависимости от тяги
			if (StartLimb.Floor)
			{
				if (StartLimb.Floor.IsMovable())
					MoveShift -= StartLimb.Floor.Speed(Hit.ImpactPoint);
			}

			//для предсказуемой поверхности восстановить идеальное положение из результатов трассировки
			if (Steppable)
				DueLoc = StableLoc + (FVector)(TraceTo * (Hit.Distance - Elevation));

			//для непредсказуемой поверхности или для полета по траектории - привязать тело к его текущей точке + сдвиг по траектории
			else DueLoc = StableLoc;

			//применить вектор движения на отрезок, соответствующий скорости
			DueLoc += FVector(MoveShift);

			//коррекция кинематической позиции якоря по отклонению от физического членика (=натяжению зацепа)
			if (!Lead() || BumpedIn)

				//если слишком большое отклонение, вернуть якорь на место физического членика, чтоб его не унесло сквозь стены и т.п.
				DueLoc = FMath::Lerp(DueLoc, RealLoc, Lead() ? FMath::Min(PosErr, 0.7f) : FMath::Clamp(PosErr, 0.3f, 0.7f));
		}
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
	CI->SetLinearPositionDrive(Flags.FixOn, Flags.FixOn, Flags.FixOn);

	//силы, ограничивающие углы по рысканью влево-вправо, и по кренам вперед/назад и по сторонам
	CI->SetOrientationDriveTwistAndSwing(Flags.TightenYaw, Flags.TightenLean);

	//жесткое ограничение по углам
	me()->SetAngles(CI, Flags.LockYaw ? 0 : 180, Flags.LockRoll ? 0 : 180, Flags.LockPitch ? 0 : 180);

	// 9 ============= ВСЯКОЕ ==============================
	//################################################################################

	//обновление вектора и скаляра скорости
	FVector3f VelocityAgainstFloor = FMath::Lerp(VelocityAtFloor(), RelativeSpeedAtPoint, 0.5);
	VelocityAgainstFloor.ToDirectionAndLength(VelDir, Speed);
	if (Speed < 1.0f) VelDir = FMath::Lerp(VelDir, GuidedMoveDir, 1 - Speed);

	//если мы только включили режим карабканья и что-то не получилось, запустить вспомогательные действиия
	if (Lead() && MyrOwner()->bClimb && !Climbing) MyrOwner()->ClimbTryActions();

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
	LINE(ELimbDebug::PhyCenterDisrotation, GetComponentLocation() + (FVector)UP*10, GetComponentTransform().GetUnitAxis(EAxis::Z) ^ BDYZ(Center)*50);

	//индикация направления
	LINELIMBWT(ELimbDebug::GirdleGuideDir, StartLimb, (FVector)GuidedMoveDir * 15, 1, 0.04f);
	LINELIMBWTC(ELimbDebug::GirdleCrouch, StartLimb, (FVector)StartLimb.Floor.Normal * 20, 1, 0.04f, Crouch);

	//сверху от туловище рисуем тестовые индикаторы
	auto Ct = BDY(Center)->GetCOMPosition() + StartLimb.Floor.Normal * 15;
	if (LDBG(ELimbDebug::GirdleConstrainMode))
	{
		auto Color = FColor(255, 0, 255, 10);
		float ang = CI->ProfileInstance.ConeLimit.Swing1LimitDegrees;
		float ang2 = CI->ProfileInstance.TwistLimit.TwistLimitDegrees;
		if (Steppable)
		{
			if (Hit.bStartPenetrating) DrawDebugString(GetWorld(), (FVector)Ct, TEXT("P"), nullptr, FColor(255 * Gain, 0, 255 * Hit.bStartPenetrating), 0.02, false, Lead() ? 1.5f : 1.0f);
			if (Ascending) DrawDebugString(GetWorld(), (FVector)Ct, TEXT("A"), nullptr, FColor(255 * Gain, 0, 255 * Hit.bStartPenetrating), 0.02, false, Lead() ? 1.5f : 1.0f);
			if (Descending) DrawDebugString(GetWorld(), (FVector)Ct, TEXT("D"), nullptr, FColor(255 * Gain, 0, 255 * Hit.bStartPenetrating), 0.02, false, Lead() ? 1.5f : 1.0f);
			else DrawDebugString(GetWorld(), (FVector)Ct, TEXT("S"), nullptr, FColor(255 * Gain, 0, 255 * Hit.bStartPenetrating), 0.02, false, Lead() ? 1.5f : 1.0f);
		}
		if (Climbing)			DrawDebugString(GetWorld(), (FVector)Ct, TEXT("_C"), nullptr, FColor(0, 255, 255), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (Flags.Vertical)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("__V"), nullptr, FColor(ang, 255, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (Flags.TightenYaw)	DrawDebugString(GetWorld(), (FVector)Ct, TEXT("____Ty"), nullptr, FColor(255, 0, ang2), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (Flags.TightenLean)	DrawDebugString(GetWorld(), (FVector)Ct, TEXT("______Tl"), nullptr, FColor(150, 200, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (Flags.FixOn)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("________F"), nullptr, FColor(255, 255, 255), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (Flags.LockPitch)	DrawDebugString(GetWorld(), (FVector)Ct, TEXT("__________p"), nullptr, FColor(255, 255, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (Flags.LockYaw)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("___________y"), nullptr, FColor(255, 255, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (Flags.LockRoll)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("____________r"), nullptr, FColor(255, 255, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (FairSteps)			DrawDebugString(GetWorld(), (FVector)Ct, TEXT("____________r"), nullptr, FColor(250, 25, 255), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (BumpedIn)			DrawDebugString(GetWorld(), (FVector)Ct, TEXT("_____________B ")+TXTENUM(ELimb, me()->Bumper1().WhatAmI), nullptr, FColor(0, 255 * me()->Bumper1().Colinea, 255), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (Flags.FixOn && MyrOwner()->CurrentState == EBehaveState::project)
								DrawDebugString(GetWorld(), (FVector)Ct, TEXT("_______________J"), nullptr, FColor(255, 255, 255), 0.02, false, Lead() ? 1.5f : 1.0f);
	}
	if (LDBG(ELimbDebug::FeetShoulders))
		DrawDebugString(GetWorld(), (FVector)(Ct+StartLimb.Floor.Normal*15), FString::SanitizeFloat(RelFootRayLeft.Length()), nullptr, FColor(255, 255, 0), 0.02, false, Lead() ? 1.5f : 1.0f);

	return false;
}

//==============================================================================================================
//механизм шагов
//==============================================================================================================

//трассировка для определение позиций шага
#define TRACEBEGIN(Foot, Start, DirThruStep) auto Hit = TraceStep(Foot, Start, DirThruStep); if (!Hit.bBlockingHit) return false; auto& StM = GetStep(Foot);
#define TRACEEND(msg, result) StM.NewFootPos = Hit.ImpactPoint; StM.Normal.EncodeDir(Hit.Normal); StM.StepLength = StM.BackToOldPos.Size(); UE_LOG(LogMyrPhyCreature,Log,TEXT("%s TraceStep %s: %s %g"), *MyrOwner()->GetName(), *TXTENUM(ELimb, Foot.WhatAmI), TEXT(msg), StM.StepLength); return result;

//определение позиции шага при непосредственном приземлении, уточннение, если что-то за время шага изменилось
bool UMyrGirdle::TraceStepToLandAt(FLimb& Foot, FVector3f DirThruStep)
{	TRACEBEGIN(Foot, GetFootRootLoc(Foot.WhatAmI), DirThruStep);
	if (StM.Phase >= 1)													// фаза в отрыве привязана к проекции положение стопы, так что точно достигли места шага
	{	Foot.Floor = LandingHurt(Foot, Hit);							// отыграть удар лапой по тверди, возможно, боль + эффекты
		MyrOwner()->MakeStep(Foot.WhatAmI, false);						// переслать событие шага наверх, для спецэффектов, фолс - это опустить, а не поднять
		StM.Phase = -0.001;	}											// сбросить фазу на начало фазы наземной
	else StM.BackToOldPos += VF(StM.NewFootPos - Hit.ImpactPoint);		// желание приземлиться раньше, но до фазы нельзя, сохранять абс. прошлый след
	TRACEEND("Land.", true);
}

//определение следующей позиции шага при отрыве ноги от старой позиции
bool UMyrGirdle::TraceStepToSoarTo(FLimb& Foot, float StepLenFromCenter)
{	TRACEBEGIN(Foot, GetFootRootLoc(Foot.WhatAmI) + GuidedMoveDir * StepLenFromCenter, IdealAbsLegLine(Foot, TargetFeetLength * 1.5));
	StM.BackToOldPos = VF(StM.NewFootPos - Hit.ImpactPoint);			// уточнить место шага, посчитав прошлое ошибочным, нужно ли менять фазу, неясно
	StM.Phase = 0.001;													// если хотим поднять ногу и просчитать следующее место шага
	MyrOwner()->MakeStep(Foot.WhatAmI, true);							// переслать событие шага наверх, для спецэффектов, фолс - это опустить, а не поднять
	TRACEEND("Soar for a new step at ",true);
}

//переопределение новой позиции шага при коррекции, если прежняя стала недоступна, плавный переход на новую траекторию
bool UMyrGirdle::TraceStepToTurnTo(FLimb& Foot, FVector3f DirThruStep)
{	TRACEBEGIN(Foot, GetFootRootLoc(Foot.WhatAmI), DirThruStep);
	StM.BackToOldPos = VF(GetFootTipLoc(Foot.WhatAmI) - Hit.ImpactPoint);
	StM.Phase = 0.001;													
	TRACEEND("Change Step to new pos at ", true);
}

//переопределение новой позиции шага при коррекции для приставки ноги при торможении
bool UMyrGirdle::TraceStepToStopAt(FLimb& Foot, float StepLenFromCenter)
{	TRACEBEGIN(Foot, GetFootRootLoc(Foot.WhatAmI) + GuidedMoveDir * StepLenFromCenter, IdealAbsLegLine(Foot, TargetFeetLength * 1.5));
	StM.BackToOldPos = VF(GetFootTipLoc(Foot.WhatAmI) - Hit.ImpactPoint);
	StM.Phase = 0.001;
	TRACEEND("Change Step to Stop at ", true);
}

//==============================================================================================================
//шаги + безшаговое позиционирование ноги
//==============================================================================================================
void UMyrGirdle::ProcessSteps(FLimb& Foot, float DeltaTime, float StepHalfNow, float StepHalfToBe, float Elevation, int ProperTurn)
{
	FStep& StM = GetStep(Foot);								// сборка механизма шагов для данной ноги
	const FVector Start = GetFootRootLoc(Foot.WhatAmI);		// точка начала трассировки, уровень плеч или таза
	float& Gain = MyrOwner()->MoveGain;						// тяга, для скоращения
	FHitResult Hit(ForceInit);								// сюда скидвать результат трассировки земли под данной ногой
	FLimb& OpFoot = me()->GetMirrorLimb(Foot);				// противоположная нога
	FStep& OpStM = GetStep(OpFoot);							// механизм шагов противоположной ноги

	//1. если шаги в принципе включены
	if (StM.StepsOn())
	{
		//преднастройки шага для этого пояса и для этого состояния поведения
		FStepGene& StG = MyrOwner()->GetBehaveCurrentData()->StepGene(IsThorax);

		float VToBe = Gain>=0.1 ? MyrOwner()->GetCurBaseVel() : 0;		// целевая скорость без учета разгона
		float VNow = abs(Speed*VelDir|GuidedMoveDir);					// текущая скорость, модуль положительный
		float MinStr = TargetFeetLength * 0.1f;							// пороговый растяг ноги в покое, при котором становится неудобно стоять
		float Stretch = LegStretch(Foot);								// реальная растяга ноги с учетом направлении движения от - до + амплитуды
		float StepHalfMax = FMath::Max3(StepHalfNow, StepHalfToBe, MinStr);

		//1.1 внезапно изменилось состояние тела и шаги на этих ногах больше не нужны
		if (!DynModel()->FairSteps)	StM.DisableSteps();

		//1.2 режим на земле, стопа привязана к месту шага 
		else if (StM.OnGround())
		{
			//фаза в состоянии покоя нужна для выборок подъема спины по графику, а так без толку
			StM.Phase = FMath::Min(-0.001, 0.5*(Stretch - StepHalfToBe) / StepHalfMax);
			PackLegLine(Foot, StM.NewFootPos - Start);

			//при старте с покоя, опираясь и обгоняя другую ногу, что стоит ближе к цели
			if ((StG.Leap || OpStM.OnGround()) && VToBe > VNow * 2)
			{
				//критерии срабатывания шага при старте с места до того как нога вытянется
				bool cFront = (Stretch > LegStretch(OpFoot));
				bool cLev = (Foot.IsLeft());
				if((StG.Farthest && !cFront) || (StG.Closest && cFront) || (StG.Right && !cLev) || (StG.Left && cLev))
					TraceStepToSoarTo(Foot, 2 * StepHalfToBe);//◘◘
			}

			//эта нога вытянулась назад до предела на данной скорости (при покое - до порога неудобства) - оторвать и наметить новый шаг 
			else if (-Stretch > FMath::Max(StepHalfNow, MinStr))
			{	float Stride = 3 * FMath::Min(StepHalfNow, StepHalfToBe);						// длина шага = 4 амплитуды от кончика ноги = 3 амплитуды от центра
				if (StG.Amble || StG.Leap)														// во скоке и шаге - править взаимное положение ног через подрезку длины шага
				{	float Corr = StG.Amble														// корректированная длина будущего шага
						? abs(VF(OpStM.NewFootPos - StM.NewFootPos) | VelDir) - Stretch			// хотьба, выравнивание расстояний между следами ног
						: abs(VF(OpStM.NewFootPos - Start) | VelDir);							// скакание, постановка ноги рядом с соседней
					Stride = FMath::Clamp(Corr, Stride * 0.8, Stride*1.05);						// ограничить, чтобы постепенными шажками корректировалось
				}
				TraceStepToSoarTo(Foot, Stride);//◘◘
			}

			//эта нога вытянулась вперед
			else if (Stretch > FMath::Max(StepHalfToBe, MinStr) && OpStM.OnGround())
				TraceStepToSoarTo(Foot, 0);//◘◘
	
			//постоянно показывать линию ноги
			LINEWTC(ELimbDebug::FeetShoulders, Start, (FVector)UnpackLegLine(Foot), 1, 0, 0.5);
			LINEWT(ELimbDebug::LimbNormals, StM.NewFootPos, (FVector)Foot.Floor.Normal * 10, 1, 0);
			LINEWTC(ELimbDebug::FairStepPath, Start, -(FVector)Foot.Floor.Normal * 10 * StM.Phase, 0.8, 10, 0.2 - StM.Phase);
		}
		//1.3 переход через единицу - немедленная постановка шага или спотыкание
		else if (StM.Phase >= 1.0f)
		{
			TraceStepToLandAt(Foot, VF(StM.NewFootPos - Start) * 2);//◘◘
			PackLegLine(Foot, VF(StM.NewFootPos - Start), 0);
		}
		//1.4 нога на весу в воздухе, фаза (0,1) полностью определяется крайними положениями
		else
		{
			//сколько еще осталось до конца шага 
			auto DisTo = VF(StM.NewFootPos - GetFootTipLoc(Foot)) | VelDir;

			//1.4.1 если нога вытянулась слишком далеко, и место шага дальше, чем можно раскоря\читься
			if (Stretch > StepHalfMax && DisTo > StepHalfMax)
	
				//вставить внеочередной шаг прямо вниз
				TraceStepToStopAt(Foot, StepHalfMax*0.5);//◘◘

			//1.4.2 предсказуемое перемещение ноги вдоль графика фазы
			else
			{
				// высота подъёма ноги, множитель длины ноги, из ассета * из кривой от скорости
				auto StepHeight = Elevation * StG.HeightMod;

				//путь, проходимый ступней за данный кадр = время*скорость, скорость = 2 скорости хода, однако если обе ноги в воздухе, надо симулировать падение
				float Drift = FMath::Max(VNow*2, 20) * DeltaTime;

				//режим падения - при замедлении ускоряемся
				if (!OpStM.OnGround() && Gain < 0.5f)
					 Drift = FMath::Lerp(Drift, DisTo*0.1, 0.5f - Gain);

				//перевод приращения реальной координаты в приращения нормированной фазы
				float DeltaPhase = Drift / StM.StepLength;

				//расшифровка кривых образа шага для текущей фазы
				auto HeightCurve = 0.0f, BendCurve = 0.0f, PushCurve = 0.0f, ParaPhaseCurve = 1.0f;
				if (StG.Shapes)
				{	if (auto Kurwa = StG.Shapes->GetRowMap().Find(TEXT("StepHeightByPhase")))	HeightCurve =	(*Kurwa)->Eval(StM.Phase);
					if (auto Kurwa = StG.Shapes->GetRowMap().Find(TEXT("StepHeightBySpeed")))	HeightCurve *=	(*Kurwa)->Eval(Speed*0.01);
					if (auto Kurwa = StG.Shapes->GetRowMap().Find(TEXT("FootAngleByPhase")))	BendCurve =		(*Kurwa)->Eval(StM.Phase);
					if (auto Kurwa = StG.Shapes->GetRowMap().Find(TEXT("FootPushByPhase")))		PushCurve =		(*Kurwa)->Eval(StM.Phase);
					if (auto Kurwa = StG.Shapes->GetRowMap().Find(TEXT("ParaPhaseBySpeed")))	ParaPhaseCurve =(*Kurwa)->Eval(Speed*0.01);
				}

				//применение кривой скорости из пользовательского ассета, пока неясно, насколько в этом параметре онон нужно
				StM.Phase += DeltaPhase * (1 + PushCurve);

				//ориентация стопы на поверхности, нормаль на поверхности по возможности должна отражать поверхность будущего шага
				FVector3f ObliqueNormal = OpStM.OnGround() ? OpFoot.Floor.Normal : (StM.Normal.IsValid() ? StM.Normal.DecodeDir() : LAXIS(LMB(Spine), Up));
				Foot.Floor.Normal = ObliqueNormal + LAXIS(LMB(Spine),Forth) * BendCurve;
				Foot.Floor.Normal.Normalize();

				//позиция кончика ноги
				FVector FootTip = StM.FootTip() + LAXIS(LMB(Spine), Up) * HeightCurve * StepHeight;
				PackLegLine(Foot, VF(FootTip - Start), 0);
			}

			//отладка
			LINEWT(ELimbDebug::FeetShoulders, Start, VD(UnpackLegLine(Foot)), 1, 0);
			LINEWT(ELimbDebug::FairStepPredictedPos, StM.NewFootPos, VD(Foot.Floor.Normal * 5), 1, 1);
			LINEWTC(ELimbDebug::FairStepPredictedPos, StM.NewFootPos, VD(StM.BackToOldPos), 0.5, 1, (1-StM.Phase));
		}
	}
	//2. реальные шаги выключены, но модель велит включить их
	else if (DynModel()->FairSteps)
	{	
		StM.Phase = 1.0f; //искусственно задрать фазу, чтобы TraceStepToLandAt сразу инициализовала опору
		TraceStepToLandAt(Foot, UnpackLegLine(Foot) + IdealAbsLegLine(Foot, TargetFeetLength*1.5));//◘◘
	}
	//3. Реальные шаги выключены, идёт подстройка стойки по узловым членикам вниз + анимация шагов
	else
	{
		//подготовить целевой вектор линии ноги, оименовать компоненты, чтоб было понятно
		FLegPos LocLineReal = GetLegLine(Foot);
		FLegPos LocLineToBe = idealLegLine(Foot);

		float SwingAlpha = LIM1(DeltaTime * (Foot.Floor ? 8 * Speed * Speed * 0.0001f : 1.5f));		// скорость схождения по раскоряке сильно от скорости, при стоянии раскоряка не меняется
		float PushAlpha = LIM1(DeltaTime * (Foot.Floor ? (LMB(Center).Floor ? 200 : 20) : 5.0f));	// скорость схождения по осям маха вперед-назад

		//внесение маленьких отклонений в сторону идеала, теперь эта линия может использоваться для трассировки
		GetLegLine(Foot).DownUp =	 FMath::Lerp(LocLineReal.DownUp,	LocLineToBe.DownUp, PushAlpha);		// проекция на ось "вверх"
		GetLegLine(Foot).BackFront = FMath::Lerp(LocLineReal.BackFront, LocLineToBe.BackFront, PushAlpha);	// проекция на ость "вперед"
		GetLegLine(Foot).LeftRight = FMath::Lerp(LocLineReal.LeftRight, LocLineToBe.LeftRight, SwingAlpha);	// проекция на ость "вдево/вправо", вокруг оси вперед

		bool MyTime = Foot.IsLeft();
		bool CanEverTrace = (me()->GetPredictedLODLevel() < 2);			// для дальних существ трассировку нафиг
		bool AltFloorFound = false;										// принесли для туловища более удобную опору

		//критерий обновления желаемой позы и, возможно, подтверждения ее с помощью трассировки
		if((FRAME & 3) == ProperTurn)
		{
			//распаковать линию ноги сверху вниз в абсолютных координатах (ту самую, что изменили выше)
			FVector3f AbsLegLine = UnpackLegLine(Foot);
			LINEWT(ELimbDebug::FeetShoulders, Start, (FVector)AbsLegLine, 1, 0);

			// брюшной сенсор уже нашупал опору, то есть у ног в принципе есть шанс правильно лечь на ту же поверхность
			if (LMB(Center).Floor.IsValid())
			{
				//модель сблизи, вдали нах лишние трассировки
				if (CanEverTrace)
				{
					//непосредственно щупать по ранее посчитанному вектору ноги, но на бОльшую длину
					if (Trace(Start, AbsLegLine * 3, Hit))
					{
						//шаги не используются, но если вдруг начнут, чтобы начинали с этой
						StM.RegNextStep(Hit);

						//если до опоры дотянулись лучом, но он длинее ноги - сорваться
						if (Hit.Distance > TargetFeetLength * MyrOwner()->GetBehaveCurrentData()->TraceFeetDepthMult * 1.2)
							Foot.EraseFloor();
						else
						{
							// сохранить опору в ногу + упаковать скорректированную (по нижнему краю) линию ноги для показа в анимации
							Foot.Floor = LandingHurt(Foot, Hit);
							PackLegLine(Foot, FMath::Lerp(AbsLegLine, FVector3f(Hit.ImpactPoint - Start), 0.1), Hit.Distance); 
						}
					}
					//трасировка вообще не нашла опоры, помечаем ногу как без опоры
					else
					{
						//убрать инфу о поле, восстановить длину ноги, чтобы не подгибалась
						Foot.EraseFloor();
						LRP(GetLegLine(Foot).Pitch, DeltaTime * 3, Elevation);
					}
				}

				//быстрый способ позиционировать ноги
				else Foot.Floor = LMB(Center).Floor;
			}
		}
		//весь пояс в воздухе - восстановить натуральную длину ноги, чтобы не подгибалась
		else LRP(GetLegLine(Foot).Pitch, DeltaTime, Elevation);
	}

	//DrawDebugString(GetWorld(), Start + VD(LAXIS(LMB(Center), Up) * 30), FString::SanitizeFloat(MaxDeltaPhase) + TEXT(" / ") + FString::SanitizeFloat(MinDeltaPhase), 0, FColor(255, 255, 0, 255), 0.02, false, 1.0f);

	//обновить устойчивость, это сильно влияет на смену состояний поведения
	if (Foot.Floor)	StandHardness += 100;		
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

	//непосредственно физически запустить с заданной скоростью
	PhyPranceDirect(Dir);
}

//==============================================================================================================
//физически подпрыгнуть этим поясом конечностей в заранее посчитанном направлении
//==============================================================================================================
void UMyrGirdle::PhyPranceDirect(FVector3f Vel)
{
	Crouch = 0.0f;							//резко отключить пригнутие, разогнуть ноги во всех частях тела
	me()->PhyJumpLimb(LMB(Spine), Vel);		//применить импульс к членику
	me()->PhyJumpLimb(LMB(Center), Vel);	//применить импульс к членику
	LMB(Center).EraseFloor();				//явно, не дожидаясь ProcessLimb, обнулить жесткость стояния
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
