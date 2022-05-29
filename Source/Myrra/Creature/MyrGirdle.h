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

	//пояс держится фиксированным к опоре
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 FixedOnFloor:1;

//свойства
public:

	//счётчик кадров для размазывания сложных задач
	uint8 FrameCounter = 0;

	// кэш скорости ОТНОСИТЕЛЬНО опоры ног, принадлежащих этому поясу
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector VelocityAgainstFloor;

	//вектор предпочтительного направления движения с учётом ограничений типа хождения по веткам
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector GuidedMoveDir;

	//вектор "вперед" нужен отдельный, поскольку спина может встать вверх, а ноги подкоситься вперед-назад
	//возможно, не нужен
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector Forward;

	//степень пригнутости обеих ног (кэш с гистерезисом)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Crouch = 0.0f;

	//кривизна опоры, считается по нормалям двух ног
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Curvature = 0.0f;

	//длина ноги, для расчётов подогнутия ног на рельефе
	//устанавливается вычислением, может быть разной из-за масштаба меша
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float LimbLength = 0.0f;

	//дайджест уровня крепкости стояния на опоре, 0 - совсем в воздухе, 1 = обеими ногами
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float StandHardness = 0.0f;

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
	FLimb&			GetLimb (EGirdleRay R);
	FBodyInstance*	GetBody (EGirdleRay R);

	//констрайнт, которым спинная часть пояса подсоединяется к узловой (из-за единой иерархии в разных поясах это с разной стороны)
	FConstraintInstance* GetGirdleSpineConstraint();
	FConstraintInstance* GetGirdleArchSpineConstraint();


//методы
public:

	//конструктор
	UMyrGirdle(const FObjectInitializer& ObjectInitializer);

	//при запуске игры
	virtual void BeginPlay() override;

	//возможно ли этим поясом немедленно прицепиться к опоре (если
	EClung CanGirdleCling();

	//пересчитать реальную длину ног
	float GetFeetLengthSq(bool Right = true);

	//пересчитать базис длины ног через расстояние от сокета-кончика для кости-плечевогосустава
	//вызывается в конце BeginPlay, когда поза инициализована
	//однако выяснилось, что из начальной позы может быть неверно извлекать длину ноги
	//конечность может быть вылеплена полусогнутой чтобы форма была равноудалена от пределов сгиба-разгиба
	//можно пытаться считать сумму расстояний плечо-локоть, локоть-запястье... 
	//однако для этого нужно хранить правильные имена костей, они для всех разные и кол-во костей может быть разным
	//в общем пока элементарная задача вновь оказывается неформализуемой, поэтому позволить пользователю в редакторе
	//задать правильное число - и домножить на масштаб всего компонента, который может рандомно меняться
	//если же 0, значит пользователь не захотел вводить число и доверяет несовершенному алгоритму
	void UpdateLegLength() { LimbLength = (LimbLength == 0) ? FMath::Sqrt(GetFeetLengthSq()) : LimbLength * GetComponentScale().Z; }
	

	//вовне, главным образом в анимацию AnimInst - радиус колеса представлющего конечность
	float GetLegRadius();

	//включить или отключить проворот спинной части относительно ног (мах ногами вперед-назад)
	void SetSpineLock(bool Set);

	//влить динамическую модель 
	void AdoptDynModel(FGirdleDynModels& Models);

	//непосредственно кинематически сдвинуть в нужное место
	void KineMove(FVector Location, FVector CentralNormal, float DeltaTime);

	//явным образом получить для ноги опору через трассировку, вне зависимости от столкновений
	bool ExplicitTraceFoot(FLimb& Foot, float HowDeep);
	bool ExplicitTraceFeet(float HowDeep) { if (HasLegs) return ExplicitTraceFoot(GetLimb(EGirdleRay::Right),HowDeep) || ExplicitTraceFoot(GetLimb(EGirdleRay::Left),HowDeep); else return false; }

	//обработать одну конкретную конечность пояса
	float ProcedeFoot(FLimb& Foot, FLimb& OppFoot, float FootDamping, float& Asideness, float &WeightAccum, float DeltaTime);

	//покадрово сдвигать и вращать
	void Procede(float DeltaTime);

	//физически подпрыгнуть этим поясом конечностей
	void PhyPrance(FVector HorDir, float HorVel, float UpVel);

	//скаляр скорости в направлении движения
	float SpeedAlongFront() { return (VelocityAgainstFloor | Forward);  }

	//выдать все уроны частей тела в одной связки - для блюпринта обновления худа
	UFUNCTION(BlueprintCallable) FLinearColor GetDamage() const;

};