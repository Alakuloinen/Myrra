// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Myrra.h"
#include "MyrDaemon.generated.h"

//свои дебаг записи
DECLARE_LOG_CATEGORY_EXTERN(LogMyrDaemon, Log, All);

//###################################################################################################################
// ◘ дополнительный тик
//###################################################################################################################
USTRUCT() struct FDaemonOtherTickFunc : public FActorTickFunction
{
	GENERATED_USTRUCT_BODY()
		class AMyrDaemon* Target;
	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
	virtual FString DiagnosticMessage() override;
};
//без этой неведомой хрени он вообще не компилируется, жалуется на отсутствие оператора присваивания
template<> struct TStructOpsTypeTraits<FDaemonOtherTickFunc> : public TStructOpsTypeTraitsBase2<FDaemonOtherTickFunc> { enum { WithCopy = false }; };



//###################################################################################################################
// бестелесное метасущество, прослойка, чтоб смешивать PlayerController и ИИ,
// также здесь камера и абсолютная точка зрения в игре, поэтому здесь ряд единичных экземпляров функций
// - - выбор пользователем, что "сказать"
// - - протагонист обплёвывается эффектом дождя, другим дождь не нужен, поэтому дождь правится через этот класс
// - - здесь звучат звуки среды как целого, возможно, и музыку сюда перенести
// - - здесь осуществляется конвейер фиксации следов на мягкой поверхности (снег, трава, грязь)
// - - здесь теперь ещё и воздух движется (глобальная переменная в материале) вне игры
//###################################################################################################################
UCLASS() class MYRRA_API AMyrDaemon : public APawn
{
	GENERATED_BODY()

	//нахера эта капсула нужна? триггеры вообще-то реагируют на тушку
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCapsuleComponent* RootComp;

	//собствено, камера, через которую смотреть 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* Camera;

	//источник эффектов частиц - для дождя, роя наскомых, может, и запаха
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UParticleSystemComponent* ParticleSystem;


	//звуки природы, также, возможно, музыка
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UAudioComponent* AmbientSounds = nullptr;

	//декал для покрытия материалов перед камерой (п)блеском дождя
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UDecalComponent* RainWetArea = nullptr;

	//захватчик сцены для рендеринга следов, присмятия травы
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class USceneCaptureComponent2D* CaptureTrails = nullptr;

public:

	//ссылка на существо, в данный момент управляемое - это не компонент
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class AMyrPhyCreature* OwnedCreature = nullptr;

	//объект, к точке которого прилипает камера, будет он указан - для катсцен и плавного перехода к ним и от них
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class AActor* ExternalCameraPoser = nullptr;


protected:

	//дополнительный объект тик-функции для расчёта кинематики - необходимо при условии, что основной тик автоматически включает все остальное
	UPROPERTY() FDaemonOtherTickFunc OtherTickFunc;

//стандартное хозяйство
public:
	
	//значения по умолчанию
	AMyrDaemon();

	// связать органы управления извне и функции-отклики 
	// эту штуку вызывать после / при условии владения существом
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//такт немного реже чем покадровый - включается только при симуляции падения для динамического изменения спецеффектов
	virtual void Tick(float DeltaTime) override;

	//после инициализации компонентов - важно, чтобы до любых BeginPlay на уровне
	virtual void PostInitializeComponents() override;

	//начать играть
	void BeginPlay();

	/** If true, actor is ticked even if TickType==LEVELTICK_ViewportsOnly */
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	//событие, посылаемое другим, когда менется канал вижимого запаха
	FSwitchToSmellChannel SwitchToSmellChannel;


protected:

#if WITH_EDITOR
	//для перевтыкания на правильное место жертвы при редактировании 
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
#endif

//свойства
public:

	//ссылка на коллекцию параметров материала для множества материалов типа позиция солнца и т.п.
	//эту штуку задать в редакторе, пусть она будет одна на всё, так проще, чем плодить разные тематические коллекции
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class UMaterialParameterCollection* EnvMatParam;

	// позиция "воздуха" сейчас, смещение текстуры облаков и шелеста травы
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FVector2D CurrentAirMassPosition;

	//текущее направление ветра
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FVector2D WindDir;


	//предыдущая позиция демона, для расчёта сдвига следов
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector PreviousPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) EMyrCameraMode MyrCameraMode = EMyrCameraMode::ThirdPerson;

	//объект к которому прикреплена функция видения, который видит подобпечное существо, и который в фокусе камеры
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	AActor* ObjectAtFocus = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	FText ObjectNameAtFocus;

//куча всякой херни связанной с позиционированием камеры
public:

	//базис расстояния камеры в реальных сантиметрах, определяется или явно, или размерами тушки подопечного существа
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float ThirdPersonDist = 150;


	//явный включатель уворота камеры вовне, пока неясно, нужен он или нет, но чтобы не обнулять внешнего актора сойдёт
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	bool AllowExtCameraPosing = false;

	//текущее расстояние кинематографической камеры (с учётом внешнего задатчика позиции, с лерпом, если он исчезает)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float CamExtTargetDist = 150;

	//единичное направление на цель кинематографической камеры (с учётом внешнего задатчика позиции, с лерпом, если он исчезает)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FVector CamExtTargetVector;

	//текущий микс между внешней позицией камеры и внутренней, управляемой игроком, введён для точности без запаздываний
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float CamExtIntMix = 1.0f;


	//радиус орешка вокруг камеры от 3 лица, вглубь которого твёрдым предметам не разрешено проникать
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float CamHardCoreRadius = 10;

	//радиус сферы вокруг камеры 3 лица, от которой начинается упругость и отталкивание от стен
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float CamSoftBallRadius = 25;


	//нормированный коэффициент расстояния камеры - текущий
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float CamDistNormFactor = 1.0f;

	//нормированный коэффициент расстояния камеры - целевой, чтобы лерпить
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float CamDistNewFactor = 1.0f;

	//учёт того, что на расстояние камеры может влиять несколько взаимонакладывающихся объёмов
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 CamDistAffectingVolumes = 0;
	float CamDistStack[4];

	//коэффициент расстояния камеры полученный по затмению - домножается на основной
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float CamDistOccluderAffect = 1.0f;

	//ориентир для выше описанного параметра, непосредственно получаемый трассировкой из камеры в конец
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float CamDistCurrentByTrace = 1.0f;

	//насколько быстро надо выталкивать камеру из стены (0 - 1)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float CamDistRepulsion = 0.0f;

public:

	// чувствительности по осям
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float BaseTurnRate;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float BaseLookUpRate;

	// коэффициенты по осям, в системе координат контроллера, не тела, реагируют на отдельные оси, могут быть отрицательные
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float XGain = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	float YGain = 0.0f;

	//новая сборка со всеми параметрами для движения подопечного существа
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	FCreatureDrive Drive;

	//битовое поле зажатых клавиш, для асинхронной проверки и установки режимов
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bMove : 1;		//двигаться ваще
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bParry : 1;		//парировать (от start до strike)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bClimb : 1;		//лазать по деревьям
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bSupport : 1;		//хз, должно быть поддерживать от падения на ветке
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bExpress : 1;		//выбор действия для выражения эмоции
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 bRelaxChoose : 1;	//выбор способа релакса

	//в режиме от игрока, использовать направление взгляда/атаки из ИИ (для автонацеливания и поворота головы)
	//устанавливается из существа (но объявлено не в нём, так как нужно только с демоном) пока только на время активной фазы атаки
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 UseActDirFromAI : 1;

	//пост-процесс материал для экрана боли, достаётся из настроек камеры
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	UMaterialInstanceDynamic* PainScreen = nullptr;

	//материал для копирования рендер цели отпечатка шагов в историю
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	UMaterialInstanceDynamic* HistorifyTrailsMat = nullptr;

	//рычаг для управления материалом капель дождя по поверхности
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	UMaterialInstanceDynamic* RainDecalMatInst = nullptr;

	//мокрота того, что вокруг камеры - инерционный эффект дождя
	//влияет на материал с гистерезисом, без дождя извне постепенно спадает
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float Wetness = 0.0f;

	//канал дополнительных чувств, регулируют, какие запахи подсвечиваются
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 AdvSenseChannel = 0;

	//для режима выражения эмоций - список доступных самодействий, типа линий фраз диалога
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	TArray<uint8> AvailableExpressActions;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 CurrentExpressAction = 255;

#if WITH_EDITOR
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool CameraCollision = false;
#endif

//всякие методы
public:

	//подвязать к существу, чтобы им управлять (пока не используется, для автоподсоса к герою уровня при загрузке)
	void ClingToCreature(AActor* a);
	void ReleaseCreature();
	void PoseInsideCreature();

	//двигать камеру согласно контроллеру
	void MoveCamera3p();
	void MoveCamera1p();

	//медленный тик
	void LowPaceTick(float DeltaTime);

	// фиксация коэффициентов движения с клавы или геймпада
	void ProcessMotionInput(float* NGain, float Value);

	//новый способ определить расстояние до камеры -пока неясно, где лучше вызывать, посему отдельной функцией
	void TraceForCamDist();

	// рассчитать кинематику / моторику для тела (и/или ждать, когда тело ее выполнит)
	bool MakeMoveDir(bool MoveIn3D = false);

	//задать уровень размытия в движении
	void SetMotionBlur(float Amount);
	void SetTimeDilation(float Amount);

	//подготовить экран для менюшной паузы
	UFUNCTION(BlueprintCallable) void SwitchToMenuPause(bool Set);


	//задать направление атаки с корректировкой если в зад
	void SetAttackDir();

	//загнать в текущую камеру настройки эффектов 
	void AdoptCameraVisuals(const FEyeVisuals& EV);

	//переключение лица
	void SetFirstPerson(bool Set);

	//для запроса извне - от первого лица или от третьего лица
	bool IsFirstPerson();

	//переместить камеру в положение строго за спиной
	void ResetCamera();

	//установить видимый уровень дождя
	void SetWeatherRainAmount(float Amount);

	//всё что связано с рендерингом текстуры шагов и следов на траве, воде и т.п.
	void UpdateTrailRender();

	//чтобы не занимать строчку и не кастовать постоянно
	class AMyrPlayerController* MyrController() { return (AMyrPlayerController*)Controller; }
	class AMyrPhyCreature* GetOwnedCreature() const { return OwnedCreature; }

	UFUNCTION(BlueprintCallable) class UMyrraGameInstance* GetMyrGameInst() { return (UMyrraGameInstance*)GetGameInstance(); }
	UFUNCTION(BlueprintCallable) class AMyrraGameModeBase* GetMyrGameMode();

	//найти идентификатор реального действия (специфичный для этого существа) привязанный к условной кнопке
	ECreatureAction FindActionForPlayerButton(uint8 Button);

	//совершенно универсальные вызывальщики действий, для управления с компа этим видом существ
	void SendAction(int Button, bool Release, ECreatureAction ExplicitAction = ECreatureAction::NONE);

	//наэкранный интерфейс игрока, управляющего этим демоном, может выдать нуль 
	class UMyrBioStatUserWidget* HUDOfThisPlayer();

	//нужно для подсказки, на какую кнопку действие уровня существа повешено - может быть ни на какую - тогда -1
	int GetButtonUsedForThisAction(ECreatureAction Action);

//реакции на управление реальным существом
public: 

	//поворот камеры по горизонту и вверх-вниз
	void TurnAtRate(float Rate0to1) { AddControllerYawInput(Rate0to1 * BaseTurnRate * GetWorld()->GetDeltaSeconds()); }
	void LookUpAtRate(float Rate0to1) { AddControllerPitchInput(Rate0to1 * BaseLookUpRate * GetWorld()->GetDeltaSeconds()); }

	//реакция на "оси", вызывается каждый кадр, поэтому реальный расчёт движения только в одной из них
	void MoveForward(float Value) { ProcessMotionInput(&XGain, Value); }
	void MoveRight(float Value) { ProcessMotionInput(&YGain, Value); }

	//действия по кнопкам мыши
	void Mouse_R_Pressed()	{ SendAction(PCK_RBUTTON, false); }
	void Mouse_R_Released() { SendAction(PCK_RBUTTON, true); }

	//действия по кнопкам мыши
	void Mouse_L_Pressed()	{ SendAction(PCK_LBUTTON, false); }
	void Mouse_L_Released() { SendAction(PCK_LBUTTON, true); }

	//действия по кнопкам мыши
	void Mouse_M_Pressed()  { SendAction(PCK_MBUTTON, false); }
	void Mouse_M_Released() { SendAction(PCK_MBUTTON, true); }

	//раньше была стойка боком - теперь запуск атаки-парирования
	void SidePoseBegin()	{ SendAction(PCK_PARRY, false); }
	void SidePoseEnd()		{ SendAction(PCK_PARRY, true); }

	//действия по подбиранию и др. использованию предметов
	void PickStart()		{ SendAction(PCK_USE, false);  }
	void PickEnd()			{ SendAction(PCK_USE, true); }

	//нажатие кнопки, на которой по умолчанию висит отпрыг
	void AltPress()			{ SendAction(PCK_ALT, false); }
	void AltRelease()		{ SendAction(PCK_ALT, true); }

	//нажатие кнопки, на которой по умолчанию висит ускорение
	void ShiftPress()		{ SendAction(PCK_SHIFT, false); }
	void ShiftRelease()		{ SendAction(PCK_SHIFT, true); }

	//нажатие кнопки, на которой по умолчанию висит скрадывание
	void CtrlPress()		{ SendAction(PCK_CTRL, false); }
	void CtrlRelease()		{ SendAction(PCK_CTRL, true); }

	//накопление энергии для прыжка и сам прыжок
	void JumpPress()		{ SendAction(PCK_SPACE, false); }
	void JumpRelease()		{ SendAction(PCK_SPACE, true); }

	//вход и выход из режима взлёта
	void FlyPress()			{ SendAction(PCK_SOAR, false); }
	void FlyRelease()		{ SendAction(PCK_SOAR, true); }


	//нажатие кнопки сказать или выразить эмоцию
	void ExpressPress();
	void ExpressRelease();

	//по нажатию кнопки либо ничего (если мы в релаксе) или выбор меню состояний релакса и сна
	void RelaxPress();
	void RelaxToggle();

	//дискретный переключатель первого и третьего лица - в ответ на какую-нибудь кнопку
	void PersonToggle();

	//эксперимент - переключать камеру нормальную и типа киношную сбоку
	void CineCameraToggle();


	//в данном случае используется для начала и конца отталкивания прыжка
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location) { JumpPress(); }
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location) { JumpRelease(); }

	//крутить колесо мыши - на нем много всего прочего повешено
	void MouseWheel(float value);

	//явная установка расстояния камеры извне (через триггер-объёмы)
	UFUNCTION(BlueprintCallable) void ChangeCameraPos(float ExactValue)
	{	if (CamDistAffectingVolumes > 3) return;
		CamDistStack[CamDistAffectingVolumes] = CamDistNewFactor;
		CamDistAffectingVolumes++;
		CamDistNewFactor = ExactValue;
	}
	//восстановление прежнего расстояния камеры (если до этого оно изменилось другим объёмом, восстановить его)
	UFUNCTION(BlueprintCallable) void ResetCameraPos()
	{	CamDistAffectingVolumes--;
		if (CamDistAffectingVolumes < 0)
		{	UE_LOG(LogTemp, Error, TEXT("%s: WTF! Wrong cam dist volume exit"), *GetName());
			return;		}
		CamDistNewFactor = CamDistStack[CamDistAffectingVolumes];
		if(CamDistAffectingVolumes<0)
			CamDistNewFactor = 1.0f;
	}
	UFUNCTION(BlueprintCallable) float GetCameraPos() const { return CamDistNewFactor; }

};
