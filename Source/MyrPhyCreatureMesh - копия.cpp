// Fill out your copyright notice in the Description page of Project Settings.


#include "MyrPhyCreatureMesh.h"
#include "AssetStructures/MyrCreatureBehaveStateInfo.h"	// данные текущего состояния моторики
#include "MyrPhyCreature.h"								// само существо
#include "PhysicsEngine/PhysicsConstraintComponent.h"	// привязь на физдвижке
#include "AssetStructures/MyrCreatureGenePool.h"		// генофонд
#include "DrawDebugHelpers.h"							// рисовать отладочные линии
#include "PhysicsEngine/PhysicsConstraintTemplate.h"	// для разбора списка физ-связок в регдолле
#include "../Dendro/MyrDendroMesh.h"					// деревья - для поддержания лазанья по ветке
#include "PhysicalMaterials/PhysicalMaterial.h"			// для выкорчевывания материала поверхности пола


//имена кривых, которые должны быть в скелете и в анимации, чтобы детектировать незапланированные шаги
FName UMyrPhyCreatureMesh::StepCurves[(int)ELimb::NOLIMB] = { NAME_None, NAME_None, NAME_None, NAME_None, NAME_None, TEXT("LeftArm_Step"), TEXT("RightArm_Step"), TEXT("LeftLeg_Step"), TEXT("RightLeg_Step"), NAME_None };

//к какому поясу конечностей принадлежит каждая из доступных частей тела - для N(0) поиска
uint8 UMyrPhyCreatureMesh::Section[(int)ELimb::NOLIMB] = {0,0,1,1,1,1,1,0,0,0};
ELimb UMyrPhyCreatureMesh::Parent[(int)ELimb::NOLIMB] = {ELimb::PELVIS, ELimb::PELVIS, ELimb::LUMBUS, ELimb::PECTUS, ELimb::THORAX, ELimb::THORAX, ELimb::THORAX, ELimb::PELVIS, ELimb::PELVIS, ELimb::PELVIS};

//перевод из номера члена в его номер как луча у пояса конечностей
EGirdleRay UMyrPhyCreatureMesh::Ray[(int)ELimb::NOLIMB] = { EGirdleRay::Center, EGirdleRay::Spine, EGirdleRay::Spine, EGirdleRay::Center, EGirdleRay::Tail, EGirdleRay::Left, EGirdleRay::Right, EGirdleRay::Left, EGirdleRay::Right, EGirdleRay::Tail };

//полный перечень ячеек отростков для всех двух поясов конечностей
ELimb UMyrPhyCreatureMesh::GirdleRays[2][(int)EGirdleRay::MAXRAYS] = 
{	{ELimb::PELVIS,		ELimb::L_LEG,	ELimb::R_LEG,	ELimb::LUMBUS,	ELimb::TAIL},
	{ELimb::THORAX,		ELimb::L_ARM,	ELimb::R_ARM,	ELimb::PECTUS,	ELimb::HEAD}
};

//как (должны быть) ориентированы все кости в подскелете каркаса-поддержки
FLimbOrient UMyrPhyCreatureMesh::Lorient = { EMyrAxis::Yn, EMyrAxis::Xn, EMyrAxis::Zn };


//==============================================================================================================
//привязь конечности для шага/цепляния. ВНИМАНИЕ, наличие genepool и правильного индекса здесь не проверяется!
//данные проверки нуждо делать до выпуска существа на уровень, а physics asset существа должен быть правильно укомплектован
//==============================================================================================================
FMachineLimbAnatomy*	UMyrPhyCreatureMesh::MachineGene				(ELimb Limb) const		{ return &MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Limb).Machine; }
FFleshLimbAnatomy*		UMyrPhyCreatureMesh::FleshGene					(ELimb Limb) const		{ return &MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Limb).Flesh; }
FBodyInstance*			UMyrPhyCreatureMesh::GetArchMachineBody			(ELimb Limb) const		{ return &MyrOwner()->GenePool->SkeletalMesh->PhysicsAsset->SkeletalBodySetups[GetMachineBodyIndex(Limb)]->DefaultInstance; }
FConstraintInstance*	UMyrPhyCreatureMesh::GetArchMachineConstraint	(ELimb Limb) const		{ return &MyrOwner()->GenePool->SkeletalMesh->PhysicsAsset->ConstraintSetup[GetMachineConstraintIndex(Limb)]->DefaultInstance; }

//==============================================================================================================
//локальная позиция в осях родителя
//==============================================================================================================
FVector UMyrPhyCreatureMesh::GetLimbLocalOffset(const FLimb& Limb) const
{
	auto unconst_this = const_cast<UMyrPhyCreatureMesh*>(this);
	auto PLimb = unconst_this->GetLimbParent(Limb);
	const FVector Devia = unconst_this->GetMachineBody(const_cast<FLimb&>(Limb))->GetCOMPosition() - unconst_this->GetMachineBody(PLimb)->GetCOMPosition();
	return FVector
	(	
		UFastArcSin (FVector::DotProduct(GetLimbAxisRight(PLimb), Devia)),		// угол маха вперед-назад (вокруг оси вбок)
		UFastArcSin (FVector::DotProduct(GetLimbAxisForth(PLimb), Devia)),		// угол маха влево-вправо (вокруг оси вперед)
		FVector::DotProduct(GetLimbAxisDown(PLimb), Devia)						// просто длина вдоль оси вниз части тела - родителя
	);
}

//==============================================================================================================
// расстояние от ступни до пояса - мера вытяжки или поджатия ноги
//==============================================================================================================
float UMyrPhyCreatureMesh::GetFootDist(const FLimb& Limb) const
{
	auto unconst_this = const_cast<UMyrPhyCreatureMesh*>(this);
	auto PLimb = unconst_this->GetLimbParent(Limb);
	FVector Devia = unconst_this->GetMachineBody(const_cast<FLimb&>(Limb))->GetCOMPosition() - unconst_this->GetMachineBody(PLimb)->GetCOMPosition();

	//подразумевается, что каркас таза всегда ориентирован так, что ноги по вертикали
	return FVector::DotProduct(GetLimbAxisDown(PLimb), Devia);
}


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
	bCastDynamicShadow = false;
	bAffectDynamicIndirectLighting = true;
	SetCanEverAffectNavigation(false);

	//тик (частый не нужен, но отрисовка висит на нем, если не каждый кадр, будет слайд-шоу)
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	PrimaryComponentTick.EndTickGroup = TG_PostPhysics;
	PrimaryComponentTick.bTickEvenWhenPaused = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.SetTickFunctionEnable(false);
	PrimaryComponentTick.TickInterval = 10000;	//за каким-то хером он тичит на паузе, чтобы этого не было, до начала большой интервал
	PrimaryComponentTick.bHighPriority = false;

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
	PrimaryComponentTick.TickInterval = 0;
	SetComponentTickEnabled(true);

	//ЭТО МАРАЗМ! если это вызвать раньше, веса физики у тел будут нулевыми
	SetSimulatePhysics(true);

	//ЭТО МАРАЗМ! Если ввести труъ, то наоборот, веса учитываться не будут, и физика всегда будет на полную
	SetEnablePhysicsBlending(false);

	//присвоить правильный профиль столкновений сначала всем телам
	//(если присвоить для всего компонента, на тела это не окажет влияния)
	for (auto& B : Bodies)
	{	B->SetMaxDepenetrationVelocity(MyrOwner()->GenePool->MaxVelocityEver);
		B->SetCollisionProfileName(TEXT("PawnMeat"));
	}

	//выделить в отдельный профиль части тел - подставки
	for (int i = 0; i < (int)ELimb::NOLIMB; i++) InitLimbCollision (GetLimb((ELimb)i));

	//инициализировать и запустить пояса конечностей
	InitGirdle (gPelvis, true);
	InitGirdle (gThorax, true);
	Super::BeginPlay();
}

//==============================================================================================================
//каждый кадр
//==============================================================================================================
void UMyrPhyCreatureMesh::TickComponent (float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Bodies.Num() == 0) return;
	FrameCounter++;

	//для перерассчёта устойчивости по двум поясам конечностей
	float NewStandHardness[2] = {0,0};

	//обработать всяческую жизнь внутри разных частей тела (их фиксированное число)
	for (int i = 0; i < (int)ELimb::NOLIMB; i++)
		ProcessLimb (GetLimb((ELimb)i), DeltaTime, NewStandHardness);

	//обновление фоясоспецифичных коэффициентов устойчивости
	gPelvis.StandHardness = NewStandHardness[0];
	gThorax.StandHardness = NewStandHardness[1];

	//просчитать кучу всего для поясов конечностей
	ProcessGirdle (gPelvis, gThorax, DeltaTime, 1);
	ProcessGirdle (gThorax, gPelvis, DeltaTime, -1);

	FVector RootBodyPos = GetComponentLocation();
	//for(auto B:Bodies)
	//	DrawDebugLine(GetOwner()->GetWorld(), B->GetCOMPosition(), B->GetCOMPosition() + FVector::RightVector * 100 * B->PhysicsBlendWeight, FColor(0, 100, 0), false, 0.01f, 100, 1);
}

//==============================================================================================================
//удар твердлого тела
//==============================================================================================================
UFUNCTION() void UMyrPhyCreatureMesh::OnHit (UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
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
//приложение сил к каждому отдельному физ-членику
//==============================================================================================================
void UMyrPhyCreatureMesh::CustomPhysics(float DeltaTme, FBodyInstance* BodyInst)
{
	//auto Type = MyrOwner()->GetGenePool()->MachineBodies[BodyInst->InstanceBodyIndex].eLimb;
	auto Type = MyrOwner()->GetGenePool()->BodyBioData[BodyInst->InstanceBodyIndex].eLimb;
	auto Limb = GetLimb(Type);
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
	{	FVector GloAxis = G.AlignedDir * Revertor;
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
	{
		if (Limb.DynModel & LDY_ROTATE)
			BodyInst->AddTorqueInRadians (CalcAxisForLimbDynModel(Limb, FVector(0,0,0)) * Power * Revertor, false, true);
		else
			BodyInst->AddForce (CalcAxisForLimbDynModel(Limb, FVector(0, 0, 0)) * Power * Revertor, false, true);
	}

	//попытка насильственно ограничить превышение скорости как лекарство от закидонов физдвижка - мешает эффектно падать
/*	if (BodyInst->GetUnrealWorldVelocity().SizeSquared() >= FMath::Square(MyrOwner()->GetGenePool()->MaxVelocityEver))
	{	float VelNorm; FVector VelDir;
		BodyInst->GetUnrealWorldVelocity().ToDirectionAndLength(VelDir, VelNorm);
		VelDir *= MyrOwner()->GetGenePool()->MaxVelocityEver;
		BodyInst->SetLinearVelocity(VelDir, false);
		UE_LOG(LogTemp, Error, TEXT("%s: %s Max Velocity %g Off Limit %g"), *GetOwner()->GetName(), *TXTENUM(ELimb, Limb.WhatAmI), VelNorm, MyrOwner()->GetGenePool()->MaxVelocityEver);
	}*/
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
		BodyInst->AddTorqueInRadians(RollAxis * AdvMult * (1 - AsIdeal), false, true);
		DrawDebugLine(GetOwner()->GetWorld(), BodyInst->GetCOMPosition(), BodyInst->GetCOMPosition() + ActualAxis * 40, FColor(255, 0, 0, 255), false, 0.02f, 100, 1);
		DrawDebugLine(GetOwner()->GetWorld(), BodyInst->GetCOMPosition(), BodyInst->GetCOMPosition() + DesiredAxis * 50, FColor(255, 255, 0, 255), false, 0.02f, 100, 1);
	}

	//если требуется просто тянуть (это само по себе не ориентирование, но если есть привязи, они помогут)
	if (Limb.DynModel & LDY_PULL)
	{	BodyInst->AddForce(DesiredAxis * (1 - AsIdeal) * AdvMult, false, true);
		DrawDebugLine(GetOwner()->GetWorld(), BodyInst->GetCOMPosition(), BodyInst->GetCOMPosition() + DesiredAxis * 0.3 * Limb.Stepped, FColor(255, 255, 255, 255), false, 0.02f, 100, 1);
	}
}


//==============================================================================================================
//применить двигательную силу к телу, полагая, что это тело является безосевым колесом а-ля шарик ручки
//==============================================================================================================
void UMyrPhyCreatureMesh::Move(FBodyInstance* BodyInst, FLimb& Limb)
{
	const auto& G = GetGirdle(Limb);
	if(!G.CurrentDynModel->Leading) return;		//не двигать, если сам пояс конечностей пассивный
	if (MyrOwner()->Mobility<0.01) return;		//не двигать, если сам организм не хочет или не может

	//отрицательная обратная связь - если актуальная скорость меньше номинальной
	const float Underspeed = MyrOwner()->GetDesiredVelocity() - FVector::DotProduct (GetGirdle(Limb).VelocityAgainstFloor, G.AlignedDir);
	if (Underspeed>0)
	{
		//дополнительное усиление при забирании на склон, включается только если в гору, чем круче, тем сильнее
		float UpHillAdvForce = FMath::Max(0.0f, G.AlignedDir.Z) * MyrOwner()->BehaveCurrentData->UphillForceFactor;

		//ось вращения - если на опоре, то через векторное произведение (и ведь работает!) если в падении - то прямо вокруг курса (для управления вращением)
		//надо тут ввести иф и два разных варианта для колеса и для вращения в воздухе
		FVector Axis = Limb.Stepped ? FVector::CrossProduct(Limb.ImpactNormal, G.AlignedDir) : MyrOwner()->AttackDirection;

		//вращательное усилие 
		if (Limb.DynModel & LDY_ROTATE)
		{
			//ось вращения с длиной в виде силы вращения
			FVector Torque;

			//режим ноги на опоре - движение колесами
			if(Limb.Stepped)
				Torque = 
					FVector::CrossProduct(Limb.ImpactNormal, G.AlignedDir)	// из направления движения и нормали вот так легко извлекается ось вращения
					* Underspeed * AdvMoveFactor							// пропорционально недодаче скорости
					* (1.0f + UpHillAdvForce);								// дополнительное усиление при забирании на склон
			
			//режим управляемого падения - стараться закрутить его так чтобы компенсировать вращение остального туловища
			else Torque = MyrOwner()->AttackDirection * AdvMoveFactor * MyrOwner()->GetDesiredVelocity();

			BodyInst->AddTorqueInRadians( Torque, false, true);
			DrawDebugLine(GetOwner()->GetWorld(),
				BodyInst->GetCOMPosition(),	BodyInst->GetCOMPosition() + Torque,
				FColor(0, 25, 255, 50), false, 0.02f, 100, 1);
		}
		//поступательное усилие - только если касаемся опоры, ибо тяга, в отличие от вращения, будет искажать падение
		if ((Limb.DynModel & LDY_PULL) && Limb.Stepped)
		{
			if (G.StandHardness < 0.1) return;			
			FVector Direct = G.AlignedDir * Underspeed * AdvMoveFactor * (1.0f + UpHillAdvForce);
			BodyInst->AddForce(Direct, false, true);
			DrawDebugLine(GetOwner()->GetWorld(),
				BodyInst->GetCOMPosition(), BodyInst->GetCOMPosition() + Direct,
				FColor(0, 255, 255, 50), false, 0.02f, 100, 1);
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
	auto CLimb = GetGirdleLimb(G, EGirdleRay::Center);

	//насколько главное тело пояса лежит на боку - если сильно на боку, то прыгать трудно
	float SideLie = FMath::Abs(GetLimbAxisRight(CLimb) | CLimb.ImpactNormal);
	float UnFall = 1.0 - SideLie;

	//особый режим - зажата кнопка "крабкаться", но опора не позволяет - всё равно встаем на дыбы и прыгаем почти точно вверх
	if(G.SeekingToCling && !G.Clung) HorDir*=0.1;

	//суммарное направление прыжка
	FVector Dir = HorDir * HorVel + FVector::UpVector * (UpVel);

	//если пояс слабо стоит или вообще поднят в воздух, то применять ипульс слабо или ообще не применять
	//это, как выяснилось, вредно, так как приводит к перекосам в воздухе
	//Dir *= FMath::Max(1.0f, G.StandHardness);

	//резко отключить пригнутие, разогнуть ноги во всех частях тела
	G.Crouch = 0.0f;

	//применить импульсы к ногам - если лежим, то слабые, чтобы встало сначала туловище
	PhyJumpLimb ( GetGirdleLimb(G, EGirdleRay::Left), Dir * UnFall);
	PhyJumpLimb ( GetGirdleLimb(G, EGirdleRay::Right), Dir * UnFall);

	//применить импульсы к туловищу
	//ослабление, связанное с лежанием, к туловищу не применяется, чтобы мочь встать, извиваясь
	PhyJumpLimb ( GetGirdleLimb(G, EGirdleRay::Spine), Dir);
	PhyJumpLimb ( CLimb, Dir );
}


//==============================================================================================================
//прыжковый импульс для одной части тела
//==============================================================================================================
void UMyrPhyCreatureMesh::PhyJumpLimb(FLimb& Limb, FVector Dir)
{
	//сбросить индикатор соприкоснутости с опорой
	Limb.Stepped = 0;

	//если в этой части тела вообще есть физ-тело каркаса
	if(HasPhy(Limb.WhatAmI))
	{	
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
		//если хоть один из поясов конечностей уже стоит на лазибельной поверхности
		if(CanGirdleCling (gThorax) || CanGirdleCling(gPelvis) )
		{
			//то ли выйти, то ли сразу вписать сюда процедуру зацепа
		}
		//если никакая часть тела не касается нужной поверхности
		else
		{	//найти специальное самодействие, отражающее вставание на дыбы, чтобы зацепиться передними лапами за вертикаль
			MyrOwner()->SelfActionFindStart (ECreatureAction::SELF_PRANCE1);
		}
	}
}


//==============================================================================================================
//в ответ на касание членика этой части тела
//==============================================================================================================
void UMyrPhyCreatureMesh::HitLimb (FLimb& Limb, uint8 DistFromLeaf, const FHitResult& Hit, FVector NormalImpulse)
{
	//столконвение с плотью, не с поддержкой (если включены для этого членика)
	if (DistFromLeaf != 255)
	{
		//повысить уровень физики до максимума, чтобы показывался огиб препятствия
		PhyDeflect(Limb, 1.0);
	}
	
	//общая инфа по конечности для данного вида существ
	const auto LimbGene = MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Limb.WhatAmI).Machine;

	//пояс, к которому принадлежит эта нога
	auto& G = GetGirdle(Limb);

	//определить абсолютную скорость нас и предмета, который нас задел, в точке столконвения
	FVector FloorVel = Hit.Component.Get() -> GetPhysicsLinearVelocityAtPoint (Hit.ImpactPoint);
	FVector MyVel = GetPhysicsLinearVelocityAtPoint(Hit.ImpactPoint);

	//вектор относителой скорости и его квадрат для первичных прикидок
	FVector RelativeVel = MyVel - FloorVel;
	float RelSpeed = RelativeVel.SizeSquared();

	//если опора скелетная (дерево, другая туша) определить также номер физ-членика опоры
	auto NewFloor = Hit.Component.Get()->GetBodyInstance();

	//пометить, что произошло касание
	Limb.Stepped = 255;

	//сохранить тип поверхности для обнаружения возможности зацепиться
	auto FM = Hit.PhysMaterial.Get();
	if(FM) Limb.Surface = (EMyrSurface)FM->SurfaceType.GetValue();

	//если обнаруживается новая опора, дать об этом знать
	if(Limb.Floor != NewFloor)
	{	
		//момент смены опоры можно отследить только здесь - включение или обновление зацепа
		//внимание, IsClimbable проверяет свойство Surface, которое уже было присвоено выше, так что
		//мы тестируем лазкость уже новой опоры, хотя указатель здесь по прежнему на старую
		if (LimbGene.CanCling && G.SeekingToCling && Limb.IsClimbable())
		{	if(G.CurrentDynModel->Leading || RelSpeed < 1)
				ClingLimb(Limb, NewFloor);
		}
		else Limb.Floor = NewFloor;
	}

	//зарегистрировать прикосновение к части тела
	Limb.ImpactNormal = Hit.Normal;

	// импульс удара - если выше порога, покалечиться, если немного ниже - подогнуть ноги
	if (ApplyHitDamage(Limb, LimbGene.HitShield, RelSpeed, Hit) > 0.3)
	{
		//если есть констрейнт (у корневого тела его видимо нет)
		if (GetMachineConstraintIndex(Limb) != 255)
		{
			//распознаём, что это конечность по наличию линейного мотора - и прижимаем конечность к телу
			auto CI = GetMachineConstraint(Limb);
			if (CI->ProfileInstance.LinearDrive.ZDrive.bEnablePositionDrive)
				CI->SetLinearPositionTarget(FVector(0, 0, -CI->ProfileInstance.LinearLimit.Limit));
		}
	}
	//UE_LOG(LogTemp, Log, TEXT("%s: HitLimb %s part %d"), *GetOwner()->GetName(), *TXTENUM(ELimb, Limb.WhatAmI), DistFromLeaf);

}

//==============================================================================================================
//рассчитать возможный урон от столкновения с другим телом - возвращает истину, если удар получен
//==============================================================================================================
float UMyrPhyCreatureMesh::ApplyHitDamage(FLimb& Limb, float TerminalDamageMult, float SpeedSquared, const FHitResult& Hit)
{
	//если нас ударило другое существо, тут может быть другая логика
	//вообще непонятно, как делить физ-членики опоры и ударных частей тела
	if (auto Skel = Cast<UMyrPhyCreatureMesh>(Hit.Component.Get())) 
	{
		//найти по имени ударившей нас кости, что это такое в координатах анатомии
		FBODY_BIO_DATA bbd = Skel->MyrOwner()->GetGenePool()->BodyBioData[Skel->GetBodyInstance(Hit.BoneName)->InstanceBodyIndex];
		FLimb& TheirLimb = Skel->GetLimb(bbd.eLimb);
		FBodyInstance* TheirBody = bbd.DistFromLeaf == 255 ? Skel->GetMachineBody(TheirLimb) : GetFleshBody(TheirLimb, bbd.DistFromLeaf); 

		//если противоположное существо выполняет какую-то атаку
		if(!Skel->MyrOwner()->NoAction())
		{	
			//добыть данные по атаки, а из них узнать, является ли ударивший член - активным боевым членом атаки
			auto TheirAttack = Skel->MyrOwner()->GetCurrentActionInfo();
			if(auto Phases = TheirAttack->WhatLimbsUsed.Find(bbd.eLimb))
			{
				//если в добавок этот боевой член таков в том числе в ту фазу атаки, которая происходит сейчас
				if(Phases->Phases & (int32)Skel->MyrOwner()->CurrentAttackPhase)
				{
					//общие настроки для этой части тела
					const auto TheirLimbGene = Skel->MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Limb.WhatAmI).Machine;

					//прямой урон от касания
					Limb.Damage += TheirAttack->TactileDamage * TheirLimbGene.HitShield;
					UE_LOG(LogTemp, Error, TEXT("%s: Violent Damage %s from %s, scale = %g"),
						*GetOwner()->GetName(),
						*TXTENUM(ELimb, Limb.WhatAmI),
						*TXTENUM(ELimb, bbd.eLimb),
						TheirAttack -> TactileDamage);
				}
			}
		}
	}

	//квадрат порога 
	float Thresh = MyrOwner()->Params.MaxSafeShock * MyrOwner()->Params.MaxSafeShock * TerminalDamageMult;

	//если даже 1/10 порога не достигнута, дальше вообще не считать
	if (SpeedSquared < Thresh * 0.1) return 0.0f; else
	{
		//удельная сила
		float Severity = SpeedSquared / Thresh;
		if (Severity > 1.0f)
		{
			float NewDamage = SpeedSquared / Thresh - 1.0f;
			Limb.Damage += NewDamage;
			UE_LOG(LogTemp, Error, TEXT("%s: Damage %s, Speed = %g, scale = %g"),
				*GetOwner()->GetName(),
				*TXTENUM(ELimb, Limb.WhatAmI),
				SpeedSquared, NewDamage);

			//поврежденную часть тела заставить висеть 
			PhyDeflect(Limb, Limb.Damage);
		}
		return Severity;
	}
	return 0.0f;
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
void UMyrPhyCreatureMesh::InitLimbCollision (FLimb& Limb)
{
	//если присутствует элемент каркаса-поддержки
	auto bi = GetMachineBodyIndex(Limb);
	if (bi < 255)
	{	
		//закрепить за телами поддержки профиль столкновений, чтобы они не сталкивались с "мясом"
		Bodies[bi]->SetCollisionProfileName(TEXT("PawnSupport"));
	}
}

//==============================================================================================================
//вычислить предпочительное направление движения для этого членика по его опоре
//==============================================================================================================
FVector UMyrPhyCreatureMesh::CalculateLimbGuideDir(FLimb& Limb, const FVector& PrimaryCourse, float* Stability)
{
	//накопитель направления, разные случаи плюют сюда свои курсы
	FVector PrimaryGuide = PrimaryCourse;

	//если тело прицепилось к опоре + крутая опора (ствол/стена), вектор взгляда давит, а не катит вдоль
	if(Limb.Clung && Limb.ImpactNormal.Z<0.5)
	{
		//плавно превращаем степень ортогональности в степень отклонения в глобальную вертикаль
		float BumpInAmount = FVector::DotProduct(-Limb.ImpactNormal, PrimaryGuide);
		FVector ReflectedMotion = PrimaryGuide * FMath::Abs(BumpInAmount);
		ReflectedMotion.Z = 2 * BumpInAmount * (0.5f - Limb.ImpactNormal.Z);
		PrimaryGuide = FVector::VectorPlaneProject(ReflectedMotion, Limb.ImpactNormal);
	}

	//если стоим на цилиндрической поверхности - направить вдоль (неважно, карабкаемся или просто идем)
	//если это ствол вверх, необходимый разворот (перед-верх) уже был сделан выше
	if(Limb.OnCapsule())
	{
		//толщина самой узкой метрики опоры
		const float Curvature = FMath::Max(GetGirdle(Limb.WhatAmI).Curvature, 0.0f);

		//направление вперед строго вдоль ветки, без учёта склона
		const FVector BranchDir = Limb.GetBranchDirection (PrimaryGuide);

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

		FVector Ct = GetMachineBody(Limb)->GetCOMPosition();
		DrawDebugLine(GetOwner()->GetWorld(), Ct, Ct + BranchDir * 15, FColor(0, 0, 0), false, 0.02f, 100, 1);
		DrawDebugLine(GetOwner()->GetWorld(), Ct, Ct + BranchTangent * WantToSupport * 15, FColor(0, 0, 0), false, 0.02f, 100, 1);

	}
	//общий случай - спроецировать на плоскость нормали, чтобы не давило и не отрывало от поверхности
	else PrimaryGuide = FVector::VectorPlaneProject(PrimaryGuide, Limb.ImpactNormal);

	//степень сходности (неотклонения) получившегося курса от изначально желаемого направления
	if(Stability) *Stability = FVector::DotProduct(PrimaryGuide, PrimaryCourse);

	//это полуфабрикат, в поясе будет суммироваться со второй ногой, так что нормировать пока не надо
	return PrimaryGuide;
}
//==============================================================================================================
//основная покадровая рутина в отношении части тела
//==============================================================================================================
void UMyrPhyCreatureMesh::ProcessLimb (FLimb& Limb, float DeltaTime, float* NewStandHardnesses)
{
	//общая инфа по конечности для данного вида существ
	const auto LimbGene = MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Limb.WhatAmI).Machine;

	//пояс, к которому принадлежит эта нога
	auto& G = GetGirdle(Limb);

	//канал аккумулятора устойчивости, специфичный для пояса, будет заполняться здесь 
	float& StandHardness = NewStandHardnesses[Section[(int)Limb.WhatAmI]];

	//если с этой частью тела ассоциирован элемент каркаса/поддержки
	if (LimbGene.TipBodyIndex != 255)
	{
		//физическое тело, ассоциированное с этой частью тела
		auto BI = GetMachineBody(Limb);
		auto ABI = GetArchMachineBody(Limb);

		//физическая привязь, ассоциированная с этой частью тела
		//auto CI = GetMachineConstraint(Limb);
		//auto ACI = GetArchMachineConstraint(Limb);

		//если эта часть тела зацеплена, есть некоторые критерии, при которых ее все же надо отцепить
		if(Limb.Clung)
		{
			//внешне поданная отмена зацепа
			if (!G.SeekingToCling)	UnClingLimb(Limb);

			//при движении не давать ведомым ногам цепляться, чтоб не создавать непредсказуемых натяжений
			//if(!G.CurrentDynModel->Leading && MyrOwner()->ExternalGain>0) UnClingLimb(Limb);
		}


		//если ранее быз щафиксирован шаг/касание к этой части тела
		//иначе данная куча фигни не нужна
		if (Limb.Stepped)
		{
			//скорость нужна здесь только для детекции потери опоры
			FVector RelativeVelocity = GetMachineBody(Limb)->GetUnrealWorldVelocity();

			//если есть апора - скорость, возможно, стоит отсчитывать от опоры, а не брать абсолютную
			if (Limb.Floor->OwnerComponent.Get()->Mobility == EComponentMobility::Movable)
				RelativeVelocity -= Limb.Floor->GetUnrealWorldVelocityAtPoint(GetMachineBody(Limb)->GetCOMPosition());
			float RelSpeedSq = RelativeVelocity.SizeSquared();

			//плавное снижение желания отдернуться от препятствия (зависит от скорости - стоит ли извлекать корень или оставить бешенные нули?)
			float Drift = DeltaTime * DeltaTime * RelSpeedSq;
			uint8 NewStepped = Drift > 0.1f ? Limb.Stepped >> 1 : Limb.Stepped;

			//пока тело чувствует опору и продолжит ближайший такт
			if (NewStepped > 0)
			{
				//внесение вклада в определение устойчивости (пока только библиотечный коэффициент, нужен еще наклон нормали)
				StandHardness += FMath::Abs(LimbGene.TouchImportance);

				//включить/обновить зацеп при неизменной опоре (только передний)
				if (G.CurrentDynModel->Leading && LimbGene.CanCling && G.SeekingToCling && Limb.IsClimbable())
					ClingLimb(Limb, Limb.Floor);
			}
			//если тело в этот момент будет в воздухе
			else
			{
				//сразу по отрыву обрать данные о поверхности, чтобы никто не счёт воздух лазибельным
				Limb.Surface = EMyrSurface::Surface_0;

				//если конечность была прицеплена, отцепить, ибо порвалась связь с опорой
				if(Limb.Clung) UnClingLimb(Limb);
			}
			Limb.Stepped = NewStepped;
		}

		//постепенное исцеление от урона (за счёт сокращения здоровья)
		if (Limb.Damage > 0)
		{
			Limb.Damage = Limb.Damage - DeltaTime * HealRate;
			if (Limb.Damage <= 0.0f) Limb.Damage = 0.0f;
			else MyrOwner()->Health -= DeltaTime * HealRate;
		}

		//запускаем на этот кадр доп-физику исключительно для членика-поддержки
		BI->AddCustomPhysics (OnCalculateCustomPhysics);
	}

	//регулировать веса анимации плоти
	ProcessBodyWeights(Limb, DeltaTime);
}

//==============================================================================================================
//отдельная рутина для конкретно ног - возвращает, на опора нога или в воздухе
//==============================================================================================================
bool UMyrPhyCreatureMesh::ProcessFoot (FLimb& Limb, float Damping, float AbsoluteCrouch, float DeltaTime)
{
	//привязь этой ноги
	auto Cons = GetMachineConstraint(Limb);

	//пояс, к которому принадлежит эта нога
	auto G = GetGirdle(Limb);

	//настроить вытянутость ноги от туловища (значение посчитано для пары ног, так что идёт извне)
	Cons->SetLinearPositionTarget (FVector(0, 0, AbsoluteCrouch));

	//спуститься на уровень тел
	auto BI = GetMachineBody(Limb);

	//если нога на опоре
	if(Limb.Stepped)
	{
		//предопределенное в редакторе значение для ножной пружины
		auto ArchDrive = GetArchMachineConstraint(Limb)->ProfileInstance.LinearDrive;
		
		//типа в прицепленном состоянии тормозить вязкостью не нужно
		//костыль, чтобы не раскорячивался, а подтягивал задние ноги
		//if (Limb.Clung) Damping = 0;

		//сбавлять тормоз сразу - для разгона, а наращивать плавно - тормозной путь
		BrakeD ( BI->LinearDamping, Damping, DeltaTime);
		BrakeD ( BI->AngularDamping, Damping*10, DeltaTime);
		BI->UpdateDampingProperties();

		//если включены "тяжелые ноги" удельный вес ног все же зависит от их повреждений - чтобы при сильном увечьи легче валиться на бок
		if(G.CurrentDynModel->HeavyLegs) BI->SetMassScale(10 - 5 * FMath::Min(Limb.Damage, 1.0f));

		//настроить силу, с которой нога упирается в опору
		Cons->SetLinearDriveParams ( 
			ArchDrive.ZDrive.Stiffness							// амплитуда - берется из редактора
				* FMath::Max(0.0f, 0.5f*Limb.ImpactNormal.Z		// чем круче опора, тем меньше сила - видимо, чтобы на горке ноги повторяли кочки
				- Limb.Damage									// при повреждении ноги удерживть тело на весу трудно
				- 0.5f*G.Crouch),								// пригнутость и занижает, и ослабляет - чтоб ноги на склоне вытягивались до касания с опорой
			10 + 100*G.Clung,									// инерция скорости, чтоб не пружинило, но пока неясно, как этим управлять
			ArchDrive.ZDrive.MaxForce);							// тупо копируем из редактора
		//DrawDebugLine(GetOwner()->GetWorld(), BI->GetCOMPosition(), BI->GetCOMPosition() + Limb.ImpactNormal*5, FColor(0, 0, 0, 10), false, 0.02f, 100, 1);
		return true;
	}
	//если нога в воздухе и если постоянные значения ЕЩЁ не применены (пока неясно, нужны ли изменения при падении)
	else if(BI->LinearDamping > 0.01f)
	{
		//сбросить почти всю туготу в движении тела
		SetLimbDamping (Limb, 0.01, 0);
	}
	return false;
}

//==============================================================================================================
//прицепиться к опоре и сопроводить движение по опоре перезацепами
//==============================================================================================================
bool UMyrPhyCreatureMesh::ClingLimb (FLimb& Limb, FBodyInstance* NewFloor)
{
	//к какому поясу принадлежит данная конечность
	FGirdle& G = GetGirdle(Limb.WhatAmI);

	//эксперимент - цепляться только одной лапой, вторая на подхвате
	//if(!Limb.Clung && G.Clung) return false;

	//всякие нужные указатели
	const auto LimbGene = MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Limb.WhatAmI).Machine;
	auto LBody = GetMachineBody(Limb);
	auto& CI = LBody->DOFConstraint;
	FVector At = LBody->GetCOMPosition();

	//квант беспечного перемещения
	float Luft = 10;

	//если констрейнт еще не создан в памяти
	if (!CI) { CI = FConstraintInstance::Alloc();
		CI->ProfileInstance.ConeLimit.bSoftConstraint = false;
		CI->ProfileInstance.TwistLimit.bSoftConstraint = false;
		CI->ProfileInstance.LinearLimit.bSoftConstraint = false;
	}

	//если происходит акт обновления опоры или включения ранее выключенного зацепа
	if (NewFloor != Limb.Floor || Limb.Clung == false)
	{	
		CI->TermConstraint();
		Limb.Floor = NewFloor;
		Limb.Clung = true;
		Limb.Stepped = 255;

		//установка пружинной силы только по оси лево-право, чтобы упругость в поворотах
		CI->SetLinearPositionDrive(false, false, true);
		CI->SetLinearDriveParams(10000, 100, 70000);
		CI->SetLinearPositionTarget(FVector(-Luft, 0, 0));

		//надо запросить смену состояния
		MyrOwner()->QueryBehaveState(EBehaveState::climb);
		G.Clung = true;
	}
	//если опоро та же, и мы обновляем уже включенный зацеп
	else
	{
		//если мы отошли от прошлой точки зацепа на слишком малое расстояние, пожалеть проц, не перестраивать
		FVector OldPos = Limb.Floor->GetUnrealWorldTransform().TransformPosition(CI->Pos1);
		const float Devia = FVector::DistSquared(At, OldPos);
		if (Devia < Luft * Luft * 0.25)
			return false;
	}

	//SetAngles(CI, TWIST, 0);		// рысканье - свободно, чтоб поворачиваться и цепляться с разных углов
	//SetAngles(CI, SWING1, 0);		// остальные пока закрепить, чтобы спина держалась, хотя неправильно
	//SetAngles(CI, SWING2, 0);		// остальные пока закрепить, чтобы спина держалась, хотя неправильно
	//CI->UpdateAngularLimit();

	//если тяга к движению нулевая, усилить привязь, чтобы вообще застыло на месте
	if (MyrOwner()->ExternalGain <= 0)
	{	if (CI->GetLinearYMotion() != ELinearConstraintMotion::LCM_Locked)
		{	SetLinear(CI, 0, 0, 0);
			CI->SetLinearVelocityDrive(false, true, false);
		}
	}
	//если пользователь включил тягу, ослабить привязь по оси намеренного движения
	else
	{	if (CI->GetLinearYMotion() != ELinearConstraintMotion::LCM_Free)
		{	SetLinear(CI, 0, -1, Luft * FMath::Max(0.0f, 1.0f - 3.0f*G.Curvature));
			CI->SetLinearVelocityDrive(false, false, false);
		}
	}

	//базис: вперед (Sec) = суммарное направление вперед по всему поясу, оно может не быть ортогонально нормали
	//поэтому дополнительно спроецировать и нормализовать
	FVector Pri = Limb.ImpactNormal;
	FVector Sec = FVector::VectorPlaneProject(G.AlignedDir, Limb.ImpactNormal); 
	Sec.Normalize();
	auto TM2 = Limb.Floor->GetUnrealWorldTransform();
	auto TM1 = LBody->GetUnrealWorldTransform();

	//координаты новой точки в системе опоры, центр констрейнта - в центре туловища TM1.GetLocation()
	CI->PriAxis1 = TM1.InverseTransformVectorNoScale(Pri);
	CI->SecAxis1 = TM1.InverseTransformVectorNoScale(Sec);
	CI->PriAxis2 = TM2.InverseTransformVectorNoScale(Pri);
	CI->SecAxis2 = TM2.InverseTransformVectorNoScale(Sec);
	CI->Pos2 = TM2.InverseTransformPosition(At);
	CI->Pos1 = TM1.InverseTransformPosition(At);
	DrawDebugLine(GetOwner()->GetWorld(), At, At + Sec * 10, FColor(255, 0, 0), false, 1.01f, 255, 2);
	DrawDebugLine(GetOwner()->GetWorld(), At, At + Pri * 10, FColor(0, 0, 255), false, 1.01f, 255, 2);

	//если происходит создание зацепа с нуля
	if (CI->IsTerminated())
	{
		//полная перестройка
		CI->InitConstraint(LBody, Limb.Floor, 1.0f, this);
		UE_LOG(LogTemp, Error, TEXT("%s: ClingLimb %s Recreate"), *GetOwner()->GetName(), *TXTENUM(ELimb, Limb.WhatAmI));
		return true;
	}

	//если происходит обновление зацепа
	else
	{
		//дальше выкорчёвывается из исходников способ обращения с нижележащим слоем абстракции и применяется в лоб
		FPhysicsInterface::ExecuteOnUnbrokenConstraintReadWrite(CI->ConstraintHandle, [&](const FPhysicsConstraintHandle& InUnbrokenConstraint)
		{	FPhysicsInterface::SetLocalPose(InUnbrokenConstraint, FTransform(CI->PriAxis1, CI->SecAxis1, CI->PriAxis1 ^ CI->SecAxis1, CI->Pos1), EConstraintFrame::Frame1);
			FPhysicsInterface::SetLocalPose(InUnbrokenConstraint, FTransform(CI->PriAxis2, CI->SecAxis2, CI->PriAxis2 ^ CI->SecAxis2, CI->Pos2), EConstraintFrame::Frame2);	});
		UE_LOG(LogTemp, Error, TEXT("%s: ClingLimb %s Pass"), *GetOwner()->GetName(), *TXTENUM(ELimb, Limb.WhatAmI));
		return false;
	}

}

//==============================================================================================================
//отцепиться
//==============================================================================================================
void UMyrPhyCreatureMesh::UnClingLimb(FLimb& Limb)
{
	auto LBody = GetMachineBody(Limb);
	auto& CI = LBody->DOFConstraint;
	if (CI)
	{
		//расслабляем все параметры зацепа, но сам зацеп пока не рвём (зачем?)
		Limb.Clung = false;
		SetFreedom(CI, -1, -1, -1, 180, 180, 180);
		CI->SetLinearPositionDrive(false, false, false);
		CI->SetLinearVelocityDrive(false, false, false);
		UE_LOG(LogTemp, Error, TEXT("%s: UnClingLimb %s"), *GetOwner()->GetName(), *TXTENUM(ELimb, Limb.WhatAmI));

		//проверяем, как с другими членами пояса - если этот разрыв последний, отменить глобальный режим карабканья
		auto& G = GetGirdle(Limb);
		if (!GetGirdleLimb(G, EGirdleRay::Center).Clung
			&& !GetGirdleLimb(G, EGirdleRay::Right).Clung 
			&& !GetGirdleLimb(G, EGirdleRay::Left).Clung)
		{	G.Clung = false;
			MyrOwner()->AdoptBehaveState(EBehaveState::fall);
		}
	}
}


//==============================================================================================================
//взять данный предмет за данный членик данной частью тела
//здесь не проверяется, может ли часть тела взять
//==============================================================================================================
void UMyrPhyCreatureMesh::Grab (FLimb& Limb, UPrimitiveComponent* Item, int8 ItemBody)
{
	//общая инфа по конечности для данного вида существ
	const auto LimbGene = MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Limb.WhatAmI);
	auto LBody = GetMachineBody(Limb);
	auto& CI = LBody->DOFConstraint;
	auto TM1 = LBody->GetUnrealWorldTransform();

	//точное место, к которому следует закрепить поднятый предмет
	FVector At = GetSocketLocation(LimbGene.Flesh.GrabSocket);

	//если констрейнт еще не создан в памяти
	if(!CI)
	{	CI = FConstraintInstance::Alloc();
		CI->ProfileInstance.ConeLimit.bSoftConstraint = false;
		CI->ProfileInstance.TwistLimit.bSoftConstraint = false;
		CI->ProfileInstance.LinearLimit.bSoftConstraint = false;
	}

	//тело, за которое хватаем
	FBodyInstance* IBody = GetBody(Item,ItemBody);
	auto TM2 = IBody->GetUnrealWorldTransform();

	//пока неясно, как лучше оси располагать, 
	FVector Pri = FVector::UpVector;
	FVector Sec = FVector::RightVector;

	//координаты новой точки в системе опоры, центр констрейнта - в центре туловища TM1.GetLocation()
	CI->PriAxis1 = TM1.InverseTransformVectorNoScale(Pri);
	CI->SecAxis1 = TM1.InverseTransformVectorNoScale(Sec);
	CI->Pos1 = TM1.InverseTransformPosition(At);
	CI->PriAxis2 = TM2.InverseTransformVectorNoScale(Pri);
	CI->SecAxis2 = TM2.InverseTransformVectorNoScale(Sec);
	CI->Pos2 = TM2.InverseTransformPosition(At);
	CI->InitConstraint(LBody,IBody, 1.0f, this);
	Limb.Grabs = true;
}

void UMyrPhyCreatureMesh::UnGrab (FLimb& Limb)
{
	if(!Limb.Grabs) return;

	//общая инфа по конечности для данного вида существ
	const auto LimbGene = MyrOwner()->GenePool->Anatomy.GetSegmentByIndex(Limb.WhatAmI);
	auto LBody = GetMachineBody(Limb);
	auto& CI = LBody->DOFConstraint;

	//разорвать связь
	CI->TermConstraint();
	Limb.Grabs = false;

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
	BI->AngularDamping = Angular;
	BI->UpdateDampingProperties();
}

//==============================================================================================================
//инициализировать пояс конечностей при уже загруженной остальной физике тела (ноги есть/нет, DOF Constraint)
//==============================================================================================================
void UMyrPhyCreatureMesh::InitGirdle (FGirdle& Girdle, bool StartVertical)
{
	//если в физической сборке-каркасы присутствует хотя бы один физ-членик для ног этого пояса
	if(GetMachineBodyIndex (GetGirdleRay(Girdle, EGirdleRay::Right)) < 255)
		Girdle.HasLegs = true;

	//центральное подвижное тело, к которому крепятся ноги
	auto CentralBody = GetGirdleBody (Girdle, EGirdleRay::Center );
	auto SpineBody = GetGirdleBody (Girdle, EGirdleRay::Spine );

	//чтобы уловые части тела не могли бешено вращаться при смене ограничений, разрушая всю остальную физику
	CentralBody->SetMaxAngularVelocityInRadians (10 , false);
	SpineBody->SetMaxAngularVelocityInRadians (MyrOwner()->GenePool->MaxVelocityEver*10, false);
}


//==============================================================================================================
//для этого конкретного пояса конечностей установить ведщесть/ведомость
//==============================================================================================================
void UMyrPhyCreatureMesh::SetGirdleLeading(FGirdle& Girdle, bool Set)
{
	//если у пояса нет конечностей, то можно сразу выходить, локомоция осуществляется прямой тягой
	if (!Girdle.HasLegs) { return; }
	Girdle.CurrentDynModel->Leading = Set;

	//привязи ног
	auto LCons = GetMachineConstraint(GetGirdleRay(Girdle, EGirdleRay::Left));
	auto RCons = GetMachineConstraint(GetGirdleRay(Girdle, EGirdleRay::Right));

	//оси вращения кроме "вперед" блокируются у ведомой секции, чтобы она туго поддавалась вне "рельсов"
	auto Param = Set ? EAngularConstraintMotion::ACM_Free : EAngularConstraintMotion::ACM_Locked;
	LCons->SetAngularSwing1Limit(Param, 0.0f); 		LCons->SetAngularSwing2Limit(Param, 0.0f);
	RCons->SetAngularSwing1Limit(Param, 0.0f);		RCons->SetAngularSwing2Limit(Param, 0.0f);
}

//==============================================================================================================
//модифицировать вектор, заменив, если нужно, нормалью с седла ветки, на которой сидим - чтобы (если не совсем вертикально) спина не кренилась
//==============================================================================================================
void UMyrPhyCreatureMesh::SetLimbOnBranchSaddleUp(FLimb& LimbOnBranch, FVector& DstUp, const FVector DefaultUp)
{
	auto Ct = GetGirdleBody(GetGirdle(LimbOnBranch), EGirdleRay::Center)->GetCOMPosition() + FVector::UpVector * 15;
/*	if (LimbOnBranch.Clung && LimbOnBranch.OnBranch() && LimbOnBranch.OnCapsule())
	{
		FVector GuideSaddleUp = LimbOnBranch.GetBranchSaddleUp();
		if (GuideSaddleUp.Z > 0.2)
		{
			DstUp = GuideSaddleUp;
			DrawDebugLine(GetOwner()->GetWorld(), Ct, Ct + DstUp*30, FColor(255, 0, 44), false, 0.02f, 100, 3);
		}
		else DstUp = DefaultUp;
	}
	else */
		DstUp = DefaultUp;
	DrawDebugLine(GetOwner()->GetWorld(), Ct, Ct + DefaultUp * 30, FColor(155, 0, 44), false, 0.02f, 100, 3);
}

void UMyrPhyCreatureMesh::ProcessGirdleFoot (FGirdle& Girdle, FLimb& FootLimb, FLimb& CentralLimb, float DampingForFeet, FVector PrimaryForth, float &WeightAccum, bool& NeedForVertical, float DeltaTime)
{
	//физическое тело ноги
	FBodyInstance* FootBody = GetMachineBody(FootLimb);
	float ClampedDamage = FMath::Min(FootLimb.Damage, 1.0f);

	//нога на опоре, принимает участие в позиционировании тела
	if(FootLimb.Stepped)
	{
		//относительная скорость условно в точке ноги (просто плавно набрасываем новые значения через Lerp)
		FVector Point = FootBody->GetCOMPosition();
		FVector RelativeSpeedAtPoint = FootBody->GetUnrealWorldVelocity() - FootLimb.Floor->GetUnrealWorldVelocityAtPoint (Point);
		Girdle.VelocityAgainstFloor = FMath::Lerp( Girdle.VelocityAgainstFloor, RelativeSpeedAtPoint, 0.1f);

		//направление движения из опоры данной ноги + насколько оно искажает первоначальный путь
		float UnBias = 1.0f; FVector GuideDir = CalculateLimbGuideDir ( FootLimb, PrimaryForth, &UnBias);
		if(UnBias < 0.0f) UnBias = 0.0f;

		//если на уровне всего пояса мы цепляемся, но конкретно эта нога стоит на нецепляемой опоре - занизить ее вес
		if(Girdle.Clung && !FootLimb.IsClimbable())	UnBias *= 0.1f;

		//если этот вес первый, или просто больший предыдущего - влить фиктивную опору всего пояса из этой ноги
		if(WeightAccum < UnBias)
			CentralLimb.Floor = FootLimb.Floor;
		WeightAccum += UnBias;

		//взвешенное аддитивное накопление курса (потом будет нормированно, 10, чтобы избежать неточностей на мелких весах
		Girdle.AlignedDir += GuideDir * (0.2f + UnBias) * 10.0f;

		//дополнить аддитивно новую нормаль (возможно, еще рано на этой стадии)
		//CentralLimb.ImpactNormal += FootLimb.ImpactNormal;

		//дополнительные условия по поддержке жетской вертикали
		NeedForVertical = NeedForVertical
			&& FootLimb.ImpactNormal.Z > 0.5			// склон достаточно пологий
			&& FootLimb.Damage < 0.5f;					// ноги не сильно повреждены


		//////////////////////////////////////////////////////////////////////////////
		//применение вязкости среды для колеса ноги - торможение
		//сбавлять тормоз сразу - для разгона, а наращивать плавно - тормозной путь
		BrakeD ( FootBody->LinearDamping, DampingForFeet, DeltaTime);
		BrakeD ( FootBody->AngularDamping, DampingForFeet*10, DeltaTime);
		FootBody->UpdateDampingProperties();

		//если включены "тяжелые ноги" удельный вес ног все же зависит от их повреждений - чтобы при сильном увечьи легче валиться на бок
		if(Girdle.CurrentDynModel->HeavyLegs) FootBody->SetMassScale(10 - 5 * ClampedDamage);

		/////////////////////////////////////////////////////////////////////////////
		// привязь этой ноги к туловищу
		auto Cons = GetMachineConstraint(FootLimb);	
		auto ARCons = GetArchMachineConstraint(FootLimb);

		//рассчёт уровня прижатия ног
		Girdle.Crouch = FMath::Lerp (Girdle.Crouch, Girdle.CurrentDynModel->Crouch, DeltaTime);
		float FreeFeetLength = FMath::Abs(ARCons->ProfileInstance.LinearDrive.PositionTarget.Z);/*FMath::Abs(ARCons->ProfileInstance.LinearLimit.Limit);*/
		float AbsoluteCrouch = FreeFeetLength * ( 1 - 2*FMath::Max(Girdle.Crouch, ClampedDamage));
		Cons->SetLinearPositionTarget (FVector(0, 0, AbsoluteCrouch));

		//настроить силу, с которой нога упирается в опору
		Cons->SetLinearDriveParams ( 
			ARCons->ProfileInstance.LinearDrive.ZDrive.Stiffness// амплитуда - берется из редактора
				* FMath::Max(0.0f, 0.5f*FootLimb.ImpactNormal.Z	// чем круче опора, тем меньше сила - видимо, чтобы на горке ноги повторяли кочки
				- FootLimb.Damage								// при повреждении ноги удерживть тело на весу трудно
				- 0.5f*Girdle.Crouch),							// пригнутость и занижает, и ослабляет - чтоб ноги на склоне вытягивались до касания с опорой
			10 + 100*Girdle.Clung,								// инерция скорости, чтоб не пружинило, но пока неясно, как этим управлять

			// максимальная сила -  копируем из редактора (или разбить функцию на более мелкие, глубокие
			ARCons->ProfileInstance.LinearDrive.ZDrive.MaxForce);
	}
	//нога оторвана от опоры
	else
	{
		//в отсутствии опоры нога влияет на оценку скорости пояса в сторону абсолютной скорости
		Girdle.VelocityAgainstFloor = FMath::Lerp(
			Girdle.VelocityAgainstFloor,
			FootBody->GetUnrealWorldVelocity(),
			DeltaTime);

		//пока нога в воздухе, плавно смещать ее "нормаль" к направлению вверх туловища
		FootLimb.ImpactNormal = FMath::Lerp(FootLimb.ImpactNormal, GetLimbAxisUp(CentralLimb), DeltaTime);
		
		//раз и на весь период полета делаем полетными праметры сил ноги
		if(FootBody->LinearDamping > 0.01f) 
		{	SetLimbDamping (FootLimb, 0.01, 0);
			GetMachineConstraint(FootLimb)->SetLinearDriveParams(0, 0, 5000);
		}

		//стоит одной ноге не быть на опоре, как вертикаль сама уже не включится, только поддержка/усиление если уже была включена
		NeedForVertical = NeedForVertical && Girdle.Vertical;
	}
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

	//скорость пояса (пока) абсолютная - плавно обновляем по новым выборкам - для всех конфигураций пояса
	const FVector NewVelocity = CentralBody -> GetUnrealWorldVelocity();

	//источник информации о пригибе - текущий динамический профиль пояса
	float NewCrouch = Girdle.CurrentDynModel->Crouch;

	//направление движения для дальнейшего уточнения (по курсам для каждой ноги)
	//ведущий пояс направлен по взгляду камеры, ведомый - туда куда уже ориентирован
	//а вообще с ведомым пока непонятно, куда его отдельно направлять + всё это для четвероногих
	FVector PrimaryForth = Girdle.CurrentDynModel->Leading ?
		MyrOwner()->MoveDirection
		: GetLimbAxisForth(CentralLimb);

	////////////////////////////////////////////////////////////////////////////////
	//если пояс конечностей не содержит конечностей
	if(!Girdle.HasLegs)
	{
		//если брюхо касается опоры
		if(CentralLimb.Stepped)
		{
			//используем "брюхо" для определения опоры и ее относительной скорости
			Girdle.VelocityAgainstFloor = FMath::Lerp (
				Girdle.VelocityAgainstFloor,
				CentralBody->GetUnrealWorldVelocity()
					- CentralLimb.Floor ->GetUnrealWorldVelocityAtPoint ( GetMachineBody(CentralLimb)->GetCOMPosition() ),
				0.1f);

			//плавно, с полужизнью полсекунды пригибаемся или разгибаемся
			Girdle.Crouch = FMath::Lerp( Girdle.Crouch, NewCrouch, 2*DeltaTime);

			//используем нормаль брюха для определения вектора направления (бок может быть накренён, посему нормализировать)
			Girdle.Forward = FVector::CrossProduct (GetLimbAxisRight(CentralLimb), CentralLimb.ImpactNormal);
			Girdle.Forward.Normalize();

			//точное направление для движения - по опоре, которую касается брюхо
			Girdle.AlignedDir = CalculateLimbGuideDir (	CentralLimb, PrimaryForth);
			Girdle.AlignedDir.Normalize();
		}
		//если брюхо в воздухе, 
		else 
		{	//плавно возвращать ее скорость к абсолютной (хотя здесь можно задействовать ветер)
			Girdle.VelocityAgainstFloor = FMath::Lerp(Girdle.VelocityAgainstFloor, NewVelocity, DeltaTime);
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
		auto& RLimb = GetGirdleLimb(Girdle, EGirdleRay::Right);	auto RBody = GetGirdleBody(Girdle, EGirdleRay::Right);
		auto& LLimb = GetGirdleLimb(Girdle, EGirdleRay::Left);	auto LBody = GetGirdleBody(Girdle, EGirdleRay::Left);

		//обстоятельная необходимость в жесткой вертикали - пропадает при сильном увечье обеих ног или туловища
		bool WeStillNeedVertical = Girdle.CurrentDynModel->HardVertical	
			&& CentralLimb.Damage < 0.8f
			&& SpineLimb.Damage < 0.8f;

		//накопление влияний ног на позицию туловища
		float WeightAccum = 0.0f;
		float DampingForFeet = CalculateGirdleFeetDamping (Girdle);
		ProcessGirdleFoot (Girdle, RLimb, CentralLimb, DampingForFeet, PrimaryForth, WeightAccum, WeStillNeedVertical, DeltaTime);
		ProcessGirdleFoot (Girdle, LLimb, CentralLimb, DampingForFeet, PrimaryForth, WeightAccum, WeStillNeedVertical, DeltaTime);

		//аккумулятор веса нулевой - обе ноги в воздухе
		if(WeightAccum == 0)
		{
			//безусловно снять всякие привязи
			ClearGirdleVertical(Girdle);
			return;
		}
		else
		{
		}


/*		//если правая нога на земле
		if (RLimb.Stepped)
		{
			//❚▄▄❚обе ноги на земле: что можно - усреднить по обеим, остальное взять от самой релевантной
			if(LLimb.Stepped)
			{
				//получить направления желаемого движения по мнению обеих ног
				//при зацепе для ориентации констрейнта будет использоваться это общее направление, а не индивидуальные
				FVector GuideL = CalculateLimbGuideDir (LLimb, PrimaryForth);
				FVector GuideR = CalculateLimbGuideDir (RLimb, PrimaryForth);

				//способ комбинирования желаемых краекторий ног: если опоры разные, то больше доверяем той, которая в направлении взора
				//однако неясен выбор констант и опасность того, что оин из векторов развернется в минус
				if(RLimb.Floor != LLimb.Floor)
				{
					//коэффициенты "насколько по пути всему телу навязанный путь для левой и правой ноги"
					float PartL = FMath::Max(0.0f, 0.2f + (GuideL | PrimaryForth));
					float PartR = FMath::Max(0.0f, 0.2f + (GuideR | PrimaryForth));

					//если глядим в одну сторону, а нога вдоль ветки должна идти в свосем другую - отцепить эту ногу
					if (PartL == 0 && LLimb.Clung) UnClingLimb(LLimb);
					if (PartR == 0 && RLimb.Clung) UnClingLimb(RLimb);

					//курс - взешенная сумма влияний ног
					Girdle.AlignedDir = GuideR * PartR + GuideL * PartL;

					//нормаль пояса - пока грубо, немного неединично, но просто - среднее по двум ногам
					CentralLimb.ImpactNormal = 0.5*(RLimb.ImpactNormal + LLimb.ImpactNormal);

					//фиктивная, дайджестовая опора для центрального узла, чтобы соседний пояс мог анализировать
					CentralLimb.Floor = PartR > PartL ? RLimb.Floor : LLimb.Floor;
				}
				//если опора обеих ног одинаковая то 
				else
				{
					//просто суммируем, чтобы получился средний по двум
					Girdle.AlignedDir = GuideR + GuideL;

					//рассчёт вектора верх для туловища - или в среднем по нормалям в ногах, или особый для веток-капсул, чтобы не кренилось
					SetLimbOnBranchSaddleUp ( RLimb, CentralLimb.ImpactNormal, 0.5*(RLimb.ImpactNormal + LLimb.ImpactNormal));

					//фиктивная, дайджестовая опора для центрального узла, чтобы соседний пояс мог анализировать
					CentralLimb.Floor = RLimb.Floor;
				}

				//относительная скорость из средней по обим ногам
				Girdle.VelocityAgainstFloor = FMath::Lerp(
					Girdle.VelocityAgainstFloor,
					0.5 * (
							(RBody->GetUnrealWorldVelocity() - RLimb.Floor -> GetUnrealWorldVelocityAtPoint ( RBody->GetCOMPosition()) ) +
							(LBody->GetUnrealWorldVelocity() - LLimb.Floor -> GetUnrealWorldVelocityAtPoint ( LBody->GetCOMPosition()) )
						), 0.1f);

				//дополнительное ограничение на необходимость вертикали - здоровье как минимум одной из опущенных ног и крутость склона
				WeStillNeedVertical = WeStillNeedVertical
					&& CentralLimb.ImpactNormal.Z > 0.5			// склон достаточно пологий
					&& (RLimb.Damage * LLimb.Damage) < 0.5f;	// ноги не сильно повреждены

				//изящный, всеобъемлющий, нетранцедентный расчёт кривизны опоры по раскояке нормалей ног
				Girdle.Curvature = FVector::DotProduct(Girdle.Forward, FVector::CrossProduct(RLimb.ImpactNormal, LLimb.ImpactNormal));
			}
			//❚▀▄❚если только одна правая нога на земле
			else
			{
				//относительная скорость только по одной из ног
				Girdle.VelocityAgainstFloor = FMath::Lerp(
					Girdle.VelocityAgainstFloor,
					RBody->GetUnrealWorldVelocity() - RLimb.Floor -> GetUnrealWorldVelocityAtPoint ( RBody->GetCOMPosition() ),
					0.1f);

				//рассчёт вектора верх для туловища - или в среднем по нормалям в ногах, или особый для веток-капсул, чтобы не кренилось
				SetLimbOnBranchSaddleUp ( RLimb, CentralLimb.ImpactNormal, RLimb.ImpactNormal);

				//дополнительное ограничение на необходимость вертикали - здоровье единственной действующей ноги и пологий склон под ней
				WeStillNeedVertical = WeStillNeedVertical
					&& RLimb.ImpactNormal.Z > 0.5			// склон достаточно пологий
					&& RLimb.Damage < 0.5f					// ноги не сильно повреждены
					&& Girdle.Vertical;						// вертикаль уже вкл. (когда обе ноги были на попре) - доводка до вертикали

				//точное направление для движения - по опоре, которую касается нога
				Girdle.AlignedDir = CalculateLimbGuideDir (	RLimb, PrimaryForth);

				//пока другая нога в воздухе, плавно смещать ее "нормаль" к направлению вверх туловища
				LLimb.ImpactNormal = FMath::Lerp(LLimb.ImpactNormal, GetLimbAxisUp(CentralLimb), DeltaTime);
			}
		}
		//если правой ноги на земле нет
		else
		{
			//❚▄▀❚если левая нога на земле, а правая в воздухе
			if(LLimb.Stepped)
			{
				//относительная скорость только по одной из ног
				Girdle.VelocityAgainstFloor = FMath::Lerp(
					Girdle.VelocityAgainstFloor,
					LBody->GetUnrealWorldVelocity() - LLimb.Floor->GetUnrealWorldVelocityAtPoint(LBody->GetCOMPosition()),
					0.1f);

				//рассчёт вектора верх для туловища - или в среднем по нормалям в ногах, или особый для веток-капсул, чтобы не кренилось
				SetLimbOnBranchSaddleUp ( LLimb, CentralLimb.ImpactNormal, LLimb.ImpactNormal);

				//дополнительное ограничение на необходимость вертикали - здоровье единственной действующей ноги и пологий склон под ней
				WeStillNeedVertical = WeStillNeedVertical
					&& LLimb.ImpactNormal.Z > 0.5			// склон достаточно пологий
					&& LLimb.Damage < 0.5f					// ноги не сильно повреждены
					&& Girdle.Vertical;						// вертикаль уже вкл. (когда обе ноги были на попре) - доводка до вертикали

				//точное направление для движения - по опоре, которую касается нога
				Girdle.AlignedDir = CalculateLimbGuideDir (LLimb, PrimaryForth);

				//пока другая нога в воздухе, плавно смещать ее "нормаль" к направлению вверх туловища
				RLimb.ImpactNormal = FMath::Lerp(RLimb.ImpactNormal, GetLimbAxisUp(CentralLimb), DeltaTime);
			}
			//❚▀▀❚обе ноги в воздухе - внимание, здесь функция кончается досрочно, ибо очень особый случай
			else
			{
				//плавно свести на нет любое пригнутие ног и кривизну поверхности
				Girdle.Crouch *= 0.9;
				Girdle.Curvature *= 0.9;

				//безусловно снять всякие привязи
				ClearGirdleVertical(Girdle);

				//плавно вернуть направление вперед к естественной ориентации центральной кости
				Girdle.Forward += GetLimbAxisForth(CentralLimb)*0.2;
				Girdle.Forward.Normalize();

				//в воздухе плавно возвращаем скорость к абсолютной
				Girdle.VelocityAgainstFloor = FMath::Lerp(Girdle.VelocityAgainstFloor, NewVelocity, DeltaTime);

				//плавное приведение нормали к естественной оси вверх (без нормировки, надеясь на маленькие кванты)
				LLimb.ImpactNormal =		FMath::Lerp (LLimb.ImpactNormal, GetLimbAxisUp(CentralLimb), DeltaTime);
				RLimb.ImpactNormal =		FMath::Lerp (RLimb.ImpactNormal, GetLimbAxisUp(CentralLimb), DeltaTime);

				//если другой пояс в это время цепляется за опору, а этот в воздухе
				if(OppositeGirdle.Clung)
				{
					//центр/таз противоположного сегмента туловища
					FLimb& OpCLimb = GetGirdleLimb(OppositeGirdle, EGirdleRay::Center);
					FBodyInstance* OpCBody = GetGirdleBody(OppositeGirdle, EGirdleRay::Center);
					//if(OpCLimb.Floor)
					

					//задать ориентационный вектор "вверх" для туловища равным таковому в соседнем поясе - чтобы не съезжал с ветки
					CentralLimb.ImpactNormal =	OpCLimb.ImpactNormal;
					Girdle.AlignedDir = OppositeGirdle.AlignedDir;
				}
				//плавное приведение нормали к естественной оси
				else
				{	CentralLimb.ImpactNormal =	FMath::Lerp (CentralLimb.ImpactNormal, GetLimbAxisUp(CentralLimb), DeltaTime);
					Girdle.AlignedDir = PrimaryForth;
				}

				//если еще не сделали этого при первом отрыве обеих ног
				if (LBody->LinearDamping > 0.01)
				{
					//отменить всякую вязкость ног
					SetLimbDamping(LLimb, 0.01, 0);
					SetLimbDamping(RLimb, 0.01, 0);

					//убрать всякую силу, пусть провиснет до пола
					GetMachineConstraint(LLimb)->SetLinearDriveParams(0, 0, 5000);
					GetMachineConstraint(RLimb)->SetLinearDriveParams(0, 0, 5000);
				}
				//весь заменитель стандартной рутины уже выполнен выше
				return;
			}
		}*/

		//вектор "правильный вперед", корректирующий pitch'и обеих частей туловища
		Girdle.Forward = FVector::CrossProduct (GetLimbAxisRight(CentralLimb), CentralLimb.ImpactNormal);
		Girdle.Forward.Normalize();

		CentralLimb.ImpactNormal = GetLimbAxisUp(CentralLimb);
		if(Girdle.Clung && CentralLimb.OnBranch() && CentralLimb.OnCapsule())
		{
			FVector GuideSaddleUp = CentralLimb.GetBranchSaddleUp();
			if (GuideSaddleUp.Z > 0.2) CentralLimb.ImpactNormal = GuideSaddleUp;
		}

		//направление вперед из произвольного угла, всегда нужно нормировать
		Girdle.AlignedDir.Normalize();

		//включить удержание вертикали в ветках выше накопилось решение, нужна вертикаль или нет, 
		if (WeStillNeedVertical) SetGirdleVertical(Girdle);
		else ClearGirdleVertical(Girdle);

		//плавно, за полсекунды пригибаемся или разгибаемся ногами
		//Girdle.Crouch = FMath::Lerp(Girdle.Crouch, NewCrouch, 2 * DeltaTime);

		//изящный, всеобъемлющий, нетранцедентный расчёт кривизны опоры по раскояке нормалей ног
		Girdle.Curvature = FVector::DotProduct(Girdle.Forward, FVector::CrossProduct(RLimb.ImpactNormal, LLimb.ImpactNormal));


		//размах ноги по оси "вверх/вниз" берется из архетипа - из ассета, предполагается, что ноги симметричны
		//неясно, какая цифра лучше - максимальный лимит или начальное отклонение, что соответствует начальной позе стоя
		//первое обеспечивает лучшее покрытие поз, второй - лучше вписывается в диапазоны кинематической анимации
		//auto ARCons = GetArchMachineConstraint(RLimb);
		//float FreeFeetLength = FMath::Abs(ARCons->ProfileInstance.LinearLimit.Limit);
		//float FreeFeetLength = FMath::Abs(ARCons->ProfileInstance.LinearDrive.PositionTarget.Z);

		//настроить ноги (NewDamp - торможение, вслед за ним - сжатие/растяжение, индивидуальное, с учётом увечия ноги)
		//float NewDamp = CalculateGirdleFeetDamping (Girdle);
		//ProcessFoot(RLimb, NewDamp, FreeFeetLength*(1 - 2*(Girdle.Crouch + RLimb.Damage)), DeltaTime);
		//ProcessFoot(LLimb, NewDamp, FreeFeetLength*(1 - 2*(Girdle.Crouch + LLimb.Damage)), DeltaTime);
		
		///////////////////////////////////////////////////////////////////

		//сверху от туловище рисуем тестовые индикаторы
		//auto Ct = CentralBody->GetCOMPosition() + FVector::UpVector * 15;
		//auto Color = FColor(255, 0, 255, 10);
		//DrawDebugLine(GetOwner()->GetWorld(), Ct, Ct + Girdle.Forward * 15,				Color, false, 0.02f, 100, Girdle.CurrentDynModel->Leading ? 1 : 2);
		//DrawDebugLine(GetOwner()->GetWorld(), Ct, Ct + CentralLimb.ImpactNormal * 15,	Color, false, 0.02f, 100, Girdle.Clung||Girdle.Vertical ? 5 : 1);
		//DrawDebugLine(GetOwner()->GetWorld(), Ct, Ct + Girdle.AlignedDir * 15,			FColor(255,0,0), false, 0.02f, 100, Girdle.CurrentDynModel->Leading ? 1 : 2);

		//float gx = FVector::DotProduct(Girdle.VelocityAgainstFloor, Girdle.Forward);
		//float gy = FVector::DotProduct(Girdle.VelocityAgainstFloor, GetLimbAxisRight(CentralLimb));
		//DrawDebugLine(GetOwner()->GetWorld(), Ct, Ct + Girdle.Forward*gx,					Color, false, 0.02f, 100, Girdle.CurrentDynModel->Leading ? 1 : 2);
		//DrawDebugLine(GetOwner()->GetWorld(), Ct, Ct + GetLimbAxisRight(CentralLimb)*gy,	Color, false, 0.02f, 100, Girdle.Clung||Girdle.Vertical ? 5 : 1);


		//маленькая метка, из какой ноги берутся данные по опоре
		//if(CentralLimb.Floor == RLimb.Floor)		Ct += GetLimbAxisRight(CentralLimb)*10; else
		//if(CentralLimb.Floor == LLimb.Floor)		Ct += GetLimbAxisLeft(CentralLimb)*10;
		//DrawDebugLine(GetOwner()->GetWorld(), Ct, Ct, FColor(0, 0, 0, 10), false, 0.02f, 100, 1);
	}
}


//==============================================================================================================
//вычислить торможение ног этого пояса через вязкость - 
//вынесено в отдельную, так как важный регулятор движения, сюда, возможно, будет добавлено что-то ещё
//на данный момент цепляние абсолютно независимо от вот этого торможения
//==============================================================================================================
float UMyrPhyCreatureMesh::CalculateGirdleFeetDamping(FGirdle &Girdle)
{
	//центральное тело пояса - туловище между ног/рук
	auto& CentralLimb = GetGirdleLimb(Girdle, EGirdleRay::Center);
	auto LBody = GetGirdleBody(Girdle, EGirdleRay::Left);

	//не тормозить если склон слишком крут (скольжение) и если режим лёгких/неустойчивых ног
	if(LBody->MassScale<1 || CentralLimb.ImpactNormal.Z < 0.5f) return 0.0f;

	//скорость в направлении желаемого движения 
	const float SpeedAlongCourse = FMath::Abs(FVector::DotProduct (Girdle.VelocityAgainstFloor, Girdle.AlignedDir));
	const float WantedSpeed = MyrOwner()->GetDesiredVelocity();

	//если набранная скорость маловата - не сдерживать движение вообще
	const float Overspeed =  SpeedAlongCourse - WantedSpeed;
	if (Overspeed < 0)	return 0.0f;

	if (WantedSpeed > 0) return Overspeed / WantedSpeed;

	//базовое значение торможения, зберется базовый коэф и в 10 раз увеличивается при крутом склоне
	float NewDamp = Girdle.CurrentDynModel->FeetBrakeDamping
		* (10 - 9*CentralLimb.ImpactNormal.Z);

	//если превышение скорости очень большое (например с бега 400 хотим остановиться до 0) - ограничить, чтобы был тормозной путь
	//if (Overspeed > 50) return NewDamp;

	//тормозить сильнее при крутой опоре и при излишке скорости
	//NewDamp *= ((Overspeed + 1)/(1 + MyrOwner()->GetDesiredVelocity()));
	//UE_LOG(LogTemp, Warning, TEXT("%s: CalculateGirdleFeetDamping %d %g"), *GetOwner()->GetName(), (int)(&Girdle - &gPelvis), NewDamp);
	return NewDamp;
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
	if (GetLimbAxisUp(CLimb).Z<0) return;//◘◘>

	//⏹пояс имеет реальные ноги, сам центр собирает инфу из них
	if(G.HasLegs)
	{
		//⏹если вообще пусто - начать создавать констрайнт с чистого листа
		if(!CBody->DOFConstraint)
		{
			//выделить память, присвоить ненужное
			CBody->DOFConstraint = FConstraintInstance::Alloc();
			CI->DisableProjection();
			CI->ProfileInstance.ConeLimit.bSoftConstraint = false;
			CI->ProfileInstance.TwistLimit.bSoftConstraint = false;
			CI->ProfileInstance.LinearLimit.bSoftConstraint = false;
		}

		//⏹если в прошлый раз была тоже вертикаль (это уже значит, что констрейнт создавался)
		else
		{
			//требуется перевести в новое состояние уже созданный ограничитель
			if(G.Vertical == false) 
			{
				//зафиксировать пояс по осям крена
				auto Param = EAngularConstraintMotion::ACM_Locked;
				CBody->DOFConstraint->SetAngularSwing2Motion(Param);
				CBody->DOFConstraint->SetAngularSwing1Motion(Param);
				G.Vertical = true;
				UE_LOG(LogTemp, Warning, TEXT("%s: SetGirdleVertical %d Switch On"), *GetOwner()->GetName(), (int)(&G - &gPelvis));
			}

			//включить уже включенную, и мкасимально выправленную
			if (CharZ > 0.99)	return;	//◘◘>

			//остался один вариант - включить уже включенную, когда ориентация тела далека от идеальной
		}
		//////////////////////////////////////////////////////////////////////////////////
		//если функция до сих пор выполняется, то: 
		// - ограничение создано с нуля или очищено от иной конфигурации (зацепа на опору)
		// - ограничение уже было включено, но ориентация далека от целевой, и надо ее постепенно приближать
		const auto TM = CBody->GetUnrealWorldTransform();

		//если текущий уклон тела от вертикали слишком велик
		if(CharZ <= 0.95)
		{
			//вращение между идеальным положением и тем, в котором привязь застала наше тело 
			FQuat QDesired = FQuat::FindBetweenNormals (MyrAxes[(int)Lorient.Up], FVector(0, 0, 1));

			//это текущий кватернион вращения
			FQuat QNormal = TM.GetRotation().Inverse();

			//выработать новое приближение к идеальной ориентации
			FQuat QNewStep = FMath::Lerp(QNormal, QDesired, 0.05f);

			//новые оси для ориентации получить из вращения только что полученным кватернионом
			CI->PriAxis1 = QNewStep.RotateVector(FVector(0, 0, 1));
			CI->SecAxis1 = QNewStep.RotateVector(GetLimbAxisForth(CLimb));
		}
		//если ориентация лишь чуть-чуть недотягивает до вертикали - задать идеальные значения и позволить физдвужку рывком их настроить
		else
		{	//задать идеальные значения и позволить физдвужку рывком их настроить
			CI->PriAxis1 = MyrAxes[(int)Lorient.Up];//TM.InverseTransformVectorNoScale(FVector(0, 0, 1));
			CI->SecAxis1 = MyrAxes[(int)Lorient.Forth];//TM.InverseTransformVectorNoScale(FVector(0, 1, 0));
		}

		//оси для мира - дабы туловище развернулось только в вертикали, но не по азимуту, вторую ось берем из туловища
		CI->PriAxis2 = FVector(0, 0, 1);
		CI->SecAxis2 = GetLimbAxisForth(CLimb);
		CI->SecAxis2.Z = 0;
		CI->SecAxis2.Normalize();
		CI->Pos2 = TM.GetLocation();

		//если привязь уже создана
		if(CI->IsTerminated())
		{
			//на старте сразу включить
			SetFreedom(CI, -1,-1,-1,  0,0,180);
			CI->InitConstraint (CBody, nullptr, 1.f, this);
			UE_LOG(LogTemp, Warning, TEXT("%s: SetGirdleVertical %d Create"), *GetOwner()->GetName(), (int)(&G - &gPelvis));
		}
		else
		{
			//дальше выкорчёвывается из исходников способ обращения с нижележащим слоем абстракции и применяется в лоб
			FPhysicsInterface::ExecuteOnUnbrokenConstraintReadWrite(CI->ConstraintHandle, [&](const FPhysicsConstraintHandle& InUnbrokenConstraint)
			{	FPhysicsInterface::SetLocalPose(InUnbrokenConstraint, FTransform(CI->PriAxis1, CI->SecAxis1, CI->PriAxis1 ^ CI->SecAxis1, CI->Pos1), EConstraintFrame::Frame1);
				FPhysicsInterface::SetLocalPose(InUnbrokenConstraint, FTransform(CI->PriAxis2, CI->SecAxis2, CI->PriAxis2 ^ CI->SecAxis2, CI->Pos2), EConstraintFrame::Frame2);
			});
			UE_LOG(LogTemp, Error, TEXT("%s: SetGirdleVertical %d Refine %g"), *GetOwner()->GetName(), (int)(&G - &gPelvis), GetLimbAxisUp(CLimb).Z);
			return;
		}
	}
	//⏹ног нет, они кинематические, поверхность исследуется брюхом
	else
	{
		//а нужно ли вообще на брюхе такое жесткое ограничение? проще сместить центр масс
	}
}


//==============================================================================================================
// отменить вертикальность
//==============================================================================================================
void UMyrPhyCreatureMesh::ClearGirdleVertical(FGirdle& G)
{
	//узел пояса конечностей
	auto& CLimb = GetGirdleLimb(G, EGirdleRay::Center);
	auto  CBody = GetGirdleBody(G, EGirdleRay::Center);
	auto& CI = CBody->DOFConstraint;
	if (!CI) return;//◘◘>

	//если уже и так отключено
	if(!G.Vertical) return;//◘◘>

	//зафиксировать или расслабить пояс по осям крена
	//зафиксировать или расслабить пояс по осям крена
	auto Param = EAngularConstraintMotion::ACM_Free;
	CBody->DOFConstraint->SetAngularSwing2Motion(Param);
	CBody->DOFConstraint->SetAngularSwing1Motion(Param);
	G.Vertical = false;
	UE_LOG(LogTemp, Warning, TEXT("%s: ClearGirdleVertical %d Switch Off"), *GetOwner()->GetName(), (int)(&G - &gPelvis));

}

//==============================================================================================================
//задать предпочтительную ориентацию пояса конечностей - при смене позы
//==============================================================================================================
void UMyrPhyCreatureMesh::SetGirdleDynamicsModel (FGirdle& Girdle, FGirdleDynModels& Models)
{
	//сохранить связку, так как в процессе понадобится уточнять
	Girdle.CurrentDynModel = &Models;
	auto& CLimb = GetGirdleLimb(Girdle, EGirdleRay::Center);
	auto& RLimb = GetGirdleLimb(Girdle, EGirdleRay::Right);
	auto& LLimb = GetGirdleLimb(Girdle, EGirdleRay::Left);
	auto& SLimb = GetGirdleLimb(Girdle, EGirdleRay::Spine);
	auto& TLimb = GetGirdleLimb(Girdle, EGirdleRay::Tail);

	//просто сменить флаги, они в основном обрабатываются в CustomPhysics
	CLimb.DynModel = Models.Center;
	SLimb.DynModel = Models.Spine;
	TLimb.DynModel = Models.Tail;
	GetGirdleBody(Girdle, EGirdleRay::Spine)->SetEnableGravity(Models.GravityForUpper);
	GetGirdleBody(Girdle, EGirdleRay::Center)->SetEnableGravity(Models.GravityForUpper);
	if(Girdle.HasLegs)
	{
		auto RBody = GetGirdleBody(Girdle, EGirdleRay::Right);
		auto LBody = GetGirdleBody(Girdle, EGirdleRay::Left);
		LLimb.DynModel = Models.Left;
		LBody->SetEnableGravity(Models.GravityForLegs);
		LBody->SetMassScale(Models.HeavyLegs ? 10 - 9 * FMath::Min(LLimb.Damage, 1.0f) : 0.5);
		RLimb.DynModel = Models.Right;
		RBody->SetEnableGravity(Models.GravityForLegs);
		RBody->SetMassScale(Models.HeavyLegs ? 10 - 9 * FMath::Min(RLimb.Damage, 1.0f) : 0.5);
	}
	//запретить ногам махать вдоль спины - сомнительная надобность, опасно
	SetGirdleSpineLock (Girdle, Models.LegsSwingLock);
	
	//внедрение дополнительной тугости на движение - из битов по разным частям тела (какую амплитуду брать?)
	SetLimbDampingFromModel (CLimb, Models.CommonDampingIfNeeded, Models.Center);
	SetLimbDampingFromModel (SLimb, Models.CommonDampingIfNeeded, Models.Spine);
	SetLimbDampingFromModel (TLimb, Models.CommonDampingIfNeeded, Models.Tail);
	SetLimbDampingFromModel (RLimb, Models.CommonDampingIfNeeded, Models.Right);
	SetLimbDampingFromModel (LLimb, Models.CommonDampingIfNeeded, Models.Left);

	//ведущесть/ведомость
	SetGirdleLeading(Girdle, Girdle.CurrentDynModel->Leading);
	UE_LOG(LogTemp, Warning, TEXT("%s: SetGirdleDynamicsModel %d"), *GetOwner()->GetName(), (int)(&Girdle - &gPelvis));
}
//==============================================================================================================
//включить или отключить проворот спинной части относительно ног (мах ногами вперед-назад)
//==============================================================================================================
void UMyrPhyCreatureMesh::SetGirdleSpineLock(FGirdle &G, bool Set)
{
	auto CI = GetGirdleSpineConstraint(G);
	SetAngles (CI, TWIST, Set ? 0 : 180);		
	CI->UpdateAngularLimit();
}


//==============================================================================================================
//суммарная потеря двигательных функций всего тела (множитель скорости)
//==============================================================================================================
float UMyrPhyCreatureMesh::GetWholeImmobility()
{
	return FMath::Min(1.0f, 
		MyrOwner()->GetGenePool()->Anatomy.Head.Machine.AffectMobility * Head.Damage +
		MyrOwner()->GetGenePool()->Anatomy.Lumbus.Machine.AffectMobility * Thorax.Damage +
		MyrOwner()->GetGenePool()->Anatomy.Pectus.Machine.AffectMobility * Pelvis.Damage +
		MyrOwner()->GetGenePool()->Anatomy.LeftArm.Machine.AffectMobility * LArm.Damage +
		MyrOwner()->GetGenePool()->Anatomy.RightArm.Machine.AffectMobility * RArm.Damage +
		MyrOwner()->GetGenePool()->Anatomy.LeftLeg.Machine.AffectMobility * LLeg.Damage +
		MyrOwner()->GetGenePool()->Anatomy.RightLeg.Machine.AffectMobility * RLeg.Damage);
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
			if (S->GetGirdleLimb(Girdle, EGirdleRay::Left).IsClimbable() || S->GetGirdleLimb(Girdle, EGirdleRay::Right).IsClimbable())
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
//одним махом установить ограничения по углам и движению
//==============================================================================================================
void UMyrPhyCreatureMesh::SetFreedom(FConstraintInstance* CI, float Lx, float Ly, float Lz, int As1, int As2, int At)
{
	CI->ProfileInstance.ConeLimit.Swing1Motion = (As1 == 180) ? EAngularConstraintMotion::ACM_Free : ((As1 == 0) ? EAngularConstraintMotion::ACM_Locked : EAngularConstraintMotion::ACM_Limited);
	CI->ProfileInstance.ConeLimit.Swing1LimitDegrees = As1;
	CI->ProfileInstance.ConeLimit.Swing2Motion = (As2 == 180) ? EAngularConstraintMotion::ACM_Free : ((As2 == 0) ? EAngularConstraintMotion::ACM_Locked : EAngularConstraintMotion::ACM_Limited);
	CI->ProfileInstance.ConeLimit.Swing2LimitDegrees = As2;
	CI->ProfileInstance.TwistLimit.TwistMotion = (At  == 180) ? EAngularConstraintMotion::ACM_Free : ((At  == 0) ? EAngularConstraintMotion::ACM_Locked : EAngularConstraintMotion::ACM_Limited);
	CI->ProfileInstance.TwistLimit.TwistLimitDegrees = At;
	CI->UpdateAngularLimit();
	CI->ProfileInstance.LinearLimit.XMotion = Lx == 0 ? ELinearConstraintMotion::LCM_Locked : (Lx < 0 ? ELinearConstraintMotion::LCM_Free : ELinearConstraintMotion::LCM_Limited);
	CI->ProfileInstance.LinearLimit.YMotion = Ly == 0 ? ELinearConstraintMotion::LCM_Locked : (Ly < 0 ? ELinearConstraintMotion::LCM_Free : ELinearConstraintMotion::LCM_Limited);
	CI->ProfileInstance.LinearLimit.ZMotion = Lz == 0 ? ELinearConstraintMotion::LCM_Locked : (Lz < 0 ? ELinearConstraintMotion::LCM_Free : ELinearConstraintMotion::LCM_Limited);
	CI->ProfileInstance.LinearLimit.Limit = FMath::Max3(Lx, Ly, Lz);
	CI->UpdateLinearLimit();
}

