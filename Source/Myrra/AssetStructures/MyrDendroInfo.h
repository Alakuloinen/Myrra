#pragma once
#include "CoreMinimal.h"
#include "../Myrra.h"
#include "Curves/CurveVector.h"		//для высчета по кривым
#include "MyrDendroInfo.generated.h"

//###################################################################################################################
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, hidecategories = (Object), meta = (BlueprintSpawnableComponent), Config = Game)
class MYRRA_API UMyrDendroInfo : public UObject
{
	GENERATED_BODY()
	friend class AMyrDendroActor;

	//число веток на данной высоте (0 - у корней, 1 - на верхушке), берется случайное в биапазоне между целочисленными срезами любых кривых
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Shape") UCurveVector* NumberOfBranchesRangeOverHeight;

	//длина ветки на данной высоте (0 - у корней, 1 - на верхушке), в долях от высоты всего дерева
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Shape") UCurveVector* LengthOverHeight;

	//уклон ветки на данной высоте (0 - у корней, 1 - на верхушке), берется случайное в биапазоне между целочисленными срезами любых кривых
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Shape") UCurveVector* SlopeRangeOverHeight;

	//загиб векток вверх или вниз на данной высоте (0 - у корней, 1 - на верхушке), берется случайное в биапазоне между целочисленными срезами любых кривых
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Shape") UCurveVector* CurlUpDownRangeOverHeight;

	//анимационная сборка, чтобы гнуть ветки
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Pose") TArray<TSubclassOf<UAnimInstance>> BranchAnimInst;

	//набор вариантов 3д-моделей веток из которых в произвольном порядке и в зависимости от размеров генерируется крона
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Examples") TArray<USkeletalMesh*> BranchExamples;

	//материал коры всех веток и ствола
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Material") UMaterialInterface* BranchWoodMaterial = nullptr;

	//материал листьев
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Material") UMaterialInterface* LeavesMaterial = nullptr;

public:

	//для вывода диапазона из трехмерновекторной кривой - береётся максимальное и второе за максимальным значения
	void GetSaneRangeFromCurveVector(UCurveVector* kurwa, const float Height, float& MinVal, float& MaxVal)
	{
		if (!kurwa) return;
		auto Bundle = kurwa->GetVectorValue(Height);
		MaxVal = FMath::Max3 (Bundle.X, Bundle.Y, Bundle.Z);
		MinVal = FMath::Max3 (FMath::Min(Bundle.X, Bundle.Y), FMath::Min(Bundle.X, Bundle.Z), FMath::Min(Bundle.Z, Bundle.Y));
	}
};