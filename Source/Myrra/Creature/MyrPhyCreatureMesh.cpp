// Fill out your copyright notice in the Description page of Project Settings.


#include "MyrPhyCreatureMesh.h"
#include "AssetStructures/MyrCreatureBehaveStateInfo.h"	// данные текущего состояния моторики
#include "Artefact/MyrArtefact.h"						// неживой предмет
#include "../MyrraGameInstance.h"						// ядро игры
#include "MyrPhyCreature.h"								// само существо
#include "MyrGirdle.h"									// новый пояс
#include "PhysicsEngine/PhysicsConstraintComponent.h"	// привязь на физдвижке
#include "AssetStructures/MyrCreatureGenePool.h"		// генофонд
#include "DrawDebugHelpers.h"							// рисовать отладочные линии
#include "PhysicsEngine/PhysicsConstraintTemplate.h"	// для разбора списка физ-связок в регдолле
#include "PhysicalMaterials/PhysicalMaterial.h"			// для выкорчевывания материала поверхности пола
#include "Engine/SkeletalMeshSocket.h"					

//к какому поясу конечностей принадлежит каждая из доступных частей тела - для N(0) поиска
uint8 UMyrPhyCreatureMesh::Section[(int)ELimb::NOLIMB] = {0,0,1,1,1,1,1,0,0,0};
ELimb UMyrPhyCreatureMesh::Parent[(int)ELimb::NOLIMB] = { ELimb::PELVIS, ELimb::PELVIS, ELimb::LUMBUS, ELimb::PECTUS, ELimb::THORAX, ELimb::THORAX, ELimb::THORAX, ELimb::PELVIS, ELimb::PELVIS, ELimb::PELVIS };
ELimb UMyrPhyCreatureMesh::DirectOpposite[(int)ELimb::NOLIMB] = { ELimb::THORAX, ELimb::PECTUS, ELimb::LUMBUS, ELimb::PELVIS, ELimb::TAIL, ELimb::R_ARM, ELimb::L_ARM, ELimb::R_LEG, ELimb::L_LEG, ELimb::HEAD };

//таблица доступа для кодов эмоциональных стимулов при повреждении определенной части тела
EEmoCause UMyrPhyCreatureMesh::FeelLimbDamaged[(int)ELimb::NOLIMB] = {
	EEmoCause::DamagedCorpus, EEmoCause::DamagedCorpus,EEmoCause::DamagedCorpus, EEmoCause::DamagedCorpus,
	EEmoCause::DamagedHead,
	EEmoCause::DamagedArm, EEmoCause::DamagedArm,
	EEmoCause::DamagedLeg, EEmoCause::DamagedLeg,
	EEmoCause::DamagedTail
};

//рисовалки отладжочных линий
#if WITH_EDITOR
#define LINE(C, A, AB) MyrOwner()->Line(C, A, AB)
#define LINEW(C, A, AB, W) MyrOwner()->Line(C, A, AB, W)
#define LINELIMB(C, L, AB) MyrOwner()->Line(C, GetMachineBody(L)->GetCOMPosition(), AB)
#define LINELIMBW(C, L, AB, W) MyrOwner()->Line(C, GetMachineBody(L)->GetCOMPosition(), AB, W)
#define LINELIMBWT(C, L, AB, W, T) MyrOwner()->Line(C, GetMachineBody(L)->GetCOMPosition(), AB, W, T)
#define LINELIMBWTC(C, L, AB, W, T, t) MyrOwner()->Linet(C, GetMachineBody(L)->GetCOMPosition(), AB, t, W, T)
#define LDBG(C) MyrOwner()->IsDebugged(C)
#else
#define LDBG(C) false
#define LINE(C, A, AB)
#define LINELIMB(C, L, AB)
#define LINELIMBW(C, L, AB, W)
#define LINELIMBWT(C, L, AB, W, T) 
#define LINEW(C, A, AB, W)
#endif


//==============================================================================================================
//привязь конечности для шага/цепляния. ВНИМАНИЕ, наличие genepool и правильного индекса здесь не проверяется!
//данные проверки нуждо делать до выпуска существа на уровень, а physics asset существа должен быть правильно укомплектован
//==============================================================================================================
FMachineLimbAnatomy*	UMyrPhyCreatureMesh::MachineGene				(ELimb Limb) const		{ return &MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Limb).Machine; }
FFleshLimbAnatomy*		UMyrPhyCreatureMesh::FleshGene					(ELimb Limb) const		{ return &MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Limb).Flesh; }
FBodyInstance*			UMyrPhyCreatureMesh::GetArchMachineBody			(ELimb Limb) const		{ return &MyrOwner()->GenePool->SkeletalMesh->GetPhysicsAsset()->SkeletalBodySetups[GetMachineBodyIndex(Limb)]->DefaultInstance; }
FConstraintInstance*	UMyrPhyCreatureMesh::GetArchMachineConstraint	(ELimb Limb) const		{ return &MyrOwner()->GenePool->SkeletalMesh->GetPhysicsAsset()->ConstraintSetup[GetMachineConstraintIndex(Limb)]->DefaultInstance; }
FConstraintInstance*	UMyrPhyCreatureMesh::GetArchConstraint			(int i) const			{ return &MyrOwner()->GenePool->SkeletalMesh->GetPhysicsAsset()->ConstraintSetup[i]->DefaultInstance; }
FLimbOrient&			UMyrPhyCreatureMesh::MBoneAxes					()			 const		{ return MyrOwner()->GenePool->MachineBonesAxes; }

FBodyInstance* UMyrPhyCreatureMesh::GetRootBody()
{	return Bodies[MyrOwner()->GetGenePool()->RootBodyIndex];}


//==============================================================================================================
//конструктор
//==============================================================================================================
UMyrPhyCreatureMesh::UMyrPhyCreatureMesh (const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//свойства всякие, хз
	AlwaysLoadOnClient = true;
	AlwaysLoadOnServer = true;
	bOwnerNoSee = false;
	VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	bAffectDynamicIndirectLighting = true;
	bVisibleInRealTimeSkyCaptures = false;
	SetCanEverAffectNavigation(false);
	DetailMode = EDetailMode::DM_Low;

	//это надо для примятия травы
	SetRenderCustomDepth(true);	

	//логично, что должен отбрасывать тень
	SetCastShadow(true);
	bCastDynamicShadow = true;


	//чтобы работало bHACK_DisableCollisionResponse, но насколько это нужно, пока не ясно
	IConsoleVariable *EnableDynamicPerBodyFilterHacks = IConsoleManager::Get().FindConsoleVariable(TEXT("p.EnableDynamicPerBodyFilterHacks"));
	EnableDynamicPerBodyFilterHacks->Set(true);

	//якобы это должно помочь от проблем с вылетом при горячей перезагрузке кода
	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	//тик (частый не нужен, но отрисовка висит на нем, если не каждый кадр, будет слайд-шоу)
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	PrimaryComponentTick.EndTickGroup = TG_PostPhysics;
	PrimaryComponentTick.bTickEvenWhenPaused = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.SetTickFunctionEnable(false);
	PrimaryComponentTick.TickInterval = 10000;	//за каким-то хером он тичит на паузе, чтобы этого не было, до начала большой интервал
	PrimaryComponentTick.bHighPriority = false;

	//чтобы вызывалась функция OverlapEnd, когда из зубов бросают
	SetGenerateOverlapEvents(true);

	//вот это, видимо, с тем, что для ориентации частей тела используются хиты, не нужно
	//bMultiBodyOverlap = true;

	//включить детекцию столкновений для всего меша
	SetCollisionProfileName(TEXT("PawnSupport"));
	SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetAllBodiesCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);

	//это то же, что флаг simulation generates hit events
	SetNotifyRigidBodyCollision(true);
	SetAllBodiesNotifyRigidBodyCollision(true);

	//принципиальная строчка! позволяет физике самой подстраиваться под анимацию
	bUpdateJointsFromAnimation = true;

	//самая первчная, логическая инициализация частей тела
	InitLimb (Pelvis,	ELimb::PELVIS);
	InitLimb (Lumbus,	ELimb::LUMBUS);
	InitLimb (Pectus,	ELimb::PECTUS);
	InitLimb (Thorax,	ELimb::THORAX);
	InitLimb (Head,		ELimb::HEAD);
	InitLimb (LArm,		ELimb::L_ARM);
	InitLimb (RArm,		ELimb::R_ARM);
	InitLimb (LLeg,		ELimb::L_LEG);
	InitLimb (RLeg,		ELimb::R_LEG);
	InitLimb (Tail,		ELimb::TAIL);

	//подвязка обработчиков столкновений
	OnComponentHit.AddDynamic(this, &UMyrPhyCreatureMesh::OnHit);

	//выход из объёма
	OnComponentEndOverlap.AddDynamic(this, &UMyrPhyCreatureMesh::OnOverlapEnd);

	//центр обработки странных физических воздействий
	OnCalculateCustomPhysics.BindUObject(this, &UMyrPhyCreatureMesh::CustomPhysics);

}

//==============================================================================================================
// при появлении в игре (часть вещей хорошо бы установить раньше, но неясно, на каком этапе компонент столь же
// всеобъемлюще настроен, как BeginPlay, практика показывает, что многое внутри выделяется в последний момент)
//==============================================================================================================
void UMyrPhyCreatureMesh::BeginPlay()
{
	//тиак чтоб только сейчас запустился каждый кадр, а до этого в паузе не тикал
	if (!IsTemplate())
	{	PrimaryComponentTick.TickInterval = 0;
		SetComponentTickEnabled(true);
	}
	else
	{	PrimaryComponentTick.TickInterval = 1000;
		SetComponentTickEnabled(false);
	}

	//ЭТО МАРАЗМ! если это вызвать раньше, веса физики у тел будут нулевыми
	SetSimulatePhysics(true);

	//ЭТО МАРАЗМ! Если ввести труъ, то наоборот, веса учитываться не будут, и физика всегда будет на полную
	SetEnablePhysicsBlending(false);

	//установить колизию с окружением
	SetPhyBodiesBumpable (true);
	Super::BeginPlay();
}

//==============================================================================================================
//каждый кадр
//==============================================================================================================
void UMyrPhyCreatureMesh::TickComponent (float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	//хз зачем
	//this->UpdateRBJointMotors();

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//проверки для отсечения всяких редакторных версий объекта, чтоб не тикало
	if (TickType == LEVELTICK_ViewportsOnly) return; //только это помогает
	if (Bodies.Num() == 0) return;

	//обработать всяческую жизнь внутри отдельных частей тела (их фиксированное число)
	for (int i = 0; i < (int)ELimb::NOLIMB; i++)
		ProcessLimb (GetLimb((ELimb)i), DeltaTime);

	//рисануть основные траектории
	LINELIMB(ELimbDebug::MainDirs, Head, (FVector)MyrOwner()->MoveDirection * 100);
	LINELIMB(ELimbDebug::MainDirs, Head, (FVector)MyrOwner()->AttackDirection * 100);
	LINELIMBWTC(ELimbDebug::IntegralBumpedNormal, Head, (FVector)IntegralBumpNormal * 100, 1, 0.04, Bumper1().GetBumpCoaxis());
	LINELIMBWTC(ELimbDebug::LimbStepped, Head, (FVector)FVector(0), (Head.WhatAmI == MaxBumpedLimb1 ? 4 : 2), 0.02, Head.GetBumpCoaxis());
	LINELIMBWTC(ELimbDebug::LimbStepped, Lumbus, (FVector)FVector(0), (Lumbus.WhatAmI == MaxBumpedLimb1 ? 4 : 2), 0.02, Lumbus.GetBumpCoaxis());
	LINELIMBWTC(ELimbDebug::LimbStepped, Pectus, (FVector)FVector(0), (Head.WhatAmI == MaxBumpedLimb1 ? 4 : 2), 0.02, Pectus.GetBumpCoaxis());
	LINELIMBWTC(ELimbDebug::LimbStepped, Tail, (FVector)FVector(0), (Head.WhatAmI == MaxBumpedLimb1 ? 4 : 2), 0.02, Tail.GetBumpCoaxis());



}

//==============================================================================================================
//удар твердлого тела
//==============================================================================================================
void UMyrPhyCreatureMesh::OnHit (UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//ЭТО МАРАЗМ, он постоянно реагирует сам на себя
	if (HitComp != OtherComp)
	{
		//возможно, следует упростить путь к доступу от имени кости до индекса сегмента тела
		FBODY_BIO_DATA bbd = MyrOwner()->GetGenePool()->BodyBioData[GetBodyInstance(Hit.MyBoneName)->InstanceBodyIndex];
		HitLimb (GetLimb(bbd.eLimb), bbd.DistFromLeaf, Hit, NormalImpulse);
	}
}

//==============================================================================================================
//отследивание конца пересечения между добычей в зубах и хватателем
//==============================================================================================================
void UMyrPhyCreatureMesh::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//если зафиксирован момент прощания с другим таким же существом - видимо, нас отпустили из зубов
	if (OverlappedComp->IsA<UMyrPhyCreatureMesh>())
	{
		//получить список акторов класса нашего, пересекающих данный объём
		TSet<AActor*> TheOverlapped; OverlappedComp->GetOverlappingActors(TheOverlapped, this->StaticClass());
		if (TheOverlapped.Num() <= 1)
		{
			//почему правильно 1? себя ещё считает? в любом случае
			if (!IsBumpable())
				SetPhyBodiesBumpable(true);
		}
	}
	//UE_LOG(LogTemp, Error, TEXT("%s: OnOverlapEnd %s->%s"), *GetOwner()->GetName(), *OtherComp->GetOwner()->GetName(), *OtherComp->GetName());
}
//==============================================================================================================
//приложение сил к каждому отдельному физ-членику
//==============================================================================================================
void UMyrPhyCreatureMesh::CustomPhysics(float DeltaTme, FBodyInstance* BodyInst)
{
	auto Type = MyrOwner()->GetGenePool()->BodyBioData[BodyInst->InstanceBodyIndex].eLimb;
	auto& Limb = GetLimb(Type);
	auto Girdle = MyrOwner()->GetGirdle(Type);

	//общая инфа по конечности для данного вида существ
	const auto LimbGene = MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Type).Machine;

	//применить поддерживающую силу (вопрос правильной амплитуды по-прежнему стоит)
	float Power = Girdle->CurrentDynModel->ForcesMultiplier;
	if (Limb.DynModel & LDY_EXTGAIN) Power *= MyrOwner()->ExternalGain; else
	if (Limb.DynModel & LDY_NOEXTGAIN) Power *= (1 - MyrOwner()->ExternalGain);

	//ограничить до стоящих на земле или наоборот
	if ((Limb.DynModel & LDY_ONGROUND)) Power *= (Girdle->StandHardness / 255.0f);
	if ((Limb.DynModel & LDY_NOTONGROUND)) Power *= 1 - (Girdle->StandHardness / 255.0f);

	//по битам флагов сложить внешнюю ось для ориентации
	FVector3f OuterAxis = CalcOuterAxisForLimbDynModel(Limb);

	//если вняя ось по флагам определена, то совершить подтяжку локальной оси к внешней
	if (OuterAxis != FVector3f::ZeroVector)
		OrientByForce(BodyInst, Limb, CalcInnerAxisForLimbDynModel(Limb, OuterAxis), OuterAxis, Power);

	//если указана ось локальная, но не указана ось глобальная - особый режим (кручение вокруг себя)
	else if ((Limb.DynModel & LDY_MY_AXIS) && !(Limb.DynModel & LDY_TO_AXIS))
	{
		FVector3f DesiredAxis = CalcInnerAxisForLimbDynModel(Limb, FVector3f(0)) * Power * Limb.OuterAxisPolarity();
		if (Limb.DynModel & LDY_ROTATE)
		{	BodyInst->AddTorqueInRadians((FVector)DesiredAxis, false, true);
			LINELIMB(ELimbDebug::LimbTorques, Limb, (FVector)DesiredAxis * 50);
		}
		else
		{	BodyInst->AddForce((FVector)DesiredAxis, false, true);
			LINELIMB(ELimbDebug::LimbForces, Limb, (FVector)DesiredAxis * 50);
		}
	}

	//попытка насильственно ограничить превышение скорости как лекарство от закидонов физдвижка
	if (BodyInst->GetUnrealWorldVelocity().SizeSquared() >= FMath::Square(MyrOwner()->BehaveCurrentData->EmergencyMaxVelocity))
	{	float VelNorm; FVector VelDir;
		BodyInst->GetUnrealWorldVelocity().ToDirectionAndLength(VelDir, VelNorm);
		VelDir *= MyrOwner()->BehaveCurrentData->EmergencyMaxVelocity;
		BodyInst->SetLinearVelocity(VelDir, false);
		UE_LOG(LogTemp, Error, TEXT("%s: %s Max Velocity %g Off Limit %g"), *GetOwner()->GetName(), *TXTENUM(ELimb, Limb.WhatAmI), VelNorm, MyrOwner()->BehaveCurrentData->EmergencyMaxVelocity);
	}
}

//==============================================================================================================
// получить по битовому полю направление собственной оси, которое недо повернуть
//==============================================================================================================
FVector3f UMyrPhyCreatureMesh::CalcInnerAxisForLimbDynModel(const FLimb& Limb, FVector3f GloAxis) const
{
	//значение по умолчанию (для случаев колеса) - чтобы при любой идеальной оси разница идеальность была нулевой и сила лилась постоянно
	//то есть если пользователь не указан ваще никакую "мою ось", глобальный вектор ориентации всё равно применялся бы, но по полной, без обратной связи
	FVector3f MyAxis = FVector3f(0,0,0);

	//если просят ориентироваться собственной осью вперед
	if (Limb.DynModel & LDY_MY_FRONT)   MyAxis = GetLimbAxisForth(Limb.WhatAmI); else

	//если просят ориентироваться собственной осью вверх
	if (Limb.DynModel & LDY_MY_UP)  	MyAxis = GetLimbAxisUp(Limb.WhatAmI); else

	//если надо брать ось в сторону - выбирается наиболее близкая к целевой оси
	if (Limb.DynModel & LDY_MY_LATERAL)
	{ 	MyAxis = GetLimbAxisRight(Limb.WhatAmI);
		if((MyAxis | GloAxis) < 0)
			MyAxis = -MyAxis;
	}
	if (Limb.DynModel & LDY_REVERSE_MY) MyAxis = -MyAxis;
	return MyAxis;
}

//==============================================================================================================
// получить по битовому полю направление внешней оси, на которую надо равняться
//==============================================================================================================
FVector3f UMyrPhyCreatureMesh::CalcOuterAxisForLimbDynModel(const FLimb& Limb) const
{
	float Revertor = Limb.OuterAxisPolarity();
	auto G = MyrOwner()->GetGirdle(Limb.WhatAmI);
	if (Limb.DynModel & LDY_TO_COURSE)		return G->GuidedMoveDir * Revertor;
	if (Limb.DynModel & LDY_TO_VERTICAL)	return FVector3f(0, 0, Revertor);
	if (Limb.DynModel & LDY_TO_ATTACK)		return MyrOwner()->AttackDirection * Revertor;
	if (Limb.DynModel & LDY_TO_NORMAL)		return (G->IsThorax?Thorax:Pelvis).Floor.Normal;
	return FVector3f(0,0,0);
}


//==============================================================================================================
//ориентировать тело силами, поворачивая или утягивая
//==============================================================================================================
void UMyrPhyCreatureMesh::OrientByForce (FBodyInstance* BodyInst, FLimb& Limb, FVector3f ActualAxis, FVector3f DesiredAxis, float AdvMult)
{
	//степень отклонения от правильной позиции = сила воздействия, стремящегося исправить отклонение (но еще не знак)
	//если актуальная ось = 0, то и эта переменная 0, то есть идеал недостижим и сила будет применяться всегда
	const float AsIdeal = ActualAxis | DesiredAxis;

	//если требуется делать это кручением тела
	if(Limb.DynModel & LDY_ROTATE)
	{
		//выворот по оси-векторному произведению, если ошибка больше 90 градусов, амплитуда константно-максимальная
		if (AsIdeal > 0.99) return;

		//векторное произведение
		auto RollAxis = ActualAxis ^ DesiredAxis; 
		if (AsIdeal < 0)	RollAxis.Normalize();
		FVector3f Final = RollAxis * AdvMult * (1 - AsIdeal);
		if (Final.ContainsNaN())
			Log(Limb, TEXT("WTF OrientByForce contains nan"));
		else
		{	BodyInst->AddTorqueInRadians((FVector)Final, false, true);
			LINELIMB(ELimbDebug::LimbTorques, Limb, (FVector)ActualAxis * 40);
			LINELIMB(ELimbDebug::LimbTorques, Limb, (FVector)DesiredAxis * 50);
		}
	}
	//если требуется просто тянуть (это само по себе не ориентирование, но если есть привязи, они помогут)
	if (Limb.DynModel & LDY_PULL)
	{
		//модуль силы тащбы
		auto Final = DesiredAxis * (1 - AsIdeal) * AdvMult;

		//если тело уже уткнуто, заранее обрезать силу
		//хз вообще что это и зачем
		if(Limb.Stepped)
			Final *= 1.0f - FMath::Max(0.0f, (-Limb.Floor.Normal) | DesiredAxis);

		//применить прямую силу по нужной оси
		if (Final.ContainsNaN()) Log(Limb, TEXT("WTF OrientByForce contains nan"));
		else
		{	BodyInst->AddForce((FVector)Final, false, true);
			LINELIMB(ELimbDebug::LimbForces, Limb, (FVector)DesiredAxis * 0.3 * Limb.Stepped);
		}
	}
}


//==============================================================================================================
//текущее положение плеча
//==============================================================================================================
FVector UMyrPhyCreatureMesh::GetLimbShoulderHubPosition(ELimb eL)
{
	//сначала попытаться взять имя той кости, которая явно указана в генофонде как позиция для отсчёта этой конечности
	FName AxisName = MachineGene(eL)->AxisReferenceBone;

	//если таковая не указана, то взять (?) позицию физического членика
	if (AxisName.IsNone())
	{	if (HasPhy(eL))
			return GetMachineBody(eL)->GetCOMPosition();
		else GetMachineBody(ELimb::PELVIS)->GetCOMPosition();
	}

	//тут нужно еще проверять, существует ли вписанная кость
	else return GetBoneLocation(AxisName);

	//по умолчанию
	return FVector(0);
}


//==============================================================================================================
//прыжковый импульс для одной части тела
//==============================================================================================================
void UMyrPhyCreatureMesh::PhyJumpLimb(FLimb& Limb, FVector3f Dir)
{
	//сбросить индикатор соприкоснутости с опорой
	Limb.EraseFloor();

	//если в этой части тела вообще есть физ-тело каркаса
	if(HasPhy(Limb.WhatAmI))
	{	
		//тело
		auto BI = GetMachineBody(Limb);

		//отменить линейное трение, чтобы оно в воздухе летело, а не плыло как в каше
		BI->LinearDamping = 0;
		BI->UpdateDampingProperties();

		//применить скорость
		BI->AddImpulse((FVector)Dir, true);
		LINELIMBWT(ELimbDebug::LimbForces, Limb, (FVector)Dir * 0.1, 1, 0.5);

		//если есть констрейнт (у корневого тела его видимо нет)
		if (GetMachineConstraintIndex(Limb) != 255)
		{
			//распознаём, что это конечность по наличию линейного мотора - и вытягиваем конечность на полную
			auto CI = GetMachineConstraint(Limb);
			if (CI->ProfileInstance.LinearDrive.ZDrive.bEnablePositionDrive)
				CI->SetLinearPositionTarget(FVector(0, 0, CI->ProfileInstance.LinearLimit.Limit));
		}
	}
}



//==============================================================================================================
//в ответ на касание членика этой части тела - к сожалению, FBodyInstance сюда напрямую не передаётся
//==============================================================================================================
void UMyrPhyCreatureMesh::HitLimb (FLimb& Limb, uint8 DistFromLeaf, const FHitResult& Hit, FVector NormalImpulse)
{
	// 1 ========================== ФИКСАЦИЯ КОНТАКТА ================================
	//################################################################################

	//пока жесткая заглушка - если этот членик несёт физически кого-то, то он не принимает сигналы о касаниях
	//это чтобы сохранить неизменным значение Floor, в котором всё время ношения ссылка на несомый объект
	if (Limb.Grabs) return;
	
	//общая инфа по конечности для данного вида существ
	const auto LimbGene = MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Limb.WhatAmI).Machine;

	//если опора скелетная (дерево, другая туша) определить также номер физ-членика опоры
	FFloor NewFloor(Hit);

	//принятие опоры, возможно, здесь надо более сложный алгоритм выбора или отклонения
	if (NewFloor) Limb.Floor = NewFloor; else return;

	//вектор относителой скорости
	FVector3f RelativeVel = BodySpeed(Limb.WhatAmI);
	if (Limb.Floor.IsMovable()) RelativeVel -= Limb.Floor.Speed(Hit.ImpactPoint);

	//уточнение модуля скорости
	float Speed = 0; RelativeVel.ToDirectionAndLength(RelativeVel, Speed);

	//если скорость маленькая для точности направления берем из пояса курс
	if (Speed < 1.5) RelativeVel = GetLimbAxis(Limb.WhatAmI, EMyrRelAxis::Front);

	//сонаправленность движения и нормали
	float Coaxis = RelativeVel | (FVector3f)(-Hit.ImpactNormal);

	//коэффициент скользящести последнего удара
	Limb.Colinea = 255 * FMath::Max(Coaxis+0.05, 0.0f);

	//воспроизвести звук, если контакт от падения (не дребезг и не качение) и если скорость соударения... откуда эта цифра?
	//в вертикальность ноль так как звук цепляния здесь не нужен, только тупое соударение
	if (Speed > 10 && Limb.Stepped == 0)
		MyrOwner()->SoundOfImpact (nullptr, Limb.Floor.Surface, Hit.ImpactPoint, Speed, 0, EBodyImpact::Bump);

	//все что ниже относится только к живым объектам
	if(MyrOwner()->Health <= 0) return;

	//пометить, что есть касание
	Limb.Stepped = STEPPED_MAX;

	// 2 ========================== ПРИНЯТИЕ УРОНА ОТ УДАРА ================================

	//рассчитать опасность удара
	float Severity = ShockResult(Limb, Speed * FMath::Max(Coaxis, 0.4f), Limb.Floor);
	if (Severity > 0.0f)
		MyrOwner()->Hurt(Limb, Severity, Hit.ImpactPoint, (FVector3f)Hit.ImpactNormal, Limb.Floor.Surface);

	// 3 ========================== ПРИНЯТИЕ УРОНА ОТ АТАКИ ================================

	//если мы атакуем именно этой частью тела
	if (MyrOwner()->IsLimbAttacking(Limb.WhatAmI))
	{
		//часть тела хватаемого существа, буде таковое есть
		FLimb* TheirLimb = nullptr;
		FBodyInstance* TheirBody = Hit.Component->GetBodyInstance(Hit.BoneName);

		//если мы ударили другое живое существо
		auto TheirMesh = Cast<UMyrPhyCreatureMesh>(Hit.Component.Get());
		if (TheirMesh)
		{
			//найти из физ-членика часть тела противника
			FBODY_BIO_DATA bbd = TheirMesh->MyrOwner()->GetGenePool()->BodyBioData[TheirBody->InstanceBodyIndex];
			TheirLimb = &TheirMesh->GetLimb(bbd.eLimb);

			//базис урона из настроек атаки - безразмерный, 1 = типичная атака для любого существа, меньше может быть
			const auto DamageBase = MyrOwner()->GetAttackActionVictim().TactileDamage;

			//нанесение урона только если наша атака увечная
			if (DamageBase > 0)
			{
				//если жертва жива и если повреждаемый членик только что не был поврежден (для предотвращения дребезга)
				if (TheirMesh->MyrOwner()->Health > 0 && !TheirLimb->LastlyHurt)
				{
					//набираем факторов, влияющих на увечье
					float dD = DamageBase * MyrOwner()->Phenotype.AttackDamageFactor();

					//также если жертва сама атакует, ее атака может давать дополнительный щит
					if (TheirMesh->MyrOwner()->Attacking())
						dD -= TheirMesh->MyrOwner()->GetAttackAction()->HitShield * TheirMesh->MyrOwner()->Phenotype.AttackShieldFactor();

					//если его контратака не полностью нивеллировала нашу
					if (dD >= 0)
					{
						//вступает в действие внутренняя сопротивляемость разных частей тела
						//но нахрена она нужна, если не прокачивается и если всё равно каждое увечье - по месту
						//лучьше внести разные степени восстанаовления разных частей тела
						dD *= TheirMesh->MachineGene(TheirLimb->WhatAmI)->HitShield;

						//пострадать от удара физически и ментально в ИИ
						TheirMesh->MyrOwner()->SufferFromEnemy(dD, MyrOwner());									
						TheirMesh->MyrOwner()->Hurt(*TheirLimb, dD, Hit.ImpactPoint, (FVector3f)Hit.ImpactNormal, TheirLimb->Floor.Surface);	
					}
				}
				//жертва мертва - пинание трупа
				else
				{
				}
			}
			//атака исцеляющая или нейтральная
			else
			{
				//пусть это существо оценит наши отношения и решит само, как реагировать на такую милость
				TheirMesh->MyrOwner()->EnjoyAGrace(DamageBase, MyrOwner());
			}
		}
		//если ударили не живое существо, а предмет
		else { }

		// 3 ========================== ВОЗМОЖНОСТЬ СХВАТИТЬ ПРЕДМЕТ ================================
		// 
		//если наша атака - это хватание и сейчас как раз самая фаза
		if (MyrOwner()->NowPhaseToGrab() && Hit.Component->Mobility == EComponentMobility::Movable)
		{
			//внимание, масса может сильно меняться через heavy legs и mass scale, так что у живых стоячих на ногах "масса" будет почти втрое больше
			//это нормально, так как эмулирует цепляние и прочее сопротивление, поэтому коэффициент снижен до 0.1
			float m1 = GetMass();
			float m2 = Hit.Component->GetMass();
			if (m2 < 0.1 * m1)
				Grab(Limb, DistFromLeaf, TheirBody, TheirLimb, Hit.ImpactPoint);
			else Log(Limb, TEXT("UnableToGrab so heave, mass ="), m2);

		}
		//если не хватание
		else
		{
			//прервать атаку, чтоб не распидорасить физдвижок от проникновения кинематической лапы в тело
			if (TheirMesh) MyrOwner()->AttackGoBack();
		}
	}
	//если мы точно не атакуем (возможно, мы жертва)
	else
	{
		//восстановление только что отпущенного из зубов "прозрачного" тела в "непрозрачность"
		if (!Cast<UMyrPhyCreatureMesh>(Hit.Component.Get()))		//если ударились об неживой предмет
			if (MyrOwner()->CurrentState != EBehaveState::held)		//если ударились не на весу в зубах
				if (!IsBumpable())									//если мы проходим сквозь стены 
					SetPhyBodiesBumpable(true);						//срочно обрести плотность, пока не провалились
	}

}

//==============================================================================================================
//удар об твердь - насколько опасно
//==============================================================================================================
float UMyrPhyCreatureMesh::ShockResult(FLimb& L, float Speed, FFloor& NewFloor)
{
	//порог для отсечения слабых толчков (без звука и эффекта) и определения болезненных (>1, с уроном)
	float Thresh = MyrOwner()->GetGenePool()->MaxSafeShock;		// базис порога небольности в единицах скорости 
	Thresh *= MyrOwner()->Phenotype.BumpShockResistance();		// безразмерный коэффициент индивидуальной ролевой прокачки
	Thresh *= MachineGene(L.WhatAmI)->HitShield;				// безразмерный коэффициент дополнительной защиты именно этой части 

	//выдрать данные о поверхности непосредственно из сборки Hit, оно нужно далее
	auto SurfInfo = MyrOwner()->GetMyrGameInst()->Surfaces.Find(NewFloor.Surface);

	//для подвижных объектов учитывается скорость опоры, но удар  слабее ,чтобы исключить увечья глюками физики
	if (NewFloor.IsMovable())
	{	Thresh *= 10;
		Speed += NewFloor.Speed().Z;
	}

	//меньше десятой части порога - вообще никак не влияет
	float Severity = Speed < 0.1 * Thresh ? 0.0f : FMath::Max(0.0f, Speed / Thresh - 1);
	if (SurfInfo) Severity += SurfInfo->HealthDamage;
	return Severity;
}


//==============================================================================================================
//применить вес физики (на данный момент нужно только чтобы хвост огибал препятствия)
//==============================================================================================================
void UMyrPhyCreatureMesh::PhyDeflect (FLimb& Limb, float Weight)
{
	if (Weight < 0) Weight = 0;
	if (Weight > 1) Weight = 1;
	for(auto ind : FleshGene(Limb.WhatAmI)->BodyIndex)
		Bodies[ind]->PhysicsBlendWeight = Weight;
}

//==============================================================================================================
//обновить веса физики и увечья для этой части тела
//==============================================================================================================
void UMyrPhyCreatureMesh::ProcessBodyWeights (FLimb& Limb, float DeltaTime)
{
	//квант восстановления (сюда будет ещё домножена скорость исцеления и влияние навыков)
	float Dimin = DeltaTime;

	//порог - если эта часть тела полностью симулируемая, нет нужды менять ее веса
	//+ увечная конечность должна оставаться с физикой
	const auto Bottom = FleshGene(Limb.WhatAmI)->MinPhysicsBlendWeight + Limb.Damage;
	if (Bottom >= 1.0f) return;
	
	//для каждого доступного мясного членика сокращаем его вес физики, пока не достигнуто дно
	for(auto ind : FleshGene(Limb.WhatAmI)->BodyIndex)
	{	auto& W = Bodies[ind]->PhysicsBlendWeight;
		W -= Dimin;	if (W < Bottom) W = Bottom;
	}
}

//==============================================================================================================
//дополнительная инициализация части тела
//==============================================================================================================
void UMyrPhyCreatureMesh::InitLimbCollision (FLimb& Limb, float MaxAngularVeloc, FName CollisionPreset)
{
	//если присутствует элемент каркаса-поддержки
	auto bi = GetMachineBodyIndex(Limb);
	if (bi < 255)
	{	
		//максимальная угловая скорость, или из генофонда, или, если там ноль - по умолчанию
		auto ge = MachineGene(Limb.WhatAmI);
		Bodies[bi]->SetMaxAngularVelocityInRadians(ge->MaxAngularVelocity>0 ? ge->MaxAngularVelocity : MaxAngularVeloc, false);
	}
}


//==============================================================================================================
//основная покадровая рутина в отношении части тела
//==============================================================================================================
void UMyrPhyCreatureMesh::ProcessLimb (FLimb& Limb, float DeltaTime)
{
	//общая инфа по конечности для данного вида существ
	const auto& LimbGene = *MachineGene(Limb.WhatAmI);

	//сбросить флаг больности, допускающий лишь один удар по членику за один кадр
	Limb.LastlyHurt = false;

	//если с этой частью тела ассоциирован элемент каркаса/поддержки
	if (LimbGene.TipBodyIndex != 255)
	{
		//если включена физика (для кинематического режима это все скипнуть)
		auto BI = GetMachineBody(Limb);
		if (BI->bSimulatePhysics && BI->bNotifyRigidBodyCollision)
		{
			//запускаем на этот кадр доп-физику исключительно для членика-поддержки
			//специально до обработки счетчиков и до ее избегания, чтобы держащий ношу членик тоже обрабатывался
			BI->AddCustomPhysics(OnCalculateCustomPhysics);

			//нужно только для хвоста
			if (HasFlesh(Limb.WhatAmI))
			{
				BI = GetFleshBody(Limb, 0);
				if (BI->BodySetup->PhysicsType != EPhysicsType::PhysType_Kinematic)
					BI->AddCustomPhysics(OnCalculateCustomPhysics);
			}

			//пока жесткая заглушка - если этот членик несёт физически кого-то, то он не принимает сигналы о касаниях
			//это чтобы сохранить неизменным значение Floor, в котором всё время ношения ссылка на несомый объект
			if (Limb.Grabs) return;

			//идеальный образ настроек этого тела из ассета
			auto ABI = GetArchMachineBody(Limb);

			//если ранее быз щафиксирован шаг/касание к этой части тела
			//иначе данная куча фигни не нужна
			if (Limb.Stepped)
			{

				//особый случай, выглядит как ошибка, но такое может быть при съеденной добыче
				if (!Limb.Floor) { Limb.EraseFloor(); return; }

				//относительная скорость нужна здесь только для детекции потери опоры
				FVector3f RelativeVelocity = (FVector3f)GetMachineBody(Limb)->GetUnrealWorldVelocity();

				//относительную скорость вычисляем только при контакте с мобильным предметом
				if (Limb.Floor.IsMovable())
					RelativeVelocity -= Limb.Floor.Speed(GetMachineBody(Limb)->GetCOMPosition());

				//вектор сдвига в пространстве
				FVector3f Shift = RelativeVelocity * DeltaTime;
				float Drift = Shift.SizeSquared();

				//плавное снижение желания отдернуться от препятствия (зависит от скорости - стоит ли извлекать корень или оставить бешенные нули?)
				//возможно, брать только проекцию скорости на нормаль
				int NewStepped = Limb.Stepped;
				if (Drift > 100) { NewStepped -= 3; }
				else
					if (Drift > 10) { NewStepped -= 2; }
					else
						NewStepped -= 1;

				//взять данные о поверхности опоры непосредственно из стандартной сборки хитрезалт
				if (NewStepped <= 0) Limb.EraseFloor();
				else Limb.Stepped = NewStepped;
			}
		}

		//постепенное исцеление от урона (за счёт сокращения здоровья)
		if (Limb.Damage > 0)
		{
			Limb.Damage = Limb.Damage - DeltaTime * HealRate;
			if (Limb.Damage <= 0.0f) Limb.Damage = 0.0f;
			else MyrOwner()->Health -= DeltaTime * HealRate;
		}
		LINELIMB(ELimbDebug::LimbAxisX, Limb, (FVector)GetLimbAxisForth(Limb) * 5);
		LINELIMB(ELimbDebug::LimbAxisY, Limb, (FVector)GetLimbAxisRight(Limb) * 5);
		LINELIMB(ELimbDebug::LimbAxisZ, Limb, (FVector)GetLimbAxisUp(Limb) * 5);
		LINELIMB(ELimbDebug::LimbNormals, Limb, (FVector)Limb.Floor.Normal * 15);

	}

	//для бесплотных члеников, как виртуальные ноги, скорость сброса опоры значительно больше
	else Limb.Stepped = Limb.Stepped >> 1;
}

//==============================================================================================================
//взять данный предмет за данный членик данной частью тела
//здесь не проверяется, может ли часть тела взять
//==============================================================================================================
void UMyrPhyCreatureMesh::Grab(FLimb& Limb, uint8 ExactBody, FBodyInstance* GrabbedBody, FLimb* GrabbedLimb, FVector At)
{
	//поймано столкновение с уже хсваченным
	if (Limb.Grabs) return;

	//идеальные оси для правильно ориентации добычи во рту
	FVector3f aF(1, 0, 0);
	FVector3f aR(0, 1, 0);

	//если эта функция вызывана с живым параметром грёбаной части тела, значит мы гребем априори другое существо
	if (GrabbedLimb)
	{
		//взять компонент как меш без дополнительных проверок
		auto CreaM = (UMyrPhyCreatureMesh*)GrabbedBody->OwnerComponent.Get();

		//зделать нас как ношу нашего противника прозрачным для столконвений с ним, чтобы не колбасило физдвижок
		aF = MyrAxes[(int)MBoneAxes().Forth];
		aR = MyrAxes[(int)MBoneAxes().Right];

		//если в генофонде сказано, что за это часть тела нельзя хватать, например за хвост очень неркрасиво смотрится
		//меняем часть тела по умолчанию на таз - он есть у всех и он массивный, будет хорошо самотреться
		if (!CreaM->FleshGene(GrabbedLimb->WhatAmI)->CanBeGrabbed)
			GrabbedBody = CreaM->GetMachineBody(ELimb::PELVIS);

		//насильно инициализировать для нас, схваченных, состояние "схваченный"
		CreaM->MyrOwner()->AdoptBehaveState(EBehaveState::held);
		CreaM->SetPhyBodiesBumpable(false);
	}
	//не живое существо, а артефакт
	else
	{		
		//аналог SetPhyBodiesBumpable, чтоб пересекалось с головой
		GrabbedBody->SetCollisionProfileName(TEXT("PawnTransparent"));
	}

	//внимание, тут вывих мозга - как стандартный контейнер для привязи берётся только КАРКАСНЫЙ физчленик этой части тела
	auto HostBody = GetMachineBody(Limb);
	auto& CI = HostBody->DOFConstraint;

	//актуальный членик, который используется как тело-операнд
	auto CarrierBody = ExactBody==255 ? GetMachineBody(Limb) : GetFleshBody(Limb,ExactBody);
	FVector Pri = FVector::UpVector;
	FVector Sec = FVector::RightVector;

	//точное место, к которому следует закрепить поднятый предмет
	auto AtNa = FleshGene(Limb.WhatAmI)->GrabSocket;
	if (!AtNa.IsNone())
	{	At = GetSocketLocation(FleshGene(Limb.WhatAmI)->GrabSocket);
		Pri = GetSocketTransform(FleshGene(Limb.WhatAmI)->GrabSocket).GetUnitAxis(EAxis::X);
		Sec = GetSocketTransform(FleshGene(Limb.WhatAmI)->GrabSocket).GetUnitAxis(EAxis::Y);
	}

	UE_LOG(LogTemp, Warning, TEXT("%s Grab Limb %s by Limb %s %s"), *GetOwner()->GetName(),
		GrabbedLimb ? *TXTENUM(ELimb, GrabbedLimb->WhatAmI) : TEXT("---"),
		*TXTENUM(ELimb, Limb.WhatAmI),
		(!AtNa.IsNone()) ? TEXT("At Socket Place") : TEXT("At Hit Place"));


	//если констрейнт еще не создан в памяти
	if(!CI)	CI = PreInit();

	//абсолютно жёсткий констрейнт
	SetFreedom(CI, 0, 0, 0, 0, 0, 0);
	CI->EnableParentDominates();
	CI->SetDisableCollision(true);
	
	//для хватателя берутся реальные относительные оси, для хватаемого - идеальные, чтобо он лёг ровно на зубах
	CI->Pos1 = FVector(0); CI->PriAxis1 = (FVector)aF; CI->SecAxis1 = (FVector)aR;
	BodyToFrame(CarrierBody, Pri, Sec, At, CI->PriAxis2, CI->SecAxis2, CI->Pos2);
	CI->InitConstraint(GrabbedBody, CarrierBody, 1.0f, CarrierBody->OwnerComponent.Get());

	//местные флаги
	Limb.Grabs = true;
	Limb.Floor.Body = GrabbedBody;
	Limb.Stepped = STEPPED_MAX;

	//просигнализировать на уровне глобального замысла
	MyrOwner()->CatchMyrLogicEvent(EMyrLogicEvent::ObjGrab, 1.0f, GrabbedBody->OwnerComponent.Get());

	//сообщить наверх и далее, если игрок, на виджет, чтоб он отметил на экране худ
	MyrOwner()->WidgetOnGrab(GrabbedBody->OwnerComponent->GetOwner());
}

//==============================================================================================================
// внешне обусловленное сбрасывание из нашей руки или пасти то, что она держала - ретурнуть отпущенный компонент 
//==============================================================================================================
UPrimitiveComponent* UMyrPhyCreatureMesh::UnGrab (FLimb& Limb)
{
	//на уровне нашего тела
	if(!Limb.Grabs) return nullptr;
	auto ToRet = Limb.Floor.Component();
	auto LBody = GetMachineBody(Limb);
	auto& CI = LBody->DOFConstraint;
	CI->TermConstraint();
	Limb.Grabs = false;

	//установить, что мы держали, если тело существа
	if(auto ItsCreatureMesh = Cast<UMyrPhyCreatureMesh>(ToRet))
	{
		//всё отпущенное тело привести к чувствительности к столкновениям
		//что вообще неправильно, так как надо сначала дать ему выйти из нашего объёма
		//а так он непонятно, куда улетит. Но для этого нужно обрабатывать перекрытия, а это лишняя сущность
		AMyrPhyCreature* Victim = ItsCreatureMesh->MyrOwner();
		//Victim->GetMesh()->SetPhyBodiesBumpable(true);

		//отпустить тело, явно задав ему дальейшее поведение (его надо тоже обнулить по всем конечностям)
		if(Victim->Health > 0) Victim->AdoptBehaveState(EBehaveState::fall);
		else Victim->AdoptBehaveState(EBehaveState::dying);
		Limb.EraseFloor();
	}
	return ToRet;
}
UPrimitiveComponent* UMyrPhyCreatureMesh::UnGrabAll ()
{	UPrimitiveComponent* ToRet = nullptr;
	for (int i = 0; i < (int)ELimb::NOLIMB; i++)
		if(!ToRet) ToRet = UnGrab(GetLimb((ELimb)i));
	return ToRet;
}

//==============================================================================================================
//установить туготу машинной части конечности
//==============================================================================================================
void UMyrPhyCreatureMesh::SetLimbDamping (FLimb& Limb, float Linear, float Angular)
{
	auto BI = GetMachineBody(Limb);
	BI->LinearDamping = Linear;
	if(Angular > 0)	BI->AngularDamping = Angular; //возможность не трогать угловое, оставив по умолчанию
	BI->UpdateDampingProperties();
}

//==============================================================================================================
//распределить часть пришедшей динамической модели на данную часть тела
//==============================================================================================================
void UMyrPhyCreatureMesh::AdoptDynModelForLimb(FLimb& Limb, uint32 Model, float DampingBase)
{
	//если вообще отсутствует эта часть тела
	if (!HasPhy(Limb.WhatAmI)) return;

	//основном физ членик этого лимба
	FBodyInstance* Body = GetMachineBody(Limb);

	//вязкость изменяется при переключении моделей
	if (!(Model & LDY_DAMPING))
		SetLimbDamping(Limb, 0, 0);	
	else
	{	float DampMult = 
		  0.25 * (bool)(Model & LDY_DAMPING_4_1)
		+ 0.25 * (bool)(Model & LDY_DAMPING_4_2)
		+ 0.5 *  (bool)(Model & LDY_DAMPING_2_1);
		SetLimbDamping(Limb, DampMult * DampingBase);
	}

	//отдельный случай для хвоста, нагружать кончик а не корень
	if(Limb.WhatAmI == ELimb::TAIL && HasFlesh(Limb.WhatAmI))
		Body = GetFleshBody(Limb, 0);

	//гравитация изменяется только в момент переключения модели
	Body->SetEnableGravity((Model & LDY_GRAVITY) == LDY_GRAVITY);


	if ((Model & LDY_FRICTION) != (Limb.DynModel & LDY_FRICTION))
	{	
		Body->SetInstanceSimulatePhysics(false);
		if (Model & LDY_FRICTION)
			Body->SetPhysMaterialOverride(MyrOwner()->GetGenePool()->AntiSlideMaterial);
		else Body->SetPhysMaterialOverride(nullptr);
		Body->SetInstanceSimulatePhysics(true);
	}

	//присвоение флагов - они оживают динамически каждый кадр
	Limb.DynModel = Model;
}

//==============================================================================================================
//расслабить все моторы в физических привязях
//==============================================================================================================
void UMyrPhyCreatureMesh::SetFullBodyLax(bool Set)
{
	//сделать регдоллом
	UE_LOG(LogMyrPhyCreature, Log, TEXT("%s: SetFullBodyLax %d"), *GetOwner()->GetName(), Set);
	if (Set)
	{	for (auto C : Constraints)
		{	C->SetOrientationDriveTwistAndSwing(false, false);
			C->SetAngularVelocityDriveTwistAndSwing(false, false);
			C->SetLinearPositionDrive(false, false, false);
			C->SetLinearVelocityDrive(false, false, false);
		}
		//внимание, без этого не берет труп в зубы, в то же время с этим собаки реалистичнее мертвы
		//хз, сделать переключение по массе или спец-настройку
		//SetPhyBodiesBumpable(false);
	}
	//сделать напряженным
	else
	{
		for (int i = 0; i < Constraints.Num(); i++)
		{
			//здесь так многословно, потому что вместе со сменой переменных еще нужно вызывать команды физ-движка
			auto pre = GetArchConstraint(i)->ProfileInstance;
			Constraints[i]->SetOrientationDriveTwistAndSwing(pre.AngularDrive.TwistDrive.bEnablePositionDrive, pre.AngularDrive.SwingDrive.bEnablePositionDrive);
			Constraints[i]->SetAngularVelocityDriveTwistAndSwing(pre.AngularDrive.TwistDrive.bEnableVelocityDrive, pre.AngularDrive.SwingDrive.bEnableVelocityDrive);
			Constraints[i]->SetLinearPositionDrive(pre.LinearDrive.XDrive.bEnablePositionDrive, pre.LinearDrive.YDrive.bEnablePositionDrive, pre.LinearDrive.ZDrive.bEnablePositionDrive);
			Constraints[i]->SetLinearVelocityDrive(pre.LinearDrive.XDrive.bEnableVelocityDrive, pre.LinearDrive.YDrive.bEnableVelocityDrive, pre.LinearDrive.ZDrive.bEnableVelocityDrive);
		}
		
		//SetPhyBodiesBumpable(true);
	}
}

//==============================================================================================================
//прописать в членики профили стокновений чтобы они были либо сталкивались с другими существами, либо нет
//==============================================================================================================
void UMyrPhyCreatureMesh::SetPhyBodiesBumpable(bool Set)
{
	//в непрозрачном состоянии членики сталкиваются с такими же члениками другого тела
	if(Set)
	{
		//без этого не работает, хотя что есть установка ко всему мешу, а не к членику - неясно
		SetCollisionProfileName(TEXT("PawnSupport"));

		//SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		for (int i = 0; i<Bodies.Num(); i++)
		{	FBODY_BIO_DATA bbd = MyrOwner()->GetGenePool()->BodyBioData[i];
			if(bbd.DistFromLeaf == 255)	Bodies[i]->SetCollisionProfileName(TEXT("PawnSupport"));
			else						Bodies[i]->SetCollisionProfileName(TEXT("PawnMeat"));
		}
		//GetRootBody()->SetCollisionProfileName(TEXT("PawnTransparent"));

		//GetRootBody()->bHACK_DisableCollisionResponse = false;
		//GetRootBody()->bHACK_DisableSkelComponentFilterOverriding = false;
		//this->SkeletalMesh->PhysicsAsset->SkeletalBodySetups[9]->CollisionReponse
	}

	//в прозрачном состоянии членики не сталкиваются с другими члениками тела, но хорошо бы сталкивались с итнерьерами
	else
	{
		//без этого не работает, хотя что есть установка ко всему мешу, а не к членику - неясно
		SetCollisionProfileName(TEXT("PawnTransparent"));
		for (int i = 0; i < Bodies.Num(); i++)
		{	Bodies[i]->SetCollisionProfileName(TEXT("PawnTransparent"));
		}
	}
	UE_LOG(LogMyrPhyCreature, Warning, TEXT("%s SetPhyBodiesBumpable %d"), *GetOwner()->GetName(), Set);
}

//==============================================================================================================
//включить или выключить физику у всех костей, которые могут быть физическими
//==============================================================================================================
void UMyrPhyCreatureMesh::SetMachineSimulatePhysics(bool Set)
{
	SetSimulatePhysics(Set);
	if (Set)
	{	for (int i = 0; i < Bodies.Num(); i++)
		{
			FBODY_BIO_DATA bbd = MyrOwner()->GetGenePool()->BodyBioData[i];
			if (bbd.DistFromLeaf == 255)		Bodies[i]->SetInstanceSimulatePhysics(true);	// машина
			else if (bbd.eLimb == ELimb::TAIL)	Bodies[i]->SetInstanceSimulatePhysics(true);	// мясо кинематическое
		}

	}
	else SetAllBodiesSimulatePhysics(false);
}

//==============================================================================================================
//изменить силы упругости в спинном сцепе
//==============================================================================================================
void UMyrPhyCreatureMesh::SetSpineStiffness(float Factor)
{
	AdjustSpineStiffness(Pectus, Factor);	//спина посередине
	AdjustSpineStiffness(Thorax, Factor);	//передний пояс 
	AdjustSpineStiffness(Lumbus, Factor);	//задний пояс
}

void UMyrPhyCreatureMesh::AdjustSpineStiffness(FLimb& Limb, float Factor)
{
	auto CI = GetMachineConstraint(Limb);
	auto aCI = GetArchMachineConstraint(Limb);
	CI->SetAngularDriveParams(
		aCI->ProfileInstance.AngularDrive.SlerpDrive.Stiffness * Factor,
		aCI->ProfileInstance.AngularDrive.SlerpDrive.Damping * Factor,
		aCI->ProfileInstance.AngularDrive.SlerpDrive.MaxForce);

}

//==============================================================================================================
// изогнуть вектор курса так, чтоб он огибал препятствие или отражался
//==============================================================================================================
bool UMyrPhyCreatureMesh::GuideAlongObstacle(FVector3f &Guide, bool Penetra, float Amount)
{	
	auto& L = Bumper1();

	//критерий истинного утыкновения = свободный физ-членик уткнулся в опору, но это не та опора, на которой мы стоим
	if (L.Stepped > 0 && L.Floor.TooSteep() && L.Floor != Thorax.Floor && L.Floor != Pelvis.Floor)
	{	
		//если уткнулись не под прямым углом - надо смазывать траекторию вдоль стены
		//причем если предмет помечен "может ступать" обтекать его нежелательно
		if(L.Colinea < 200 - 50*(int)L.Floor.CharCanStep())
		{
			float A = L.Stepped / (float)STEPPED_MAX * ((int)L.Colinea + 200) / 455.0 * Amount;
			auto& No = IntegralBumpNormal;
			auto ProjOnNo = Guide.ProjectOnToNormal(No);	// вытянуть по нормали - полуфабрикат для расчёта проекции и отражения
			auto SlideOrRefl = Guide - ProjOnNo;			// насальное значение - курс, уложенный в плоскость стены
			if (Penetra)									// если кинематическим якорем просочились внутрь то
				SlideOrRefl -= ProjOnNo;					// довести вектор курса до полного отражения от стены
			Guide = FMath::Lerp(Guide, SlideOrRefl, A);
			return true;
		}
	}
	return false;
}

//==============================================================================================================
//обнаружение частичного провала под поверхность/зажатия медлу плоскостью
//==============================================================================================================
ELimb UMyrPhyCreatureMesh::DetectPasThrough()
{
	if (Tail.Stepped && Lumbus.Stepped)
		if ((Tail.Floor.Normal | Lumbus.Floor.Normal) < -0.8 && Tail.Floor == Lumbus.Floor)
			return (Tail.Floor.Normal.Z > Lumbus.Floor.Normal.Z) ? ELimb::TAIL : ELimb::LUMBUS;
	if (Lumbus.Stepped && Pectus.Stepped)
		if ((Lumbus.Floor.Normal | Pectus.Floor.Normal) < -0.8 && Pectus.Floor == Lumbus.Floor)
			return (Lumbus.Floor.Normal.Z > Pectus.Floor.Normal.Z) ? ELimb::LUMBUS : ELimb::PECTUS;
	if (Pectus.Stepped && Head.Stepped)
		if ((Pectus.Floor.Normal | Head.Floor.Normal) < -0.8 && Head.Floor == Pectus.Floor)
			return (Pectus.Floor.Normal.Z > Head.Floor.Normal.Z) ? ELimb::PECTUS : ELimb::HEAD;
	return ELimb::NOLIMB;
}

//==============================================================================================================
//суммарная потеря двигательных функций всего тела (множитель скорости)
//эта функция нужна только в  AMyrPhyCreature::UpdateMobility, но туда переносить - громоздко
//==============================================================================================================
float UMyrPhyCreatureMesh::GetWholeImmobility()
{
	float Mob = 0; 
	for (int i = (int)ELimb::NOLIMB-1; i >= 0; i--)	
		Mob += FMath::Min(1.0f, GetLimb((ELimb)i).Damage) * MachineGene((ELimb)i) -> AffectMobility;
	return FMath::Min(Mob, 1.0f);
}




