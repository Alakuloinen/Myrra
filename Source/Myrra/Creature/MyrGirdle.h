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

//общие флаги
public:

	//элементарная адресация передний/задний
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 IsThorax : 1;

	//пояс ползет по вертикальной опоре, обязательно наличие FixedOnFloor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 Climbing : 1;

//свойства
public:

	//полный перечень ячеек отростков для всех двух поясов конечностей
	static ELimb GirdleRays[2][(int)EGirdleRay::MAXRAYS];

	//кэш скорости ОТНОСИТЕЛЬНО опоры ног, принадлежащих этому поясу
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector3f VelocityAgainstFloor;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Speed;

	//вектор предпочтительного направления движения с учётом ограничений типа хождения по веткам
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector3f GuidedMoveDir;

	//степень пригнутости обеих ног (кэш переменной из динамодели + гистерезис)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Crouch = 0.0f;

	//нормальная длина ноги, для расчётов подгиба ног на рельефе
	//вычисляется в начале как разность позиций костей, из-за масштаба меша разная у разных объектов класса
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float TargetFeetLength = 0.0f;

	//плечи конечностей в координатах центрального туловища
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector3f RelFootRayLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector3f RelFootRayRiht;

	//указатель на сборку настроек динамики/ориентации для этого пояса
	//прилетает с разных ветвей иерархии - из состояния BehaveState, далее из настроек (само)действия 
	FGirdleDynModels* CurrentDynModel = nullptr;

	//дайджест уровня крепкости стояния на опоре, 0 - совсем в воздухе, 255 = обеими ногами и еще брюхом 
	//набирается по касаниям разных частей тела, нужно для оценки устойчивости
	//или не нужно?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 StandHardness = 0;

//флаги ограничений
public:

	//пояс держится фиксированным к опоре через констрейнт
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 FixedOnFloor : 1;

	//включить силовые моторы, которые сдерживают отклонения по сторонам (Yaw) и опрокидывание (RollPitch)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 TightenYaw : 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 TightenLean : 1;

	//не даёт крениться на бок (Roll), клониться взад-вперед (Pitch)  рыскать по сторонам (Yaw)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 LockRoll : 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 LockPitch : 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 LockYaw : 1;

	//если этот пояс (его центр) ограничен констрейнтом так, что поддерживается вертикальным
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 Vertical : 1;

	//заставить ноги быть полностью перепендикулярными ближайшей части спины
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 LockShoulders : 1;


//возвращуны
public:

	//выдать все биты ограничений якоря скопом (ОСТОРОЖНО! попытка обхитрить память!)
	uint8& AllFlags() { return ((uint8*)&StandHardness+1)[0]; }

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
	FBodyInstance*  GetBody (EGirdleRay R) const;

	FLimb& GetCenter() { return GetLimb(EGirdleRay::Center); }
	FLimb& GetSpine() { return GetLimb(EGirdleRay::Spine); }
	FLimb& GetLeft() { return GetLimb(EGirdleRay::Left); }
	FLimb& GetRight() { return GetLimb(EGirdleRay::Right); }

	//привязь, используемая для якоря и фиксации на поверхности
	FConstraintInstance* GetCI() const { return GetBody(EGirdleRay::Center)->DOFConstraint;	}

	//актуальные режимы закрепленности якорем
	bool GetFixOnFloor() const { return GetCI()->ProfileInstance.LinearDrive.XDrive.bEnablePositionDrive; }
	bool GetTightenLean() const { return GetCI()->ProfileInstance.AngularDrive.SwingDrive.bEnablePositionDrive; }
	bool GetTightenYaw() const { return GetCI()->ProfileInstance.AngularDrive.TwistDrive.bEnablePositionDrive; }

	//констрайнт, которым спинная часть пояса подсоединяется к узловой (из-за единой иерархии в разных поясах это с разной стороны)
	FConstraintInstance* GetGirdleSpineConstraint();
	FConstraintInstance* GetGirdleArchSpineConstraint();

	//ведущий пояс
	bool Lead() const { return CurrentDynModel->Leading; }

	//пояс не стоит ногами на опоре
	bool IsInAir() const { return (!GetLimb(EGirdleRay::Center).Floor.IsValid()); }
	bool Stands(int Thr = 200) { return (StandHardness >= Thr); }
	bool Lies(int Mn = 1, int Mx = 199) { return (StandHardness >= Mn && StandHardness <= Mx); }

	//режим попячки назад
	bool IsMovingBack() const;

	//скаляр скорости в направлении движения
	float SpeedAlongFront() { return (VelocityAgainstFloor | GuidedMoveDir); }

	//множитель для различения левого и правого
	float mRight(FLimb& Foot) const { return Foot.IsLeft() ? -1 : 1; }

	//позиция откуда нога растёт, просто транслирует функцию из Mesh
	FVector GetFootRootLoc(ELimb eL);

	//вычислить ориентировочное абсолютное положение конца ноги по геометрии скелета
	FVector GetFootTipLoc(ELimb eL);

	//локальный вектор линии ноги, как есть
	FVector3f& GetLegLineLocal(FLimb& L) { return L.IsLeft() ? RelFootRayRiht : RelFootRayLeft; }

	//распаковать вектор на абсолютные координаты
	FVector3f UnpackLegLine(FVector3f ExtLocLine);
	FVector3f UnpackLegLine(FLimb& L) { return UnpackLegLine(GetLegLineLocal(L)); }

	//загрузить новый радиус вектор ноги из точки касания с опорой
	FVector3f PackLegLine(FVector3f AbsRay);
	void PackLegLine(FLimb& L, FVector3f AbsRay) { GetLegLineLocal(L) = PackLegLine(AbsRay);	}
	
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
	bool AdoptDynModel(FGirdleDynModels& Models);

	//прощупать поверхность в поисках опоры
	bool Trace(FVector Start, FVector3f Dst, FHitResult& Hit);

	//прощупать поверхность всем поясом и совершить акт движения
	bool SenseForAFloor(float DeltaTime);

	//прощупать трасировкой позицию ступни, если применимо
	bool ProbeFloorByFoot(float DeltaTime, FLimb& Foot, int ProperTurn, FFloor& MainFloor, FVector &AltHitPoint);

	//немедленно стереть всю связь с поверхностью
	void DetachFromFloor()
	{	GetLimb(EGirdleRay::Center).EraseFloor();
		GetLimb(EGirdleRay::Right).EraseFloor();
		GetLimb(EGirdleRay::Left).EraseFloor();
		Climbing = false;
		AllFlags() = CurrentDynModel->AllFlagsAir();
	}

	//переместить в нужное место (центральный членик)
	void ForceMoveToBody();

	//полностью разорвать привязь к якорю, для мертвых
	void DetachFromBody();

	//физически подпрыгнуть этим поясом конечностей
	void PhyPrance(FVector3f HorDir, float HorVel, float UpVel);

	//выдать все уроны частей тела в одной связки - для блюпринта обновления худа
	UFUNCTION(BlueprintCallable) FLinearColor GetDamage() const;

};