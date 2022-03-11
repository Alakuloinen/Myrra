/// Fill out your copyright notice in the Description page of Project Settings.

#include "MyrDendroActor.h"
#include "Animation/AnimInstance.h"			//для подбора анимаций под ветви
#include "Animation/AnimClassInterface.h"	//для подбора анимаций под ветви
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "MyrDendroAnimInst.h"
#include "../AssetStructures/MyrDendroInfo.h"
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"


//==============================================================================================================
// Sets default values
//==============================================================================================================
AMyrDendroActor::AMyrDendroActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	//крона для дали
	Crown = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crown"));
	RootComponent = Crown;

	//ствол
	Trunk = CreateDefaultSubobject<UMyrDendroMesh>(TEXT("Trunk"));
	Trunk->SetupAttachment(RootComponent);

	if (HasAnyFlags(RF_ClassDefaultObject)) return;
	Trunk->SetSimulatePhysics(false);
}

//==============================================================================================================
// для запуска перестройки структуры дерева после изменения параметров
//==============================================================================================================
#if WITH_EDITOR
void AMyrDendroActor::PostEditChangeProperty (FPropertyChangedEvent & PropertyChangedEvent)
{	
/*	//this->Postcom
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	//замена примера
	if(PropertyName == GET_MEMBER_NAME_CHECKED(AMyrDendroActor, BranchExamplesActor))
	{
		//эта проверка не работает - попытка понять, блюпринт превью это или объект в сцене
		if(this != GetClass()->GetDefaultObject())
			ReregisterAllComponents();

		//найти и закешировать ствол, чтобы не путать его с ветвями и брать оттуда места для роста
		FindTrunk();

		//если на стволе были ветки, удалить старые
		RemoveBranches();

		//рандомно создать новые
		GenerateBranches();
	}

	//перестройка веток
	else
	if( PropertyName == GET_MEMBER_NAME_CHECKED(AMyrDendroActor, ShowBranchesOrCrown) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMyrDendroActor, CrownThickness))
	{
		//если на стволе были ветки, удалить старые
		RemoveBranches();

		//рандомно создать новые
		GenerateBranches();
	} 

	//обновление материалов
	else if ((PropertyName == GET_MEMBER_NAME_CHECKED(AMyrDendroActor, BranchWoodMaterial)) ||
		(PropertyName == GET_MEMBER_NAME_CHECKED(AMyrDendroActor, LeavesMaterial)))
	{ 
		UpdateBranchesMaterials();
	}
	else UpdateBranches();

	//------------------------------------------------------------------------------
	//проверяем изменение свойства количества веток
	//------------------------------------------------------------------------------
	*/


	//Have the editor rebuild the object
	Super::PostEditChangeProperty(PropertyChangedEvent);
	//RemoveBranches();
	GenerateBranches();
}
#endif

//==============================================================================================================
// Called when the game starts or when spawned
//==============================================================================================================
void AMyrDendroActor::BeginPlay()
{
	Super::BeginPlay();
	//RemoveBranches();
	GenerateBranches();
}

//==============================================================================================================
// здесь мы будем искать ствол в списке созданных компонентов
//==============================================================================================================
void AMyrDendroActor::PostInitializeComponents()
{
	//FindTrunk();
	Super::PostInitializeComponents();
}

//==============================================================================================================
// Called every frame
//==============================================================================================================
void AMyrDendroActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


//==============================================================================================================
// переназначить материал всем веткам (типа в редакторе выбрал березу и все векти березовые)
//==============================================================================================================
void AMyrDendroActor::UpdateBranchesMaterials()
{
	//указатели на метриал коры и листьев - отдельные переменные
	if (!DendroInfo) return;
	if (!DendroInfo->BranchWoodMaterial && !DendroInfo->LeavesMaterial) return;

	//если в стволе не подцеплена конкретная модкль
	if (!Trunk->SkeletalMesh) return;

	//не понятно, насколько быстрее цыклить по внутреннему массиву, чем создавать свой фильтрованный
	for (auto& Co : GetComponents())
	{
		//убедиться, что это ветка, и не ствол
		const auto Bra = Cast<UMyrDendroMesh>(Co);
		if (Bra)
		{
			//названия сегментов материала пока жёстко заданы, их надо импортировать с такими
			if (DendroInfo->BranchWoodMaterial)		Bra->SetMaterialByName(TEXT("cortex"), DendroInfo->BranchWoodMaterial);
			if (DendroInfo->LeavesMaterial)			Bra->SetMaterialByName(TEXT("folia"),  DendroInfo->LeavesMaterial);

			//обновить форму
			Bra->TranslateData();
		}
	}

}

bool AMyrDendroActor::AreBranchesGenerated()
{
	return (GetComponents().Num() > 3);
}



//==============================================================================================================
// найти ветки среди примеров, подходящие под требования
//==============================================================================================================
void AMyrDendroActor::FindConsistentBranches (float MinLength, float MaxLength, float Spread, float Thickness, USkeletalMesh** OutBranches, const int NumMax, int& NumFound)
{
	int ci = 0;				// индекс заполняемого элемента
	bool refine = false;	// повтор/улучшение выборки

	NumFound = 0; if (!NumMax) return;

	//не понятно, насколько быстрее цыклить по внутреннему массиву, чем создавать свой фильтрованный
	for (auto Bra : DendroInfo->BranchExamples)
	{
		if (!Bra->GetPhysicsAsset())
		{	UE_LOG(LogTemp, Error, TEXT("ACTOR %s:-----ERROR: branch %s lacks PhysicsAsset"), *GetName(), *Bra->GetName());
			continue;
		}
		//расчет опорных размеров новой ветки
		float BranchLength = Bra->GetBounds().BoxExtent.Y;
		float BranchWidth = Bra->GetBounds().BoxExtent.X;
		float BranchSpread = FMath::RadiansToDegrees(FMath::Atan2(BranchLength, BranchWidth*0.5f)*2.0f);
		float BranchThickness = UMyrDendroMesh::FindBranchThickness(Bra);

		//все условия скопом по верхнему краю, потому что мелким веткам рады в любой ситуации
		if (BranchLength <= MaxLength && BranchThickness < Thickness)
		{
			//режим вторичного, привередливого выбора
			if (refine)
			{
				//нижний предел - критерий отбора лучших веток
				if (BranchSpread <= Spread && BranchLength >= MinLength)
				{
					//включить ветку в результат поиска
					OutBranches[ci] = Bra;
					ci++;

					//ёмкость результата, если превысили - далее перезалив с начала
					if (ci > NumMax) ci = 0;
				}
			}
			//режим первичного набора веток, пока не заполнится 
			else
			{
				//включить ветку в результат поиска
				OutBranches[ci] = Bra;
				ci++;

				//ёмкость результата, если превысили - включение режима перезалива с начала
				if (ci > NumMax) { ci = 0;	refine = true; }
			}
		}
	}
	
	//получение результирующего числа
	NumFound = refine ? NumMax : ci;

}


//==============================================================================================================
//создать ветку в заданном месте ствола по заданной модели
//==============================================================================================================
UMyrDendroMesh* AMyrDendroActor::GenerateBranch (USkeletalMesh* Model, FVector Pos, FQuat Rot, FString NewName, FName Socket)
{
	//клонировать объект из модельного к нам (оказывается, параметр Outer - в данном случае должен быть нашим актором)
	//и перенести клонированную ветку в иерархию нашего актора, подцепив к имеющемуся стволу
	UMyrDendroMesh* Branch = NewObject<UMyrDendroMesh> ( this, FName(*NewName) ) ;

	//внимание, если рантайм регистрировать компонент, то следом внутри сразу вызывается BeginPlay
	Branch->RegisterComponent();
	//AddOwnedComponent(Branch);
	Branch -> AttachToComponent (Trunk, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
	Branch->SetSkeletalMesh(Model);

	//применить вращение (сориентировав так, чтобы верхом вверх (хотя нужно ввести рандом)
	Branch->SetWorldLocationAndRotation(Pos, Rot);
	//Branch -> OnComponentCreated();

	//вернуть указатель, хотя пока это не нужно
	UE_LOG(LogTemp, Log, TEXT("ACTOR %s: - - GenerateBranch %s from %s, length %g"),
		*GetName(),
		*NewName,
		*Model->GetName(),
		Branch->GetLength()	);
	return Branch;
}


//==============================================================================================================
//сгенерировать ветви на основе кол-ва, модели ствола, моделей ветвей
//==============================================================================================================
void AMyrDendroActor::GenerateBranches()
{
	//если ветки уже сгенерированы
	if (AreBranchesGenerated()) return;

	//ствол 
	if (!Trunk->SkeletalMesh || !Trunk->Bodies.Num())
	{		UE_LOG(LogTemp, Log, TEXT("ACTOR %s: GenerateBranches - No Trunk"), *GetName());
		return;
	}
	if (!Trunk->SkeletalMesh->GetPhysicsAsset())
	{	UE_LOG(LogTemp, Log, TEXT("ACTOR %s: GenerateBranches - No PhysicsAsset"), *GetName());
		return;
	}
	//а нехрен это вызывать при редактировании блюпринта!
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{	UE_LOG(LogTemp, Log, TEXT("ACTOR %s: GenerateBranches - No, No, Not in Blueprint Window!"), *GetName());
		return;
	}

	//если заполнен пример (без примера вообще ничего сделать нельзя)
	//получить массив указателей на примеры веток
	if (!DendroInfo)
	{	UE_LOG(LogTemp, Log, TEXT("ACTOR %s: GenerateBranches No Gene Pool"), *GetName());
		return;
	}

	//список позиций, откуда растить ветки (включая голые кости, если имеются)
	TArray<FName> AllSockets = Trunk->GetAllSocketNames();
	int NumberOfBranches = AllSockets.Num();
	UE_LOG(LogTemp, Log, TEXT("ACTOR %s: GenerateBranches started %d"), *GetName(), NumberOfBranches);

	//--------------------------------------------------------------------------

	//процесс создания ветки - по всем местам, где можно сотворить ветку
	for (int i = 0; i < NumberOfBranches; i++)
	{
		const FName ChildSocketName = AllSockets[i];// взять имя кости этого места и родительскую кость, если есть
		FName ParentSocketName = NAME_None;			// родительская кость

		float ParentTruncSegmentLength = 1000;		// длина части ствола, на которой крепится ветка
		float TrunkSegmentThickness = 0.0;
		float BranchOnAngularArea = 1.0;				// коэффициент сдавленности простанства 2 ветками (если паралельны - ноль)
		float HeightLevel = 0;						// удельная высота посадки текущего узла в рамках высоты ствола (0-1)
		FTransform ParentTrans;						// трансформация корня ветки
		FTransform ChildTrans;						// трансформация следующего сегмента ствола
		FVector DirAway;							// направление наиболее свободного роста в бок, прочь от направлений обоих сегментов
		int BranchesInThisNode = 0;					// количество генерируемых боковых веток
		float InvNumBranches = 1.0;

		//родительская кость (если нема - корень ствола - это обработается позднее)
		ParentSocketName = Trunk->GetParentBone(ChildSocketName);

		//физ.членик обязан быть для реальных сегментов, если нет - модели нет на этом участке скелета, и ветки не нужны
		auto PhyChild = Trunk->GetBodyInstance(ChildSocketName);
		auto PhyParent = Trunk->GetBodyInstance(ParentSocketName);

		//отсутствие физчленика может объясняться более коротким деревом, которое не занимает все кости скелета
		if (!PhyChild) continue;

		//разбор строки имени физ-членика, здесь пока только сдавленность разрешенной для ветвей области
		FString NameToParse = UMyrDendroMesh::GetBodyName (PhyChild->InstanceBodyIndex, Trunk->SkeletalMesh->GetPhysicsAsset()).ToString();
		if (NameToParse.Len() > 2) BranchOnAngularArea = (NameToParse[1] - '0')*0.1f;


		UE_LOG(LogTemp, Log, TEXT("ACTOR %s: %d/%d GenerateBranches at node [parent:%s - child:%s]"), *GetName(), i, NumberOfBranches, *ParentSocketName.ToString(), *ChildSocketName.ToString());

		//необходим родитель, иначе это самый корень дерева, оттуда ветви не нужны
		if(PhyParent)
		{
			//трансформация текущего и родительского сегмента - берется из костей, не из физ-члеников
			ChildTrans = Trunk->GetSocketTransform (ChildSocketName);
			ParentTrans = Trunk->GetSocketTransform (ParentSocketName);

			//угол, откуда отситываются ветки, берется из ассета, из перекрученных члеников, чтобы они все как спицы на колесах не совпадали
			//как ни удивительно, капсулы-членики так повернуты, что нужный угол отмеряется по оси Pitch
			float StartAngleForBranches = UMyrDendroMesh::GetBodyTrueRotation(PhyChild->InstanceBodyIndex, Trunk->SkeletalMesh->GetPhysicsAsset()).Pitch;

			//направление вверх родительского сегмента 
			FVector ParentUpDir = -PhyParent->GetUnrealWorldTransform().GetScaledAxis(EAxis::Y);
			
			//длина берется прямо из физчленика ствола, на который надо навесить ветку
			ParentTruncSegmentLength = UMyrDendroMesh::GetBodyLength (PhyChild->InstanceBodyIndex, Trunk->SkeletalMesh->GetPhysicsAsset());

			//толщина сегмента ствола той же иерархии, что и создаваемые ветки
			TrunkSegmentThickness = Trunk->GetBranchWidth(i);

			//оценка высотности места для ветки
			HeightLevel = (ParentTrans.GetTranslation().Z - RootComponent->GetComponentLocation().Z) / TrunkHeight();

			//----------------------------------------------------------------------------------------
			//подготовка ограничений для поиска подходящих веток
			//----------------------------------------------------------------------------------------

			//диапазоны длин ветвей на данной высоте
			float MinLength = 0.0f;	float MaxLength = 0.0f;
			DendroInfo->GetSaneRangeFromCurveVector (DendroInfo->LengthOverHeight, HeightLevel, MinLength, MaxLength);
			MinLength *= TrunkHeight(); MaxLength *= TrunkHeight();
			
			//число ветвей в данном узле - рандом в промежутке между явно заданными значениями
			float MinNumber = 0.0f;	float MaxNumber = 0.0f;
			DendroInfo->GetSaneRangeFromCurveVector (DendroInfo->NumberOfBranchesRangeOverHeight, HeightLevel, MinNumber, MaxNumber);
			BranchesInThisNode = FMath::RandRange(MinNumber, MaxNumber);

			//если в данном узле решено не делать ветвей, вообще нах прервать и перейти к следующему узлу
			if (BranchesInThisNode == 0) continue;

			//инверсное число ветвей, чтобы не считать длинную операцию деления
			InvNumBranches = 1.0f / (float)BranchesInThisNode;

			//размах ветки - чем больше веток, тем более узкие выбирать, иначе не пролезть
			float DesiredSpread = FMath::Clamp(360.0f * InvNumBranches, 40.0f, 360.0f);

			//----------------------------------------------------------------------------------------
			//набирание моделей веток для данного узла
			//----------------------------------------------------------------------------------------
			int ModelsFound = 0;
			USkeletalMesh* OutBranches[10];
			memset(OutBranches, 0, 10 * sizeof(void*));
			FindConsistentBranches (MinLength, MaxLength, DesiredSpread*2, TrunkSegmentThickness, OutBranches, BranchesInThisNode, ModelsFound);
			UE_LOG(LogTemp, Log, TEXT("ACTOR %s: - Node %s on height %g, len=%g, up=%s, wanted len=(%g,%g), num(%g,%g)>>%d, spread=%g, width=%g >> found=%d"),
				*GetName(),
				*ParentSocketName.ToString(),
				HeightLevel,
				ParentTruncSegmentLength,
				*ParentUpDir.ToString(),
				MinLength, MaxLength,
				MinNumber, MaxNumber, BranchesInThisNode,
				DesiredSpread,
				TrunkSegmentThickness,
				ModelsFound	);

			//----------------------------------------------------------------------------------------
			// генерация ветвей на секции ствола
			//----------------------------------------------------------------------------------------
			if (ModelsFound)
			{
				//направление наиболее предпочтительного роста ветки (но если ветка наклонная, то нафиг чтобы ветка росла вниз
				DirAway = PhyParent->GetUnrealWorldTransform().GetScaledAxis(EAxis::Z);
				if (DirAway.Z < 0) { DirAway.Z = 0.01f;	DirAway.Normalize(); }

				//имя - базис для всех веток в узле
				FString PreName = TEXT("Branch_") + ParentSocketName.ToString() + TEXT("_");

				//строго по заданному числу
				for (int j = 0; j < BranchesInThisNode; j++)
				{
					//расположение ветки на стволе (через равные промежутки для простоты лазания)
					float NewBranchVerticalOffset = (float)j * InvNumBranches * ParentTruncSegmentLength;
					FVector NewBranchLoc = ParentTrans.GetTranslation() - ParentUpDir * NewBranchVerticalOffset;

					//переоценка высотности уже точного места севшей ветки
					HeightLevel = (NewBranchLoc.Z - RootComponent->GetComponentLocation().Z) / TrunkHeight();

					//вычисление диапазона поднятия вверх-вниз для ветки
					DendroInfo->GetSaneRangeFromCurveVector(DendroInfo->SlopeRangeOverHeight, HeightLevel, MinNumber, MaxNumber);
					float RaiseUpDown = FMath::RandRange(MinNumber, MaxNumber);

					//угловой сдвиг в плоскости XY для распределения веток вокруг узла
					float BranchRotationAngles = StartAngleForBranches + 180.0f * (-1.0f + 2.0f*j * InvNumBranches) * BranchOnAngularArea;

					//повернуть вектор влево родительского сегмента ствола на найденный угол (вокруг себя)
					FVector ActualBranchDir = DirAway.RotateAngleAxis(BranchRotationAngles, ParentUpDir);

					FQuat NewBranchRot = FRotationMatrix::MakeFromZX (ParentUpDir, ActualBranchDir).ToQuat();

					//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
					//непосредственная генерация
					auto NewNranchMesh = OutBranches[FMath::RandRange(0, ModelsFound - 1)];
					UMyrDendroMesh* ResultBranch = nullptr;
					ResultBranch = NewObject<UMyrDendroMesh>(this, FName(*(NewNranchMesh->GetName() + PreName + FString::FromInt(j))));
					ResultBranch -> AttachToComponent (Trunk, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ParentSocketName);
					ResultBranch -> SetSkeletalMesh (NewNranchMesh);
					ResultBranch -> SetWorldLocationAndRotation (NewBranchLoc, NewBranchRot);
					//ResultBranch->SetWorldTransform(ParentTrans);
					//ResultBranch->AddLocalRotation(FRotator(BranchRotationAngles, 0, -90));
					//ResultBranch->AddLocalRotation(FRotator(0, 0, -90));
					//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

					//вместо параметра для скелета сразу полымаем ветку на уровне трансформаций
					//однако, почему именно Roll? Криво ветки импортированы? Переимпортировать, чтобы был Pitch?
					ResultBranch->AddLocalRotation(FRotator(0, 0, RaiseUpDown*90));
					
					//расстояние видимости установить эквивалетнтным растоянию видимости ствола
					ResultBranch -> LDMaxDrawDistance = Trunk->LDMaxDrawDistance;

					//подцеп нужной анимационной сборки под скелет выбранной ветви
					ResultBranch -> SetAnimationMode (EAnimationMode::AnimationBlueprint);
					for(auto ABP : DendroInfo->BranchAnimInst)
					{	IAnimClassInterface* AnimClassInterface = IAnimClassInterface::GetFromClass(ABP);
						if (AnimClassInterface->GetTargetSkeleton() == ResultBranch->SkeletalMesh->GetSkeleton())
						{	ResultBranch -> SetAnimInstanceClass (ABP);
							break;
						}
					}

					//этот параметр был плохой идеей, так как можно было просто менять тангаж трансформации
					ResultBranch->DendroForm.CurlUpDown = 0;

					//изгиб вверх-вниз
					DendroInfo->GetSaneRangeFromCurveVector(DendroInfo->CurlUpDownRangeOverHeight, HeightLevel, MinNumber, MaxNumber);
					ResultBranch->DendroForm.CurlUpDown = FMath::RandRange(MinNumber, MaxNumber);

					//прочие искривления ветки, пока рандомно
					ResultBranch->DendroForm.SlopeRightLeft = FMath::RandRange(-0.1f, 0.1f);
					ResultBranch->DendroForm.SpreadCondense = FMath::RandRange(-0.1f, 0.1f);

					//названия сегментов материала пока жёстко заданы, их надо импортировать с такими
					if (DendroInfo->BranchWoodMaterial)		ResultBranch->SetMaterialByName(TEXT("cortex"), DendroInfo->BranchWoodMaterial);
					if (DendroInfo->LeavesMaterial)			ResultBranch->SetMaterialByName(TEXT("folia"), DendroInfo->LeavesMaterial);
					
#if WITH_EDITOR
					//это нужно, чтобы отобразилось в редакторе
					AddInstanceComponent(ResultBranch);
					ResultBranch->OnComponentCreated();
#endif

					//здесь должна обновляться форма
					ResultBranch->RegisterComponent();
#if WITH_EDITOR
					ResultBranch->TranslateData();
#endif
					UE_LOG(LogTemp, Log, TEXT("ACTOR %s: - - GenerateBranch %s from %s, length %g, offset=%g"),
						*GetName(),
						*ResultBranch->GetName(),
						*ResultBranch->SkeletalMesh->GetName(),
						ResultBranch->GetLength(),
						NewBranchVerticalOffset);
				}
			}

		}
		else UE_LOG(LogTemp, Log, TEXT(" - No Parent for %s"), *ParentSocketName.ToString());
	}

	//это нужно только в редакторе, при обновлении помещенных на сцену объектов
#if WITH_EDITOR
	RerunConstructionScripts();
#endif

}

//==============================================================================================================
//найтить и удалить предыдущие ветки
//==============================================================================================================
void AMyrDendroActor::RemoveBranches()
{
	//найтить и удалить предыдущие ветки
	TArray<UMyrDendroMesh*> MyBranches;
	GetComponents<UMyrDendroMesh>(MyBranches);
	UE_LOG(LogTemp, Log, TEXT("ACTOR %s: RemoveBranches %d"), *GetName(), MyBranches.Num());
	for (auto Br : MyBranches)
	{
		//удаляем содержимое каждого из найденных компонентов, только если это не ствол
		if (Br != Trunk) Br->DestroyComponent(true);
	}
}

//обновить анимационную форму всех ветвей
void AMyrDendroActor::UpdateBranches()
{
	for (auto Co : GetComponents()) 
	{
		if(auto Br = Cast<UMyrDendroMesh>(Co))
			if (Br != Trunk)
			{
/*				//коэффициент высоты ветки между корнем и верхушкой
				float HeightLevel = (Br->GetComponentLocation().Z - RootComponent->GetComponentLocation().Z) / TrunkHeight();

				//изгиб вверх-вниз
				Br->DendroForm.CurlUpDown = FMath::Lerp(
					BranchCurlingRange.GetLowerBoundValue(),
					BranchCurlingRange.GetUpperBoundValue(),
					HeightLevel) + FMath::RandRange(-BranchShapeVariation, BranchShapeVariation);

				Br->DendroForm.SlopeRightLeft = FMath::RandRange(-BranchShapeVariation*0.5f, BranchShapeVariation*0.5f);
				Br->DendroForm.SpreadCondense = FMath::RandRange(-BranchShapeVariation*0.5f, BranchShapeVariation*0.5f);*/

				Br->TranslateData();
			}
	}

}


