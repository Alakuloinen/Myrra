#pragma once
#include "CoreMinimal.h"
#include "AdvMath.h"
#include "myrra_surface.h"
#include "myrra_anatomy.generated.h"

//номер членика в скелетной опоре, когда она вовсе не скелетная
#define NO_BODY -127

#define BI_QUA(a,b) (((uint8)(a)) + (((uint8)(b))<<4))

//константы для сегментов тела (хардкод для позвоночных, но остальные могут их использовать в нестандартном смысле)
UENUM() enum class ELimb : uint8
{ 
	PELVIS,
	LUMBUS,
	PECTUS,
	THORAX,
	HEAD,
	L_ARM,
	R_ARM,
	L_LEG,
	R_LEG,
	TAIL,
	NOLIMB
};

//###################################################################################################################
//каким образом использовать каркасы-поддержки частей тела для движения и поддержания
//реализованно голым перечислителем, чтобы не таскать "ELongLongEnumName::A"
//###################################################################################################################
//тип динамики части тела - число для отображения в редакторе
UENUM(BlueprintType) enum class ELDY : uint8
{
	ToDoPull			UMETA(DisplayName = "Pull/Push, "),
	ToDoRotate			UMETA(DisplayName = "Rotate/Roll"),
	WithMyWheel			UMETA(DisplayName = "- With My Wheel (Roll)"),
	WithMyFront			UMETA(DisplayName = "- With Local Front"),
	WithMyUp			UMETA(DisplayName = "- With Local Up"),
	WithMyLateral		UMETA(DisplayName = "- With Local Lateral"),
	TowardsCourse		UMETA(DisplayName = "- Towards Move Direction"),
	TowardsAttack		UMETA(DisplayName = "- Towards Attack/View"),
	TowardsVertical		UMETA(DisplayName = "- Towards Global Up Vector"),
	TowardsGirdleNormal UMETA(DisplayName = "- Towards Girdle/Local Up Vector"),
	ReverseVector		UMETA(DisplayName = "/ Reverse `Towards` Vector"),
	ReverseMyVector		UMETA(DisplayName = "/ Reverse `With Local` Vector"),
	ApplyDampingQ1		UMETA(DisplayName = "+ 1/4 damping"),
	ApplyDampingQ2		UMETA(DisplayName = "+ 1/4 damping"),
	ApplyDampingH		UMETA(DisplayName = "+ 1/2 damping"),
	Gravity				UMETA(DisplayName = "+ GRAVITY"),
	ExtGain				UMETA(DisplayName = "+ Only If Move Gain"),
	NoExtGain			UMETA(DisplayName = "+ Only If No Move Gain"),
	OnGround			UMETA(DisplayName = "+ Only If On Ground"),
	NotOnGround			UMETA(DisplayName = "+ Only If Off Ground"),
};

//стандартный перечислитель, не анриловский, так как андрил не понимает перечислители-классы больше 8 бит
ENUM_CLASS_FLAGS(ELDY); enum LDY
{
	LDY_PASSIVE = 0,

	LDY_PULL = 1 << (int)ELDY::ToDoPull,
	LDY_ROTATE = 1 << (int)ELDY::ToDoRotate,
	LDY_MOTORICS = LDY_PULL | LDY_ROTATE,

	LDY_MY_WHEEL = 1 << (int)ELDY::WithMyWheel,
	LDY_MY_FRONT = 1 << (int)ELDY::WithMyFront,
	LDY_MY_UP = 1 << (int)ELDY::WithMyUp,
	LDY_MY_LATERAL = 1 << (int)ELDY::WithMyLateral,
	LDY_MY_AXIS = LDY_MY_FRONT | LDY_MY_UP | LDY_MY_LATERAL,

	LDY_TO_COURSE = 1 << (int)ELDY::TowardsCourse,
	LDY_TO_ATTACK = 1 << (int)ELDY::TowardsAttack,
	LDY_TO_VERTICAL = 1 << (int)ELDY::TowardsVertical,
	LDY_TO_NORMAL = 1 << (int)ELDY::TowardsGirdleNormal,
	LDY_TO_AXIS = LDY_TO_COURSE | LDY_TO_VERTICAL | LDY_TO_NORMAL | LDY_TO_ATTACK,

	LDY_REVERSE = 1 << (int)ELDY::ReverseVector,
	LDY_REVERSE_MY = 1 << (int)ELDY::ReverseMyVector,

	LDY_DAMPING_4_1 = 1 << (int)ELDY::ApplyDampingQ1,
	LDY_DAMPING_4_2 = 1 << (int)ELDY::ApplyDampingQ2,
	LDY_DAMPING_2_1 = 1 << (int)ELDY::ApplyDampingH,
	LDY_DAMPING = LDY_DAMPING_4_1 | LDY_DAMPING_4_2 | LDY_DAMPING_2_1,

	LDY_GRAVITY = 1 << (int)ELDY::Gravity,
	LDY_EXTGAIN = 1 << (int)ELDY::ExtGain,
	LDY_NOEXTGAIN = 1 << (int)ELDY::NoExtGain,
	LDY_ONGROUND = 1 << (int)ELDY::OnGround,
	LDY_NOTONGROUND = 1 << (int)ELDY::NotOnGround

};

//ориентация конкретной чачсти тела или группы частей тела (чтобы по-разному экспортированные кости можно было двигать в понятных направлениях)
USTRUCT(BlueprintType) struct FLimbOrient
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EMyrAxis Forth = EMyrAxis::Yn;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EMyrAxis Right = EMyrAxis::Xn;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EMyrAxis Up = EMyrAxis::Zn;

	//получение абсолютной оси по константе относительной словесной 
	//если ось инверсная, помимо выкорчевывания поля из этой структуры по номеру ещё и модификация его бита, отвечающего за знак
	EMyrAxis ByEnum(EMyrRelAxis A)
	{
		auto R = ((EMyrAxis*)this)[((uint8)A) & 3];
		if ((int)A <= (int)EMyrRelAxis::_MaxNonInv) return R;
		else return AntiAxis(R);
	}
};

//по каким осям поясница и голова способны крутиться - для оптимизации, чтобы исключать расчёт арксинусов
USTRUCT(BlueprintType) struct FBodyFreedom
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 HeadLeftRight : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 HeadUpDown : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 HeadTwist : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 SpineLeftRight : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 SpineUpDown : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 SpineTwist : 1;
};

//###################################################################################################################
// дополнительные данные к физическому членику - части тела и их здоровье, которое вычисляется так:
// - если счётчик времени на восстановление не заполнялся / опустел до нуля - членик полностью цел и здоров
// - если счётчик достиг базового коэффициента - здоровье=0, членик выбит из строя, но уже начинает восстанавливаться
// - если счётчик взведён выше базового коэффициента, здоровье=0, и членик будет висеть еще долго, пока счётчик
// не достигнет базового коэффициента... фактически здоровье отрицательное, и при суммировании общего здоровья 
// включается со знаком минус, то есть если один членик глубоко подбит, а остальные совершенно здоровы, то суммарное
// здоровье опустится ниже нуля, и всё тело падёт
//###################################################################################################################
USTRUCT(BlueprintType) struct FBODY_BIO_DATA
{
	GENERATED_USTRUCT_BODY()

	// отношение данного членика - к какому сегменту тела он принадлежит
	UPROPERTY(VisibleAnywhere) ELimb eLimb = ELimb::PELVIS;

	// актуальный счётчик времени восстановления - из него вычитается реальное время
	// переименовать во что-то более гибкое, без привязки ко времени, возможно даже целочисленный
	UPROPERTY(VisibleAnywhere) float DamageWeight = 0.0f;

	//уровень листовости (если лист, то 0, и далее по иерархии) чтобы сравнивать дальность члеников в сегменте
	//это надо заполнять до игры, при разборе физ.лошарика, проще всего ввести вручную в строку имен члеников 
	uint8 DistFromLeaf = 0;

	// этот членик никогда не опасен, даже если опасен весь его макросегмент (зачем?)
	uint8 NeverDangerous : 1;
};

//свойства каркасного узла
USTRUCT(BlueprintType) struct FMachineLimbAnatomy
{
	GENERATED_USTRUCT_BODY()

	//номер финального членика, для этого скелета - вероятно, единственный членик
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, config) uint8 TipBodyIndex = 255;

	//номер констрейнта, которым привязан финальный членик
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, config) uint8 TipConstraintIndex = 255;

	//уровень прочности на удар - множитель максимальной взаимной скорости
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) float HitShield = 1.0f;

	//важность касания этой части тела об стену/пол для определения ориентации тела, 1.0 = шаг ногой, у других тел меньше
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) float TouchImportance = 1.0f;

	//максимальная угловая скорость, прямо вводится в физику, если ноль, используются значения по умолчанию
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) float MaxAngularVelocity = 0.0f;

	//может ли этот членик физ-привязью привязываться к окружению (лазанье) или к другому существу (хватание) - разделить эти фичи?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) bool CanCling = false;

	//насколько здоровье в этом сегменте тела влияет на способность двигаться
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) float AffectMobility = 0.0f;

	//имя кости, которая используется как точка отсчёта при расчёте длины "плеча" машинной конечности, для позиционирования реальной уонечности
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FName AxisReferenceBone;
};

//свойства плотского узла
USTRUCT(BlueprintType) struct FFleshLimbAnatomy
{
	GENERATED_USTRUCT_BODY()
		
	//номер финального членика, для этого скелета
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, config) TArray<uint8> BodyIndex;

	// имя кости стопы, всё из-за того, что OnHit выдает имя кости, а не индекс тела
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, config) FName StepBoneName;

	//может ли этот членик хватать предметы
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) bool CanGrab = false;

	//может ли этот членик быть ухвачен - если нет, то должна взяться какая-то другая часть тела
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) bool CanBeGrabbed = true;

	//явно указываем имя кости/сокета, которой можно хватать предметы - он же используется для локализации шагов
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FName GrabSocket;

	//минимальный вес, до которого может падать показ физики для этой части тела, для кинематических - 0.0, для вечно висящих 1.0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) float MinPhysicsBlendWeight = 1.0f;
};

//###################################################################################################################
// ◘ данные по сегменту тела для скелетного меша - распределение здоровья, атакующих ролей по частям тела
// надо чистить, слишком много остатков от прошлых версий
//###################################################################################################################
USTRUCT(BlueprintType) struct FMeshPerBodySegmentData
{
	GENERATED_USTRUCT_BODY()

	//анатомия каркаса
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMachineLimbAnatomy Machine;

	//анатомия плоти
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FFleshLimbAnatomy Flesh;

	//очистить от привязок к сборке
	void Reset()
	{	Flesh.BodyIndex.SetNum(0);	}
};

//###################################################################################################################
// вспомогательная структура анатомии персноажа, чтобы удобно было редактировать в блюпринте
// содержит подробные характеристики частей тела для всех персонажей класса
//###################################################################################################################
USTRUCT(BlueprintType) struct FMeshAnatomy
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData Default;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData Lumbus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData Pectus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData Thorax;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData Head;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData LeftArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData RightArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData LeftLeg;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData RightLeg;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData Tail;

	//количество макросегментов - пока константа, потом...?
	int Num() const { return (int)ELimb::NOLIMB; }

	//представить этот же объект как массив
	FMeshPerBodySegmentData& GetSegmentByIndex(ELimb i) { return ((FMeshPerBodySegmentData*)this)[(int)i]; }
	FMeshPerBodySegmentData& GetSegmentByIndex(int i) { return ((FMeshPerBodySegmentData*)this)[i]; }

	FMeshAnatomy()
	{
		Default.Machine.TouchImportance = 0.2;
		Lumbus.Machine.TouchImportance = 0.15;
		Pectus.Machine.TouchImportance = 0.15;
		Head.Machine.TouchImportance = 0.1;
		Tail.Machine.TouchImportance = 0.05;

	}

};

#define STEPPED_MAX 15
//###################################################################################################################
// часть тела
//###################################################################################################################
USTRUCT(BlueprintType) struct FLimb
{
	GENERATED_BODY()

	//номер части тела для самоидентификации
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) ELimb WhatAmI;

	// параметры точки касания этой части тела со внешней поверхностью
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector ImpactNormal;			// если это узел пояса, то не своя нормаль, а усредненная по ногам
	FBodyInstance* Floor = nullptr;												// физическое тело опоры - там всё
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EMyrSurface Surface;			// запомнить поверхность опоры
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 Grabs:1;				// держит что-то этой конечностью
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 LastlyHurt:1;			// предыдущее столкновение было атакующим и повреждающим, это - пропустить, чтоб не спамить систему

	//модель приложения сил, заносится извне на уровне режима поведения, текущего самодействия
	//возможно, не нужно, так как еть доступ через -> Girdle -> DynModel -> GirdleRay. Но слишком длинный.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (Bitmask, BitmaskEnum = ELDY)) int32 DynModel = LDY_PASSIVE;

	//повреждение данной части тела
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Damage = 0.0f;

	//быстрая определялка, наступает или нет (по умолчанию контакта нет, ибо Floor = nullptr)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 Stepped = 0;

	////////////////////////////////////////////////////////////

	void EraseFloor() { Stepped = 0; Floor = nullptr; Surface = EMyrSurface::Surface_0; }
	void GetFloorOf(const FLimb& Oth) { Floor = Oth.Floor; Surface = Oth.Surface; }
	void TransferFrom(const FLimb& Oth) { Stepped = Oth.Stepped; GetFloorOf(Oth); }

	////////////////////////////////////////////////////////////


	bool IsClimbable() const  { return (Surface == EMyrSurface::Surface_WoodRaw || Surface == EMyrSurface::Surface_Fabric/* || Surface == EMyrSurface::Surface_Flesh*/); }

	//эта часть тела хочет быть вертикальной (но не факт, что есть)
	bool IsVertical() const { return (DynModel & LDY_TO_VERTICAL) && (DynModel & LDY_MY_UP); }

	//эта часть тела несёт предмет (и принимать толчки/шаги не может)
	bool HoldsItem() const { return Stepped == 255; }

	//зафиксировать и расфиксировать тот факт, что объект, введенный как опора, является носимым предметом
	void GrabCurrent() { Stepped = 1; }
	void UnGrabCurrent() { Stepped = 0; }

	//выдать компонент опоры из физического тела опоры
	UPrimitiveComponent* GetComp() { return Floor->OwnerComponent.Get(); }

	//тело касается модульной опоры типа ветки
	bool OnBranch() { return (Floor->InstanceBodyIndex != INDEX_NONE); }
	bool OnCapsule() { if(!Floor->GetBodySetup()) return false;  return (Floor->GetBodySetup()->AggGeom.SphylElems.Num() == 1); }
	bool OnSphere() { if(!Floor->GetBodySetup()) return false; return (Floor->GetBodySetup()->AggGeom.SphereElems.Num()>=1); }

	//радиус ветки/опоры, про которую точно известно, что она оформлена капусл ой
	float GetBranchRadius() { return Floor->GetBodySetup()->AggGeom.SphylElems[0].Radius; }

	//выдать локальный верх седла ветки, нормали в точке равновесия от съезжаний в бок - дважды векторное произведение
	//внимание, результат не нормируется, потому что фигурирует в составе процедуры, где и так и так в конце нужна нормирвка
	FVector GetBranchSaddleUp() {
		auto lX = GetBranchDirection();		//направление ветки
		auto lY = lX^FVector::UpVector;		//векторное произведение с вертикалью даст или ноль, или вектор в бок
		lY = lY^lX;							//полученный вектор вбок снова в.п. с направлением ветки - получится тот самый седловой вверх или вниз
		if(lY.Z<0) lY = -lY;				//выбираем то направление, что вверх (в.п. дает разные знаки от последовательности операндов)
		return lY;	}

	//если опора ветка, то это будет ее направление (однако, почему Y и насколько это универсально)
	FVector GetBranchDirection () const { return Floor->GetUnrealWorldTransform().GetUnitAxis(EAxis::Y); }

	//направление ветки с учётом желаемого хода - по ветке или против ветки
	FVector GetBranchDirection (const FVector& InDir) const { auto g = GetBranchDirection(); if(FVector::DotProduct(g,InDir)>0) return g; else return -g;  }

	FLimb() {Grabs = 0; LastlyHurt = 0; }
};

//учет подцепленных к поясу частей тела
UENUM(BlueprintType) enum class EGirdleRay : uint8 { Center, Left, Right, Spine, Tail, MAXRAYS };
ENUM_CLASS_FLAGS(EGirdleRay);

UENUM() enum class EStepDetect : uint8
{
	None,
	TraceRare,
	TraceEveryFrame,
	Hit,
	HitAndRetrace
};
//###################################################################################################################
// сборка моделей динамики для всех частей пояса конечностей - для структур состояний и самодействий, чтобы
// иметь компактную переменную для всех частей тела, содержащую только вот это необходимое добро
// видимо, добавить Leading
//###################################################################################################################
USTRUCT(BlueprintType) struct FGirdleDynModels
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = ELDY)) int32 Center = LDY_ROTATE | LDY_MY_UP | LDY_TO_VERTICAL;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = ELDY)) int32 Left =	LDY_ROTATE | LDY_MY_WHEEL | LDY_TO_COURSE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = ELDY)) int32 Right = LDY_ROTATE | LDY_MY_WHEEL | LDY_TO_COURSE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = ELDY)) int32 Spine = LDY_ROTATE | LDY_MY_UP | LDY_TO_VERTICAL;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = ELDY)) int32 Tail = LDY_PASSIVE;

	//ведущий или ведомый, если ведомый, то ноги более скованы и отстают
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Leading")) bool Leading = true;

	//сразу вкл/выкл гравитацию для члеников ног в пределах пояса
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Fix On Floor")) bool FixOnFloor = false;

	//выключить сгибания сразу всех ног, а именно таза с ногами вперед назад
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Legs Swing Lock")) bool LegsSwingLock = false;

	//использовать констрейнт для жесткого задания ориентации центра пояса по направлению движения - например, чтобы препятствие не разворачивало
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Hard Course Align")) bool HardCourseAlign = false;

	//вертикаль через констрейнт, жесткая, может покалечить
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Hard Vertical")) bool HardVertical = false;

	//сделать массу моделей ноги особо высокой, чтобы центр тяжести был смещён вниз
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Heavy Legs")) bool HeavyLegs = true;


	//насколько пригибать пояс ногами к земле 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Crouch")) float Crouch = 0.0;

	//насколько расставлять или стягивать ноги (безразмерно, на глазок, влияет на рассчет позы в анимации)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Legs Spread")) float LegsSpread = 0.0;

	//множитель сил, которые применяются к членам согласно выше заполненным моделям - для подгонки в редакторе
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Forces Mult")) float ForcesMultiplier = 1000.0;

	//вязкость, включаемая для ног при торможении
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Feet Brake")) float FeetBrakeDamping = 50.0;

	//вязкость, подбираемая к другим телам при указании бита
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Damping")) float CommonDampingIfNeeded = 0.01;

	//получить по индексу
	uint8 At(const EGirdleRay Where) const { return ((uint8*)(&Center))[(int)Where]; }
};


//###################################################################################################################
//сборка динамических моделей, чтоб разом менять для состояний поведения, а поверх них - для действий в разных фазах
//похоже, это становится сердцем всей физики тела - возможно, перенсти в отдельные ассеты
//###################################################################################################################
USTRUCT(BlueprintType) struct FWholeBodyDynamicsModel
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGirdleDynModels Thorax;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGirdleDynModels Pelvis;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Anim Rate")) float AnimRate = 1.0f;		// если задействована анимация, то регулировать ее скорость
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "+Health/Sec")) float HealthAdd = 0.0f;	// удельная прибавка или отъем здоровья	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "+Stamina/sec")) float StaminaAdd = 0.0f;	// удельная прибавка или отъем запаса сил	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Move Gain")) float MotionGain = 1.0f;	// ослабление скорости движения	

	// звук, который играет, пока существо находится в данном состоянии
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio") TWeakObjectPtr<USoundBase> Sound;
	FWholeBodyDynamicsModel() { Thorax.Leading = true; Pelvis.Leading = false; }
};

//отмечать результат попытки зацепиться
UENUM() enum class EClung : uint8
{
	Recreate, Update, Kept,
	NoWill, NoSurface, NoLeading, BadSurface, BadAngle, NoClimbableSurface, DangerousSpread
};
inline bool operator>(EClung A, EClung B) { return (int)A > (int)B; }
inline bool operator<(EClung A, EClung B) { return (int)A < (int)B; }
inline bool operator==(EClung A, EClung B) { return (int)A == (int)B; }
inline bool operator>=(EClung A, EClung B) { return (int)A >= (int)B; }
inline bool operator<=(EClung A, EClung B) { return (int)A <= (int)B; }


//###################################################################################################################
// сборка производных параметров анимации для целого шарика - старая
// MyrAnimInst_BodySection.cpp
//###################################################################################################################
USTRUCT(BlueprintType) struct FBodySectionAnimData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float GainDirect;		// проекция курса на позвоночник
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float GainLateral;		// проекция курса на поперёк
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float Velocity;			// скаляр скорости
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float FeetSpread;		// расставление ног --|- -|-  -
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float Curvature;		// степень кривизны поверхности \/ -- /\ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float Crouch;			// степень пригнутости
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)	float LeanLeftOrRight;	// степень уклона влево или вправо

	// перевычислить данные для секции тела, пригодные для непосредственной анимации
	void Update(class UMyrBodyContactor* Body, class AMyrraCreature* Owner, float AdvancedCrouch = 0.0);
};

//каналы отображения отладочных линий для векторных данных
UENUM() enum class ELimbDebug : uint8
{
	LimbAxisX,
	LimbAxisY,
	LimbAxisZ,
	LimbForces,
	LimbTorques,
	GirdleGuideDir,
	GirdleStepped,
	MainDirs,
	LimbNormals,
	ConstrPri,
	FeetShoulders,
	LimbConstrForceLin,
	LimbConstrForceAng,
	CentralConstrForceLin,
	CentralConstrForceAng,
	LineTrace,
	FeetBrakeDamping,
	AILookDir
};
