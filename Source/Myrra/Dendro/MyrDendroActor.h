// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyrDendroMesh.h"
#include "AssetStructures/MyrDendroInfo.h"			// агрегатор настроек для вида дерева, чтобы не плодить сущности в объектах
#include "Engine/Classes/Engine/SkeletalMesh.h"
#include "MyrDendroActor.generated.h"



//###################################################################################################################
// класс целостного объекта дерева. Содержит обязательный комопнент ствол+венчик (скелетный, модифицированный)
// который показывается на небольшом расстоянии и обязательный компонент-крона (статический), который показывается
//набольшом расстоянии и исчезает вблизи. Крона - крайне простой и быстрый меш.
// при добавлении на сцену к стволу генерируются ветви (скелетные), того же класса, что и ствол. По ним можно лазить.
// Ветви также исчезают на расстоянии. Гнеофонд дерева, откуда объект берет все параметры - свойство DendroInfo
// из этого класса определяются Blueprint-классы разных видов деревьев, различающиеся мешами стволов
//наборы веток и материалы коры/листьев определяются полем DendroInfo, которым подцепляется отдельный файл ресурсов
//###################################################################################################################
UCLASS(HideCategories = (Mesh, Material, Shape, Physics, Navigation, Collision, Animation)) class MYRRA_API AMyrDendroActor : public AActor
{
	GENERATED_BODY()

public:

	//▄▀ - обязательные компоненты - 

	//ствол - того же типа, что и ветки, но сосдаётся по умолчанию изначально
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UMyrDendroMesh* Trunk = nullptr;

	//указатель на компонент крона, которая отображается вместо всех веток 
	//(пока непонятно, насколько автоматически это должно переключаться)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UStaticMeshComponent* Crown = nullptr;

	//генофонд дерева, агрегатор всех параметров, по которым можно сгненрировать это дерево с основы до полноценного вида
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
		UMyrDendroInfo* DendroInfo = nullptr;

public:	

	// Sets default values for this actor's properties
	AMyrDendroActor();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//здесь мы будем искать ствол в списке созданных компонентов
	virtual void PostInitializeComponents() override;

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//для запуска перестройки структуры дерева после изменения параметров
#if WITH_EDITOR
	virtual void PostEditChangeProperty (FPropertyChangedEvent & PropertyChangedEvent) override;
#endif

public:	

	//высота ствола, из соображения габаритов, если ствол вообще есть
	float TrunkHeight() { if (Trunk) return Trunk->Bounds.BoxExtent.Z * 2; else return 1.0f; }

	//сгенерировать ветви на основе кол-ва, модели ствола (и сокетов, откуда растут), списка моделей примеров ветвей
	void GenerateBranches();
	void RemoveBranches();
	void UpdateBranches();

	//переназначить материал всем веткам (типа в редакторе выбрал березу и все векти березовые)
	void UpdateBranchesMaterials();

	//проверить, сгенерированы ли уже ветви
	bool AreBranchesGenerated();

protected:

	// найти ветки среди примеров, подходящие под требования
	void FindConsistentBranches (float MinLength, float MaxLength, float Spread, float Thickness, USkeletalMesh** OutBranches, const int NumMax, int& NumFound);

	//создать ветку в заданном месте ствола по заданной модели (вызывается внутри GenerateBranches)
	UMyrDendroMesh* GenerateBranch	(USkeletalMesh* Model, FVector Pos, FQuat Rot, FString NewName, FName Socket);

};
