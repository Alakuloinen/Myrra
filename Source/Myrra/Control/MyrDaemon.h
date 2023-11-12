// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraShakeBase.h"
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
// также здесь камера и абсолютная точка зрения в игре,
// поскольку этот объект существует в сцене даже тогда, когда нет GameInstance и GameMode
// сюда перенесена куча уникальных задач
// - - выбор пользователем, что "сказать"
// - - протагонист обплёвывается эффектом дождя, другим дождь не нужен видным, поэтому дождь правится через этот класс
// - - здесь звучат звуки среды как целого, возможно, и музыку сюда перенести
// - - здесь осуществляется конвейер фиксации следов на мягкой поверхности (снег, трава, грязь)
// - - здесь теперь ещё и воздух движется (глобальная переменная в материале) вне игры
// - - здесь гнездится маркер квестов, отсюда он переносится с предмета на предмет
//###################################################################################################################
UCLASS() class MYRRA_API AMyrDaemon : public APawn
{
	GENERATED_BODY()

	//нахера эта капсула нужна? триггеры вообще-то реагируют на тушку
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCapsuleComponent* RootComp;

	//собствено, камера, через которую смотреть 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UMyrCamera* Camera;

	//источник эффектов частиц - для дождя, роя наскомых, может, и запаха
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UNiagaraComponent* ParticleSystem;

	//маркер цели квеста, должен пришпиливаться к указанным компонентам, пока неясно, нужен ли вообще
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UWidgetComponent* ObjectiveMarkerWidget;



	//звуки природы, также, возможно, музыка
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UAudioComponent* AmbientSounds = nullptr;

	//захватчик сцены для рендеринга следов, присмятия травы
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class USceneCaptureComponent2D* CaptureTrails = nullptr;

	//захватчик сцены для определения освещенности, размера зрачков и незаметности
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class USceneCaptureComponentCube* CaptureLighting = nullptr;


public:

	//ссылка на существо, в данный момент управляемое - это не компонент
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class AMyrPhyCreature* OwnedCreature = nullptr;

	//эффект тряски камеры - это не объект, это в редакторе подвязать класс
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<UCameraShakeBase> Shake;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<UCameraShakeBase> PainShake;

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

	//прямая ссылка на сборку небес для кореляции параметров наблюдателя напрямую в обход GameInst GameMode
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class AMyrKingdomOfHeaven* Sky;

	//ссылка на коллекцию параметров материала для множества материалов типа позиция солнца и т.п.
	//эту штуку задать в редакторе, пусть она будет одна на всё, так проще, чем плодить разные тематические коллекции
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class UMaterialParameterCollection* EnvMatParam;

	//предыдущая позиция демона, для расчёта сдвига следов
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector PreviousPosition;

	//накопитель пройденного пути
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DistWalkedAccum = 0;

	//объект к которому прикреплена функция видения, который видит подобпечное существо, и который в фокусе камеры
	FGestalt* ObjectAtFocus = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool AutoAim = false;

//куча всякой херни связанной с позиционированием камеры
public:

	//базис расстояния камеры в реальных сантиметрах, определяется или явно, или размерами тушки подопечного существа
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float ThirdPersonDist = 150;

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

	//материал для копирования рендер цели отпечатка шагов в историю
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	UMaterialInstanceDynamic* HistorifyTrailsMat = nullptr;

	//материал для декаля мокроты, когда дождь прошел мимо крыши
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	UMaterialInstance* WetDecalMat = nullptr;

	//дождь, который генерируется, когдя понятно, что крыши нет
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)	UNiagaraSystem* SecondaryRain = nullptr;


	//мокрота того, что вокруг камеры - инерционный эффект дождя
	//влияет на материал с гистерезисом, без дождя извне постепенно спадает
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float Wetness = 0.0f;

	//степень воздействия психоделического шейдера, включается при потреблении травы, при смерти, возможно и во сне
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float Psychedelic = 0.0f;

	// сонность, накапливается при бодрствовании (возможно, убрать, использовать А у эмоции)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Sleepiness = 0.0f;	

	// уровень освещенности в глаза - для расширения зрачков        
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Luminance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FRatio RainAmount = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool CurLocNoRain = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 CurLocFliesMod = 255;



	//канал дополнительных чувств, регулируют, какие запахи подсвечиваются
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 AdvSenseChannel = 0;

	//для режима выражения эмоций - список доступных самодействий, типа линий фраз диалога
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	TArray<uint8> AvailableExpressActions;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)	uint8 CurrentExpressAction = 255;

	//когда логика игры врубает геймовер, сюда заносится выдержка в секундах перед окончательным геймовером в меню
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float PreGameOverTimer = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float PreGameOverLimit = 0.0f;

	//возможность отключать обплыв камерой препятсвтвий чтоб посмотреть под
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool CameraCollision = false;
#endif

//всякие методы
public:

	//подвязать к существу, чтобы им управлять (пока не используется, для автоподсоса к герою уровня при загрузке)
	void ClingToCreature(AActor* a);
	void ReleaseCreature();
	void PoseInsideCreature(bool ResetCameraRot = true);

	//медленный тик
	void LowPaceTick(float DeltaTime);

	// фиксация коэффициентов движения с клавы или геймпада
	void ProcessMotionInput(float* NGain, float Value);

	// рассчитать кинематику / моторику для тела (и/или ждать, когда тело ее выполнит)
	bool MakeMoveDir(bool MoveIn3D = false);

	//задать уровень размытия в движении
	void SetTimeDilation(float Amount);

	//подготовить экран для менюшной паузы
	UFUNCTION(BlueprintCallable) void SwitchToMenuPause(bool Set);


	//задать направление атаки с корректировкой если в зад
	void SetAttackDir();

	//переключение лица
	void SetFirstPerson(bool Set);

	//для запроса извне - от первого лица или от третьего лица
	bool IsFirstPerson();

	//раздать всем запахам команду вкл выкл запах
	void UpdateSmellVision() { SwitchToSmellChannel.Broadcast(IsFirstPerson() ? AdvSenseChannel : -1); }

	//переместить камеру в положение строго за спиной
	void ResetCamera();

	//установить видимый уровень дождя
	void SetWeatherRainAmount(float Amount);

	//установить количество мух летающих вокруг 
	void SetFliesAmount(float Amount);

	//всё что связано с рендерингом текстуры шагов и следов на траве, воде и т.п.
	void UpdateTrailRender();

	//чтобы не занимать строчку и не кастовать постоянно
	class AMyrPlayerController* MyrController() { return (AMyrPlayerController*)Controller; }
	class AMyrPhyCreature* GetOwnedCreature() const { return OwnedCreature; }

	UFUNCTION(BlueprintCallable) class UMyrraGameInstance* GetMyrGameInst() { return (UMyrraGameInstance*)GetGameInstance(); }
	UFUNCTION(BlueprintCallable) class AMyrraGameModeBase* GetMyrGameMode();

	//найти идентификатор реального действия (специфичный для этого существа) привязанный к условной кнопке
	EAction FindActionForPlayerButton(uint8 Button);

	//совершенно универсальные вызывальщики действий, для управления с компа этим видом существ
	void SendAction(int Button, bool Release, EAction ExplicitAction = EAction::NONE);

	//наэкранный интерфейс игрока, управляющего этим демоном, может выдать нуль 
	class UMyrBioStatUserWidget* HUDOfThisPlayer();

	//нужно для подсказки, на какую кнопку действие уровня существа повешено - может быть ни на какую - тогда -1
	int GetButtonUsedForThisAction(EAction Action);

	//поместить или удалить маркер квеста - извне
	UFUNCTION(BlueprintCallable) void PlaceMarker(USceneComponent* Dst);
	UFUNCTION(BlueprintCallable) void RemoveMarker();

	//выдать уровень размытия в движении, который был установлен для текущего состояния
	float GetCurMotionBlur() const;

	//получить уровень освещенности
	float GetLightingAtVector(FVector3f V);

	//среагировать на удар капли по земле
	UFUNCTION(BlueprintCallable) void ReactOnRainDrop(float Size, FVector Position, FVector Velocity);

	//источник данных камеры по лицам
	FEyeVisuals* GetEyeVisuals(int P);

	//привзяать камеру к внешнему актору
	void AdoptExtCameraPoser(USceneComponent* A);
	void DeleteExtCameraPoser();
	bool IsCameraOnExternalPoser();

	//явная установка расстояния камеры извне (через триггер-объёмы)
	UFUNCTION(BlueprintCallable) void ChangeCameraPos(float ExactValue);

	//восстановление прежнего расстояния камеры (если до этого оно изменилось другим объёмом, восстановить его)
	UFUNCTION(BlueprintCallable) float ResetCameraPos();

	//отобразить параболу прыжка
	void ShowJumpTrajectory(FVector StartPosition, FVector JumpStartVelocity, float Time, FVector EndPosition);
	void EnableJumpTrajectory(bool Set);

	USceneComponent* GetCamera() { return (USceneComponent*)Camera; }
	UMyrCamera* GetMyrCamera() { return (UMyrCamera*)Camera; }


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


};
