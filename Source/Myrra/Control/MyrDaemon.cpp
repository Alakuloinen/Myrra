// Fill out your copyright notice in the Description page of Project Settings.

// сам
#include "MyrDaemon.h"									

//компоненты
#include "Camera/CameraComponent.h"						// камера
#include "Components/InputComponent.h"					// ввод с клавы мыши
#include "GameFramework/SpringArmComponent.h"			// пружинка камеры
#include "Components/AudioComponent.h"					// звук
#include "Components/DecalComponent.h"					// декал
#include "Components/SceneCaptureComponent2D.h"			// рендер в текстуру примятия травы
#include "Components/SceneCaptureComponentCube.h"		// рендер в текстуру примятия травы
#include "Components/ArrowComponent.h"					// херь для корня и индикации
#include "Components/CapsuleComponent.h"					// херь для корня и индикации
#include "Components/SphereComponent.h"					// херь для регистрации помех камеры
#include "Materials/MaterialParameterCollectionInstance.h"	// для подводки материала неба
#include "Engine/TextureRenderTarget2D.h"				//текстура для рендера в текстуру
#include "../UI/MyrBioStatUserWidget.h"					// индикатор над головой
#include "Components/WidgetComponent.h"					// ярлык с инфой
#include "Engine/TextureRenderTargetCube.h"

#include "NiagaraComponent.h"							//для нового эффекта дождя					
#include "NiagaraFunctionLibrary.h"						//для нового эффекта дождя					

#include "Sky/MyrKingdomOfHeaven.h"						//небо отдаёт свои дрючки на тик этому классу, потому что он существует вне игры

//системные дополнения
#include "Engine/Classes/Kismet/GameplayStatics.h"		// замедление времени
#include "Materials/MaterialInstanceDynamic.h"			// для управления материалами эффектов камеры
#include "Engine/Public/EngineUtils.h"					// для ActorIterator
#include "Kismet/KismetRenderingLibrary.h"				// чтоб рендерить в RT и копировать, не могли методами RT сделать, уроды
#include "GameFramework/InputSettings.h"				// чтоб выдавать названия клавиш

//свои
#include "MyrPlayerController.h"						// специальный контроллер игрока
#include "../MyrraGameInstance.h"						// самые общие на всю игру данные
#include "../MyrraGameModeBase.h"						// общие для данного уровня данные
#include "../Creature/MyrPhyCreature.h"					// ПОДОПЕЧНОЕ СУЩЕСТВО
#include "../UI/MyrBioStatUserWidget.h"					// модифицированный виджет со вложенными указателями на игрока, ИИ, контроллер
#include "../Artefact/MyrTriggerComponent.h"			// триггер объём
#include "AssetStructures/MyrCreatureGenePool.h"		// данные для всех существ этого вида
#include "AssetStructures/MyrCreatureBehaveStateInfo.h"	// данные по текущему состоянию моторики

//свой лог
DEFINE_LOG_CATEGORY(LogMyrDaemon);

//выполнить доморощенную тик-функцию
void FDaemonOtherTickFunc::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{	Target->LowPaceTick(DeltaTime);}
FString FDaemonOtherTickFunc::DiagnosticMessage() { return Target->GetFullName() + TEXT("[TickAction]"); }




AMyrraGameModeBase* AMyrDaemon::GetMyrGameMode()
{ return GetWorld()->GetAuthGameMode<AMyrraGameModeBase>(); }

//==============================================================================================================
//найти идентификатор реального действия (специфичный для этого существа) привязанный к условной кнопке
//==============================================================================================================
ECreatureAction AMyrDaemon::FindActionForPlayerButton(uint8 Button)
{
	if (Button > PCK_MAXIMUM) return ECreatureAction::NONE;
	auto BehaveSpecificActions = OwnedCreature->GetGenePool()->PlayerStateTriggeredActions.Find(OwnedCreature->GetBehaveCurrentState());
	if (BehaveSpecificActions) return BehaveSpecificActions->ByIndex(Button);
	else return OwnedCreature->GetGenePool()->PlayerDefaultTriggeredActions.ByIndex(Button);
}


//==============================================================================================================
// выделить память на компоненты, данные по умолчанию
//==============================================================================================================
AMyrDaemon::AMyrDaemon()
{
	//этот тик используется для управления замедленным движением (чо уж, каждый кадр для плавности)
	//также для движения камеры от первого к третьему лицу
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	//дополнительная тик-функция  (здесь всё, кроме регистрации)
	OtherTickFunc.bCanEverTick = true;
	OtherTickFunc.bTickEvenWhenPaused = true;
	OtherTickFunc.bStartWithTickEnabled = true;
	OtherTickFunc.Target = this;
	OtherTickFunc.SetTickFunctionEnable(true);
	OtherTickFunc.TickInterval = 0.35f;

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	RootComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Root"));
	RootComp->SetUsingAbsoluteScale(true);
	RootComponent = RootComp;
	RootComp->SetCollisionProfileName(TEXT("PawnTransparent"));
	RootComp->SetUsingAbsoluteRotation(true);	// чтобы не поворачивалось пространства вместе с существом

	// Create a follow camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	Camera->SetupAttachment(RootComponent);
	MoveCamera3p();
	Camera->bUsePawnControlRotation = false;


	//компонент персональных шкал здоровья и прочей хрени (висит над мешем)
	ObjectiveMarkerWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Objective Marker Widget"));
	ObjectiveMarkerWidget->SetupAttachment(RootComponent);
	ObjectiveMarkerWidget->SetGenerateOverlapEvents(false);
	ObjectiveMarkerWidget->SetUsingAbsoluteRotation(true);	// чтоб маркер прилипал к реальному месту
	ObjectiveMarkerWidget->SetVisibility(false);

	//источник эффекта частиц
	ParticleSystem = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Particles"));
	ParticleSystem->SetupAttachment(RootComponent);
	ParticleSystem->SetRelativeLocation(FVector(0, 0, 0));
	ParticleSystem->SetUsingAbsoluteRotation(true);

	//вспомогательный источник эффекта частиц
	SecondaryParticleSystem = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SecondaryParticles"));
	SecondaryParticleSystem->SetupAttachment(RootComponent);
	SecondaryParticleSystem->SetRelativeLocation(FVector(0, 0, 0));
	SecondaryParticleSystem->SetUsingAbsoluteRotation(true);
	SecondaryParticleSystem->SetUsingAbsoluteLocation(true);

	//звуки природы, окружения
	AmbientSounds = CreateDefaultSubobject<UAudioComponent>(TEXT("Ambient Sound"));
	AmbientSounds->SetupAttachment(Camera);
	AmbientSounds->SetRelativeLocation(FVector(0, 0, 0));

	//захват вида сцены
	CaptureTrails = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Scene Capture 2D"));
	CaptureTrails->SetupAttachment(RootComponent);
	CaptureTrails->ProjectionType = ECameraProjectionMode::Orthographic;
	CaptureTrails->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	CaptureTrails->CompositeMode = ESceneCaptureCompositeMode::SCCM_Overwrite;
	CaptureTrails->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	CaptureTrails->bCaptureOnMovement = false;
	CaptureTrails->bCaptureEveryFrame = false;	//будет вызываться тот же каждый кадр, но вручную
	CaptureTrails->bAutoActivate = true;
	CaptureTrails->bAlwaysPersistRenderingState = true;
	CaptureTrails->DetailMode = EDetailMode::DM_High;
	CaptureTrails->SetRelativeLocation(FVector(0, 0, -700));
	CaptureTrails->SetRelativeRotation(FRotator(90, 0, 90));
	//тут ещё надо ShowFlags устанавливать на одни Decals, но как-то мутно из кода это делается

	CaptureLighting = CreateDefaultSubobject<USceneCaptureComponentCube>(TEXT("Scene Capture Cube Lighting"));
	CaptureLighting->SetupAttachment(RootComponent);
	CaptureLighting->bCaptureOnMovement = false;
	CaptureLighting->bCaptureEveryFrame = false;	//будет вызываться изредка
	CaptureLighting->bAutoActivate = true;
	CaptureLighting->DetailMode = EDetailMode::DM_High;

	//битовое поле зажатых клавиш, для асинхронной проверки и установки режимов
	bMove = 0;
	bParry = 0;
	bClimb = 0;

	//в режиме от игрока, использовать направление взгляда/атаки из ИИ (для автонацеливания и поворота головы)
	UseActDirFromAI = 0;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

//==============================================================================================================
//после инициализации компонентов - важно, чтобы до BeginPlay
//==============================================================================================================
void AMyrDaemon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	//регистрация текущего на данный момент игрока во всей игре
	if (GetWorld())
		if (GetMyrGameMode())
			if(!GetMyrGameMode()->Protagonist)
				GetMyrGameMode()->Protagonist = this;

	//создание динамического рычажка для материала, которым рекуррировать следы
	if(GetMyrGameInst())
		if(GetMyrGameInst()->MaterialToHistorifyTrails)
			HistorifyTrailsMat = UMaterialInstanceDynamic::Create(GetMyrGameInst()->MaterialToHistorifyTrails, this);

}


//==============================================================================================================
// реакция на изменения в редакторе - подцеп к существу со сдвигом осуществляется явным выбором актора из списка
//==============================================================================================================
#if WITH_EDITOR
void AMyrDaemon::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if ((PropertyName == GET_MEMBER_NAME_CHECKED(AMyrDaemon, OwnedCreature)))
	{
		//привязать к правильному компоненту
		if (OwnedCreature) ClingToCreature(OwnedCreature);
	}
	else ParticleSystem->SetNiagaraVariableObject(TEXT("MyrDaemonBP"), this);
}
//==============================================================================================================
// вызывается, когда параметр уже изменен, но имеется старое значение параметра - правильно с ним расстаться
//==============================================================================================================
void AMyrDaemon::PreEditChange(FProperty* PropertyThatWillChange)
{
	if (PropertyThatWillChange->GetFName() == TEXT("OwnedCreature")) ReleaseCreature();
}
#endif

//==============================================================================================================
//переопределение -  связать органы управления извне и функции-отклики 
//в этой функции надо создать 2 ветви - с существом и свободное - соотвественно, привязывать разные функции
//==============================================================================================================
void AMyrDaemon::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	//если нет существа, все эти функции пасны, так как в них не проверяется наличие существа
	if (!OwnedCreature) return;

	//обычное движение через ждойстик
	PlayerInputComponent->BindAxis("MoveForward", this, &AMyrDaemon::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyrDaemon::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMyrDaemon::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMyrDaemon::LookUpAtRate);

	//колесо мыши - выбор варианта экспрессии, отдыха, расстояния камеры
	PlayerInputComponent->BindAxis("MouseWheel", this, &AMyrDaemon::MouseWheel);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AMyrDaemon::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AMyrDaemon::TouchStopped);


	//прыжок вперед, атака в прыжке (Space)
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMyrDaemon::JumpPress);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMyrDaemon::JumpRelease);

	//прыжок назад, отскок, уход от удара (Alt - надо заменить на Z или X)
	PlayerInputComponent->BindAction("Evade", IE_Pressed, this, &AMyrDaemon::AltPress);
	PlayerInputComponent->BindAction("Evade", IE_Released, this, &AMyrDaemon::AltRelease);

	//режим разворота боком и контратаки (Q)
	PlayerInputComponent->BindAction("Counter", IE_Pressed, this, &AMyrDaemon::SidePoseBegin);
	PlayerInputComponent->BindAction("Counter", IE_Released, this, &AMyrDaemon::SidePoseEnd);

	//режим поднятия и общего использования объектов (E)
	PlayerInputComponent->BindAction("Pick", IE_Pressed, this, &AMyrDaemon::PickStart);
	PlayerInputComponent->BindAction("Pick", IE_Released, this, &AMyrDaemon::PickEnd);

	//режим диалога и выражения эмоций (R)
	PlayerInputComponent->BindAction("Express", IE_Pressed, this, &AMyrDaemon::ExpressPress);
	PlayerInputComponent->BindAction("Express", IE_Released, this, &AMyrDaemon::ExpressRelease);

	//переход к отдыху (T)
	PlayerInputComponent->BindAction("Relax", IE_Pressed, this, &AMyrDaemon::RelaxPress);
	PlayerInputComponent->BindAction("Relax", IE_Released, this, &AMyrDaemon::RelaxToggle);

	// спринт (Shift)
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMyrDaemon::ShiftPress);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMyrDaemon::ShiftRelease);

	// стэлс и прочая пригнутость (Ctrl)
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AMyrDaemon::CtrlPress);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AMyrDaemon::CtrlRelease);

	// включить и выключить полёт (Z,X,C - хз пока)
	PlayerInputComponent->BindAction("Fly", IE_Pressed, this, &AMyrDaemon::FlyPress);
	PlayerInputComponent->BindAction("Fly", IE_Released, this, &AMyrDaemon::FlyRelease);

	//переключение альтернативной камеры, пока для теста 
	PlayerInputComponent->BindAction("CineCamera", IE_Pressed, this, &AMyrDaemon::CineCameraToggle);

	//переключение перового и третьего лица (Z, надо поменять опять на F, ибо редкая штука на важной клавише)
	PlayerInputComponent->BindAction("PersonToggle", IE_Pressed, this, &AMyrDaemon::PersonToggle);

	//обработка кнопок мыши - здесь надо переделать в местные функции,
	PlayerInputComponent->BindAction("Attack_R", IE_Pressed, this, &AMyrDaemon::Mouse_R_Pressed);
	PlayerInputComponent->BindAction("Attack_R", IE_Released, this, &AMyrDaemon::Mouse_R_Released);
	PlayerInputComponent->BindAction("Attack_L", IE_Pressed, this, &AMyrDaemon::Mouse_L_Pressed);
	PlayerInputComponent->BindAction("Attack_L", IE_Released, this, &AMyrDaemon::Mouse_L_Released);
	PlayerInputComponent->BindAction("Attack_M", IE_Pressed, this, &AMyrDaemon::Mouse_M_Pressed);
	PlayerInputComponent->BindAction("Attack_M", IE_Released, this, &AMyrDaemon::Mouse_M_Released);

}


//==============================================================================================================
// Called when the game starts or when spawned
//==============================================================================================================
void AMyrDaemon::BeginPlay()
{
	//если нет контроллера, всё остальное не нужно
	if (!Controller) return;

	// из-за этой хни нельзя второй тик полностью в конструкторе
	OtherTickFunc.RegisterTickFunction(GetLevel());

	//подстпудное
	Super::BeginPlay();

	//если нет подвязанного существа, всё остальное не нужно
	if (!OwnedCreature) return;

	//исключаем из расчёта освещенности себя
	CaptureLighting->HiddenActors.Add(OwnedCreature);

	//расстояние камеры от цели, зависит от размера цели, хотя лучше завести переменную в генофонде
	ThirdPersonDist = OwnedCreature->GetBodyLength()*2;
	MoveCamera3p();

	//закрепить камеру на правильной секции
	PoseInsideCreature();

	//здесь инициализируется экран боли
	SetFirstPerson(false);

	//сбросить счетчик, пока не собираемся выбывать
	PreGameOverLimit = 0;

	//очистить текстуры следов, а то в редакторе их сохраняет с предыдущего запуска
	//почему нельзя сделать вместо этой длинючки просто  Target->Clear()? Тупые, блин
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), CaptureTrails->TextureTarget, CaptureTrails->TextureTarget->ClearColor);
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), GetMyrGameInst()->TrailsTargetPersistent, GetMyrGameInst()->TrailsTargetPersistent->ClearColor);

	PreviousPosition = GetActorLocation();

	//запустить тряску камеры, она бесконечна и регулируется параметрами
	if (Shake.Get())	MyrController()->AddCameraShake(Shake);
	if (PainShake.Get())	MyrController()->AddPainCameraShake(PainShake);

	//разблокировать настройку контраста = его регулирует сонность
	Camera->PostProcessSettings.bOverride_ColorContrastMidtones = true;

	//для реакции на капли дождя
	ParticleSystem->SetNiagaraVariableObject(TEXT("MyrDaemonBP"), this);
}

//==============================================================================================================
//такт покадровый - фактический самый главный тик в игре, в нём обновляется вся субъективная мировая визуальность
//==============================================================================================================
void AMyrDaemon::Tick(float DeltaTime)
{
	//совсем базовое
	Super::Tick(DeltaTime);

	//проделать для неба покадрово самые интерактивные вещи
	//чтобы в небе не заводить 2 тика, покадровый и медленный
	if (Sky)
	{
		//здесь небо изменяет переменную влажности
		Sky->PerFrameRoutine(DeltaTime, Wetness);
	}

	//инстанция глобальных параметров материалов
	//нужна именно локальная переменная, поскольку новые значения отгружаются только в деструкторе
	if (!EnvMatParam) return;
	auto MPC = GetWorld()->GetParameterCollectionInstance(EnvMatParam);

	if(!GetMyrGameInst()) return;
	if(!GetMyrGameMode()) return;
	if(!OwnedCreature) return;

	//направление действий/атак/головы - куда смотрим, туда и направляем
	Drive.ActDir = (FVector3f)Controller->GetControlRotation().Vector();
	if (IsCameraOnExternalPoser()) Drive.ActDir = OwnedCreature->GetPelvis()->Forward;

	//усиление двоения в глазах при резкой боли - здесь нужно каждый кадр ибо боль скоротечна
	Camera->PostProcessSettings.SceneFringeIntensity = OwnedCreature->Pain;

	//инстанция глобальных параметров материалов
	//нужна именно локальная переменная, поскольку новые значения отгружаются только в деструкторе
	//auto MPC = GetMyrGameInst()->MakeMPCInst();
	// 
	//каждый кадр записывать для всех материалов локацию игрока
	//и в четвертый компонент еще и сырость, чтоб не создавать лишний параметр
	MPC->SetVectorParameterValue(TEXT("PlayerLocation"), FLinearColor(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z, Wetness));
	MPC->SetVectorParameterValue(TEXT("PlayerOffsetFromPreviousLocation"), GetActorLocation() - PreviousPosition);

	//подсчёт глобального пути главного героя (через буфер, чтобы заиметь хорошую точность при больших числах)
	DistWalkedAccum += FVector::Dist(GetActorLocation(), PreviousPosition);
	if(DistWalkedAccum >= 20)
	{	GetMyrGameInst()->Statistics.DistanceWalked += DistWalkedAccum;
		DistWalkedAccum = 0;
	}
	PreviousPosition = GetActorLocation();

	//применение расчёта следов
	UpdateTrailRender();

	////////////////////////////////////////////////////////////////////////////////////
	//если указан внешний источник позиции камеры - нас хотят видно в катстцену загнать
	if (ExternalCameraPoser && AllowExtCameraPosing)
	{
		//разложение значений позиции внешнего оператора камеры в координаты, гомологичные внутреннему представлению
		FVector TempDir;	float TempLngt;
		(this->GetActorLocation() - ExternalCameraPoser->GetComponentLocation()).ToDirectionAndLength(TempDir, TempLngt);

		//порог малости микса = полностью внешний контроль за камерой, простые вычисления
		if (CamExtIntMix < 0.01)
		{	CamExtIntMix = 0.0f;
			CamExtTargetVector = TempDir;
			CamExtTargetDist = TempLngt;
		}

		//нахождение промежуточного значения между текущим внешним местом и текущим внутренним
		//сам микс здесь стремится к нулю
		else
		{	CamExtTargetVector = FMath::Lerp(TempDir, GetControlRotation().Vector(), CamExtIntMix);
			CamExtTargetDist = FMath::Lerp(TempLngt, ThirdPersonDist, CamExtIntMix);
			CamExtIntMix *= 0.95;
		}
	}
	//если внешнего позёра камеры нет или он недавно исчез = возврат к управляемой мышью камере
	else
	{
		//в условиях пропажи внешнего конца микса - миксуем рекуррентно, с постоянной альфой
		//сам же микс для простоты растим линейно
		if (CamExtIntMix < 0.99)
		{	CamExtTargetVector = FMath::Lerp(CamExtTargetVector, GetControlRotation().Vector(), 0.1f);
			CamExtTargetDist = FMath::Lerp(CamExtTargetDist, ThirdPersonDist, 0.1f);
			CamExtIntMix += DeltaTime*2;
		}
		//достижение стабильности - упростить вычисления, досшли в конечную точку
		else
		{	CamExtIntMix = 1.0f;
			CamExtTargetVector = GetControlRotation().Vector();
			CamExtTargetDist = ThirdPersonDist;
		}
	}

	//если вошли в режим паузы связанной с меню
	if (MyrController()->IsNowUIPause())
	{
		//плавно снести уровень цветности до нуля (ЧБ)
		Camera->PostProcessSettings.ColorSaturation.W *= 0.8f;
	}
	
	//в режиме активной игры
	else 
	////////////////////////////////////////////////////////////////////////////////////
	//проверка подопечного существа на мертвость - переход на экран конца игры
	if (PreGameOverLimit > 0)
	{
		//выдержка, показываем мертвое тело еще какое-то время
		if (PreGameOverTimer < PreGameOverLimit)
		{
			//включение замедления времени
			Camera->PostProcessSettings.MotionBlurAmount = 20 * PreGameOverTimer;

			//включить функцию обесцвечивания экрана
			Camera->PostProcessSettings.bOverride_ColorSaturation = true;

			//плавно снести уровень цветности до нуля (ЧБ)
			Camera->PostProcessSettings.ColorSaturation.W *= 0.8f;

			PreGameOverTimer += DeltaTime;

			BaseTurnRate *= 1 - DeltaTime;
			BaseLookUpRate *= 1 - DeltaTime;

			//усилить слепоту
			HealthReactionScreen->SetScalarParameterValue(TEXT("Psychedelic"), FMath::Min(FMath::Max(Psychedelic, PreGameOverTimer/PreGameOverLimit), 1.0f));
			HealthReactionScreen->SetScalarParameterValue(TEXT("Desaturate"), FMath::Min(PreGameOverTimer, 1.0f));
		}
		else
		{
			//выход в меню через контроллер игрока, такая вот извращенная логика
			//а вообщездесь надо сначала выйти на уровень GameMode, оттуда решить что делать - кончить игру или перейти на новый уровень
			if (auto MyrPlayer = Cast<AMyrPlayerController>(Controller))
				MyrPlayer->CmdGameOver();
			return;

		}
	}
	else
	{
		//если есть градус упоротости
		if (Psychedelic > 0)
		{
			//упоротость передается в шейдер и снижается, чем больше здоровья и сил, тем быстрее
			HealthReactionScreen->SetScalarParameterValue(TEXT("Psychedelic"), Psychedelic);
			Psychedelic *= 1 - 0.01 * DeltaTime * (OwnedCreature->Health + OwnedCreature->Stamina + OwnedCreature->GetMetabolism());
			SetMotionBlur(Psychedelic * 10);
		}

		//размах тряски камеры
		if (MyrController())
		{
			if (MyrController()->GetCameraShake())
				MyrController()->GetCameraShake()->ShakeScale = Psychedelic;
			if (MyrController()->GetPainCameraShake())
				MyrController()->GetPainCameraShake()->ShakeScale = FMath::Clamp(OwnedCreature->Pain-0.9f, 0.0f, 2.0f);
		}

		//от сонности изображение становится блёклым
		Camera->PostProcessSettings.ColorContrastMidtones.W = 1.0f - Sleepiness*0.5f;

	}



	
	/////////////////////////////////////////////////////////////////////////////////
	//постоянная подстройка расстояния камеры
	//ещё здесь можно добавить смесь фиксированного угла и задаваемого мышкой
	switch (MyrCameraMode)
	{
		//##############################
		case EMyrCameraMode::FirstPerson:
			MoveCamera1p();
			break;

		//##############################
		case EMyrCameraMode::Transition31:

			//плавно приближать текущее отдаление к целевому
			CamDistNormFactor -= 0.05;

			//если добрались до конца - включить собственно первое
			if (CamDistNormFactor <= 0.05)
				SetFirstPerson(true);

			//самое главное, позиционирование камеры по выше расчитанным данным
			MoveCamera3p();
			break;

		//##############################
		case EMyrCameraMode::ThirdPerson:

			//честный расчёт дистанции камеры через поиск препятствий
			TraceForCamDist();

			//плавный выкат множителя расстояния камеры на уровень 
			CamDistOccluderAffect = FMath::Lerp(CamDistOccluderAffect, CamDistCurrentByTrace, CamDistRepulsion);

			//комбинированная целевая растяжка камеры, учёт огиба препятствий и жесткого задания объёмами
			float FinalCamDist = CamDistNewFactor * CamDistOccluderAffect;

			//пока не устаканилось приближение
			if (CamDistNormFactor != FinalCamDist)
			{
				//разность - мера дальности пути к целевому значению
				float Raznice = FMath::Abs(FinalCamDist - CamDistNormFactor);

				//если отбрык максимальный, важно недопустить погружения камеры в стену, поэтому сразу перейти к новой дистанции
				if (CamDistRepulsion >= 1) CamDistNormFactor = FinalCamDist; else

				//плавно приближать текущее отдаление к целевому - если разность большая, то медленно
				//чтобы скомпенсировать свойства лерпа сначала делать большой скачок а потом всё мельче и мельче
				CamDistNormFactor = FMath::Lerp(CamDistNormFactor, FinalCamDist, FMath::Lerp(0.01, 0.5, (1 - Raznice) * (1 - Raznice)));
				if (CamDistNormFactor > 1)
				{
					UE_LOG(LogTemp, Error, TEXT("Tick %s WTF CamDistNormFactor %g CamDistRepulsion %g"),
						*OwnedCreature->GetName(),
						CamDistNormFactor, CamDistRepulsion);
					CamDistNormFactor = 1;
				}

			}

			//самое главное, позиционирование камеры по выше расчитанным данным
			MoveCamera3p();
	}
}


//==============================================================================================================
//медленный тик
//==============================================================================================================
void AMyrDaemon::LowPaceTick(float DeltaTime)
{
	//похоже, что этот тик может быть вызван до динамической привязку существа
	if (!OwnedCreature) return;

	//подстройка звука проблем со здоровьем под актуальное здоровье ведомого существа
	//закоментарено, потому что пока непонятно, как будет выражен метаболизм в новом существе
	GetMyrGameMode()->SetSoundLowHealth(1.0f - OwnedCreature->Health, 1.0f - OwnedCreature->GetMetabolism(), OwnedCreature->Pain);

	//спецэффект низкого здоровья - переименовать, ведь это не эффект боли, а эффект агонии
	if (HealthReactionScreen)
		HealthReactionScreen->SetScalarParameterValue(TEXT("LowHealthAmount"), FMath::Clamp(1.2f - 1.1f * OwnedCreature->Health, 0.0f, 1.0f));

	//применение эффекта размытия, специфичного для данного состояния
	if (Camera->PostProcessSettings.bOverride_MotionBlurAmount)
		Camera->PostProcessSettings.MotionBlurAmount = FMath::Lerp(
			0.0f,
			OwnedCreature->GetBehaveCurrentData()->MotionBlurAmount,
			FMath::Clamp(OwnedCreature->StateTime, 0.0f, 1.0f));

	//вывод имени объекта в фокусе камеры (хз, нужно ли теперь)
	auto R = OwnedCreature->FindObjectInFocus(0.1, 0.3, ObjectAtFocus, ObjectNameAtFocus);

	//пока неясно, как нормально выставлять приращение сонности,
	Sleepiness += 0.001*DeltaTime;

	//пересчитать уровень освещенности для зрачков (освещенность камеры не подходит, так как если видны зрачки, камера смотрит в другую сторону)
	if(!IsFirstPerson())
	{
		//только если камера смотрит хоть чуть-чуть в сторону лица, если же на спину, то нах не нужно, ибо глаз не видно
		if( (OwnedCreature->GetLookingVector() | (FVector3f)GetControlRotation().Vector()) < 0 )
		{
			//собственно, пересчитать освещенность - тяжелая операция
			CaptureLighting->CaptureSceneDeferred();

			//перебрать пиксели и найти среднее, тоже нелегкая операция
			OwnedCreature->LightingAtView = GetLightingAtVector(OwnedCreature->GetLookingVector());
		}
	}

}


//==============================================================================================================
//проделура прикрепления к носителю с точки зрения демона (вызывается не сама, лишь отвечает на событие
//прикрепления корневого компонента - при спавне этого актора AttachTo
//==============================================================================================================
void AMyrDaemon::ClingToCreature(AActor* a)
{
	//если на вход подается правильный объект
	if (AMyrPhyCreature* Myr = Cast<AMyrPhyCreature>(a))
	{
		//сохранить указатель
		OwnedCreature = Myr;

		//обновить базис расстояния камеры согласно реальному размеру существа
		ThirdPersonDist = OwnedCreature->GetBodyLength() * 2;

		//привязаться и разместиться в иерархии подцепленного существа
		PoseInsideCreature();

		//привязать себя в управляемом объекте
		OwnedCreature->MakePossessedByDaemon(this);
		UE_LOG(LogTemp, Log, TEXT("ClingToCreature %s to %s"), *GetName(), *OwnedCreature->GetName());

		//посредством контроллера игрока переподключить новое существо к худ виджету
		if(MyrController()) MyrController()->ChangeWidgetRefs(OwnedCreature);

		//переопределить связки с клавишами
		if (InputComponent)
			SetupPlayerInputComponent(InputComponent);

		//переинициализировать лицо - там внутри экран боли, визуальные эффекты и много чего
		SetFirstPerson(false);

	}
}

//==============================================================================================================
//отделиться от существа
//==============================================================================================================
void AMyrDaemon::ReleaseCreature()
{
	//если мы владеем существом
	if (OwnedCreature)
	{
		//сначала удалить себя из памяти существа
		OwnedCreature->MakePossessedByDaemon(nullptr);

		//потом удалить существо из памяти себя
		OwnedCreature = nullptr;

		//переопределить связки с клавишами (пока не сработает, так как нет свободных вариантов)
		if (InputComponent)
			SetupPlayerInputComponent(InputComponent);
	}
}

//==============================================================================================================
//привязаться и разместиться в иерархии подцепленного существа
//==============================================================================================================
void AMyrDaemon::PoseInsideCreature(bool ResetCameraRot)
{
	//в скелете должен быть сокет, обычно в районе головы, иначе подвяжет к задним ногам
	AttachToComponent((USceneComponent*)OwnedCreature->GetMesh(), FAttachmentTransformRules::KeepWorldTransform, TEXT("HeadLook"));

	//соместить позицию с точнкой в голове
	if(ResetCameraRot) SetActorRelativeLocation(FVector(0));

	//если вызывать из редактора, то контролер будет отсутствовать
	if (Controller)
	{
		//типа обнулить
		if(ResetCameraRot) Controller->SetControlRotation(FRotator());

		//вектор движения сделать правильным до того, как само движение поимеет место
		Drive.MoveDir = (FVector3f)Controller->GetControlRotation().Vector();
	}

	Drive.MoveDir.Z = 0;
	Drive.MoveDir.Normalize();
	MoveCamera3p();
}

//==============================================================================================================
//двигать камеру согласно контроллеру
//==============================================================================================================
void AMyrDaemon::MoveCamera3p()
{
	//FQuat CamQ = FQuat(CamExtTargetVector, 0.0f);
	auto CamRot = FRotationMatrix::MakeFromXZ(CamExtTargetVector, FVector::UpVector).ToQuat();
	Camera->SetRelativeLocationAndRotation(
		CamRot.RotateVector(FVector(-CamDistNormFactor * CamExtTargetDist, 0, 0)),
		CamRot);
}
void AMyrDaemon::MoveCamera1p()
{
	Camera->SetRelativeLocationAndRotation(FVector(0, 0, 0), GetControlRotation());
}


//==============================================================================================================
// общая функция для захвата коэффициента движения (для WASD он дискретен, +1, -1)
//==============================================================================================================
void AMyrDaemon::ProcessMotionInput(float* NGain, float Value)
{
	//произошли изменения по какой-то из осей
	if (*NGain != Value)
	{
		//присвоить
		*NGain = Value;

		//если эти изменения выливаются в обнуление коэффициентов - движение прервалось
		if (XGain == 0.0f && YGain == 0.0f)
		{	bMove = 0;
			SendAction(PCK_MAXIMUM, false, ECreatureAction::TOGGLE_MOVE);
		}

		//иначе, если коэффициенты не нулевые, движение возобновилось или изменило силу/курс
		else
		{	SendAction(PCK_MAXIMUM, true, ECreatureAction::TOGGLE_MOVE);
			bMove = 1;
		}
	}
}



//==============================================================================================================
// сменить направление движения (из направляения взгляда камеры), получить все необходимые данные из рук игрока
// на вход актора существа. Покадрово, вызывается не само, а из тика существа - ИЗВРАТ, но синхрон
// надо максимально упростить здесь, чтобы само существо под себя преобразовывало вот это всё
//и все же вызывать из себя, дабы можно было летать без подопечного существа
//==============================================================================================================
bool AMyrDaemon::MakeMoveDir(bool MoveIn3D)
{
	//упрощать ротатор в кватернион бессмысленно - он изначально в контроллере ротатор
	//если нет резона летать в пространстве, одну из осей нужно сразу убрать, иначе сильные погрешности
	//нормирования при взгляде сверху вниз. Это неплатно, т.к. контроллер и так хранит углы в ротаторе
	FRotationMatrix44f RotationMatrix = FRotator3f(
		MoveIn3D ? Controller->GetControlRotation().Pitch : 0.0f,
		Controller->GetControlRotation().Yaw,
		0.0f);

	if (bMove)
	{
		//взвешенный вектор направления
		//Nota bene: для матрицы GetUnitAxis тяжелее, чем GetScaledAxis, ибо требует нормализации
		//а для трансформации, наоборот, GetUnitAxis проще, ибо не требует применения масштаба
		FVector3f DirToGo =
			RotationMatrix.GetScaledAxis(EAxis::X) * XGain +
			RotationMatrix.GetScaledAxis(EAxis::Y) * YGain;

		//вычисление единичного направления и коэффициента (затирание предыдущих)
		DirToGo.ToDirectionAndLength(Drive.MoveDir, Drive.Gain);

		//при ходе сразу вперед и вбок длина получается больше единицы, и это опасно
		if(Drive.Gain>1.0f) Drive.Gain = 1.0f;
	}
	//ВНИМАНИЕ: если оси движения не задействованы, направление движения всё равно считается по оси Х
	//это нужно, что не получить в этот вектор 0 и не обкакать ниже лежащие расчёты
	//но это бред, лучше в самом начале инициализировать и сохранять последний
	else
	{	
		//Drive.MoveDir = RotationMatrix.GetScaledAxis(EAxis::X);
		Drive.Gain = 0;
	}
	return true;
}


//==============================================================================================================
//новый способ определить расстояние до камеры -пока неясно, где лучше вызывать, посему отдельной функцией
//==============================================================================================================
void AMyrDaemon::TraceForCamDist()
{
#if WITH_EDITOR
	if (!CameraCollision) return;
#endif

	//ориентировочное расстоянияе камеры с учётом ужатия, продиктованого триггерами
	float RealCamDist = CamExtTargetDist * CamDistNewFactor;

	//заряжаем пушку
	FHitResult Hit;
	FVector Start = RootComp->GetComponentLocation();
	FVector End = RootComp->GetComponentLocation() - CamExtTargetVector * (CamSoftBallRadius + RealCamDist);

	//здесь имя, фолс - трассировать по простым объектам, а не по полигонам, зыс - текущий актор (демон) игнорировать
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("Daemon_TraceForCamera")), true, this);

	//помимо себя добавить еще подопечное существо
	RV_TraceParams.AddIgnoredActor(OwnedCreature);

	//трассируем из центра демона в позицию камеры и ещё дальше на радиус сферы сенсора препядствий
	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, RV_TraceParams);

	//поскольку Hit.Distance почему-то всегда ноль, хотя ничего не задевает, высчитываем расстояние по-другому
	float TrueDistance = Hit.Time * (RealCamDist + CamSoftBallRadius);

	//реакция на канал камеры - для отсеения случая когда трассировка обнаружила препятствие, но его совсем не надо огибать
	auto ObstacleToCam = ECollisionResponse::ECR_Ignore;
	auto ObstacleMob = EComponentMobility::Movable;
	if (Hit.Component.Get())
	{	ObstacleMob = Hit.Component->Mobility;
		ObstacleToCam = Hit.Component->GetCollisionResponseToChannel(ECollisionChannel::ECC_Camera);
	}

	//по оси камеры сфера всё же впячивается в стену - ставим целевую позицию на центр сферы минус ещё немного - чтобы не затмило плоскостю
	if (Hit.Time < 1.0f && ObstacleToCam != ECollisionResponse::ECR_Ignore)
		CamDistCurrentByTrace = (TrueDistance - CamSoftBallRadius - CamHardCoreRadius) / RealCamDist;
	else CamDistCurrentByTrace = 1.0f;

	//насколько глубоко шарик вошёл в стену по оси згляда (ProbeSize - чтобы отодвинуть мягкую зону от "орешка" вокруг камеры)
	//0 - значит орешек ксается стены; 1 - значит расстояние полное; -Х - значит перекрывает орешек и скорее всего обзор
	float UnPenetration = (TrueDistance - RealCamDist - CamHardCoreRadius) / (CamSoftBallRadius - CamHardCoreRadius);

	//камера не загораживается и сзади ничего не мешает - надо побыстрее вернуть нормальную оттяжку от тела
	if (UnPenetration >= 1)
	{	UnPenetration = 1;
		CamDistRepulsion = 0.2;
	}

	//камера все видит, но сзади уже давит какая-то стена - нужно упруго отодвигать
	//чем ближе к нулю тем сильнее упругая реакция отжима от стены
	else if (UnPenetration > 0)
	{
		//чем вертикальнее нормаль, тем быстрее отбрыкивать - способ борьбы с уходом под землю
		CamDistRepulsion = (1.5 - UnPenetration) * (0.01 + FMath::Max(Hit.Normal.Z, 0.0f));
		if (CamDistRepulsion > 1) CamDistRepulsion = 1.0f;
	}

	//UnPenetration<=0 орешек погружен в стену - надо срочно оттянуть камеру, чтоб не утопилась в толще
	else
	{
		//резкая подтяжка камеры сквозь неподвижные объекты
		if (ObstacleMob == EComponentMobility::Static)
		{
			CamDistRepulsion = 1.0f;
		}

		//плавная подтяжка камеры 
		else CamDistRepulsion = 0.1 + 0.2 * FMath::Min(-UnPenetration, 1.0f);
	}
	//if (CamDistRepulsion>1)
	//	UE_LOG(LogTemp, Error, TEXT("TraceForCamDist %s WTF CamDistRepulsion NaN"), *OwnedCreature->GetName());

}

//==============================================================================================================
//переместить камеру в положение строго за спиной
//==============================================================================================================
void AMyrDaemon::ResetCamera()
{
	//с катстценной камерой пока неясно, как, нужно ли
	if (ExternalCameraPoser) return;

	//отстоять от центра так, чтобы попадало точно в место головы, но было развернуто по направлению носа
	SetActorLocationAndRotation (OwnedCreature->GetHeadLocation(), OwnedCreature->GetActorRotation());
	FRotator R = OwnedCreature->GetActorRotation();
	R.Pitch = GetControlRotation().Pitch;
	Controller->SetControlRotation(R);
}


//==============================================================================================================
//установить величину размытия в движении, если надо, включить/выключить
//==============================================================================================================
void AMyrDaemon::SetMotionBlur(float Amount)
{
	if (Amount == 0.0f)
		Camera->PostProcessSettings.bOverride_MotionBlurAmount = false;
	else
	{	Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
		Camera->PostProcessSettings.MotionBlurAmount = 0.1;
	}
}

//==============================================================================================================
//установить величину глобального замедления времени, слоумо
//==============================================================================================================
void AMyrDaemon::SetTimeDilation(float Amount)
{	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), Amount); }

//==============================================================================================================
//подготовить экран для менюшной паузы - не используется, хз зачем это нужно
//==============================================================================================================
void AMyrDaemon::SwitchToMenuPause(bool Set)
{
	//включить функцию обесцвечивания экрана
	Camera->PostProcessSettings.bOverride_ColorSaturation = Set;
	auto& W = Camera->PostProcessSettings.ColorSaturation.W;
	if (!Set) W = 0.0f;
	else if (W > 0.4) W = 0.4;
}

//==============================================================================================================
//дискретный переключатель первого и третьего лица - в ответ на какую-нибудь кнопку
//==============================================================================================================
void AMyrDaemon::PersonToggle()
{
	//транзиция к первому лицу происходит независимо от настроек расстояния при 3ем лице,
	//чтобы не сбить установки, продиктованные окружением
	if (IsFirstPerson())
		SetFirstPerson(false);							
	else MyrCameraMode = EMyrCameraMode::Transition31;
}

//==============================================================================================================
//эксперимент - переключать камеру нормальную и типа киношную сбоку
//==============================================================================================================
void AMyrDaemon::CineCameraToggle()
{
	AllowExtCameraPosing = !AllowExtCameraPosing;
}

//==============================================================================================================
//по нажатию кнопки либо ничего (если мы в релаксе) или выбор меню состояний релакса и сна
//==============================================================================================================
void AMyrDaemon::RelaxPress()
{
	//не позволять мешать с процессом выбора экспрессий)
	if (!OwnedCreature) return;
	if (bExpress) return;

	//если деется релакс уже - даже не начинать
	if (OwnedCreature->DoesRelaxAction()) return;
	bRelaxChoose = true;

	//получить список доступных самодействий по теме выражения эмоций
	AvailableExpressActions.SetNum(0);
	OwnedCreature->ActionFindList(true, AvailableExpressActions);

	//если нашлось - сделать активной рандомную, чтобы если игрок ничего не выбирает, результат всё же различался
	if (AvailableExpressActions.Num() > 0)
		CurrentExpressAction = FMath::RandRange(0, AvailableExpressActions.Num() - 1);

	//отобразить меню (это все плетется в блюпринте худа)
	UE_LOG(LogTemp, Log, TEXT("RelaxPress %s %d expressions"), *GetName(), AvailableExpressActions.Num());
	HUDOfThisPlayer()->OnExpressionStart(true);

}

//==============================================================================================================
//включение и выключение состояния отдыха (сидя или лежа)
//==============================================================================================================
void AMyrDaemon::RelaxToggle()
{
	//снять режим
	if (!OwnedCreature) return;
	bRelaxChoose = false;

	//если нашли применимые редакс-действия, и если пока ни одно релакс действие не выполняется - запустить
	if (OwnedCreature->NoRelaxAction())
	{	if (AvailableExpressActions.Num() > 0)
			OwnedCreature->RelaxActionStart(AvailableExpressActions[CurrentExpressAction]);

		//скрыть меню в независимости от результатов поиска действий
		HUDOfThisPlayer()->OnExpressionRelease(true);
	}
	//если уже деется релакс - выводить из релакса
	else OwnedCreature->RelaxActionStartGetOut();

}




//==============================================================================================================
//команды выбора неатакового выражения эмоций
//==============================================================================================================
void AMyrDaemon::ExpressPress()
{
	//не позволять выбирать, если уже в процессе выбора релакса
	if (bRelaxChoose) return;

	//взвести режим
	bExpress = true;
	if (!OwnedCreature) return;

	//сразу отозвать все самодействия, которые выполнялись, чтоб не мешали
	OwnedCreature->SelfActionCease();

	//получить список доступных самодействий по теме выражения эмоций, false - самодействия, не релакс-действия
	AvailableExpressActions.SetNum(0);
	OwnedCreature -> ActionFindList (false, AvailableExpressActions, ECreatureAction::SELF_EXPRESS1);

	//если нашлось - сделать активной рандомную, чтобы если игрок ничего не выбирает, результат всё же различался
	if(AvailableExpressActions.Num()>0)
		CurrentExpressAction = FMath::RandRange(0, AvailableExpressActions.Num()-1);

	//отобразить меню (это все плетется в блюпринте худа)
	HUDOfThisPlayer()->OnExpressionStart(false);
}

void AMyrDaemon::ExpressRelease()
{
	//не позволять выбирать, если уже в процессе выбора релакса
	if (bRelaxChoose) return;

	//убрать режим
	bExpress = false;
	if (!OwnedCreature) return;

	//если есть, что выражать
	if(AvailableExpressActions.Num()>0)
	{
		//запустить (проверка наложения на уже проводимое действие - внутри)
		OwnedCreature -> SelfActionStart(AvailableExpressActions[CurrentExpressAction]);
	}
	//скрыть меню
	HUDOfThisPlayer()->OnExpressionRelease(false);
}


//==============================================================================================================
// явно задать направление атаки по оси контроллера игрока
//==============================================================================================================
void AMyrDaemon::SetAttackDir()
{
	//направление атаки взять из направления взгляда камеры
	OwnedCreature->AttackDirection = (FVector3f)Controller->GetControlRotation().Vector();

	//если направление атаки в зад (например смотрим камерой спереди)
	if ((-OwnedCreature->GetAxisForth() | OwnedCreature->AttackDirection) < 0)
	{
		//хитро развернуть
		OwnedCreature->AttackDirection *= -1;
	}
}

//==============================================================================================================
//загнать в текущую камеру настройки эффектов - при смене вида 1/3 лица
//==============================================================================================================
void AMyrDaemon::AdoptCameraVisuals(const FEyeVisuals& EV)
{
	//переприсвоение структуры с эффектами камеры и глобальный расстояний отсечения
	Camera->PostProcessSettings = EV.OphtalmoEffects;
	Camera->FieldOfView = EV.FieldOfView;
	GNearClippingPlane = EV.NearClipPlane;

	//установка материала для постэффектов
	//мы заранее не знаем, что это за материал, и для разных лиц и существ он может быть разный, 
	//но ссылка на него хранится в структуре PostProcessSettings в массиве WeightedBlendables
	if (Camera->PostProcessSettings.WeightedBlendables.Array.Num() > 0)
	{
		//ссылка на материал в структуре - статическая
		//нужно сделать из нее динамическую. непонятно, будет ли удаляться старая 
		auto* mat = Cast<UMaterialInterface>(Camera->PostProcessSettings.WeightedBlendables.Array[0].Object);
		HealthReactionScreen = UMaterialInstanceDynamic::Create(mat, this);

		//заменить в том же слоте статическую на динамическую
		Camera->PostProcessSettings.WeightedBlendables.Array[0].Object = HealthReactionScreen;

		//сразу же настроить параметр (надо бы переименовать, так как это не боль, а просто низкое здоровье)
		HealthReactionScreen->SetScalarParameterValue(TEXT("LowHealthAmount"), FMath::Min(1.0f - OwnedCreature->Health, 1.0f));
		HealthReactionScreen->SetScalarParameterValue(TEXT("Psychedelic"), 0);
	}
}


//==============================================================================================================
//установить уровень дождя и мокроты - вызывается из KingdomOfHeaven в редком тике
//==============================================================================================================
void AMyrDaemon::SetWeatherRainAmount(float Amount)
{
	//если мы в локации, которыая исключает дождь, 
	if(CurLocNoRain) Amount = 0;

	//вторичный источник дождя ваще обрать, если дождя нет, чтоб не мешался
	//а вот первичный не убирать, там еще насекомые и пушинки
	SecondaryParticleSystem->SetVisibility(Amount > 0.01);
	

	//регулирование плотности падающих капель дождя - без других условий
	ParticleSystem->SetFloatParameter(TEXT("RainAmount"), Amount);
	SecondaryParticleSystem->SetFloatParameter(TEXT("RainAmount"), Amount);
}

//==============================================================================================================
//установить уровень дождя и мокроты - вызывается из KingdomOfHeaven в редком тике
//==============================================================================================================
void AMyrDaemon::SetFliesAmount(float Amount)
{
	//регулирование количества мух, с учетом возможной локации, где их меньше
	ParticleSystem->SetFloatParameter(TEXT("FliesAmount"), Amount * (CurLocFliesMod / 255.0f));
}
//==============================================================================================================
//всё что связано с рендерингом текстуры шагов и следов на траве, воде и т.п.
//==============================================================================================================
void AMyrDaemon::UpdateTrailRender()
{
	//собственно, считать текущие следы
	CaptureTrails->CaptureSceneDeferred();

	//применить материал ко второму полотнищу, в материале уже будет задан источник копирования, то есть первый рендер таргет
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), GetMyrGameInst()->TrailsTargetPersistent, GetMyrGameInst()->MaterialToHistorifyTrails);
}

//==============================================================================================================
//переключение вида от первого и от третьего лица
//==============================================================================================================
void AMyrDaemon::SetFirstPerson(bool Set)
{
	//перейти к первому лицу
	if (Set)
	{
		//коэффициент близости к первому лицу
		CamDistNormFactor = 0.0;

		//полностью сменить настройки камеры на специфичные для существа
		AdoptCameraVisuals(OwnedCreature->GetGenePool()->FirstPersonVisuals);

		//чтобы не поворачивался внутрь шеи
		MyrController()->SetFirstPerson(true);

		//флаг
		MyrCameraMode = EMyrCameraMode::FirstPerson;

		//поставить камеру в нужную точку
		MoveCamera1p();

		//включить запахи
		SwitchToSmellChannel.Broadcast(AdvSenseChannel);

	}
	//перейти к третьему лицу
	else
	{
		//полностью сменить настройки камеры - вернуть глобальные
		auto GM = GetMyrGameMode();
		if(GM) AdoptCameraVisuals(GM->ThirdPersonVisuals);

		//убрать ограничения на поворот - эти действия проще делаются внутри контроллера
		if(MyrController())	MyrController()->SetFirstPerson(false);

		//флаг
		MyrCameraMode = EMyrCameraMode::ThirdPerson;

		//поставить камеру в нужную точку
		MoveCamera3p();

		//выключить запахи
		SwitchToSmellChannel.Broadcast(-1);
	}

	//дополнительно, если будут введены два сокета для разных лиц
	PoseInsideCreature();

	//перезапустить возможность вывода боли на экран в виде разкоряки цветов
	Camera->PostProcessSettings.bOverride_SceneFringeIntensity = true;
}


//==============================================================================================================
//найти подходящее действие для выбранных элементов управления
//пока не перенесено
//==============================================================================================================
void AMyrDaemon::SendAction(int Button, bool StrikeOrRelease, ECreatureAction ExplicitAction)
{
	//если действие не указано явно, найти его как соответствие кнопке
	if(ExplicitAction == ECreatureAction::NONE)
		ExplicitAction = FindActionForPlayerButton(Button);

	//вложить в движ, если место свободно
	if (Drive.DoThis == ECreatureAction::NONE)
	{	Drive.DoThis = ExplicitAction;
		Drive.Release = StrikeOrRelease;
	}
}

//==============================================================================================================
//наэкранный интерфейс игрока, управляющего этим демоном, может выдать нуль 
//==============================================================================================================
UMyrBioStatUserWidget* AMyrDaemon::HUDOfThisPlayer()
{
	return MyrController()->GetWidgetHUD();
}

//==============================================================================================================
//нужно для подсказки, на какую кнопку действие уровня существа повешено - может быть ни на какую - тогда -1
//==============================================================================================================
int AMyrDaemon::GetButtonUsedForThisAction(ECreatureAction Action)
{
	//распределение действий зависит от состояния поведения, так что сначала найти по текущему
	auto CurBehave = OwnedCreature->GetBehaveCurrentState();
	auto BehTrigAct = OwnedCreature->GetGenePool()->PlayerStateTriggeredActions.Find(CurBehave);

	//если отдельной разнарядки для текущего не предсумотрено - взять поумолчанию
	if (!BehTrigAct) BehTrigAct = &OwnedCreature->GetGenePool()->PlayerDefaultTriggeredActions;

	//а дальше тупо перебором
	for (int i = 0; i < PCK_MAXIMUM; i++)
		if (BehTrigAct->ByIndex(i) == Action)
			return i;
	return -1;
}


//==============================================================================================================
//зменение расстояния камеры третьего лица - как-то слишком сложно, вероятно ввести бинарное переключение
//бинароне переключение введено, однако проблема плавного перехода между позициями камеры остаётся
//==============================================================================================================
void AMyrDaemon::MouseWheel(float value)
{
	//режим выбора варианта что сказать
	if(bExpress || bRelaxChoose)
	{
		if (value < 0 && CurrentExpressAction < AvailableExpressActions.Num() - 1)
		{	CurrentExpressAction++;
			HUDOfThisPlayer()->OnExpressionSelect(bRelaxChoose, CurrentExpressAction);
		}
		if (value > 0 && CurrentExpressAction > 0)
		{	CurrentExpressAction--;
			HUDOfThisPlayer()->OnExpressionSelect(bRelaxChoose, CurrentExpressAction);
		}
		return;
	}

	//это двигать камеру для эксперимента и отладки
	if (MyrCameraMode == EMyrCameraMode::ThirdPerson)
	{
		//просто присвоить опору, а дальше в тике само
		if (value > 0 && CamDistNewFactor < 1)	CamDistNewFactor += 0.05;
		if (value < 0 && CamDistNewFactor > 0)	CamDistNewFactor -= 0.05;
	}
	//от первого лица - менять каналы запаха
	else
	{	if (value > 0 && AdvSenseChannel < 16)	{ AdvSenseChannel++; SwitchToSmellChannel.Broadcast(AdvSenseChannel);	}
		if (value < 0 && AdvSenseChannel > 0)	{ AdvSenseChannel--; SwitchToSmellChannel.Broadcast(AdvSenseChannel);	}
	}
}
float AMyrDaemon::ResetCameraPos()
{	if (OwnedCreature->Overlap0)
		CamDistNewFactor = OwnedCreature->Overlap0->GetCameraDistIfPresent();
	else CamDistNewFactor = 1.0f;
	return CamDistNewFactor;
}
bool AMyrDaemon::IsFirstPerson() { return (MyrCameraMode == EMyrCameraMode::FirstPerson); }

//==============================================================================================================
//поместить или удалить маркер квеста - извне
//==============================================================================================================
void AMyrDaemon::PlaceMarker(USceneComponent* Dst)
{
	ObjectiveMarkerWidget->SetVisibility(true);
	ObjectiveMarkerWidget->AttachToComponent(Dst, FAttachmentTransformRules::KeepWorldTransform);
	ObjectiveMarkerWidget->SetRelativeLocation(FVector(0));

}

//==============================================================================================================
//поместить или удалить маркер квеста - извне
//==============================================================================================================
void AMyrDaemon::RemoveMarker()
{
	ObjectiveMarkerWidget->SetVisibility(false);
	ObjectiveMarkerWidget->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	ObjectiveMarkerWidget->SetRelativeLocation(FVector(0));

}

//==============================================================================================================
//найти уровень освещения по заданному вектору взгляда
//==============================================================================================================
float AMyrDaemon::GetLightingAtVector(FVector3f V)
{
	float Accum = 0.0f;
	TArray<FFloat16Color> ColorBuffer;
	FTextureRenderTargetCubeResource* textureResource = 
		(FTextureRenderTargetCubeResource*)CaptureLighting->TextureTarget->GetResource();

	//если вектор нулевой, то ищется средняя освещенность по всем сторонам, хз, нужно ли так брутфорсно
	if(V.X == 0 && V.Z == 0)
	{
		for(int i=0;i<6;i++)
		{	if(textureResource->ReadPixels (ColorBuffer, FReadSurfaceDataFlags(RCM_MinMax, (ECubeFace)i)))
				for (auto Pi : ColorBuffer)
					Accum += Pi.R + Pi.G + Pi.B;
		}
		Accum /= 6 * 3 * ColorBuffer.Num();
	}

	//если вектор единичный, то берется один квадрат куба в нужной стороне
	else
	{
		ECubeFace Whether = ECubeFace::CubeFace_PosX;
		if(V.X >  0.72) Whether = ECubeFace::CubeFace_PosX; else
		if(V.X < -0.72) Whether = ECubeFace::CubeFace_NegX; else
		if(V.Y >  0.72) Whether = ECubeFace::CubeFace_PosY; else
		if(V.Y < -0.72) Whether = ECubeFace::CubeFace_NegY; else
		if(V.Z >  0.72) Whether = ECubeFace::CubeFace_PosZ; else
		if(V.Z < -0.72) Whether = ECubeFace::CubeFace_NegZ;
		if(textureResource->ReadPixels (ColorBuffer, FReadSurfaceDataFlags(RCM_MinMax, Whether)))
		{
			//тут пока неясно, стоит ли искать средний, или все же максимальный уровень яркости
			for (auto Pi : ColorBuffer)
				Accum += Pi.R + Pi.G + Pi.B;
			Accum /= 3 * ColorBuffer.Num();
		}
	}
	return Accum;
}

//среагировать на удар капли по земле
void AMyrDaemon::ReactOnRainDrop(float Size, FVector Position, FVector Velocity)
{
	//радиус вектор - если удар произошёл на метр и выше нас - значит то была крыша и декал создавать не надо
	FVector Ra = GetActorLocation() - Position;
	if (Ra.Z < -100.0f) return;

	//участок кмокроты
	if (WetDecalMat)
	{	auto Decal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), WetDecalMat, FVector(50, 50, 150), Position, FRotator(), 4);
		Decal->SetFadeIn(0, 1);
		Decal->SetFadeOut(0, 2);
	}

	if (Ra.SizeSquared2D() < 10000)
		SecondaryParticleSystem->SetWorldLocation(Position);

	//запустить дождь дополнительный для густоты
	//if (SecondaryRain && Ra.SizeSquared2D() < 10000)
	//{
	//	auto Rain = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SecondaryRain, Position, FRotator(), FVector(1, 1, 1), true);
	//}


	//промокание
	if (Wetness < 1.0f)
	{
		float Affect = 10000 / FVector::DistSquared(GetActorLocation(), Position);
		Wetness = FMath::Min(1.0f, Wetness + Affect);

		UE_LOG(LogTemp, Log, TEXT("DAEMON %s: ReactOnRainDrop Wetness + %g = %g"), *GetName(), Affect, Wetness);
	}
	else Wetness = 1.0f;
}
