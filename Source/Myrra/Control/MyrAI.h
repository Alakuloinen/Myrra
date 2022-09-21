// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "AIController.h"
#include "AIModule/Classes/Perception/AIPerceptionTypes.h"
#include "MyrAI.generated.h"

//свои дебаг записи
DECLARE_LOG_CATEGORY_EXTERN(LogMyrAI, Log, All);

//###################################################################################################################
// ◘ дополнительный тик
//###################################################################################################################
//возможно, сделать с шаблоном
USTRUCT(BlueprintType) struct MYRRA_API FEmoStimulEntry
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EMyrLogicEvent Why;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEmoStimulus How;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TWeakObjectPtr<UObject> Whence;
};

//###################################################################################################################
// ◘ искусственный интеллект
//###################################################################################################################
UCLASS() class MYRRA_API AMyrAI : public AAIController
{
	GENERATED_BODY()
	friend class AMyrPhyCreature;

	//из стандартных чувств будет использоваться только слух - он прост и всеобъемлющ
	//а у зрения многое под капотом, его проще пользовательскими Trace в нужное место с нужной частотой
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true")) class UAISenseConfig_Hearing* ConfigHearing = nullptr;

	//самое главное - генерирует сообщения при замечании и упускании актера по зрению, слуху и т.п.
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true")) class UAIPerceptionComponent* AIPerception = nullptr;
		
	// последовательно увеличивается каждый такт, для генерации циклов и ультра редких событий
	uint32 Counter = 0;		

//свойства
public:

	static uint32 RandVar;	// перетасывается каждый такт

	//ПАМЯТЬ - множество известных/знакомых объектов и "архетипические" отношения к ним, накапливается с первого обнаружения до полного удаления объекта //см. GetKnownObj(int i)
	//недостаток - если объект на другом уровне, то он будет удаляться отсюда без следов с выгрузкой уровня
	//проще, наверно, держать всех самых важных персонажей постоянно в оперативной памяти, тогда этот контейнер всегда валиден, пока жив носитель 
	//другой недостаток - указатель нельзя архивировать. Поэтому для сохранений в файл придётся лить не сам контейнер, а создавать подобный, но со строками имен вместо указателей.
	//см. MyrraSaveGame
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<UPrimitiveComponent*, FGestaltRelation> Memory;

	//память о предыдуще сделанных действиях - точно прошедшее только последнее, остальные перемешаны, но позволяют оценить частотность разных акций
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	FEmotionMemory EventMemory;

	//◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘
	//новая форма учёта эмоциональных воздействий, просто добавляется и оставляет пустую ячейку, когда исчезает
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEmoMemory EmoMemory;


	//оперативные цели (ложатся как попало, старшинство варьируется индексами)
	FGoal Goals[2];	/////////////////////////////////////////////////////////
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	uint8 PrimaryGoal = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	uint8 SecondaryGoal = 1;

	//принятое решение по движению - возможно нах не нужно вне отладки
	//а лучше сделать на основе этой переменной конечный автомат
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) uint8 MoveBranch;

	//уровень воли или влияения ИИ на тело, при подсоединении игрока должен падать в идеале до нуля
	//если не ноль, ИИ всячески геймплейно мешает игроку
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	float AIRuleWeight = 1.0;

	//уровень уверенности, что плохие нас видят
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	float Paranoia = 0.0;

	//высота - для летающих и падающих, через трассировку, чтобы регулировать высоту полёта
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	float FlyHeight = 0.0;

	//новая сборка со всеми параметрами для движения подопечного существа
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	FCreatureDrive Drive;

	//интегральная эмоция существа, компбинация из отношения к целям и отношения к себе (и, видимо, каких-то внешних факторов)
	//переводится в базис, в котором любовь^2 = мощность - страх^2 - гнев^2, мозность - норма вектора 3д, таким образом
	//в анимации страх и гнев = 2д карта сильных эмоций, в середине которой - любовь
	//покой же анимируется смесью этой карты с нейтральной спокойной позой, с коэффициентом Power
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float IntegralEmotionRage;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float IntegralEmotionFear;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float IntegralEmotionPower;


//стандартные функции, переопределение
public:

	//конструктор
	AMyrAI(const FObjectInitializer &ObjInit);

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

	//вспомнить/запомнить встреченного актера в долговременной памяти
	FGestaltRelation* Remember (UPrimitiveComponent* NewObj);

	//если мы прокачались в чувствительности, изменить радиусы зрения, слуха...
	void UpdateSenses();

	//уведомить о нашей атаке ИИ-систему, чтобы другие участники среагирвали (вызывается, например, при нажатии)
	void LetOthersNotice(EHowSensed Event, float Strength = 1.0f);
	void NotifyNoise(FVector Location, float Strength = 1.0);

	//забрать образ в текущую операционную часть ИИ - и вернуть указатель на ячейку, где вся подноготная варится
	EGoalAcceptResult Notice(UPrimitiveComponent* NewGoalObj, EHowSensed HowSensed, float Strength, FGoal** WhatNoticed);

	//забыть из операционной памяти, пересохранив в вдолговременную память
	void Forget(FGoal& Goal, FGestaltRelation* Gestalt);

	//в ответ на приём стимула, внести в досье на цель новые данные, между ними вызывается MeasureGoal() для полноты картины
	void PraePerceive(FGoal& Goal, EHowSensed HowSensed, float Strength, UPrimitiveComponent* Obj, FGestaltRelation* Gestalt);	
	void PostPerceive(FGoal& Goal, EHowSensed HowSensed, float Strength, FGestaltRelation* Gestalt, EGoalAcceptResult Result);

	//посчитать метрики, видимость, вес цели - вызывается либо рутинно в тике, либо в акте перцепции, если видим цель в первый раз
	void MeasureGoal(FGoal& Goal, FGestaltRelation* NeuGestalt, float DeltaTime, bool Sole, EHowSensed HowSensed, FHitResult* OutVisTrace = nullptr);

	//выполнить трассировку к цели чтобы явно пересчитать видимость цели или второстепенного объекта
	bool TraceForVisibility (UPrimitiveComponent *Obj, FGestaltRelation* Gestalt, FVector ExactRadius, FHitResult* Hit);

	//получить (из кривых от расстояния до цели) аналоговые прикидки для тяги (вызращается) и уровней желания спрятаться и развернуть тело 
	float CalcAdditiveGain(FGoal& Goal, float &StealthAmount, float &FacingAmount);


	//сложная трассировка (цель получает значение MoveToDir и, возможно, ClosestPoint)
	ERouteResult Route(FGoal& Goal, FGestaltRelation* Gestalt, FHitResult* SawMain);

	//сложное вычисление, что делать с препятствием
	ERouteResult DecideOnObstacle(FVector3f Dir, FGoal& Goal, FGestaltRelation* Gestalt, FHitResult* Hit, FVector3f& OutMoveDir);

	//просто обойти препятствие, вычислив его размеры
	ERouteResult SimpleWalkAroundObstacle(FVector3f Dir, FHitResult* Hit, FVector3f& OutMoveDir);


	//обновить видимость и слышимость при поступлении сигнала (гештальт как параметр, чтобы лишний раз не искать в TMap)
	float RecalcGoalVisibility(FGoal& Goal, AMyrPhyCreature* MyrAim, const FGestaltRelation* Gestalt);	
	float RecalcGoalAudibility(FGoal& Goal, const float NewStrength, const FGestaltRelation* Gestalt);	
	float RecalcGoalWeight(const FGoal& Goal) const;
	void RecalcIntegralEmotion();
	UFUNCTION(BlueprintCallable) FLinearColor GetIntegralEmotion() const;

	//притушить или загасить предывдущеезначение параметров моторики, перед вводом новых
	void CleanMotionParameters(float C) { Drive.ActDir*=C; Drive.MoveDir*=C; Drive.Gain*=C; }

//метаболизм эмоциональных откликов на события
public:

	//новая функция по добавлению эмоционального стимула 
	void AddEmotionStimulus(FEmoStimulEntry New);

	//зарегистрировать событие, влияющее на жизнь существа - в рамках ИИ
	void RecordMyrLogicEvent(EMyrLogicEvent Event, float Mult, UPrimitiveComponent* Obj, FMyrLogicEventData* EventInfo);

	//уяснить для себя и запомнить событие, когда цель становится недоступной или наоборот доступной
	void MyrLogicChangeReachable(FGoal& Goal, FGestaltRelation* Gestalt, bool Reachable);

	//рутинный процесс варения данной памяти эмоциональных откликов
	void MyrLogicTick(FEmotionMemory& Memory, bool ForGoal);

//обработка атак
public:

	//проанализировать соотношение нас и агрессора и выбрать подходящий эмоциональный отклик и геймлпейное событие
	void DesideHowToReactOnAggression(float Amount, AMyrPhyCreature* Motherfucker);
	void DesideHowToReactOnGrace(float Amount, AMyrPhyCreature* Sweetheart);

	//реакция на чью-то атаку, "услышанную" в радиусе слуха - недописано, вызывается в PostPerceive, после MeasureGoal
	EAttackEscapeResult BewareAttack (UPrimitiveComponent* Suspect, FGoal* GoalIfPresent, FGestaltRelation* Gestalt, bool Strike = false);

	//непосредственно запустить нашу атаку в ответ на чужую атаку
	EAttackEscapeResult CounterStrike(float Danger, AMyrPhyCreature* You, FVector3f NewAttackDir);

//прочее
public:

	//рассчитать микс из воздействий звух управляющих существом сущностей
	FCreatureDrive MixWithOtherDrive(FCreatureDrive* Other);

	//автонацеливание на найденного противника - вызывается из AMyrPhyCreature, управляемого игроком
	bool AimBetter (FVector3f& OutAttackDir, float Accuracy = 0.6);

	//действия, если актор исчез - удалить из долговременной памяти, и из целей
	void SeeThatObjectDisappeared(UPrimitiveComponent* ExactObj);

	//запустить самоуничтожение, если игрок далеко  отошел от этого существа
	bool AttemptSuicideAsPlayerGone();

//возвращалки
public:

	//раздобыть данные по конкретному эмоуиональному событию
	FMyrLogicEventData* MyrLogicGetData(EMyrLogicEvent What);

	//управляемое существ в приведенном типе
	class AMyrPhyCreature* ME() { return (AMyrPhyCreature*)GetPawn(); }
	class AMyrPhyCreature* me() const { return (AMyrPhyCreature*)GetPawn(); }

	//цели - для краткости вызовов извне
	FGoal& Goal_1() { return Goals[PrimaryGoal]; }
	FGoal& Goal_2() { return Goals[SecondaryGoal]; }

	//найти цель которая в направлении взгляда, если есть - для подписи внизу экрана
	int FindGoalInView(FVector3f View, const float Devia2D, const float Devia3D, const bool ExactGoal2, AActor*& Result);

	//найти поданный объект среди текущих целей или возвратит 0 - это нужно для отсева лишних вычислений, так как в цели уже все есть
	FGoal* FindAmongGoals(UPrimitiveComponent* A) { if (Goal_1().Object == A) return &Goal_1(); else if (Goal_2().Object == A) return &Goal_2(); else return nullptr; }
	UFUNCTION(BlueprintCallable) int FindAmongGoalsIndex(UPrimitiveComponent* A) { if (Goal_1().Object == A) return 0; else if (Goal_2().Object == A) return 1; else return -1; }

	//доступ к правящей верхушке системы
	UFUNCTION(BlueprintCallable) class UMyrraGameInstance* GetMyrGameInst() const { return (UMyrraGameInstance*)GetGameInstance(); }
	UFUNCTION(BlueprintCallable) class AMyrraGameModeBase* GetMyrGameMode() const { return (AMyrraGameModeBase*)GetWorld()->GetAuthGameMode(); }

	//межклассовое взаимоотношение - чтобы сформировать отношение при первой встрече с представителем другого класса
	FColor ClassRelationToClass (AActor* obj);

	//утилиты для выяснения отношений объекта к нам
	FEmotion	WhatYouFeelOrRememberAboutMe	(UPrimitiveComponent* You)			const;
	float		HowDangerousAreYou				(UPrimitiveComponent* You)			const;
	float		HowClearlyYouPerceiveMe			(AMyrPhyCreature* You )	const;

	//выдать полные данные по нам как цели, которую имеет существо, находящееся у нас в целях
	FGoal* MeAsGoalOfThis(const AMyrPhyCreature* MyrAim) const;

	//периодически изменять величину с определенной скважностью
	template <class T> T Period (T IfLo, T IfHi, float Amount, uint8 digitbase = 31)
	{ if((Counter & digitbase) <= (Amount * digitbase)) return IfHi; else return IfLo; }

	//служебные функции - вероятность/скважность
	bool ChancePeriod(float Amount, uint8 digitbase = 31) { return (Counter & digitbase) <= (Amount * digitbase); }
	bool ChanceRandom(float Amount, uint8 digitbase = 31) { return (RandVar & digitbase) <= (Amount * digitbase); }
	bool ChanceRandom(uint8 Chance) { return (uint8)(RandVar & 255) <= Chance; }
	bool AITooWeak() { return !ChanceRandom(AIRuleWeight); }	//из веса в вероятность тру согласно весу (если вес 0 то и тру не будет)
	float Rand0_1() { return (float)RandVar / (float)RAND_MAX; }


	//в основном для диагностики через худ
	UFUNCTION(BlueprintCallable) const FGoal&		GetGoalSlot(int i) const { return Goals[i]; }
	UFUNCTION(BlueprintCallable) FGestaltRelation	GetGoalSlotGestalt(int i) const { if (Goals[i].Object) if (auto g = Memory.Find(Goals[i].Object)) return *g; return FGestaltRelation(); }
	UFUNCTION(BlueprintCallable) UPrimitiveComponent*			GetGoalSlotGestaltObject(int i) const { return Goals[i].Object; }
	UFUNCTION(BlueprintCallable) float				GetGoalVisibility(int i) const { return Goals[i].Visibility; }
	UFUNCTION(BlueprintCallable) float				GetGoalAudibility(int i) const { return Goals[i].Audibility; }
	UFUNCTION(BlueprintCallable) float				GetGoalWeight(int i) const { return Goals[i].Weight; }
};
