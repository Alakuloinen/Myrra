// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Myrra.h"
#include "../AssetStructures/MyrDendroInfo.h"
#include "MyrDendroActor.generated.h"



UCLASS() class MYRRA_API AMyrDendroActor : public AActor
{
	GENERATED_BODY()

	//основное (в случае мелких и уникальных) и/или самое дальнее представление дерева (в случае модульных)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))	class UStaticMeshComponent* BaseForm;

public:

	//архетип дерева, определяет какие секции создавать динамически
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	UMyrDendroInfo* Archetype = nullptr;

public:	

	// Sets default values for this actor's properties
	AMyrDendroActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
