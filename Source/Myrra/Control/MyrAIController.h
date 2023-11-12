// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "AIController.h"
#include "AIModule/Classes/Perception/AIPerceptionTypes.h"
#include "MyrAIController.generated.h"

//свои дебаг записи
DECLARE_LOG_CATEGORY_EXTERN(LogMyrAIC, Log, All);

//рисовалки отладжочных линий
#if WITH_EDITOR
#define LINE(C, A, AB) me()->Line(ELimbDebug::##C, A, AB)
#define LINEWT(C, A, AB,W,T) me()->Line(ELimbDebug::##C, A, AB,W,T)
#define LINEWTC(C, A, AB,W,T, tint) me()->Linet(ELimbDebug::##C, A, AB,tint, W,T)
#define LINEW(C, A, AB, W) me()->Line(ELimbDebug::##C, A, AB, W)
#define LINELIMB(C, L, AB) me()->Line(ELimbDebug::##C, me()->GetMachineBody(L)->GetCOMPosition(), AB)
#define LINELIMBW(C, L, AB, W) me()->Line(ELimbDebug::##C, me()->GetMachineBody(L)->GetCOMPosition(), AB, W)
#define LINELIMBWT(C, L, AB, W, T) me()->Line(ELimbDebug::##C, me()->GetMachineBody(L)->GetCOMPosition(), AB, W, T)
#define LINELIMBWTC(C, L, AB, W, T, t) me()->Linet(ELimbDebug::##C, me()->GetMachineBody(L)->GetCOMPosition(), AB, t, W, T)
#define LDBG(C) me()->IsDebugged(C)
#else
#define LDBG(C) false
#define LINE(C, A, AB)
#define LINELIMB(C, L, AB)
#define LINELIMBW(C, L, AB, W)
#define LINELIMBWT(C, L, AB, W, T) 
#define LINELIMBWTC(C, L, AB, W, T, t)
#define LINEW(C, A, AB, W)
#endif

//###################################################################################################################
// ◘ динамическая цель, надстройка над самыми весомыми из гештальтов для трассировки пути к/от
//###################################################################################################################
USTRUCT(BlueprintType) struct MYRRA_API FAIGoal
{
	GENERATED_USTRUCT_BODY()
	FGestalt* pGestalt = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AnalogGain = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AnalogStealth = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector3f MoveToDir = FVector3f::ForwardVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FirstDist = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ERouteResult WhatToDo = ERouteResult::NoGoal;

	//перезагрузить перед внесением нового гештальта, нужно при постоянных переменных
	void SetAsVoid()
	{	pGestalt = nullptr;	AnalogGain = 0; AnalogStealth = 0; }

	//занять новым гештальтом
	void Seize(FGestalt* nG, float nAG)
	{	pGestalt = nG;
		AnalogGain = nAG;
	}
	void SeizeIf(FGestalt& G, float Gain, bool MaxLure)
	{	if((AnalogGain < Gain)==MaxLure || AnalogGain == 0) Seize(&G, Gain);	}

	//передать из другой цели уже посчитанную дорогу
	void TransferRoute(FAIGoal& O, ERouteResult ToReplace)
	{	MoveToDir = O.MoveToDir;
		FirstDist = O.FirstDist;
		WhatToDo = ToReplace;
	}

	FVector3f LookAtDir() const {	return pGestalt->Location.DecodeDir();	}
	AActor* Owner() {	return pGestalt->Obj->GetOwner();	}
	float Dist() const { return pGestalt->Location.DecodeDist();	}
	float UGain() const { return FMath::Abs(AnalogGain); }
	float clUGain() const { return FMath::Min(UGain(), 1.0f); }

	FVector3f CombineMove(FAIGoal& O) { return (MoveToDir*UGain() + O.MoveToDir*O.UGain()).GetSafeNormal(); }

};


//###################################################################################################################
// ◘ искусственный интеллект
//###################################################################################################################
UCLASS() class MYRRA_API AMyrAIController : public AAIController
{
	GENERATED_BODY()

	//из стандартных чувств будет использоваться только слух - он прост и всеобъемлющ
	//а у зрения многое под капотом, его проще пользовательскими Trace в нужное место с нужной частотой
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true")) class UAISenseConfig_Hearing* ConfigHearing = nullptr;

	//самое главное - генерирует сообщения при замечании и упускании актера по зрению, слуху и т.п.
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true")) class UAIPerceptionComponent* AIPerception = nullptr;

//свойства
public:

	// последовательно увеличивается каждый такт, для генерации циклов и ультра редких событий
	uint32 Counter = 0;		

	//действие, которое можно сделать в данный такт жизни, дабы перебором всех не нагружать каждый такт
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 ActionToTryNow = 0;

	//память об окружающих объектах в сцене (добавляются по встрече) и глобальных знакомых
	//сохраняется и загружается в виде FName, должны быть загружены на карту(уровень) до загрузки сохранения, чтобы подцепить имена
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSet<FGestalt> Memory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FUrGestalt Mich;

	//душа конкректного существа набор сложных рефлексов на комбинации раздражителей, может прокачиваться, сохраняется в сохранении
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	TSet<FReflex> ComplexEmotions;

	//когда происходит акт атаки, сюда пишается тело конкретной жертвы, получившей урон, чтобы все могли найти и агенса и пациенса одновременно
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	USceneComponent* ExactVictim = nullptr;

	//уровень контроля ИИ над телом при наличии демона
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AIRuleWeight;

	//внешняя эмоция, по которой строится мимика, жесты, атаки, формируется из текущих целей
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FPathia IntegralEmotion;

	//запас эмоциональных сил, тратится на простое переживание эмоций выше среднего
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float EmoResource = 0;

	//запас эмоциональных сил, тратится на простое переживание эмоций выше среднего
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EmoChangeAccum = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EmoBalanceAccum = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EmoWorkAccum = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EmoEnergyAccum = 0;


	//эмоциональный опыт - позволяет изменять эмоции
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 EmoExperience = 0;

	//новая сборка со всеми параметрами для движения подопечного существа
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FCreatureDrive Drive;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) ERouteResult LastRouteResult;

//стандартные функции, переопределение
public:

	//конструктор
	AMyrAIController(const FObjectInitializer &ObjInit);

	//Called when the AI Controller possesses a Pawn
	virtual void OnPossess(APawn* InPawn) override;

	//начать
	virtual void BeginPlay() override;

	// покадровая, губящая производительность, рутина
	virtual void Tick(float DeltaTime) override;

	//нужно, когда существо исчезает в норе или на дистанции
	virtual void OnUnPossess() override;

	//событие обновления восприятия - по видимому, единичное событие с определенным, одним из управляемых данным контроллером актёров
	UFUNCTION() void OnTargetPerceptionUpdated (AActor* Actor, FAIStimulus Stimulus);

//свои функции
public:

	//если мы прокачались в чувствительности, изменить радиусы зрения, слуха...
	void UpdateSenses();

	//рутинная обработка в тике одного конкретного гештальта
	void UpdateEmotions(FUrGestalt& Gestalt, float Weight, int32 OldInfluences);

	//посмотреть на объект, определить его видимость
	bool See(FHitResult& Hit, FVector ExplicitPos, USceneComponent* NeededObj = nullptr);

	//просто обойти препятствие, вычислив его размеры
	ERouteResult SimpleWalkAroundObstacle(FVector3f TempLookDir, FHitResult* Hit, FAIGoal& G);

	//различные варианты движения к найденной цели
	ERouteResult SimpleMoveToGoal(FAIGoal& G);

	//бежать прочь от цели, без конкретной ориентации, рассчитать вектор бегства с учётом второй цели
	ERouteResult FreeMoveAway(FAIGoal& G, FAIGoal* Ilflu = nullptr);

	//рассчитать желаемый уровень скрытности для этой цели
	void CalcStealth(FAIGoal& Goal);

	//для живой цели сгенерировать влияние прицеливания... неправильно, будет гонка влиятелей
	void NotifyYouAreAim(class AMyrPhyCreature* Myr, bool Set)
	{	if (auto R = MeInYourMemory(Myr))
			if (R->PerceptAmount() > 5)
				R->Influences.Set(EInfluWhy::Aim, Set);
	}

	//уведомить о нашей атаке ИИ-систему, чтобы другие участники среагирвали (вызывается, например, при нажатии)
	void LetOthersNotice(EHowSensed Event, float Strength = 1.0f);
	void NotifyNoise(FVector Location, float Strength = 1.0);

	//попробовать в отношении текущей цели какое-нибудь действие, продвинуть счетчик
	void ProposeAction(FAIGoal &Goal);

	//найти еомплексный рефлекс наиболее близкий к введенному по 
	FReflex* FindClosest(int32 InfluBits) const;

	//зарегистрировать численную форму изменения эмоций в результате единичного воздействия раздражителя
	void RegModEmotion(int Change) { EmoBalanceAccum += Change; EmoChangeAccum += FMath::Abs(Change); }

//возвр
public:

	//управляемое существ в приведенном типе
	class AMyrPhyCreature* ME() { return (AMyrPhyCreature*)GetPawn(); }
	class AMyrPhyCreature* me() const { return (AMyrPhyCreature*)GetPawn(); }
	class USceneComponent* meC() { return GetPawn()->GetRootComponent(); }
	class USceneComponent* meC() const { return GetPawn()->GetRootComponent(); }

	//если этот образ - существо, то оно, возможно, имеет нас в памяти
	FGestalt* MeInYourMemory(AMyrPhyCreature* Myr);

	//случайное в диапазоне 0 - 1
	float fRand() const { return FUrGestalt::RandVar / RAND_MAX; }
	bool ChanceRandom(uint8 Chance) { return (uint8)(FUrGestalt::RandVar & 255) <= Chance; }
	bool Period(uint8 Lng, float Chance) { return ((Counter & Lng) <= Lng*Chance); }
	bool Period(uint8 Lng) { return ((Counter & Lng) <= Lng/2u); }

	UFUNCTION(BlueprintCallable) float GetRage() const { return IntegralEmotion.fRage(); }
	UFUNCTION(BlueprintCallable) float GetLove() const { return IntegralEmotion.fLove(); }
	UFUNCTION(BlueprintCallable) float GetFear() const { return IntegralEmotion.fFear(); }

	UFUNCTION(BlueprintCallable) FGestalt GetGestalt(USceneComponent* KeyObj) const { auto A = Memory.Find(KeyObj); if (A) return *A; else return FGestalt(); }


	//доступ к правящей верхушке системы
	UFUNCTION(BlueprintCallable) class UMyrraGameInstance* GetMyrGameInst() const { return (UMyrraGameInstance*)GetGameInstance(); }
	UFUNCTION(BlueprintCallable) class AMyrraGameModeBase* GetMyrGameMode() const { return (AMyrraGameModeBase*)GetWorld()->GetAuthGameMode(); }

	UFUNCTION(BlueprintCallable) FGestalt GetMichAlsGestalt() const { FGestalt R = (FGestalt)Mich; R.Obj = meC(); return R; }
	UFUNCTION(BlueprintCallable) FPathia GetMeEmotion() const { return Mich.Emotion; }

	//найти точный вектор от нас на данный объект
	FVector3f GetMeToYouVector(USceneComponent* C);

};
