// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "Components/CapsuleComponent.h"
#include "MyrGirdle.generated.h"

//###################################################################################################################
//новый пояс конечностей, и одновременно кинематический якорь и рыскалка
//###################################################################################################################
UCLASS() class MYRRA_API UMyrGirdle : public UCapsuleComponent
{
	GENERATED_BODY()

//свойства
public:

	//элементарная адресация передний/задний
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 IsThorax:1;

	//для простых существ физ-модель = ползок на брюхе и полностью кинематические ноги, для бипедалов руки тоже ни к чему
	//нахрена, неясно, это статический показатель, вытекающий из всей структуры
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 HasLegs:1;

	//если этот пояс (его центр) ограничен констрейнтом так, что поддерживается вертикальным
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 Vertical:1;

	//пояс ограничен так, что не может вращаться в стороны, для стабилизации и взбирания в вертикаль
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 NoTurnAround:1;

	//пояс держится фиксированным к опоре через констрейнт
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 FixedOnFloor:1;

	//пояс ползет по вертикальной опоре, обязательно наличие FixedOnFloor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 Climbing : 1;


//свойства
public:

	//кэш скорости ОТНОСИТЕЛЬНО опоры ног, принадлежащих этому поясу
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector3f VelocityAgainstFloor;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Speed;

	//вектор предпочтительного направления движения с учётом ограничений типа хождения по веткам
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector3f GuidedMoveDir;

	//степень пригнутости обеих ног (кэш с гистерезисом)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Crouch = 0.0f;

	//нормальная длина ноги, для расчётов подогнутия ног на рельефе
	//вычисляется в начале как разность позиций костей, может быть разной из-за масштаба меша
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float TargetFeetLength = 0.0f;

	//дайджест уровня крепкости стояния на опоре, 0 - совсем в воздухе, 255 = обеими ногами и еще брюхом 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 StandHardness = 0;

	//плечи конечностей в координатах центрального туловища
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector3f RelFootRayLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector3f RelFootRayRiht;

	//указатель на сборку настроек динамики/ориентации для этого пояса
	//прилетает с разных ветвей иерархии - из состояния BehaveState, далее из настроек (само)действия 
	FGirdleDynModels* CurrentDynModel = nullptr;



	//полный перечень ячеек отростков для всех двух поясов конечностей
	static ELimb GirdleRays[2][(int)EGirdleRay::MAXRAYS];

//возвращуны
public:

	//актор-существо в правильном типе
	class AMyrPhyCreature* MyrOwner() { return (AMyrPhyCreature*)GetOwner(); }
	class AMyrPhyCreature* MyrOwner() const { return (AMyrPhyCreature*)GetOwner(); }

	//меш этого существа
	class UMyrPhyCreatureMesh* me();
	class UMyrPhyCreatureMesh* mec() const;

	//выдать членик и его физ-тело в системе коориднат конкретно этого пояса
	ELimb			GetELimb(EGirdleRay R) { return GirdleRays[IsThorax][(int)R]; }
	ELimb			GetELimb(EGirdleRay R) const { return GirdleRays[IsThorax][(int)R]; }
	FLimb&			GetLimb (EGirdleRay R);
	const FLimb&	GetLimb (EGirdleRay R) const;
	FBodyInstance*	GetBody (EGirdleRay R);

	//констрайнт, которым спинная часть пояса подсоединяется к узловой (из-за единой иерархии в разных поясах это с разной стороны)
	FConstraintInstance* GetGirdleSpineConstraint();
	FConstraintInstance* GetGirdleArchSpineConstraint();

	//ведущий пояс
	bool Lead() const { return CurrentDynModel->Leading; }

	//пояс не стоит ногами на опоре
	bool IsInAir() const { return (GetLimb(EGirdleRay::Center).Stepped != STEPPED_MAX); }
	bool Stands(float Thr = 200) { return (StandHardness >= Thr);  }

	//режим попячки назад
	bool IsMovingBack() const;

	//скаляр скорости в направлении движения
	float SpeedAlongFront() { return (VelocityAgainstFloor | GuidedMoveDir); }

	//вычислить ориентировочное абсолютное положение конца ноги по геометрии скелета
	FVector GetFootTipLoc(ELimb eL);

	FVector3f& GetRelLegRay(FLimb& L) { return L.IsLeft() ? RelFootRayRiht : RelFootRayLeft; }

	//распаковать вектор на абсолютные координаты
	FVector3f GetLegRay(FLimb& L);

	//загрузить новый радиус вектор ноги из точки касания с опорой
	void SetLegRay(FLimb& L, FVector3f AbsRay);
	
	//позиция точки проекции ноги на твердь
	FVector GetFootVirtualLoc(FLimb& L);

	//идеальная длина ног с учетом подгиба
	float GetTargetLegLength() const { return TargetFeetLength * (1.0f - 0.5 * Crouch); }

//методы
public:

	//конструктор
	UMyrGirdle(const FObjectInitializer& ObjectInitializer);

	//при запуске игры
	virtual void BeginPlay() override;

	//пересчитать реальную длину ног
	float GetFeetLengthSq(bool Right = true);
	float GetFeetLengthSq(ELimb Lmb);

	//пересчитать базис длины ног через расстояние от сокета-кончика для кости-плечевогосустава
	//вызывается в конце BeginPlay, когда поза инициализована
	//однако выяснилось, что из начальной позы может быть неверно извлекать длину ноги
	//конечность может быть вылеплена полусогнутой чтобы форма была равноудалена от пределов сгиба-разгиба
	//можно пытаться считать сумму расстояний плечо-локоть, локоть-запястье... 
	//однако для этого нужно хранить правильные имена костей, они для всех разные и кол-во костей может быть разным
	//в общем пока элементарная задача вновь оказывается неформализуемой, поэтому позволить пользователю в редакторе
	//задать правильное число - и домножить на масштаб всего компонента, который может рандомно меняться
	//если же 0, значит пользователь не захотел вводить число и доверяет несовершенному алгоритму
	void UpdateLegLength();

	//включить или отключить проворот спинной части относительно ног (мах ногами вперед-назад)
	void SetSpineLock(bool Set);

	//влить динамическую модель 
	void AdoptDynModel(FGirdleDynModels& Models);

	//прощупать поверхность в поисках опоры
	bool Trace(FVector Start, FVector3f Dst, FHitResult& Hit);

	//прощупать поверхность всем поясом и совершить акт движения
	bool SenseForAFloor(float DeltaTime);

	//прощупать опору одной ногой для ее правильной графической постановки
	void SenseFootAtStep(float DeltaTime, FLimb& Foot, FVector CentralImpact, float FallSeverity, int ATurnToCompare);

	//переместить в нужное место (центральный членик)
	void ForceMoveToBody();

	//физически подпрыгнуть этим поясом конечностей
	void PhyPrance(FVector3f HorDir, float HorVel, float UpVel);

	//выдать все уроны частей тела в одной связки - для блюпринта обновления худа
	UFUNCTION(BlueprintCallable) FLinearColor GetDamage() const;

};