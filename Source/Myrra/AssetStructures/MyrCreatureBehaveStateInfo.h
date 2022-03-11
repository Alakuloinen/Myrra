#pragma once
#include "..\Myrra.h"
#include "MyrCreatureBehaveStateInfo.generated.h"

//###################################################################################################################
//###################################################################################################################
USTRUCT(BlueprintType) struct FBumpReaction
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Threshold = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = ELimb)) int32 BumperLimb = 1<<(int)ELimb::HEAD;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MinVelocity = 20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECreatureAction Reaction = ECreatureAction::NONE;
};

//как поступать при отклонении курса от позиции тела
UENUM(BlueprintType) enum class ETurnMode : uint8
{
	None,			//никак, довериться физдвижку, обычно означает резкий поворот
	SlowConst,		//медленно, с постоянным углом в кадр, разворачиваться пока раскоряка не будет устранена
	SlowLerp,		//плавно приближать угол к требуемому
	RailBack		//не крутить курса, как по резльсам сдвинуться в ту или иную сторону
};

UENUM(BlueprintType) enum class ETurnForWhom : uint8
{	NPC = 0,
	PlayerFirstPerson = 1,
	PlayerThirdPerson = 2,
};

//сборка параметров, как поступать при отклонении курса от позиции тела
USTRUCT(BlueprintType) struct FTurnReaction
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = ETurnForWhom)) uint8 ForWhom;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Threshold = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float SpeedFactor = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ETurnMode DoIfReached = ETurnMode::None;

	bool ForFirstPerson() { return (ForWhom & (1 << (int)ETurnForWhom::PlayerFirstPerson)); }
	bool ForThirdPerson() { return (ForWhom & (1 << (int)ETurnForWhom::PlayerThirdPerson)); }
	bool ForNPC() { return (ForWhom & (1 << (int)ETurnForWhom::NPC)); }
};

//###################################################################################################################
// ▄▀ дополнительные параметры, связанные с текущим режимом поведения - стоит ли их кешировать в каждом акторе?
// ▄▀ внимание, фабрика по созданию ресурсов этого типа в режакторе реализуется в модуле MyrraEditor/asset_factories.h
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, hidecategories = (Object), meta = (BlueprintSpawnableComponent), Config = Game)
class MYRRA_API UMyrCreatureBehaveStateInfo : public UObject
{
	GENERATED_BODY()
	friend class UMyrAnimNotify;

public:


	//применять особую динамику к частям тела только в этом состоянии поведения
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General") FWholeBodyDynamicsModel WholeBodyDynamicsModel;

	//учитывать взгляд вверх-вниз, планируя курс
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General") bool bOrientIn3D = false;

	// коментарий, чтобы объяснять, почему выставлены именно эти значения
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General") FString Comment;


public:

	// (опционально) минимальная скорость, с короторй начинается режим - в целом не нужно, но для некоторых нужно
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition") float MinVelocityToAdopt = 0.0f;

	// максимальная скорость для данного режима поведения, может быть ноль, оопределяет ограничитель, не актуальную скорость
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") float MaxVelocity = 0.0f;

	// максимальная скорость, которая ограничивается при физических заскоках
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") float EmergencyMaxVelocity = 600.0f;

	// типичный метаболизм для состояния, определяет скорость анимации дыхания
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") float MetabolismBase = 1.0f;

	// домножатель силы, если требуется переть в горку, а стандартного усилия не хватает
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") float UphillForceFactor = 10.0f;

	// домножатель силы, общий, для движения по плоскости, чтобы компенсировать вязкости, трения и т.п.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") float PlainForceFactor = 1.0f;

	// слагаемое, прибавляемое к расставке ног в зависимости от кривизны поверхности
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") float AffectSideSlopeOnLegOffset = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") float AffectLegsSpreadOnLegOffset = 1.0;

	// коэффициент заметности (1.0 - полная заметность, 0.0, полная незметность)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") float StealthPenalty = 1.0f;

	// базис роста здоровья, именно исцеления урона (и уменьшения здоровья) а не общего здоровья
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") float HealRate = 0.1f;

	// насколько выражать позу, соответствующую эмоции, при данном состоянии, например при стоянии максимуму, при беге нуль
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") float ExpressEmotionPower = 1.0f;


	// расслабить все моторы в теле, чтобы был рэгдол (при смерти и т.п.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") bool MakeAllBodyLax = false;

	//делать урон при очень сильных ударах - отключение помогает пережить заскоки физического движка
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") bool HurtAtImpacts = true;

	//список условий и реакций на разные по силе и безнадеге затыки в движении
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") TArray<FBumpReaction> BumpReactions;

	//список условий и реакций на моменты, когда нужно сильно повернуть тело
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties") TArray<FTurnReaction> TurnReactions;

public:

	// если существо игрок, то вход в это состояние включает в камере размытие в движении
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player") float MotionBlurAmount = 0.0f;

	// если существо игрок, то вход в это состояние включает в мире замедление времени
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player") float TimeDilation = 1.0f;

public:

	// звук, который простоянно играет, пока существо находится в данном состоянии
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio") USoundBase* SoundLoopWhileInState;

};