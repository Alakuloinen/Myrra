// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "Components/SkeletalMeshComponent.h"
#include "MyrPhyCreatureMesh.generated.h"

//номер членика в скелетной опоре, когда она вовсе не скелетная
//#define NO_BODY 255; //-127

//для менее многословноых настроек констрейнтов
enum { TWIST, SWING1, SWING2 };

//отмечать результат попытки зацепиться
UENUM() enum class EClung : uint8
{ 	Recreated,	Updated,	Kept,
	CantCling,	NoSeekToCling,	NoLeading,	BadSurface, BadAngle, OneHandNotEnough, DangerousSpread
};
inline bool operator>(EClung A, EClung B) { return (int)A > (int)B; }
inline bool operator<(EClung A, EClung B) { return (int)A < (int)B; }
inline bool operator==(EClung A, EClung B) { return (int)A == (int)B; }
inline bool operator>=(EClung A, EClung B) { return (int)A >= (int)B; }
inline bool operator<=(EClung A, EClung B) { return (int)A <= (int)B; }



//###################################################################################################################
// попытка создать новое управляемое тело для существа, на этот раз полностью на физдвижке
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYRRA_API UMyrPhyCreatureMesh : public USkeletalMeshComponent
{
	GENERATED_BODY()

//свойства
public:

	//части тела идут в таком порядка, в  котором они перечислены в  ELimb
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FLimb Pelvis;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FLimb Lumbus;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FLimb Pectus;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FLimb Thorax;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FLimb Head;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FLimb LArm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FLimb RArm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FLimb LLeg;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FLimb RLeg;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FLimb Tail;

	//пояса конечностей - здесь регистрируется скорость относительно пола - последовательность от зада к переду
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGirdle gPelvis;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGirdle gThorax;

	//счётчик кадров для размазывания сложных задач
	uint32 FrameCounter = 0;

	//скорость излечения полученного повреждения, 1 означает, что полное исцеление за секунду
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HealRate = 0.1;

	//для централизованного вычисления физики опоры
	FCalculateCustomPhysics OnCalculateCustomPhysics;

	//общая ориентация тела (надо как-то вичитать по ногам плюс плавно блендить по направлению движения
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector WholeDirection;

	//костыль для настройки правильной силы толкания
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AdvMoveFactor = 1.0f;

	//доп выключатель урона при касании, чтоб можно было кинематический свип делать
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool InjureAtHit = true;

	//эксперимент с телепртом - устанавливается извне, после телепорта тут же сбрасывается
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool WaitingForTeleport = false;

	//кэш последней настроенной (из BehaveState или SelfAction) модели сил для всех поясов
	FWholeBodyDynamicsModel* DynModel;

#if WITH_EDITOR
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbAxes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbForces;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugGirdleGuideDir;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugGirdleStepped;
	FColor DebugLineChannel(ELimbDebug Ch) { return ((FColor*)(&DebugLimbAxes))[(int)Ch];  }
#endif

//статические - только для этого типа скелета
private:

	//имя кривых в скелете, ответственных за шанс встретить шаг в текущий момент времени (задается в анимации шагов)
	static FName StepCurves[(int)ELimb::NOLIMB];

	//к какому поясу конечностей принадлежит каждая из доступных частей тела - для N(0) поиска
	static uint8 Section[(int)ELimb::NOLIMB];

	//какой родитель у каждой части тела (в рамках этого класса фиксирован)
	static ELimb Parent[(int)ELimb::NOLIMB];

	//номер такого же, но с другой стороны, для лап это лево-право, для спины \то перед-назад
	static ELimb DirectOpposite[(int)ELimb::NOLIMB];

	//перевод из номера члена в его номер как луча у пояса конечностей
	static EGirdleRay Ray[(int)ELimb::NOLIMB];

	//полный перечень ячеек отростков для всех двух поясов конечностей
	static ELimb GirdleRays[2][(int)EGirdleRay::MAXRAYS];


//стандартные методы и переопределения
public: 

	//конструктор
	UMyrPhyCreatureMesh(const FObjectInitializer& ObjectInitializer);

	//при запуске игры
	virtual void BeginPlay() override;

	//каждый кадр
	virtual void TickComponent(float DeltaTime,	enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

//-----------------------------------------------------
protected: //делегаты - отклики на особые события
//-----------------------------------------------------

	//удар твердлого тела
	UFUNCTION()	void OnHit (UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	//отследивание конца пересечения между добычей в зубах и хватателем
	UFUNCTION()	void OnOverlapEnd(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	//приложение сил
	void CustomPhysics(float DeltaTme, FBodyInstance* BodyInstance);

//свои методы - возвращуны
public: 

	//получить ось из трансформации, в том числе отрицательную
	static FVector GetAxis(const FTransform& T, const EMyrAxis Axis) { return T.TransformVectorNoScale(MyrAxes[(int)Axis]); }

	//текущее положение плеча
	FVector GetLimbShoulderHubPosition(ELimb eL);

	//позиция головы в мировых координатах - для подвязки камеры и ИИ
	FVector GetHeadLocation() {	return GetSocketLocation(TEXT("HeadLook")); }

	//актор-существо в правильном типе
	class AMyrPhyCreature* MyrOwner() { return (AMyrPhyCreature*)GetOwner(); }
	class AMyrPhyCreature* MyrOwner() const { return (AMyrPhyCreature*)GetOwner(); }

	//доступ к данным из генофонда по опоре и по мясу для конкретной части тела
	FMachineLimbAnatomy* MachineGene(ELimb Limb) const;
	FFleshLimbAnatomy* FleshGene(ELimb Limb) const;
	FLimbOrient& MBoneAxes() const;

	//физ-членик, принадлежащий корневой кости, планируется использовать его как большой кинематический отбойник
	FBodyInstance* GetRootBody();

	//взять часть тела по индексу (трюк с последовательным расположением в памяти)
	FLimb& GetLimb(ELimb li)					{ return ((FLimb*)(&Pelvis))[(int)(li)]; }
	FLimb& GetLimbParent(ELimb li)				{ return ((FLimb*)(&Pelvis))[(int) Parent[(int)li] ]; }
	FLimb& GetLimbParent(FLimb Limb)			{ return GetLimbParent(Limb.WhatAmI); }
	FLimb& GetOppositeLimb(const FLimb& Limb)	{ return GetLimb((DirectOpposite[(int)Limb.WhatAmI])); }


	//механизм-поддержка для части тела
	int				GetMachineBodyIndex (ELimb Limb)  const			{ return MachineGene(Limb)->TipBodyIndex; }
	int				GetMachineBodyIndex (const FLimb& Limb) const	{ return GetMachineBodyIndex (Limb.WhatAmI); }
	FBodyInstance*	GetMachineBody		(ELimb li)					{ return Bodies [GetMachineBodyIndex(li)]; } 
	FBodyInstance*	GetMachineBody		(const FLimb& Limb)			{ return Bodies [GetMachineBodyIndex(Limb)]; }
	bool			HasPhy (ELimb Limb)								{ return (GetMachineBodyIndex (Limb)!=255); }

	//профиль тела поддержки из предопределенного в редакторе
	FBodyInstance*	GetArchMachineBody	(ELimb Limb)  const;
	FBodyInstance*	GetArchMachineBody	(FLimb& Limb) const { return GetArchMachineBody (Limb.WhatAmI); }


	//привязь механизма-поддержки части тела
	int						GetMachineConstraintIndex(ELimb Limb) const		{ return MachineGene(Limb)->TipConstraintIndex; }
	int						GetMachineConstraintIndex	(FLimb& Limb) const { return GetMachineConstraintIndex (Limb.WhatAmI); }
	FConstraintInstance*	GetMachineConstraint		(ELimb Limb)		{ return Constraints[ GetMachineConstraintIndex (Limb) ]; }
	FConstraintInstance*	GetMachineConstraint		(FLimb& Limb)		{ return Constraints[ GetMachineConstraintIndex (Limb) ]; }

	//значения для тела и констрейна, предопределенные в редакторе
	FConstraintInstance*	GetArchConstraint			(int i) const;
	FConstraintInstance*	GetArchMachineConstraint	(ELimb Limb) const;
	FConstraintInstance*	GetArchMachineConstraint	(FLimb& Limb) const	{ return GetArchMachineConstraint (Limb.WhatAmI); }


	//членики реальной графической репрезентации части тела
	bool			HasFlesh			(ELimb Limb) const						{ return (bool)FleshGene(Limb)->BodyIndex.Num(); }
	int				GetFleshBodyIndex	(ELimb Limb, int DistFromLeaf=0) const  { return FleshGene(Limb)->BodyIndex[DistFromLeaf]; }
	int				GetFleshBodyIndex	(FLimb& Limb, int DistFromLeaf=0) const { return GetFleshBodyIndex (Limb.WhatAmI,DistFromLeaf); }
	FBodyInstance*	GetFleshBody		(ELimb Limb, int DistFromLeaf=0)		{ return Bodies[GetFleshBodyIndex (Limb,DistFromLeaf)]; }
	FBodyInstance*	GetFleshBody		(FLimb& Limb, int DistFromLeaf=0)		{ return Bodies[GetFleshBodyIndex (Limb,DistFromLeaf)]; }
	
	//к какому поясу конечностей принадлежит часть тела под заданным индексом
	FGirdle& GetGirdle (ELimb Limb) { return Section[(int)Limb] ? gThorax : gPelvis; }
	FGirdle& GetGirdle (const FLimb &Limb) { return Section[(int)Limb.WhatAmI] ? gThorax : gPelvis; }
	FGirdle& GetAntiGirdle(const FGirdle* G) { return G == &gThorax ? gPelvis : gThorax; }

	//стандартные ячейки пояса конечностей
	ELimb  GetGirdleRay (const FGirdle& G, const EGirdleRay R) { return GirdleRays[&G - &gPelvis][(int)R]; }
	FLimb& GetGirdleLimb(const FGirdle& G, const EGirdleRay R) { return GetLimb(GetGirdleRay(G, R)); }
	FBodyInstance* GetGirdleBody(const FGirdle& G, const EGirdleRay R) { return GetMachineBody(GetGirdleRay(G, R)); }

	//констрайнт, которым спинная часть пояса подсоединяется к узловой (из-за единой иерархии в разных поясах это с разной стороны)
	FConstraintInstance* GetGirdleSpineConstraint(const FGirdle& G) { return (&G == &gThorax) ? GetMachineConstraint(Thorax) : GetMachineConstraint(Lumbus); }
	FConstraintInstance* GetGirdleArchSpineConstraint(const FGirdle& G) { return (&G == &gThorax) ? GetArchMachineConstraint(Thorax) : GetArchMachineConstraint(Lumbus); }

	//целиковые трансформации тел (на самом деле нижележащих костей)
	FTransform GetLimbTrans(ELimb Limb)  const { return const_cast<UMyrPhyCreatureMesh*>(this)->GetMachineBody(Limb)->GetUnrealWorldTransform(); }
	FTransform GetLimbTrans(FLimb& Limb)  const { return const_cast<UMyrPhyCreatureMesh*>(this)->GetMachineBody(Limb)->GetUnrealWorldTransform(); }

	//универсальный способ указания оси со знаком (возможно, углубить до матрицы)
	FVector GetLimbAxis(ELimb Limb, const EMyrAxis Axis) const { return GetAxis (GetLimbTrans(Limb), Axis); }

	//универсальный геттер оси членика по ее мнемоническому названию (лево, вверх...)
	FVector GetLimbAxis(ELimb Limb, const EMyrRelAxis Axis) const { return GetAxis (GetLimbTrans(Limb), MBoneAxes().ByEnum(Axis)); }

	// это попытка подстроиться под кости загруженного скелета, почему имено так, хз
	FVector GetLimbAxisUp(ELimb Limb)  const { return GetLimbAxis(Limb, MBoneAxes().Up); }
	FVector GetLimbAxisDown(ELimb Limb)  const { return GetLimbAxis(Limb, AntiAxis(MBoneAxes().Up)); }
	FVector GetLimbAxisRight(ELimb Limb) const { return GetLimbAxis(Limb, MBoneAxes().Right); }
	FVector GetLimbAxisLeft(ELimb Limb) const { return GetLimbAxis(Limb, AntiAxis(MBoneAxes().Right)); }
	FVector GetLimbAxisForth(ELimb Limb) const { return GetLimbAxis(Limb, MBoneAxes().Forth); }
	FVector GetLimbAxisBack(ELimb Limb) const { return GetLimbAxis(Limb, AntiAxis(MBoneAxes().Forth)); }

	FVector GetLimbAxisUp(FLimb& Limb)  const { return GetLimbAxisUp(Limb.WhatAmI); }
	FVector GetLimbAxisDown(FLimb& Limb)  const { return GetLimbAxisDown(Limb.WhatAmI); }
	FVector GetLimbAxisRight(FLimb& Limb) const { return GetLimbAxisRight(Limb.WhatAmI); }
	FVector GetLimbAxisLeft(FLimb& Limb) const { return GetLimbAxisLeft(Limb.WhatAmI); }
	FVector GetLimbAxisForth(FLimb& Limb) const { return GetLimbAxisForth(Limb.WhatAmI); }
	FVector GetLimbAxisBack(FLimb& Limb) const { return GetLimbAxisBack(Limb.WhatAmI); }

	//направление вперед всего тела корневая трасформация - это из корневой кости,
	//если корневая кость встаёт неправильно, для нее в PhAT делается тело и подвязывается жестким констрейнтом к более стабильным телам
	FVector GetWholeForth() const { return GetComponentTransform().GetScaledAxis(EAxis::Y); }


	//тела поясов конечностей
	FBodyInstance* GetPelvisBody() { return GetMachineBody(Pelvis); }
	FBodyInstance* GetThoraxBody() { return GetMachineBody(Thorax); }
	FBodyInstance* GetGirdleBody (FGirdle& Girdle) { return GetMachineBody(GetLimb(GetGirdleRay(Girdle, EGirdleRay::Center))); }

	//скаляр скорости в направлении движения
	float GetGirdleDirectVelocity(FGirdle& G) { return (G.VelocityAgainstFloor | G.Forward);  }

	//суммарный урон всех частей тела (возможно, ввести коэффициенты важности)
	UFUNCTION(BlueprintCallable) float GetWholeDamage() const { return Head.Damage + Lumbus.Damage + Thorax.Damage + Tail.Damage + RArm.Damage + LArm.Damage + RLeg.Damage + LLeg.Damage;  }

	//суммарная потеря двигательных функций всего тела (множитель скорости)
	UFUNCTION(BlueprintCallable) float GetWholeImmobility();

	//вязкость по отношению к подставке
	UFUNCTION(BlueprintCallable) float GetRootDamping() const { return const_cast<UMyrPhyCreatureMesh*>(this)->GetPelvisBody()->LinearDamping; }

	//возможно ли этим поясом немедленно прицепиться к опоре (если
	bool CanGirdleCling(const FGirdle& Girdle) const;

	//выдать правильное физическое тело из компонента в принятой местной нотации по индексам тел
	FBodyInstance* GetBody(UPrimitiveComponent* C, int8 CB) { return (CB==NO_BODY) ? C->GetBodyInstance() : ((USkeletalMeshComponent*)C)->Bodies[CB]; }

	//можно ли по этой поверхности лазать
	static bool IsClimbable(EMyrSurface surf) { return (surf == EMyrSurface::Surface_WoodRaw || surf == EMyrSurface::Surface_Fabric/* || surf == EMyrSurface::Surface_Flesh*/); }

	//лежим боком на земле
	bool IsLyingDown() const { return (Lumbus.Stepped && Lumbus.ImpactNormal.Z > 0.5) || (Pectus.Stepped && Pectus.ImpactNormal.Z > 0.5); }

	//степень прямохождения
	float Erection() const { return 0.5*(GetLimbAxisForth(ELimb::LUMBUS).Z + GetLimbAxisForth(ELimb::PECTUS).Z); }

	//порог опрокидываемости берется по задней-центральной части спины, так как "хабы" могут быть вывернуты из-за подкошенных ног
	bool IsSpineDown(float Thresh = 0.0f) const { return (GetLimbAxisUp(ELimb::LUMBUS).Z < Thresh); }

	//зацеп включен, но настроен так, чтобы удерживать при отсутствии тяги, а не помогать двигать 
	bool IsClingAtStop(FConstraintInstance* CI) {	return CI->GetLinearYMotion() == ELinearConstraintMotion::LCM_Locked;	}

	//функция перехода к вертикали, жесткая вертикаль еще не установлена 
	bool IsVerticalTransition(FConstraintInstance* CI) { return CI->ProfileInstance.AngularDrive.SwingDrive.bEnablePositionDrive; }

	//касается ли данная тушка данной частью тела данного актора
	bool Tango(FLimb& Limb, AActor* A) { if (Limb.Floor) if (Limb.Floor->OwnerComponent->GetOwner() == A) return true; return false; }
	bool Tango(FLimb& Limb, UPrimitiveComponent* A) { if (Limb.Floor) if (Limb.Floor->OwnerComponent.Get() == A) return true; return false; }
	bool IsTouchingActor(AActor* A) { return Tango(Pelvis, A) || Tango(Lumbus, A) || Tango(Thorax, A) || Tango(Pectus, A) || Tango(RArm, A) || Tango(LArm, A) || Tango(LLeg, A) || Tango(RLeg, A) || Tango(Head, A) || Tango(Tail, A); }
	bool IsTouchingComponent(UPrimitiveComponent* A) { return Tango(Pelvis, A) || Tango(Lumbus, A) || Tango(Thorax, A) || Tango(Pectus, A) || Tango(RArm, A) || Tango(LArm, A) || Tango(LLeg, A) || Tango(RLeg, A) || Tango(Head, A) || Tango(Tail, A); }

	//детекция намеренного натыкания на стену
	float StuckToStepAmount(const FLimb& Limb, FVector Dir) const;

	//детекция намеренного натыкания на стену
	float StuckToStepAmount(FVector Dir, ELimb& LevelOfContact) const;
	
	//стоит ли тушка на этой опоре - берутся только нижние части тела в зависимости от того, есть ноги или нет
	bool IsStandingOnActor(AActor* A) 
	{ 	if(gPelvis.HasLegs) { if(Tango(LLeg, A)||Tango(RLeg, A)) return true; else if(Tango(Lumbus, A)||Tango(Pelvis, A)) return true; }
		if(gThorax.HasLegs) { if(Tango(LArm, A)||Tango(RArm, A)) return true; else if(Tango(Thorax, A)||Tango(Pectus, A)) return true; } return false;
	}

	//тело в режиме восприимчивости к столкновениям, то есть непрозрачно
	bool IsBumpable() { return GetCollisionObjectType() != ECollisionChannel::ECC_Vehicle; }


//свои методы - процедуры
public:

	//костылик для обработчика физики части тела - получить по битовому полю направление собственной оси, которое недо повернуть
	FVector CalcAxisForLimbDynModel(const FLimb Limb, FVector GloAxis) const;

	//двигаться на физдвижке по горизонтальной поверхности
	void Move (FBodyInstance* BodyInst, FLimb& Limb);

	//ориентировать тело силами, поворачивая или утягивая
	void OrientByForce (FBodyInstance* BodyInst, FLimb& Limb, FVector ActualDir, FVector DesiredDir, float AdvMult = 1.0);


	//совершить прыжок
	void PhyJump(FVector Horizontal, float HorVel, float UpVel);
	void PhyJumpLimb(FLimb& Limb, FVector Dir);

	//рвануть/встать на дыбы одной из секций тела
	void PhyPrance(FGirdle& G, FVector HorDir, float HorVel, float UpVel );

	//телепортировать всё тело на седло ветки
	void TeleportOntoBranch(FBodyInstance* CapsuleBranch);

	//подготовить тело к зацеплению (выщывается из существа, а в существо из нажатия)
	void ClingGetReady(bool Set);


	//рассчитать возможный урон от столкновения с другим телом
	float ApplyHitDamage(FLimb& Limb, uint8 DistFromLeaf, float TermialDamageMult, float SpeedSquared, const FHitResult& Hit);

	//рассчитать длины ног - для акнимации, но покая неясно, где вызывать
	void InitFeetLengths(FGirdle& G);

	//дополнительная инициализация части тела
	void InitLimb(FLimb& Limb, ELimb Me) {Limb.WhatAmI = Me;}
	void InitLimbCollision(FGirdle& G, EGirdleRay GR, float MaxAngularVeloc, FName CollisionPreset = NAME_None);

	//в ответ на касание членика этой части тела
	void HitLimb(FLimb& Limb, uint8 DistFromLeaf, const FHitResult &Hit, FVector NormalImpulse);

	//основная покадровая рутина в отношении части тела
	void ProcessLimb(FLimb& Limb, float DeltaTime, float* NewStandHardnesses);

	//установить туготу машинной части конечности (угловую можно не менять)
	void SetLimbDamping(FLimb& Limb, float Linear, float Angular = -1);

	//прожевать биты и перевести в линейную вязкость
	void SetLimbDampingFromModel(FLimb& Limb, float Base, uint32 BitField)
	{	if (!(BitField & LDY_DAMPING))	{	SetLimbDamping(Limb, 0, 0);	return;		}
		float DampMult = 0.25 * (bool)(BitField & LDY_DAMPING_4_1) + 0.25 * (bool)(BitField & LDY_DAMPING_4_2) + 0.5 * (BitField & LDY_DAMPING_2_1);
		SetLimbDamping(Limb, DampMult * Base);
	}

	//применить вес физики
	void PhyDeflect(FLimb& Limb, float Weight);

	//обновить веса физики и увечья для этой части тела
	void ProcessBodyWeights(FLimb& Limb, float DeltaTime);

	//вычислить предпочительное направление движения для этого членика по его опоре
	FVector CalculateLimbGuideDir(FLimb& Limb, const FVector& PrimaryCourse, bool SupportOnBranch, float* Stability = nullptr);

	//принять или отклонить только что обнаруженную новую опору для этого членика
	int ResolveNewFloor(FLimb &Limb, FBodyInstance* NewFloor, FVector NewNormal, FVector NewHitLoc);

	//прицепиться к опоре и сопроводить движение по опоре перезацепами
	EClung ClingLimb(FLimb& Limb, bool NewFloor);
	void UnClingLimb(FLimb& Limb);
	void UntouchLimb(FLimb& Limb) { if (Limb.Clung) UnClingLimb(Limb); Limb.Stepped = 0; Limb.Floor = nullptr; Limb.Surface = EMyrSurface::Surface_0; }

	//взять данный предмет за данный членик данной частью тела
	void Grab (FLimb& GrabberLimb, uint8 ExactBody, FBodyInstance* GrabbedBody, FLimb* GrabbedLimb, FVector ExactAt);
	void UnGrab (FLimb& GrabberLimb);
	void UnGrabAll ();


	//инициализировать пояс конечностей при уже загруженной остальной физике тела (ноги есть/нет, DOF Constraint)
	void InitGirdle (FGirdle& Girdle, bool StartVertical=true);

	//задать предпочтительную ориентацию пояса конечностей - при смене позы (видимо, нах, или судить функции)
	void AdoptDynModelForGirdle (FGirdle& Girdle, FGirdleDynModels& Models);
	void AdoptDynModelForLimb(FGirdle& Girdle, FLimb& Limb, uint32 Model, float DampingBase);

	//обновить "перёд" констрейнта центрального членика (ограничение на уклон от вертикали или на юление влево-вправо)
	void UpdateCentralConstraintFrames(FGirdle& G);

	//внести в констрейнт центрального членика новые данные по разрешению жесткой вертикали и юления по сторонам
	void UpdateCentralConstraintLimits(FGirdle &G);

	//основная покадровая рутина для пояса конечностей
	void ProcessGirdle (FGirdle& Girdle, FGirdle& OppositeGirdle, float DeltaTime, float DirToMiddle);
	void ProcessGirdleFoot(FGirdle& Girdle, FLimb& FootLimb, FLimb& OppositeLimb, float DampingForFeet, FVector PrimaryForth, float& WeightAccum, bool& NeedForVertical, float DeltaTime);

	//float CalculateGirdleVelocity();

	//вычислить торможение ног этого пояса через вязкость
	float CalculateGirdleFeetDamping(FGirdle &Girdle);

	//создать или просто включить вертикальное выравнивание пояса конечностей
	void SetGirdleVertical (FGirdle& G);
	void ClearGirdleVertical(FGirdle& G);

	//ограничить юление по сторонам
	void SetGirdleYawLock(FGirdle &G, bool Set);

	//включить или отключить проворот спинной части относительно ног (мах ногами вперед-назад)
	void SetGirdleSpineLock(FGirdle &G, bool Set);

	//расслабить все моторы в физических привязях
	void SetFullBodyLax(bool Set);

	//прописать в членики профили стокновений чтобы они были либо сталкивались с другими существами, либо нет
	void SetPhyBodiesBumpable(bool Set);


//фигня для отладки
public:

	//чтоб громоздкую строку не взвать
	void Log(FLimb& L, FString Addi)				{ UE_LOG(LogTemp, Log, TEXT("%s: Limb %s: %s"), *GetOwner()->GetName(), *TXTENUM(ELimb, L.WhatAmI), *Addi); }
	void Log(FLimb& L, FString Addi, float Param) { UE_LOG(LogTemp, Log, TEXT("%s: Limb %s: %s %g"), *GetOwner()->GetName(), *TXTENUM(ELimb, L.WhatAmI), *Addi, Param); }
	void Log(FLimb& L, FString Addi, float Param, float Param2) { UE_LOG(LogTemp, Log, TEXT("%s: Limb %s: %s %g %g"), *GetOwner()->GetName(), *TXTENUM(ELimb, L.WhatAmI), *Addi, Param, Param2); }
	void Log(FLimb& L, FString Addi, FString Param) { UE_LOG(LogTemp, Log, TEXT("%s: Limb %s: %s %s"), *GetOwner()->GetName(), *TXTENUM(ELimb, L.WhatAmI), *Addi, *Param); }
	void Log(FGirdle& G, FString Addi)				{ UE_LOG(LogTemp, Log, TEXT("%s: Girdle:%s: %s"), *GetOwner()->GetName(), (&G == &gThorax) ? TEXT("Thorax") : TEXT("Pelvis"), *Addi); }
	void Log(FGirdle& G, FString Addi, float Param) { UE_LOG(LogTemp, Log, TEXT("%s: Girdle:%s: %s %g"), *GetOwner()->GetName(), (&G == &gThorax) ? TEXT("Thorax") : TEXT("Pelvis"), *Addi, Param); }

	void DrawMove(FVector A, FVector AB, FColor C, float W = 1, float Time = 0.02);
	void DrawAxis(FVector A, FVector AB, FColor C, float W = 1, float Time = 0.02);
	void DrawLimbAxis(FLimb& Limb, FVector AB, FColor C, float W = 1, float Time = 0.02);

//статические
public:

	//дурацкая рутина с установкой штук по умолчанию
	static FConstraintInstance* PreInit()
	{	FConstraintInstance* CI = FConstraintInstance::Alloc();
		//CI->DisableProjection();
		CI->SetDisableCollision(false);
		CI->ProfileInstance.ConeLimit.bSoftConstraint = false;
		CI->ProfileInstance.TwistLimit.bSoftConstraint = false;
		CI->ProfileInstance.LinearLimit.bSoftConstraint = false;
		return CI;
	}

	//вычислить фрейм констрейнта через трансформацию конкретного тела (часть можно оставить)
	static void BodyToFrame (FBodyInstance* Body,
		FVector InPri,		FVector InSec,		FVector InAt,
		FVector &OutPri,	FVector &OutSec,	FVector &OutAt,
		bool TunePri = true, bool TuneSec = true, bool TuneAt = true)
	{	
		if (!Body) return;
		FTransform TM = Body->GetUnrealWorldTransform();
		if(TunePri) OutPri	= TM.InverseTransformVectorNoScale(InPri);
		if(TuneSec) OutSec	= TM.InverseTransformVectorNoScale(InSec);
		if(TuneAt)  OutAt	= TM.InverseTransformPosition(InAt);
	}

	//направление вдоль ветки вверх, неясно, почему У, но так работает
	static FVector GetBranchUpDir(FBodyInstance* Branch)
	{	auto DirAlong = Branch->GetUnrealWorldTransform().GetUnitAxis(EAxis::Y);
		if (DirAlong.Z < 0) DirAlong = -DirAlong; return DirAlong;
	}

	//точка нижнего седла сегмента ветки - пока неясно, в каком классе стоит эту функцию оставлять: меша, веткаи или какого другого
	//она просто вычисляет коррдинаты, куда телепортировать точку отсчёта кошки (низ-зад)
	static bool GetBranchSaddlePoint(FBodyInstance* Branch, FVector& Pos, float DeviaFromLow = 0.0f, FQuat* ProperRotation = nullptr, bool UsePreviousPos = false);

	//внести субъекты в констрейнт
	static EClung MakeConstraint(FConstraintInstance* CI, FVector Pri, FVector Sec, FVector At, FBodyInstance* B1, FBodyInstance* B2, bool TunePri = true, bool TuneSec = true, bool TuneAt = true);

	//"торможение" переменной - если новое значение меньше - принять сразу (разгон), если больше - впитывать плавно (тормозной путь) со скоростью А
	static inline void BrakeD (float &D, const float nD, const float A) { if(nD < D) D = nD; else D = FMath::Lerp(D, nD, A); }

	//преобразование отдельных числе лимитов в режимы
	static ELinearConstraintMotion  LinValToMode (float L) { return (L == 0)   ? ELinearConstraintMotion::LCM_Locked : ((L < 0)  ? ELinearConstraintMotion::LCM_Free    : ELinearConstraintMotion::LCM_Limited);  }
	static EAngularConstraintMotion AngValToMode (float L) { return (L == 180) ? EAngularConstraintMotion::ACM_Free :  ((L == 0) ? EAngularConstraintMotion::ACM_Locked : EAngularConstraintMotion::ACM_Limited); }

	static inline void SetLinear (FConstraintInstance* CI, float Lx, float Ly, float Lz)
	{	auto& LL = CI->ProfileInstance.LinearLimit;
		LL.XMotion = LinValToMode(Lx);		LL.YMotion = LinValToMode(Ly);		LL.ZMotion = LinValToMode(Lz);
		LL.Limit = FMath::Max3(Lx,Ly,Lz);
		CI->UpdateLinearLimit();
	}
	static inline void SetAngles (FConstraintInstance* CI, const uint8 What, const int How)
	{	switch(What)
		{	case SWING1: CI->ProfileInstance.ConeLimit.Swing1Motion = AngValToMode(How); break;
			case SWING2: CI->ProfileInstance.ConeLimit.Swing2Motion = AngValToMode(How); break;
			case TWIST:  CI->ProfileInstance.TwistLimit.TwistMotion = AngValToMode(How); break;
		}
	}

	//определить лимиты констрейнта в одну строчку
	static void SetFreedom(FConstraintInstance* CI, float X, float Y, float Z, int As1, int As2, int At)
	{	SetLinear (CI, X, Y, Z);
		auto& CL = CI->ProfileInstance.ConeLimit;
		CL.Swing1Motion = AngValToMode(As1); 	CL.Swing1LimitDegrees = As1;
		CL.Swing2Motion = AngValToMode(As2);	CL.Swing2LimitDegrees = As2;
		CI->ProfileInstance.TwistLimit.TwistMotion = AngValToMode(At);
		CI->ProfileInstance.TwistLimit.TwistLimitDegrees = At;
		CI->UpdateAngularLimit();
	}

};
