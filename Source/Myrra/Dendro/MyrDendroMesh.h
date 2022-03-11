// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "MyrDendroAnimInst.h"
#include "MyrDendroMesh.generated.h"

//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYRRA_API UMyrDendroMesh : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:

	//к какому сегменту вышестоящего меша мы крепимся (для детекции маршрута, пока не используется)
	UPROPERTY(Category = "Pose", EditAnywhere, BlueprintReadWrite)	uint8 BaseMeshBody;

	//дополнительная форма ветки - выбор рандома
	UPROPERTY(Category = "Pose", EditAnywhere, BlueprintReadWrite)	uint8 Pose;

	//всем скопом базовая форма ветки
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Shape)	FDENDROFORM DendroForm;

	//готовые ресурсы анимации, связанные с базовыми и дополнительными формами
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Shape)	class UPoseAsset *BasePoses;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Shape)	class UAnimSequence *RandomPoses;

public:

	//правильный конструктор
	UMyrDendroMesh(const FObjectInitializer& ObjectInitializer);

	//оброботчики судьбоносных событий
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent) override;
#endif
	virtual void PostLoad() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnAttachmentChanged() override;

	//передать все данные в анимационную инстанцию
	void TranslateData(FDENDROFORM* NewForm = nullptr);

	//вызов извне, когда полностью порвали связь с веткой - чтобы она начала отключаться
	void StartRecover() { /*StandStill = false;*/ }

public:

	//толщина текущего сегмента ветки (ВНИМАНИЕ! Это диаметр! Не радиус!)
	float GetBranchWidth (uint32 BodyIndex);

	//длина всей ветки
	float GetLength();

	//позиция центра масс, что не очень актуально для ветки
	FVector GetBranchPosition (uint32 BodyIndex) const { return Bodies[BodyIndex]->GetCOMPosition(); }

	//получить направление роста текущего сегмента ветви - для управления маршрутом вдоль ветки
	//(почему Y хз, видимо так экспортировали кости - надо поправить на Х, или куда там ориентированы кости?)
	FVector GetBranchDirection (uint32 BodyIndex) const { return Bodies[BodyIndex]->GetUnrealWorldTransform().GetUnitAxis(EAxis::Y); }

	//направление ветки с учётом желаемого хода - по ветке или против ветки
	FVector GetBranchDirection (uint32 BodyIndex, const FVector& InDir) const { auto g = GetBranchDirection(BodyIndex); if(FVector::DotProduct(g,InDir)>0) return g; else return -g;  }

	// общая сонаправленность желаемого направления и данной ветки, единица - если мы прямо вдоль, 0 - если мы перпендикулярно ветке
	float GetBranchCoincidence (uint32 BodyIndex, const FVector& WishedDir) const { return FMath::Abs( FVector::DotProduct( WishedDir, GetBranchDirection (BodyIndex) ) ); }

	//направление на седло ветки - подшаманенная трансформация ветки так, чтобы исключить её крен (ПОЧЕМУ МИНУС?)
	FVector GetRollFreeUpDirection (uint32 BodyIndex)	{ return FRotationMatrix::MakeFromXY (-GetBranchDirection (BodyIndex), FVector::RightVector).GetUnitAxis (EAxis::Z); }

	//получить рекомендуемое направление, чтобы эффективно идти вдоль ветки (= +автоподталкивание на гребне + терминальный угол для спрыгивания с ветки)
	FVector GetMoveGuideDirection (int32 BodyIndex, FVector ConctactPoint, FVector ConctactNormal, FVector MoveVec, float FallTolerance = 0.2f);

	//место в центре сегмента ветки, точка посадки
	FVector GetBranchCenterSeat (int32 BodyIndex);

	//задать первый узел в маршруте между целевым сегментом и тем, на котором мы сидим
	uint8 GetFirstBodyToGetToGoal ( uint8 BodyWeReOn, uint8 GoalBody);
	uint8 GetFirstBodyToGetToRoot ( uint8 BodyWeReOn);

	//взять имя и длину физического объема, который в physics asset висит по данному индексу
	static FName GetBodyName(int bodyindex, class UPhysicsAsset* PhA);
	static FRotator GetBodyTrueRotation(int bodyindex, class UPhysicsAsset* PhA);
	static float GetBodyLength(int bodyindex, class UPhysicsAsset* PhA);


protected:

	//реакция на касание (включение физдвижка)
	UFUNCTION()	void OnHit (UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:

	//выдать толщину ветки, именно толщину основного сука ветки, для отбора веток, подходящих по толщине к стволу
	static float FindBranchThickness(class USkeletalMesh* BranchModel);
};
