#include "MyrCreatureGenePool.h"

#include "Engine/SkeletalMeshSocket.h"			//для поиска сокетов-хваталок
#include "MyrCreatureAttackInfo.h"				//для сортировки атак

#include "PhysicsEngine/PhysicsConstraintTemplate.h"//для разбора списка физ-связок в регдолле

//==============================================================================================================
// после инициализации свойств
//==============================================================================================================
void UMyrCreatureGenePool::PostLoad()
{
	//переопределить свойства (возможно, незачем, ибо ассет сохраняется на диске)
	Super::PostLoad();
	AnalyseBodyParts();
	AnalyzeActions();
}

//==============================================================================================================
// реакция на изменения в редакторе 
//==============================================================================================================
#if WITH_EDITOR
void UMyrCreatureGenePool::PostEditChangeProperty (FPropertyChangedEvent & PropertyChangedEvent)
{
	//если изменили ссылку на скелетный меш, типичный для этого класса существ
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UMyrCreatureGenePool, SkeletalMesh)))
	{
		//переопределить свойства
		AnalyseBodyParts ();
	}
	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UMyrCreatureGenePool, Actions)))
	{
		//пересортировать атаки
		AnalyzeActions();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif


//==============================================================================================================
//найти среди сторон сустава нужную кость - если она там есть, выдать вторую кость
//==============================================================================================================
FName UMyrCreatureGenePool::FindConstraintOppositeBody(int CsIndex, FName FirstBoneName)
{
	auto PhA = SkeletalMesh->GetPhysicsAsset();
	auto& CI = PhA->ConstraintSetup[CsIndex]->DefaultInstance;

	if (CI.ConstraintBone1 == FirstBoneName)
		return CI.ConstraintBone2;
	else if(CI.ConstraintBone2 == FirstBoneName)
		return CI.ConstraintBone1;
	else return NAME_None;
}


//==============================================================================================================
// взять имя физического объема, который в physics asset висит по данному индексу. костыль №Х.
// кабы разработчики вообще не отменили наличие / редактируемость этого свойства
//==============================================================================================================
FName UMyrCreatureGenePool::GetBodyName(int i, UPhysicsAsset* Pha)
{
	if (i < 0) return NAME_None;

	//неужели нет стандартной функции, делающей эту рутину?
	const auto AggGeom = Pha->SkeletalBodySetups[i]->AggGeom;
	if (AggGeom.SphereElems.Num() > 0)	return AggGeom.SphereElems[0].GetName(); else
		if (AggGeom.SphylElems.Num() > 0)	return AggGeom.SphylElems[0].GetName(); else
			if (AggGeom.TaperedCapsuleElems.Num() > 0)	return AggGeom.TaperedCapsuleElems[0].GetName(); else
				if (AggGeom.BoxElems.Num() > 0)	return AggGeom.BoxElems[0].GetName();
				else return NAME_None;
}



//==============================================================================================================
//разобрать добавленные действия и построить карту для быстрого поиска
//==============================================================================================================
void UMyrCreatureGenePool::AnalyzeActions()
{
	for (int i = 0; i < Actions.Num(); i++)
		if (Actions[i])
		{
			ActionMap.Add(Actions[i]->Type, i);
			UE_LOG(LogTemp, Log, TEXT("GenePool %s, AnalyzeActions, add %s=%d, map num = %d"), *GetName(),
				*TXTENUM(ECreatureAction, Actions[i]->Type), i, ActionMap.Num());
		}
	
}

//==============================================================================================================
// проанализировать скелет (из общеигровой базы) и собрать инфу по количеству члеников для разных сегментов тела
// выполняется до начала игры, чтобы не выполнять рутину для каждого представителя этого скелета
//==============================================================================================================
void UMyrCreatureGenePool::AnalyseBodyParts ()
{
	// сущестование регдолла проверяется здесь
	if(!SkeletalMesh) return;
	auto PhA = SkeletalMesh->GetPhysicsAsset();
	if(!PhA) return;

	//сохранить точное количество члеников
	BodyBioData.SetNum (PhA->SkeletalBodySetups.Num());

	//дурацкий черезжопный код по итерации по всем костям
	//пока только для нахождения осей, а так здесь можно много чего ещё распозанавть
	for(int i=0; i<SkeletalMesh->GetSkeleton()->GetReferenceSkeleton().GetNum(); i++)
	{ 
		FName bn = SkeletalMesh->GetSkeleton()->GetReferenceSkeleton().GetBoneName(i);
		if (bn.ToString().Contains(TEXT("RArmAxis"))) Anatomy.RightArm.Machine.AxisReferenceBone = bn;
		if (bn.ToString().Contains(TEXT("LArmAxis"))) Anatomy.LeftArm.Machine.AxisReferenceBone = bn;
		if (bn.ToString().Contains(TEXT("RLegAxis"))) Anatomy.RightLeg.Machine.AxisReferenceBone = bn;
		if (bn.ToString().Contains(TEXT("LLegAxis"))) Anatomy.LeftLeg.Machine.AxisReferenceBone = bn;
	}

	//////////////////////////////////////////////////////////////////

	//сокеты в скелете
	const auto Sockets = SkeletalMesh->GetActiveSocketList();

	//очистить от привязок к физ-ассету, если вся эта инициализация делается повторно
	for (int i = 0; i < (int)ELimb::NOLIMB; i++) Anatomy.GetSegmentByIndex(i).Reset();

	//по всем членикам
	for (int i = 0; i < PhA->SkeletalBodySetups.Num(); i++)
	{
		if(GetBodyName(i, PhA).IsNone()) RootBodyIndex = i;

		//сопоставить имя членика классу сегмента тела (вызывается одна из этих процедур)
		FString nmn = GetBodyName (i, PhA).ToString();
		FName bn = PhA->SkeletalBodySetups[i]->BoneName;
		ELimb cuLimb = ELimb::NOLIMB;

		if (nmn.Contains(TEXT("pelvis")))	cuLimb = ELimb::PELVIS; else
		if (nmn.Contains(TEXT("lumbus")))	cuLimb = ELimb::LUMBUS; else
		if (nmn.Contains(TEXT("pectus")))	cuLimb = ELimb::PECTUS; else
		if (nmn.Contains(TEXT("thorax")))	cuLimb = ELimb::THORAX; else
		if (nmn.Contains(TEXT("tail")))		cuLimb = ELimb::TAIL; else
		if (nmn.Contains(TEXT("head")))		cuLimb = ELimb::HEAD; else
		if (nmn.Contains(TEXT("r_arm")))	cuLimb = ELimb::R_ARM; else
		if (nmn.Contains(TEXT("l_arm")))	cuLimb = ELimb::L_ARM; else
		if (nmn.Contains(TEXT("l_leg")))	cuLimb = ELimb::L_LEG; else
		if (nmn.Contains(TEXT("r_leg")))	cuLimb = ELimb::R_LEG; else
		if (nmn.Contains(TEXT("root")))
		{
			RootBodyIndex = i;
			continue;
		}
		else continue;

		//заполнение эталонного массива по членикам - этот массив будет просто копироваться на живые объекты
		BodyBioData[i].eLimb = cuLimb;
		BodyBioData[i].DamageWeight = 0.0f;
		auto& LimbGene = Anatomy.GetSegmentByIndex(cuLimb);
		//Anatomy.GetSegmentByIndex(cuLimb).nPhyBodies++;

		//для кончиков частей тела нужно поискать сокет для хвата или шага
		FName* SlotForGrabSocket = nullptr;
	
		//разбор подстроки с индексами члеников конткретной части тела
		auto resh = nmn.Find(TEXT("#"));
		if (resh!=INDEX_NONE)
		{
			if (nmn[resh + 1] == TEXT('m'))
			{
				//особый случай, расстояние не нужно
				BodyBioData[i].DistFromLeaf = 255;

				//записать номер, чтобы в меше по индексу было легко к нему применить силу
				Anatomy.GetSegmentByIndex(cuLimb).Machine.TipBodyIndex = i;

				//найт констрейнт
				FJsonSerializableArrayInt Cs;
				PhA->BodyFindConstraints(i, Cs);
				if (Cs.Num() == 1)
				{
					Anatomy.GetSegmentByIndex(cuLimb).Machine.TipConstraintIndex = Cs[0];
				}

				///////////////////////////////////////////////////////////////////////////////////////
				//почленно собрать некоторые рассыпанные данные
				switch (cuLimb)
				{
				case ELimb::PELVIS: PelvisBodyIndex = i; break; //это для нового существа нах избыточно
				case ELimb::THORAX: ThoraxBodyIndex = i; break;
				case ELimb::L_ARM:
				case ELimb::R_ARM:

					//радиус колеса, эмулирующего ступню - чтобы по одной только нормали и центру допетривать до координат точки контакта
					/*if (PhA->SkeletalBodySetups[i]->AggGeom.SphereElems.Num())
						ArmWheelRadius = PhA->SkeletalBodySetups[i]->AggGeom.SphereElems[0].Radius;*/
					break;

				case ELimb::L_LEG:
				case ELimb::R_LEG:

					//радиус колеса, эмулирующего ступню - чтобы по одной только нормали и центру допетривать до координат точки контакта
					/*if (PhA->SkeletalBodySetups[i]->AggGeom.SphereElems.Num())
						LegWheelRadius = PhA->SkeletalBodySetups[i]->AggGeom.SphereElems[0].Radius;*/
					break;
				}
			}
			else
			{
				//через строки вычислить индекс, который хотели внедрить в этот членик
				int ind = nmn[resh + 1] - TEXT('0');

				//ноль- кончик цепочки - здесь может быть сокет, отмечающий место ступни или зацепа
				if(ind == 0)
				for (auto Socket : Sockets)
					if (Socket->BoneName == bn)
						LimbGene.Flesh.GrabSocket = Socket->SocketName;

				//заполнить массив
				if (LimbGene.Flesh.BodyIndex.Num()<=ind)
					LimbGene.Flesh.BodyIndex.SetNum(ind+1);
				LimbGene.Flesh.BodyIndex[ind] = i;
			}
		}

		//звёздочкой помечены членики, которыев принципе могут быть боевыми и приносящими урон при касании
		if (nmn.Contains(TEXT("*")))
			BodyBioData[i].NeverDangerous = 0;

		UE_LOG(LogTemp, Log, TEXT("GenePool %s, Body %d (seg=%d)"), *GetName(), i, (int)cuLimb);

	}
	//за каким-то хером эта процедура на данном уровне оказывается не сделана, и многие функции не работают
	PhA->UpdateBodySetupIndexMap();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//по всем констрейнтам-суставам
	//никакой оптимизации, полный брутал N^3
	for (int i = 0; i < PhA->ConstraintSetup.Num(); i++)
	{
		int b1i = PhA->FindBodyIndex(PhA->ConstraintSetup[i]->DefaultInstance.ConstraintBone1);
		int b2i = PhA->FindBodyIndex(PhA->ConstraintSetup[i]->DefaultInstance.ConstraintBone2);
		if (b1i != -1 && b2i != -1)
		{
			auto B1Gene = Anatomy.GetSegmentByIndex(BodyBioData[b1i].eLimb);
			auto B2Gene = Anatomy.GetSegmentByIndex(BodyBioData[b2i].eLimb);

			//если в соответствующих частях тела выловленные стороны кострейнта занимают должность члеников мащины-поддержки
			if (B1Gene.Machine.TipBodyIndex == b1i && B2Gene.Machine.TipBodyIndex == b2i)
			{
				//соединяем индексы части тела так, чтоб помещались в одном байте, тем самым было просто их перебирать
				uint8 Comb = (uint8)BodyBioData[b1i].eLimb + ((uint8)BodyBioData[b2i].eLimb << 4);
				switch (Comb)
				{
				case BI_QUA(ELimb::PELVIS, ELimb::LUMBUS):
				case BI_QUA(ELimb::LUMBUS, ELimb::PELVIS):
					Anatomy.Lumbus.Machine.TipConstraintIndex = i;
					break;

				case BI_QUA(ELimb::LUMBUS, ELimb::PECTUS):
				case BI_QUA(ELimb::PECTUS, ELimb::LUMBUS):
					Anatomy.Pectus.Machine.TipConstraintIndex = i;
					break;

				case BI_QUA(ELimb::PECTUS, ELimb::THORAX):
				case BI_QUA(ELimb::THORAX, ELimb::PECTUS):
					Anatomy.Thorax.Machine.TipConstraintIndex = i;
					break;

				case BI_QUA(ELimb::PECTUS, ELimb::HEAD):
				case BI_QUA(ELimb::HEAD, ELimb::PECTUS):
					Anatomy.Head.Machine.TipConstraintIndex = i;
					break;

				case BI_QUA(ELimb::THORAX, ELimb::R_ARM):
				case BI_QUA(ELimb::R_ARM, ELimb::THORAX):
					Anatomy.RightArm.Machine.TipConstraintIndex = i;
					break;

				case BI_QUA(ELimb::THORAX, ELimb::L_ARM):
				case BI_QUA(ELimb::L_ARM, ELimb::THORAX):
					Anatomy.LeftArm.Machine.TipConstraintIndex = i;
					break;

				case BI_QUA(ELimb::PELVIS, ELimb::L_LEG):
				case BI_QUA(ELimb::L_LEG, ELimb::PELVIS):
					Anatomy.LeftLeg.Machine.TipConstraintIndex = i;
					break;

				case BI_QUA(ELimb::PELVIS, ELimb::R_LEG):
				case BI_QUA(ELimb::R_LEG, ELimb::PELVIS):
					Anatomy.RightLeg.Machine.TipConstraintIndex = i;
					break;

				case BI_QUA(ELimb::LUMBUS, ELimb::TAIL):
				case BI_QUA(ELimb::TAIL, ELimb::LUMBUS):
				case BI_QUA(ELimb::PELVIS, ELimb::TAIL):
				case BI_QUA(ELimb::TAIL, ELimb::PELVIS):
					Anatomy.Tail.Machine.TipConstraintIndex = i;
					break;
				}
			}
		}

	}

	UE_LOG(LogTemp, Log, TEXT("GenePool %s, constraints:%d, bones %d "), *GetName(), PhA->ConstraintSetup.Num(), BodyBioData.Num());

}







