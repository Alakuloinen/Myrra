#pragma once

#include "CoreMinimal.h"
#include "..\Myrra.h"
#include "MyrCreatureGenePool.generated.h"

class UMyrActionInfo;


//###################################################################################################################
// ▄▀ генофонд - полное собрание свойств, применимых для данного класса существ, статические, доступ по указателю
// ▄▀ внимание, фабрика по созданию ресурсов этого типа в режакторе реализуется в модуле MyrraEditor/asset_factories.h
// после создания свежего генофонда надо выбрать для него скелетный меш, и большая часть данных сюда зальется сама
// однако кости осей машинной части скелета для лапок надо заносить в ручную, если их нет в скелете со стандартными именами
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, hidecategories = (Object), meta = (BlueprintSpawnableComponent), Config = Game)
class MYRRA_API UMyrCreatureGenePool : public UObject
{
	GENERATED_BODY()

	//человекопонятное имя, которое можно отобразить на экране как пункт меню и переовдить
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), BlueprintReadWrite) FText HumanReadableName;


//------------------------------------------------
public: // диапазоны умений
//------------------------------------------------
	


	//новая попытка в ИИ, "безусловные рефлексы", эмоциональные реакции на отдельные элементарные стимулы
	//UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Role")	FPathia	ElementaryEmotionsYe[(int)EYeAt::Latent];
	//UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Role")	FPathia	ElementaryEmotionsMe[(int)EMeAt::Latent];

	//базис для рассчёта терминальной скорости, с которой соударяясь, не получаешь травм
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Role") float MaxSafeShock = 600;

	//минимальный уровень, с которого существо способно летать (и помжет получать от ИИ запросы на взлёт) - на практике просто разрешитель (0) и запретитель (255) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Role") uint8 MinLevelToFly = 255;

//------------------------------------------------
public: // анатомия, части тела
//------------------------------------------------

	//собственно, ресурс скелетного меша, который используют объекты этого класса, 
	//в него уже должны быть включены PhysicsAsset и т.д., так что это наиболее полный ресурс
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Anatomy") class USkeletalMesh* SkeletalMesh = nullptr;

	//физические характеристики по каждому сегменту тела (заполняются тщательно вручную)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Anatomy", config) FMeshAnatomy Anatomy;

	//эталонный набор данных по здоровью члеников, генерируется перед игрой (при спавне реального объекта просто копируется в него)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Anatomy") TArray<FBODY_BIO_DATA> BodyBioData;

	//оси костей каркаса существа, какая ось вверх, какая в бок, какая вперед - потому что 3d редактор + экспорт + импорт, и в каждой модельке они выходят свои
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Anatomy") FLimbOrient MachineBonesAxes;

	//по каким осям может крутиться голова и спина - вообще-то надо это вычислять из проставленных констрейнтов, но гемор
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Anatomy") FBodyFreedom BodyDegreesOfFreedom;

	// индекс членика (по нулям, чтобы не огребать ошибок) - хз, насколько теперь это нужно
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Anatomy") uint8 RootBodyIndex = 0;

	// материал с высоким трением, на который переключаются членики тела, когда требуется физически застыть на месте и не скользить до дна (лежание, смерть)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Anatomy") class UPhysicalMaterial* AntiSlideMaterial;

	//характеристика существа как еды и того, что после съедения остаётся
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Anatomy")	FDigestiveEffects DigestiveEffects;

	// объект, который остаётся после съедения
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Anatomy") TSubclassOf<AActor> Remains;					


//------------------------------------------------
public: // визуальные дополнения
//------------------------------------------------

	//система частиц при ударе по данному телу (в дополнение к глобальной таблице по типам поверхности - чтобы отображать разную шерсть)
	//но это всё равно не достаточно, так как на одном генофонде могут быть несколько окрасок
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Visual") class UParticleSystem* SplatterAtImpact;

	//система частиц, рендеремая при чутье запаха
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Visual") class UParticleSystem* Smell;

	//канал, на котором "виден" запах существ этого класса
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Visual")  uint8 SmellChannel = 0;

	//декалы следов, оставляемые на разных поверхностях
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Visual") TMap<EMyrSurface, UMaterialInterface*> FeetDecals;

	//настройки спецэффектов, связанные со зрением этого существа, действуют от первого лица
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Visual") FEyeVisuals FirstPersonVisuals;

	//станартные окрасы, если не предусмотрено генерируемого окраса, если в этом массиве что-то есть, окрас не генерируется
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Visual") TArray<UTexture*> StaticCoatTextures;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Visual") TArray<UAnimSequenceBase*> PhysiquePoses;


//------------------------------------------------
public: // взаимодействие
//------------------------------------------------

	//допустимые состояния поведения - подцепляется из ресурсов
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Behave")
		TMap<EBehaveState, class UMyrCreatureBehaveStateInfo*> BehaveStates;

	//*********************************новый вариант***************в этом массиве и атаки и всё прочее
	//все характеристики атак, индекс имеет значение, указывается в CurrentAtack
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Actions") TArray <UMyrActionInfo*> Actions;
	TMultiMap<EAction,uint8> ActionMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Actions") bool UpdateActions = false;



//------------------------------------------------
public: // управление со стороны игрока, не ИИ
//------------------------------------------------

	//карта альтернативных действий на кнопки игрока
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Player", config) TMap<EBehaveState, FBehaveStatePlayerTriggeredActions> PlayerStateTriggeredActions;

	//назначение стандартных действий на кнопки игрока
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Player", config) FBehaveStatePlayerTriggeredActions PlayerDefaultTriggeredActions;

//------------------------------------------------
public: // искусственный интеллект
//------------------------------------------------

	//настройки, которые сбрасываются в ИИ компонент при его инициализации. Возможно, не нужно - иметь на каждый класс существа свой субкласс ИИ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI", config) float DistanceSeeing = 3000;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI", config) float DistanceUnSeeing = 3500;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI", config) float DistanceHearing = 2000;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI", config) float DistanceOlfaction = 2000;

	//коэфициенты скоростей для пограничных эмоций
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI") class UCurveLinearColor* MotionGainForEmotions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI") class UCurveLinearColor* StealthGainForEmotions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI") class UCurveLinearColor* TurnToFaceForEmotions;

	//распределение эмоций по времени суток - базис, к которому стремится настроение при отсутствии раздражителей
	//для NPC, возможно, брать с коэффициентом ослабления, чтобы стремилось к нулю и не вносило искажения в реакцию на протагониста
	//если протагонист, влияет на играемую музыку (вместе с погодой и местом под ногами)
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI") class UCurveLinearColor* IdleEmotionsThroughDayTime;

//------------------------------------------------
public: // аудио 
//------------------------------------------------

	//звук, или сборка звуков, зависящая от силы, для выражение боли - именно голосом
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Audio") class USoundBase* SoundAtPain;

	//звук, проигрываемый при зарабатывании очка опыта при игре за существо этого класса
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Audio") class USoundBase* SoundAtExpGain;

	//звук, проигрываемый при смерти игрока этого вида
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Audio") class USoundBase* SoundSelfDeath;

//--------------------------------------------------------------------------------------
public:	// стандартные переобпределяемые методы
//--------------------------------------------------------------------------------------

	//после загрузки (если массив свойств заполняется во в Blueprint Defaults, их обработка выполнится здесь)
	virtual void PostLoad() override;

#if WITH_EDITOR
	//при перезакреплении типового скелета в редакторе - возможно, нах
	virtual void PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent) override;
#endif

//--------------------------------------------------------------------------------------
public:	// свои методы
//--------------------------------------------------------------------------------------

	UMyrCreatureGenePool() { ActionMap.Empty(); PrepareEmotions(); }

	void PrepareEmotions();

	//разобрать добавленные действия и построить карту для быстрого поиска
	void AnalyzeActions();

	//проанализировать регдолл и собрать инфу по количеству члеников для разных сегментов тела
	void AnalyseBodyParts ();

	//найти среди сторон сустава нужную кость - если она там есть, выдать вторую кость
	FName FindConstraintOppositeBody(int CsIndex, FName FirstBoneName);

	//покопаться в объёмах столкновений и вывести имя
	FName GetBodyName(int i, UPhysicsAsset* Pha);




};

