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
#define PROJ(V,N) V = FVector3f::VectorPlaneProject(V,N);

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
	CI->SecAxis2 = FVector(-1,0,0);	//вперед почему тут минус, ВАЩЕ ХЗ Я В АХУЕ, но без него при ограничении по этой оси существо переворачивается
	CI->Pos1 = FVector(0,0,0);

	//окончательно инициализировать привязь, возможно, фреймы больше не будут меняться
	CI->InitConstraint(BDY(Center), GetBodyInstance(), 1.0f, this);

	//привязь, удерживающая спину в заданном положении
	me()->SetFreedom(CI, TargetFeetLength, TargetFeetLength, TargetFeetLength, 180, 180, 180);
	CI->SetLinearPositionDrive(false, false, false);
	CI->SetLinearDriveParams(6000, 100, 10000);
	CI->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
	CI->SetAngularDriveParams(9000, 200, 10000);
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
	SetLegRay(LMB(Right), BAXIS(BDY(Center), Down) * TargetFeetLength);
	SetLegRay(LMB(Left), BAXIS(BDY(Center), Down) * TargetFeetLength);
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
FVector3f UMyrGirdle::GetLegRay(FLimb& L)
{	return 
		BAXIS(BDY(Spine), Up)		* GetRelLegRay(L).X +
		BAXIS(BDY(Spine), Forth)	* GetRelLegRay(L).Y +
		BAXIS(BDY(Spine), Right)	* GetRelLegRay(L).Z;
}

//==============================================================================================================
//загрузить новый радиус вектор ноги из точки касания с опорой
//==============================================================================================================
void UMyrGirdle::SetLegRay(FLimb& L, FVector3f AbsRay)
{
	GetRelLegRay(L) = FVector3f(
		(BAXIS(BDY(Spine), Up)		| AbsRay),
		(BAXIS(BDY(Spine), Forth)	| AbsRay),
		(BAXIS(BDY(Spine), Right)	| AbsRay)
	);
}


//==============================================================================================================
//позиция точки проекции ноги на твердь
//==============================================================================================================
FVector UMyrGirdle::GetFootVirtualLoc(FLimb& L)
{ return me()->GetLimbShoulderHubPosition(L.WhatAmI) + (FVector)GetLegRay(L); }

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

	//запретить ногам махать вдоль спины - сомнительная надобность, опасно
	SetSpineLock(CurrentDynModel->LegsSwingLock);
	return CurrentDynModel->Use;
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
	bool R = Hit.Component.IsValid() && Hit.PhysMaterial.IsValid();
	if (R)	LINEWT(ELimbDebug::LineTrace, Start + (FVector)Dst * Hit.Time, FVector(0), 1, 0.5);
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

	//откуда
	FLimb& StartLimb = LMB(Center);
	uint8 OldStepped = StartLimb.Stepped;
	const FVector Start = 0.5 * (me()->GetLimbShoulderHubPosition(ELMB(Left)) + me()->GetLimbShoulderHubPosition(ELMB(Right)));

	//противоположный пояс - нужен при расчётё ведомого
	auto Opposite = MyrOwner()->GetAntiGirdle(this);

	//нормальное приятие поверхности = она пологая или по ней можно карабкаться
	bool Steppable = StartLimb.ImpactNormal.Z > 0.5 || Climbing;

	//направление - по умолчанию по оси таза вниз, крутится вместе с телом
	FVector3f LookAt = LAXIS(StartLimb, Down);

	//направление на противоположный пояс
	FVector3f ToOpp = MyrOwner()->SpineVector * (IsThorax ? -1 : 1);

	//если уже есть твердь, то мы знаем нормаль и суть поверхности, можно уточнить, куда щупать
	if (StartLimb.Stepped >= STEPPED_MAX-2)
	{
		//на ветви ориентироваться в центр цилиндра, иначе будет качаться
		if (StartLimb.OnCapsule())
		{	
			//если мы на капсуле не просто так, а реально способны удержаться
			if (Steppable)
			{
				//сначала смотрим из холки в центр масс капсулы
				LookAt = (FVector3f)(StartLimb.Floor->GetCOMPosition() - Start);

				//проецируем этот косой длинный вектор на плоскость круглого среза ветки
				//получается вектор, направленный на ближайшую точку оси капсулы
				PROJ(LookAt, StartLimb.GetBranchDirection());
				LookAt.Normalize();
			}
			//для неустойчивой поверхности ориентироваться на собственный низ
			else LookAt = LAXIS(StartLimb, Down);
		}

		//если жесткая вертикаль, то вне зависимости от крена таза щупать строго вниз
		else if (Vertical) LookAt = FVector3f::DownVector;

		//если без вертикали, но на тверди, ориентроваться на нормаль
		else LookAt = -StartLimb.ImpactNormal;
	}

	//хз насколько это нужно, но направить луч немного вперед и еще больше вверх, при карабканьи
	//LookAt += GuidedMoveDir * 0.4 * FMath::Abs(GuidedMoveDir.Z);

	//текущий уровень прижатия к земле
	Crouch = FMath::Lerp(Crouch, CurrentDynModel->Crouch, DeltaTime * 5);

	//поднятие над землей, норма, от плеч до ступней (сдесдь уже запечаан crouch)
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
	if (MyrOwner()->bClimb) ReachMod = (Crouch + 1);

	//особый случай, если активная фаза прыжка, прервать фиксацию на опоре, чтоб оторваться
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

	//реальный центр координат центра, может отличаться от якоря и создавать натяг
	FVector RealLoc = BDY(Center)->GetUnrealWorldTransform().GetLocation();

	//центр якоря, нуль натяга, положение, в котором тело не съезжает
	FVector StableLoc = GetComponentLocation();

	//если это падение, а не шаг/скольжение, тут будет сила удара
	float FallSeverity = 0.0f;

	//нормальное приятие поверхности = она пологая или по ней можно карабкаться
	Steppable = Hit.ImpactNormal.Z > 0.5 || Climbing;

	//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	//нашупали правильную опору
	if (Hit.Component.IsValid() && Hit.PhysMaterial.IsValid())
	{
		//считанная нормаль
		auto NewNormal = (FVector3f)Hit.ImpactNormal;

		//для хода по капсуле (ветке) 
		if (StartLimb.OnCapsule() && Steppable)
		{
			//не допускать загиба нормали на залупах капсулы
			PROJ(NewNormal, StartLimb.GetBranchDirection());

			//коррекция точки контакта, чтобы позиция туловища, точка контакта и точка на оси были на одной прямой
			Hit.ImpactPoint = (Hit.ImpactPoint - Start).ProjectOnToNormal(Hit.ImpactNormal) + Start;
		}

		//если извне хочется лазать по цепкой поверхности
		if (MyrOwner()->bClimb)
		{
			// подходящая для лазанья поверхность
			bool Climbable = FLimb::IsClimbable(Hit.PhysMaterial->SurfaceType.GetValue());

			//условия включения режима лазанья - +подходящий угол
			if (!Climbing) Climbing = Climbable && (NewNormal | LAXIS(StartLimb, Up)) > -0.5f;

			//условия поддержания состояния лазанья
			else
			{	if (!Climbable)
					UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s: SenseForAFloor No Climbable!"), *GetOwner()->GetName());
				Climbing = Climbable;
			}
		}
		else Climbing = false;


		//ведомый пояс может цепляться только если ведущий уже зацепился
		if (!Lead() && !Opposite->Climbing)
			Climbing = false;

		//обновить нормальное приятие поверхности = она пологая или по ней можно карабкаться
		Steppable = NewNormal.Z > 0.5 || Climbing;

		//при соприкосновении с землей из полета нужно будет внеочередно проверить ноги
		if (StartLimb.Stepped < STEPPED_MAX)
		{
			//безусловно вписать новую опору, чтобы расчитать силу удара  - пусть даже эта повторится ниже
			me()->GetFloorFromHit(StartLimb, Hit);

			//сила удара, урон, скорость берется по оси нормали, чтобы избежать касательных быстрот, но общий модуль тоже учитывается
			FallSeverity = me()->ShockResult(StartLimb, (VelocityAgainstFloor | (-NewNormal)) + Speed * 0.1, Hit.Component.Get());

			//если долго летели, то сильно пригнуться, так проще, чем брать скорость или урон
			Crouch = Crouch + FallSeverity * 5;
			if (IS_MACRO(MyrOwner()->CurrentState, AIR)) Crouch = Crouch + MyrOwner()->StateTime;
			if (Crouch > 1) Crouch = 1;
			Elevation = GetTargetLegLength();

			//туловище повреждается только если урон значительный
			if (FallSeverity > 0.5f)
				MyrOwner()->Hurt(StartLimb, FallSeverity * 0.1, Hit.ImpactPoint);
		}

		//вес - насколько быстро переходить к новым значениям
		float Weight = 0.1;

		//для ходибельной поверхности
		if (Steppable)
		{
			//сохранить часть данных сразу в ногу
			me()->GetFloorFromHit(StartLimb, Hit);

			//при резком изменении нормали (наступили на желоб/яму) новая инфа о тверди недостоверна
			//тут нужно еще как-то включатьнаправление взглядом на ветку
			Weight = StartLimb.ImpactNormal | NewNormal;

			//учёт направления взгляда - если нащупали новую ветку, по которой по пути, сделать вес этого шага больше
			auto& AD = MyrOwner()->AttackDirection;
			if (StartLimb.OnCapsule())
				Weight += FMath::Max(0.0f, (StartLimb.GetBranchDirection(AD) | AD) - 0.5f);
			Weight = FMath::Clamp(Weight, 0.1f, 1.0f);

			//грубая коррекция возвышения на ногах
			//на случай если приземлился на брюхо
			if (Hit.Distance < Elevation * 0.9)
				StableLoc -= (FVector)LookAt * (Elevation - Hit.Distance) * DeltaTime;
			else if (Hit.Distance > Elevation * 1.1)
				StableLoc += (FVector)LookAt * Elevation * DeltaTime;

			//корректировка поднятия над землей если какие-то ноги слишком вытянуты
			auto& RLen = GetRelLegRay(LMB(Right)).X;
			auto& LLen = GetRelLegRay(LMB(Left)).X;
			if (FMath::Max(LLen, RLen) > Elevation)
				Elevation = FMath::Min3(Elevation, RLen, LLen);


			//нормаль, пока неясно, как плавно ее брать для середины
			if (CurrentDynModel->NormalAlign) StartLimb.ImpactNormal = NewNormal; else
				StartLimb.ImpactNormal = FMath::Lerp(StartLimb.ImpactNormal, NewNormal, 0.5f * Weight);

			//для невертикальной стойки стойка подстраивается под текущую нормаль
			if (Vertical) DueUp = FVector3f::UpVector;
			else DueUp = NewNormal;

			//идеально положение якоря спины, протянуть из точки по нормали на высоту ног
			DueLoc = Hit.ImpactPoint + (FVector)DueUp * Elevation;

			//скорректировать относительную скорость 
			if(StartLimb.Floor->OwnerComponent->Mobility == EComponentMobility::Movable)
				RelativeSpeedAtPoint -= (FVector3f)StartLimb.Floor->GetUnrealWorldVelocityAtPoint(Hit.ImpactPoint) * Weight;

			//отладка веса шага
			LINELIMBWTC(ELimbDebug::GirdleStepWeight, StartLimb, (FVector)StartLimb.ImpactNormal * 15, 1, 0.03, Weight);
		}
		//нащупали недостижимую поверхность
		else
		{
			//сорваться/соскользнуть
			FixedOnFloor = false;

			//фэйковая точка контакта для расчёта раскоряки ног SenseFootAtStep
			Hit.ImpactPoint = Start + (FVector)LAXIS(StartLimb, Down) * Elevation;
		}
	}

	//полное отсутствие опоры - пока нерадикально, просто уменьшаем
	else
	{
		StartLimb.Stepped /= 2;
	}

	//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	//если на прошлом этапе была найдена правильная опора
	if(StartLimb.Stepped == STEPPED_MAX)
	{
		//пересчитать маршрут на ближайший кадр
		if (Lead())
		{
			//основа направления - курс из контроллера
			GuidedMoveDir = MyrOwner()->MoveDirection;

			//выворот курса в вертикаль, если уткнулись в препятствие
			if (Hit.ImpactNormal.Z < 0.5)
			{
				//коэффициент, отменяющий выворот вверх в случае глядения вниз
				float NoLookDown = MyrOwner()->AttackDirection.Z + 0.7f;

				//уткнутость, соосность, берется только 2д проекция
				float BumpInAmount = FVector2f(-StartLimb.ImpactNormal) | FVector2f(MyrOwner()->MoveDirection);
				BumpInAmount -= Steppable ? 0.4 : 0.7;

				//выворот вверх только если упираемся близко к прямому углу
				if (BumpInAmount > 0)
					GuidedMoveDir.Z += (Climbing ? 2 : 1) * BumpInAmount * FMath::Sign(NoLookDown);
			}
		}
		//вектор курса для ведомого
		else
		{
			//по направлению на ведущий
			GuidedMoveDir = ToOpp;

			//костыль: если верх уже зацепился, а низ пришпилен к земле, отпустить низ и подтянуть к верху
			if (Opposite->Climbing && !Climbing)
				FixedOnFloor = false;
		}

		//если ползем по ветке, дополнительно сдерживать курс вдоль ветки
		//это для всех поясов, и ведомых тоже
		if (StartLimb.OnCapsule() && FixedOnFloor && Steppable)
		{
			//направление вдоль ветки в сторону уже выработанного курса
			float Coaxis = 0.0f; // и выдать степень соосности, чтобы поправлять только если пользователь действительно направил вдоль
			auto DirAlong = StartLimb.GetBranchDirection(GuidedMoveDir, Coaxis);

			if (Coaxis > 0.8 && StartLimb.GetBranchRadius() < MyrOwner()->SpineLength * 0.25) GuidedMoveDir *= 0;

			//добавить направление вдоль ветки
			//добавить направление вверх, чтобы переместиться в седло ветки
			GuidedMoveDir += DirAlong * Coaxis
				+ FVector3f::UpVector * (1 - DirAlong.Z * DirAlong.Z) * (1 - StartLimb.ImpactNormal.Z);
		}

		//если имеется сильная уткнутость, то обтекаем препятствие
		me()->GuideAlongObstacle(GuidedMoveDir);

		//ортонормировать текущий курс движения по актуальной нормали
		if(Steppable) PROJ(GuidedMoveDir, StartLimb.ImpactNormal);

		//после стольких мытарств надо нормализовать вектор
		GuidedMoveDir.Normalize();

		//возвращаемся к проблеме ведомости - если посчитанные курсы идут враскоряку, курс ведомого надо изменить
		if(!Lead())
		{
			//порог разрыва мал при лазаньи, иначе велик, но уменьшается на высокой скорости и при упертости
			float UnSplit = Opposite->GuidedMoveDir | GuidedMoveDir;
			if (Climbing)
			{
				//сильное отклонение - просто оторвать ведомый от поверхности и предоставить слово физдвижку
				if(UnSplit < 0.0f) FixedOnFloor = false; else

				//при небольшом отклонении заново вернуть ведомому простое направление на ведущий
				if(UnSplit < 0.5) GuidedMoveDir = ToOpp;
			}
			//для нелазательного движения только терминальный случай, когда разрыв велик, но уменьшается на высокой скорости и при упертости
			else if (UnSplit < -0.5 + Speed * 0.002 + me()->Bumper1().GetBumpCoaxis() * 0.4)
				FixedOnFloor = false;
		}

		//ориентировать якорь по посчитанному
		DueFront = GuidedMoveDir;

		//отладка
		LINEWT(ELimbDebug::GirdleConstrainOffsets, RealLoc, FVector::VectorPlaneProject(DueLoc - RealLoc, Hit.ImpactNormal), 1, 0); //DisplacedInPlane
		LINEWT(ELimbDebug::GirdleConstrainOffsets, RealLoc, (DueLoc - RealLoc).ProjectOnTo(Hit.ImpactNormal), 1, 0); //DisplacedInHeight

	}
	//не нащупали опору
	else 
	{
		if (Climbing)
			UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s: SenseForAFloor No Climbable! - No Floor"), *GetOwner()->GetName());

		//если нет опоры, то не на что цепляться
		if (!me()->DynModel->FlyFixed)
		{	FixedOnFloor = false;
			Climbing = false;
		}
		if (!MyrOwner()->BehaveCurrentData->KeepVerticalEvenInAir)
			Vertical = false;

		//ведущий пояс в воздухе
		if (Lead())
		{
			//если модель велит фиксированный кинематически полет, то сразу назначить свободный курс, иначе плавно 
			if (me()->DynModel->FlyFixed) GuidedMoveDir = MyrOwner()->MoveDirection;

			//случай, если в полете в принципе курс имеет значения, но меняется очень постепенно 
			else if (me()->DynModel->MotionGain > 0 && MyrOwner()->MoveGain > 0)
				GuidedMoveDir = FMath::Lerp(GuidedMoveDir, MyrOwner()->MoveDirection, DeltaTime);

			//прочие случаи, когда в воздухе не важен курс, принимается локальная ось туловища
			else GuidedMoveDir = LAXIS(StartLimb, Forth);

			//нормаль ортогонализируется относительно курса
			StartLimb.ImpactNormal = GuidedMoveDir ^ (LAXIS(StartLimb,Right));
			if((StartLimb.ImpactNormal | BAXIS(BDY(Center), Up)) < 0) StartLimb.ImpactNormal *= -1;
		}
		//ведомый пояс в воздухе
		else
		{
			//по направлению на ведущий
			GuidedMoveDir = ToOpp;

			//костыль который возможно поможет смягчить виляние на ветке
			//if(StartLimb.OnCapsule() && (GuidedMoveDir| Opposite->GuidedMoveDir) > 0.7) GuidedMoveDir = Opposite->GuidedMoveDir;

			// нормаль плавно  переходить вниз, хотя зачем?
			StartLimb.ImpactNormal += FVector3f(0, 0, -5 * DeltaTime);
		}
		StartLimb.ImpactNormal.Normalize();

		//даже в воздухе если имеется сильная уткнутость, то обтекаем препятствие
		if (me()->GuideAlongObstacle(GuidedMoveDir)) GuidedMoveDir.Normalize();

		//фэйковая точка контакта для расчёта раскоряки ног SenseFootAtStep
		Hit.ImpactPoint = Start + (FVector)LookAt * Elevation;

		//в свободном состоянии никакой особой правильной осанки нет, просто повторять за физикой тушки
		DueUp = BAXIS(BDY(Center), Up);
	}

	//окоончательно вычислить скорость
	VelocityAgainstFloor = FMath::Lerp(VelocityAgainstFloor, RelativeSpeedAtPoint, IsInAir() ? 0.1f : 0.5f);
	Speed = VelocityAgainstFloor.Size();


	//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	//вычислить желаемую позицию для якоря, а также приращение
	if (FixedOnFloor || me()->DynModel->FlyFixed)
	{
		//специальный режим для лазанья, чтоб не виляло из-за физики
		if (Lead() && Climbing)
		{
			DueLoc = StableLoc;
		}
		else
		{
			//плавно подводим якорь спины в нужную точку
			DueLoc = FMath::Lerp(StableLoc,								//с места собственного положения якоря с предыдущего кадра
				FMath::Lerp(DueLoc,										//виртуально-желаемое положение, протянутое обратно из трассировки
					RealLoc, 0.5f),										//реальное положение членика туловище, подверженного физике
				0.5f * MyrOwner()->MoveGain);							//плавно трогаемся с тягой местной
		}

		float DriftStep = MyrOwner()->GetDesiredVelocity() * DeltaTime;

		//для случая, когда действие изнутри двигает тело
		if(me()->DynModel->MoveWithNoExtGain)
			 DueLoc += (FVector)me()->CalcOuterAxisForLimbDynModel(StartLimb) * DriftStep;

		//для режима управляемости - просто добавляем сдвиг в сторону желаемого курса
		else DueLoc += (FVector)GuidedMoveDir * DriftStep;

		//учёт относительного движения пола, то есть если пол движется, то и тело двигать с этой скоростью вне зависимости от тяги
		if(StartLimb.Floor && StartLimb.Floor->OwnerComponent.IsValid())
			if(StartLimb.Floor->OwnerComponent->Mobility == EComponentMobility::Movable)
				DueLoc -= StartLimb.Floor->GetUnrealWorldVelocityAtPoint(Hit.ImpactPoint);
	}
	//не цепляемся за поверхность, возможно, вообще в воздухе
	else
	{
		//поворот совпадает с реальным члеником, чтобы никак на него не влиять
		DueFront = BAXIS(BDY(Center), Forth);
		DueUp = BAXIS(BDY(Center), Up);
		DueLoc = RealLoc;
	}




	//подстройка притяжения к зацепу
	auto CI = BDY(Center)->DOFConstraint;

	//кинематически переместить якорь в новое нужное место с нужным углом поворота
	SetWorldTransform(FTransform((FVector)DueFront, (FVector)(DueFront ^ DueUp), (FVector)DueUp, DueLoc));

	//включение сил, прижимающих спину к якорю, действует только если режим цепа к опоре
	CI->SetLinearPositionDrive(FixedOnFloor, FixedOnFloor, FixedOnFloor);

	//включение сил, держащих спину по заданным углам
	CI->SetOrientationDriveTwistAndSwing(NoTurnAround, Vertical || CurrentDynModel->NormalAlign);

	//жесткое ограничение по углам
	if (CurrentDynModel->NormalAlign != (CI->ProfileInstance.ConeLimit.Swing1Motion == EAngularConstraintMotion::ACM_Locked))
	{	int Sw = CurrentDynModel->NormalAlign ? 0 : 180; 
		me()->SetAngles(CI, Sw, Sw, 180);
	}

	//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	// прощупь найденной поверхности виртуальными ногами, чтоб понять, как их ставить
	SenseFootAtStep (DeltaTime, LMB(Right), Hit.ImpactPoint, FallSeverity, 2 * IsThorax);
	SenseFootAtStep (DeltaTime, LMB(Left), Hit.ImpactPoint, FallSeverity, 2 * IsThorax + 1);

	//ноги также оказывают влияние на чувство опоры, если две ноги не нашли опору, то брюшной сенсор не играет роли, всё равно нет земли
	if (LMB(Right).Stepped + LMB(Left).Stepped == 0 && !StartLimb.OnCapsule()) StartLimb.Stepped = OldStepped-1;
	//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

	//отладка устойчивости - только после обработки ног
	LINELIMBWTC(ELimbDebug::GirdleStandHardness, StartLimb, (FVector)StartLimb.ImpactNormal * 15, 1, 0.03, StandHardness/255.0f);

	//индикация направления
	LINELIMBWT(ELimbDebug::GirdleGuideDir, StartLimb, (FVector)GuidedMoveDir * 15, 1, 0.04f);
	LINELIMBWTC(ELimbDebug::GirdleCrouch, StartLimb, (FVector)Hit.Normal * 20, 1, 0.04f, Crouch);

	//сверху от туловище рисуем тестовые индикаторы
	if (LDBG(ELimbDebug::GirdleConstrainMode))
	{
		auto Ct = BDY(Center)->GetCOMPosition() + StartLimb.ImpactNormal * 15;
		auto Color = FColor(255, 0, 255, 10);
		float ang = CI->ProfileInstance.ConeLimit.Swing1LimitDegrees;
		float ang2 = CI->ProfileInstance.TwistLimit.TwistLimitDegrees;
		if (Steppable)			DrawDebugString(GetWorld(), (FVector)Ct, TEXT(" S"), nullptr, FColor(255, 0, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (Climbing)			DrawDebugString(GetWorld(), (FVector)Ct, TEXT("   C"), nullptr, FColor(0, 255, 255), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (Vertical)			DrawDebugString(GetWorld(), (FVector)Ct, TEXT("     V"), nullptr, FColor(ang, 255, 0), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (NoTurnAround)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("       Y"), nullptr, FColor(255, 0, ang2), 0.02, false, Lead() ? 1.5f : 1.0f);
		if (FixedOnFloor)		DrawDebugString(GetWorld(), (FVector)Ct, TEXT("         F"), nullptr, FColor(255, 255, 255), 0.02, false, Lead() ? 1.5f : 1.0f);
	}

	//если мы только включили режим карабканья и что-то не получилось, запустить вспомогательные действиия
	if (MyrOwner()->bWannaClimb) MyrOwner()->ClimbTryActions();

	return false;
}

//==============================================================================================================
//побочная трассировка позици ноги,  коррекция раскорячивания ног
//==============================================================================================================
void UMyrGirdle::SenseFootAtStep(float DeltaTime, FLimb& Foot, FVector CentralImpact, float FallSeverity, int ATurnToCompare)
{
	//центральный членик, туловище
	FLimb& CNTR = LMB(Center);

	//старт трассировки,уровень плечь или таза
	const FVector Start = me()->GetLimbShoulderHubPosition(Foot.WhatAmI);

	//направление вовне в сторону
	FVector3f Lateral = Foot.IsLeft() ? LAXIS(LMB(Center), Left) : LAXIS(LMB(Center), Right);

	//распаковка в абсолютные координаты
	FVector3f CurFootRay = GetLegRay(Foot);

	//суррогатная точка контакта, если новой не считать, из центрального следа и в сторону
	FVector EndPoint = Start + (FVector)CurFootRay;

	//размах ноги в сторону - обратная связь для коррекции размаха
	float SwingAlpha = 0.0;

	//скорость обновления размаха ног
	float SpeedAlpha = 0.1;

	//критерий необходимости детального просчёта ноги с трассировкой в поверхность
	bool TraceOn = CNTR.Stepped >= STEPPED_MAX - 1 &&				// брюшной сенсор уже нашупал опору
		me()->GetPredictedLODLevel() < 2 &&							// модель сблизи, вдали нах такие сложности
		!CNTR.OnThinCapsule();										// туловище не держится на тонкой ветке, иначе трассировка ног только ухудшает всё

	//такт выделенный на полный просчёт этой ноги
	bool MyTime = ((MyrOwner()->FrameCounter & 3) == ATurnToCompare);

	//пределы раскоряки для данного режима моторики
	float SwingMin = CurrentDynModel->LegsSpreadMin * TargetFeetLength;
	float SwingMax = CurrentDynModel->LegsSpreadMax * TargetFeetLength;

	//самое желаемое положение ноги без оглядки на реальные точки земли
	//базовый вариант, не учитывающи расставку ног
	FVector3f DesiredRay = (FVector3f)(CentralImpact - Start);

	//учет мах туловищем вперед назад
	DesiredRay += (LAXIS(CNTR, Down) * GetTargetLegLength()).ProjectOnTo(GuidedMoveDir);

	//критерии полного просчёта ноги = вне очереди при падении или своя очередь, для оптимизации раз в 4 такта
	if (TraceOn)										
	{
		if (FallSeverity > 0 || MyTime)
		{
			//глубина трассировки вниз 
			//1. ногами вниз глубже, чтоб не терять опору, 2. для бега глубже по тем же причинам
			float Depth = (1 + Crouch) * (1.2 - 0.2*LAXIS(CNTR, Down).Z);
			if (Speed > 100) Depth *= 1 + (Speed - 100) * 0.002;

			//###############################
			//сюда скидывать результаты
			FHitResult Hit(ForceInit);
			if (Trace(Start, CurFootRay * Depth, Hit))
			{
				//сохранить часть данных сразу в ногу
				auto FM = Hit.PhysMaterial.Get();
				if (FM) Foot.Surface = (EMyrSurface)FM->SurfaceType.GetValue();

				//дополнение случая приземления через ногу - пересчитать силу удара конкретно для этой ноги
				//скорость берется по оси нормали, чтобы избежать касательных быстрот, но общий модуль тоже учитывается
				if (IS_MACRO(MyrOwner()->CurrentState, AIR))
					FallSeverity = me()->ShockResult(Foot, (VelocityAgainstFloor | (-(FVector3f)Hit.Normal)) + Speed*0.1, Hit.Component.Get());

				//сохранить часть данных сразу в ногу
				me()->GetFloorFromHit(Foot, Hit);
				Foot.ImpactNormal = (FVector3f)Hit.ImpactNormal;
				StandHardness += 100;

				//вариация размаха ног при касании опоры - только во время шага или в момент падения
				if (MyrOwner()->MoveGain > 0 || FallSeverity > 0)
				{
					//расчёт показаеля отставки или вжатия ноги с этой стороны
					if (CNTR.OnThinCapsule()) SwingAlpha = 0;
					else
					{
						//кривизна в ноге: + опора выпуклая или кренимся в другую сторону, - опора вогнутая или кренимся в эту сторону
						//очевидно, что расставка ног должна быть обратна этому
						float FootUneven = Foot.ImpactNormal | Lateral;

						//крен в центре, кривизну поверхности не определяет, для обеих ног идентичен до знака
						//очевидно, что расставка ног должна быть с обратным знаком
						float CenterUneven = CNTR.ImpactNormal | Lateral;

						//влияние вертикальности через нормаль в центре - на пологой ровной поверхности ноги луче сжать, на вертикальной - максимально расставить
						SwingAlpha = -CNTR.ImpactNormal.Z - CenterUneven - FootUneven;
						SwingAlpha = FMath::Clamp(0.5 * SwingAlpha + 0.5, 0.0f, 1.0f);//перевод из [-1;+1] в [0;+1]
					}

					//скорость приближения к желаемому отгибу ноги пропроциональна скорости
					SpeedAlpha = DeltaTime * 4 * FMath::Min(FMath::Abs(Speed) * 0.01f, 1.0f);
				}
				//для стоячей стойки ноги не должны двигаться в сторону, скорость нулевая
				else SpeedAlpha = 0;

				//обновление вектора на ногу из посчитанных данных, реальная нога будет немного сдвинута по тренду раскоряки
				CurFootRay = (FVector3f)(Hit.ImpactPoint - Start);
			}
			//луч не нащупал опоры под этой ногой
			else
			{
				//на воздухе пусть будет вариация между размахами ног через уровень вертикальности
				if (CNTR.OnCapsule()) SwingAlpha = 0;
				else SwingAlpha = FMath::Max(LAXIS(CNTR, Up).Z, 0.0f);

				//стремление к желаемой расокряке - как и для трассировки, при стоянии не происходит
				SpeedAlpha = (MyrOwner()->MoveGain > 0) ? 1.5*DeltaTime : 0.0f;
			}
		}
		//междутактье нормальной трассировки
		else
		{
			//стойкость обнуляется извне каждый кадр вне зависимости от чередов ног, ее нужно обновлять
			if (Foot.Stepped || Climbing) StandHardness += 100;

			//стремление к желаемой расокряке - как и для трассировки, при стоянии не происходит
			SpeedAlpha = MyrOwner()->MoveGain > 0 ? 0.5*DeltaTime : 0.0f;
		}
	}
	//просчёт по простому пути без трассировки
	else
	{
		//принять посчитанные данные из центра пояса
		if (CNTR.Stepped == STEPPED_MAX)
		{	StandHardness += 100;
			Foot.TransferFrom(LMB(Center));
			Foot.ImpactNormal = CNTR.ImpactNormal;
		}
		//в случае, если трассировка отменена из-за нахождения в воздухе
		else
		{	SwingAlpha = FMath::Max(LAXIS(CNTR, Up).Z, 0.0f);
			SpeedAlpha = 3*DeltaTime;
			if (Foot.Stepped) Foot.EraseFloor();
		}
	}

	// если до была обнаружена ненулевая сила удара (или сверху, из просчёта туловища, или здесь)
	if (FallSeverity > 0)
		MyrOwner()->Hurt(Foot, FallSeverity, EndPoint, LMB(Center).ImpactNormal, Foot.Surface);

	//сначала безусловно направляем окончательный вектор к желаемому без учёта раскоряки ног
	//чтобы по вертикали ноги сразу опускались
	CurFootRay = FMath::Lerp(CurFootRay, DesiredRay, DeltaTime);
	
	//затем дополнить желаемый вектор в бок ноги от центра - желаемая расставка ног
	DesiredRay += Lateral * FMath::Lerp(SwingMin, SwingMax, SwingAlpha);

	//устремить окончательный вектор (плечо - ступня) в абсолютных координатах к желаемому с расставкой ног
	CurFootRay = FMath::Lerp(CurFootRay, DesiredRay, SpeedAlpha);

	//костыль, чтоб не качались, при жесткой вертикали резко поддержать вертикальность в плоскости хода
	if (Vertical) PROJ(CurFootRay, GuidedMoveDir);

	//загиб ступни вперед/назад
	Foot.FootPitch() = 0.5 + ((FVector3f)(EndPoint - Start) | GuidedMoveDir) / 2 / TargetFeetLength;

	//запоковка в локальные координаты спины
	SetLegRay(Foot, CurFootRay);
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
	if(CLimb.ImpactNormal.Z < 0.5)
	{ 	PROJ(HorDir, CLimb.ImpactNormal);
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
