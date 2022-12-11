// Fill out your copyright notice in the Description page of Project Settings.

#include "MyrAnimNotify.h"
#include "../../Control/MyrAI.h"

#include "../MyrPhyCreature.h"
#include "../MyrPhyCreatureMesh.h"
#include "MyrPhyCreatureAnimInst.h"

#include "Kismet/GameplayStatics.h"						// для вызова SpawnEmitterAtLocation
#include "Particles/ParticleSystemComponent.h"			// для генерации всплесков шерсти при ударах


//==============================================================================================================
//украшательство, чтобы при добавлении были соответствующие цвета
//==============================================================================================================
#if WITH_EDITOR
void UMyrAnimNotify::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	switch (Cause)
	{
	//это уже не нужно, поскольку релакс теперь такое же самодействие как остальные
	case EMyrNotifyCause::RelaxStart:NotifyColor = FColor(255, 100, 30); break;
	case EMyrNotifyCause::RelaxStop:NotifyColor = FColor(255, 100, 30); break;

	case EMyrNotifyCause::StepFrontLeft:	NotifyColor = FColor(255, 100, 30); break;
	case EMyrNotifyCause::StepFrontRight:	NotifyColor = FColor(255, 150, 10); break;
	case EMyrNotifyCause::StepBackLeft:		NotifyColor = FColor(255, 100, 30); break;
	case EMyrNotifyCause::StepBackRight:	NotifyColor = FColor(255, 150, 10); break;
	case EMyrNotifyCause::StepBackBoth:		NotifyColor = FColor(255, 150, 60); break;
	case EMyrNotifyCause::StepFrontBoth:	NotifyColor = FColor(255, 100, 60); break;

	case EMyrNotifyCause::RaiseFrontLeft:	NotifyColor = FColor(100, 255, 30); break;
	case EMyrNotifyCause::RaiseFrontRight:	NotifyColor = FColor(150, 255, 10); break;
	case EMyrNotifyCause::RaiseBackLeft:	NotifyColor = FColor(100, 255, 30); break;
	case EMyrNotifyCause::RaiseBackRight:	NotifyColor = FColor(150, 255, 10); break;
	case EMyrNotifyCause::RaiseBackBoth:	NotifyColor = FColor(150, 255, 60); break;
	case EMyrNotifyCause::RaiseFrontBoth:	NotifyColor = FColor(100, 255, 60); break;

	case EMyrNotifyCause::AttackBegin:		NotifyColor = FColor(255, 255, 60); break;	// самое начало ролика
	case EMyrNotifyCause::AttackGetReady:	NotifyColor = FColor(255, 255, 255); break;	// момент перед началом опасной фазы 
	case EMyrNotifyCause::AttackGiveUp:		NotifyColor = FColor(255, 255, 120); break;	// атака очень медленно идёт вспять и достигает этой точки выдыхания
	case EMyrNotifyCause::AttackComplete:	NotifyColor = FColor(255, 255, 160); break;	// момент конца опасной фазы
	case EMyrNotifyCause::AttackEnd:		NotifyColor = FColor(200, 255, 255); break;	// конец ролика

	case EMyrNotifyCause::NewPhase:			NotifyColor = FColor(100, 100, 200); break;	// сменить фазу самодействия
	case EMyrNotifyCause::SelfActionEnd:	NotifyColor = FColor(100, 100, 100); break;	// конец ролика
	}
}
#endif

//==============================================================================================================
// событие достижения закладки в анимационном ролике
//==============================================================================================================
void UMyrAnimNotify::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Anim)
{
	//получить текущую сборку анимации
	UMyrPhyCreatureAnimInst* PhyAnimInst = Cast<UMyrPhyCreatureAnimInst>(Mesh->GetAnimInstance());
	AMyrPhyCreature* MyrPhyCreature = Cast<AMyrPhyCreature>(Mesh->GetOwner());
	if (!MyrPhyCreature || !PhyAnimInst) return;
	//int AttackSlot;

	//не ображая внимания на режимы, рассматривает только то, что указано на самой закладке
	switch (Cause)
	{


	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	//здесь анимация подразумевает касание земли ногами
	case EMyrNotifyCause::StepFrontLeft:  MyrPhyCreature->MakeStep(ELimb::L_ARM, false);	return;
	case EMyrNotifyCause::StepFrontRight: MyrPhyCreature->MakeStep(ELimb::R_ARM, false);	return;
	case EMyrNotifyCause::StepBackLeft:	  MyrPhyCreature->MakeStep(ELimb::L_LEG, false);	return;
	case EMyrNotifyCause::StepBackRight:  MyrPhyCreature->MakeStep(ELimb::R_LEG, false);	return;

	case EMyrNotifyCause::StepBackBoth:
		MyrPhyCreature->MakeStep(ELimb::R_LEG, false);
		MyrPhyCreature->MakeStep(ELimb::L_LEG, false);
		return;

	case EMyrNotifyCause::StepFrontBoth:
		MyrPhyCreature->MakeStep(ELimb::R_ARM, false);
		MyrPhyCreature->MakeStep(ELimb::L_ARM, false);
		return;

	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	//здесь анимация подразумевает касание земли ногами
	case EMyrNotifyCause::RaiseFrontLeft:  MyrPhyCreature->MakeStep(ELimb::L_ARM, true);	return;
	case EMyrNotifyCause::RaiseFrontRight: MyrPhyCreature->MakeStep(ELimb::R_ARM, true);	return;
	case EMyrNotifyCause::RaiseBackLeft:   MyrPhyCreature->MakeStep(ELimb::L_LEG, true);	return;
	case EMyrNotifyCause::RaiseBackRight:  MyrPhyCreature->MakeStep(ELimb::R_LEG, true);	return;

	case EMyrNotifyCause::RaiseBackBoth:
		MyrPhyCreature->MakeStep(ELimb::R_LEG, true);
		MyrPhyCreature->MakeStep(ELimb::L_LEG, true);
		return;

	case EMyrNotifyCause::RaiseFrontBoth:
		MyrPhyCreature->MakeStep(ELimb::R_ARM, true);
		MyrPhyCreature->MakeStep(ELimb::L_ARM, true);
		return;

	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	/////////////////////////////////////////////////////////////////////////////////////////////////
	//начало анимации атаки, в самом начале ролика
	case EMyrNotifyCause::AttackBegin:

		//если в режиме отступления дошли до начала ролика - завершить атаку в этом слоте (
		if (MyrPhyCreature->CurrentAttackPhase == EActionPhase::DESCEND)
			MyrPhyCreature->AttackEnd();
		break;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	//достижение точки готовности (много разных вариантов все под капотом AttackGetReady)
	case EMyrNotifyCause::AttackGetReady:
		MyrPhyCreature->AttackNotifyGetReady();
		break;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	//достижение точки выгорания при ожидании момента атаки
	case EMyrNotifyCause::AttackGiveUp:
		MyrPhyCreature->AttackNotifyGiveUp();
		break;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	//атака прошла удачно, но в данный момент активная фаза окончилась и наступила фаза возврата к обычной позе
	case EMyrNotifyCause::AttackComplete:
		MyrPhyCreature->AttackNotifyFinish();
		break;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	//конец анимации атаки
	case EMyrNotifyCause::AttackEnd:
		MyrPhyCreature->AttackNotifyEnd();
		break;


	//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
	/////////////////////////////////////////////////////////////////////////////////////////////////
	//смена фазы самодействия
	//теперь объект самодействие используется как для собственно самодействий, так и для действий-релаксов,
	//требуется отдельная проверка, к кому из них относится анимация, где была поймана эта закладка
	case EMyrNotifyCause::NewPhase:

		//для случая простого самодействия - смена фазы ничего не значит, кроме смены физики для тела
		if (MyrPhyCreature->CurrentSelfAction < 255)
		{
			if (MyrPhyCreature->GetSelfAction()->Motion == Anim)
				MyrPhyCreature->SelfActionNewPhase();
		}

		//для случая анимации перехода в отдых - смена фазы означает переход в стабильный отдых
		if (MyrPhyCreature->CurrentRelaxAction < 255)
		{
			if (MyrPhyCreature->GetRelaxAction()->Motion == Anim)
				MyrPhyCreature->RelaxActionReachPeace();
		}
		break;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	//конец анимации действия на себя
	//теперь объект самодействие используется как для собственно самодействий, так и для действий-релаксов,
	//требуется отдельная проверка, к кому из них относится анимация, где была поймана эта закладка
	case EMyrNotifyCause::SelfActionEnd:

		//для случая простого самодействия
		if (MyrPhyCreature->CurrentSelfAction < 255)
		{	
			if (MyrPhyCreature->GetSelfAction()->Motion == Anim)
				MyrPhyCreature->SelfActionCease();
			else for(auto A : MyrPhyCreature->GetSelfAction()->AlternativeRandomMotions)
				if (A == Anim)
					MyrPhyCreature->SelfActionCease();
			if(MyrPhyCreature->CurrentSelfAction != 255)
				UE_LOG(LogTemp, Error, TEXT("WTF Self Action Never Ends!!11"));
		}
		else UE_LOG(LogTemp, Error, TEXT("WTF Self Action Anim Ended without beginning!!11")); 

		//для случая анимации перехода в отдых - смена фазы означает переход в стабильный отдых
		if (MyrPhyCreature->CurrentRelaxAction < 255)
		{	//if (MyrPhyCreature->GetRelaxAction()->Motion == Anim)
				MyrPhyCreature->RelaxActionReachEnd();
		}
		break;
	}
	UE_LOG(LogMyrPhyCreature, Log, TEXT("notify anim:%s cause:%s attack:%s currentstate:%s"),
		*Anim->GetName(),
		*TXTENUM(EMyrNotifyCause, Cause),
		*MyrPhyCreature->GetCurrentAttackName(),
		*TXTENUM(EActionPhase, MyrPhyCreature->CurrentAttackPhase));

}



FString UMyrAnimNotify::GetNotifyName_Implementation() const
{
	return FindObject<UEnum>(ANY_PACKAGE, TEXT("EMyrNotifyCause"))->GetNameStringByValue((int)Cause);
}



void UMyrAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{

}
void UMyrAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{

}
void UMyrAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{

}

