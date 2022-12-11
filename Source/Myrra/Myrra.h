// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Map.h"


//дополнительные математтические функции в том числе лерп в обе стороны
#include "BaseDefinitions/AdvMath.h"

//поверхности (энум, особенности хождения, звуки, брызги)
#include "BaseDefinitions/myrra_surface.h"

//состояния/режимы поведения - основа управления существом
#include "BaseDefinitions/myrra_behave.h"

//тупые определения для перевода текста в позы рта
#include "BaseDefinitions/myrra_lipsync.h"

//эмоции/цвета + отношения к объектам
#include "BaseDefinitions/myrra_emotions.h"
#include "BaseDefinitions/myrra_emotio.h"

//анатомия - членики и сегменты тела
#include "BaseDefinitions/myrra_anatomy.h"

//действия/атаки, их фазы, результаты (но не полные свойства)
#include "BaseDefinitions/myrra_actions.h"

//базовые типы связанные с ИИ, но востребованные глубже
#include "BaseDefinitions/myrra_ai.h"

//сборка параметров опыта + прокачка опыта вещь в себе
#include "BaseDefinitions/myrra_roleparams.h"

//новая сборка высокой логики игры, в том числе прокачка
#include "BaseDefinitions/myrra_gameplay.h"

//новая сборка высокой логики игры, в том числе прокачка
#include "BaseDefinitions/myrra_food.h"

#include "Myrra.generated.h"

#if WITH_EDITOR
#define DEBUG_LINE_TRACES 0
#define DEBUG_LINE_ATTACKS 0
#define DEBUG_LINE_AXES 0
#define DEBUG_LINE_CONTACTS 0
#define DEBUG_LINE_STEPS 0
#define DEBUG_LINE_MOVES 1
#define DEBUG_LINE_CONSTRAINTS 0
#define DEBUG_LINE_AI 1
#define DEBUG_LINE_ANIMATION 0
#define DEBUG_LOG_SKY 0
#else
#define DEBUG_LINE_TRACES 0
#define DEBUG_LINE_AXES 0
#define DEBUG_LINE_ATTACKS 0
#define DEBUG_LINE_CONTACTS 0
#define DEBUG_LINE_STEPS 0
#define DEBUG_LINE_MOVES 0
#define DEBUG_LINE_CONSTRAINTS 0
#define DEBUG_LINE_AI 0
#define DEBUG_LINE_ANIMATION 0
#define DEBUG_LOG_SKY 0
#endif

//сокращалка для вывода в лог имён перечислителей как строк
#define TXTENUM(type,var) FindObject<UEnum>(ANY_PACKAGE, TEXT(# type))->GetNameStringByValue((int)var)

//ось, которой надо ориентироваться по нужному направлению для совершения некоего действия
UENUM(BlueprintType) enum class EMasterAxis : uint8 { FRONT, BACK, LEFT, RIGHT, SIDE, TIP, OMNI, NONE };

//делегат для вкл выкл показа запаха
DECLARE_MULTICAST_DELEGATE_OneParam(FSwitchToSmellChannel, int)



//режим положение камеры, определяет также эффекты движения, постэффекты и т.п.
UENUM(BlueprintType) enum class EMyrCameraMode : uint8
{
	ThirdPerson,
	Transition13,
	Transition31,
	FirstPerson
};

//новая струкрура для непрерывного управления существом из демона или ИИ
//здесь самодостаточная сборка с указанием куда двигаться, куда смотреть, с какой силой, какое желательно состояние
USTRUCT(BlueprintType) struct FCreatureDrive
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector3f MoveDir;			// курс движения
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector3f ActDir;				// курс взгляда и/или удара в атаке
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Gain = 0.0f;			// сила тяги к движению
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECreatureAction DoThis;		// команда (простая или сборочная) которую вот прям щас выполнить
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Release = false;		// тру = отжатие кнопки, для атак переход к удару, для переключателей режимов - выключение, возврат
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool NeedCheck = false;		// нужно ли дополнительно проверять применимость действия-ассета, или контроллер уже сам его проверил

	FCreatureDrive()
	{	MoveDir = FVector3f(0, 0, 0);
		ActDir = FVector3f(1, 0, 0);
		Gain = 0; 
		DoThis = ECreatureAction::NONE;
	}
};

//стандартные части дерева в мнемонической форме, могут использоваться как биты присутствия секции
UENUM(BlueprintType) enum class EDendroSection : uint8
{
	Central_Lower,
	Central_Mid_1,
	Central_Mid_2,
	Central_Mid_3,
	Central_Mid_4,
	Central_Mid_5,
	Central_Mid_6,
	Central_Top
};





//###################################################################################################################
//выдать данные о съедобности объекта (заместо пихдохуёбанных через жопу интерфейсов)
//глобальная функция, потому что съедобными могут быть разные классы
FDigestiveEffects* GetDigestiveEffects(AActor* Obj);

//аналогично выдать человеческое локализуемое имя
FText GetHumanReadableName(AActor* Whatever);
//###################################################################################################################


//								███████   ███████   ███████   
//								███████   ███████   ███████    
//								███████   ███████   ███████   НАВЫКИ, РОЛИ И ДАННЫЕ ДЛЯ СОХРАНЕНИЯ ИГРЫ
//								███████   ███████   ███████   
//								███████   ███████   ███████    





//###################################################################################################################
// достаточный данных для сохранения души каждого из существ при сохранении игры. Skill Levels просто копируется
// координаты копируются из корня, в общем процес сборки этой структуры из живого MyrCreature - отдельная процедура
//###################################################################################################################
USTRUCT(BlueprintType) struct FSocialMemory 
{
	GENERATED_USTRUCT_BODY()
	FName Key; FGestaltRelation Value;
};
USTRUCT(BlueprintType) struct FCreatureSaveData
{
	GENERATED_USTRUCT_BODY()

	//скаляры состояния - 
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	uint8 Health;			
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	uint8 Stamina;			
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	uint8 Energy;			
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	FName Name;				

	//номер текстуры шкуры/расцветки
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	uint8 Coat;

	//класс конкретный существа - нужно для тех, что нет на уровне, который спавнились динамически
	//хз как этот указатель сохраняется, но вроде так можно
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	UClass* CreatureClass;


	//самое главное, абсолютное положение в пространстве
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	FTransform Transform;	

	//урон по частям тела (сделать тоже целочисленными?)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	TArray<float> Damages; 

	//все текущие способности данного существа - 
	//безликий массив потому что дабл не поддержиается и приходится вручную сериализовывать
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	FPhenotype Phenotype;

	//компактное представление памяти ИИ о встреченных существах
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) TArray<FSocialMemory> AIMemory;




	//здесь также нужны список удерживаемых предметов (идентификаторы?) и вообще тайник/инвентарь (как?)
	//и, главное, стадия сюжета, в которой это существо задействовано. Если сюжет нелинейный, это вообще
	//вырождается в список квестов, причем не только для PC, просто РС может войти во все квесты, а NPC
	//только в те, которые заранее для него предусмотрены. Вероятно, проще развернуть наоборот и сохранять 
	//сюжет вне этого массива, в отдельной структуре, и там уже указывать, какие персонажи на какой стадии
	//там здаействованы

	//старое
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	TArray<FBODY_BIO_DATA> BodyBioData; // здоровье по членикам
};

//###################################################################################################################
// массив данных для сохранения предметов
//###################################################################################################################
USTRUCT(BlueprintType) struct FArtefactSaveData
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	FName Name;				// хз, может, не нужно, может лучше ID
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	FTransform Transform;	// самое главное, положение в пространстве
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	FName Owner;			// владелец, что бы это не значило
};

//###################################################################################################################
// массив данных для сохранения предметов
//###################################################################################################################
USTRUCT(BlueprintType) struct FLocationSaveData
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	FName Name;				// хз, может, не нужно, может лучше ID
	TArray<uint8> ComponentStates;											// пока неясно, как лучше кодировать
	//пока неясно, стоит ли сохранять принадлежности или они сами при загрузке тронут триггер и зарегистрируются
};

//###################################################################################################################
// сохранение квеста
//###################################################################################################################
USTRUCT(BlueprintType) struct FQuestSaveData
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)	FName Name;		// имя
	TArray<FName> PassedStates;										// список пройденных этапов в правильной последовательности
};


//								███████   ███████   ███████   ███████   
//								███████   ███████   ███████   ███████    
//								███████   ███████   ███████   ███████   СВОЙСТВА КЛАССА СУЩЕСТВ   
//								███████   ███████   ███████   ███████   
//								███████   ███████   ███████   ███████    



//###################################################################################################################
//агрегатор настроек для камеры
//###################################################################################################################
USTRUCT(BlueprintType) struct FEyeVisuals
{
	GENERATED_USTRUCT_BODY()

	//настройки спецэффектов, связанные со зрением этого существа, действуют от первого лица
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FPostProcessSettings OphtalmoEffects;

	//поле зрения для камеры
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FieldOfView = 90.f;

	//передняя отсекающая плоскость
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float NearClipPlane = 10.f;
};



//								███████   ███████   ███████   ███████   
//								███████   ███████   ███████   ███████    
//								███████   ███████   ███████   ███████   Интерфейс   
//								███████   ███████   ███████   ███████   
//								███████   ███████   ███████   ███████    


//###################################################################################################################
// РЕЖИМ интерфейса - что поверх игры и как этим управлять - агрегатор для набирания множества простых режимов
// и редактирования в редакторе. Почему то рушит стек при инициализации в TMap
//###################################################################################################################
USTRUCT(BlueprintType) struct FInterfaceMode
{
	GENERATED_USTRUCT_BODY()

	// виджет, что застилает экран
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	TSubclassOf<UUserWidget> Widget;

	// текстовое называние меню, которое открывает виджет
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FText HumanReadableName;

	// 1 - ГУЙ, управляется мышкой
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	bool UI_notHUD;

	// замедлитель времен, если 0 - ваще пауза
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float TimeFlowMod;		

public:

	//конструктор
	UUserWidget* GetWidget() { return Cast<UUserWidget>(Widget);  }

	//хз, чтоб битовое поле заполнить
	FInterfaceMode() { UI_notHUD = 0; TimeFlowMod = 1.0; }

	//внести все простые данные, виджет придётся заполнять в блюпринте/редакторе
	FInterfaceMode (bool UI, float Tf) { UI_notHUD = UI; TimeFlowMod = Tf; }
};


//режим положение камеры, определяет также эффекты движения, постэффекты и т.п.
UENUM(BlueprintType) enum class EUIEntry : uint8
{
	NONE,
	Start,
	Pause,
	GameOver,
	GameComplete,
	Continue,
	NewGame,
	LoadLast,
	QuickSave,
	Quests,
	Stats,
	Saves,
	Known,
	EmoStimuli,
	Phenes,
	Options,
	Authors,
	Quit,
	MAX
};

USTRUCT(BlueprintType) struct FUIEntryData
{
	GENERATED_USTRUCT_BODY()

	// виджет, что вываливается при выборе этого пункта меню
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	TSubclassOf<UUserWidget> Widget;

	// текстовое называние меню, которое открывает виджет
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	FText HumanReadableName;

	// можно ли закрыть и продолжить игру или тупиковый
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	UMaterialInterface* BackgroundMaterial;

	//если вызов этой функции открывает меню, то какие еще строки меню в нем содержатся
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = EUIEntry)) int32 AsMenu = 0;

	FUIEntryData() {}
	FUIEntryData(FString InText, int32 InSet) : AsMenu(InSet) { HumanReadableName = FText::FromString(InText); }
};

//сборщик статистики
USTRUCT(BlueprintType) struct FGameStats
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	int32 DistanceWalked = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)	int32 AttacksMade = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	int32 SuccessfulHitsMade = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float DamageCollected = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)	int32 FoodEaten = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	int32 SleepPeriods = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	float TotalTimeSleeping = 0;

};

//###################################################################################################################
// набор опций
//###################################################################################################################
UENUM(BlueprintType) enum class EMyrOptions : uint8
{
	VSync,
	Screen,
	FrameRate,
	ViewDist,
	Antialiasing,
	Shading,
	Textures,
	Shadows,
	PostProc,
	VisualEffects,

	GRAPHMAX,

	SoundAmbient,
	SoundSubjective,
	SoundNoises,
	SoundVoice,
	SoundMusic,

	AUDIOMAX
};
