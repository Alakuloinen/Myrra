#pragma once
#include "CoreMinimal.h"
#include "..\Myrra.h"
#include "MyrActionInfo.generated.h"

//###################################################################################################################
// структура полностью посвященная условиям для срабатывания атаки либо самодействия
//###################################################################################################################
USTRUCT(BlueprintType) struct FActionCondition
{
	GENERATED_BODY()

	//пределы эмоционального отношения к цели, при которых хочется совершить сие действия
	//UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) FEmotionRadius FeelToCause;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) FPathia EmotionLim1 = EPathia::Void;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) FPathia EmotionLim2 = EPathia::Insanity;

	//пределы скорости, здоровья, запаса сил, давности установления состояния поведения когда это действие осмысленно
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) FFloatRange Velocities;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) FFloatRange Healthes;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) FFloatRange Staminas;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) FFloatRange StateAges;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) FFloatRange Sleepinesses;

	//список состояний тела, в которых можно или нельзя выполнять это действие
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) bool StatesForbidden = true;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) TSet<EBehaveState> States;

	//состояния, которые никак не влияют на начала действия, но которые вырубают это действие при переходе на себя
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) TSet<EBehaveState> StatesToCeaseAtTransit;

	//список типов поверхностей, на которых это действие возможно... или, наоборот, запрещено
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) bool SurfacesForbidden = true;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) TSet<EMyrSurface> Surfaces;

	//список типов акторов, которые могут использовать на себе это действие (например, нельзя атаковать кого-то)
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) bool ActorsForbidden = true;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) TSet<TSubclassOf<AActor>> Actors;

	//вероятность (полные диапазон байта) что в данный такт полностью подходящее действие выстрелит
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) uint8 Chance = 255;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) uint8 ChanceForPlayer = 0;

	//приоритет, самодействия с более высоким приоритетом могут выбиватьужевыполняющиеся самодействия с болеенизким приоритетом
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) uint8 Priority = 255;

	//если выпадает делать атаку при незаконченном самодействии, это самодействие резко прерывается
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) bool SkipDuringAttack = true;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) bool SkipDuringSpeaking = true;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) bool SkipAtGrab = true;
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) bool SkipDuringRelax = true;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	FActionCondition() : Velocities(0.0f, 150.0f), Healthes(0.0f, 1.1f), Staminas(0.0f, 1.1f), StateAges(0.0f, 1000.1f), Sleepinesses(-0.1f, 1.1f) {  }

	//подходит ли данное действие для выполнения данным существом
	EResult IsFitting(AActor* Owner, bool PlayerCommand = true);



};

//###################################################################################################################
// новая попытка - модуль жертвы для действия, превращающий его в атаку
//###################################################################################################################
USTRUCT(BlueprintType) struct FVictimType
{
	GENERATED_BODY()

	//условия срабатывания - характеристики жертвы, необходимые для того, чтоб на нее можно было применить эту атаку
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) FActionCondition Condition;

	//анимация атаки в виде сборки aim offset по проекциям курса на цель вперед-назад и верх-вниз - отсчитывается от нулевой точки тела (от жопы)
	//сюда кладётся самая базовая анимация, преимущественно физической части скелета, преимущественно содержащая уклонения влево-вправо, так как вверх-вниз можно сделать и кинематикой
	UPROPERTY(EditAnywhere, Category = "Resources", BlueprintReadWrite) class UAimOffsetBlendSpace* RawTargetAnimation;

	//анимация атаки в виде сборки Blend Space (в локальном пространстве) по проекциям курса на цель вперед-назад и верх-вниз
	//но уже не от жопы, а от груди - поэтому в Local Space, а не Mesh Space - самое точное нацеливание, либо коррекция плечевым поясом нацеливания, совершенного от жопы
	UPROPERTY(EditAnywhere, Category = "Resources", BlueprintReadWrite) class UBlendSpace* PreciseTargetAnimation;

	//прямой физический урон той части тела, до которой дотронулись (может быть отрицательным и исцелять)
	//единица означает урон такому же существу без учёта щита в целое здоровье
	UPROPERTY(EditAnywhere, Category = "Damage", BlueprintReadWrite) float TactileDamage = 1.0f;

	//если здесь прыжок, то это импульс
	UPROPERTY(EditAnywhere, Category = "Damage", BlueprintReadWrite) float JumpVelocity = 600;
	UPROPERTY(EditAnywhere, Category = "Damage", BlueprintReadWrite) float JumpUpVelocity = 0;

	//какие части тела используются при касании противника
	UPROPERTY(EditAnywhere, Category = "Damage", BlueprintReadWrite) TMap<ELimb, FActionPhaseSet> WhatLimbsUsed;

	//расстояние, на котором атака наиболее эффективна, и размах плеча от 0 до максимума
	UPROPERTY(EditAnywhere, Category = "Spatial", BlueprintReadWrite) float DistMaxEffect = 30.0f;
	UPROPERTY(EditAnywhere, Category = "Spatial", BlueprintReadWrite) float DistRange = 15.0f;

	//угловой размах опасной зоны, косинус угла (скалярное произведение) минус единица
	UPROPERTY(EditAnywhere, Category = "Spatial", BlueprintReadWrite) float AngleCosRange = 0.5f;

	// возможность развернуть и повторить основное ударное действие атаки до ее полного завершения (количество таких ударов без замаха)
	UPROPERTY(EditAnywhere, Category = "Advanced", BlueprintReadWrite) uint8 NumInstantRepeats = 1;

	// запретить автоматический или любой другой вызов самодействий на всё время дления этой атаки
	UPROPERTY(EditAnywhere, Category = "Advanced", BlueprintReadWrite) bool ForbidSelfActions = true;

	//фазы, в какую хватать цель, отпускать, прыгать, жрать
	UPROPERTY(EditAnywhere, Category = "Advanced", BlueprintReadWrite) EActionPhase GrabAtTouchPhase	= EActionPhase::NONE;
	UPROPERTY(EditAnywhere, Category = "Advanced", BlueprintReadWrite) EActionPhase UngrabPhase			= EActionPhase::NONE;
	UPROPERTY(EditAnywhere, Category = "Advanced", BlueprintReadWrite) EActionPhase EatHeldPhase		= EActionPhase::NONE;

	//посылать сообщение триггеру, буде такой в пересечении с этим существом, что выполнено (возможно запрашиваемое им) активное действие
	//логика обратножоповая, но это единственный способ для триггера отслеживать, например, кнопку Е без нужды включать тик в самом триггере
	UPROPERTY(EditAnywhere, Category = "Consequences", BlueprintReadWrite) bool SendApprovalToTriggerOverlapperOnStart = false;
	UPROPERTY(EditAnywhere, Category = "Consequences", BlueprintReadWrite) bool SendApprovalToTriggerOverlapperOnStrike = false;

public:

	//величина (вероятность) достать цель / получить урон от расстояния до цели
	float AmountByRadius(float CheckThisDist) { return FMath::Max(0.0f, DistRange - FMath::Abs(DistMaxEffect - CheckThisDist)) / DistRange; }

	//величина (вероятность) достать цель / получить урон от угла между направлением атаки и актуальным азимутом (жесткое смешение с очень тонким переходом)
	float AmountByAngle(float DirDotAttack) { return FMath::Max(0.0f, 1.0f - 10.0f * FMath::Max(0.0f, AngleCosRange - DirDotAttack)); }

	//порог (попал/не попал) от расстояния (любое вхождение в зону поражения засчитывается)
	bool CheckByRadius(float CheckThisDist) { return FMath::Abs(DistMaxEffect - CheckThisDist) < DistRange; }
	bool CheckByRadius2(float CheckThisDist2) { return FMath::Square(CheckThisDist2) < DistMaxEffect + DistRange; }

	//порог (попал/не попал) от от угла между направлением атаки и актуальным азимутом (любое вхождение в зону поражения засчитывается)
	bool CheckByAngle(float DirDotAttack) { return DirDotAttack > AngleCosRange; }
	bool CheckByAngle(FVector AMasterAxis, FVector DirToYou) const { return FVector::DotProduct(AMasterAxis, DirToYou) > AngleCosRange; }

	//интегральная мера опасности атаки
	float IntegralAmount(float Dist, float AngleDot) {	if (!CheckByRadius(Dist)) return 0.0f; else return AmountByRadius(Dist) * AmountByAngle(AngleDot);	}

	//подходит ли данное данное существо на роль жертвы данной атаки
	EResult IsVictimFitting(class AMyrPhyCreature* Owner, USceneComponent* Victim, float Rad, float Ang, bool CheckByChance = true, bool ByAI = true)
	{
		if (!Victim) return EResult::WRONG_ACTOR;//◘◘>
		if (Condition.Actors.Contains(((AActor*)Owner)->StaticClass()) == Condition.ActorsForbidden)
			return EResult::WRONG_ACTOR;//◘◘>
		if (ByAI)
		{	if (!CheckByRadius(Rad))	return EResult::OUT_OF_RADIUS;//◘◘>
			if (!CheckByAngle(Ang))		return EResult::OUT_OF_ANGLE;//◘◘>
		}

		return Condition.IsFitting(Victim->GetOwner(), CheckByChance);
	}

};

//###################################################################################################################
// действие. Новый класс, в котором будет всё - и атака, и самодействие, и релакс
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, hidecategories = (Object), meta = (BlueprintSpawnableComponent))
class MYRRA_API UMyrActionInfo : public UObject
{
	GENERATED_BODY()

public:

	//основная, анимация (с закладками), аддитивная, которая передаётся в AnimInst в виде переменной.
	//для атак сюда следует пихать пустую анимацию с правильными закладками, да и вообще закладки держать сздесь
	UPROPERTY(EditAnywhere, Category = "Resources", BlueprintReadWrite) class UAnimSequenceBase* Motion;
	UPROPERTY(EditAnywhere, Category = "Resources", BlueprintReadWrite) TArray<UAnimSequenceBase*> AlternativeRandomMotions;

	//человекопонятное имя, которое можно отобразить на экране как пункт меню и переовдить
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText HumanReadableName;

	//новый вариант идентификации, одна сборка может использоваться по нескольким типам действий
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSet<EAction> UsedAs;


public:

	//условия срабатывания для агента действия
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) FActionCondition Condition;

	//новый способ совместить атаки и самодействия - массив типов жертв, обычно в нём только одна строчка
	UPROPERTY(EditAnywhere, Category = "Victim", BlueprintReadWrite) TArray<FVictimType> VictimTypes;

	//если вес ИИ выше этого значения, ИИ может сам инициировать это дйствие,
	//если тут 0, значит действие может случайно вызываться на персе игрока в нормальном состоянии,
	//если 1 - ИИ может вызывать это действие только будучи полноправлным хозяином неписи
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) float MinAIWeight = 0.0f;

	//если противник сам нас атакует, то вот это - его урон, который мы можем отразить этим действием (даже самодействием) как контратакой, если превышает, то прекратить атаку
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) float MaxDamageWeCounterByThis = 0.0f;

	//прерывать или отправлять вспять атаку, если произошло столкновение или увечье
	UPROPERTY(EditAnywhere, Category = "Conditions", BlueprintReadWrite) float MinHitDamageToCease = 10.0f;

public:

	//кинематический режим, анимация накладывается на стартовую позу без учёта физики и окружения
	UPROPERTY(EditAnywhere, Category = "Appearance", BlueprintReadWrite) bool LayAnimOnRefPose = false;

	//использовать введенные здесь динамические модели - или не использовать, а продолжать те, что уже установлены
	//например, если действие содержит мелкую анимацию, но не содержит отдельных физических поз
	UPROPERTY(EditAnywhere, Category = "Appearance", BlueprintReadWrite) bool UseDynModels = true;

	/////////////////////////////////////////////////////////////////////////////////////////
	//новый вариант - чтобы обеспесить разные модели на разных отрезках анимации
	UPROPERTY(EditAnywhere, Category = "Appearance", BlueprintReadWrite) TArray<FWholeBodyDynamicsModel> DynModelsPerPhase;
	/////////////////////////////////////////////////////////////////////////////////////////

	//включать размытие в движении разных фаз  
	UPROPERTY(EditAnywhere, Category = "Appearance", BlueprintReadWrite) float MotionBlurBeginToStrike = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Appearance", BlueprintReadWrite) float MotionBlurStrikeToEnd = 0.0f;

	//уровень метаболизма, насколько напряжённо сцена отражается на дёрганьи и дыхании
	UPROPERTY(EditAnywhere, Category = "Appearance", BlueprintReadWrite) float MetabolismMult = 0.0f;


public:

	//навязываемая этим действием эмоция - для себя и для цели, которая эту акцию через ИИ воспримет - редкий случай, когда отклик берется не из глобальной таблицы откликов
	//даже если это не самодействие-"экспрессия", а что-то более полезное, экспрессия всё равно передётся жертве (например, если атака мимо)
	UPROPERTY(EditAnywhere, Category = "Consequences", BlueprintReadWrite) FMyrLogicEventData EmotionalInducing;

	//кэш изменения стамины, должен расчитываться, отображает трудоемкость атаки
	UPROPERTY(VisibleAnywhere, Category = "Consequences", BlueprintReadOnly) float OverallStaminaFactor = 0.0f;

	//скорость накопления силы с момента включения Hold до Jump
	UPROPERTY(EditAnywhere, Category = "Consequences", BlueprintReadWrite) float HoldForceAccumulationSpeed = 1.0f;

	//коэффициент урона, аддитивный, который поглощается, если во время этого действия само существо будет атаковано
	UPROPERTY(EditAnywhere, Category = "Consequences", BlueprintReadWrite) float HitShield = 0.0f;

public:

	//подходит ли данное действие для выполнения данным существом
	//простая форма для игрока, который сам должен думать о цели
	//вызывается при поиске по ИД действия, может попасть и в ИИ, если прописать
	EResult IsActionFitting(class AMyrPhyCreature* Owner, bool CheckByChance = true)
	{		return Condition.IsFitting((AActor*)Owner, CheckByChance);	}

	//подходит ли данное действие для выполнения данным существом
	//сложная форма, только для ИИ, проверяет резон применения на конкретную цель
	//в ИИ вызывается в переборе всех доступных 
	EResult IsActionFitting(class AMyrPhyCreature* Owner,
		USceneComponent* ExactVictim,
		float LookAtDist, float LookAlign, 
		bool ByAI = true, bool CheckByChance = true);

	//упрощенный вариант перетестирования для ИИ
	EResult RetestForStrikeForAI(class AMyrPhyCreature* Owner, UPrimitiveComponent* ExactVictim, float Dist, float Ang);

	//рассчитать время, которое займёт прокат ролика между данными фазами
	float PhaseTimes(TArray<float> &OutTimes);

	//пересчитать энергоемкость действия
	void RecalcOverallStaminaFactor();

	//это не самодействие а атака - с указанием типа жертвы
	bool IsAttack() { return (VictimTypes.Num()>0); }

	//признак релакс-действия - 3 фазы, причём средняя по возможности останавливает время
	bool IsRelax() { return (DynModelsPerPhase.Num() == 3 && DynModelsPerPhase[1].AnimRate < 0.1); }

	//для сравнения одинаоквых по критерию силоёмкости
	bool Better(UMyrActionInfo* Other) const {	return OverallStaminaFactor >= Other->OverallStaminaFactor; 	}

	//проверка возможности попадения в цель, для автонацеливания
	bool QuickAimResult(class AMyrPhyCreature* Owner, FGestalt Gestalt, float Accuracy);

	//после загрузки (здесь пересчитывается энергоемкость)
	virtual void PostLoad() override
	{
		Super::PostLoad();
	}

	//найти среди списка динамических моделей, внедряемых этим действием, ту, которая результирует прыжок
	FWholeBodyDynamicsModel* FindJumpDynModel(int StartFrom = 0)
	{	for (int i = StartFrom; i<DynModelsPerPhase.Num(); i++)
			if (DynModelsPerPhase[i].IsJump()) return &DynModelsPerPhase[i];
		return nullptr;
	}

	//пересчитать суммарную, максимальную способность этого действия противостоять урону
	void RecalcMaxDamage()
	{	MaxDamageWeCounterByThis = 0;
		for(auto& DM : DynModelsPerPhase)
			if(DM.DamageLim > MaxDamageWeCounterByThis) MaxDamageWeCounterByThis = DM.DamageLim;
	}

	
	//для автоматического пересчета при редактировании
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void Refine()
	{
		//if (UsedAs.Num() == 0)	UsedAs.Add(Type);
		RecalcOverallStaminaFactor();
		bool Mut = false;
		for (auto& DM : DynModelsPerPhase)
			Mut = Mut || DM.Correction();
		if(Mut) MarkPackageDirty();
	}
#endif

};
