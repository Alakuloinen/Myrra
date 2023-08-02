// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "Components/CapsuleComponent.h"
#include "MyrGirdle.generated.h"

USTRUCT(BlueprintType) struct FStep
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Phase = 0;		// фаза (-1..0..1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector3f BackToOldPos;	// отсчёт направления от предрасчитанного нового до свершенного старого
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector NewFootPos;		// положение следующего следа
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FPsyLoc Normal;			// нормаль в точке следующего следа

	float StepCurve() const { return Bell(FMath::Abs(Phase)); }

	//однозначное определение позиции кончика ноги между следами (с
	FVector FootTip() const { return NewFootPos + BackToOldPos * (1 - Phase); }

	bool StepsOn() const { return Phase != 0.0f; }
	bool OnGround() const { return Phase < 0.0f; }
	bool OffGround() const { return Phase > 0.0f; }

	void DisableSteps() { Phase = 0.0f; Normal.SetInvalid(); }
	void RegNextStep(FHitResult Hit) { NewFootPos = Hit.ImpactPoint; Normal.EncodeDir(Hit.ImpactNormal);  }
	void Land() { Phase = -0.001;  }
	void Soar() { Phase = 0.001; }

	void ShiftPhaseInAir(float Delta) { Phase = FMath::Min(Phase + Delta, 1.0f); }
	void ShiftPhaseOnGnd(float Delta) { Phase = FMath::Max(Phase - Delta, -1.0f); }
};
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

	//возможно, чисто для отладки - режим быстрого забирания по стене без карабканья
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 Ascending : 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 Descending : 1;

	//пояс в режиме точного позиционирования шагов на опоре
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 FairSteps : 1;

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

	//дайджест уровня крепкости стояния на опоре, 0 - совсем в воздухе, 255 = обеими ногами и еще брюхом 
	//набирается по касаниям разных частей тела, нужно для оценки устойчивости
	//или не нужно?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 StandHardness = 0;

	//флаги ограничений
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGirdleFlags Flags = 0;

//механизм шагов
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FStep RStep;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FStep LStep;

	//плечи конечностей в координатах центрального туловища
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLegPos RelFootRayLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLegPos RelFootRayRiht;

//возвращуны
public:

	//актор-существо в правильном типе
	class AMyrPhyCreature* MyrOwner() { return (AMyrPhyCreature*)GetOwner(); }
	class AMyrPhyCreature* MyrOwner() const { return (AMyrPhyCreature*)GetOwner(); }

	//меш этого существа
	class UMyrPhyCreatureMesh* me();
	class UMyrPhyCreatureMesh* mec() const;

	//динамическая модель для этого пояса (часто общей дин-модели, которая указателем сохраняется в меше)
	FGirdleDynModels* DynModel();
	FGirdleDynModels* DynModel() const { return const_cast<UMyrGirdle*>(this)->DynModel(); }

	//доступен механизм шагов
	bool HasSteps() const;

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

	//объект на котором этот пояс стоит ногами
	USceneComponent* GetFloor() { return GetCenter().Floor.Component(); }

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
	bool Lead() const { return DynModel()->Leading; }

	//пояс не стоит ногами на опоре
	bool IsInAir() const { return (!GetLimb(EGirdleRay::Center).Floor.IsValid()); }
	bool CanStep() const				{ auto& F = GetLimb(EGirdleRay::Center).Floor; return F ? F.TooSteep() : false; }
	bool CanMount(FVector3f& Dir) const { auto& F = GetLimb(EGirdleRay::Center).Floor; return F ? (F.TooSteep() && F.CharCanStep() && (F.Normal|Dir)<-0.5) : false;	}
	bool Stands(int Thr = 200) { return (StandHardness >= Thr); }
	bool Lies(int Mn = 1, int Mx = 199) { return (StandHardness >= Mn && StandHardness <= Mx); }
	bool Projected() const { return IsInAir() && Flags.FixOn; }

	//режим попячки назад
	bool IsMovingBack() const;

	//скаляр скорости в направлении движения
	float SpeedAlongFront() { return (VelocityAgainstFloor | GuidedMoveDir); }
	FVector3f SpeedDir() const { return VelocityAgainstFloor/Speed; }

	//множитель для различения левого и правого
	float mRight(FLimb& Foot) const { return Foot.IsLeft() ? -1 : 1; }

	//позиция откуда нога растёт, просто транслирует функцию из Mesh
	FVector GetFootRootLoc(ELimb eL);

	//вычислить ориентировочное абсолютное положение конца ноги по геометрии скелета
	FVector GetFootTipLoc(ELimb eL);

	//локальный вектор линии ноги, как есть
	constexpr FLegPos& GetLegLine(FLimb& L) { return L.IsLeft() ? RelFootRayLeft : RelFootRayRiht; }

	//сборка для шагов
	FStep& GetStep(FLimb& L) { return L.IsLeft() ? LStep : RStep; }

	//распаковать вектор на абсолютные координаты
	FVector3f UnpackLegLine(FLegPos ExtLocLine);
	FVector3f UnpackLegLine(FLimb& L) { return UnpackLegLine(GetLegLine(L)); }

	//загрузить новую линию ноги из абсолютноых координат в локальные спинные
	FLegPos PackLegLine(FVector3f AbsRay, float Lng = 0);
	void PackLegLine(FLimb& L, FVector3f AbsRay, float Lng = 0) {	GetLegLine(L) = PackLegLine(AbsRay, Lng); }
	void PackLegLine(FLimb& L, FVector AbsRay) { GetLegLine(L) = PackLegLine((FVector3f)AbsRay, 0); }

	//когда уже неудобно держать ногу на земле (тело сместилось) и надо отнять ногу от земли
	bool FootDetachCriteria(FLimb& L);


	//идеальное расположение ноги для текущей позы тела, для данного пояса
	FLegPos idealLegLine(FLimb& L);

	//идеальное расположение ноги для текущей позы тела, для данного пояса
	FVector3f IdealAbsLegLine(FLimb& Foot, float Length);

	//направление из плеча на место предполагаемого шага
	FVector3f VecToWannabeStep(FLimb& Foot, float Leng) {	return (IdealAbsLegLine(Foot, TargetFeetLength) + GuidedMoveDir * Leng)*1.7;	}

	//эффективная раскоряка ноги, 0 - полный назад, 1 - полный вперед
	float LegStretch(FLimb& Foot);

	//идеальная длина ног с учетом подгиба
	float GetTargetLegLength() const { return TargetFeetLength * (1.0f - 0.5 * Crouch);  }

	//максимальная длина шага исходя из длины ноги и высоты
	float MaxStepLength() const { return FMath::Sqrt(FMath::Square(TargetFeetLength*1.1) - FMath::Square(GetTargetLegLength()/*GetLegLine(Foot).DownUp*/)); }
	/*TargetFeetLength * (0.35 + FMath::Min(0.3, V*0.1) + 0.3*0.01*V + 0.2*DynModel()->Crouch);*/

	//идеальный размер шага вперед (зависит от скорости и приседания, пока сложно сказать, как лучше)
	float IdealStepSizeX(float V) const { return MaxStepLength()*(0.3 + FMath::Min(0.7, V * 0.01)); }

	//позиция точки проекции ноги на твердь
	FVector GetFootVirtualLoc(FLimb& L);


	//в незадействованное для кинематических компонентов поле ComponentVelocity перед прыжком вкладывается позиция якоря до прыжка
	FVector& CachedPreJumpPos() { return ComponentVelocity; }

//методы, самое главное
public:

	//------------------------------------------------------------------------------------
	//прощупать поверхность всем поясом и совершить акт движения
	bool SenseForAFloor(float DeltaTime);

	//прощупать трасировкой позицию ступни, если применимо
	bool ProbeFloorByFoot(float DeltaTime, FLimb& Foot, int ProperTurn, FFloor& MainFloor, FVector& AltHitPoint);
	//-------------------------------------------------------------------------------------

	//шаги, пока не работает, строится
	void ProcessSteps(FLimb& Foot, float DeltaTime, int ProperTurn);

	//получить точную позицию шага или текущего, или следующего
	bool TraceStep(FLimb &Foot, char LandNotSoar, FVector3f Dir);
	bool TraceStep(FLimb& Foot, char LandNotSoar, float StepLength) { return TraceStep(Foot, LandNotSoar, VecToWannabeStep(Foot, StepLength)); }

	//начать двигаться из бесшагового состояния или из неподвижности 
	bool StartFoot(FLimb& Foot);

//методы, формальное
public:

	//конструктор
	UMyrGirdle(const FObjectInitializer& ObjectInitializer);

	//при запуске игры
	virtual void BeginPlay() override;

//методы, прочее
public:

	//пересчитать реальную длину ног
	float GetFeetLengthSq(bool Right = true);
	float GetFeetLengthSq(ELimb Lmb);

	//выдрать длину ноги из скелета
	float GetRefLegLength();

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

	//установить силу, с которой махи вперед-назад двумя лапами будут сдерживаться
	void SetJunctionStiffness(float Stiffness);

	//обнаружить призмеление и применить боль если нужно
	FFloor LandingHurt(FLimb& Limb, FHitResult Hit);

	//влить динамическую модель 
	bool AdoptDynModel(FGirdleDynModels& Models);

	//прощупать поверхность в поисках опоры
	bool Trace(FVector Start, FVector3f Dst, FHitResult& Hit);

	//немедленно стереть всю связь с поверхностью
	void DetachFromFloor()
	{	GetLimb(EGirdleRay::Center).EraseFloor();
		GetLimb(EGirdleRay::Right).EraseFloor();
		GetLimb(EGirdleRay::Left).EraseFloor();
		Climbing = false;
		Flags = DynModel()->InAir;
	}

	//переместить в нужное место (центральный членик)
	void ForceMoveToBody();

	//полностью разорвать привязь к якорю, для мертвых
	void DetachFromBody();

	//физически подпрыгнуть этим поясом конечностей
	void PhyPrance(FVector3f HorDir, float HorVel, float UpVel);
	void PhyPranceDirect(FVector3f Vel);

	//в незадействованное для кинематических компонентов поле ComponentVelocity перед прыжком вкладывается позиция якоря до прыжка
	void SetCachedPreJumpPos() { ComponentVelocity = GetComponentLocation(); }

	//отцензурить поступающую извне динюмодель
	int32 CensorDynModel(int32 In)
	{
		//исключить линейную тягу, если членик привязан к якорю
		if (DynModel()->InAir.FixOn || DynModel()->OnGnd.FixOn)
			return In & (~LDY_PULL);
		return In;
	}

	//выдать все уроны частей тела в одной связки - для блюпринта обновления худа
	UFUNCTION(BlueprintCallable) FLinearColor GetDamage() const;

};