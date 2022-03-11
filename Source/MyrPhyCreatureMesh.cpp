// Fill out your copyright notice in the Description page of Project Settings.


#include "MyrPhyCreatureMesh.h"
#include "AssetStructures/MyrCreatureBehaveStateInfo.h"	// данные текущего состояния моторики
#include "Artefact/MyrArtefact.h"						// неживой предмет
#include "../MyrraGameInstance.h"						// ядро игры
#include "MyrPhyCreature.h"								// само существо
#include "PhysicsEngine/PhysicsConstraintComponent.h"	// привязь на физдвижке
#include "AssetStructures/MyrCreatureGenePool.h"		// генофонд
#include "DrawDebugHelpers.h"							// рисовать отладочные линии
#include "PhysicsEngine/PhysicsConstraintTemplate.h"	// для разбора списка физ-связок в регдолле
#include "../Dendro/MyrDendroMesh.h"					// деревья - для поддержания лазанья по ветке
#include "PhysicalMaterials/PhysicalMaterial.h"			// для выкорчевывания материала поверхности пола
#include "Engine/SkeletalMeshSocket.h"					

//имена кривых, которые должны быть в скелете и в анимации, чтобы детектировать незапланированные шаги
FName UMyrPhyCreatureMesh::StepCurves[(int)ELimb::NOLIMB] = { NAME_None, NAME_None, NAME_None, NAME_None, NAME_None, TEXT("LeftArm_Step"), TEXT("RightArm_Step"), TEXT("LeftLeg_Step"), TEXT("RightLeg_Step"), NAME_None };

//к какому поясу конечностей принадлежит каждая из доступных частей тела - для N(0) поиска
uint8 UMyrPhyCreatureMesh::Section[(int)ELimb::NOLIMB] = {0,0,1,1,1,1,1,0,0,0};
ELimb UMyrPhyCreatureMesh::Parent[(int)ELimb::NOLIMB] = { ELimb::PELVIS, ELimb::PELVIS, ELimb::LUMBUS, ELimb::PECTUS, ELimb::THORAX, ELimb::THORAX, ELimb::THORAX, ELimb::PELVIS, ELimb::PELVIS, ELimb::PELVIS };
ELimb UMyrPhyCreatureMesh::DirectOpposite[(int)ELimb::NOLIMB] = { ELimb::THORAX, ELimb::PECTUS, ELimb::LUMBUS, ELimb::PELVIS, ELimb::TAIL, ELimb::R_ARM, ELimb::L_ARM, ELimb::R_LEG, ELimb::L_LEG, ELimb::HEAD };

//перевод из номера члена в его номер как луча у пояса конечностей
EGirdleRay UMyrPhyCreatureMesh::Ray[(int)ELimb::NOLIMB] = { EGirdleRay::Center, EGirdleRay::Spine, EGirdleRay::Spine, EGirdleRay::Center, EGirdleRay::Tail, EGirdleRay::Left, EGirdleRay::Right, EGirdleRay::Left, EGirdleRay::Right, EGirdleRay::Tail };

//полный перечень ячеек отростков для всех двух поясов конечностей
ELimb UMyrPhyCreatureMesh::GirdleRays[2][(int)EGirdleRay::MAXRAYS] =
{ {ELimb::PELVIS,		ELimb::L_LEG,	ELimb::R_LEG,	ELimb::LUMBUS,	ELimb::TAIL},
	{ELimb::THORAX,		ELimb::L_ARM,	ELimb::R_ARM,	ELimb::PECTUS,	ELimb::HEAD}
};

#define STEPPED_MAX 15

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

	//инициализировать и запустить пояса конечностей
	InitGirdle (gPelvis, true);
	InitGirdle (gThorax, true);
	SetPhyBodiesBumpable (true);

	Super::BeginPlay();

}

//==============================================================================================================
//каждый кадр
//==============================================================================================================
void UMyrPhyCreatureMesh::TickComponent (float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//проверки для отсечения всяких редакторных версий объекта, чтоб не тикало
	if (TickType == LEVELTICK_ViewportsOnly) return; //только это помогает
	if (Bodies.Num() == 0) return;
	FrameCounter++;

	//для перерассчёта устойчивости по двум поясам конечностей
	float NewStandHardness[2] = {0,0};

	//обработать всяческую жизнь внутри отдельных частей тела (их фиксированное число)
	//почему это не перенесено в обработку поясов?
	for (int i = 0; i < (int)ELimb::NOLIMB; i++)
		ProcessLimb (GetLimb((ELimb)i), DeltaTime, NewStandHardness);

	//обновление поясоспецифичных коэффициентов устойчивости
	gPelvis.StandHardness = NewStandHardness[0];
	gThorax.StandHardness = NewStandHardness[1];

	//просчитать кучу всего для поясов конечностей
	ProcessGirdle (gPelvis, gThorax, DeltaTime, 1);
	ProcessGirdle (gThorax, gPelvis, DeltaTime, -1);

	DrawLimbAxis(Thorax, MyrOwner()->MoveDirection * 100, FColor(55, 0, 55));
	DrawLimbAxis(Thorax, MyrOwner()->AttackDirection * 100, FColor(55, 55, 55));

	//FVector RootBodyPos = GetComponentLocation();
	//for (auto B : Bodies)
	//	DrawDebugLine(GetOwner()->GetWorld(), B->GetCOMPosition(), B->GetCOMPosition() + FVector::RightVector * 100 * B->PhysicsBlendWeight, FColor(0, 100, 0), false, 0.01f, 100, 1);
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
	auto& G = GetGirdle(Limb);

	//общая инфа по конечности для данного вида существ
	const auto LimbGene = MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Type).Machine;

	//для простого учета развернутости осей (возможно, не нужен, так как ForcesMultiplier можно задать отрицательным
	float Revertor = (Limb.DynModel & LDY_REVERSE) ? -1.0f : 1.0f;

	//применить поддерживающую силу (вопрос правильной амплитуды по-прежнему стоит)
	float Power = G.CurrentDynModel->ForcesMultiplier;

	//исключить вызов дополнительной тянулки-вертелки, если "по курсу" включено, а дополнительной оси не задано
	bool OnlyOrient = true;

	//если указано что-то делать в сторону курса, то это режим управлеямого игроком движения
	if ((Limb.DynModel & (LDY_MY_WHEEL)))
	{	Move(BodyInst, Limb);
		OnlyOrient = false;
	}

	//если указано что-то ориентировать по направлению курса
	if((Limb.DynModel & LDY_TO_COURSE) && OnlyOrient)
	{	FVector GloAxis = G.GuidedMoveDir * Revertor;
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
		FVector GloAxis = (Limb.Stepped ? Limb.ImpactNormal : GetGirdleLimb(G, EGirdleRay::Center).ImpactNormal) * Revertor;
		OrientByForce(BodyInst, Limb, CalcAxisForLimbDynModel(Limb, GloAxis), GloAxis, Power);
	}

	//если указана ось локальная, но не указана ось глобальная - особый режим (кручение вокруг себя)
	else if ((Limb.DynModel & LDY_MY_AXIS) && !(Limb.DynModel & LDY_TO_AXIS))
	{	if (Limb.DynModel & LDY_ROTATE)		BodyInst->AddTorqueInRadians	( CalcAxisForLimbDynModel(Limb, FVector(0,0,0)) * Power * Revertor, false, true);
		else								BodyInst->AddForce				( CalcAxisForLimbDynModel(Limb, FVector(0, 0, 0)) * Power * Revertor, false, true);
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

	/*if ((int)eL >= (int)ELimb::L_ARM)
	{	int32 Index = (&LArmAxisSocket)[(int)eL - (int)ELimb::L_ARM];
		if (Index >= 0)
		{	USkeletalMeshSocket* Socket = SkeletalMesh->GetSocketByIndex(Index);
			if (Socket)
				return Socket->GetSocketLocation(this);
		}
	}*/
	return FVector();
}



//==============================================================================================================
//детекция степени утыкновения конкретным члеником в стену четко по направлению
//==============================================================================================================
float UMyrPhyCreatureMesh::StuckToStepAmount(const FLimb& Limb, FVector Dir) const
{
	//если членик ничего не касается, то он и ни на что не натыкается
	if (Limb.Stepped)
	{	
		//степень давления в опору, Dir - это направление, но может быть и скорость/сила, так было бы вернее
		float StuckAmount = FMath::Max(0.0f, Limb.ImpactNormal | (-Dir));

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
float UMyrPhyCreatureMesh::StuckToStepAmount(FVector Dir, ELimb& LevelOfContact) const
{
	//магнитуда копится по модулю, вердикт - со знаками (лезть или отстраняться)
	float Magnitude = 0.0f; float Verdict = 0.0f; LevelOfContact = ELimb::NOLIMB;

	//голова - самая болезненно реагирующая часть
	float NewProbe = StuckToStepAmount(Head, Dir);
	Magnitude += FMath::Abs(NewProbe); Verdict += NewProbe;
	LevelOfContact = ELimb::HEAD;
	if (Magnitude > 0.7) return Verdict;

	//передние ноги скопом, первые замечают невысокие препятствия
	NewProbe = 0.5 * (StuckToStepAmount(LArm, Dir) + StuckToStepAmount(RArm, Dir));
	Magnitude += FMath::Abs(NewProbe); Verdict += NewProbe;
	LevelOfContact = ELimb::R_ARM;
	if (Magnitude > 0.7) return Verdict;

	//грудь - это больше для человека актуально (возможно, ввести всё же 
	NewProbe = 0.5 * (StuckToStepAmount(Pectus, Dir) + StuckToStepAmount(Thorax, Dir));
	Magnitude += FMath::Abs(NewProbe); Verdict += NewProbe;
	LevelOfContact = ELimb::THORAX;
	if (Magnitude > 0.7) return Verdict;

	//задние ноги - это тоже больше для человека актуально 
	NewProbe = 0.5 * (StuckToStepAmount(LLeg, Dir) + StuckToStepAmount(RLeg, Dir));
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
			DrawMove(BodyInst->GetCOMPosition(), ActualAxis * 40, FColor(255, 0, 0));
			DrawMove(BodyInst->GetCOMPosition(), DesiredAxis * 50, FColor(255, 0, 0));
		}
	}

	//если требуется просто тянуть (это само по себе не ориентирование, но если есть привязи, они помогут)
	if (Limb.DynModel & LDY_PULL)
	{
		//модуль силы тащбы
		FVector Final = DesiredAxis * (1 - AsIdeal) * AdvMult;

		//если тело уже уткнуто, заранее обрезать силу
		if(Limb.Stepped)Final *= 1.0f - FMath::Max(0.0f, FVector::DotProduct(-Limb.ImpactNormal, DesiredAxis));

		//применить
		if (Final.ContainsNaN()) Log(Limb, TEXT("WTF OrientByForce contains nan"));
		else
		{	BodyInst->AddForce(Final, false, true);
			DrawMove(BodyInst->GetCOMPosition(), DesiredAxis * 0.3 * Limb.Stepped, FColor(255, 255, 255));
		}
	}
}


//==============================================================================================================
//применить двигательную силу к телу, полагая, что это тело является безосевым колесом а-ля шарик ручки
//==============================================================================================================
void UMyrPhyCreatureMesh::Move(FBodyInstance* BodyInst, FLimb& Limb)
{
	const auto& G = GetGirdle(Limb);
	//if(!G.CurrentDynModel->Leading) return;		//не двигать, если сам пояс конечностей пассивный
	if (MyrOwner()->MoveGain < 0.01) return;		//не двигать, если сам организм не хочет или не может

	//разность между текущим вектором скорости и желаемым (вектор курса * заданный скаляр скорости)
	//убрано, так как в тяге очень резко и нестабильно меняется курс
	//неясно, насколько это критично, надо вводить торможение по боковой скорости
	//также при FullUnderspeed кошка на колёсах не разворачивается под 90 грдусов, застывает
	//const FVector FullUnderspeed = G.GuidedMoveDir * MyrOwner()->GetDesiredVelocity() - G.VelocityAgainstFloor;

	//отрицательная обратная связь - если актуальная скорость меньше номинальной
	const float SpeedProbe = FVector::DotProduct(GetGirdle(Limb).VelocityAgainstFloor, G.GuidedMoveDir);
	const float Underspeed = MyrOwner()->GetDesiredVelocity() - SpeedProbe;
	if (SpeedProbe > -MyrOwner()->GetDesiredVelocity())
	{
		//дополнительное усиление при забирании на склон, включается только если в гору, чем круче, тем сильнее
		float UpHillAdvForce = FMath::Max(0.0f, G.GuidedMoveDir.Z) * MyrOwner()->BehaveCurrentData->UphillForceFactor;

		//угнетене движения, если наткнулись на препятствие
		if(Limb.Stepped)
			if (Limb.Floor->OwnerComponent.Get())
			{	if (Limb.Floor->OwnerComponent->CanCharacterStepUpOn == ECanBeCharacterBase::ECB_No)
					if (Limb.ImpactNormal.Z < 0.6)
						UpHillAdvForce *= 1.0f - FMath::Max(FVector::DotProduct(-Limb.ImpactNormal, G.GuidedMoveDir), 0.0f);
			}
			else Log(Limb, TEXT("WTF floor suddenly disappeared"));

		//вращательное усилие 
		if (Limb.DynModel & LDY_ROTATE)
		{
			//ось вращения - если на опоре, то через векторное произведение (и ведь работает!) если в падении - то прямо вокруг курса (для управления вращением)
			//надо тут ввести иф и два разных варианта для колеса и для вращения в воздухе
			FVector Axis = Limb.Stepped ? FVector::CrossProduct(Limb.ImpactNormal, G.GuidedMoveDir) : MyrOwner()->AttackDirection;

			//ось вращения с длиной в виде силы вращения
			FVector Torque;

			//режим ноги на опоре - движение колесами
			if(Limb.Stepped)
				Torque = 
					FVector::CrossProduct(Limb.ImpactNormal, G.GuidedMoveDir)	// из направления движения и нормали вот так легко извлекается ось вращения
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
				DrawMove(BodyInst->GetCOMPosition(), Torque, FColor(0, 25, 255));
			}
		}

		//поступательное усилие - только если касаемся опоры, ибо тяга, в отличие от вращения, будет искажать падение
		if ((Limb.DynModel & LDY_PULL) && Limb.Stepped)
		{
			if (G.StandHardness < 0.1) return;	
			FVector Direct = G.GuidedMoveDir							// направление хода
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
				DrawMove(BodyInst->GetCOMPosition(), Direct, FColor(0, 255, 255) );
			}
		}
	}
}

//==============================================================================================================
//совершить прыжок
//==============================================================================================================
void UMyrPhyCreatureMesh::PhyJump(FVector Horizontal, float HorVel, float UpVel)
{
	PhyPrance(gPelvis, Horizontal, HorVel, UpVel);
	PhyPrance(gThorax, Horizontal, HorVel, UpVel);
}

//==============================================================================================================
//встать не дыбы - прыгнуть всего одним поясом конечностей 
//==============================================================================================================
void UMyrPhyCreatureMesh::PhyPrance(FGirdle &G, FVector HorDir, float HorVel, float UpVel )
{
	//центральная часть тела пояса конечностей
	auto& CLimb = GetGirdleLimb(G, EGirdleRay::Center);

	//насколько главное тело пояса лежит на боку - если сильно на боку, то прыгать трудно
	float SideLie = FMath::Abs(GetLimbAxisRight(CLimb) | CLimb.ImpactNormal);
	float UnFall = 1.0 - SideLie;

	//особый режим - зажата кнопка "крабкаться", но опора не позволяет - всё равно встаем на дыбы и прыгаем почти точно вверх
	if(G.SeekingToCling && !G.Clung) HorDir *= 0.1;

	//суммарное направление прыжка
	FVector Dir = HorDir * HorVel + FVector::UpVector * (UpVel);

	//резко sотключить пригнутие, разогнуть ноги во всех частях тела
	G.Crouch = 0.0f;

	//применить импульсы к ногам - если лежим, то слабые, чтобы встало сначала туловище
	PhyJumpLimb ( GetGirdleLimb(G, EGirdleRay::Left), Dir * UnFall);
	PhyJumpLimb ( GetGirdleLimb(G, EGirdleRay::Right), Dir * UnFall);
	PhyJumpLimb ( GetGirdleLimb(G, EGirdleRay::Tail), Dir * UnFall);

	//применить импульсы к туловищу
	//ослабление, связанное с лежанием, к туловищу не применяется, чтобы мочь встать, извиваясь
	PhyJumpLimb ( GetGirdleLimb(G, EGirdleRay::Spine), Dir);
	PhyJumpLimb ( CLimb, Dir );

	//явно, не дожидаясь ProcessLimb, обнулить жесткость стояния, чтобы в автомате состояний поведения не переклинивало
	G.StandHardness = 0;
}


//==============================================================================================================
//телепортировать всё тело на седло ветки - пока вызывается в HitLimb, что, возможно, плохо, ибо не среагирует при застывании
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
		DrawMove(Pos, Rot.GetUpVector()*10, FColor(255, 30, 255), 10, 10);
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
		UntouchLimb(Limb);

		//тело
		auto BI = GetMachineBody(Limb);

		//отменить линейное трение, чтобы оно в воздухе летело, а не плыло как в каше
		BI->LinearDamping = 0;
		BI->UpdateDampingProperties();

		//применить скорость
		BI->AddImpulse(Dir, true);

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
// в ответ на внешнее включение желания залезть - подготовить тело к зацеплению
//==============================================================================================================
void UMyrPhyCreatureMesh::ClingGetReady(bool Set)
{
	//установить желание во всех поясах, ведущесть может меняться в процессе
	gPelvis.SeekingToCling = Set;
	gThorax.SeekingToCling = Set;

	//только если включаем зацеп
	if(Set)
	{	
		bool BeenClung = false;

		//если не стоим на ногах
		if (gThorax.StandHardness < 0.8 && gPelvis.StandHardness < 0.8)
		{
			//попытаться извиваться всем позвоночником в попытке встать
			MyrOwner()->SelfActionFindStart(ECreatureAction::SELF_WRIGGLE1);
		}
		else
		{
			//если мы не лежим на боку - прошерстить все части тела на предмет условий для зацепа - и зацепить
			for (int i = 0; i < (int)ELimb::NOLIMB; i++)
			{
				auto& Limb = GetLimb((ELimb)i);
				if (!Limb.Stepped) continue;

				//немедленный зацеп (переход в состояние BehaveState.climb - внутри)
				if (ClingLimb(Limb, false) <= EClung::Kept)	BeenClung = true;//◘#>
			}
			//если до этого не вышли из функции - значит, нет под руками поверхности - 
			//найти специальное самодействие, отражающее вставание на дыбы, чтобы зацепиться передними лапами за вертикаль
			if (!BeenClung)
			{
				//если тело наклонено вниз или ровно - подниматься на дыбы
				if (Erection() < 0.5)
					MyrOwner()->SelfActionFindStart(ECreatureAction::SELF_PRANCE1);

				//иначе встать на дыбы задом
				else MyrOwner()->SelfActionFindStart(ECreatureAction::SELF_PRANCE_BACK1);
			}
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

	//пояс, к которому принадлежит эта нога
	auto& G = GetGirdle(Limb);

	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	//если опора скелетная (дерево, другая туша) определить также номер физ-членика опоры
	auto NewFloor = Hit.Component.Get()->GetBodyInstance(Hit.BoneName);

	//сохранить тип поверхности для обнаружения возможности зацепиться
	auto FM = Hit.PhysMaterial.Get();
	if (FM) Limb.Surface = (EMyrSurface)FM->SurfaceType.GetValue();

	//результат для новой опоры: 0 - новая равна старой, 1 - новая заменила старую (надо перезагрузить зацеп), -1 - новая отброшена, старая сохранена
	const int NewFloorDoom = ResolveNewFloor(Limb, NewFloor, Hit.ImpactNormal, Hit.ImpactPoint);

	//если надо цепляться или обновить зацеп
	const auto ClungR = ClingLimb(Limb, (NewFloorDoom == 1));

	//если оказалось, что зацеп вообще или больше невозможен
	if (ClungR > EClung::Kept)
	{	if (Limb.Clung)
		{	UnClingLimb(Limb);
			UE_LOG(LogTemp, Log, TEXT("%s: UnClingLimb %s cause %s"), *GetOwner()->GetName(), *TXTENUM(ELimb, Limb.WhatAmI), *TXTENUM(EClung, ClungR));
		}
	}
	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№

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
	if (BumpSpeed > 1000)
		Log(Limb, TEXT("HitLimb WTF BumpSpeed "), BumpSpeed);
	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№

	//воспроизвести звук, если контакт от падения (не дребезг и не качение) и если скорость соударения... откуда эта цифра?
	//в вертикальность ноль так как звук цепляния здесь не нужен, только тупое соударение
	if (BumpSpeed > 10 && Limb.Stepped == 0)
		MyrOwner()->SoundOfImpact (nullptr, Limb.Surface, Hit.ImpactPoint, BumpSpeed, 0, EBodyImpact::Bump);


	//эксперимент (как только на капсуле и как только смена пола на капсульно-веточный
	if (Limb.OnCapsule() && G.SeekingToCling && WaitingForTeleport)
	{	TeleportOntoBranch(Limb.Floor);
		WaitingForTeleport = false;
	}

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

#if DEBUG_LINE_CONTACTS
	DrawMove(Hit.ImpactPoint, FVector(0), FColor(Limb.Stepped, 255, 0), 3);
#endif

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
	Thresh *= MyrOwner()->Params.GetBumpShockResistance();		// безразмерный коэффициент индивидуальной ролевой прокачки
	Thresh *= TerminalDamageMult;								// безразмерный коэффициент дополнительной защиты именно этой части тела

	//выдрать данные о поверхности, оно нужно далее
	auto SurfInfo = MyrOwner()->GetMyrGameInst()->Surfaces.Find(Limb.Surface);

	//обработка ушибов от сильных ударов
	//если даже 1/10 порога не достигнута, дальше вообще не считать
	if (InjureAtHit && BumpSpeed > Thresh * 0.1)
	{
		//убиваться можно только об твердые неподвижные предметы, чтоб не впускать косяков физдвижка при взаимодействии подвижных с подвижными
		if(Hit.Component->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic 
			/* || Hit.Component->GetCollisionObjectType() == ECollisionChannel::ECC_Pawn*/)
		{
			//удельная сила, через деление - для мягких диапазонов и неразрывности, может быть меньше 0, посему через Макс
			Severity += FMath::Max(0.0f, BumpSpeed / Thresh - 1);
			if (Severity > 0.0f)
			{	Limb.Damage += Severity;
				MyrOwner()->Hurt(Severity, Hit.ImpactPoint, Hit.ImpactNormal, Limb.Surface, SurfInfo);
			}
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
		MyrOwner()->Hurt (Severity, Hit.ImpactPoint, Hit.ImpactNormal, Limb.Surface, SurfInfo);
	}


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
			const auto DamageBase = MyrOwner()->GetCurrentAttackInfo()->TactileDamage;

			//нанесение урона только если наша атака увечная
			if(DamageBase > 0)
			{	
				//если жертва жива и если повреждаемый членик только что не был поврежден (для предотвращения дребезга)
				if(TheirMesh->MyrOwner()->Health > 0 && !TheirLimb->LastlyHurt)
				{
					//набираем факторов, влияющих на увечье
					float dD = DamageBase * MyrOwner()->Params.GetAttackStrength();

					//также если жертва сама атакует, ее атака может давать дополнительный щит
					if(TheirMesh->MyrOwner()->Attacking())
						dD -= TheirMesh->MyrOwner()->GetCurrentAttackInfo()->HitShield * TheirMesh->MyrOwner()->Params.GetAttackShieldStrength();

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
						TheirMesh->MyrOwner()->Hurt(dD, Hit.ImpactPoint, Hit.ImpactNormal, TheirLimb->Surface);	// визуально, звуково, общий урон
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
		}
		//если не хватание
		else
		{
			//прервать атаку, чтоб не распидорасить физдвижок от проникновения кинематической лапы в тело
			if(TheirMesh) MyrOwner()->AttackGoBack();
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
//рассчитать длины ног - для акнимации, но покая неясно, где вызывать
//==============================================================================================================
void UMyrPhyCreatureMesh::InitFeetLengths(FGirdle& G)
{
	//расчёт длин ног и рук
	if (G.HasLegs)
	{
		ELimb eL = GetGirdleLimb(G, EGirdleRay::Left).WhatAmI;
		FName TipName = FleshGene(eL)->GrabSocket;
		FVector Tip = TipName.IsNone() ? GetMachineBody(eL)->GetCOMPosition() : GetSocketLocation(TipName);
		FVector Root = GetLimbShoulderHubPosition(eL);
		G.LimbLength = FVector::Dist(Root, Tip);
	}
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
void UMyrPhyCreatureMesh::InitLimbCollision (FGirdle& G, EGirdleRay GR, float MaxAngularVeloc, FName CollisionPreset)
{
	//если присутствует элемент каркаса-поддержки
	FLimb& Limb = GetGirdleLimb(G, GR);
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
	//накопитель направления, разные случаи плюют сюда свои курсы
	FVector PrimaryGuide = PrimaryCourse;

	//если опора крутая, забраться на нее непросто, может, и не нужно, ибо препятствие
	if (Limb.ImpactNormal.Z < 0.5)
	{
		//степень упирания в препятствие и невозможность скользнуть в сторону
		float BumpInAmount = FVector::DotProduct(-Limb.ImpactNormal, PrimaryGuide);

		//другой отдельный случай - тело прицепилось к опоре и ползнёт/карабкается по ней вверх
		if (Limb.Clung)
		{
			//превращаем степень ортогональности в степень отклонения в глобальную вертикаль
			//максимально просто, так как всё равно проецировать на плоскость нормали
			PrimaryGuide.Z += BumpInAmount;

		}
	}

	//если стоим на цилиндрической поверхности - направить вдоль (неважно, карабкаемся или просто идем)
	//если это ствол вверх, необходимый разворот (перед-верх) уже был сделан выше
	if(Limb.OnCapsule())
	{
		//толщина самой узкой метрики опоры
		const float Curvature = FMath::Max(GetGirdle(Limb.WhatAmI).Curvature, 0.0f);

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
		DrawMove(Ct, BranchDir * 15, FColor(0, 0, 0));
		DrawMove(Ct, BranchTangent * WantToSupport * 15, FColor(0, 0, 0));

	}
	//общий случай - спроецировать на плоскость нормали, чтобы не давило и не отрывало от поверхности
	else PrimaryGuide = FVector::VectorPlaneProject(PrimaryGuide, Limb.ImpactNormal);

	//степень сходности (неотклонения) получившегося курса от изначально желаемого направления
	if(Stability) *Stability = FVector::DotProduct(PrimaryGuide, PrimaryCourse);

	//это полуфабрикат, в поясе будет суммироваться со второй ногой, так что нормировать пока не надо
	if (PrimaryGuide.ContainsNaN())
		Log(Limb, TEXT("WTF CalculateLimbGuideDir::PrimaryGuide contains nan"));
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
			if (Limb.Clung)
			{
				//здесь будет случай, когда старая опора успела далеко отлететь по сравнению с новой
				//но он громоздок и может быть не нужен, раз проверка только в режиме зацепа
				//float DistOld, DistNew;
				//if(Limb.Floor->GetSquaredDistanceToBody(NewHitLoc) < NewFloor->GetSquaredDistanceToBody(NewHitLoc)*10)
				{
					//релевантность через путь кончика нормали, параллельного курсу движения
					FVector Devia = Limb.ImpactNormal - NewNormal;
					if (FVector::DotProduct(GetGirdle(Limb).GuidedMoveDir, Devia) < 0)
					{
						Limb.Stepped = STEPPED_MAX;
						Log(Limb, TEXT("ResolveNewFloor Discard "), *NewFloor->OwnerComponent->GetName());
						return -1;
					}
				}
			}
			//особый случай, когда уже действующая опора пытается смениться на нелазибельное припятствие
			else if (NewFloor->OwnerComponent.Get())
				if (NewFloor->OwnerComponent->CanCharacterStepUpOn == ECanBeCharacterBase::ECB_No)
				{
					Limb.Stepped = STEPPED_MAX;
					Log(Limb, TEXT("ResolveNewFloor Discard cuz not steppable "), *NewFloor->OwnerComponent->GetName());
					return -1;
				}
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
void UMyrPhyCreatureMesh::ProcessLimb (FLimb& Limb, float DeltaTime, float* NewStandHardnesses)
{
	//общая инфа по конечности для данного вида существ
	const auto& LimbGene = *MachineGene(Limb.WhatAmI);

	//пояс, к которому принадлежит эта нога
	auto& G = GetGirdle(Limb);

	//сбросить флаг больности, допускающий лишь один удар по членику за один кадр
	Limb.LastlyHurt = false;

	//канал аккумулятора устойчивости, специфичный для пояса, будет заполняться здесь 
	float& StandHardness = NewStandHardnesses[Section[(int)Limb.WhatAmI]];

	//если с этой частью тела ассоциирован элемент каркаса/поддержки
	if (LimbGene.TipBodyIndex != 255)
	{
		//тело
		auto BI = GetMachineBody(Limb);

		//запускаем на этот кадр доп-физику исключительно для членика-поддержки
		//специально до обработки счетчиков и до ее избегания, чтобы держащий ношу членик тоже обрабатывался
		BI->AddCustomPhysics(OnCalculateCustomPhysics);

		//пока жесткая заглушка - если этот членик несёт физически кого-то, то он не принимает сигналы о касаниях
		//это чтобы сохранить неизменным значение Floor, в котором всё время ношения ссылка на несомый объект
		if (Limb.Grabs) return;		

		//идеальный образ настроек этого тела из ассета
		auto ABI = GetArchMachineBody(Limb);

		//если эта часть тела зацеплена, есть некоторые критерии, при которых ее все же надо отцепить
		if(Limb.Clung)
		{
			//обновить уровень прицепленности ибо переход от стояния к движению
			if (MyrOwner()->ExternalGain > 0)
				ClingLimb(Limb, false);

			//извне отжали зацеп (случай смены опоры на нецепкую проверяется в HitLimb)
			if (!G.SeekingToCling)
			{	UnClingLimb(Limb);
				Log(Limb, TEXT("UnClingLimb cause unwanted"));
			}
		}
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
				{	UntouchLimb(Limb);
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

			//плавное снижение желания отдернуться от препятствия (зависит от скорости - стоит ли извлекать корень или оставить бешенные нули?)
			int NewStepped = Limb.Stepped;
			if (Drift > 100)	{ NewStepped -= 3; } else
			if (Drift > 10)		{ NewStepped -= 2; } else
			if (Drift > 0.1)	{ NewStepped -= 1; }
			

			//тело чувствует опору - внесение вклада в определение устойчивости (пока только библиотечный коэффициент, нужен еще наклон нормали)
			if (NewStepped > 0)
			{	StandHardness += FMath::Abs(LimbGene.TouchImportance);
				Limb.Stepped = NewStepped;
			}
			//если тело в этот момент будет в воздухе - полностью отдернуть его от поверхности, забыть данные о ней
			else UntouchLimb(Limb);	
		}

		//постепенное исцеление от урона (за счёт сокращения здоровья)
		if (Limb.Damage > 0)
		{	Limb.Damage = Limb.Damage - DeltaTime * HealRate;
			if (Limb.Damage <= 0.0f) Limb.Damage = 0.0f;
			else MyrOwner()->Health -= DeltaTime * HealRate;
		}
#if DEBUG_LINE_AXES
	DrawLimbAxis(Limb, GetLimbAxisForth(Limb) * 10, FColor(255, 0, 0));
	DrawLimbAxis(Limb, GetLimbAxisRight(Limb) * 10, FColor(0, 255, 0));
	DrawLimbAxis(Limb, GetLimbAxisUp(Limb) * 10, FColor(0, 0, 255));
#endif

#if DEBUG_LINE_CONTACTS
	DrawLimbAxis(Limb, Limb.ImpactNormal * 10 *Limb.Stepped/((float)STEPPED_MAX), FColor(Limb.Stepped, 255, 0));
#endif
	}

	//регулировать веса анимации плоти (?)
	//ProcessBodyWeights(Limb, DeltaTime);
}

//==============================================================================================================
//прицепиться к опоре и сопроводить движение по опоре перезацепами
//==============================================================================================================
EClung UMyrPhyCreatureMesh::ClingLimb (FLimb& Limb, bool NewFloor)
{
	return EClung::CantCling;//временно














/*	//к какому поясу принадлежит данная конечность
	FGirdle& G = GetGirdle(Limb.WhatAmI);
	if ( !G.SeekingToCling )					return EClung::NoSeekToCling;	//◘◘> не приказано искать сцепления
	if ( !G.CurrentDynModel->Leading)			return EClung::NoLeading;		//◘◘> не поставлен флаг ведущий в этом поясе
	if ( !MachineGene(Limb.WhatAmI)->CanCling )	return EClung::CantCling;		//◘◘> явно прописано, что этой конечностью невозможно цепляться
	if ( !Limb.IsClimbable() )					return EClung::BadSurface;		//◘◘> поверхность, котрую касается эта конечность, не цеплябельна

	//если на входе нога, то противоположная конечность - нога с другой стороны
	FLimb OLimb = GetOppositeLimb(Limb);
	auto OBody = GetMachineBody(Limb);

	//если опора противоположной ноги отличаеся от опоры этой ноги - не создавать зацеп
	//или может, разорвать существующий? чтобы проползти на инерции
	//с другой стороны хотя бы у одной ноги зацеп нужен
	//может, вообще оставить зацеп только на одну ногу, самую релевантную
	if (OLimb.Floor && Limb.Floor != OLimb.Floor)
		return EClung::DangerousSpread;//◘◘>

	//всякие нужные указатели
	auto LBody = GetMachineBody(Limb);
	auto& CI = LBody->DOFConstraint;
	FVector At = LBody->GetCOMPosition();	// сюда, возможно, стоит не центр масс, а точку контакта
	FVector OldPos = FVector(0,0,-1000);	// абсурдная локация, чтобы при расчёте выдал дистанцию, заведомо поощряющую к обновлению
	float Luft = 10;						// квант беспечного перемещения - сюда бы диаметр колеса

	//если констрейнт еще не создан в памяти
	if (!CI) CI = PreInit(); 

	//пересоздача зацепа нужна только если изменились операнды, а именно - опора 
	//но опора может быть просто задета, и перепривязка может привести к дребезгу
	if (NewFloor)
	{	CI->TermConstraint();
		Log(Limb, TEXT("ClingLimb: New Floor, Terminate Old"));
	}
	//честный расчёт предыдущей зафиксированной точки зацепа в надежде избежать перестройки при здвиге на слишком маленькое расстояние
	else OldPos = Limb.Floor->GetUnrealWorldTransform().TransformPosition(CI->Pos2);

	//акт перехода OFF -> ON (по внешнему нажатию)
	if (Limb.Clung == false)
	{	
		//ЗАКОМЕНТАРЕНО,ИБО ЖЕСТКИЕ УСЛОВИЯ НЕ ДАЮТ ЗАЛЕЗАТЬ НА СОВСЕМ ВЕРТИКАЛЬ ТИПА СТВОЛА
		//исключаем случай, когда пытаемся уцепиться одной лапой - нет, только двумя!
		//FLimb OLimb = GetOppositeLimb(Limb);
		//if(OLimb.Stepped == 0) EClung::OneHandNotEnough;//◘◘>

		//исключаем случай, когда зацепиться можем, но нас так колбасит и под даким углом к доске, что лучше не цепляться
		//для этого - средняя нормаль в точке касания этой ноги и противоположной ноги
		//это вообще-то неправильно, потому что ненормирована, возможно, стоит брать единичную нормаль в точке
		FVector CharNorm = 0.5 * (Limb.ImpactNormal + OLimb.ImpactNormal);

		//если вектор из спины сильно отличается от нормали в зацепе, то спина как бы лежит и может перевернуться
		if(FVector::DotProduct(CharNorm,GetLimbAxisUp(GetGirdleLimb(G, EGirdleRay::Center))) < 0.5f)
			return EClung::BadAngle;//◘◘>



		//сразу переключить состояние, применить физ-модель
		MyrOwner()->AdoptBehaveState(EBehaveState::climb);	

		//пружина вверх-вниз и влево-вправо
		CI->SetLinearPositionDrive(true, false, true);	

		//силы пружин и вязкости - немного, слабенько, чтоб не распидорашивало
		CI->SetLinearDriveParams(1000, 0, 70000);		

		//укон равновесия пружин - оттянуть вниз, чтобы прилипало к поверхности с некоторой силой
		CI->SetLinearPositionTarget(FVector(-Luft, 0, 0));

		//пометить, что зацеп состоялся
		G.Clung = Limb.Clung = true;
		Log(Limb, TEXT("ClingLimb: Turn On Constraint"));

		//реальный лимиты на движение в стороны от точки посадки 
		//чем более выпуклая поверхность, тем меньше свободы, видимо, чтоб не проваливаться влево-вправо
		//чем более вертикальная поверхность тем больше люфт - чтобы на горизонтальной тонкой ветке поддерживать строгую траекторию вдоль?
		float LeftRightFreedom = Luft * FMath::Max(0.0f, 2 * G.GuidedMoveDir.Z - G.Curvature);
		SetFreedom(CI, -1, -1, LeftRightFreedom, 180, 180, 180);	

		// задать абсурдное удаление, чтобы обновилось обязательно
		OldPos = FVector(0,0,-1000);						
	}


	//если не вышли ранее - происходит бессобытийная рутина - есть шанс, что мы не сдвинулись, и потому не надо перестраивать зацеп
	//разность от предыдущего значения
	float Devia = FVector::DistSquared(At, OldPos);
	EClung PreResult = EClung::Kept;

	//если разность от последней точки оновления подцепа превышает некий порог (тут эксперименты)
	if (Devia > Luft * Luft * 0.1)
	{
		//перезацепить на новую точку
		Log(Limb, TEXT("ClingLimb: Renew Constraint, because moved on "), Devia);
		PreResult = MakeConstraint(CI, Limb.ImpactNormal, FVector::VectorPlaneProject(G.GuidedMoveDir, Limb.ImpactNormal), At, LBody, Limb.Floor);//◘◘>
	}

	//включение и обновление могут по любому соотноситься с тягой/стопом, каждый событийный вариант приводит к перестройке зацепа
	if(IsClingAtStop(CI))
	{
		//зацеп на стопе, но игрок жмет газ - освободить
		if(MyrOwner()->ExternalGain > 0)
		{	
			//неясно, стоит ли ограничивать размах влево-вправо жестко, или ограничиться силами моторов. Возможно, оставить что-то больше нуля
			CI->SetLinearVelocityDrive(true, false, true);
			float LeftRightFreedom = Luft * FMath::Max(0.0f, 2 * G.GuidedMoveDir.Z - G.Curvature);
			SetFreedom(CI, -1, -1, LeftRightFreedom, 180, 180, 180);
			Log(Limb, TEXT("ClingLimb: Keep/Unbrake"));
			return MakeConstraint (CI, Limb.ImpactNormal, FVector::VectorPlaneProject(G.GuidedMoveDir, Limb.ImpactNormal), At, LBody, Limb.Floor);//◘◘>
		}
	}
	//если зацеп не на тормозе, но игрок прекратил подавать газ - затормозить, чтоб не скатиться
	else if(MyrOwner()->ExternalGain <= 0)
	{
		CI->SetLinearVelocityDrive(false, true, false);
		SetFreedom(CI, 0, 0, -1, 180, 180, 0);
		Log(Limb, TEXT("ClingLimb: Keep/Brake"));
		return MakeConstraint (CI, Limb.ImpactNormal, FVector::VectorPlaneProject(G.GuidedMoveDir, Limb.ImpactNormal), At, LBody, Limb.Floor);//◘◘>
	}
	
	//UE_LOG(LogTemp, Log, TEXT("%s: ClingLimb %s no need %g"), *GetOwner()->GetName(), *TXTENUM(ELimb, Limb.WhatAmI), Devia);
	return PreResult;//◘◘>*/
}

//==============================================================================================================
//отцепиться
//==============================================================================================================
void UMyrPhyCreatureMesh::UnClingLimb(FLimb& Limb)
{
/*	auto LBody = GetMachineBody(Limb);
	auto& CI = LBody->DOFConstraint;
	if (CI)
	{
		//расслабляем все параметры зацепа, но сам зацеп пока не рвём (зачем?)
		Limb.Clung = false;
		CI->SetLinearPositionDrive(false, false, false);
		CI->SetLinearVelocityDrive(false, false, false);
		CI->TermConstraint();
		DrawAxis(LBody->GetCOMPosition(), FVector(0), FColor(0, 0, 0), 1, 100);

		//проверяем, как с другими членами пояса - если этот разрыв последний, отменить глобальный режим карабканья
		auto& G = GetGirdle(Limb);
		if (!GetGirdleLimb(G, EGirdleRay::Center).Clung
			&& !GetGirdleLimb(G, EGirdleRay::Right).Clung 
			&& !GetGirdleLimb(G, EGirdleRay::Left).Clung)
		{	G.Clung = false;
			MyrOwner()->AdoptBehaveState(EBehaveState::fall);
		}
	}*/
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

	//сообщить наверх и далее, если игрок, на виджет, чтоб он отметил на экране худ
	MyrOwner()->WidgetOnGrab(GrabbedBody->OwnerComponent->GetOwner());
}

//==============================================================================================================
// внешне обусловленное сбрасывание из нашей руки или пасти то, что она держала 
//==============================================================================================================
void UMyrPhyCreatureMesh::UnGrab (FLimb& Limb)
{
	//на уровне нашего тела
	if(!Limb.Grabs) return;
	auto LBody = GetMachineBody(Limb);
	auto& CI = LBody->DOFConstraint;
	CI->TermConstraint();
	Limb.Grabs = false;

	//установить, что мы держали, если тело существа
	if(auto ItsCreatureMesh = Cast<UMyrPhyCreatureMesh>(Limb.Floor->OwnerComponent.Get()))
	{
		//всё отпущенное тело привести к чувствительности к столкновениям
		//что вообще неправильно, так как надо сначала дать ему выйти из нашего объёма
		//а так он непонятно, куда улетит. Но для этого нужно обрабатывать перекрытия, а это лишняя сущность
		AMyrPhyCreature* Victim = ItsCreatureMesh->MyrOwner();
		//Victim->GetMesh()->SetPhyBodiesBumpable(true);

		//отпустить тело, явно задав ему дальейшее поведение (его надо тоже обнулить по всем конечностям)
		if(Victim->Health > 0) Victim->AdoptBehaveState(EBehaveState::fall);
		else Victim->AdoptBehaveState(EBehaveState::dying);
		UntouchLimb(Limb);
	}
}
void UMyrPhyCreatureMesh::UnGrabAll ()
{
	for (int i = 0; i < (int)ELimb::NOLIMB; i++)
		UnGrab(GetLimb((ELimb)i));
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
//инициализировать пояс конечностей при уже загруженной остальной физике тела (ноги есть/нет, DOF Constraint)
//==============================================================================================================
void UMyrPhyCreatureMesh::InitGirdle (FGirdle& Girdle, bool StartVertical)
{
	//если в физической сборке-каркасы присутствует хотя бы один физ-членик для ног этого пояса
	if (GetMachineBodyIndex(GetGirdleRay(Girdle, EGirdleRay::Right)) < 255)
	{	Girdle.HasLegs = true;
		InitLimbCollision(Girdle, EGirdleRay::Left, 100000);
		InitLimbCollision(Girdle, EGirdleRay::Right, 100000);
	}
	else Girdle.HasLegs = false;
	InitLimbCollision(Girdle, EGirdleRay::Center, 100);
	InitLimbCollision(Girdle, EGirdleRay::Spine, 100);
	InitLimbCollision(Girdle, EGirdleRay::Tail, 100);

	//начать свободным в пространстве
	Girdle.FixedOnFloor = false;

	//сразу создать "гироскоп" для этого пояса в виде по умолчанию
	auto CBody = GetGirdleBody(Girdle, EGirdleRay::Center);
	auto& CI = CBody->DOFConstraint;
	CBody->DOFConstraint = PreInit();
	CI->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
	CI->PriAxis1 = MyrAxes[(int)MBoneAxes().Up];
	CI->SecAxis1 = MyrAxes[(int)MBoneAxes().Forth];
	CI->PriAxis2 = FVector(0, 0, 1);
	CI->SecAxis2 = FVector(1, 0, 0);
	CI->Pos2 = CBody->GetUnrealWorldTransform().GetLocation();

	//2 последних false - значит не будут для осей 1 пересчитываться трансформации - они уже впендюрены правильные
	MakeConstraint(CI, CI->PriAxis2, CI->SecAxis2, CI->Pos2, CBody, nullptr, false, false);
}

//==============================================================================================================
//обновить "перёд" констрейнта центрального членика (ограничение на уклон от вертикали или на юление влево-вправо)
//==============================================================================================================
void UMyrPhyCreatureMesh::UpdateCentralConstraintFrames(FGirdle& G)
{
	//взять сначала физ-тело, потом из неё встроенную привязь, уже до этого инициализированную!
	auto  CBody = GetGirdleBody(G, EGirdleRay::Center);
	auto& CI = CBody->DOFConstraint;

	//фиксация по месту прикрепления
	if (G.FixedOnFloor)
	{
		//центральный членик
		auto  CLimb = GetGirdleLimb(G, EGirdleRay::Center);

		

		//вторичная ось - направление движения вычисленное заранее из ног
		CI->SecAxis2 = G.GuidedMoveDir;
	}
	//фиксация в абсолютной вертикали + нужном направлении "вперед"
	else
	{
		//первый фрейм - просто эквиваленты осей в коодинатах тела
		//не переприсваиваются, потому что тело всегда ориентируется своими главными осями и всегда от своего центра
		//хотя насч1т центра может быть стоит его менять при фиксации к опоре
		//CI->PriAxis1 = MyrAxes[(int)MBoneAxes().Up];
		//CI->SecAxis1 = MyrAxes[(int)MBoneAxes().Forth];
		//CI->Pos2 = FVector(0);

		//первичная ось - абсолютная вертикаль мира, это нужно для фиксации вертикали
		CI->PriAxis2 = FVector(0, 0, 1);

		//вторичная ось - направление движения непосредственно из камеры
		CI->SecAxis2 = MyrOwner()->MoveDirection;

		//непонятно, нужна ли ортогональность на этом месте, но в случае явной неортогональности выгнуть и нормировать
		if (MyrOwner()->BehaveCurrentData->bOrientIn3D)
		{	CI->SecAxis2.Z = 0;
			CI->SecAxis2.Normalize();		}

		//помещаем сердце зацепа в центр центрального членика, это проще всего и в данном режиме ни на что не влияет
		//а вообще здесь должно быть то место, где констрейнт хочет иметь данное тело
		CI->Pos2 = CBody->GetUnrealWorldTransform().GetLocation();
	}

	//выкорчёвывается из исходников способ обращения с нижележащим слоем абстракции и применяется в лоб - обновляется только фрейм два
	FPhysicsInterface::ExecuteOnUnbrokenConstraintReadWrite(CI->ConstraintHandle, [&](const FPhysicsConstraintHandle& InUnbrokenConstraint)
	{	FPhysicsInterface::SetLocalPose(InUnbrokenConstraint, FTransform(CI->PriAxis2, CI->SecAxis2, CI->PriAxis2 ^ CI->SecAxis2, CI->Pos2), EConstraintFrame::Frame2);	});
}

//==============================================================================================================
//внести в констрейнт центрального членика новые данные по разрешению жесткой вертикали и юления по сторонам
//==============================================================================================================
void UMyrPhyCreatureMesh::UpdateCentralConstraintLimits(FGirdle &G)
{
	auto  CBody = GetGirdleBody(G, EGirdleRay::Center);
	auto& CI = CBody->DOFConstraint;
	SetFreedom(CI,		
		G.FixedOnFloor ? 0:-1,		//ось вверх-вниз (нормаль) при фиксированности жестко застороить ее
		G.FixedOnFloor && MyrOwner()->ExternalGain == 0 ? 0:-1,	//ось вперед-назад, пока хз, надо видимо при остановке закреплять
		G.FixedOnFloor ? 0:-1,		//ось лево-право, при фиксированности застопорить (смена курса только сменой фрейма)
		G.Vertical ? 0:180,			//если жесткая вертикаль, то запретить "падать"
		G.Vertical ? 0:180,			//если жесткая вертикаль, то запретить "падать"
		G.NoTurnAround ? 0:180);	//если фиксация на переде, то запретить рыскать вбок
}


//==============================================================================================================
// отдельная процедура по полной обработке колеса-ноги в контексте пояса конечностей
// 2 таких вызова покрывают 2 ноги, часть параметров для ног, туловища и всего пояса накапливают аддитивно
//==============================================================================================================
void UMyrPhyCreatureMesh::ProcessGirdleFoot (
	FGirdle& Girdle,			// пояс, содержащий эту ногу
	FLimb& FootLimb,			// текущая нога, к ней все рассчитывается
	FLimb& OppositeLimb,		// противоположная нога, для некоторых операций, где нужны обе ноги
	float DampingForFeet,		// вязкость тормозящяя ногу, посчитанная заранее
	FVector PrimaryForth,		// направление вперед, курс, пока без проекций на поверхность
	float &WeightAccum,			// аккумулятор веса, для совместного расчёта важности поворота в эту ногу или в соседнюю
	bool& NeedForVertical,		// базис нужы в ертикали, здесь он дополняется условиями увечий ноги и т.п.
	float DeltaTime)			// квант времени
{
	//физическое тело ноги
	FBodyInstance* FootBody = GetMachineBody(FootLimb);
	FBodyInstance* ArchFootBody = GetArchMachineBody(FootLimb);

	//центральный хабовый членик
	FLimb& CentralLimb = GetGirdleLimb(Girdle, EGirdleRay::Center);
	FBodyInstance* CentralBody = GetMachineBody(CentralLimb);

	//повреждение ноги ограничивается, чтоб не получить заоблачных производных вычислений
	float ClampedDamage = FMath::Min(FootLimb.Damage, 1.0f);

	//дорогое уточнение актуальности флага приземленност ина опору - трассировка сферой
	if (FootLimb.Stepped == 1)
	{
		FHitResult Hit;
		FCollisionShape Shape;
		if (FootBody->GetBodySetup()->AggGeom.SphereElems.Num())
			Shape.SetSphere(FootBody->GetBodySetup()->AggGeom.SphereElems[0].Radius);

		//чесануть по опре вымышленной сферой ноги, если попали - обновить счётчик, если нет, зарубить досрочно
		if(FootLimb.Floor->Sweep(Hit,FootBody->GetCOMPosition(),FootBody->GetCOMPosition() - FootLimb.ImpactNormal,	FQuat(), Shape, true))
		{	FootLimb.Stepped = STEPPED_MAX;
			Log(FootLimb, TEXT("%s: ProcessGirdleFoot - Reestablish steppedness"));
		}
		else
		{	UntouchLimb(FootLimb);
			UE_LOG(LogTemp, Log, TEXT("%s: ProcessGirdleFoot %s early untouch 0"),
				*GetOwner()->GetName(), *TXTENUM(ELimb, FootLimb.WhatAmI));
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////

	// привязь этой ноги к туловищу
	auto Cons = GetMachineConstraint(FootLimb);
	auto ARCons = GetArchMachineConstraint(FootLimb);

	//амплитуда ног - пока неясно, какой параметр использовать, таргет или лимит, лимит обычно указывается больше
	float FreeFeetLength = FMath::Abs(ARCons->ProfileInstance.LinearDrive.PositionTarget.Z);/*FMath::Abs(ARCons->ProfileInstance.LinearLimit.Limit);*/

		//линейный целевой уровень пригиба ноги, зависит от того, насколько телу нужно пригнуться и насколько нога повреждена
		//-1 - полностью пригнуто, +1 - полностью вытянуто, неясно пока, нужно ли ограничивать диапазон
		//плюс к этому в режиме лазанья уровень пригнутости зависит от вертикальности
		//фактор наклона, в горизонте 0, при любом уклоне возрастает
		//float SlopeFactor = Girdle.Clung ? 1.0f : (1 - FootLimb.ImpactNormal.Z);

		//при добавке экстремльных пригибов вертикальное расслабление нивелируется сильной пригиба
		//if(Girdle.Crouch > 1) SlopeFactor

		//абсолютный сдвиг целевого положения ноги в сантиметрах от нуля (полусогнутости)
		//в горизонте без урона и пригиба = 1 * FreeFeetLength, то есть выгиб в плюс, в сторону земли до полного выпрямления ног
	float AbsoluteCrouch = FreeFeetLength * (1 - 2 * FMath::Max(Girdle.Crouch, ClampedDamage));

	//финальная запись точки равновесия лапки
	//поскольку мы не знаем, на какой оси висит направление вниз, изменяем сразу все, нужная определяется режимами лимитов
	Cons->SetLinearPositionTarget(FVector(AbsoluteCrouch, AbsoluteCrouch, AbsoluteCrouch));

	//сила между колесом ноги и туловищем - вот это значение по умолчанию для режима карабканья
	//вот здесь неясно, в редакторе сила сразу для всех устанавливается
	float ElbowForce = ARCons->ProfileInstance.LinearDrive.ZDrive.Stiffness;

	//если не карабкаемся, надо сложно вычислять жесткость ноги, чтобы красиво пригибать на крутом склоне
	if (!Girdle.Clung)
	{
		//пока пригнутость меньше единицы, сила выпрямления снижается с уровнем пригнутости, чтобы не противиться гравитации и разгибать наболее низком склоне
		//но можно вывернуть экстремальную пригнутость больше единицы, чтобы ноги ещё и активно подтягивались
		float CrouchInfluence = Girdle.Crouch < 1 ? (-0.5f * Girdle.Crouch) : (0.5 * Girdle.Crouch - 1);

		//расчёт силы распрямления индивидуальной лапки
		ElbowForce *= FMath::Max(0.0f,	// чтобы выдержать снижающие слагаемые и не уйти в минус
			0.5f * FootLimb.ImpactNormal.Z				// чем круче опора, тем меньше сила - видимо, чтобы на горке ноги повторяли кочки
			- (FootLimb.Damage + OppositeLimb.Damage)	// при повреждении ноги удерживть тело на весу трудно, но надо симметрично по обоим ногам, иначе кренится нереалистично
			+ CrouchInfluence);							// ослабление или усиление разгибания от степени намеренного пригиба тела
	}

	//пока неясно, как лучше расчитывать скорость
	//брать саму скорость не из членика-колеса, а из центрального членика
	FVector RelativeSpeedAtPoint;

	//RelativeSpeedAtPoint = FootBody->GetUnrealWorldVelocity();	// не работает если корень кинематический
	RelativeSpeedAtPoint = CentralBody->GetUnrealWorldVelocity();	// не работает если корень кинематический
	//Girdle.LastPosition = CentralBody->GetCOMPosition();


	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//нога на опоре, опору знаем, по ней можем позиционировать тело
	//второе условие - исключить контакты колёс в верхней полусфере - это не касание лап, это паразитное
	//возможно, ограничение на широту ввести в в генофонд
	if(FootLimb.Stepped)
	{
		//относительная скорость условно в точке ноги (просто плавно набрасываем новые значения через Lerp)
		FVector Point = FootBody->GetCOMPosition();

		//сделать скорость относительной, вычтя скорость пола в точке
		RelativeSpeedAtPoint -= FootLimb.Floor->GetUnrealWorldVelocityAtPoint (Point);

		//финальное значение плавно подвигать - хз, может лишнее
		Girdle.VelocityAgainstFloor = FMath::Lerp(Girdle.VelocityAgainstFloor, RelativeSpeedAtPoint, 0.1f);

		//куда опора позволяет двигать ногу + насколько эта колея искажает первоначальный путь
		float EstimWeight = 1.0f;

		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
		//вычислить направление движения для данной ноги из внешних факторов
		FVector GuideDir = CalculateLimbGuideDir ( FootLimb, PrimaryForth, true, &EstimWeight);
		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№

		//исключить неправильный вес
		if(EstimWeight < 0.0f) EstimWeight = 0.05f;

		//если на уровне всего пояса мы цепляемся, но конкретно эта нога стоит на нецепляемой опоре - занизить ее вес
		if(Girdle.Clung && !FootLimb.IsClimbable())	EstimWeight *= 0.1f;

		//ноги стоят на разных опорах - возможно, стоит жетко выбрать одну из них
		if (OppositeLimb.Floor != FootLimb.Floor && OppositeLimb.Stepped)
		{
			//нога или весомее первой или сама первая (как оказалось, это одинаково, WeightAccum=0 перед первой ногой)
			if (EstimWeight > WeightAccum)
			{	
				//курс для текущей ноги = вычисленный курс, умноженный на масштаб, который понадобится при вычислении среднего вектора
				//в случае если две ноги стоят на разных опорах, иначе этот множитель нивелируется через nprmalize
				Girdle.GuidedMoveDir = GuideDir * (1.0f + Girdle.Curvature + EstimWeight);
				CentralLimb.Floor = FootLimb.Floor;		
				Log(FootLimb, TEXT("Just accumulating dir EstimWeight/WeightAccum: "), EstimWeight, WeightAccum);
			}
			//нога незначима, для режима лазанья разность курсов ведёт к силовой раскоряке
			//поэтому отдёрнуть слабую ногу от опоры и не изменить вектор курса GuidedMoveDir
			else if(FootLimb.Clung)
			{	UntouchLimb(FootLimb);
				Log(FootLimb, TEXT("UntouchLimb cause wrong direction! EstimWeight/WeightAccum: "),EstimWeight, WeightAccum);
			}
		}
		//обе ноги на одной опоре
		else
		{
			//курс - просто взвешенная сумма, почти среднее
			Girdle.GuidedMoveDir += GuideDir * (1.0f + Girdle.Curvature + EstimWeight);

			//внести лепту в нормаль - если ее потом не подстроят по ветке, будет среднее по двум ногам
			//важно именно среднее по ногам, а не текущий верх членика, чтобы можно было подстраивать под позицию между ног
			CentralLimb.ImpactNormal += FootLimb.ImpactNormal * (1.0f + Girdle.Curvature + EstimWeight);

			//генерализация опоры на центральный узел - только той, ноги, что весомее
			if (EstimWeight > WeightAccum || CentralLimb.Floor == nullptr)
				CentralLimb.Floor = FootLimb.Floor;
		}

		//аккумуляция веса
		WeightAccum += EstimWeight;

		//дополнительные условия по поддержке жетской вертикали
		NeedForVertical = NeedForVertical
			&& FootLimb.ImpactNormal.Z > 0.5			// склон достаточно пологий
			&& FootLimb.Damage < 0.5f;					// ноги не сильно повреждены

		//применение вязкости среды для колеса ноги - торможение
		//сбавлять тормоз сразу - для разгона, а наращивать плавно - тормозной путь
		BrakeD ( FootBody->LinearDamping, DampingForFeet, DeltaTime);
		BrakeD ( FootBody->AngularDamping, DampingForFeet*10, DeltaTime);
		FootBody->UpdateDampingProperties();

		//если включены "тяжелые ноги" удельный вес ног все же зависит от их повреждений - чтобы при сильном увечьи легче валиться на бок
		if(Girdle.CurrentDynModel->HeavyLegs) FootBody->SetMassScale(ArchFootBody->MassScale*(1 - 0.5 * ClampedDamage));

		//настройки силы сгиба лапы
		Cons->SetLinearDriveParams(ElbowForce,
			100 + 0 * Girdle.Clung,									// инерция скорости, чтоб не пружинило, но пока неясно, как этим управлять
			ARCons->ProfileInstance.LinearDrive.ZDrive.MaxForce);	// максимальная сила -  копируем из редактора (или разбить функцию на более мелкие, глубокие) или вообще пусть 0 будет

		//если в процессе атаки эта нога совершает аткивные действия, то сбросить уровень контакта с землей для всего пояса
		//это нужно для того, чтобы при атаке атакующие лапы не делали анимацию ходьбы
		if (MyrOwner()->IsLimbAttacking(FootLimb.WhatAmI))
			Girdle.StandHardness = 0;
	}
	//нога оторвана от опоры
	else
	{
		//если вторая нога всё же обеспечивает зацеп
		if (Girdle.Clung)
		{
			FootBody->AddForce(-1000 * OppositeLimb.ImpactNormal, false, true);
		}

		//без опоры скорость мееедленно переходит на отсчёт абсолютный
		Girdle.VelocityAgainstFloor = FMath::Lerp (	Girdle.VelocityAgainstFloor, RelativeSpeedAtPoint, DeltaTime);

		//пока нога в воздухе, меедленно смещать ее "нормаль" к направлению вверх туловища
		FootLimb.ImpactNormal = FMath::Lerp(FootLimb.ImpactNormal, GetLimbAxisUp(CentralLimb), DeltaTime);
		
		//раз и на весь период полета делаем полетными праметры сил ноги
		if(FootBody->LinearDamping > 0.01f) 
		{	SetLimbDamping (FootLimb, 0.01, 0);
			GetMachineConstraint(FootLimb)->SetLinearDriveParams(ElbowForce, 0, 5000);
		}

		//стоит одной ноге не быть на опоре, как вертикаль сама уже не включится, только поддержка/усиление если уже была включена
		NeedForVertical = NeedForVertical && Girdle.Vertical;
	}

	//отладка, директивы препроцессора внутри
	/*DrawMove(FootBody->GetCOMPosition(), FVector(0),
		FColor(FootLimb.Clung*255, FootBody->LinearDamping, FootBody->DOFConstraint ? 255*IsClingAtStop(FootBody->DOFConstraint) : 0),
		FootLimb.Stepped * 0.5); */
}

//==============================================================================================================
//основная покадровая рутина для пояса конечностей
//==============================================================================================================
void UMyrPhyCreatureMesh::ProcessGirdle (FGirdle& Girdle, FGirdle& OppositeGirdle, float DeltaTime, float DirToMiddle)
{
#if WITH_EDITOR
	if (!Girdle.CurrentDynModel) return;
#endif

	//тело хаба для ног
	auto& CentralLimb = GetGirdleLimb (Girdle, EGirdleRay::Center );
	auto CentralBody = GetGirdleBody (Girdle, EGirdleRay::Center );
	auto& SpineLimb = GetGirdleLimb (Girdle, EGirdleRay::Spine );

	//направление движения для дальнейшего уточнения (по курсам для каждой ноги)
	//ведущий пояс направлен по взгляду камеры, ведомый - туда куда уже ориентирован
	//а вообще с ведомым пока непонятно, куда его отдельно направлять + всё это для четвероногих
	FVector PrimaryForth = Girdle.CurrentDynModel->Leading
		? MyrOwner()->MoveDirection
		: GetLimbAxisForth(CentralLimb) * FMath::Sign(MyrOwner()->MoveDirection | GetLimbAxisForth(CentralLimb));

	//уровень пригнутости для пояса - просто сглаженное повторение циферки из дин.модели
	Girdle.Crouch = FMath::Lerp (Girdle.Crouch, Girdle.CurrentDynModel->Crouch, DeltaTime);

	//начальный признак - необходимость в жесткой вертикали - сразу пропадает при сильном увечье обеих ног или туловища
	bool WeStillNeedVertical = Girdle.CurrentDynModel->HardVertical
		&& CentralLimb.Damage < 0.8f
		&& SpineLimb.Damage < 0.8f;


	////////////////////////////////////////////////////////////////////////////////
	//если пояс конечностей не содержит конечностей
	if(!Girdle.HasLegs)
	{
		//если брюхо касается опоры
		if(CentralLimb.Stepped)
		{
			//рассчитать вязкость для ног, имитирующую торможение и трение покоя (почему только линейное?)
			float DampingForFeet = CalculateGirdleFeetDamping(Girdle);
			SetLimbDamping (CentralLimb, DampingForFeet, 0);

			//используем "брюхо" для определения опоры и ее относительной скорости
			Girdle.VelocityAgainstFloor = FMath::Lerp (
				Girdle.VelocityAgainstFloor,
				CentralBody->GetUnrealWorldVelocity()
					- CentralLimb.Floor ->GetUnrealWorldVelocityAtPoint ( GetMachineBody(CentralLimb)->GetCOMPosition() ),
				0.1f);


			//используем нормаль брюха для определения вектора направления (бок может быть накренён, посему нормализировать)
			Girdle.Forward = FVector::CrossProduct (GetLimbAxisRight(CentralLimb), CentralLimb.ImpactNormal);
			Girdle.Forward.Normalize();

			//точное направление для движения - по опоре, которую касается брюхо (тру - это вкл. поддержку на ветке)
			Girdle.GuidedMoveDir = CalculateLimbGuideDir (	CentralLimb, PrimaryForth, true);
			Girdle.GuidedMoveDir.Normalize();

			//включить удержание вертикали в ветках выше накопилось решение, нужна вертикаль или нет, 
			if (WeStillNeedVertical) SetGirdleVertical(Girdle);
			else ClearGirdleVertical(Girdle);

			//обновить оси констрейнта ориентации в пространстве мира, используется он для ограничений или нет
			UpdateCentralConstraintFrames(Girdle);

		}
		//если брюхо в воздухе, 
		else 
		{	//плавно возвращать ее скорость к абсолютной (хотя здесь можно задействовать ветер)
			Girdle.VelocityAgainstFloor = FMath::Lerp ( Girdle.VelocityAgainstFloor, CentralBody->GetUnrealWorldVelocity(), DeltaTime );
			Girdle.Crouch *= (1 - DeltaTime);
			Girdle.Forward = GetLimbAxisForth(CentralLimb);
		}
		return;
	}
	////////////////////////////////////////////////////////////////////////////////
	//если у пояса есть ноги
	else
	{
		//части тела конечностей
		auto& RLimb = GetGirdleLimb(Girdle, EGirdleRay::Right);	
		auto& LLimb = GetGirdleLimb(Girdle, EGirdleRay::Left);	

		//накопление влияний ног на позицию туловища
		float WeightAccum = 0.0f;
		float DampingForFeet = CalculateGirdleFeetDamping (Girdle);
		//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
		ProcessGirdleFoot (Girdle, RLimb, LLimb, DampingForFeet, PrimaryForth, WeightAccum, WeStillNeedVertical, DeltaTime);
		ProcessGirdleFoot (Girdle, LLimb, RLimb, DampingForFeet, PrimaryForth, WeightAccum, WeStillNeedVertical, DeltaTime);
		//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
		//аккумулятор веса нулевой - обе ноги в воздухе
		if(WeightAccum == 0)
		{
			//безусловно снять всякие привязи
			ClearGirdleVertical(Girdle);
			CentralLimb.ImpactNormal = GetLimbAxisUp(CentralLimb);
			Girdle.GuidedMoveDir = PrimaryForth;

			//если соседний пояс успешно держится зацепом
			if(OppositeGirdle.Clung)
			{
				//пока просто сравнять, чтобы удерживать спину
				CentralLimb.ImpactNormal = GetGirdleLimb(OppositeGirdle, EGirdleRay::Center).ImpactNormal;
				Girdle.GuidedMoveDir = OppositeGirdle.GuidedMoveDir;
			}
		}
		//одна или две ноги стоят на поверхности
		else
		{
			//при лазаньи по ветке берется так, чтобы держаться ровно к вертикали
			if (Girdle.Clung && CentralLimb.OnBranch() && CentralLimb.OnCapsule())
			{
				FVector GuideSaddleUp = CentralLimb.GetBranchSaddleUp();
				if (GuideSaddleUp.Z > 0.2) CentralLimb.ImpactNormal = GuideSaddleUp;
			}
			//нормировка
			CentralLimb.ImpactNormal.Normalize();

			//вектор "правильный вперед", корректирующий pitch'и обеих частей туловища
			Girdle.Forward = FVector::CrossProduct (GetLimbAxisRight(CentralLimb), CentralLimb.ImpactNormal);
			Girdle.Forward.Normalize();

			//направление вперед из произвольного угла, всегда нужно нормировать
			Girdle.GuidedMoveDir.Normalize();

			//включить удержание вертикали в ветках выше накопилось решение, нужна вертикаль или нет, 
			if (WeStillNeedVertical) SetGirdleVertical(Girdle);
			else ClearGirdleVertical(Girdle);

			//изящный, всеобъемлющий, нетранцедентный расчёт кривизны опоры по раскояке нормалей ног
			//здесь надо проверять, что обе ноги стоят, тначе неправильно
			//а можно вообще брать не ногу, а угол между нормалью и верхом туловища
			//Girdle.Curvature = FVector::DotProduct(Girdle.Forward, FVector::CrossProduct(RLimb.ImpactNormal, LLimb.ImpactNormal));
			Girdle.Curvature = FVector::DotProduct(
				Girdle.Forward,
					FVector::CrossProduct(RLimb.ImpactNormal, GetLimbAxisUp(CentralLimb))
					- FVector::CrossProduct(LLimb.ImpactNormal, GetLimbAxisUp(CentralLimb))
			);
		}
		
		///////////////////////////////////////////////////////////////////

		//обновить оси констрейнта ориентации в пространстве мира, используется он для ограничений или нет
		UpdateCentralConstraintFrames(Girdle);

		//сверху от туловище рисуем тестовые индикаторы
		auto Ct = CentralBody->GetCOMPosition() + CentralLimb.ImpactNormal * 15;
		auto Color = FColor(255, 0, 255, 10);
		if (Girdle.Vertical)			DrawDebugString(GetWorld(), Ct, TEXT("V"), nullptr, FColor(255, 255, 0), 0.01, false, 1.0f);
		if (Girdle.TransToVertical)		DrawDebugString(GetWorld(), Ct, TEXT("  T"), nullptr, FColor(0, 255, 255), 0.01, false, 1.0f);
		if (Girdle.NoTurnAround)		DrawDebugString(GetWorld(), Ct, TEXT("    Y"), nullptr, FColor(255, 0, 255), 0.01, false, 1.0f);
	}
}


//==============================================================================================================
//вычислить торможение ног этого пояса через вязкость - 
//вынесено в отдельную, так как важный регулятор движения, сюда, возможно, будет добавлено что-то ещё
//на данный момент цепляние абсолютно независимо от вот этого торможения
//==============================================================================================================
float UMyrPhyCreatureMesh::CalculateGirdleFeetDamping(FGirdle &Girdle)
{
	//если мертвы, торможение максимально без вопросов
	if (MyrOwner()->Health <= 0) return Girdle.CurrentDynModel->FeetBrakeDamping;//◘◘>

	//центральное тело пояса - туловище между ног/рук
	auto& CentralLimb = GetGirdleLimb(Girdle, EGirdleRay::Center);

	//в качестве агента касания земли берем одну из ног, если они есть или сам центральный, если их нет
	auto LBody = GetGirdleBody(Girdle, Girdle.HasLegs ? EGirdleRay::Left : EGirdleRay::Center);

	//не тормозить если склон слишком крут (скольжение) и если режим лёгких/неустойчивых ног
	if(/*LBody->MassScale < 1*/!Girdle.CurrentDynModel->HeavyLegs || CentralLimb.ImpactNormal.Z < 0.5f) return 0.0f;//◘◘>

	//скорость в направлении желаемого движения и в боковом направлении (в некоторых режимах этого стоит избегать)
	const float SpeedAlongCourse  = FMath::Abs ( FVector::DotProduct (Girdle.VelocityAgainstFloor, Girdle.GuidedMoveDir));
	const float SpeedAlongLateral = FMath::Abs ( FVector::DotProduct (Girdle.VelocityAgainstFloor, GetLimbAxisRight(CentralLimb)));
	const float WantedSpeed = MyrOwner()->GetDesiredVelocity();

	//если набранная скорость маловата - не сдерживать движение вообще
	float Overspeed =  SpeedAlongCourse - WantedSpeed;
	if(!Girdle.Leading) Overspeed += SpeedAlongLateral;
	if (Overspeed < 0)	return 0.0f;//◘◘>

	//если хотим двигаться, но медленнее чем сейчас - плавно усилить тормоз, не стопоря движ
	if (WantedSpeed > 0) return Overspeed / WantedSpeed;//◘◘>

	//если включена жесткая вертикаль, но в мягкой фазе, не тормозить
	if (Girdle.TransToVertical)	return 1.0f;//◘◘>

	//базовое значение торможения, зберется базовый коэф и в 10 раз увеличивается при крутом склоне
	float NewDamp = Girdle.CurrentDynModel->FeetBrakeDamping
		* (10 - 9*CentralLimb.ImpactNormal.Z);
	return NewDamp;//◘◘>
}

//==============================================================================================================
//создать или просто в(ы)ключить вертикальное выравнивание пояса конечностей
//==============================================================================================================
void UMyrPhyCreatureMesh::SetGirdleVertical (FGirdle& G)
{
	//узел пояса конечностей
	auto& CLimb = GetGirdleLimb(G, EGirdleRay::Center);
	auto  CBody = GetGirdleBody(G, EGirdleRay::Center);
	auto& CI = CBody->DOFConstraint;

	//ориентация вверх центрального членика
	float CharZ = GetLimbAxisUp(CLimb).Z;

	//если членик, который должен был смотреть вверх, смотрит вниз - нас опрокинули, нельзя пока
	//хотя если ног нет, то можно, иначе сложно
	if (GetLimbAxisUp(CLimb).Z<0) return;//◘◘>

	//если отклонение от вертикали слишком большое
	//закоментированно, так как слишком позорно теряет устойчивость
	//если depenetration vel не бесконечность, то резкая вертикаль не будет распидорашивать
/*	if(CharZ < 0.95)
	{
		//если ещё не включён режим доводки - включить режим доводки:
		if (!IsVerticalTransition(CI))
		{
			//для доводки использовать меры сил, указанный в дин-модели, но вот как их домножать, пока не ясно
			CI->SetAngularDriveParams(
				G.CurrentDynModel->ForcesMultiplier * 10 * CBody->GetBodyMass(),
				G.CurrentDynModel->ForcesMultiplier * 0.1 * CBody->GetBodyMass(),
				100000);
			CI->SetOrientationDriveTwistAndSwing(false, true);
			CI->SetAngularVelocityDriveTwistAndSwing(false, true);
			G.TransToVertical = true;
			Log(G, TEXT("SetGirdleVertical Switch to Refine from Z = "), CharZ);
		}
		//доводка уже включена и работает, просто вывести степень готовности
		else Log(G, TEXT("SetGirdleVertical Keep Refining, Z = "), CharZ);			
	}*/
	//если вертикаль набрана, но ещё работает доводка (=отключить) или сам флаг вертикали не взведен (=сразу ровно, доводка не понадобилась)
	else if(G.TransToVertical==true || G.Vertical!=true)
	{	
		//досижение вертикали - отключение свобод и сил
		Log(G, TEXT("SetGirdleVertical, Final Z = "), CharZ);
		CI->SetOrientationDriveTwistAndSwing(false, false);
		CI->SetAngularVelocityDriveTwistAndSwing(false, false);
		G.TransToVertical = false;
		G.Vertical = true;
	}

	//обновить пределы по выставленным флагам
	UpdateCentralConstraintLimits(G);
	//здесь не обновляется физика, она и так каждый кадр, см. UpdateCentralConstraintFrames
}



//==============================================================================================================
// отменить вертикальность
//==============================================================================================================
void UMyrPhyCreatureMesh::ClearGirdleVertical(FGirdle& G)
{
	//если уже и так отключено
	if(!G.Vertical) return;//◘◘>

	//узел пояса конечностей
	auto  CBody = GetGirdleBody(G, EGirdleRay::Center);
	auto& CI = CBody->DOFConstraint;
	if (!CI) return;//◘◘>

	//зафиксировать или расслабить пояс по осям крена
	auto& CLimb = GetGirdleLimb(G, EGirdleRay::Center);
	CI->SetOrientationDriveTwistAndSwing(false, false);
	CI->SetAngularVelocityDriveTwistAndSwing(false, false);
	SetFreedom(CI, -1, -1, -1, 180, 180, 180);
	G.Vertical = false;
	G.TransToVertical = false;
	Log(G, TEXT("ClearGirdleVertical"));
}

//==============================================================================================================
//ограничить юление по сторонам
//==============================================================================================================
void UMyrPhyCreatureMesh::SetGirdleYawLock(FGirdle& G, bool Set)
{
	G.NoTurnAround = Set;

	//пока предполагается, что фреймы у привязи уже настроены, поэтому просто обновить пределы
	UpdateCentralConstraintLimits(G);
}






//==============================================================================================================
//задать предпочтительную ориентацию пояса конечностей - при смене позы
//==============================================================================================================
void UMyrPhyCreatureMesh::AdoptDynModelForGirdle (FGirdle& Girdle, FGirdleDynModels& Models)
{
	//сохранить связку, так как в процессе понадобится уточнять
	Girdle.CurrentDynModel = &Models;
	auto& CLimb = GetGirdleLimb(Girdle, EGirdleRay::Center);
	auto& RLimb = GetGirdleLimb(Girdle, EGirdleRay::Right);
	auto& LLimb = GetGirdleLimb(Girdle, EGirdleRay::Left);
	auto& SLimb = GetGirdleLimb(Girdle, EGirdleRay::Spine);
	auto& TLimb = GetGirdleLimb(Girdle, EGirdleRay::Tail);

	//влить почленно
	AdoptDynModelForLimb(Girdle, CLimb, Models.Center, Models.CommonDampingIfNeeded);
	AdoptDynModelForLimb(Girdle, SLimb, Models.Spine, Models.CommonDampingIfNeeded);
	AdoptDynModelForLimb(Girdle, TLimb, Models.Tail, Models.CommonDampingIfNeeded);

	if(Girdle.HasLegs)
	{
		AdoptDynModelForLimb(Girdle, RLimb, Models.Left, Models.CommonDampingIfNeeded);
		AdoptDynModelForLimb(Girdle, LLimb, Models.Right, Models.CommonDampingIfNeeded);
		auto RBody = GetGirdleBody(Girdle, EGirdleRay::Right);
		auto LBody = GetGirdleBody(Girdle, EGirdleRay::Left);
		auto aRBody = GetArchMachineBody(RLimb.WhatAmI);
		auto aCBody = GetArchMachineBody(CLimb.WhatAmI);

		//минимальная плотность ног берется из плотонсти таза
		const float MinFeetMassScale = aCBody->MassScale*0.5;
		const float MAxFeetMassScale = aRBody->MassScale;
		LBody->SetMassScale(Models.HeavyLegs ? MAxFeetMassScale * (1 - 0.1 * FMath::Min(LLimb.Damage, 1.0f)) : MinFeetMassScale);
		RBody->SetMassScale(Models.HeavyLegs ? MAxFeetMassScale * (1 - 0.1 * FMath::Min(RLimb.Damage, 1.0f)) : MinFeetMassScale);

	}
	else
	{
		//пока костыль, типа без ног функции ног несут вот эти хабовые членики
		//GetGirdleBody(Girdle, EGirdleRay::Center)->SetEnableGravity(Models.GravityForLegs);
	}

	//жестко привязать центр к ориентации на курс
	SetGirdleYawLock(Girdle, Models.HardCourseAlign);

	//запретить ногам махать вдоль спины - сомнительная надобность, опасно
	SetGirdleSpineLock(Girdle, Models.LegsSwingLock);

}
void UMyrPhyCreatureMesh::AdoptDynModelForLimb(FGirdle& Girdle, FLimb& Limb, uint32 Model, float DampingBase)
{
	//если вообще отсутствует эта часть тела
	if (!HasPhy(Limb.WhatAmI)) return;

	//основном физ членик этого лимба
	FBodyInstance* Body = GetMachineBody(Limb);

	//присвоение флагов - они оживают динамически каждый кадр
	Limb.DynModel = Model;

	//гравитация изменяется только в момент переключения модели
	Body->SetEnableGravity((Model & LDY_GRAVITY) == LDY_GRAVITY);

	//вязкость изменяется при переключении моделей
	if (!(Model & LDY_DAMPING))
		SetLimbDamping(Limb, 0, 0);	
	else
	{	float DampMult = 0.25 * (bool)(Model & LDY_DAMPING_4_1) + 0.25 * (bool)(Model & LDY_DAMPING_4_2) + 0.5 * (Model & LDY_DAMPING_2_1);
		SetLimbDamping(Limb, DampMult * DampingBase);
	}

}
//==============================================================================================================
//включить или отключить проворот спинной части относительно ног (мах ногами вперед-назад)
//==============================================================================================================
void UMyrPhyCreatureMesh::SetGirdleSpineLock(FGirdle &G, bool Set)
{
	//если изначально был залочен, то и будет залочен
	auto CI = GetGirdleSpineConstraint(G);
	auto aCI = GetGirdleArchSpineConstraint(G);
	if (Set)
		CI->ProfileInstance.TwistLimit.TwistMotion = EAngularConstraintMotion::ACM_Locked;
	else
		CI->ProfileInstance.TwistLimit.TwistMotion = aCI->ProfileInstance.TwistLimit.TwistMotion;
	CI->UpdateAngularLimit();
}


//==============================================================================================================
//расслабить все моторы в физических привязях
//==============================================================================================================
void UMyrPhyCreatureMesh::SetFullBodyLax(bool Set)
{
	//сделать регдоллом
	UE_LOG(LogTemp, Log, TEXT("%s: SetFullBodyLax %d"), *GetOwner()->GetName(), Set);
	if (Set)
	{	for (auto C : Constraints)
		{	C->SetAngularPositionDrive(false, false);
			C->SetLinearPositionDrive(false, false, false);
		}
		SetPhyBodiesBumpable(false);
	}
	//сделать напряженным
	else
	{
		for (int i=0; i<Constraints.Num(); i++)
			Constraints[i]->ProfileInstance = GetArchConstraint(i)->ProfileInstance;
		SetPhyBodiesBumpable(true);
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
		GetRootBody()->SetCollisionProfileName(TEXT("PawnTransparent"));

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
	UE_LOG(LogTemp, Warning, TEXT("%s SetPhyBodiesBumpable %d"), *GetOwner()->GetName(), Set);
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
//может этот пояс конечностей вот прям щас прицепиться к поверхности под ногами
//==============================================================================================================
bool UMyrPhyCreatureMesh::CanGirdleCling(const FGirdle& Girdle) const
{
	//если не ведущий, сразу нет... хотя можно оставить эту проверку на внешний код
	if (!Girdle.CurrentDynModel->Leading) return false;
	else
	{ 
		//если пояс двигается на ногах
		if (Girdle.HasLegs)
		{
			//если хотя бы одна нога касается правильной поверхности
			auto S = const_cast<UMyrPhyCreatureMesh*>(this);
			if (S->GetGirdleLimb(Girdle, EGirdleRay::Left).IsClimbable() 
				|| S->GetGirdleLimb(Girdle, EGirdleRay::Right).IsClimbable()
				|| S->GetGirdleLimb(Girdle, EGirdleRay::Center).IsClimbable())
				return true;
		}
		//если ног не добавили, и пояс елозит на брюхе
		else
		{
			//если хотя бы один из сегментов брюха, принадежащий этому поясу, стоит на нужной поверхности
			auto S = const_cast<UMyrPhyCreatureMesh*>(this);
			if (S->GetGirdleLimb(Girdle, EGirdleRay::Center).IsClimbable() || S->GetGirdleLimb(Girdle, EGirdleRay::Spine).IsClimbable())
				return true;
		}
	}
	return false;
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

//==============================================================================================================
// служебная хрень - сделать или обновить субъекты констрейнта
//Tune... - правильно рассчетать оси и положение, иначе взять рассчитанные до вызова этой функции
//==============================================================================================================
EClung UMyrPhyCreatureMesh::MakeConstraint (FConstraintInstance* CI, FVector Pri, FVector Sec, FVector At, FBodyInstance* B1, FBodyInstance* B2, bool TunePri, bool TuneSec, bool TuneAt)
{
	//берется тело, из него - трансформация, из неё - обратные преобразования, но если тела нет, то полагаемся на ранее введенные значения
	BodyToFrame (B1, Pri, Sec, At, CI->PriAxis1, CI->SecAxis1, CI->Pos1, TunePri, TuneSec, TuneAt);
	BodyToFrame (B2, Pri, Sec, At, CI->PriAxis2, CI->SecAxis2, CI->Pos2);

	//если зацеп был по каким-то причинам терминирован (например, нужно сменить операнды) происходит создание зацепа с нуля
	if (CI->IsTerminated())
	{	CI->InitConstraint(B1, B2, 1.0f, B1->OwnerComponent.Get());
		#if DEBUG_LINE_CONSTRAINTS
		UE_LOG(LogTemp, Log, TEXT("---- Constraint Recreate"));
		DrawDebugLine(B1->OwnerComponent->GetOwner()->GetWorld(), At, At + Sec * 10, FColor(255, 0, 0), false, 100, 100, 2);
		DrawDebugLine(B1->OwnerComponent->GetOwner()->GetWorld(), At, At + Pri * 10, FColor(0, 0, 255), false, 100, 100, 2);
		#endif
		return EClung::Recreated;
	}
	else
	{	//дальше выкорчёвывается из исходников способ обращения с нижележащим слоем абстракции и применяется в лоб
		FPhysicsInterface::ExecuteOnUnbrokenConstraintReadWrite(CI->ConstraintHandle, [&](const FPhysicsConstraintHandle& InUnbrokenConstraint)
		{	FPhysicsInterface::SetLocalPose(InUnbrokenConstraint, FTransform(CI->PriAxis1, CI->SecAxis1, CI->PriAxis1 ^ CI->SecAxis1, CI->Pos1), EConstraintFrame::Frame1);
			FPhysicsInterface::SetLocalPose(InUnbrokenConstraint, FTransform(CI->PriAxis2, CI->SecAxis2, CI->PriAxis2 ^ CI->SecAxis2, CI->Pos2), EConstraintFrame::Frame2);	});
		#if DEBUG_LINE_CONSTRAINTS
		UE_LOG(LogTemp, Log, TEXT("---- Constraint Update"));
		DrawDebugLine(B1->OwnerComponent->GetOwner()->GetWorld(), At, At + Sec * 5, FColor(255, 0, 0), false, 100, 100, 1);
		DrawDebugLine(B1->OwnerComponent->GetOwner()->GetWorld(), At, At + Pri * 5, FColor(0, 0, 255), false, 100, 100, 1);
		#endif
		return EClung::Updated;
	}
}


void UMyrPhyCreatureMesh::DrawMove(FVector A, FVector AB, FColor C, float W, float Time)
{
#if DEBUG_LINE_MOVES
	DrawDebugLine(GetOwner()->GetWorld(), A, A + AB, C, false, Time, 100, W);
#endif
}
void UMyrPhyCreatureMesh::DrawAxis(FVector A, FVector AB, FColor C, float W, float Time)
{
#if DEBUG_LINE_CONSTRAINTS
	DrawDebugLine(GetOwner()->GetWorld(), A, A + AB, C, false, Time, 100, W);
#endif
}

void UMyrPhyCreatureMesh::DrawLimbAxis(FLimb& Limb, FVector AB, FColor C, float W, float Time)
{
#if DEBUG_LINE_CONSTRAINTS
	DrawDebugLine(GetOwner()->GetWorld(), GetMachineBody(Limb)->GetCOMPosition(), GetMachineBody(Limb)->GetCOMPosition() + AB, C, false, Time, 100, W);
#endif
}
