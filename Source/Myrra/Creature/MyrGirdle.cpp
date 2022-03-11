// Fill out your copyright notice in the Description page of Project Settings.


#include "MyrGirdle.h"
#include "../MyrraGameInstance.h"						// ядро игры
#include "MyrPhyCreature.h"								// само существо
#include "MyrPhyCreatureMesh.h"							// тушка
#include "DrawDebugHelpers.h"							// рисовать отладочные линии

//рисовалки отладжочных линий
#if WITH_EDITOR
#define LINE(C, A, AB) MyrOwner()->Line(C, A, AB)
#define LINEWT(C, A, AB,W,T) MyrOwner()->Line(C, A, AB,W,T)
#define LINEW(C, A, AB, W) MyrOwner()->Line(C, A, AB, W)
#define LINELIMB(C, L, AB) MyrOwner()->Line(C, me()->GetMachineBody(L)->GetCOMPosition(), AB)
#define LINELIMBW(C, L, AB, W) MyrOwner()->Line(C, me()->GetMachineBody(L)->GetCOMPosition(), AB, W)
#define LINELIMBWT(C, L, AB, W, T) MyrOwner()->Line(C, me()->GetMachineBody(L)->GetCOMPosition(), AB, W, T)
#define LDBG(C) MyrOwner()->IsDebugged(C)
#else
#define LDBG(C) false
#define LINE(C, A, AB)
#define LINELIMB(C, L, AB)
#define LINELIMBW(C, L, AB, W)
#define LINELIMBWT(C, L, AB, W, T) 
#define LINEW(C, A, AB, W)
#endif

//меш этого существа
UMyrPhyCreatureMesh* UMyrGirdle::me() { return MyrOwner()->GetMesh(); }

//статический, таблица поиска абсолютного идентификатора части тела по его относительному идентификатору ("лучу")
ELimb UMyrGirdle::GirdleRays[2][(int)EGirdleRay::MAXRAYS] =
{ {ELimb::PELVIS,		ELimb::L_LEG,	ELimb::R_LEG,	ELimb::LUMBUS,	ELimb::TAIL},
	{ELimb::THORAX,		ELimb::L_ARM,	ELimb::R_ARM,	ELimb::PECTUS,	ELimb::HEAD}
};

//выдать членик и его физ-тело в системе коориднат конкретно этого пояса
FLimb&			UMyrGirdle::GetLimb (EGirdleRay R)	{ return me() -> GetLimb		(GetELimb(R)); }
FBodyInstance*  UMyrGirdle::GetBody (EGirdleRay R)	{ return me() -> GetMachineBody	(GetELimb(R)); }

//констрайнт, которым спинная часть пояса подсоединяется к узловой (из-за единой иерархии в разных поясах это с разной стороны)
FConstraintInstance* UMyrGirdle::GetGirdleSpineConstraint() { return (IsThorax) ? me()->GetMachineConstraint(me()->Thorax) : me()->GetMachineConstraint(me()->Lumbus); }
FConstraintInstance* UMyrGirdle::GetGirdleArchSpineConstraint() { return (IsThorax) ? me()->GetArchMachineConstraint(me()->Thorax) : me()->GetArchMachineConstraint(me()->Lumbus); }

//то же для краткости и инлайновости
#define ELMB(R)							GirdleRays[IsThorax][(int)EGirdleRay::R]
#define LMB(R)		me()->GetLimb       (GirdleRays[IsThorax][(int)EGirdleRay::R])
#define BDY(R)		me()->GetMachineBody(GirdleRays[IsThorax][(int)EGirdleRay::R])

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

	//чтобы таскать отдельно от остального тела
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
	me()->InitLimbCollision(LMB(Left), 100000);
	me()->InitLimbCollision(LMB(Right), 100000);
		
	////////////////////////////////////////////////////////
	//инициализировать зацеп
	BDY(Center)->DOFConstraint = me()->PreInit();
	auto CI = BDY(Center)->DOFConstraint;

	//фрейм в мире самого членика = его локальные оси в его же локальных координатах, не меняются
	CI->PriAxis1 = MyrAxes[(int)me()->MBoneAxes().Up];
	CI->SecAxis1 = MyrAxes[(int)me()->MBoneAxes().Forth];
	CI->Pos1 = FVector(0,0,0);

	//фрейм в мире данного компонента = первичная ось=верх, вторичная=вперед, пока неясно, нужно ли смещать позицию от нуля
	CI->PriAxis2 = FVector(0,0,1);	//вверх, вертикаль
	CI->SecAxis2 = FVector(-1,0,0);	//почему тут минус, ВАЩЕ ХЗ Я В АХУЕ, но без него при ограничении по этой оси существо переворачивается
	CI->Pos1 = FVector(0,0,0);

	//окончательно инициализировать привязь, возможно, фреймы больше не будут меняться
	CI->InitConstraint(BDY(Center), GetBodyInstance(), 1.0f, this);

	//на старте убрать вообще все стеснители
	me()->SetFreedom(CI, -1, -1, -1, 180, 180, 180);

	//////////////////////////////////////////////////
	Super::BeginPlay();
}

//==============================================================================================================
//возможно ли этим поясом немедленно прицепиться к опоре (если
//==============================================================================================================
EClung UMyrGirdle::CanGirdleCling()
{
	// если не ведущий
	//пока убрана возможность прикрепляться ведомому, слишком много нюансов кинематики
	if (!CurrentDynModel->Leading)	
		//if (!MyrOwner()->GetAntiGirdle(this)->FixedOnFloor)//и в это время главный тоже еще не включен
			return EClung::NoLeading;//◘◘>

	//лучшая из опор ног заносится в центральный членик
	auto& CentralLimb = LMB(Center);

	//есть ноги
	if (HasLegs)
	{
		//если нет опоры вообще
		if (!CentralLimb.Floor)
			return EClung::NoSurface;//◘◘>

		//нельзя пришпиливаться к опорам, на которые сказано что нельзя ваще
		if (CentralLimb.Floor->OwnerComponent->CanCharacterStepUpOn != ECanBeCharacterBase::ECB_Yes)
			return EClung::BadSurface;//◘◘>

		//если опора хорошая, но материал нелазибельный тоже нельзя
		if (!CentralLimb.IsClimbable()) return EClung::NoClimbableSurface;//◘◘>

		//если сейчас подходит, но уже и так включено - запрос на обновление фреймов
		if (FixedOnFloor) return EClung::Update;//◘◘>

		//если еще не включено - запрос на включение
		else return EClung::Recreate;//◘◘>

	}
	//ног не, елозит на брюхе
	else
	{
		//безног устойчивость показывает неверный угол/крен и важна
		if(StandHardness < 0.5)	return EClung::BadAngle;//◘◘>

		//если хотя бы один из сегментов брюха, принадежащий этому поясу, стоит на нужной поверхности
		if (CentralLimb.IsClimbable() || LMB(Spine).IsClimbable())
		{	if (FixedOnFloor)	return EClung::Update;  //◘◘>
			else				return EClung::Recreate;//◘◘>
		}
	}
	return EClung::BadSurface;//◘◘>
}

//==============================================================================================================
//пересчитать реальную длину ног
//==============================================================================================================
float UMyrGirdle::GetFeetLengthSq(bool Right)
{
	//расчёт длин ног и рук
	if (HasLegs)
	{
		//берем для примера левую конечность 
		ELimb eL = Right ? ELMB(Right) : ELMB(Left);

		//у конечности должен быть сокет, расположеный на самом кончике, он же используется для постановки следа и пыли
		FName TipName = me()->FleshGene(eL)->GrabSocket;

		//если сокета нет придётся брать локацию физического членика (он обычно выше)
		FVector Tip = TipName.IsNone() ? me()->GetMachineBody(eL)->GetCOMPosition() : me()->GetSocketLocation(TipName);

		//другой конец тоже неочевиден - обычно это конец "ключицы" или специальная кость, помечающая место вокруг которого конечность крутится
		FVector Root = me()->GetLimbShoulderHubPosition(eL);

		//для быстроты берется квадрат расстояния
		return FVector::DistSquared(Root, Tip);
	}
	return 0;
}


//вовне, главным образом в анимацию AnimInst - радиус колеса представлющего конечность
float UMyrGirdle::GetLegRadius()
{
	auto& ag = me()->GetMachineBody(IsThorax ? ELimb::R_ARM : ELimb::R_LEG)->GetBodySetup()->AggGeom;
	if (ag.SphereElems.Num()) return ag.SphereElems[0].Radius; else
	if (ag.SphylElems.Num()) return ag.SphylElems[0].Radius*0.01; return 0.0f;
}

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

	//для конечностей, если есть
	if (HasLegs)
	{
		auto& RLimb = LMB(Right);
		auto& LLimb = LMB(Left);
		me()->AdoptDynModelForLimb(RLimb, Models.Left, Models.CommonDampingIfNeeded);
		me()->AdoptDynModelForLimb(LLimb, Models.Right, Models.CommonDampingIfNeeded);
		auto aRBody = me()->GetArchMachineBody(RLimb.WhatAmI);
		auto aCBody = me()->GetArchMachineBody(LMB(Center).WhatAmI);

		//минимальная плотность ног берется из плотонсти таза
		const float MinFeetMassScale = aCBody->MassScale * 0.5;
		const float MAxFeetMassScale = aRBody->MassScale;
		BDY(Left)-> SetMassScale (Models.HeavyLegs ? MAxFeetMassScale * (1 - 0.1 * FMath::Min(LLimb.Damage, 1.0f)) : MinFeetMassScale);
		BDY(Right)->SetMassScale (Models.HeavyLegs ? MAxFeetMassScale * (1 - 0.1 * FMath::Min(RLimb.Damage, 1.0f)) : MinFeetMassScale);
	}

	//запретить ногам махать вдоль спины - сомнительная надобность, опасно
	SetSpineLock(Models.LegsSwingLock);

}


//==============================================================================================================
//обработать одну конкретную конечность пояса
//==============================================================================================================
float UMyrGirdle::ProcedeFoot (FLimb &FootLimb, FLimb& OppositeLimb, float FootDamping, float& Asideness, float &WeightAccum, float DeltaTime)
{
	float Weight = 0;

	//физическое тело ноги и его прототип из первоночальных настроек
	FBodyInstance* FootBody = me()->GetMachineBody(FootLimb);
	FBodyInstance* ArchFootBody = me()->GetArchMachineBody(FootLimb);

	//центральный хабовый членик
	FLimb& CentralLimb = LMB(Center);
	FBodyInstance* CentralBody = BDY(Center);

	//повреждение ноги ограничивается, чтоб не получить заоблачных производных вычислений
	float ClampedDamage = FMath::Min(FootLimb.Damage, 1.0f);

	//уточнение актуальности флага приземленности на опору - трассировка
	//возможно, проще переделать трассировкой линией
	if (FootLimb.Stepped == 1)
	{
		//сюда скидывать результаты
		FHitResult Hit(ForceInit);

		//направление и глубина трэйса - от нормали на 0.2 диаметра ножного колеса в опору
		FVector StepDst = -FootLimb.ImpactNormal * 1.2 * GetLegRadius();
		const FVector Start = FootBody->GetCOMPosition();

		//параметры: фолс - не трассировать по полигонам, последний параметр - игнрорить себя
		FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("Daemon_TraceForSteppedness")), false, MyrOwner());

		//проверить из центра колеса
		GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + StepDst, ECC_WorldStatic, RV_TraceParams);

		//обнаружена опора (это не совсем правильно, нужно проверять та же опора или нет)
		if (Hit.bBlockingHit)
		{	FootLimb.Stepped = STEPPED_MAX;
			UE_LOG(LogTemp, Verbose, TEXT("%s: ProcessGirdleFoot %s stepped restore for %s"),
				*GetOwner()->GetName(), *TXTENUM(ELimb, FootLimb.WhatAmI), *FootLimb.Floor->OwnerComponent->GetName());
		}
		//опоры не обнаружено, стереть инфу об опоре
		else
		{	FootLimb.EraseFloor();
			UE_LOG(LogTemp, Verbose, TEXT("%s: ProcessGirdleFoot %s trace not found floor, untouch"),
				*GetOwner()->GetName(), *TXTENUM(ELimb, FootLimb.WhatAmI));
		}

		LINELIMBWT(ELimbDebug::LineTrace, FootLimb, StepDst, Hit.bBlockingHit ? 2 : 1, 1.5);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//привязь этой ноги к туловищу
	auto Cons = me()->GetMachineConstraint(FootLimb);
	auto ARCons = me()->GetArchMachineConstraint(FootLimb);

	//амплитуда ног - пока неясно, какой параметр использовать, таргет или лимит, лимит обычно указывается больше
	float FreeFeetLength = FMath::Abs(ARCons->ProfileInstance.LinearDrive.PositionTarget.Z);/*FMath::Abs(ARCons->ProfileInstance.LinearLimit.Limit);*/

	//абсолютный сдвиг целевого положения ноги в сантиметрах от нуля (полусогнутости)
	//в горизонте без урона и пригиба = 1 * FreeFeetLength, то есть выгиб в плюс, в сторону земли до полного выпрямления ног
	float AbsoluteCrouch = FreeFeetLength * (1 - 2 * FMath::Max(Crouch, ClampedDamage));

	//финальная запись точки равновесия лапки
	//поскольку мы не знаем, на какой оси висит направление вниз, изменяем сразу все, нужная определяется режимами лимитов
	Cons->SetLinearPositionTarget(FVector(AbsoluteCrouch, AbsoluteCrouch, AbsoluteCrouch));

	//сила между колесом ноги и туловищем - вот это значение по умолчанию для режима карабканья
	//вот здесь неясно, в редакторе сила сразу для всех устанавливается
	float ElbowForce = ARCons->ProfileInstance.LinearDrive.ZDrive.Stiffness
		* (1.0 - 0.3*FootLimb.Damage - 0.3*OppositeLimb.Damage);
	if (ElbowForce < 0) ElbowForce = 0;

	//пока неясно, как лучше расчитывать скорость
	//брать саму скорость не из членика-колеса, а из центрального членика
	FVector RelativeSpeedAtPoint = CentralBody->GetUnrealWorldVelocity();

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//нога на опоре, опору знаем, по ней можем позиционировать тело
	//второе условие - исключить контакты колёс в верхней полусфере - это не касание лап, это паразитное
	//возможно, ограничение на широту ввести в в генофонд
	if(FootLimb.Stepped)
	{
		//пока пригнутость меньше единицы, сила выпрямления снижается с уровнем пригнутости, чтобы не противиться гравитации и разгибать наболее низком склоне
		//но можно вывернуть экстремальную пригнутость больше единицы, чтобы ноги ещё и активно подтягивались
		float CrouchInfluence = Crouch < 1 ? (-0.5f * Crouch) : (0.5 * Crouch - 1);

		//расчёт силы распрямления индивидуальной лапки
		ElbowForce *= FMath::Max(0.0f,					// чтобы выдержать снижающие слагаемые и не уйти в минус
			0.6f * FootLimb.ImpactNormal.Z				// чем круче опора, тем меньше сила - видимо, чтобы на горке ноги повторяли кочки
			+ CrouchInfluence);							// ослабление или усиление разгибания от степени намеренного пригиба тела
			
		//сделать скорость относительной, вычтя скорость пола в точке
		RelativeSpeedAtPoint -= FootLimb.Floor->GetUnrealWorldVelocityAtPoint (FootBody->GetCOMPosition());

		//финальное значение плавно подвигать - хз, может лишнее
		VelocityAgainstFloor = FMath::Lerp(VelocityAgainstFloor, RelativeSpeedAtPoint, 0.1f);

		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
		//вычислить направление движения для данной ноги из внешних факторов
		//здесь вычисляется вес ноги как степень сонаправленности исходного курса
		//и той дороги, куда сама опора вынуждает двигаться данную ногу
		//например, у более пологой опоры будет больший вес, потому что начальный курс скорее всего в плоскости земли
		FVector GuideDir = me()->CalculateLimbGuideDir ( FootLimb, MyrOwner()->MoveDirection, true, &Weight);

		//боковизна касания, для вычисления кривизны
		Asideness = FootLimb.ImpactNormal | BAXIS(CentralBody,Right);

		//коэффициент боковизны касания, учитывающий только экстремальные крены
		float SideHardness = FMath::Clamp(1.5f - FMath::Abs(Asideness), 0.1f, 1.0f);

		//при экстремальных кренах вес ноги падает
		Weight *= SideHardness;

		//при кренах лапа расслабляется, видимо, чтобы плавно обнять препятствие
		ElbowForce *= 1.0f - FMath::Abs(Asideness);

		//вклад в устойчивость стойки сильно падает, если эта нога касается опоры боком
		//а если это атакующая конечность - так вообще нуль, чтобы отключать на них анимацию шага
		if (!MyrOwner()->IsLimbAttacking(FootLimb.WhatAmI))
			StandHardness += 0.5 * SideHardness;

		//если пол на этом такте меняется, надо крайне осторожно изменять направления
		if (CentralLimb.Floor != FootLimb.Floor) Weight *= 0.3;

		//чтобы был хоть какой-то вес-множитель, иначе можно думать, что нет опоры
		if (Weight < 0.1) Weight += 0.1;

		//если это первая нога или более весомая, чем предыдущая
		//назначаем ее ответственной за сообщение опоры для всего пояса
		//если далее будет более весомая нога, она перепишет воё
		if (Weight > WeightAccum)
			CentralLimb.GetFloorOf(FootLimb);

		//случай если нога вторая и менее весомая чем первая - всё равно пусть будет вес ненулевой
		else Weight = 0.05;

		//начать/продолжить набор взвешенной суммы
		GuidedMoveDir += GuideDir * Weight;

		//взвешенно накапливать нормаль для спины только если не карабкаемся по ветке
		//иначе спина сама подчитает для себя оптимальное положение нормали
		if(!FixedOnFloor || !FootLimb.OnCapsule())
			CentralLimb.ImpactNormal += FootLimb.ImpactNormal * Weight;

		//применение вязкости среды для колеса ноги - торможение
		//сбавлять тормоз сразу - для разгона, а наращивать плавно - тормозной путь
		me()->BrakeD ( FootBody->LinearDamping,	FootDamping,	DeltaTime);
		me()->BrakeD ( FootBody->AngularDamping,FootDamping*10, DeltaTime);
		FootBody->UpdateDampingProperties();

		//если включены "тяжелые ноги" удельный вес ног все же зависит от их повреждений - чтобы при сильном увечьи легче валиться на бок
		if(CurrentDynModel->HeavyLegs)
			FootBody->SetMassScale(ArchFootBody->MassScale*(1 - 0.5 * ClampedDamage));

		//настройки силы сгиба лапы (это надо переписать так чтобы не вытаскивать каждый раз 3 параметр
		Cons->SetLinearDriveParams(ElbowForce,						// собственно сила
			100,													// инерция скорости, чтоб не пружинило, но пока неясно, как этим управлять
			ARCons->ProfileInstance.LinearDrive.ZDrive.MaxForce);	// максимальная сила - копируем из редактора (или разбить функцию на более мелкие, глубокие) или вообще пусть 0 будет
		LINELIMB(ELimbDebug::GirdleGuideDir, FootLimb, GuideDir * 5 * (1 + 2 * Weight));

	}
	else
	{
		//без опоры скорость мееедленно переходит на отсчёт абсолютный
		VelocityAgainstFloor = FMath::Lerp ( VelocityAgainstFloor, RelativeSpeedAtPoint, DeltaTime);

		//пока нога в воздухе, меедленно смещать ее "нормаль" к направлению вверх туловища
		FootLimb.ImpactNormal = FMath::Lerp(FootLimb.ImpactNormal, BAXIS(CentralBody,Up), DeltaTime);
		
		//раз и на весь период полета делаем полетными праметры сил ноги
		if(FootBody->LinearDamping != 0.01f) 
		{	me()->SetLimbDamping (FootLimb, 0.01f, 0);
			Cons->SetLinearDriveParams(ElbowForce, 1, 50000);
		}
	}

	//аккумуляция веса
	WeightAccum += Weight;

	return Weight;
}

//==============================================================================================================
//покадрово сдвигать и вращать
//==============================================================================================================
void UMyrGirdle::Procede(float DeltaTime)
{
	//центральный ограничитель
	auto& CentralLimb = LMB(Center);
	auto CentralBody = BDY(Center);
	auto CentralUpAxis = BAXIS(CentralBody,Up);
	auto CI = CentralBody->DOFConstraint;

	//новый запрос на смену вертикали 
	bool NeedVertical = CurrentDynModel->HardVertical &&	//если просят извне
		LMB(Center).Damage + LMB(Spine).Damage < 0.8f;		//если урон частям пояса невелик
	bool TransToVertical = Vertical && CI->ProfileInstance.ConeLimit.Swing1LimitDegrees > 0;

	//новый запрос на смену режима строго азимута
	bool NeedYawLock = CurrentDynModel->HardCourseAlign;
	bool TransToYawLock = NoTurnAround && CI->ProfileInstance.TwistLimit.TwistLimitDegrees > 0;

	//новый запрос на пришпиленность
	bool NeedFixed = MyrOwner()->bClimb || CurrentDynModel->FixOnFloor;

	//устойчивость = обнулить с прошлого кадра + внести слабый вклад других частей, чтобы при лежании на боку было >0
	StandHardness = 0.1 * (bool)(LMB(Spine).Stepped) + 0.012 * (bool)(LMB(Tail).Stepped);

	//уровень пригнутости для пояса - просто сглаженное повторение циферки из дин.модели
	Crouch = FMath::Lerp (Crouch, CurrentDynModel->Crouch, DeltaTime);

	FrameCounter++;

	///////////////////////////////////////////////////////////////////////////////
	//расчёт торможения для ног или брюха, чтоб не скатывалось когда не надо
	//внимание, на этом месте берутся данные с предыдущего такта, это не совсем правильно
	//однако полностью развязать отдельные такты, возможно, не получится

	//степень торможения ногами или брюхом, если ног нет - сначала базис из внешней модели динамики
	float DampingForFeet = CurrentDynModel->FeetBrakeDamping;

	//если прямо в сборке дин-модели указан ноль (не тормозить) то вообще не нужно ничего вычислять
	if(DampingForFeet > 0 && MyrOwner()->Health > 0)
	{
		//почти не тормозить если склон слишком крут (соскользнуть)
		if(!FixedOnFloor && CentralLimb.ImpactNormal.Z < 0.7f) DampingForFeet *= 0.05f;

		//тело в состоянии активного сужения лимитов в нужных углах - не мешать ему перекатиться в нужную позу
		else if(TransToVertical || TransToYawLock) DampingForFeet *= 0.01f;

		//подгон торможения под отклонения в скорости
		else 
		{
			//скорость в направлении желаемого движения и в боковом направлении (в некоторых режимах этого стоит избегать)
			const float SpeedAlongCourse  = FMath::Abs ( FVector::DotProduct (VelocityAgainstFloor, GuidedMoveDir));
			const float SpeedAlongLateral = FMath::Abs ( FVector::DotProduct (VelocityAgainstFloor, BAXIS(CentralBody,Right)));
			const float WantedSpeed = MyrOwner()->GetDesiredVelocity();

			//если набранная скорость маловата - не сдерживать движение вообще
			float Overspeed =  SpeedAlongCourse - WantedSpeed;

			//для ведомого пояса контролировать скорость в сторону, чтоб предотвращять виляние задом влево-вправо
			if(!CurrentDynModel->Leading) Overspeed += SpeedAlongLateral;

			//если скорость недостигла превышения, просто ваще не тормозить
			if (Overspeed < 0)	DampingForFeet = 0.0f;

			//если хотим двигаться, но медленнее чем сейчас - плавно усилить тормоз, не стопоря движ
			//вообще тут был раньше базисный множитель, но с ним как-то резко переходит с бега на шаг, тормозит до нудля
			else if (WantedSpeed > 0) DampingForFeet = Overspeed / WantedSpeed;

			//Overspeed>=0 WantedSpeed=0 - желание полностью остановиться
			//базовое значение торможения, зберется базовый коэф и в 10 раз увеличивается при крутом склоне
			else DampingForFeet = CurrentDynModel->FeetBrakeDamping * (10 - 9*CentralLimb.ImpactNormal.Z);
		}
	}

	LINELIMB(ELimbDebug::FeetBrakeDamping, CentralLimb, -GuidedMoveDir*DampingForFeet);

	//////////////////////////////////////////////////////////////////////////////
	//просчёт общиз векторов вперед, вверх
	//в этом поясе ноги есть
	if(HasLegs)
	{
		//части тела
		auto& RLimb = LMB(Right);
		auto& LLimb = LMB(Left);

		//предобнулить накопление влияний ног на позицию туловища
		float WeightAccum = 0.0f;

		//стереть лучшую опору с прошлого кадра, чтобы набрать новых
		CentralLimb.EraseFloor();

		//процедура, взвешивание и накопление по ногам
		float RAside = 0, LAside = 0;
		float LW = ProcedeFoot (LLimb, RLimb, DampingForFeet, LAside, WeightAccum, DeltaTime);
		float RW = ProcedeFoot (RLimb, LLimb, DampingForFeet, RAside, WeightAccum, DeltaTime);

		//новый способ определения кривизны поверхности
		Curvature = FMath::Lerp(Curvature, RAside - LAside, 0.2);

		//аккумулятор веса нулевой - обе ноги в воздухе
		if(WeightAccum == 0)
		{	
			//ориентация на чистый курс из контроллера нужена для полета
			GuidedMoveDir  = MyrOwner()->MoveDirection;
			CentralLimb.ImpactNormal = GuidedMoveDir^(BAXIS(CentralBody, Right));
			CentralLimb.ImpactNormal.Normalize();
			Forward = FVector::CrossProduct(BAXIS(CentralBody, Right), CentralLimb.ImpactNormal);
			Forward.Normalize();
			NeedVertical = false;
			NeedYawLock = false;

			if(NeedFixed) me()->Log(CentralLimb, TEXT("ProcessGirdle NeedFixed=0 Cause no touch"));
			NeedFixed = false;
		}
		//одна или две ноги стоят на поверхности
		else
		{
			//направление движение нормировать после взвешенной суммы по ногам
			GuidedMoveDir.Normalize();

			//для режима лазанья по ветке-капсуле не использовать веса из ног
			if (FixedOnFloor && CentralLimb.OnCapsule())
			{
				//использовать прямой путь к капсуле от центра спины, что получить нормаль
				//так будет однозначная нормаль вне зависимости от положения ног обнимающих ветку
				//эта нормаль будет перпендикулярна курсу (вдоль капсулы) везде кроме попок,
				//поэтому ортогонализировать ее всё же нужно
				FVector Ctr = CentralBody->GetUnrealWorldTransform().GetLocation();
				FVector PointOnFloor;
				float DistToFloor;
				CentralLimb.Floor->GetSquaredDistanceToBody(Ctr, DistToFloor, PointOnFloor);

				//само расстояние пока не используется (чтоб не считать корень), оно просто прибавляется целиком
				//в надежде на дальнейшую нормировку. Но можно ввести какой-нибудь коэффициент для плавности
				CentralLimb.ImpactNormal += (Ctr - PointOnFloor);
			}

			//ортогонализация взвешенной нормали пояса (всегда под 90* к вектору курса и вверх, а не вниз)
			CentralLimb.ImpactNormal = GuidedMoveDir ^ (GuidedMoveDir ^ CentralLimb.ImpactNormal);
			if (FVector::DotProduct(CentralLimb.ImpactNormal, BAXIS(CentralBody, Up)) < 0)
				CentralLimb.ImpactNormal *= -1;

			//нормализовать
			CentralLimb.ImpactNormal.Normalize();

			//вектор "правильный вперед", корректирующий pitch'и обеих частей туловища
			Forward = FVector::CrossProduct(BAXIS(CentralBody, Right), CentralLimb.ImpactNormal);
			Forward.Normalize();

			//расчёт кривизны = как-то сложно
			/*Curvature = FVector::DotProduct(
				Forward,	FVector::CrossProduct(RLimb.ImpactNormal, CentralLimb.ImpactNormal) -
							FVector::CrossProduct(LLimb.ImpactNormal, CentralLimb.ImpactNormal));*/

			//разбор критериев "аварийного" сброса ограничений на позицию пояса
			///////////////////////////////////
			//если тело повернуто сильно не так как желаемый курс - пока не включать жестко заданный азимут
			if (FVector::DotProduct(GuidedMoveDir, BAXIS(CentralBody, Forth)) < 0.1)
			{	if(NeedYawLock) me()->Log(CentralLimb, TEXT("ProcessGirdle NeedYawLock=0 Cause Yaw Extreme"));
				NeedYawLock = false;
			}

			//суммарный крен в одну сторону - признак явной неустойчивости
			if( FMath::Abs(RAside+LAside) > 1.4)
			{	if(NeedVertical) me()->Log(CentralLimb, TEXT("ProcessGirdle NeedVertical=0 Cause Roll aside"));
				NeedVertical = false;
			}
		}
	}
	//БЕЗНОГNМ - простой случай с детекцией всего по брюху
	else
	{
		//если брюхо касается опоры
		if(CentralLimb.Stepped)
		{
			//рассчитать вязкость для ног, имитирующую торможение и трение покоя (почему только линейное?)
			me()->SetLimbDamping (CentralLimb, DampingForFeet, 0);

			//степень устойчивость зависит от крена брюха, но не сильно
			StandHardness += 1.0f * FMath::Clamp( FVector::DotProduct(CentralLimb.ImpactNormal, BAXIS(CentralBody,Up)), 0.0f, 1.0f);

			//используем "брюхо" для определения опоры и ее относительной скорости
			VelocityAgainstFloor = FMath::Lerp (
				VelocityAgainstFloor,
				CentralBody->GetUnrealWorldVelocity()
					- CentralLimb.Floor->GetUnrealWorldVelocityAtPoint ( CentralBody->GetCOMPosition() ),
				0.1f);

			//точное направление для движения - по опоре, которую касается брюхо (тру - это вкл. поддержку на ветке)
			GuidedMoveDir = me()->CalculateLimbGuideDir ( CentralLimb, MyrOwner()->MoveDirection, true);
			GuidedMoveDir.Normalize();

			//используем нормаль брюха для определения вектора направления (бок может быть накренён, посему нормализировать)
			Forward = FVector::CrossProduct(BAXIS(CentralBody,Right), CentralLimb.ImpactNormal);
			Forward.Normalize();

		}

		//если брюхо в воздухе, 
		else 
		{
			//плавно возвращать ее скорость к абсолютной (хотя здесь можно задействовать ветер)
			VelocityAgainstFloor = FMath::Lerp ( VelocityAgainstFloor, CentralBody->GetUnrealWorldVelocity(), DeltaTime );
			Forward = BAXIS(CentralBody,Forth);
			Crouch *= (1 - DeltaTime);
			StandHardness = 0;
			NeedVertical = false;
			NeedYawLock = false;
		}

	}

	//если требуется режим фиксации на поверхности
	if(NeedFixed)
	{
		//полный расчёт возможности зацепиться + выдача аргументов почему нельзя
		auto CR = CanGirdleCling();
		if ((int)CR > (int)EClung::Kept)
		{
			//запретить лазанье при аномальном результате кроме случая, когда
			//фикс задаётся режимом поведения для более устойчивого хода по произвольной поверхности
			if(MyrOwner()->bClimb || CR != EClung::NoClimbableSurface)
				NeedFixed = false;
		}
	}


	//если хоть одно ограничение было включено в предыдущий такт
	if (Vertical | NoTurnAround | FixedOnFloor)
	{
		//выдать степени натяжения констрейнта
		FVector CI_ForceLin, CI_ForceAng;
		CI->GetConstraintForce(CI_ForceLin, CI_ForceAng);
		LINELIMB(ELimbDebug::CentralConstrForceLin, CentralLimb, CI_ForceLin);
		LINELIMB(ELimbDebug::CentralConstrForceAng, CentralLimb, CI_ForceAng);
		//здесь надо при превышении порогов расслаблять
	}

	//////////////////////////////////////////////////////////////////////////////
	//сложное условие, запускающее пересчёт лимитов (смена режима + недоводка мягкого зажатия пределов)
	bool UpAngLim = (Vertical != NeedVertical) || (NoTurnAround != NeedYawLock) || (FixedOnFloor != NeedFixed);
	if (TransToVertical) UpAngLim = true;
	if (TransToYawLock) UpAngLim = true;
	if (UpAngLim)
	{
		if(FixedOnFloor != NeedFixed)
			me()->Log(CentralLimb, TEXT("NeedFixed change"), NeedFixed ? TEXT("+"): TEXT("-"));

		//переприсовить сразу все не думая
		Vertical = NeedVertical;
		NoTurnAround = NeedYawLock;
		FixedOnFloor = NeedFixed;

		//лимит падения на бок и вперед-назад - плавно подтягивать в случае закрепления (при вертикали)
		int VerAngLim = CI->ProfileInstance.ConeLimit.Swing1LimitDegrees;
		if (!Vertical) VerAngLim = 180;
		else VerAngLim = FMath::Max(VerAngLim - 5, 0);

		//лимит вращения по сторонам - плавно подтягивать в случае закрепления (при фиксации азимута)
		int HorAngLim = CI->ProfileInstance.TwistLimit.TwistLimitDegrees;
		if (!NoTurnAround) HorAngLim = 180;
		else HorAngLim = FMath::Max(HorAngLim - 5, 0);

		//закрепитель позиции при режиме прищпиленност
		bool LockLeftRight =	FixedOnFloor;						
		bool LockFrontBack =	FixedOnFloor && CurrentDynModel->Leading;
		bool LockUpDown =		FixedOnFloor && CurrentDynModel->Leading;
		int LimUpDown = (LockUpDown && MyrOwner()->MoveGain == 0) ? 0 : (FixedOnFloor ? 1 : -1);

		//применить сразу все лимиты
		me()->SetFreedom(CI,
			LimUpDown, LockFrontBack?0:-1, LockLeftRight?0:-1,
			VerAngLim, VerAngLim, HorAngLim);
	}

	//////////////////////////////////////////////////////////////////////////////
	//кинематическое перемещение якоря пояса в нужную позицию
	FVector L = CentralBody->GetUnrealWorldTransform().GetLocation();
	FVector F, U;

	//в режиме пришпиленности к опоре
	if(FixedOnFloor)
	{
		//если тело хочет и может двигаться (нажата кнопка)
		if(MyrOwner()->MoveGain > 0)
		{
			//движение за этот кадр
			FVector Offset = GuidedMoveDir * MyrOwner()->GetDesiredVelocity() * DeltaTime;

			//только для поясов с реальными ногами
			if(HasLegs)
			{
				//текущаяя вытяжка ног (минимальная: пусть 1 нога будет свободна)
				float CurH = FMath::Min(GetFeetLengthSq(false), GetFeetLengthSq(false));

				//отклонение от номинальной длины (берется чуть меньше, чтобы детектировать вытяжку)
				float errorH = CurH - FMath::Square(LimbLength * 0.8);

				//обнаружено значительное отклонение = скомпнесировать
				if (errorH > FMath::Square(LimbLength * 0.05))
				{	Offset -= CentralLimb.ImpactNormal * LimbLength * 0.02;
					UE_LOG(LogTemp, Log, TEXT("%s: Procede %s push into, length = %g, base = %g"),
						*GetOwner()->GetName(), *GetName(), FMath::Sqrt(CurH), LimbLength*0.8);

				}
				else if (errorH < -FMath::Square(LimbLength * 0.05))
				{	Offset += CentralLimb.ImpactNormal * LimbLength * 0.02;
					UE_LOG(LogTemp, Log, TEXT("%s: Procede %s pull out, length = %g, base = %g"),
						*GetOwner()->GetName(), *GetName(), FMath::Sqrt(CurH), LimbLength*0.8);
				}
			}

			//для ведомого пояса
			if (!CurrentDynModel->Leading)
			{
				//противоположный пояс
				auto AG = MyrOwner()->GetAntiGirdle(this);

				//радиус вектор прямой спины от этого до противоположного
				auto ToAG = AG->GetComponentLocation() - GetComponentLocation();

				//текущее отклонение растяжки спины
				float errorL = ToAG.SizeSquared() - FMath::Square(LimbLength);

				//отклонение растяжки спины если бы мы пошли по ранее расчитанному курсу для этого пояса
				float errorLplus = (ToAG+Offset).SizeSquared() - FMath::Square(LimbLength);

				//наверстывание отклонения
				if (errorLplus >  FMath::Square(MyrOwner()->SpineLength*0.05))
				{
					Offset -= ToAG.GetSafeNormal() * LimbLength * 0.01;
				}
				else if (errorLplus < -FMath::Square(MyrOwner()->SpineLength*0.05))
				{
					Offset += ToAG.GetSafeNormal() * LimbLength * 0.01;
				}

			}

			if((FrameCounter&3)==0) LINEWT(ELimbDebug::MainDirs, L, Offset, 1, 0.5);
			L += Offset;
		}

		//оси вращения = посчитанным предпочтительным для пояса (уже ортонормированы)
		F = GuidedMoveDir;
		U = CentralLimb.ImpactNormal;
	}
	//в режиме свободного перемещения в пространстве
	else
	{
		//оси вращения - по абсолютному азимуту
		F = MyrOwner()->MoveDirection;
		U = FVector::UpVector;

		// если не ортогонален - ортонормировать его
		if (MyrOwner()->BehaveCurrentData->bOrientIn3D)		
		{	F.Z = 0;
			F.Normalize();
		}
	}

	//отобразить базис курса в центре якоря пояса
	LINE(ELimbDebug::GirdleGuideDir, L, GuidedMoveDir * 50);

	//кинематически переместить якорь в нужное место с нужным углом повотора
	SetWorldTransform(FTransform(F, F ^ U, U, L));

	//////////////////////////////////////////////////////////////////////////////
	//сверху от туловище рисуем тестовые индикаторы
	if (LDBG(ELimbDebug::ConstrPri))
	{	auto Ct = CentralBody->GetCOMPosition() + CentralLimb.ImpactNormal * 15;
		auto Color = FColor(255, 0, 255, 10);
		float ang = CI->ProfileInstance.ConeLimit.Swing1LimitDegrees;
		float ang2 = CI->ProfileInstance.TwistLimit.TwistLimitDegrees;
		//DrawDebugString(GetWorld(), Ct, FString::SanitizeFloat(DampingForFeet), nullptr, FColor(255, 0, 0), 0.02, false, 1.0f);
		if (Vertical)			DrawDebugString(GetWorld(), Ct, TEXT("     V"), nullptr, FColor(ang, 255, 0), 0.02, false, 1.0f);
		if (NoTurnAround)		DrawDebugString(GetWorld(), Ct, TEXT("       Y"), nullptr, FColor(255, 0, ang2), 0.02, false, 1.0f);
		if (FixedOnFloor)		DrawDebugString(GetWorld(), Ct, TEXT("         F"), nullptr, FColor(255, 255, 255), 0.02, false, 1.0f);
	}
}

//==============================================================================================================
//физически подпрыгнуть этим поясом конечностей
//==============================================================================================================
void UMyrGirdle::PhyPrance(FVector HorDir, float HorVel, float UpVel)
{
	//центральная часть тела пояса конечностей
	auto& CLimb = LMB(Center);

	//насколько главное тело пояса лежит на боку - если сильно на боку, то прыгать трудно
	float SideLie = FMath::Abs(LAXIS(CLimb,Right) | CLimb.ImpactNormal);
	float UnFall = 1.0 - SideLie;

	//суммарное направление прыжка
	FVector Dir = HorDir * HorVel + FVector::UpVector * (UpVel);

	//резко sотключить пригнутие, разогнуть ноги во всех частях тела
	Crouch = 0.0f;

	//применить импульсы к ногам - если лежим, то слабые, чтобы встало сначала туловище
	me()->PhyJumpLimb(LMB(Left), Dir * UnFall);
	me()->PhyJumpLimb(LMB(Right), Dir * UnFall);
	me()->PhyJumpLimb(LMB(Tail), Dir * UnFall);

	//применить импульсы к туловищу	// ослабление, связанное с лежанием, не применяется, чтобы мочь встать, извиваясь
	me()->PhyJumpLimb(LMB(Spine), Dir);
	me()->PhyJumpLimb(CLimb, Dir);

	//явно, не дожидаясь ProcessLimb, обнулить жесткость стояния, чтобы в автомате состояний поведения не переклинивало
	StandHardness = 0;
}
