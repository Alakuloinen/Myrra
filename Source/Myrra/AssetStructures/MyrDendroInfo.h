#pragma once
#include "CoreMinimal.h"
#include "../Myrra.h"
#include "MyrDendroInfo.generated.h"

//###################################################################################################################
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType) class MYRRA_API UMyrDendroInfo : public UDataAsset
{
	GENERATED_BODY()

public:

	//набор вариантов 3д-моделей веток из которых в произвольном порядке и в зависимости от размеров генерируется крона
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<EDendroSection, USkeletalMesh*> Sections;

	//материал коры всех веток и ствола
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UMaterialInterface* BranchWoodMaterial = nullptr;

	//материал листьев
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UMaterialInterface* LeavesMaterial = nullptr;



};