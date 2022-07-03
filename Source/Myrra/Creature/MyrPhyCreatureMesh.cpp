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
#include "../Dendro/MyrDendroMesh.h"					// деревья - для поддержания лазанья по ветке
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
	this->UpdateRBJointMotors();

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//проверки для отсечения всяких редакторных версий объекта, чтоб не тикало
	if (TickType == LEVELTICK_ViewportsOnly) return; //только это помогает
	if (Bodies.Num() == 0) return;

	//обработать всяческую жизнь внутри отдельных частей тела (их фиксированное число)
	for (int i = 0; i < (int)ELimb::NOLIMB; i++)
		ProcessLimb (GetLimb((ELimb)i), DeltaTime);

	//рисануть основные траектории
	LINELIMB(ELimbDebug::MainDirs, Head, MyrOwner()->MoveDirection * 100);
	LINELIMB(ELimbDebug::MainDirs, Head, MyrOwner()->AttackDirection * 100);

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

	//для простого учета развернутости осей (возможно, не нужен, так как ForcesMultiplier можно задать отрицательным
	float Revertor = (Limb.DynModel & LDY_REVERSE) ? -1.0f : 1.0f;


	//применить поддерживающую силу (вопрос правильной амплитуды по-прежнему стоит)
	float Power = Girdle->CurrentDynModel->ForcesMultiplier;
	if (Limb.DynModel & LDY_EXTGAIN) Power *= MyrOwner()->ExternalGain; else
	if (Limb.DynModel & LDY_NOEXTGAIN) Power *= (1 - MyrOwner()->ExternalGain);

	//ограничить до стоящих на земле или наоборот
	if ((Limb.DynModel & LDY_ONGROUND)) Power *= Girdle->StandHardness;
	if ((Limb.DynModel & LDY_NOTONGROUND)) Power *= 1 - Girdle->StandHardness;


	//исключить вызов дополнительной тянулки-вертелки, если "по курсу" включено, а дополнительной оси не задано
	bool OnlyOrient = true;

	//если указано что-то делать в сторону курса, то это режим управлеямого игроком движения
	if ((Limb.DynModel & (LDY_MY_WHEEL)))
	{	Move(BodyInst, Limb);
		OnlyOrient = false;
	}

	//если указано что-то ориентировать по направлению курса
	if((Limb.DynModel & LDY_TO_COURSE) && OnlyOrient)
	{	FVector GloAxis = Girdle->GuidedMoveDir * Revertor * MyrOwner()->ExternalGain;
		if(MyrOwner()->bMoveBack) GloAxis = -GloAxis; //пятиться назад, но не поворачиваться 
		OrientByForce(BodyInst, Limb, CalcAxisForLimbDynModel(Limb, GloAxis), GloAxis, Power);
	}

	//если указано ориентировать по направлению глобально вертикально вверх
	if(Limb.DynModel & LDY_TO_VERTICAL)
	{	FVector GloAxis = FVector(0, 0, Revertor);
		OrientByForce(BodyInst, Limb, CalcAxisForLimbDynModel(Limb, GloAxis), GloAxis, Power);
	} 

	//если указано ориентировать по направлению куда смотрит камера
	if(Limb.DynModel & LDY_TO_ATTACK)
	{	FVector GloAxis = MyrOwner()->AttackDirection*Revertor;
		OrientByForce(BodyInst, Limb, CalcAxisForLimbDynModel(Limb, GloAxis), GloAxis, Power);
	} 

	//если что-то вытянуть по направлению вверх или своей нормали, или средней нормали своего пояса
	//элс, потому что или уж по глобальному верху, или по локальной нормали
	else if(Limb.DynModel & LDY_TO_NORMAL)
	{	
		//в качестве нормали - если нога, то вдавливать в местную нормаль, иначе в "фиктивную нормаль" таза
		auto CLimb = Girdle->GetLimb(EGirdleRay::Center);
		FVector GloAxis = (Limb.Stepped ? Limb.ImpactNormal : GetLimbAxisUp(CLimb)) * Revertor;
		OrientByForce(BodyInst, Limb, CalcAxisForLimbDynModel(Limb, GloAxis), GloAxis, Power);
	}

	//если указана ось локальная, но не указана ось глобальная - особый режим (кручение вокруг себя)
	else if ((Limb.DynModel & LDY_MY_AXIS) && !(Limb.DynModel & LDY_TO_AXIS))
	{
		FVector DesiredAxis = CalcAxisForLimbDynModel(Limb, FVector(0)) * Power * Revertor;
		if (Limb.DynModel & LDY_ROTATE)
		{	BodyInst->AddTorqueInRadians(DesiredAxis, false, true);
			LINELIMB(ELimbDebug::LimbTorques, Limb, DesiredAxis * 50);
		}
		else
		{	BodyInst->AddForce(DesiredAxis, false, true);
			LINELIMB(ELimbDebug::LimbForces, Limb, DesiredAxis * 50);
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
//текущее положение плеча
//==============================================================================================================
FVector UMyrPhyCreatureMesh::GetLimbShoulderHubPosition(ELimb eL)
{
	//сначала попытаться взять имя той кости, которая явно указана в генофонде как позиция для отсчёта этой конечности
	FName AxisName = MachineGene(eL)->AxisReferenceBone;

	//если таковая не указана, то взять (?) позицию физического членика
	if (AxisName.IsNone()) return GetMachineBody(eL)->GetCOMPosition();

	//тут нужно еще проверять, существует ли вписанная кость
	else return GetBoneLocation(AxisName);

	//по умолчанию
	return FVector(0);
}



//==============================================================================================================
//детекция степени утыкновения конкретным члеником в стену четко по направлению
//==============================================================================================================
float UMyrPhyCreatureMesh::StuckToStepAmount(const FLimb& Limb) const
{
	//если членик ничего не касается, то он и ни на что не натыкается
	if (Limb.Stepped)
	{	
		//направление, сопротивление которому оценивается
		FVector& MoveDir = MyrOwner()->GetGirdle(Limb.WhatAmI)->GuidedMoveDir;

		//коэффициент, сбавляющий результат при бодании на совсем низкой скорости
		float Attenu = MyrOwner()->GetGirdle(Limb.WhatAmI)->SpeedAlongFront();

		//степень давления в опору, max - исключить отрицательную скалярку (тянем от)
		//неясно, как орудовать со скоростью, но пока коэффициент насыщения 2 см/с
		float StuckAmount = FMath::Max(0.0f,
			Limb.ImpactNormal | (-MoveDir*FMath::Min(Attenu*2, 1.0f)));

		//позитивный исход - если объект помечен как лазибельный, число - степнь желания как-то действовать по самоподнятию на это препятствие
		//хотя это не всегда правильно, нужен верхний порог, который бы говорил - слишком крутое, не взобраться
		//но это можно сделать вне данной функции
		if (Limb.Floor->OwnerComponent.Get())
			if (Limb.Floor->OwnerComponent->CanCharacterStepUpOn == ECanBeCharacterBase::ECB_Yes)
				return StuckAmount;

		//отрицательный исход, препятствие не лазибельно, и нужно сбавить силу или отразиться
		//тут нужен нижний порог, выше - сдаваться, ниже - делать вид, что не замечаешь
		return -StuckAmount;
	}
	return 0.0f;
}

//==============================================================================================================
//детекция намеренного натыкания на стену
//==============================================================================================================
float UMyrPhyCreatureMesh::StuckToStepAmount(ELimb& LevelOfContact) const
{
	//магнитуда копится по модулю, вердикт - со знаками (лезть или отстраняться)
	float Magnitude = 0.0f; float Verdict = 0.0f; LevelOfContact = ELimb::NOLIMB;

	//голова - самая болезненно реагирующая часть
	float NewProbe = StuckToStepAmount(Head);
	Magnitude += FMath::Abs(NewProbe); Verdict += NewProbe;
	LevelOfContact = ELimb::HEAD;
	if (Magnitude > 0.7) return Verdict;

	//передние ноги скопом, первые замечают невысокие препятствия
	NewProbe = 0.5 * (StuckToStepAmount(LArm) + StuckToStepAmount(RArm));
	Magnitude += FMath::Abs(NewProbe); Verdict += NewProbe;
	LevelOfContact = ELimb::R_ARM;
	if (Magnitude > 0.7) return Verdict;

	//грудь - это больше для человека актуально (возможно, ввести всё же 
	NewProbe = 0.5 * (StuckToStepAmount(Pectus) + StuckToStepAmount(Thorax));
	Magnitude += FMath::Abs(NewProbe); Verdict += NewProbe;
	LevelOfContact = ELimb::THORAX;
	if (Magnitude > 0.7) return Verdict;

	//задние ноги - это тоже больше для человека актуально 
	NewProbe = 0.5 * (StuckToStepAmount(LLeg) + StuckToStepAmount(RLeg));
	Magnitude += FMath::Abs(NewProbe); Verdict += NewProbe;
	LevelOfContact = ELimb::R_LEG;
	if (Magnitude > 0.7) return Verdict;

	return 0.0f;
}

//==============================================================================================================
//костылик для обработчика физики части тела - получить по битовому полю направление собственной оси, которое недо повернуть
//==============================================================================================================
FVector UMyrPhyCreatureMesh::CalcAxisForLimbDynModel(const FLimb Limb, FVector GloAxis) const
{
	//значение по умолчанию (для случаев колеса) - чтобы при любой идеальной оси разница идеальность была нулевой и сила лилась постоянно
	//то есть если пользователь не указан ваще никакую "мою ось", глобальный вектор ориентации всё равно применялся бы, но по полной, без обратной связи
	FVector MyAxis = FVector(0,0,0);

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
//ориентировать тело силами, поворачивая или утягивая
//==============================================================================================================
void UMyrPhyCreatureMesh::OrientByForce (FBodyInstance* BodyInst, FLimb& Limb, FVector ActualAxis, FVector DesiredAxis, float AdvMult)
{
	//степень отклонения от правильной позиции = сила воздействия, стремящегося исправить отклонение (но еще не знак)
	//если актуальная ось = 0, то и эта переменная 0, то есть идеал недостижим и сила будет применяться всегда
	const float AsIdeal = FVector::DotProduct(ActualAxis, DesiredAxis);

	//если требуется делать это кручением тела
	if(Limb.DynModel & LDY_ROTATE)
	{
		//выворот по оси-векторному произведению, если ошибка больше 90 градусов, амплитуда константно-максимальная
		if (AsIdeal > 0.99) return;
		auto RollAxis = FVector::CrossProduct(ActualAxis, DesiredAxis);
		if (AsIdeal < 0)	RollAxis.Normalize();
		FVector Final = RollAxis * AdvMult * (1 - AsIdeal);
		if (Final.ContainsNaN())
			Log(Limb, TEXT("WTF OrientByForce contains nan"));
		else
		{	BodyInst->AddTorqueInRadians(Final, false, true);
			LINELIMB(ELimbDebug::LimbTorques, Limb, ActualAxis * 40);
			LINELIMB(ELimbDebug::LimbTorques, Limb, DesiredAxis * 50);
		}
	}

	//если требуется просто тянуть (это само по себе не ориентирование, но если есть привязи, они помогут)
	if (Limb.DynModel & LDY_PULL)
	{
		//модуль силы тащбы
		FVector Final = DesiredAxis * (1 - AsIdeal) * AdvMult;

		//если тело уже уткнуто, заранее обрезать силу
		//хз вообще что это и зачем
		if(Limb.Stepped)
			Final *= 1.0f - FMath::Max(0.0f, FVector::DotProduct(-Limb.ImpactNormal, DesiredAxis));

		//применить прямую силу по нужной оси
		if (Final.ContainsNaN()) Log(Limb, TEXT("WTF OrientByForce contains nan"));
		else
		{	BodyInst->AddForce(Final, false, true);
			LINELIMB(ELimbDebug::LimbForces, Limb, DesiredAxis * 0.3 * Limb.Stepped);
		}
	}
}


//==============================================================================================================
//применить двигательную силу к телу, полагая, что это тело является безосевым колесом а-ля шарик ручки
//==============================================================================================================
void UMyrPhyCreatureMesh::Move(FBodyInstance* BodyInst, FLimb& Limb)
{
	auto Girdle = MyrOwner()->GetGirdle(Limb.WhatAmI);
	if (MyrOwner()->MoveGain < 0.01) return;		//не двигать, если сам организм не хочет или не может

	//направление: его можно инвертировать
	FVector Dir(0);
	if (Limb.DynModel & LDY_TO_COURSE)		Dir = Girdle->GuidedMoveDir; else
	if (Limb.DynModel & LDY_TO_VERTICAL)	Dir = FVector(0,0,1); 

	if (Limb.DynModel & LDY_REVERSE)
		Dir = -Dir;

	//разность между текущим вектором скорости и желаемым (вектор курса * заданный скаляр скорости)
	//убрано, так как в тяге очень резко и нестабильно меняется курс
	//неясно, насколько это критично, надо вводить торможение по боковой скорости
	//также при FullUnderspeed кошка на колёсах не разворачивается под 90 грдусов, застывает
	//const FVector FullUnderspeed = G.GuidedMoveDir * MyrOwner()->GetDesiredVelocity() - G.VelocityAgainstFloor;

	//отрицательная обратная связь - если актуальная скорость меньше номинальной
	const float SpeedProbe = FVector::DotProduct(Girdle->VelocityAgainstFloor, Dir);
	const float Underspeed = MyrOwner()->GetDesiredVelocity() - SpeedProbe;
	if (SpeedProbe > -MyrOwner()->GetDesiredVelocity())
	{
		//дополнительное усиление при забирании на склон, включается только если в гору, чем круче, тем сильнее
		float UpHillAdvForce = FMath::Max(0.0f, Dir.Z) * MyrOwner()->BehaveCurrentData->UphillForceFactor;

		//угнетение движения, если наткнулись на препятствие (хз зачем тепер, ведь есть система Stuck/BumpReaction)
		if(Limb.Stepped)
			if (Limb.Floor->OwnerComponent.Get())
			{	if (Limb.Floor->OwnerComponent->CanCharacterStepUpOn == ECanBeCharacterBase::ECB_No)
					if (Limb.ImpactNormal.Z < 0.6)
						UpHillAdvForce *= 1.0f - FMath::Max(FVector::DotProduct(-Limb.ImpactNormal, Dir), 0.0f);
			}
			else Log(Limb, TEXT("WTF floor suddenly disappeared"));

		//вращательное усилие 
		if (Limb.DynModel & LDY_ROTATE)
		{
			//ось вращения - если на опоре, то через векторное произведение (и ведь работает!) если в падении - то прямо вокруг курса (для управления вращением)
			//надо тут ввести иф и два разных варианта для колеса и для вращения в воздухе
			FVector Axis = Limb.Stepped ? FVector::CrossProduct(Limb.ImpactNormal, Dir) : MyrOwner()->AttackDirection;

			//ось вращения с длиной в виде силы вращения
			FVector Torque;

			//режим ноги на опоре - движение колесами
			if(Limb.Stepped)
				Torque = 
					FVector::CrossProduct(Limb.ImpactNormal, Dir)			// из направления движения и нормали вот так легко извлекается ось вращения
					* Underspeed											// пропорционально недодаче скорости
					* MyrOwner()->BehaveCurrentData->PlainForceFactor		// поправочный коэффициент для не в горку
					* (1.0f + UpHillAdvForce);								// дополнительное усиление при забирании на склон
			
			//режим управляемого падения - стараться закрутить его так чтобы компенсировать вращение остального туловища
			else Torque = MyrOwner()->AttackDirection * MyrOwner()->BehaveCurrentData->PlainForceFactor * MyrOwner()->GetDesiredVelocity();

			//применить
			if (Torque.ContainsNaN()) 
			{	Log(Limb, TEXT("WTF Move LDY_ROTATE contains nan, cur state = "),
					TXTENUM(EBehaveState, MyrOwner()->GetBehaveCurrentState()));
			}
			else
			{	BodyInst->AddTorqueInRadians(Torque, false, true);
				LINELIMB(ELimbDebug::LimbTorques, Limb, Torque);
			}
		}

		//поступательное усилие - только если касаемся опоры, ибо тяга, в отличие от вращения, будет искажать падение
		if ((Limb.DynModel & LDY_PULL))
		{
			//в режиме тяжелых ног тяга применяется только при касании поверхности (ходьба)
			//для прочих случаев постоянной тяги (летать) нужно сбросить флаг HevyLegs, он и так тут не нужен
			if (!Limb.Stepped && Girdle->CurrentDynModel->HeavyLegs) return;

			//if (G.StandHardness < 0.1) return;					//хз зачем так оптимизировать, никому не мешает
			FVector Direct = Dir									// направление хода
				* Underspeed										// недостача скорости = обратная связь
				* MyrOwner()->BehaveCurrentData->PlainForceFactor	// поправочный коэффициент
				* (1.0f + UpHillAdvForce);							// поправка для облегчения взбирания на горку

			// применить
			if (Direct.ContainsNaN())
			{	Log(Limb, TEXT("WTF Move LDY_PULL contains nan, cur state = "),
					TXTENUM(EBehaveState, MyrOwner()->GetBehaveCurrentState()));
			}
			else
			{
				BodyInst->AddForce(Direct, false, true);
				LINELIMB(ELimbDebug::LimbForces, Limb, Direct);
			}
		}
	}
}

//==============================================================================================================
//телепортировать всё тело на седло ветки - пока вызывается в HitLimb, что, возможно, плохо, ибо не среагирует при застывании
//что-то старое, не используется, возможно, удалить
//==============================================================================================================
void UMyrPhyCreatureMesh::TeleportOntoBranch(FBodyInstance* CapsuleBranch)
{
	FQuat Rot;
	//подготовить исходную позицию
	FVector Pos = GetComponentLocation();

	//если возможно поместить на ветку
	if (GetBranchSaddlePoint(CapsuleBranch, Pos, 0.0f, &Rot, true))
	{	
		//собственно трансместить
		FHitResult Hit;
		SetWorldLocationAndRotation(Pos, Rot, false, &Hit, ETeleportType::TeleportPhysics);
		BodyInstance.SetLinearVelocity(FVector(0), false);

		//подправить камеру и траекторию, если нужно
		MyrOwner()->PostTeleport();

		//всякая диагностика
		UE_LOG(LogTemp, Error, TEXT("%s: TeleportOntoBranch %s "), *GetOwner()->GetName());
		//DrawMove(Pos, Rot.GetUpVector()*10, FColor(255, 30, 255), 10, 10);
	}
	else UE_LOG(LogTemp, Error, TEXT("%s: TeleportOntoBranch - Unable, to upphill %s "), *GetOwner()->GetName());
}


//==============================================================================================================
//прыжковый импульс для одной части тела
//==============================================================================================================
void UMyrPhyCreatureMesh::PhyJumpLimb(FLimb& Limb, FVector Dir)
{
	//если в этой части тела вообще есть физ-тело каркаса
	if(HasPhy(Limb.WhatAmI))
	{	
		//сбросить индикатор соприкоснутости с опорой
		Limb.EraseFloor();

		//тело
		auto BI = GetMachineBody(Limb);

		//отменить линейное трение, чтобы оно в воздухе летело, а не плыло как в каше
		BI->LinearDamping = 0;
		BI->UpdateDampingProperties();

		//применить скорость
		BI->AddImpulse(Dir, true);
		LINELIMBWT(ELimbDebug::LimbForces, Limb, Dir * 0.1, 1, 0.5);

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
	//столконвение с плотью, не с поддержкой (если включены для этого членика)
	//if (DistFromLeaf != 255) PhyDeflect(Limb, 1.0);

	//пока жесткая заглушка - если этот членик несёт физически кого-то, то он не принимает сигналы о касаниях
	//это чтобы сохранить неизменным значение Floor, в котором всё время ношения ссылка на несомый объект
	if (Limb.Grabs) return;
	
	//общая инфа по конечности для данного вида существ
	const auto LimbGene = MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Limb.WhatAmI).Machine;

	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	//если опора скелетная (дерево, другая туша) определить также номер физ-членика опоры
	auto NewFloor = Hit.Component.Get()->GetBodyInstance(Hit.BoneName);

	//сохранить тип поверхности для обнаружения возможности зацепиться
	auto FM = Hit.PhysMaterial.Get();
	if (FM) Limb.Surface = (EMyrSurface)FM->SurfaceType.GetValue();

	//результат для новой опоры: 0 - новая равна старой, 1 - новая заменила старую (надо перезагрузить зацеп), -1 - новая отброшена, старая сохранена
	const int NewFloorDoom = ResolveNewFloor(Limb, NewFloor, Hit.ImpactNormal, Hit.ImpactPoint);

	//определить абсолютную скорость нас и предмета, который нас задел, в точке столконвения
	FVector FloorVel = Hit.Component.Get() -> GetPhysicsLinearVelocityAtPoint (Hit.ImpactPoint);
	FVector MyVel = GetPhysicsLinearVelocityAtPoint(Hit.ImpactPoint);

	//вектор относителой скорости
	FVector RelativeVel = MyVel - FloorVel;

	//и его квадрат для первичных прикидок
	float RelSpeed = RelativeVel.SizeSquared();

	//коэффициент болезненности удара, изначально берется только та часть скорости, что в лоб (по нормали)
	float BumpSpeed = FVector::DotProduct(RelativeVel, -Hit.ImpactNormal);

	//относительная скорость больше 10см/с, то есть произвольно большая
	if (RelSpeed > 100)
	{
		//полный модуль относительной скорости
		float ScalarVel = FMath::Sqrt(RelSpeed);

		//коррекция с учетом касательной компоненты скорости
		BumpSpeed += 0.1 * ScalarVel;

	}
	//скорость незначительна, точность не нужна
	else
	{
		//коррекция с учетом касательной компоненты скорости
		BumpSpeed += 0.01 * RelSpeed;
	}

	//странно большое значение скорости - попытка найти причину улёта
	if (BumpSpeed > 1000) Log(Limb, TEXT("HitLimb WTF BumpSpeed "), BumpSpeed);

	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	//воспроизвести звук, если контакт от падения (не дребезг и не качение) и если скорость соударения... откуда эта цифра?
	//в вертикальность ноль так как звук цепляния здесь не нужен, только тупое соударение
	if (BumpSpeed > 10 && Limb.Stepped == 0)
		MyrOwner()->SoundOfImpact (nullptr, Limb.Surface, Hit.ImpactPoint, BumpSpeed, 0, EBodyImpact::Bump);

	//все что ниже относится только к живым объектам
	if(MyrOwner()->Health <= 0) return;

	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	//возможность покалечиться - импульс удара выше порога,
	if (ApplyHitDamage(Limb, DistFromLeaf, LimbGene.HitShield, BumpSpeed, Hit) > 0.3)
	{ 
		//а если высок, немного ниже - просто подогнуть ноги (если это конечно ноги - у них точно есть констрейнт)
		if (GetMachineConstraintIndex(Limb) != 255)
		{
			//распознаём, что это конечность по наличию линейного мотора - и прижимаем конечность к телу
			auto CI = GetMachineConstraint(Limb);
			if (CI->ProfileInstance.LinearDrive.ZDrive.bEnablePositionDrive)
				CI->SetLinearPositionTarget(FVector(0, 0, -CI->ProfileInstance.LinearLimit.Limit));
		}
	}

	//отобразить отладку, место контакта с опорой
	LINEW(ELimbDebug::GirdleStepped, Hit.ImpactPoint, FVector(0), 3);
	//UE_LOG(LogTemp, Log, TEXT("%s: HitLimb %s part %d"), *GetOwner()->GetName(), *TXTENUM(ELimb, Limb.WhatAmI), DistFromLeaf);

}

//==============================================================================================================
//рассчитать возможный урон от столкновения с другим телом - возвращает истину, если удар получен
// возвращает отношение скорости удара к пороговой скорости, начиная с которой идут травмы
//==============================================================================================================
float UMyrPhyCreatureMesh::ApplyHitDamage(FLimb& Limb, uint8 DistFromLeaf, float TerminalDamageMult, float BumpSpeed, const FHitResult& Hit)
{
	float OldDamage = Limb.Damage;
	float Severity = 0;

	//порог для отсечения слабых толчков (без звука и эффекта) и определения болезненных (>1, с уроном)
	float Thresh = MyrOwner()->GetGenePool()->MaxSafeShock;	// базис порога небольности в единицах скорости 
	Thresh *= MyrOwner()->Phenotype.BumpShockResistance();		// безразмерный коэффициент индивидуальной ролевой прокачки
	Thresh *= TerminalDamageMult;								// безразмерный коэффициент дополнительной защиты именно этой части тела

	//выдрать данные о поверхности, оно нужно далее
	auto SurfInfo = MyrOwner()->GetMyrGameInst()->Surfaces.Find(Limb.Surface);

	//обработка ушибов от сильных ударов
	//если даже 1/10 порога не достигнута, дальше вообще не считать
	if (InjureAtHit && BumpSpeed > Thresh * 0.1)
	{
		//костыль ad hoc, типа если стояли, то крепко на земле, а сильно ударили - не устояли и начали смещаться
		if(MyrOwner()->CurrentState == EBehaveState::stand)
			MyrOwner()->AdoptBehaveState(EBehaveState::walk);

		//удельная сила, через деление - для мягких диапазонов и неразрывности, может быть меньше 0, посему через Макс
		switch(Hit.Component->GetCollisionObjectType())
		{
			//неподвижные глыбы - самый низкий порог, ибо минимум глюков
			case ECC_WorldStatic:
				Severity += FMath::Max(0.0f, BumpSpeed / Thresh - 1);
				break;

			//столкновение с другим существом, могут быть глюки физдвижка, поэтому коэффициент десятикратный
			case ECC_Pawn:
				Severity += FMath::Max(0.0f, BumpSpeed / (Thresh*10) - 1);
				break;
		}

		//больше нуля значит импульс был больше порогового
		if (Severity > 0.0f)
		{	
			//увеличиваем урон
			Limb.Damage += Severity;

			//поднимаем сигнал боли наверх, где будет сыгран звук удара и крик
			MyrOwner()->Hurt(Limb.WhatAmI, Severity, Hit.ImpactPoint, Hit.ImpactNormal, Limb.Surface, SurfInfo);
		}
	}
	//обработка кусачих поверхностей, типа раскалённых или крапивы, для этого нужно влезть в список данных по поверхностям
	if(SurfInfo)
	{	if(SurfInfo->HealthDamage > 0.1)
			Severity += SurfInfo->HealthDamage;
	}

	//по сумме от силового удара и от кусачести поверхности поднимаем на уровень всего существа инфу, что больно
	if(Severity > 0.01)
	{	Limb.Damage += Severity;
		MyrOwner()->Hurt (Limb.WhatAmI, Severity, Hit.ImpactPoint, Hit.ImpactNormal, Limb.Surface, SurfInfo);
	}

	//если данный удар призошлел из состояния полета или падения, то 
	//вне зависимости от силы удара это считается поводом для прерывания действий
	if (MyrOwner()->GotUntouched())
		MyrOwner()->CeaseActionsOnHit();

	//если мы атакуем именно этой частью тела
	if(MyrOwner()->IsLimbAttacking(Limb.WhatAmI))
	{
		//часть тела хватаемого существа, буде таковое есть
		FLimb* TheirLimb = nullptr;

		//физ-тело противника (целое или физ-членик по имени кости)
		FBodyInstance* TheirBody = Hit.Component->GetBodyInstance(Hit.BoneName);

		//////////////////////////////////////////////////
		//если мы ударили другое живое существо
		auto TheirMesh = Cast<UMyrPhyCreatureMesh>(Hit.Component.Get());
		if(TheirMesh)
		{
			//найти из физ-членика часть тела противника
			FBODY_BIO_DATA bbd = TheirMesh->MyrOwner()->GetGenePool()->BodyBioData[TheirBody->InstanceBodyIndex];
			TheirLimb = &TheirMesh->GetLimb(bbd.eLimb);

			//базис урона из настроек атаки - безразмерный, 1 = типичная атака для любого существа, меньше может быть
			const auto DamageBase = MyrOwner()->GetAttackActionVictim().TactileDamage;

			//нанесение урона только если наша атака увечная
			if(DamageBase > 0)
			{	
				//если жертва жива и если повреждаемый членик только что не был поврежден (для предотвращения дребезга)
				if(TheirMesh->MyrOwner()->Health > 0 && !TheirLimb->LastlyHurt)
				{
					//набираем факторов, влияющих на увечье
					float dD = DamageBase * MyrOwner()->Phenotype.AttackDamageFactor();

					//также если жертва сама атакует, ее атака может давать дополнительный щит
					if(TheirMesh->MyrOwner()->Attacking())
						dD -= TheirMesh->MyrOwner()->GetAttackAction()->HitShield * TheirMesh->MyrOwner()->Phenotype.AttackShieldFactor();

					//если его контратака не полностью нивеллировала нашу
					if (dD >= 0)
					{
						//вступает в действие внутренняя сопротивляемость разных частей тела
						//но нахрена она нужна, если не прокачивается и если всё равно каждое увечье - по месту
						//лучьше внести разные степени восстанаовления разных частей тела
						dD *= TheirMesh->MachineGene(TheirLimb->WhatAmI)->HitShield;
						TheirLimb->Damage += dD;

						//сопутствующие действия
						TheirLimb->LastlyHurt = true;
						TheirMesh->MyrOwner()->SufferFromEnemy(dD, MyrOwner());									// ментально, ИИ, только насилие
						TheirMesh->MyrOwner()->Hurt(TheirLimb->WhatAmI, dD, Hit.ImpactPoint, Hit.ImpactNormal, TheirLimb->Surface);	// визуально, звуково, общий урон
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
				TheirMesh->MyrOwner()->EnjoyAGrace (DamageBase, MyrOwner());
			}
		}
		//если ударили не живое существо, а предмет
		else
		{

		}

		//если наша атака - это хватание и сейчас как раз самая фаза
		if (MyrOwner()->NowPhaseToGrab())
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
			if(TheirMesh) MyrOwner()->NewAttackGoBack();
		}
	}
	//несли мы не атакуем (возможно, мы жертва)
	else
	{
		//восстановление только что отпущенного из зубов "прозрачного" тела в "непрозрачность"
		if(!Cast<UMyrPhyCreatureMesh>(Hit.Component.Get())) 
			if(MyrOwner()->CurrentState != EBehaveState::held)
				if (!IsBumpable())
					SetPhyBodiesBumpable(true);
	}


	//поврежденную часть тела заставить висеть 
	//PhyDeflect(Limb, Limb.Damage);

	//при достаточно сильном ударе членик зафиксируется как подбитый и больше не принимает спам ударов до следующего кадра
	if (Severity > 0.3) Limb.LastlyHurt = true;
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
	auto ge = MachineGene(Limb.WhatAmI);
	if (bi < 255)
	{	
		//максимальная угловая скорость, или из генофонда, или, если там ноль - по умолчанию
		Bodies[bi]->SetMaxAngularVelocityInRadians(ge->MaxAngularVelocity>0 ? ge->MaxAngularVelocity : MaxAngularVeloc, false);
	}
}

//==============================================================================================================
//вычислить предпочительное направление движения для этого членика по его опоре
//==============================================================================================================
FVector UMyrPhyCreatureMesh::CalculateLimbGuideDir(FLimb& Limb, const FVector& PrimaryCourse, bool SupportOnBranch, float* Stability)
{
	//опорный курс, с которым сравнивать новый для вычисления приоритетности этой ноги
	FVector RightCourse = PrimaryCourse;

	//накопитель направления, разные случаи плюют сюда свои курсы
	FVector PrimaryGuide = PrimaryCourse;

	//если опора крутая, забраться на нее непросто, может, и не нужно, ибо препятствие
	if (Limb.ImpactNormal.Z < 0.5)
	{
		//степень упирания в препятствие и невозможность скользнуть в сторону
		float BumpInAmount = FVector::DotProduct(-Limb.ImpactNormal, PrimaryGuide);

		//превращаем степень ортогональности в степень отклонения в глобальную вертикаль
		//максимально просто, так как всё равно проецировать на плоскость нормали
		PrimaryGuide.Z += BumpInAmount;

		//подменяем опорный курс, чтобы при движении в вертикали лучше чувствовать отклонения в ногах
		RightCourse = PrimaryGuide;
	}

	//если стоим на цилиндрической поверхности - направить вдоль (неважно, карабкаемся или просто идем)
	//если это ствол вверх, необходимый разворот (перед-верх) уже был сделан выше
	if(Limb.OnCapsule())
	{
		//пояс
		auto Girdle = MyrOwner()->GetGirdle(Limb.WhatAmI);

		//толщина самой узкой метрики опоры
		const float Curvature = FMath::Max(Girdle->Curvature, 0.0f);

		//направление вперед строго вдоль ветки, без учёта склона
		const FVector BranchDir = Limb.GetBranchDirection (PrimaryGuide);

		//если не нужно съёзживать вверх, то выдать направление воль ветки как финальное
		if(!SupportOnBranch)
		{	if(Stability) *Stability = FVector::DotProduct(BranchDir, PrimaryCourse);
			if (BranchDir.ContainsNaN())
				Log(Limb, TEXT("WTF CalculateLimbGuideDir::BranchDir contains nan"));
			return BranchDir;
		}

		//просто касательная в бок - пока неясно, в какую сторону, множители это уточнят
		FVector BranchTangent = (BranchDir ^ Limb.ImpactNormal);

		//множитель касательной: куда смотрим, в ту сторону и вытягивать
		float WantToTurn = MyrOwner()->AttackDirection | BranchTangent;

		//множитель касательной: где вверх к седлу ветки, туда и вытягиваем
		float WantToSupport = (Limb.ImpactNormal.Z > 0) ? BranchTangent.Z : FMath::Sign(BranchTangent.Z);

		//выбрать, насколько мы хотим вилять в стороны, а насколько - стремиться занять безопасный верх седла
		//чем более пологая ветка, тем важнее удержаться на седловой линии, чем более крутая, тем важнее вертеться вокруг
		//тут пока неясно, стоит ли привлекать дополнительные зависимости вроде толщины ветки
		float TangentPreference = FMath::Max(Limb.ImpactNormal.Z + Curvature, 0.0f);
		float TangentMult = FMath::Lerp(WantToTurn, WantToSupport, TangentPreference);

		//финальное поддерживающее направление - простая сумма, но касательная вбок может стать нулевой
		PrimaryGuide = BranchDir + BranchTangent * TangentMult;

		//отладка с рисованием направы ветки и направы как не соскользнуть
		FVector Ct = GetMachineBody(Limb)->GetCOMPosition();

	}
	//общий случай - спроецировать на плоскость нормали, чтобы не давило и не отрывало от поверхности
	else PrimaryGuide = FVector::VectorPlaneProject(PrimaryGuide, Limb.ImpactNormal);

	//степень сходности (неотклонения) получившегося курса от изначально желаемого направления
	if (Stability)
	{	*Stability = FVector::DotProduct(PrimaryGuide, RightCourse);
		if (*Stability < 0) *Stability = 0;
	}

	//это ненормированный полуфабрикат, в поясе будет суммироваться со второй ногой, так что нормировать пока не надо
	if (PrimaryGuide.ContainsNaN())	Log(Limb, TEXT("WTF CalculateLimbGuideDir::PrimaryGuide contains nan"));
	return PrimaryGuide;
}
//==============================================================================================================
//принять или отклонить только что обнаруженную новую опору для этого членика
//==============================================================================================================
int UMyrPhyCreatureMesh::ResolveNewFloor(FLimb &Limb, FBodyInstance* NewFloor, FVector NewNormal, FVector NewHitLoc)
{
	//новая опора в результате удара не такая, как предыдуще зарегистрированная
	if (Limb.Floor != NewFloor)
	{
		//если тело уже контачит с другой опорой (стык в стык или не успело забыть)
		if (Limb.Stepped)
		{
/*			//пояс этой конечности
			auto& G = GetGirdle(Limb);

			//если пояс привязан
			if (G.FixedOnFloor)
			{
				//здесь будет случай, когда старая опора успела далеко отлететь по сравнению с новой
				//но он громоздок и может быть не нужен, раз проверка только в режиме зацепа
				//float DistOld, DistNew;
				//if(Limb.Floor->GetSquaredDistanceToBody(NewHitLoc) < NewFloor->GetSquaredDistanceToBody(NewHitLoc)*10)
				{
					//релевантность через путь кончика нормали, параллельного курсу движения
					FVector Devia = Limb.ImpactNormal - NewNormal;
					if (FVector::DotProduct(G.GuidedMoveDir, Devia) < 0)
					{
						Limb.Stepped = STEPPED_MAX;
						Log(Limb, TEXT("ResolveNewFloor Discard "), *NewFloor->OwnerComponent->GetName());
						return -1;
					}
				}
			}else*/

			//особый случай, когда уже действующая опора пытается смениться на нелазибельное припятствие
			if (NewFloor->OwnerComponent.Get())
				if (NewFloor->OwnerComponent->CanCharacterStepUpOn == ECanBeCharacterBase::ECB_No)
				{
					Limb.Stepped = STEPPED_MAX;
					Log(Limb, TEXT("ResolveNewFloor Discard cuz not steppable "), *NewFloor->OwnerComponent->GetName());
					return -1;
				}
		}
		//произошло касание ранее находящейся в воздухе части тела
		else
		{
			//для не ног сообщить наверх, что произошёл импакт
		}

		//если тело уже не контачило, но старый пол схранился - отбросить его по умолчанию
		//также эта ветка, если при сравнении нормалей новая опора всё же нужнее
		Limb.Floor = NewFloor;
		Limb.ImpactNormal = NewNormal;
		Limb.Stepped = STEPPED_MAX;
		return 1;
	}

	//новая опора та же, что и на прошлом касании - обновить только нормаль
	Limb.ImpactNormal = NewNormal;
	Limb.Stepped = STEPPED_MAX;
	return 0;
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
		//тело
		auto BI = GetMachineBody(Limb);

		//если включена физика (для кинематического режима это все скипнуть)
		if(BI->bSimulatePhysics)
		{
			//запускаем на этот кадр доп-физику исключительно для членика-поддержки
			//специально до обработки счетчиков и до ее избегания, чтобы держащий ношу членик тоже обрабатывался
			BI->AddCustomPhysics(OnCalculateCustomPhysics);

			//нужно только для хвоста
			if (HasFlesh(Limb.WhatAmI))
			{	BI = GetFleshBody(Limb, 0);
				if(BI->BodySetup->PhysicsType != EPhysicsType::PhysType_Kinematic)
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
				//относительная скорость нужна здесь только для детекции потери опоры
				FVector RelativeVelocity = GetMachineBody(Limb)->GetUnrealWorldVelocity();
				if (Limb.Floor)
				{ 	
					//особый случай, выглядит как ошибка, но такое может быть при съеденной добыче
					if (!Limb.Floor->OwnerComponent.Get())
					{	Limb.EraseFloor();
						return;
					}
					//относительную скорость вычисляем только при контакте с мобильным предметом
					else if (Limb.Floor->OwnerComponent.Get()->Mobility == EComponentMobility::Movable)
						RelativeVelocity -= Limb.Floor->GetUnrealWorldVelocityAtPoint(GetMachineBody(Limb)->GetCOMPosition());

				}
				else Log(Limb, TEXT("ProcessLimb WTF no floor component! "), (float)Limb.Stepped);

				//вектор сдвига в пространстве
				FVector Shift = RelativeVelocity * DeltaTime;
				float Drift = Shift.SizeSquared();

				//для ног делается еще ступень
				bool IsFoot = ((int)Limb.WhatAmI >= (int)ELimb::L_ARM && (int)Limb.WhatAmI < (int)ELimb::TAIL);

				//плавное снижение желания отдернуться от препятствия (зависит от скорости - стоит ли извлекать корень или оставить бешенные нули?)
				//возможно, брать только проекцию скорости на нормаль
				int NewStepped = Limb.Stepped;
				if (Drift > 100)	{ NewStepped -= 3; } else
				if (Drift > 10)		{ NewStepped -= 2; } else
				if (Drift > 0.1)	{ NewStepped -= 1; } else
			
				//для ног - при любой скорости минусовать (но до 1), поправка трассировкой все исправит
				if(IsFoot) NewStepped -= 1; 
				if (NewStepped > 0)	Limb.Stepped = NewStepped;
				else if (IsFoot) Limb.Stepped = 1;

				//если тело не нога в этот момент будет в воздухе - полностью отдернуть его от поверхности, забыть данные о ней
				else Limb.EraseFloor();
			}
		}

		//постепенное исцеление от урона (за счёт сокращения здоровья)
		if (Limb.Damage > 0)
		{	Limb.Damage = Limb.Damage - DeltaTime * HealRate;
			if (Limb.Damage <= 0.0f) Limb.Damage = 0.0f;
			else MyrOwner()->Health -= DeltaTime * HealRate;
		}
		LINELIMB(ELimbDebug::LimbAxisX, Limb, GetLimbAxisForth(Limb) * 20);
		LINELIMB(ELimbDebug::LimbAxisY, Limb, GetLimbAxisRight(Limb) * 20);
		LINELIMB(ELimbDebug::LimbAxisZ, Limb, GetLimbAxisUp(Limb) * 20);
		LINELIMB(ELimbDebug::LimbNormals, Limb, Limb.ImpactNormal * 10);

		//отладка сил, накопленных в констрейнтах
		if ((LDBG(ELimbDebug::LimbConstrForceLin) || LDBG(ELimbDebug::LimbConstrForceAng)))
		{
			if (GetMachineConstraintIndex(Limb) == 255) return;
			auto CI = Constraints[GetMachineConstraintIndex(Limb)];
			FVector LiF, AnF;
			CI->GetConstraintForce(LiF, AnF);
			LINELIMB(ELimbDebug::LimbConstrForceLin, Limb, LiF * 0.001);
			LINELIMB(ELimbDebug::LimbConstrForceAng, Limb, AnF * 0.001);
		}
	}

	//регулировать веса анимации плоти (?)
	//ProcessBodyWeights(Limb, DeltaTime);
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
	FVector aF(1, 0, 0);
	FVector aR(0, 1, 0);

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
	CI->Pos1 = FVector(0); CI->PriAxis1 = aF; CI->SecAxis1 = aR;
	BodyToFrame(CarrierBody, Pri, Sec, At, CI->PriAxis2, CI->SecAxis2, CI->Pos2);
	CI->InitConstraint(GrabbedBody, CarrierBody, 1.0f, CarrierBody->OwnerComponent.Get());

	//местные флаги
	Limb.Grabs = true;
	Limb.Floor = GrabbedBody;
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
	auto ToRet = Limb.Floor->OwnerComponent.Get();
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

	//присвоение флагов - они оживают динамически каждый кадр
	Limb.DynModel = Model;


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
			C->SetLinearPositionDrive(false, false, false);
		}
		//внимание, без этого не берет труп в зубы, в то же время с этим собаки реалистичнее мертвы
		//хз, сделать переключение по массе или спец-настройку
		//SetPhyBodiesBumpable(false);
	}
	//сделать напряженным
	else
	{
		for (int i=0; i<Constraints.Num(); i++)
			Constraints[i]->ProfileInstance = GetArchConstraint(i)->ProfileInstance;
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

//==============================================================================================================
//точка нижнего седла сегмента ветки - пока неясно, в каком классе стоит эту функцию оставлять: меша, веткаи или какого другого
//она просто вычисляет коррдинаты, куда телепортировать точку отсчёта кошки (низ-зад)
//==============================================================================================================
bool UMyrPhyCreatureMesh::GetBranchSaddlePoint(FBodyInstance* Branch, FVector& Pos, float DeviaFromLow, FQuat* ProperRotation, bool UsePreviousPos)
{
	//направление вдоль ветки вверх, неясно, почему У, но так работает
	auto DirAlong = GetBranchUpDir(Branch);
	if (DirAlong.Z < 0) DirAlong = -DirAlong;
	if (DirAlong.Z > 0.7) return false; //на очень крутые векти пока не телепортировать

	//направлени в сторону горизонта в сторону (вниманин, не нормировано)
	FVector ToHorizon = DirAlong ^ FVector::UpVector;

	//направление на седло, на точку равновесия наклонной капсулы
	FVector ToSaddle = ToHorizon ^ DirAlong;
	if (ToSaddle.Z < 0) ToSaddle = -ToSaddle;
	ToSaddle.Normalize();

	//здесь должно быть уже ясно, что капусла есть, капсула одна и капсула правильно вывернута
	auto& G = Branch->GetBodySetup()->AggGeom.SphylElems[0];

	//за одно, если надо, создать вращатор, соответствующий правильной посадке на седло ветки
	if (ProperRotation)
	{
		//исходя из направления вверх (нормаль в высшей точке) и вперед (собственно, направдение ветки
		*ProperRotation = FRotationMatrix::MakeFromZX(ToSaddle, DirAlong).ToQuat();
	}

	//использовать переменную Пос как вход, где указано текущее положение существа
	if(UsePreviousPos)
	{
		//длина проекции = положение на длине ветки, если ноль - висимо четко посередине касулы над ее центром масс
		//если же сильно больше длины, то это мы далеко вне ветки
		float Devia = FVector::DotProduct(Pos - Branch->GetCOMPosition(), DirAlong);
		if(FMath::Abs(Devia) > G.Length*1.5) return false;
		else Pos = Branch->GetCOMPosition() + DirAlong*FMath::Clamp(Devia,-G.Length,+G.Length) + ToSaddle*G.Radius;
		UE_LOG(LogTemp, Log, TEXT("%s: GetBranchSaddlePoint - pos %g"), *Branch->OwnerComponent->GetOwner()->GetName(), Devia);
	}
	else Pos = Branch->GetCOMPosition() - DirAlong*G.Length*(0.5 - DeviaFromLow) + ToSaddle * G.Radius;

	//length - это длина без пупыря, так что радиус отнимать не нужно
	
	return true;
}



