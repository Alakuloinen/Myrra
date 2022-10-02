// Fill out your copyright notice in the Description page of Project Settings.


#include "Creature/Animation/MyrPhyCreatureAnimInst.h"
#include "../MyrPhyCreature.h"
#include "../MyrPhyCreatureMesh.h"
#include "AssetStructures/MyrCreatureGenePool.h"		// данные генофонда
#include "DrawDebugHelpers.h"							// рисовать отладочные линии

#define ROTATE(Out,En,L1,A1,L2,A2,ASIN) DriveBodyRotation(Out, En, ELimb::##L1, EMyrRelAxis::##A1, ELimb::##L2, EMyrRelAxis::##A2, ASIN)

//рисовалки отладжочных линий
#if WITH_EDITOR
#define LINE(C, A, AB) Creature->Line(C, A, AB)
#define LINEWT(C, A, AB,W,T) Creature->Line(C, A, AB,W,T)
#define LINEW(C, A, AB, W) Creature->Line(C, A, AB, W)
#define LINELIMB(C, L, AB) Creature->Line(C, me()->GetMachineBody(L)->GetCOMPosition(), AB)
#define LINELIMBW(C, L, AB, W) Creature->Line(C, me()->GetMachineBody(L)->GetCOMPosition(), AB, W)
#define LINELIMBWT(C, L, AB, W, T) Creature->Line(C, me()->GetMachineBody(L)->GetCOMPosition(), AB, W, T)
#define LDBG(C) Creature->IsDebugged(C)
#else
#define LDBG(C) false
#define LINE(C, A, AB)
#define LINELIMB(C, L, AB)
#define LINELIMBW(C, L, AB, W)
#define LINELIMBWT(C, L, AB, W, T) 
#define LINEW(C, A, AB, W)
#endif

//==============================================================================================================
//инициализация 
//==============================================================================================================
void UMyrPhyCreatureAnimInst::NativeInitializeAnimation()
{
	//Very Important Line
	Super::NativeInitializeAnimation();

	//Cache the owning pawn for use in Tick
	Creature = Cast<AMyrPhyCreature>(TryGetPawnOwner());
	if (!Creature) return;

}

//==============================================================================================================
//пересчёт (здесь в основном всё в лоб)
//==============================================================================================================
void UMyrPhyCreatureAnimInst::NativeUpdateAnimation(float DeltaTimeX)
{
	Super::NativeUpdateAnimation(DeltaTimeX);
	if (HasAnyFlags(RF_ClassDefaultObject)) return;
	if (!Creature) return;

	auto Ma = Creature->GetMesh();
	if (!Ma->Bodies.Num()) return;
	if (!Ma->DynModel) return;

	//тело рисуется в максимальной детализации, это первые два лода
	//нулевой лод это вообще ультра прямо перед глазами, поэтому берется еще и первый
	const bool AllowMoreCalc = (GetLODLevel() <= 1);

	//целевое значение обмена веществ определяется режимом поведения и скоростью расхода стамины
	float NewMetabolism = Creature->GetBehaveCurrentData()->MetabolismBase - 10 * Ma->DynModel->StaminaAdd;

	//текущий произносимый звук (пока пологаемся, что Blend сделает плавный переход без ухищрений
	CurrentSpelledSound = Creature->CurrentSpelledSound;

	//лоически несколько состояний - графически одно и то же, шаг (даже если бег и если увечье/усталость и не выжимается скорость)
	CurrentState = Creature->CurrentState;
	if (CurrentState == EBehaveState::crouch ||
		CurrentState == EBehaveState::stand
		|| (CurrentState == EBehaveState::run && Creature->MoveGain < 0.5f))
		CurrentState = EBehaveState::walk;
	StateTime = Creature->StateTime;

	//случай, если действие резко заменилось без акта окончания - вставить этот акт искусственно, обнулив ролик
	if (CurrentSelfAction != Creature->CurrentSelfAction)
	{	SelfAction = nullptr;
		CurrentSelfAction = Creature->CurrentSelfAction;
	}
	//самодействия
	if (CurrentSelfAction != 255)
	{
		//скорость анимации (она изменяетсч каждый кадр потому что могут быть секции с разной скоростью)
		SelfActionPlayRate = Creature->GetSelfAction()->DynModelsPerPhase[Creature->SelfActionPhase].AnimRate;

		//перед началом анимации вписать указатель на ресурс с кадрами
		if (SelfAction == nullptr)
		{
			//главная анимация вероятнее всего, затем альтернативные, если есть
			SelfAction = Creature->GetSelfAction()->Motion;
			if (auto NR = Creature->GetSelfAction()->AlternativeRandomMotions.Num())
				if (FMath::RandBool())
					SelfAction = Creature->GetSelfAction()->AlternativeRandomMotions[FMath::RandRange(0, NR - 1)];

			//возможность подставлять в сборку анимацию разной локальности - тупой анрил не умеет выкорчевывать это свойства в АнимБП
			SelfAnimLocalSpace = (SelfAction->GetAdditiveAnimType() == EAdditiveAnimationType::AAT_LocalSpaceBase);

			//изменятель скорости обмена веществ для этого конкретного действия
			NewMetabolism *= Creature->GetSelfAction()->MetabolismMult;
		}
	}
	//вне самодействий поддерживать указатель на роик пустым
	else SelfAction = nullptr;

	//действия по отдыху
	CurrentRelaxAction = Creature->CurrentRelaxAction;
	if (CurrentRelaxAction != 255)
	{
		//здесь CurrentRelaxAction адресуется не по номеру, а по понятию, чтобы анимации можено было подправлять для отдельных групп
		CurrentRelaxAction = (uint8)Creature->GetRelaxAction()->Type;
		RelaxActionPlayRate = Creature->GetRelaxAction()->DynModelsPerPhase[Creature->RelaxActionPhase].AnimRate;
		RelaxMotion = Creature->GetRelaxAction()->Motion;

		//возможность подставлять в сборку анимацию разной локальности - тупой анрил не умеет выкорчевывать это свойства в АнимБП
		RelaxAnimLocalSpace = (RelaxMotion->GetAdditiveAnimType() == EAdditiveAnimationType::AAT_LocalSpaceBase);

		//изменятель скорости обмена веществ для этого конкретного действия
		NewMetabolism *= Creature->GetRelaxAction()->MetabolismMult;
	}

	//переписать новые значения компонентов общей эмоции существа, чтоб отразить в позах
	Creature->TransferIntegralEmotion(EmotionRage, EmotionFear, EmotionPower, EmotionAmount);

	//случай, если атака резко заменилась без акта окончания - вставить этот акт искусственно, обнулив ролик
	if (CurrentAttack != Creature->CurrentAttack)
	{
		AttackCurvesAnimation = nullptr;
		CurrentAttack = Creature->CurrentAttack;
	}
	//атаки (если в материнском объекте атака на задана, то и тут ничего не надо делать)
	if (Creature->CurrentAttack != 255)
	{
		//аним-ролики атаки меняются только при изменении номера атаки, признак этого - пустая анимация
		if (AttackCurvesAnimation == nullptr)
		{	CurrentAttack = Creature->CurrentAttack;
			AttackAnimation = Creature->GetAttackActionVictim().RawTargetAnimation;
			AttackPreciseAnimation = Creature->GetAttackActionVictim().PreciseTargetAnimation;
			AttackCurvesAnimation = Creature->GetAttackAction()->Motion;
		}
		//фаза меняется при неизменной атаке
		auto AttackInfo = Creature->GetAttackAction();
		CurrentAttackPhase = Creature->CurrentAttackPhase;
		CurrentAttackPlayRate = AttackInfo->DynModelsPerPhase[(int)Creature->CurrentAttackPhase].AnimRate;

		//если фаза атаки подразумевает прыжок, то плоскость проекции на карту анимаций - не передняя, а нижняя
		//чтобы одной сборкой обрабатывать прыжки назад и вперед
		FVector3f VerOrAx = Creature->NowPhaseToJump() ? Ma->GetLimbAxisForth(Ma->Lumbus) : Ma->GetLimbAxisUp(Ma->Lumbus);

		//разложение абсолютного направления по базисам (пока неясно, какие в точности части тела брать, как точнее будет визуально)
		AttackDirRawLeftRight =		Creature->AttackDirection | Ma->GetLimbAxisRight(Ma->Lumbus);
		AttackDirRawUpDown =		Creature->AttackDirection | VerOrAx;
		AttackDirAdvLeftRight =		Creature->AttackDirection | Ma->GetLimbAxisRight(Ma->Pectus);
		AttackDirAdvUpDown =		Creature->AttackDirection | Ma->GetLimbAxisUp(Ma->Pectus);

		//изменятель скорости обмена веществ для этого конкретного действия
		NewMetabolism *= AttackInfo->MetabolismMult;
	}
	//вне атак поддерживать указатель на ролик с кривыми пустым
	else AttackCurvesAnimation = nullptr;

	//метаболизм - очень плавное изменение: чем больше расход (со знаком минус), тем сильнее метаболизм
	Metabolism = FMath::Lerp(Metabolism, NewMetabolism, DeltaTimeX * 0.5);

	//проекции скоростей на оси поясов конечностей - для анимации шагов
	//если шаги будут процедурные, то это нахрен не нужно
	auto gT = Creature->GetGirdle(ELimb::THORAX);
	auto gP = Creature->GetGirdle(ELimb::PELVIS);


#if WITH_EDITOR
	if (!gT->CurrentDynModel) return;
	if (!gP->CurrentDynModel) return;
#endif

	if (CurrentState == EBehaveState::climb)
		WholeBodyUpDown = Ma->Erection();
	else
	{
		//глобальный уклон тела - сложный расчёт только для птицы
		float WaveOrSoar = (gP->VelocityAgainstFloor | Ma->GetLimbAxisUp(ELimb::PELVIS)) * 0.2f + 0.005 * gP->Speed;
		if (WaveOrSoar < 0)
			WholeBodyUpDown = FMath::Lerp(WholeBodyUpDown, 1.0f + WaveOrSoar, 0.05);
		else WholeBodyUpDown = FMath::Lerp(WholeBodyUpDown, 1.0f + WaveOrSoar, 0.15);
	}

	//поворот физической спины влево-вправо (змейка) - регулируется направлением хода
	//включается только когда имеется один ведущий, один ведомый пояс
	if (gT->CurrentDynModel->Leading != gP->CurrentDynModel->Leading)
		MachineSpineLeftOrRight = FMath::Lerp(
			MachineSpineLeftOrRight,
			Creature->ExternalGain * (gT->GuidedMoveDir | Ma->GetLimbAxisRight(Ma->Pectus)),
			DeltaTimeX);

	//в ориентациях вбок змейка не нужна
	else MachineSpineLeftOrRight *= (1 - DeltaTimeX);

	//поворот глаз/ушей
	EyesLeftOrRight = (Creature->AttackDirection |	Ma->GetLimbAxisRight(Ma->Head));
	EyesUpOrDown =	(Creature->AttackDirection |	Ma->GetLimbAxisUp(Ma->Head));

	//подгон детального скелета под положение каркаса (определяемое физическим взаимодействием)
	//спины по всем осям (вероятно переделать под single frame и тем самым уйти от тригонометрии к (-1 +1)
	auto DOF = Creature->GetGenePool()->BodyDegreesOfFreedom;
	ROTATE(SpineLeftOrRight,	DOF.SpineLeftRight, LUMBUS, Right,	PECTUS, Front, AllowMoreCalc);
	ROTATE(SpineUpOrDown,		DOF.SpineUpDown,	LUMBUS, Up,		PECTUS, Front, AllowMoreCalc);
	ROTATE(SpineTwist,			DOF.SpineTwist,		LUMBUS, Up,		PECTUS, Left,  AllowMoreCalc);
	//DrawDebugString(Creature->GetWorld(), Devia, FString::SanitizeFloat(SpineLeftOrRight, 1), Creature, FColor(255, 255, 0, 255), 0.02, false, 1.0f);

	//если от третьего лица
	if (!Creature->IsFirstPerson())
	{
		//только в третьем лице вычисляется освещение, влияющее на зрачки
		Lighting = StepTo (Lighting, Creature->LightingAtView, DeltaTimeX*0.5f);

		//голову крутим только здесь, иначе вид будет кривой
		ROTATE(HeadLeftOrRight, DOF.HeadLeftRight,	PECTUS, Right,	HEAD, Front, AllowMoreCalc);
		ROTATE(HeadUpOrDown,	DOF.HeadUpDown,		PECTUS, Up,		HEAD, Front, AllowMoreCalc);
		ROTATE(HeadTwist,		DOF.HeadTwist,		PECTUS, Up,		HEAD, Left,  AllowMoreCalc);
	}

	//если в теле есть хвост
	if (Ma->HasPhy(ELimb::TAIL))
	{
		//наклон хвоста
		ROTATE(TailLeftOrRight, true, LUMBUS, Right, TAIL, Front, AllowMoreCalc);
		ROTATE(TailUpOrDown,	true, LUMBUS, Up,	 TAIL, Front, AllowMoreCalc);

		//если физический хвост представлен еще и кончиком
		if (Ma->HasFlesh(ELimb::TAIL))
		{
			//отклонение кончика хвоста - здесь внезапно всё вышло за пределы блистательной схемы, и приходится использовать "мясо" как каркас
			auto TipForth = Ma->GetAxis(Ma->GetFleshBody(Ma->Tail, 0)->GetUnrealWorldTransform(), EMyrAxis::Yn);
			TailTipLeftOrRight = (Ma->GetLimbAxisRight(Ma->Tail) | TipForth);
			if (AllowMoreCalc) TailTipLeftOrRight = 0.5 + FMath::FastAsin(TailTipLeftOrRight) / 3.1415926;
			else TailTipLeftOrRight = 0.5 + TailTipLeftOrRight / 2;

			TailTipUpOrDown = (Ma->GetLimbAxisDown(Ma->Tail) | TipForth);
			if (AllowMoreCalc) TailTipUpOrDown = 0.5 + FMath::FastAsin(TailTipUpOrDown) / 3.1415926;
			else TailTipUpOrDown = 0.5 + TailTipUpOrDown / 2;
		}
	}


	//позиционирование лап по положению колес
	const auto GP = Creature->GenePool;
	UpdateGirdle(Thorax, gT, &RArmUpDown, AllowMoreCalc);
	UpdateGirdle(Pelvis, gP, &RLegUpDown, AllowMoreCalc);
}

//==============================================================================================================
//Executed when begin play is called on the owning component
//==============================================================================================================
void UMyrPhyCreatureAnimInst::NativeBeginPlay()
{
	Super::NativeBeginPlay();
}

//==============================================================================================================
//обновить анимионные сборки по отдельному поясу конечностей
//==============================================================================================================
void UMyrPhyCreatureAnimInst::UpdateGirdle(FAGirdle& AnimGirdle, class UMyrGirdle* PhyGirdle, float* LimbChunk, bool AllowMoreCalc)
{
	//сам меш
	const auto M = Creature->GetMesh();
	if (!PhyGirdle->CurrentDynModel) return;

	//скорость берется 1/100, чтобы проще было ассоциировать с animation Rate, который хорош когда 1.0
	FVector3f DirVel = PhyGirdle->VelocityAgainstFloor * 0.01;

	//проекции скорости - вдоль туловища и поперек туловища
	AnimGirdle.GainDirect =		DirVel | PhyGirdle->GuidedMoveDir;
	AnimGirdle.GainLateral =	DirVel | M->GetLimbAxisLeft(PhyGirdle->GetLimb(EGirdleRay::Center));

	//тот пояс, который оказывается оторван от земли, плавно переводить в малоподвижное состояние
	if (PhyGirdle->StandHardness > 25) AnimGirdle.Stands = PhyGirdle->StandHardness/255.0f;
	else AnimGirdle.Stands = FMath::Lerp(AnimGirdle.Stands, PhyGirdle->StandHardness, 0.1f);

	//получить адреса выходного пучка данных
	float& RUpDown = LimbChunk[0];
	float& LUpDown = LimbChunk[4];

	if (PhyGirdle->HasLegs)
	{

		//вот через такую жопу добывается радиус сферы - тут важно, чтобы сфера существовала, но проверять надо не здесь
		const auto CeLimb = PhyGirdle->GetLimb(EGirdleRay::Center);
		const auto SpLimb = PhyGirdle->GetLimb(EGirdleRay::Spine);
		//SetLimbTransform(PhyGirdle->GetLimb(EGirdleRay::Right), CeLimb, SpLimb, &RUpDown, WheelRadius, AnimGirdle, -1);
		//SetLimbTransform(PhyGirdle->GetLimb(EGirdleRay::Left), CeLimb, SpLimb, &LUpDown, WheelRadius, AnimGirdle, 1);
		SetLegPosition(PhyGirdle->GetLimb(EGirdleRay::Right), &RUpDown);
		SetLegPosition(PhyGirdle->GetLimb(EGirdleRay::Left), &LUpDown);
	}
	//нет ног
	else
	{
		SetLegPosition(PhyGirdle->GetLimb(EGirdleRay::Right), &RUpDown);
		SetLegPosition(PhyGirdle->GetLimb(EGirdleRay::Left), &LUpDown);

		//даже если нет ног, FrontBack управляет вытягиванием/махом пояса вперед назад
		//float& FrontBack = LimbChunk[1];
		//ROTATE(FrontBack, true, PELVIS, Up, LUMBUS, Front, AllowMoreCalc);
	}
}


//==============================================================================================================
//получить букет трансформаций конечности в правильном формате из физ-модели существа
//==============================================================================================================
void UMyrPhyCreatureAnimInst::SetLegPosition(FLimb& Limb, float* LimbChunk)
{
	//получить адреса выходного пучка данных

	FVector3f* OutPos = (FVector3f*)LimbChunk;
	float& FootPitch = LimbChunk[3];
	const auto M = Creature->GetMesh();
	auto Girdle = Creature->GetGirdle(Limb.WhatAmI);
	
	//отладка
	LINEWT(ELimbDebug::FeetShoulders, M->GetLimbShoulderHubPosition(Limb.WhatAmI), (FVector)Girdle->GetLegRay(Limb), Limb.Stepped?1:0.5, 0);

	//перевести координаты в диапазоны blendspace
	FVector3f NewFootRay = Girdle->GetRelLegRay(Limb);
	if (Limb.IsLeft()) NewFootRay.Z *= -1;
	NewFootRay = NewFootRay / (2 * Girdle->TargetFeetLength) + FVector3f(0.5f);
	*OutPos = FMath::Lerp(*OutPos, NewFootRay, 0.3f);

	//загиб ступни вперед/назад
	FootPitch = Limb.FootPitch();


	//для отладки
	//LINE(ELimbDebug::FeetShoulders, LimbAxisPos, M->GetLimbAxisForth(SpineLimb.WhatAmI) * FrontBack);
	//LINE(ELimbDebug::FeetShoulders, LimbAxisPos, M->GetLimbAxisUp(SpineLimb.WhatAmI) * UpDown);
	//LINE(ELimbDebug::FeetShoulders, LimbAxisPos, M->GetLimbAxisRight(SpineLimb.WhatAmI) * LeftRight);

}


//==============================================================================================================
//вычислить параметр для изгиба видимой части тела, подстраиваясь к изгибу нужной физдвижковой части
//всё в сборе, вкл/выкл, подгон диапозона под 0-1, оптимизация, вычислять ли точно арксинус или апроксимировать скалярным произведением
//==============================================================================================================
void UMyrPhyCreatureAnimInst::DriveBodyRotation(float& Dest, bool En, ELimb LLo, EMyrRelAxis LoA, ELimb LHi, EMyrRelAxis HiA, bool Asin)
{
	if (!En) { Dest = 0.5; return; }
	Dest = Creature->GetMesh()->GetLimbAxis(LLo, LoA) | Creature->GetMesh()->GetLimbAxis(LHi, HiA);
	if (Asin) Dest = 0.5 + FMath::FastAsin(Dest)/3.1415926;
	else Dest =  0.5 + Dest/2;
}
