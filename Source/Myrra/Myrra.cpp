// Fill out your copyright notice in the Description page of Project Settings.

#include "Myrra.h"
#include "Modules/ModuleManager.h"
#include "Artefact/MyrArtefact.h"
#include "Creature/MyrPhyCreature.h"
#include "PhysicalMaterials/PhysicalMaterial.h"			// для выкорчевывания материала поверхности пола

//==============================================================================================================
//оси соответствующие перечислителям EMyrAxis - для быстроты константной
//==============================================================================================================
FVector3f MyrAxes[8] =
{
	FVector3f(0,0,0),
	FVector3f(1,0,0),
	FVector3f(0,1,0),
	FVector3f(0,0,1),
	FVector3f(-1,-1,-1),
	FVector3f(-1,0,0),
	FVector3f(0,-1,0),
	FVector3f(0,0,-1),
};

int32 FUrGestalt::RandVar = 0;

//здесь, видимо, происходит что-то важное, сакральное и судьбоносное
IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, Myrra, "Myrra" );


//==============================================================================================================
//выдать данные о съедобности объекта (заместо пиздохуёбанных в жопу интерфейсов)
//глобальная функция, потому что съедобными могут быть разные классы
//==============================================================================================================
FDigestiveEffects* GetDigestiveEffects(AActor* Obj)
{
	if (auto A = Cast<AMyrArtefact>(Obj))
	{	return &A->EffectsWhileEaten;
	}
	else if(auto C = Cast<AMyrPhyCreature>(Obj))
		return &C->GetGenePool()->DigestiveEffects;
	else return nullptr;
}

//==============================================================================================================
//аналогично выдать человеческое локализуемое имя
//==============================================================================================================
FText GetHumanReadableName(AActor* Whatever)
{
	if (auto A = Cast<AMyrArtefact>(Whatever))
		return A->HumanReadableName;
	else if(auto C = Cast<AMyrPhyCreature>(Whatever))
		return A->HumanReadableName;
	return FText();
}


//==============================================================================================================
// инициализировать сборку опоры из структуры хит
//==============================================================================================================
bool FFloor::FromHit(const FHitResult& Hit)
{
	if (!Hit.Component.IsValid() || !Hit.PhysMaterial.IsValid())
	{
		Erase();
		return false;
	}
	Body = Hit.GetComponent()->GetBodyInstance(Hit.BoneName);
	if (!Body) return false;
	Surface = (EMyrSurface)Hit.PhysMaterial->SurfaceType.GetValue();
	Normal = (FVector3f)Hit.ImpactNormal;
	return true;
}
