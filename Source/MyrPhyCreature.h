// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "MyrPhyCreatureMesh.h"
#include "GameFramework/Pawn.h"
#include "AssetStructures/MyrCreatureGenePool.h"
#include "AssetStructures/MyrCreatureAttackInfo.h"
#include "AssetStructures/MyrCreatureBehaveStateInfo.h"	// ������ �������� ��������� ��������
#include "MyrPhyCreature.generated.h"

//���� ����� ������
DECLARE_LOG_CATEGORY_EXTERN(LogMyrPhyCreature, Log, All);

UCLASS() class MYRRA_API AMyrPhyCreature : public APawn
{
	GENERATED_BODY()

	//���������� ������ ���� �� ��������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	class UMyrPhyCreatureMesh* Mesh;

	//����������� � ������, �������� ��� ����� ���� ��� ��������������� �������� �� ������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	class UCapsuleComponent* KinematicSweeper;

	//��������� �������� � ������ ����� ��� ������� (��������, �����, ����� ����� � ����, � ������ �� ��������� �����)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	class UWidgetComponent* LocalStatsWidget;

	//�����-��������� ������������� ��� ��������� ����, � ��� ����� � ����, �������� �� ���������������� ��� ����� � ������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	class UAudioComponent* Voice;

public:

	//��������� �� ����� ���������� ������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) class AMyrDaemon* Daemon;

	//���, ������� ����� ������������ ������������ ������ - ����� ���� ��������� � ����������
	//��� ������ ������������ ���������� �� ��������� � ������� ������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText HumanReadableName;

	//������� ����/�������� - � ������ ������ ����� ���� ��������, ���� ����� ����� ����� ��������������
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 CurrentAttack = 255;		// �������� �� ����
	UPROPERTY(EditAnywhere, BlueprintReadonly) EActionPhase CurrentAttackPhase = EActionPhase::ASCEND;
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 CurrentAttackRepeat = 0;
	UPROPERTY(EditAnywhere, BlueprintReadonly) float AttackForceAccum = 0.0f;		//����������� ���� ��� ������ ���� ������ �� ��������

	//������������ - ���������� ���� ����� �� ������� �� ��������
	UPROPERTY(VisibleAnywhere, BlueprintReadonly) uint8 CurrentSelfAction = 255;	// ������� �����������
	UPROPERTY(VisibleAnywhere, BlueprintReadonly) uint8 SelfActionToTryNow = 0;		// �����������, ���� ��������, �� ��������� (�������)
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 SelfActionPhase = 0;			// � ������������ ����� ���� ��������� ��� (�� ��������� � ������) � ������ �������

	//������������ ������, �������� ��������� ���������� �������� �� ������ � ����� ������ 3 ����: ����, ����������, �����
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 CurrentRelaxAction = 255;
	UPROPERTY(EditAnywhere, BlueprintReadonly) uint8 RelaxActionPhase = 0;			

	//�����, �� ������� "�����" ����� ������� ����� ������
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)  uint8 SmellChannel = 0;

	//���� ������������ ���� - ����� ��� �������� ����� ���
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EPhoneme CurrentSpelledSound = EPhoneme::S0;

	//���� ��������� ����� ���� ����������� ������ ��� ������� ������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UMyrCreatureGenePool* GenePool;

	//��������� ������� ����������� ��������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector MoveDirection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector AttackDirection;

	//������� ��������� ����������� ����������
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ExternalGain = 0.0f;			// ����������� ��������, ��� ������� �����
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MoveGain = 1.0f;				// ����� � �������� ��-�� ���������, ������
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Health = 1.0f;					// ��������
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Stamina = 1.0f;				// ����� ���
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Energy = 1.0f;					// ���������� ����� ���, ����������������� ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Pain = 0.0f;					// ���� - ������ ��������� ������������� ��� �������� ��������
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float AttackStrength = 0.0f;			// ���� �����, ���� ������������ ��� ��������� ������ � ��������� �����, ���� �������� ���� �������� ��� ������� ������
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Age = 0.0f;					// �������, ������ ������� � ��������, ����� ����������� � ������� ������� (��� double ��� int64?)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Stuck = 0.0f;					// ������� ����������� (���� = �����, ����� �������, ����� = �����������, ������� ��� ��� ������)

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FDigestivity Stomach;				// ����� ������ ����������� ��������� ��������� ����						

#if WITH_EDITOR
	UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 Coat = 0;						// ����� - ������ ��� ���������, ����� ����������� ������ ������� �� ����� ��������					
#endif

																					//������� ������
	uint16 FrameCounter = 0;

	//����������, ��������������� ��������� ��������, �������������, 																			
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRoleParams Params;
	
	// ������� ��������� ���������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EBehaveState CurrentState;			// ������� ���������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EBehaveState UpcomingState;			// ��������� �������� (���� ��� ���, �� ��������� ���������)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float StateTime = 0.0f;				// ����� � �������� � ������� ��������� ���������

	//��������� �� ������ �������� ������ ���������, ��� ��� ��������, ��� ����� ������ ��������������
	//������ ������� ����� ��� ��������� �������� ��� ����, ���� � �.�.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UMyrCreatureBehaveStateInfo* BehaveCurrentData;

public:
	//����� ��������� ���������� � �������� �� ���������
	AMyrPhyCreature();

	//����� �������������� �����������, ����� ���������� �� �������� �� ������������ ������� �� ����� ����� �������������
	virtual void PreInitializeComponents() override;

	//������ ����
	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	
	//��� ��������������� ������� ��� ������ ���������
	virtual void PostEditChangeProperty (FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	//��������� � ���� - ��������� �����������, ����� �� ��������� ������
	virtual void BeginPlay() override;

	//������� � ���������� ����������� ����� (������������ �����, �������� ������ - LipSync � �.�.)
	UFUNCTION()	void OnAudioPlaybackPercent(
		const class USoundWave* PlayingSoundWave,
		const float PlaybackPercent);
	UFUNCTION()	void OnAudioFinished();

//���� ���������
public:	

	//������������� ��� ��� ���������� ��������� ����� ����
	//����������� ����� � ����������, ��� ��� � ������� �� �� �������
	UFUNCTION(BlueprintImplementableEvent) FText GenerateHumanReadableName();
	
	//��� �������-������ ��������� ����������������
	void RegisterAsSpawned(AActor* Spawner);

	//���������� ������� ������ ��������������
	void ConsumeDrive(FCreatureDrive *D);

	//����� ������� � ������ �� ������������ ������ � �� � ������ ���������
	void ConsumeInputFromControllers(float DeltaTime);

	//������ ������� ����������� ��������
	bool TurnMoveDirection(FVector OldMoveDir, float StartThresh = 0.0f, float SidewayThresh = -0.8, float SmoothGradus = 0.1);

	//��������� ������� ���������� � ����������� � ��� �� ���������� �������������
	void WhatIfWeBumpedIn();

	//�������� ���������� ���������� �������� �� ��������� � ����� ��� ������� ������������� ���������
	void UpdateMobility(bool IncludeExternalGain = true);

	//������ ��������� ���� � �������� �������
	void RelievePain(float DeltaTime) { Pain *= (0.9 - 0.1 * (Stamina + Health)); }

	//��� ������������ ����������� ������
	void MakePossessedByDaemon(AActor* aDaemon);

	//��������� ��������� ����� ������ ��� ��� ����� ����
	void AdoptWholeBodyDynamicsModel(FWholeBodyDynamicsModel* DynModel, bool Fully);

	//������� � ����� ���� �����
	void AdoptAttackPhase(EActionPhase NewPhase);

	//�������� ��� ��������� ������� ��� ���������� ����������� ���������� �� ���
	void SetWannaClimb(bool Set);

	//����� ������ � ������ - ��� ������, ������ ������������ �� ��, ����� ����� �� ����� ��
	int FindObjectInFocus(const float Devia2D, const float Devia3D, AActor*& Result, FText& ResultName);

	//���������� ����� ��� (���������) ���� ��������>0, ��� ������ (������) - ���������� �� ������� ����
	void StaminaChange(float Dp);
	void HealthChange(float Dp);

	//����� ����������� �� ����� - ����� ������������� ������
	void PostTeleport();

	//������ ���, ���������� ���� ��� � �������
	void RareTick(float DeltaTime);

	//������� ��� - ������������� ����, ������ ����, ����
	void MakeStep(ELimb WhatFoot, bool Raise);

	//���������� �� ���������� ����������� (��������� �� ����)
	void Hurt(float Amount, FVector ExactHitLoc, FVector Normal, EMyrSurface ExactSurface, FSurfaceInfluence* ExactSurfInfo = nullptr);

	//��������� ��������, ��� ���������� �� �������� ������� �������� ��� �������� ���� �� ���������
	void SufferFromEnemy(float Amount, AMyrPhyCreature* Motherfucker);
	void EnjoyAGrace(float Amount, AMyrPhyCreature* Sweetheart);

	//��������� ����� ������ ���� � �.�. �� ���� ������� �������� �����������
	void SurfaceBurst(UParticleSystem* Dust, FVector ExactHitLoc, FQuat ExactHitRot, float BodySize, float Scale);

	//��������� ���� �������� � ������������ / ���, ����
	void SoundOfImpact(FSurfaceInfluence *SurfInfo, EMyrSurface Surface, FVector Loc, float Strength, float Vertical, EBodyImpact ImpactType);

	//��������� ����� ���� (��������������� ����� � ������ ����������� �����)
	void MakeVoice(USoundBase* Sound, uint8 strength, bool interrupt);

	//////////////////////////////////////////////////////////////////////

	//��������� ����� (����������� ��������) ����� ���������
	void QueryBehaveState(EBehaveState NewState);

	//������� ����� ��������� ��������� (��������� ����������, �� ���������� ����� ���-�� �� ���������, ������ ����)
	bool AdoptBehaveState(EBehaveState NewState);

	//�������� ������ �������� - ����� � �������� ������� ��������� � ���������� ����� ��������
	void ProcessBehaveState(FBodyInstance* BodyInst, float DeltaTime);

	//���������� �������� �� ����� ��� ������
	bool BewareFall() { if(Mesh->gThorax.StandHardness + Mesh->gPelvis.StandHardness <= 0.01f) return AdoptBehaveState(EBehaveState::fall); else return false; }

	//�������� �� ������ �������� ��� ������ ������ ���� - ��������, ������ ���������� ����� BewareFall, ��� ��� ������� "��� � �������" ����� ������
	bool BewareHangTop() { if(Mesh->gThorax.StandHardness < 0.01f) return AdoptBehaveState(EBehaveState::hang); else return false; }
	bool BewareHangBottom() { if(Mesh->gPelvis.StandHardness < 0.01f) return AdoptBehaveState(EBehaveState::hang); else return false; }

	//��������� ��������� (������ ����� �������� Z ������� ����� �����)�� ������� ��������� ������� lie ��� �������� tumble
	bool BewareOverturn(float Thresh = 0.0f, bool NowWalking = true);

	//� ��������� ����������������� ���������, ���� �� �����������, ������ �� ����
	bool BewareFeetDown(float Thresh = 0.0f) { return (!Mesh->IsSpineDown(Thresh)) ? AdoptBehaveState(EBehaveState::walk) : false; }

	//������������� ��������� �� ����� � ����� ���������
	bool BewareStuck(float Thresh = 0.5);

	//��������� �� ����� ��� ������
	bool BewareGround()
	{	if (Mesh->gThorax.StandHardness + Mesh->gPelvis.StandHardness > 0.01)
		{	if (Attacking())
				if (GetCurrentAttackInfo()->JumpPhase <= CurrentAttackPhase)
					AttackEnd();						//��������� ����� ������ ���� ��������� ��������� ��� ��������� ������
			AdoptBehaveState(EBehaveState::walk);		//walk - ��� �� ���������, ����� ��� ������ ����������� � ������ ���������
			return true;								//�������� ������ �� ������ �������������� ���������
		} else return false;							//���������� ����������� ������ �����������
	}

	//������� �������� � ������
	bool BewareDying();

	//��������� �������� �� ������� � �������
	bool BewareLowVel(EBehaveState NewState, float T = 1.0f) { return (Mesh->gPelvis.VelocityAgainstFloor.SizeSquared() < T && Mesh->gThorax.VelocityAgainstFloor.SizeSquared() < T) ? AdoptBehaveState(NewState) : false; }

	//��������� �� ������� � ����������� �������� - ����� ������ �������� ��������� � �����, ��� ��� �� ���������� � �������
	bool BewareSoarFall() { return (Mesh->GetPhysicsLinearVelocity().Z<-50) ? AdoptBehaveState(EBehaveState::fall) : false; }

	//////////////////////////////////////////////////////////////////////
	//����� - ������, �������, ���������� ��������
	
	//��������� ������ �� ��������� ��������� ��������
	bool JumpAsAttack();

	EAttackAttemptResult AttackStart (int SlotNum, UPrimitiveComponent* ExactGoal = nullptr);
	EAttackAttemptResult AttackStrike (ECreatureAction ExactType = ECreatureAction::NONE, UPrimitiveComponent* ExactGoal = nullptr);
	void AttackGetReady();
	void AttackEnd();					//���������� 
	void AttackGoBack();
	bool AttackNotifyGiveUp();			//���������� ���������
	void AttackNotifyEnd();				//������ ��������		
	void AttackNotifyFinish();			//������ �������� ����� - ������� � ��������� ���� ����� �����������
	EAttackAttemptResult AttackStrikePerform(UPrimitiveComponent* ExactGoal = nullptr);	//���������� ����������� ����������
	void PrepareAttackingLimbs();		//����������� ������ �������

	//������, ������� � �������� ������������
	void SelfActionStart(int Slot);
	void SelfActionCease();
	void SelfActionNewPhase();
	void SelfActionFindStart(ECreatureAction Type, bool CheckForChance = true);
	void ActionFindList(bool RelaxNotSelf, TArray<uint8>& OutActionIndices, ECreatureAction Restrict = ECreatureAction::NONE);

	//�����, ������� � ������ �������� ������
	bool RelaxActionFindStart();	//����� ���������� �������� � ��������� �������
	void RelaxActionStart(int Slot);//���������� ������ ��� ��������� �� �������
	void RelaxActionReachPeace();	// * ������� ���� ������������ (���������� �� �������� �������� MyrAnimNotify)
	void RelaxActionStartGetOut();	//������ ����� �� ���� ������������
	void RelaxActionReachEnd();		// * ������� ����� � ������� ������ (���������� �� �������� �������� MyrAnimNotify)

	//�������� ��� ��������� ���������� ��������, ��� ������� ������� ��������� ��� ������� �������
	void CeaseActionsOnHit();		

	//������ ��������� �����-�� �������� �� ������� �������������� (���� ���� � ���������)
	bool ActionFindStart(ECreatureAction Type);

	//��������� ��, ��� ������� ������� ���������� (� �����)
	void UnGrab();	

	//�������� �� ������� ��, ��� �� ����� � �������� ���-��
	void WidgetOnGrab(AActor* Victim);
	void WidgetOnUnGrab(AActor* Victim);

	////////////////////////////����� ����////////////////////////////////

	//����� ��� ����� ������ ���� (���������� ������)
	UPrimitiveComponent* FindWhatToEat(FDigestivity*& D, FLimb*& LimbWith);	

	//���������� �������� ����������� ���������� ������� (������ ��� �� ������)
	bool EatConsume(UPrimitiveComponent* Food, FDigestivity* D);

	//��������� ������ ������� � ���� ����� ���� ��� �������
	bool Eat();

	//////////////////////������� �� ������� �������//////////////////////

	//����������, ��� �� ���������� ����� ����� � �������� ���, ��� �� ���������, ��������, ������������ ��������
	void SendApprovalToOverlapper();

	//��������� ������� ��� �����������, ��� ��� ������� ����� ���� ����� �������������,
	//������ ��� ������� ��������� ��������� ���� ��������� �� �����������
	bool CouldSendApprovalToTrigger(class UMyrTriggerComponent* Sender);

	//�������� �� ������� ���� ��, ��� ������������ �������-����� ���� ��� ��������
	void WigdetOnTriggerNotify(ETriggerNotify Notify, AActor* What, USceneComponent* WhatIn, bool On);

	//////////////////////////////////////////////////////////////////////

	//����� �� ��������� ��������, ��������������� ������ � ����������� �������� �����
	void SetCoatTexture(int Num);

	//�������� ��� �������� ��� �������� � ���������� ����
	UFUNCTION(BlueprintCallable) void Save(FCreatureSaveData& Dst);
	UFUNCTION(BlueprintCallable) void Load(const FCreatureSaveData& Src);
	
	//��������� �������� �� ����� ����������� ������ - �������� ����, ������, �����
	//��������, ������ AActor ������� ������ UObject, ����� ���������� � ����������, � �� ������
	UFUNCTION(BlueprintCallable) void CatchMyrLogicEvent(EMyrLogicEvent Event, float Param, UPrimitiveComponent* Goal);

	//�������� ���������� � �������� �� �� (����� �� ������� �� � ������ ��������)
	void TransferIntegralEmotion(float& Rage, float& Fear, float& Power);

//���� ����������
public:	

	//������ � ���������� �����
	UFUNCTION(BlueprintCallable) class UMyrraGameInstance* GetMyrGameInst() const { return (UMyrraGameInstance*)GetGameInstance(); }
	UFUNCTION(BlueprintCallable) class AMyrraGameModeBase* GetMyrGameMode() const { return (AMyrraGameModeBase*)GetWorld()->GetAuthGameMode(); }

	//������ ��������� �� ���������
	UMyrPhyCreatureMesh* GetMesh() { return Mesh; }

	//������ ��������� �� ���������
	UCapsuleComponent* GetSweeper() { return KinematicSweeper; }

	//����������� �������
	bool IsUnderDaemon() { return Daemon != nullptr; }

	//������� ��� �� ������� ���� (���������� ����� ������)
	bool IsFirstPerson();

	//���������� �������������� ����������
	UFUNCTION(BlueprintCallable) class AMyrAI *MyrAI() const { return (AMyrAI*)Controller; }

	//������ ��������� �� �������� �����
	UFUNCTION(BlueprintCallable) UMyrCreatureGenePool* GetGenePool() const { return GenePool; }

	//������� ���������, ���������������� �� ����, ���� �����-�� �������
	EBehaveState GetBehaveCurrentState() const { return CurrentState; }

	//������ ����� ������, ��������� � ������� ���������� ��������
	UFUNCTION(BlueprintCallable) UMyrCreatureBehaveStateInfo* GetBehaveCurrentData() const { return BehaveCurrentData; }

	//������ ������ �� �������� (�����) �� ���� ������� - ��������, ������� ������� �� �����������
	UFUNCTION(BlueprintCallable) UMyrAttackInfo* GetCurrentAttackInfo() const { return GenePool->AttackActions[(int)CurrentAttack]; }
	UFUNCTION(BlueprintCallable) UMyrActionInfo* GetCurrentSelfActionInfo() const { return GenePool->SelfActions[(int)CurrentSelfAction]; }
	UFUNCTION(BlueprintCallable) UMyrActionInfo* GetCurrentRelaxActionInfo() const { return GenePool->RelaxActions[(int)CurrentRelaxAction]; }

	UFUNCTION(BlueprintCallable) UMyrAttackInfo* GetAttackInfo(int i) const { return GenePool->AttackActions[i]; }
	UFUNCTION(BlueprintCallable) UMyrActionInfo* GetSelfActionInfo(int i) const { return GenePool->SelfActions[i]; }
	UFUNCTION(BlueprintCallable) UMyrActionInfo* GetRelaxActionInfo(int i) const { return GenePool->RelaxActions[i]; }

	//������� ����������� - ���������� �������� �������, �����������, �������� � ������
	UFUNCTION(BlueprintCallable) float GetMetabolism() const;

	//��������� �� ��� ����� ���� � ��������������� �����
	bool IsLimbAttacking(ELimb eLimb);

	//�������� �� ��/����� �� �� �� ���� �������
	bool IsTouchingThisActor(AActor* A);
	bool IsTouchingThisComponent(UPrimitiveComponent* A);
	bool IsStandingOnThisActor(AActor* A);

	//������� ���� ����� ������������� ��������� � ������ ���� ����� ��� ������ ��������: ������������� � ������, ��������, �������� ��� �������
	bool NowPhaseToJump() { return (GetCurrentAttackInfo()->JumpPhase == CurrentAttackPhase); }
	bool NowPhaseToHold() { return (GetCurrentAttackInfo()->JumpHoldPhase == CurrentAttackPhase); }

	//������ ���� ����� �������� - ��� �������, ���� ��� ���������, ����� �� ��������� � ����� ����������, ����� ��� ������
	bool NowPhaseToGrab()
	{ 	auto P = GetCurrentAttackInfo()->GrabAtTouchPhase;
			if (P == CurrentAttackPhase)
				if (GetCurrentAttackInfo()->UngrabPhase != CurrentAttackPhase) return true;
		return false;
	}

	//���� �� ���������� ������� �����
	bool NoAttack() const { return (CurrentAttack == 255); }
	bool Attacking() const { return (CurrentAttack != 255); }
	bool NoSelfAction() const { return (CurrentSelfAction == 255); }
	bool DoesSelfAction() const { return CurrentSelfAction<255; }
	bool NoRelaxAction() const { return (CurrentRelaxAction == 255); }
	bool DoesRelaxAction() const { return (CurrentRelaxAction < 255); }

	bool PreStrike() const { return (int)CurrentAttackPhase <= (int)EActionPhase::READY; }

	//������ �����-�� �� ���� ����� ��������
	bool DoesAnyAction() const { return (CurrentAttack < 255) || (CurrentSelfAction<255) || (CurrentRelaxAction<255); }

	//��������
	bool Dead() { return (CurrentState == EBehaveState::dead);  }

	//��� ���������� �������� (������������� �� ����, ��������� ����� ����� ������ ����������)
	FVector GetAxisForth() const { return ((UPrimitiveComponent*)Mesh)->GetComponentTransform().GetScaledAxis(EAxis::X); }
	FVector GetAxisLateral() const { return ((UPrimitiveComponent*)Mesh)->GetComponentTransform().GetScaledAxis(EAxis::Y); }

	//������� ������ � ������� ����������� (������� �� ������� ����)
	FVector GetHeadLocation();

	//������ ���������� ������ ���� ���������� ����� ����
	FVector GetBodyPartLocation(ELimb Limb);

	//�������� ����� ��� ����������� � ����� �������� ���������
	FVector GetVisEndPoint();

	//����� ������� ��� ����� ����, ������� ����� ����� � ������������ �����
	FVector GetClosestBodyLocation(FVector Origin, FVector Earlier, float EarlierD2);

	//������, � ������� ���� ������� ������� - ��� ����������� "�� ��������"
	FVector GetLookingVector() const;

	//������ ���� �����
	FVector GetUpVector() const;

	//�������� ��������� ������������, ����������� ������������, �� ������� �� �����
	float GetCurrentSurfaceStealthFactor() const;

	//������� �������� ����� �������� ��� ��� ��� ������� ��������
	float GetNutrition() const;

	//������� ��������, � ������� �� ����� ���������
	UFUNCTION(BlueprintCallable) float GetDesiredVelocity() const { return BehaveCurrentData->MaxVelocity * MoveGain; }

	//����� ����� 
	UFUNCTION(BlueprintCallable) float GetBodyLength() const { return Mesh->Bounds.BoxExtent.X*2; }

	//������ ��� ������� - ��� ���������
	class UMyrBioStatUserWidget* StatsWidget();

	//������������ ��������������� - ��� ��, ����� ������������ ��������� ��� ������ ������� � �������������� ������� ������
	FColor ClassRelationToClass(AActor* obj);
	FColor ClassRelationToDeadClass(AMyrPhyCreature* obj);

	//��� �������, ����� ���������� � ����
	UFUNCTION(BlueprintCallable) FString GetCurrentAttackName() const { return !NoAttack() && GetCurrentAttackInfo() ? GetCurrentAttackInfo()->GetName() : FString(TEXT("NO")); }
	UFUNCTION(BlueprintCallable) FString GetCurrentSelfActionName() const { return (!NoSelfAction() && GetCurrentSelfActionInfo()) ? GetCurrentSelfActionInfo()->GetName() : FString(TEXT("NO")); }
	UFUNCTION(BlueprintCallable) FString GetCurrentRelaxActionName() const { return (!NoRelaxAction() && GetCurrentRelaxActionInfo()) ? GetCurrentRelaxActionInfo()->GetName() : FString(TEXT("NO")); }

};
