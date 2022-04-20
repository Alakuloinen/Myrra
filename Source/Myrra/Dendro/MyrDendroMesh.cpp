// Fill out your copyright notice in the Description page of Project Settings.

#include "MyrDendroMesh.h"
#include "Animation/AnimInstance.h"
#include "Runtime/Engine/Classes/Animation/PoseAsset.h"
#include "Runtime/Engine/Classes/Animation/AnimSequence.h"
#include "Runtime/Engine/Classes/Animation/AnimClassInterface.h"
#include "DrawDebugHelpers.h"
//#include "Contactor.h"

//==============================================================================================================
//конструктор
//==============================================================================================================
UMyrDendroMesh::UMyrDendroMesh(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
	//вот это уже важно, иначе не видать отдельных костей в момент перекрытия
	bMultiBodyOverlap = true;

	//подвязка обработчиков пересечений
	OnComponentHit.AddDynamic(this, &UMyrDendroMesh::OnHit);

	//подвязка собственного класса анимации - в идеале делающего всё
	//в реале как это сделать хз, поэтому всего лишь родителя разных AnimBP для разных форм веток
	SetAnimInstanceClass(UMyrDendroAnimInst::StaticClass());

	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;					// чтобы не тикало в редакторе 
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.Target = this;
	PrimaryComponentTick.SetTickFunctionEnable(true);
	PrimaryComponentTick.TickInterval = 0.23;							// сюда бы что-то более редкое

	//оптимизация - веток много, тиковать их не нужно, когда не видно или далеко
	bEnableUpdateRateOptimizations = 1;
	VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	//габариты ветки считать не по физ-ассету, чтобы не гасил листву, когда бревна вне поля зрения
	bComponentUseFixedSkelBounds = true;

}

//==============================================================================================================
//передать все данные в инстанцию анимации - пока нормально не работает, не откликается
//==============================================================================================================
void UMyrDendroMesh::TranslateData(FDENDROFORM* NewForm)
{
	//подача формы извне - для гиба веток в масштабе всего дерева
	if(NewForm) DendroForm = *NewForm;

	//если имеется правильный контроллер анимации
	if (UMyrDendroAnimInst* AnimInst = Cast<UMyrDendroAnimInst>(GetAnimInstance()))
	{
		//просто передать переменные
		AnimInst->DendroForm = DendroForm;

		//произвольная поза зависит от
		AnimInst->CurrentRandomPose = FMath::RandRange(0.0f, 0.3f);
		//AnimInst->BasePoses = BasePoses;
		//AnimInst->RandomPoses = RandomPoses;
		AnimInst->InitializeAnimation();
		AnimInst->UpdateAnimation(0.01f, false);

		UE_LOG(LogTemp, Log, TEXT("Branch %s TranslateData Pos=%g, ----- Raise=%g, Curl=%g, Slope=%g, Spread=%g"),
			*GetName(),
			AnimInst->CurrentRandomPose,
			DendroForm.RaiseUpDown,
			DendroForm.CurlUpDown,
			DendroForm.SlopeRightLeft,
			DendroForm.SpreadCondense	);
	}

	//как взять объекты UPhysicsConstraint из skeletal mesh
	//обновить
	//тут пока непонятно, как правильно обновлять, но пока что физика ветвей отключена
	for( auto& ConstrInst : Constraints )
	{
		//ConstrInst -> UpdateAngularLimit ();
		//ConstrInst -> SetAngularPositionDrive (true, true);
	}

}


//==============================================================================================================
//всякие события инициализации - чтобы обновлять данные всегда, когда нужно, не не каждый кадр
//==============================================================================================================
#if WITH_EDITOR
void UMyrDendroMesh::PostEditChangeProperty (FPropertyChangedEvent & PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	//инициализация самого меша
	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UMyrDendroMesh, SkeletalMesh)))
	{
	}
	//изменение всех остальных свойств
	else
	{
		TranslateData();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

//это видимо нужно, чтобы в редакторе он выглядел
void UMyrDendroMesh::PostLoad()	
{	
	TranslateData();
	Super::PostLoad();
}	


//==============================================================================================================
//начать игру
//==============================================================================================================
void UMyrDendroMesh::BeginPlay()
{
	Super::BeginPlay();

	//включить детекцию столкновений. Эти параметры, видимо, придётся динамически менять
	SetCollisionProfileName(TEXT("Dendro"));
	SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	SetNotifyRigidBodyCollision(true);
	SetAllBodiesNotifyRigidBodyCollision(true);
	SetSimulatePhysics(false);						//полумолчанию физику отключить

	//вот это обязательно, без него ветки не переключают лоды самостоятельно
	bSyncAttachParentLOD = false;

	//PrimaryComponentTick.RegisterTickFunction(GetOwner()->GetLevel());	// из-за этой хни нельзя в конструкторе
	TranslateData();
}

//==============================================================================================================
// обработчик события столкновения / касания на уровне тел регдолла
//==============================================================================================================
UFUNCTION() void UMyrDendroMesh::OnHit (UPrimitiveComponent * HitComp, AActor * OtherActor, UPrimitiveComponent * OtherComp, FVector NormalImpulse, const FHitResult & Hit)
{
	//нас интересует только соприкосновение с живым существом
	if (APawn* HitPawn = Cast<APawn>(OtherActor))
	{
		//если симуляция физики вообще отключена, ничего не делать
		//пока физика отключена, а то как-то криво работает
		FBodyInstance* CurrentBodyWeTouch = GetBodyInstance(Hit.MyBoneName);
		if (CurrentBodyWeTouch->bSimulatePhysics)
		{
			//включить физдвижок для всех нижележащих сегментов ветки
			//и запустить тик-функцию для постепенного расслабления ветки
			UE_LOG(LogTemp, Log, TEXT("Drevo OnHit by %s"), *Hit.MyBoneName.ToString());
			//SetAllBodiesBelowSimulatePhysics(Hit.MyBoneName, true, true);
			//SetAllBodiesBelowPhysicsBlendWeight(Hit.MyBoneName, 1.0);
			PrimaryComponentTick.SetTickFunctionEnable(true);

			//запретить восстановление к кинематическому состоянию
			//этот флаг должны снять вручную извне, чтобы ветка снова заснула
			//StandStill = true;
		}
	}
}

//==============================================================================================================
//выдать толщину ветки, именно толщину основного сука ветки, для отбора веток, подходящих по толщине к стволу
//==============================================================================================================
float UMyrDendroMesh::FindBranchThickness(USkeletalMesh* BranchModel)
{
	//здесь берется нулевой членик, потому что СУКА НЕТ ФУНКЦИИ GetRootBone. Как правильно искать корневой сегмент, пока непонятно
	auto AggGeom = BranchModel->GetPhysicsAsset()->SkeletalBodySetups[0]->AggGeom;
	if (AggGeom.SphylElems.Num() > 0) return AggGeom.SphylElems[0].Radius * 2;
	else if (AggGeom.SphereElems.Num() > 0) return AggGeom.SphereElems[0].Radius * 2;
	return 0.0f;
}

//==============================================================================================================
// покадровое обновление (нужен ли сниженный темп?)
//==============================================================================================================
void UMyrDendroMesh::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	//здесь выключение всяких вычислений костей, если уровень детализации не первый, то есть на минимальном отджалении
	if(GetPredictedLODLevel() > 0) bRenderStatic = true;
	else bRenderStatic = false;


/*	bool needsmore = false;
	UE_LOG(LogTemp, Log, TEXT("Drevo restore "));

	//зафиксировать состояние ветки
	//return;

	//ослабление физики только если не включен режим "зажатия"
	if(!StandStill)
	{
		//для каждого сегмента ветки уменьшаем вес физики - возвращаем в состояние покоя
		for (auto BI : Bodies)
		{
			BI->PhysicsBlendWeight -= 0.01f;
			if (BI->PhysicsBlendWeight > 0.0f)
				needsmore = true;
			else
			{
				BI->PhysicsBlendWeight = 0.0f;
				//BI->bSimulatePhysics = false;
			}
		}
	}

	//все веса физики вернулись в нуль и физика больше не нужна
	if(!needsmore) 	PrimaryComponentTick.SetTickFunctionEnable(false);*/

	//здесь много полезного, например перериосвка в измененной позе
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

//==============================================================================================================
//изменение иерархии - если это ветка, она после создания цепляется к стволу, и вызывается это
//==============================================================================================================
void UMyrDendroMesh::OnAttachmentChanged()
{
	Super::OnAttachmentChanged();

	//вот это обязательно, без него ветки не переключают лоды самостоятельно
	bSyncAttachParentLOD = false;
}

//==============================================================================================================
// взять имя физического объема, который в physics asset висит по данному индексу. костыль №Х.
// кабы разработчики вообще не отменили наличие / редактируемость этого свойства
//==============================================================================================================
FName UMyrDendroMesh::GetBodyName(int i, UPhysicsAsset* Pha)
{
	//неужели нет стандартной функции, делающей эту рутину?
	const auto AggGeom = Pha->SkeletalBodySetups[i]->AggGeom;
	if (AggGeom.SphereElems.Num() > 0)	return AggGeom.SphereElems[0].GetName(); else
		if (AggGeom.SphylElems.Num() > 0)	return AggGeom.SphylElems[0].GetName(); else
			if (AggGeom.TaperedCapsuleElems.Num() > 0)	return AggGeom.TaperedCapsuleElems[0].GetName(); else
				if (AggGeom.BoxElems.Num() > 0)	return AggGeom.BoxElems[0].GetName();
				else return NAME_None;
}

FRotator UMyrDendroMesh::GetBodyTrueRotation(int bodyindex, UPhysicsAsset* PhA)
{
	//неужели нет стандартной функции, делающей эту рутину?
	const auto AggGeom = PhA->SkeletalBodySetups[bodyindex]->AggGeom;
	if (AggGeom.SphereElems.Num() > 0)	return FRotator(); else
		if (AggGeom.SphylElems.Num() > 0)	return AggGeom.SphylElems[0].Rotation; else
			if (AggGeom.TaperedCapsuleElems.Num() > 0)	return AggGeom.TaperedCapsuleElems[0].Rotation; else
				if (AggGeom.BoxElems.Num() > 0)	return AggGeom.BoxElems[0].Rotation;
				else return FRotator();
}

//==============================================================================================================
//длина физ-членика ветки/ствола
//==============================================================================================================
float UMyrDendroMesh::GetBodyLength(int i, UPhysicsAsset* Pha)
{
	//неужели нет стандартной функции, делающей эту рутину?
	const auto AggGeom = Pha->SkeletalBodySetups[i]->AggGeom;
	if (AggGeom.SphereElems.Num() > 0)	return AggGeom.SphereElems[0].Radius*2; else
		if (AggGeom.SphylElems.Num() > 0)	return AggGeom.SphylElems[0].Length; else
			if (AggGeom.TaperedCapsuleElems.Num() > 0)	return AggGeom.TaperedCapsuleElems[0].Length; else
				if (AggGeom.BoxElems.Num() > 0)	return AggGeom.BoxElems[0].Z;
				else return 0.0f;
}



//эти функции объявлены в теле, но здесь продублированы для понятности

//получить направление роста текущего сегмента ветви - для управления маршрутом вдоль ветки
//(почему Y хз, видимо так экспортировали кости - надо поправить на Х, или куда там ориентированы кости?)
//FVector GetBranchDirection (uint32 BodyIndex) const { return Bodies[BodyIndex]->GetUnrealWorldTransform().GetUnitAxis(EAxis::Y); }

//направление ветки с учётом желаемого хода - по ветке или против ветки
//FVector GetBranchDirection (uint32 BodyIndex, const FVector& InDir) const { auto g = GetBranchDirection(BodyIndex); if(FVector::DotProduct(g,InDir)>0) return g; else return -g;  }

//направление на седло ветки - подшаманенная трансформация ветки так, чтобы исключить её крен (ПОЧЕМУ МИНУС?)
//FVector GetRollFreeUpDirection (uint32 BodyIndex)	{ return FRotationMatrix::MakeFromXY (-GetBranchDirection (BodyIndex), FVector::RightVector).GetUnitAxis (EAxis::Z); }


//==============================================================================================================
// толщина текущего сегмента ветки (ВНИМАНИЕ! Это диаметр! Не радиус!)
//==============================================================================================================
float UMyrDendroMesh::GetBranchWidth (uint32 BodyIndex)
{
	//неужели нет стандартной функции, делающей эту рутину?
	const auto Aggregator = GetPhysicsAsset() -> SkeletalBodySetups[BodyIndex] -> AggGeom;
	if (Aggregator.SphereElems	.Num() > 0)	return Aggregator.SphereElems[0]	.Radius * 2; else
	if (Aggregator.SphylElems	.Num() > 0)	return Aggregator.SphylElems[0]		.Radius * 2; else
	if (Aggregator.BoxElems		.Num() > 0)	return Aggregator.BoxElems[0]		.Y * 2;
	else return 1000.0f;
}

//==============================================================================================================
//длина всей ветки - требуется для подбора веток под место
//==============================================================================================================
float UMyrDendroMesh::GetLength()
{
	//проработать оси, теоретически здесь должна быть Х
	return Bounds.BoxExtent.Y * 2;
}

//==============================================================================================================
//место в центре сегмента ветки, точка посадки
//==============================================================================================================
FVector UMyrDendroMesh::GetBranchCenterSeat (int32 BodyIndex)
{
	//центр масс капсулы сегмента ветки + сместить вверх на толщину капсулы - в это место можно приземлять
	return Bodies[BodyIndex] -> GetCOMPosition() + GetRollFreeUpDirection(BodyIndex)*GetBranchWidth(BodyIndex);
}

//==============================================================================================================
// получить рекомендуемое направление, чтобы эффективно идти вдоль ветки
// функция несамостоятельная - ее надо вызывать из некоего OnHit события, так как сюда подаются параметры
// точки контакта. Она также не дает реальной скорости/импулся/силы, а только коэффициент воздействия
//==============================================================================================================
FVector UMyrDendroMesh::GetMoveGuideDirection (int32 BodyIndex, FVector ConctactPoint, FVector ContactNormal, FVector MoveVec, float FallTolerance)
{
	//направильная ветка (почему?) - неважно, оставляем изначально желаемое направление движения
	if (BodyIndex == 255) return MoveVec;

	//абсолютное направдение сегмента ветки
	const FVector BranchDir = GetBranchDirection (BodyIndex);

	//если ветка растёт сильно вертикально, то вычисления седла не нужны, просто устремить вдоль ветки 
//	if (FMath::Abs(BranchDir.Z) > 0.7f)
//		return FVector::VectorPlaneProject (MoveVec, ContactNormal);
//		return MoveVec;

	//берется реальное направление ветки и какая-то абсолютная сторона "право"
	//в итоге получается трансформация, где ось Z наклонена только по тангажу ветки, но не по крену
	const FVector AlignedZAxis = GetRollFreeUpDirection(BodyIndex); 

	//коэффициент сонаправленности вектора желаемого жвижэения и вектора ветки
	float AlongAmount = FVector::DotProduct(BranchDir, MoveVec);

	//проекция вектора движения на ось ветки
	FVector OutVec = BranchDir * AlongAmount;

	//коэффициент сонаправленности мог быть и меньше 0, если вектор ветки нам в зад
	AlongAmount = FMath::Abs(AlongAmount);

	//если проекция слишком короткая, значит направление изначально почти ПОПЕРЁК ветки
	//что может значить желание спрыгнуть или дурость - предоставить такую возможность
	//например, если FallTolerance = 0.2, то проекция желаемого курса на ось ветки должна
	//стать 1/5 от длины изначального вектора движения, чтобы боковое движение начало учитываться
	if (AlongAmount < FallTolerance)
	{
		return FMath::Lerp(OutVec, MoveVec, FallTolerance - AlongAmount);
	}

	//поправка на СОСКАЛЬЗЫВАНИЕ - найти касательную "в горку" и добавить к основному направлению (=вперед и вверх)
	//векторное произведение пропорционально синусу угла (здесь sin(П/2) = 1.0) и модулю входного вектора (нормаль единична), т.е. нормирование не нужно
	//коэффициент "опасности скатывания" = Z-координата касательной, которая, если >0, то прибавляем как есть, если <0, то противоположный
	//если висим отвесно - Z-координата "в горку" = 1.0 (резко вверх), и наоборот, если мы на самом горбу ветки SideTangent.Z=0 (голый путь по ветке)
	FVector SideTangent = FVector::CrossProduct (OutVec, ContactNormal); 
	OutVec = OutVec + SideTangent * SideTangent.Z;

	//диагностика
#if WITH_EDITOR
	UE_LOG(LogTemp, Log, TEXT("BRANCH %s: GetMoveGuideDirection body=%d fall=%g, (%g;%g;%g) >> (%g;%g;%g)"),
		*GetName(),
		BodyIndex, FallTolerance,
		BranchDir.X, BranchDir.Y, BranchDir.Z, OutVec.X, OutVec.Y, OutVec.Z);
	DrawDebugLine(GetWorld(),ConctactPoint,ConctactPoint + OutVec * 20, FColor(25, 0, 45), false, 1, 100, 1);
#endif

	return OutVec;
}

//==============================================================================================================
//задать первый узел в маршруте между целевым сегментом и тем, на котором мы сидим
//АХТУНГ: можно вызывать только если я и он сидят на одном дереве, проверить перед вызовом!
//АХТУНГ: вероятно, она работает только если всем костям соответствуют членики, т.е. типичное дерево
//==============================================================================================================
uint8 UMyrDendroMesh::GetFirstBodyToGetToGoal ( uint8 BodyWeReOn, uint8 GoalBody)
{
	//элементарный случай
	if (BodyWeReOn == GoalBody) return GoalBody;

	//перейти от физ-членика к кости
	auto BoneMy = Bodies[BodyWeReOn]->InstanceBoneIndex;
	auto BoneUr = Bodies[GoalBody]->InstanceBoneIndex;
	uint8 BoneGoal = BoneMy;

	//строим цепочки пересадок до корня дерева от нашей кости и от его кости Path[0] - наша кость, 
	//внимание, после этих проходов переменные BoneUr, BoneMy губятся до -1
	uint8 PathMy[10]; memset(PathMy, 0, 10); int i = 0; 
	uint8 PathUr[10]; memset(PathUr, 0, 10); int j = 0;
	while (BoneMy != -1) { PathMy[i] = BoneMy;	BoneMy = SkeletalMesh->GetRefSkeleton().GetParentIndex(BoneMy); i++; }
	while (BoneUr != -1) { PathUr[j] = BoneUr;	BoneUr = SkeletalMesh->GetRefSkeleton().GetParentIndex(BoneUr); j++;	}

	//сличение построенных путей с конца (с корневой кости)
	int k=0;
	while (k<i && k<j)
	{
		//концы путей одинаковы - значит мы с целью на одной подветви
		if(PathMy[i-1-k] == PathUr[j-1-k]) k++;

		//если мы дошли до нас (PathMy[i-1-k]=BoneMy), значит цель на нашей подветви, но ещё доальше от конца, взять узел параллельный нашему, но на 1 шаг ближе к цели
		else if (k==i+1) BoneGoal = PathUr[j-1-k-1];

		//дошли до цели (PathMy[i-1-k]=BoneMy), значит цель ближе к корню, а мы на более дальней ветке, ступаем на 1 шаг вниз
		else if (k==j+1) BoneGoal = PathMy[1];

		//общий путь разделяется, но до нужных костей еще несколько шагов? просто спуститься со своего места на 1 кость к центру
		else BoneGoal = PathMy[1];
	}

	// перевести обратно в индекс членика. может выдать неверный индекс, поэтому, возможно, устроить тут доп. проверку / сдвиг по массиву
	return GetPhysicsAsset() -> FindBodyIndex (GetBoneName(BoneGoal));
}

//упрощённая версия, когда нам точно нужно к родительской кости и так до корня
uint8 UMyrDendroMesh::GetFirstBodyToGetToRoot ( uint8 BodyWeReOn)
{
	//перейти от физ-членика к кости
	auto BoneMy = Bodies[BodyWeReOn]->InstanceBoneIndex;
	
	//перевести обратно в индекс членика. может выдать неверный индекс, поэтому, возможно, устроить тут доп. проверку / сдвиг
	return GetPhysicsAsset() -> FindBodyIndex (GetBoneName(SkeletalMesh->GetRefSkeleton().GetParentIndex(BoneMy)));
}

