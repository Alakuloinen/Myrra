// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "Components/SkeletalMeshComponent.h"
#include "MyrPhyCreatureMesh.generated.h"

//����� ������� � ��������� �����, ����� ��� ����� �� ���������
//#define NO_BODY 255; //-127

//��� ����� ������������� �������� ������������
enum { TWIST, SWING1, SWING2 };

//�������� ��������� ������� ����������
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
// ������� ������� ����� ����������� ���� ��� ��������, �� ���� ��� ��������� �� ���������
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYRRA_API UMyrPhyCreatureMesh : public USkeletalMeshComponent
{
	GENERATED_BODY()

//��������
public:

	//����� ���� ���� � ����� �������, �  ������� ��� ����������� �  ELimb
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

	//����� ����������� - ����� �������������� �������� ������������ ���� - ������������������ �� ���� � ������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGirdle gPelvis;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGirdle gThorax;

	//������� ������ ��� ������������ ������� �����
	uint32 FrameCounter = 0;

	//�������� ��������� ����������� �����������, 1 ��������, ��� ������ ��������� �� �������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HealRate = 0.1;

	//��� ����������������� ���������� ������ �����
	FCalculateCustomPhysics OnCalculateCustomPhysics;

	//����� ���������� ���� (���� ���-�� �������� �� ����� ���� ������ �������� �� ����������� ��������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector WholeDirection;

	//������� ��� ��������� ���������� ���� ��������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AdvMoveFactor = 1.0f;

	//��� ����������� ����� ��� �������, ���� ����� ���� �������������� ���� ������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool InjureAtHit = true;

	//����������� � ��������� - ��������������� �����, ����� ��������� ��� �� ������������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool WaitingForTeleport = false;

	//��� ��������� ����������� (�� BehaveState ��� SelfAction) ������ ��� ��� ���� ������
	FWholeBodyDynamicsModel* DynModel;

#if WITH_EDITOR
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbAxes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugLimbForces;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugGirdleGuideDir;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug) FColor DebugGirdleStepped;
	FColor DebugLineChannel(ELimbDebug Ch) { return ((FColor*)(&DebugLimbAxes))[(int)Ch];  }
#endif

//����������� - ������ ��� ����� ���� �������
private:

	//��� ������ � �������, ������������� �� ���� ��������� ��� � ������� ������ ������� (�������� � �������� �����)
	static FName StepCurves[(int)ELimb::NOLIMB];

	//� ������ ����� ����������� ����������� ������ �� ��������� ������ ���� - ��� N(0) ������
	static uint8 Section[(int)ELimb::NOLIMB];

	//����� �������� � ������ ����� ���� (� ������ ����� ������ ����������)
	static ELimb Parent[(int)ELimb::NOLIMB];

	//����� ������ ��, �� � ������ �������, ��� ��� ��� ����-�����, ��� ����� \�� �����-�����
	static ELimb DirectOpposite[(int)ELimb::NOLIMB];

	//������� �� ������ ����� � ��� ����� ��� ���� � ����� �����������
	static EGirdleRay Ray[(int)ELimb::NOLIMB];

	//������ �������� ����� ��������� ��� ���� ���� ������ �����������
	static ELimb GirdleRays[2][(int)EGirdleRay::MAXRAYS];


//����������� ������ � ���������������
public: 

	//�����������
	UMyrPhyCreatureMesh(const FObjectInitializer& ObjectInitializer);

	//��� ������� ����
	virtual void BeginPlay() override;

	//������ ����
	virtual void TickComponent(float DeltaTime,	enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

//-----------------------------------------------------
protected: //�������� - ������� �� ������ �������
//-----------------------------------------------------

	//���� ��������� ����
	UFUNCTION()	void OnHit (UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	//������������ ����� ����������� ����� ������� � ����� � ����������
	UFUNCTION()	void OnOverlapEnd(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	//���������� ���
	void CustomPhysics(float DeltaTme, FBodyInstance* BodyInstance);

//���� ������ - ����������
public: 

	//�������� ��� �� �������������, � ��� ����� �������������
	static FVector GetAxis(const FTransform& T, const EMyrAxis Axis) { return T.TransformVectorNoScale(MyrAxes[(int)Axis]); }

	//������� ��������� �����
	FVector GetLimbShoulderHubPosition(ELimb eL);

	//������� ������ � ������� ����������� - ��� �������� ������ � ��
	FVector GetHeadLocation() {	return GetSocketLocation(TEXT("HeadLook")); }

	//�����-�������� � ���������� ����
	class AMyrPhyCreature* MyrOwner() { return (AMyrPhyCreature*)GetOwner(); }
	class AMyrPhyCreature* MyrOwner() const { return (AMyrPhyCreature*)GetOwner(); }

	//������ � ������ �� ��������� �� ����� � �� ���� ��� ���������� ����� ����
	FMachineLimbAnatomy* MachineGene(ELimb Limb) const;
	FFleshLimbAnatomy* FleshGene(ELimb Limb) const;
	FLimbOrient& MBoneAxes() const;

	//���-������, ������������� �������� �����, ����������� ������������ ��� ��� ������� �������������� ��������
	FBodyInstance* GetRootBody();

	//����� ����� ���� �� ������� (���� � ���������������� ������������� � ������)
	FLimb& GetLimb(ELimb li)					{ return ((FLimb*)(&Pelvis))[(int)(li)]; }
	FLimb& GetLimbParent(ELimb li)				{ return ((FLimb*)(&Pelvis))[(int) Parent[(int)li] ]; }
	FLimb& GetLimbParent(FLimb Limb)			{ return GetLimbParent(Limb.WhatAmI); }
	FLimb& GetOppositeLimb(const FLimb& Limb)	{ return GetLimb((DirectOpposite[(int)Limb.WhatAmI])); }


	//��������-��������� ��� ����� ����
	int				GetMachineBodyIndex (ELimb Limb)  const			{ return MachineGene(Limb)->TipBodyIndex; }
	int				GetMachineBodyIndex (const FLimb& Limb) const	{ return GetMachineBodyIndex (Limb.WhatAmI); }
	FBodyInstance*	GetMachineBody		(ELimb li)					{ return Bodies [GetMachineBodyIndex(li)]; } 
	FBodyInstance*	GetMachineBody		(const FLimb& Limb)			{ return Bodies [GetMachineBodyIndex(Limb)]; }
	bool			HasPhy (ELimb Limb)								{ return (GetMachineBodyIndex (Limb)!=255); }

	//������� ���� ��������� �� ����������������� � ���������
	FBodyInstance*	GetArchMachineBody	(ELimb Limb)  const;
	FBodyInstance*	GetArchMachineBody	(FLimb& Limb) const { return GetArchMachineBody (Limb.WhatAmI); }


	//������� ���������-��������� ����� ����
	int						GetMachineConstraintIndex(ELimb Limb) const		{ return MachineGene(Limb)->TipConstraintIndex; }
	int						GetMachineConstraintIndex	(FLimb& Limb) const { return GetMachineConstraintIndex (Limb.WhatAmI); }
	FConstraintInstance*	GetMachineConstraint		(ELimb Limb)		{ return Constraints[ GetMachineConstraintIndex (Limb) ]; }
	FConstraintInstance*	GetMachineConstraint		(FLimb& Limb)		{ return Constraints[ GetMachineConstraintIndex (Limb) ]; }

	//�������� ��� ���� � ����������, ���������������� � ���������
	FConstraintInstance*	GetArchConstraint			(int i) const;
	FConstraintInstance*	GetArchMachineConstraint	(ELimb Limb) const;
	FConstraintInstance*	GetArchMachineConstraint	(FLimb& Limb) const	{ return GetArchMachineConstraint (Limb.WhatAmI); }


	//������� �������� ����������� ������������� ����� ����
	bool			HasFlesh			(ELimb Limb) const						{ return (bool)FleshGene(Limb)->BodyIndex.Num(); }
	int				GetFleshBodyIndex	(ELimb Limb, int DistFromLeaf=0) const  { return FleshGene(Limb)->BodyIndex[DistFromLeaf]; }
	int				GetFleshBodyIndex	(FLimb& Limb, int DistFromLeaf=0) const { return GetFleshBodyIndex (Limb.WhatAmI,DistFromLeaf); }
	FBodyInstance*	GetFleshBody		(ELimb Limb, int DistFromLeaf=0)		{ return Bodies[GetFleshBodyIndex (Limb,DistFromLeaf)]; }
	FBodyInstance*	GetFleshBody		(FLimb& Limb, int DistFromLeaf=0)		{ return Bodies[GetFleshBodyIndex (Limb,DistFromLeaf)]; }
	
	//� ������ ����� ����������� ����������� ����� ���� ��� �������� ��������
	FGirdle& GetGirdle (ELimb Limb) { return Section[(int)Limb] ? gThorax : gPelvis; }
	FGirdle& GetGirdle (const FLimb &Limb) { return Section[(int)Limb.WhatAmI] ? gThorax : gPelvis; }
	FGirdle& GetAntiGirdle(const FGirdle* G) { return G == &gThorax ? gPelvis : gThorax; }

	//����������� ������ ����� �����������
	ELimb  GetGirdleRay (const FGirdle& G, const EGirdleRay R) { return GirdleRays[&G - &gPelvis][(int)R]; }
	FLimb& GetGirdleLimb(const FGirdle& G, const EGirdleRay R) { return GetLimb(GetGirdleRay(G, R)); }
	FBodyInstance* GetGirdleBody(const FGirdle& G, const EGirdleRay R) { return GetMachineBody(GetGirdleRay(G, R)); }

	//����������, ������� ������� ����� ����� �������������� � ������� (��-�� ������ �������� � ������ ������ ��� � ������ �������)
	FConstraintInstance* GetGirdleSpineConstraint(const FGirdle& G) { return (&G == &gThorax) ? GetMachineConstraint(Thorax) : GetMachineConstraint(Lumbus); }
	FConstraintInstance* GetGirdleArchSpineConstraint(const FGirdle& G) { return (&G == &gThorax) ? GetArchMachineConstraint(Thorax) : GetArchMachineConstraint(Lumbus); }

	//��������� ������������� ��� (�� ����� ���� ����������� ������)
	FTransform GetLimbTrans(ELimb Limb)  const { return const_cast<UMyrPhyCreatureMesh*>(this)->GetMachineBody(Limb)->GetUnrealWorldTransform(); }
	FTransform GetLimbTrans(FLimb& Limb)  const { return const_cast<UMyrPhyCreatureMesh*>(this)->GetMachineBody(Limb)->GetUnrealWorldTransform(); }

	//������������� ������ �������� ��� �� ������ (��������, �������� �� �������)
	FVector GetLimbAxis(ELimb Limb, const EMyrAxis Axis) const { return GetAxis (GetLimbTrans(Limb), Axis); }

	//������������� ������ ��� ������� �� �� �������������� �������� (����, �����...)
	FVector GetLimbAxis(ELimb Limb, const EMyrRelAxis Axis) const { return GetAxis (GetLimbTrans(Limb), MBoneAxes().ByEnum(Axis)); }

	// ��� ������� ������������ ��� ����� ������������ �������, ������ ����� ���, ��
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

	//����������� ������ ����� ���� �������� ������������ - ��� �� �������� �����,
	//���� �������� ����� ����� �����������, ��� ��� � PhAT �������� ���� � ������������� ������� ������������ � ����� ���������� �����
	FVector GetWholeForth() const { return GetComponentTransform().GetScaledAxis(EAxis::Y); }


	//���� ������ �����������
	FBodyInstance* GetPelvisBody() { return GetMachineBody(Pelvis); }
	FBodyInstance* GetThoraxBody() { return GetMachineBody(Thorax); }
	FBodyInstance* GetGirdleBody (FGirdle& Girdle) { return GetMachineBody(GetLimb(GetGirdleRay(Girdle, EGirdleRay::Center))); }

	//������ �������� � ����������� ��������
	float GetGirdleDirectVelocity(FGirdle& G) { return (G.VelocityAgainstFloor | G.Forward);  }

	//��������� ���� ���� ������ ���� (��������, ������ ������������ ��������)
	UFUNCTION(BlueprintCallable) float GetWholeDamage() const { return Head.Damage + Lumbus.Damage + Thorax.Damage + Tail.Damage + RArm.Damage + LArm.Damage + RLeg.Damage + LLeg.Damage;  }

	//��������� ������ ������������ ������� ����� ���� (��������� ��������)
	UFUNCTION(BlueprintCallable) float GetWholeImmobility();

	//�������� �� ��������� � ���������
	UFUNCTION(BlueprintCallable) float GetRootDamping() const { return const_cast<UMyrPhyCreatureMesh*>(this)->GetPelvisBody()->LinearDamping; }

	//�������� �� ���� ������ ���������� ����������� � ����� (����
	bool CanGirdleCling(const FGirdle& Girdle) const;

	//������ ���������� ���������� ���� �� ���������� � �������� ������� ������� �� �������� ���
	FBodyInstance* GetBody(UPrimitiveComponent* C, int8 CB) { return (CB==NO_BODY) ? C->GetBodyInstance() : ((USkeletalMeshComponent*)C)->Bodies[CB]; }

	//����� �� �� ���� ����������� ������
	static bool IsClimbable(EMyrSurface surf) { return (surf == EMyrSurface::Surface_WoodRaw || surf == EMyrSurface::Surface_Fabric/* || surf == EMyrSurface::Surface_Flesh*/); }

	//����� ����� �� �����
	bool IsLyingDown() const { return (Lumbus.Stepped && Lumbus.ImpactNormal.Z > 0.5) || (Pectus.Stepped && Pectus.ImpactNormal.Z > 0.5); }

	//������� �������������
	float Erection() const { return 0.5*(GetLimbAxisForth(ELimb::LUMBUS).Z + GetLimbAxisForth(ELimb::PECTUS).Z); }

	//����� ���������������� ������� �� ������-����������� ����� �����, ��� ��� "����" ����� ���� ��������� ��-�� ����������� ���
	bool IsSpineDown(float Thresh = 0.0f) const { return (GetLimbAxisUp(ELimb::LUMBUS).Z < Thresh); }

	//����� �������, �� �������� ���, ����� ���������� ��� ���������� ����, � �� �������� ������� 
	bool IsClingAtStop(FConstraintInstance* CI) {	return CI->GetLinearYMotion() == ELinearConstraintMotion::LCM_Locked;	}

	//������� �������� � ���������, ������� ��������� ��� �� ����������� 
	bool IsVerticalTransition(FConstraintInstance* CI) { return CI->ProfileInstance.AngularDrive.SwingDrive.bEnablePositionDrive; }

	//�������� �� ������ ����� ������ ������ ���� ������� ������
	bool Tango(FLimb& Limb, AActor* A) { if (Limb.Floor) if (Limb.Floor->OwnerComponent->GetOwner() == A) return true; return false; }
	bool Tango(FLimb& Limb, UPrimitiveComponent* A) { if (Limb.Floor) if (Limb.Floor->OwnerComponent.Get() == A) return true; return false; }
	bool IsTouchingActor(AActor* A) { return Tango(Pelvis, A) || Tango(Lumbus, A) || Tango(Thorax, A) || Tango(Pectus, A) || Tango(RArm, A) || Tango(LArm, A) || Tango(LLeg, A) || Tango(RLeg, A) || Tango(Head, A) || Tango(Tail, A); }
	bool IsTouchingComponent(UPrimitiveComponent* A) { return Tango(Pelvis, A) || Tango(Lumbus, A) || Tango(Thorax, A) || Tango(Pectus, A) || Tango(RArm, A) || Tango(LArm, A) || Tango(LLeg, A) || Tango(RLeg, A) || Tango(Head, A) || Tango(Tail, A); }

	//�������� ����������� ��������� �� �����
	float StuckToStepAmount(const FLimb& Limb, FVector Dir) const;

	//�������� ����������� ��������� �� �����
	float StuckToStepAmount(FVector Dir, ELimb& LevelOfContact) const;
	
	//����� �� ����� �� ���� ����� - ������� ������ ������ ����� ���� � ����������� �� ����, ���� ���� ��� ���
	bool IsStandingOnActor(AActor* A) 
	{ 	if(gPelvis.HasLegs) { if(Tango(LLeg, A)||Tango(RLeg, A)) return true; else if(Tango(Lumbus, A)||Tango(Pelvis, A)) return true; }
		if(gThorax.HasLegs) { if(Tango(LArm, A)||Tango(RArm, A)) return true; else if(Tango(Thorax, A)||Tango(Pectus, A)) return true; } return false;
	}

	//���� � ������ ��������������� � �������������, �� ���� �����������
	bool IsBumpable() { return GetCollisionObjectType() != ECollisionChannel::ECC_Vehicle; }


//���� ������ - ���������
public:

	//�������� ��� ����������� ������ ����� ���� - �������� �� �������� ���� ����������� ����������� ���, ������� ���� ���������
	FVector CalcAxisForLimbDynModel(const FLimb Limb, FVector GloAxis) const;

	//��������� �� ��������� �� �������������� �����������
	void Move (FBodyInstance* BodyInst, FLimb& Limb);

	//������������� ���� ������, ����������� ��� ��������
	void OrientByForce (FBodyInstance* BodyInst, FLimb& Limb, FVector ActualDir, FVector DesiredDir, float AdvMult = 1.0);


	//��������� ������
	void PhyJump(FVector Horizontal, float HorVel, float UpVel);
	void PhyJumpLimb(FLimb& Limb, FVector Dir);

	//�������/������ �� ���� ����� �� ������ ����
	void PhyPrance(FGirdle& G, FVector HorDir, float HorVel, float UpVel );

	//��������������� �� ���� �� ����� �����
	void TeleportOntoBranch(FBodyInstance* CapsuleBranch);

	//����������� ���� � ���������� (���������� �� ��������, � � �������� �� �������)
	void ClingGetReady(bool Set);


	//���������� ��������� ���� �� ������������ � ������ �����
	float ApplyHitDamage(FLimb& Limb, uint8 DistFromLeaf, float TermialDamageMult, float SpeedSquared, const FHitResult& Hit);

	//���������� ����� ��� - ��� ���������, �� ����� ������, ��� ��������
	void InitFeetLengths(FGirdle& G);

	//�������������� ������������� ����� ����
	void InitLimb(FLimb& Limb, ELimb Me) {Limb.WhatAmI = Me;}
	void InitLimbCollision(FGirdle& G, EGirdleRay GR, float MaxAngularVeloc, FName CollisionPreset = NAME_None);

	//� ����� �� ������� ������� ���� ����� ����
	void HitLimb(FLimb& Limb, uint8 DistFromLeaf, const FHitResult &Hit, FVector NormalImpulse);

	//�������� ���������� ������ � ��������� ����� ����
	void ProcessLimb(FLimb& Limb, float DeltaTime, float* NewStandHardnesses);

	//���������� ������ �������� ����� ���������� (������� ����� �� ������)
	void SetLimbDamping(FLimb& Limb, float Linear, float Angular = -1);

	//��������� ���� � ��������� � �������� ��������
	void SetLimbDampingFromModel(FLimb& Limb, float Base, uint32 BitField)
	{	if (!(BitField & LDY_DAMPING))	{	SetLimbDamping(Limb, 0, 0);	return;		}
		float DampMult = 0.25 * (bool)(BitField & LDY_DAMPING_4_1) + 0.25 * (bool)(BitField & LDY_DAMPING_4_2) + 0.5 * (BitField & LDY_DAMPING_2_1);
		SetLimbDamping(Limb, DampMult * Base);
	}

	//��������� ��� ������
	void PhyDeflect(FLimb& Limb, float Weight);

	//�������� ���� ������ � ������ ��� ���� ����� ����
	void ProcessBodyWeights(FLimb& Limb, float DeltaTime);

	//��������� ��������������� ����������� �������� ��� ����� ������� �� ��� �����
	FVector CalculateLimbGuideDir(FLimb& Limb, const FVector& PrimaryCourse, bool SupportOnBranch, float* Stability = nullptr);

	//������� ��� ��������� ������ ��� ������������ ����� ����� ��� ����� �������
	int ResolveNewFloor(FLimb &Limb, FBodyInstance* NewFloor, FVector NewNormal, FVector NewHitLoc);

	//����������� � ����� � ����������� �������� �� ����� ������������
	EClung ClingLimb(FLimb& Limb, bool NewFloor);
	void UnClingLimb(FLimb& Limb);
	void UntouchLimb(FLimb& Limb) { if (Limb.Clung) UnClingLimb(Limb); Limb.Stepped = 0; Limb.Floor = nullptr; Limb.Surface = EMyrSurface::Surface_0; }

	//����� ������ ������� �� ������ ������ ������ ������ ����
	void Grab (FLimb& GrabberLimb, uint8 ExactBody, FBodyInstance* GrabbedBody, FLimb* GrabbedLimb, FVector ExactAt);
	void UnGrab (FLimb& GrabberLimb);
	void UnGrabAll ();


	//���������������� ���� ����������� ��� ��� ����������� ��������� ������ ���� (���� ����/���, DOF Constraint)
	void InitGirdle (FGirdle& Girdle, bool StartVertical=true);

	//������ ���������������� ���������� ����� ����������� - ��� ����� ���� (������, ���, ��� ������ �������)
	void AdoptDynModelForGirdle (FGirdle& Girdle, FGirdleDynModels& Models);
	void AdoptDynModelForLimb(FGirdle& Girdle, FLimb& Limb, uint32 Model, float DampingBase);

	//�������� "����" ����������� ������������ ������� (����������� �� ����� �� ��������� ��� �� ������ �����-������)
	void UpdateCentralConstraintFrames(FGirdle& G);

	//������ � ���������� ������������ ������� ����� ������ �� ���������� ������� ��������� � ������ �� ��������
	void UpdateCentralConstraintLimits(FGirdle &G);

	//�������� ���������� ������ ��� ����� �����������
	void ProcessGirdle (FGirdle& Girdle, FGirdle& OppositeGirdle, float DeltaTime, float DirToMiddle);
	void ProcessGirdleFoot(FGirdle& Girdle, FLimb& FootLimb, FLimb& OppositeLimb, float DampingForFeet, FVector PrimaryForth, float& WeightAccum, bool& NeedForVertical, float DeltaTime);

	//float CalculateGirdleVelocity();

	//��������� ���������� ��� ����� ����� ����� ��������
	float CalculateGirdleFeetDamping(FGirdle &Girdle);

	//������� ��� ������ �������� ������������ ������������ ����� �����������
	void SetGirdleVertical (FGirdle& G);
	void ClearGirdleVertical(FGirdle& G);

	//���������� ������ �� ��������
	void SetGirdleYawLock(FGirdle &G, bool Set);

	//�������� ��� ��������� �������� ������� ����� ������������ ��� (��� ������ ������-�����)
	void SetGirdleSpineLock(FGirdle &G, bool Set);

	//���������� ��� ������ � ���������� ��������
	void SetFullBodyLax(bool Set);

	//��������� � ������� ������� ����������� ����� ��� ���� ���� ������������ � ������� ����������, ���� ���
	void SetPhyBodiesBumpable(bool Set);


//����� ��� �������
public:

	//���� ���������� ������ �� ������
	void Log(FLimb& L, FString Addi)				{ UE_LOG(LogTemp, Log, TEXT("%s: Limb %s: %s"), *GetOwner()->GetName(), *TXTENUM(ELimb, L.WhatAmI), *Addi); }
	void Log(FLimb& L, FString Addi, float Param) { UE_LOG(LogTemp, Log, TEXT("%s: Limb %s: %s %g"), *GetOwner()->GetName(), *TXTENUM(ELimb, L.WhatAmI), *Addi, Param); }
	void Log(FLimb& L, FString Addi, float Param, float Param2) { UE_LOG(LogTemp, Log, TEXT("%s: Limb %s: %s %g %g"), *GetOwner()->GetName(), *TXTENUM(ELimb, L.WhatAmI), *Addi, Param, Param2); }
	void Log(FLimb& L, FString Addi, FString Param) { UE_LOG(LogTemp, Log, TEXT("%s: Limb %s: %s %s"), *GetOwner()->GetName(), *TXTENUM(ELimb, L.WhatAmI), *Addi, *Param); }
	void Log(FGirdle& G, FString Addi)				{ UE_LOG(LogTemp, Log, TEXT("%s: Girdle:%s: %s"), *GetOwner()->GetName(), (&G == &gThorax) ? TEXT("Thorax") : TEXT("Pelvis"), *Addi); }
	void Log(FGirdle& G, FString Addi, float Param) { UE_LOG(LogTemp, Log, TEXT("%s: Girdle:%s: %s %g"), *GetOwner()->GetName(), (&G == &gThorax) ? TEXT("Thorax") : TEXT("Pelvis"), *Addi, Param); }

	void DrawMove(FVector A, FVector AB, FColor C, float W = 1, float Time = 0.02);
	void DrawAxis(FVector A, FVector AB, FColor C, float W = 1, float Time = 0.02);
	void DrawLimbAxis(FLimb& Limb, FVector AB, FColor C, float W = 1, float Time = 0.02);

//�����������
public:

	//�������� ������ � ���������� ���� �� ���������
	static FConstraintInstance* PreInit()
	{	FConstraintInstance* CI = FConstraintInstance::Alloc();
		//CI->DisableProjection();
		CI->SetDisableCollision(false);
		CI->ProfileInstance.ConeLimit.bSoftConstraint = false;
		CI->ProfileInstance.TwistLimit.bSoftConstraint = false;
		CI->ProfileInstance.LinearLimit.bSoftConstraint = false;
		return CI;
	}

	//��������� ����� ����������� ����� ������������� ����������� ���� (����� ����� ��������)
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

	//����������� ����� ����� �����, ������, ������ �, �� ��� ��������
	static FVector GetBranchUpDir(FBodyInstance* Branch)
	{	auto DirAlong = Branch->GetUnrealWorldTransform().GetUnitAxis(EAxis::Y);
		if (DirAlong.Z < 0) DirAlong = -DirAlong; return DirAlong;
	}

	//����� ������� ����� �������� ����� - ���� ������, � ����� ������ ����� ��� ������� ���������: ����, ������ ��� ������ �������
	//��� ������ ��������� ����������, ���� ��������������� ����� ������� ����� (���-���)
	static bool GetBranchSaddlePoint(FBodyInstance* Branch, FVector& Pos, float DeviaFromLow = 0.0f, FQuat* ProperRotation = nullptr, bool UsePreviousPos = false);

	//������ �������� � ����������
	static EClung MakeConstraint(FConstraintInstance* CI, FVector Pri, FVector Sec, FVector At, FBodyInstance* B1, FBodyInstance* B2, bool TunePri = true, bool TuneSec = true, bool TuneAt = true);

	//"����������" ���������� - ���� ����� �������� ������ - ������� ����� (������), ���� ������ - ��������� ������ (��������� ����) �� ��������� �
	static inline void BrakeD (float &D, const float nD, const float A) { if(nD < D) D = nD; else D = FMath::Lerp(D, nD, A); }

	//�������������� ��������� ����� ������� � ������
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

	//���������� ������ ����������� � ���� �������
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
