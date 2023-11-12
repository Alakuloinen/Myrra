#pragma once
#include "CoreMinimal.h"
#include "AdvMath.h"
#include "myrra_surface.h"
#include "myrra_anatomy.generated.h"

//номер членика в скелетной опоре, когда она вовсе не скелетная
#define NO_BODY -127

#define BI_QUA(a,b) (((uint8)(a)) + (((uint8)(b))<<4))

#define BODYALL(index, name2, section, name3, parent, name4, mirror, name5, oppos, name6, isleg) ((index) + ((parent)<<4) + ((mirror)<<8) + ((oppos)<<12) + ((section)<<13) + ((isleg)<<14))
#define LSECTION(limb)	(((limb)>>13)&1)
#define LPARENT(limb)	(((limb)>>4)&15)
#define LMIRROR(limb)	(((limb)>>8)&15)
#define LOPPOSITE(limb) (((limb)>>12)&15)
#define LISLEG(limb)	(((limb)>>14)&1)

enum PRAEBODY {	PELV,LUMB,PECT,THOR,HEAD,TAIL,LARM,RARM,LLEG,RLEG,NOLIMB };
constexpr uint16 OFLIMB[] = 
{
	BODYALL (PELV, "Section:", 0, "Parent:", PELV,	"Mirror:", PELV, "Opposite:", THOR, "IsLeg:", false),
	BODYALL	(LUMB, "Section:", 0, "Parent:", PELV,	"Mirror:", LUMB, "Opposite:", PECT, "IsLeg:", false),
	BODYALL	(PECT, "Section:", 1, "Parent:", LUMB,	"Mirror:", PECT, "Opposite:", LUMB, "IsLeg:", false),
	BODYALL	(THOR, "Section:", 1, "Parent:", PECT,	"Mirror:", THOR, "Opposite:", PELV, "IsLeg:", false),
	BODYALL	(HEAD, "Section:", 1, "Parent:", PECT,	"Mirror:", HEAD, "Opposite:", TAIL, "IsLeg:", false),
	BODYALL	(TAIL, "Section:", 0, "Parent:", LUMB,	"Mirror:", TAIL, "Opposite:", HEAD, "IsLeg:", false),
	BODYALL	(LARM, "Section:", 1, "Parent:", THOR,	"Mirror:", RARM, "Opposite:", LLEG, "IsLeg:", true),
	BODYALL	(RARM, "Section:", 1, "Parent:", THOR,	"Mirror:", LARM, "Opposite:", RLEG, "IsLeg:", true),
	BODYALL	(LLEG, "Section:", 0, "Parent:", PELV,	"Mirror:", RLEG, "Opposite:", LARM, "IsLeg:", true),
	BODYALL	(RLEG, "Section:", 0, "Parent:", PELV,	"Mirror:", LLEG, "Opposite:", RARM, "IsLeg:", true)
};

//константы для сегментов тела (хардкод для позвоночных, но остальные могут их использовать в нестандартном смысле)
UENUM() enum class ELimb : uint8
{ 
	PELVIS = ::PELV,
	LUMBUS = ::LUMB,
	PECTUS = ::PECT,
	THORAX = ::THOR,
	HEAD = ::HEAD,
	TAIL = ::TAIL,
	L_ARM = ::LARM,
	R_ARM = ::RARM,
	L_LEG = ::LLEG,
	R_LEG = ::RLEG,
	NOLIMB
};

ELimb	FORCEINLINE LimbParent	(ELimb eL) { return (ELimb)((OFLIMB [ (int)eL ]>>4 )&15); }
ELimb	FORCEINLINE LimbMirror	(ELimb eL) { return (ELimb)((OFLIMB [ (int)eL ]>>8 )&15); }
ELimb	FORCEINLINE LimbOpposite(ELimb eL) { return (ELimb)((OFLIMB [ (int)eL ]>>12)&15); }
int		FORCEINLINE LimbSection (ELimb eL) { return			(OFLIMB [ (int)eL ]>>13)&1; }


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
	EvenIfFixed			UMETA(DisplayName = "+ Even If Fixed"),
	Friction			UMETA(DisplayName = "+ FRICTION")
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
	LDY_NOTONGROUND = 1 << (int)ELDY::NotOnGround,
	LDY_EVENIFFIXED = 1 << (int)ELDY::EvenIfFixed,

	LDY_FRICTION = 1 << (int)ELDY::Friction

};

//###################################################################################################################
// ориентация конкретной чачсти тела или группы частей тела
// чтобы по-разному экспортированные кости можно было двигать в понятных направлениях
//###################################################################################################################
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

//###################################################################################################################
//по каким осям поясница и голова способны крутиться - для оптимизации, чтобы исключать расчёт арксинусов
//###################################################################################################################
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
// дополнительные данные к физическому членику - отношение к секции тела и тип (физ-тело или кинематическое мясо)
//###################################################################################################################
USTRUCT(BlueprintType) struct FBODY_BIO_DATA
{
	GENERATED_USTRUCT_BODY()

	// отношение данного членика - к какому сегменту тела он принадлежит
	UPROPERTY(VisibleAnywhere) ELimb eLimb = ELimb::PELVIS;

	//уровень листовости (если лист, то 0, и далее по иерархии) чтобы сравнивать дальность члеников в сегменте
	//это надо заполнять до игры, при разборе физ.лошарика, проще всего ввести вручную в строку имен члеников 
	uint8 DistFromLeaf = 0;
};

//###################################################################################################################
//свойства каркасного узла
//###################################################################################################################
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

	// имя кости стопы, всё из-за того, что ебучий OnHit выдает имя кости, а не индекс тела
	// может, уже не нужно, заюзать грабсокет
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, config) FName StepBoneName;

	//может ли этот членик хватать предметы
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) bool CanGrab = false;

	//может ли этот членик быть ухвачен - если нет, то должна взяться какая-то другая часть тела
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) bool CanBeGrabbed = true;

	//явно указываем имя кости/сокета, которой можно хватать предметы - он же используется для локализации шагов
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FName GrabSocket;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData Tail;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData LeftArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData RightArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData LeftLeg;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config) FMeshPerBodySegmentData RightLeg;

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

#define STEPPED_MAX 63

//###################################################################################################################
// объект опоры при ходьбе или касании физ-движком
//###################################################################################################################
USTRUCT(BlueprintType) struct FFloor
{
	GENERATED_BODY()

	//тип поверхности предмета касания, добывается из физ-материала
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EMyrSurface Surface = EMyrSurface::Surface_0;

	//нормаль касания, если это центральный узел пояса или нога, то не своя нормаль, а расчитанная по трассировкам
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector3f Normal;

	//физическое тело опоры - там всё
	FBodyInstance* Body = nullptr;


	//инициализировать из структуры хит (в Myrra.cpp)
	bool FromHit(const FHitResult& Hit);

	//принять из другой сборки опоры с плавным обретением нормали
	void Transfer(const FFloor& Other, float A = 1.0f) { Body = Other.Body; Surface = Other.Surface; Normal = FMath::Lerp(Normal, Other.Normal, A); }

	//удалить инфу об опоре
	void Erase() { Body = nullptr; Surface = EMyrSurface::Surface_0; }

	//актор владеющий телом опоры
	UPrimitiveComponent* Component() { return Body->OwnerComponent.Get(); }
	UPrimitiveComponent* Component() const { return Body->OwnerComponent.Get(); }

	//актор владеющий телом опоры
	AActor* Actor() { return Body->OwnerComponent->GetOwner(); }

	UBodySetup* BSetup() const		{ return Body->GetBodySetup(); }
	FKAggregateGeom& AgGeo() const	{ return Body->GetBodySetup()->AggGeom; }
	bool IsValid() const			{ return Body && Body->OwnerComponent.IsValid() && (int)Surface > 0;  }
	operator bool() const			{ return IsValid(); }
	bool operator==(const FFloor& O) { return (Body == O.Body); }
	bool operator!=(const FFloor& O) { return (Body != O.Body); }
	bool IsMovable() const			{ return Component()->Mobility == EComponentMobility::Movable; }
	bool IsClimbable() const		{ return (Surface == EMyrSurface::WoodRaw || Surface == EMyrSurface::Fabric || Surface == EMyrSurface::Flesh); }
	bool IsCapsule() const			{ return (BSetup() && AgGeo().SphylElems.Num() >= 1); }
	bool IsBox() const				{ return (BSetup() && AgGeo().BoxElems.Num() >= 1); }
	bool IsSphere() const			{ return (BSetup() && AgGeo().SphereElems.Num() >= 1); }

	//характеристический вектор "вдоль" если опора - простая продолговатая форма
	FVector3f Dir() const			{ return (FVector3f)Body->GetUnrealWorldTransform().GetUnitAxis(EAxis::Y); }

	//характерная толщина опоры, если это продолговатая простая форма
	float Thickness() const			{	if(IsCapsule())	return AgGeo().SphylElems[0].Radius;
										if(IsBox())		return FMath::Min3(AgGeo().BoxElems[0].X, AgGeo().BoxElems[0].Y, AgGeo().BoxElems[0].Z); 
										if(IsSphere())	return AgGeo().SphereElems[0].Radius;
										return 10000;	}

	bool Thin(float T = 5) const	{ return (Thickness() <= T); }

	//вектор ориентации опоры, если это ветка или брусок, сообразно внешнему вектору, фактически проекция взгляда на линию ветки
	FVector3f Dir(FVector3f F)const { auto G = Dir(); return (G|F)>0 ? G : -G; }

	//сонаправленность ориентации опоры внешнему вектору
	float Coax(FVector3f Look)const { return Dir(Look)|Look; }

	//крутой уклон исключающий возможность захода пешком
	bool TooSteep() const { return (Normal.Z < 0.5f); }

	//приемлемость опоры для стояния и фиксации, либо пологая, либо крутая, но мы можем карабкаться по ней
	bool Steppable(bool Climb)const { return (IsClimbable() && Climb) || TooSteep(); }


	//покомпонентная настройка условно ходимых предметов
	bool CharCanStep() const { return Component()->CanCharacterStepUpOn == ECanBeCharacterBase::ECB_Yes; }
	bool CharNoStep() const { return Component()->CanCharacterStepUpOn == ECanBeCharacterBase::ECB_No; }

	//количественная приемлемость опоры, удобство на ней стоять, ее вес при выборе пути, (0;1) 
	float Rating(FVector3f Look, FVector3f Up, bool Climb) const
	{	if (!IsValid()) return 0.0f;
		if(!Steppable(Climb)) return 0.0f; else
		return FMath::Max(0.0f, Up|Normal) * IsCapsule() ? FMath::Max(0.0f,Coax(Look)) : 1.0f;
	}

	//скорость тела опоры в точке
	FVector3f Speed(FVector Point) const { return (FVector3f)Body->GetUnrealWorldVelocityAtPoint(Point); }
	FVector3f Speed() const { return (FVector3f)Body->GetUnrealWorldVelocity(); }

	//степень уткнутости вглубь, для
	float BumpAmount(FVector3f ADir) {	return FVector2f(-Normal) | FVector2f(ADir);	}

	FFloor() :Surface(EMyrSurface::Surface_0), Body(nullptr) {}
	FFloor(const FHitResult& Hit) { FromHit(Hit); }

};


//###################################################################################################################
// часть тела
//###################################################################################################################
USTRUCT(BlueprintType) struct FLimb
{
	GENERATED_BODY()

	//номер части тела для самоидентификации
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) ELimb WhatAmI;

	//быстрая определялка, наступает или нет (по умолчанию контакта нет, ибо Floor = nullptr)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 Stepped = 0;

	//степень соосности вектора скорости и нормали при ударе
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FRatio Colinea = 0;

	// держит что-то этой конечностью (или карабкается этим туловищем)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 Grabs:1;

	// предыдущее столкновение было атакующим и повреждающим, это - пропустить, чтоб не спамить систему
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 LastlyHurt:1;			

	//модель приложения сил, заносится извне на уровне режима поведения, текущего самодействия
	//возможно, не нужно, так как еть доступ через -> Girdle -> DynModel -> GirdleRay. Но слишком длинный.
	//также сюда можно заносить/уносить рантайм события LastlyHurt / Grabs
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (Bitmask, BitmaskEnum = ELDY)) int32 DynModel = LDY_PASSIVE;

	//повреждение данной части тела
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Damage = 0.0f;

	//сборка данных по опоре, на которой стоим
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FFloor Floor;

	////////////////////////////////////////////////////////////

	//получить и удалить опору
	void AcceptFloor(FFloor& N, float Alpha) { Stepped = STEPPED_MAX; Floor.Transfer(N, Alpha); }
	void EraseFloor() { Stepped = 0; Floor.Erase(); }

	////////////////////////////////////////////////////////////

	//ВНИМАНИЕ, ОПАСНОЕ ПРИВЕДЕНИЕ!
	//если это нога
	float& FootPitch() { return *((float*)&DynModel); }

	//нога, не туловище 
	bool IsLeg() const { return (WhatAmI >= ELimb::L_ARM && WhatAmI <= ELimb::R_LEG); }

	//левая нога 
	bool IsLeft() const { return (WhatAmI == ELimb::L_ARM || WhatAmI == ELimb::L_LEG);	}

	//для простого учета развернутости осей (возможно, не нужен, так как ForcesMultiplier можно задать отрицательным
	float OuterAxisPolarity() const {	return (DynModel & LDY_REVERSE) ? -1.0f : 1.0f;	}

	//эта часть тела хочет быть вертикальной (но не факт, что есть)
	bool IsVertical() const { return (DynModel & LDY_TO_VERTICAL) && (DynModel & LDY_MY_UP); }

	//сонаправленность движения членика с последней опорой
	float GetColinea(int Balancer = 0) const { return Stepped ? (Colinea + Balancer) / (255.0f + Balancer) : 0; }

	//сонаправленность движения членика с последней опорой
	float GetTouch(int Balancer = 0) const { return (Stepped + Balancer) / ((float)(STEPPED_MAX) + Balancer); }


	//степень контакта с поверхностью, актуальность * соосность 
	float GetBumpCoaxis() const { return Stepped ? GetTouch()*GetColinea() : 0.0f; }

	//нормаль взвешенная коэффициентом значимости касания
	FVector3f GetWeightedNormal() const { return Floor.Normal * GetBumpCoaxis(); }

	FLimb() {Grabs = 0; LastlyHurt = 0; }
};

//учет подцепленных к поясу частей тела
UENUM(BlueprintType) enum class EGirdleRay : uint8 { Center, Left, Right, Spine, Tail, MAXRAYS };
ENUM_CLASS_FLAGS(EGirdleRay);

USTRUCT(BlueprintType) struct FGirdleFlags
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Fix On"))			uint8 FixOn : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Tighten Yaw"))		uint8 TightenYaw : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Tighten Lean"))		uint8 TightenLean : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Lock Roll"))			uint8 LockRoll : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Lock Pitch"))		uint8 LockPitch : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Lock Yaw"))			uint8 LockYaw : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Vertical Align"))	uint8 Vertical : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Lock Shoulders"))	uint8 LockLegsSwing : 1;
	FGirdleFlags(uint8 AsInt = 0) { *((uint8*)this) = AsInt; }
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = ELDY)) int32 Spine = LDY_ROTATE | LDY_MY_UP | LDY_TO_VERTICAL;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = ELDY)) int32 Tail = LDY_PASSIVE;

	//использовать эту модель или оставить/переключить нижележащую по иерархии
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Use")) bool Use = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "On Ground")) FGirdleFlags OnGnd;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "In Air"))	FGirdleFlags InAir;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Leads")) bool Leading = true;

	//шаги с точным позиционирпованием ног
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool FairSteps = false;

	//насколько пригибать пояс ногами к земле 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Crouch", UIMin = "0.0", UIMax = "1.0")) float Crouch = 0.0;

	//насколько расставлять или стягивать ноги (безразмерно, на глазок, влияет на рассчет позы в анимации)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Legs Spread %Length", UIMin = "-0.3", UIMax = "0.5")) float LegsSpreadMin = -0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Legs Spread %Length", UIMin = "-0.3", UIMax = "0.5")) float LegsSpreadMax = 0.3f;

	//множитель сил, которые применяются к членам согласно выше заполненным моделям - для подгонки в редакторе
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Forces Mult")) float ForcesMultiplier = 1000.0;

	//жесткость констрейнта между центральным и спинным члеником
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Joint Stiffness")) FRatio JunctionStiffness = 1.0f;

	//вязкость, подбираемая к другим телам при указании бита
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Damping")) float CommonDampingIfNeeded = 0.01;

	//получить по индексу
	uint8 At(const EGirdleRay Where) const { return ((uint8*)(&Center))[(int)Where]; }

	//выдать готовый коэф размаха ног по заданной альфе смешивания
	float GetLegSpread(float A) const { return FMath::Lerp(LegsSpreadMin, LegsSpreadMax, A); }

	//применяется когда надо перенести старые данные в новые переменные и пересохранить ассет
	bool Correction()
	{
		//OnGnd = AllFlagsFloor();
		//InAir = AllFlagsAir();
		//if (HardVertical) { VerticalOnFloor = true; TightenLeanOnFloor = true; }
		//if (HardCourseAlign) { TightenYawOnFloor = true; }
		//if (NormalAlign) { TightenLeanOnFloor = true; }
		//if (LegsSwingLock) { LockLegsSwingOnFloor = true;  } 
		return true;
	}

	//FGirdleDynModels() { }
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

	// если задействована анимация, то регулировать ее скорость
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Animation Rate")) float AnimRate = 1.0f;

	// удельная прибавка или отъем здоровья	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "+Health,Stamina/Sec")) float HealthAdd = 0.0f;

	// удельная прибавка или отъем запаса сил	
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float StaminaAdd = 0.0f;

	// ослабление скорости движения	(<1) или, при наличии JumpImpulse, импульс прыжка вперед (+) или назад (-)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Gain/Jump Forth,Up")) float MotionGain = 1.0f;

	//при входе в эту модель начинается накопление сил для прыжка с доли (<0) или сам прыжок с таким импульсом вверх (>0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float JumpImpulse = 0.0f;

	// двигать даже если извне не задана тяга - чтобы существо, например, отпрянуло от стены, когда не жмется на WASD
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "AutoMove")) bool MoveWithNoExtGain = false;

	// готовиться к прыжку, накапливать энергию, прижиматься к земле
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "PreJump")) bool PreJump = false;

	// двигаться даже без опоры кинематически, будучи ведомым якорями
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Fly Fixed")) bool FlyFixed = false;

	// коэффициент масштаба сил, сдерживающих поворот спины по любым осям
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float SpineStiffness = 1.0f;


	// исли флс то игнорировать эту модель, а оставить вышестоящую
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Use")) bool Use = true;

	// максимальный урон, который может выдержать существо, находясь в этой модели, при превышении действие, внедрившее эту модель, прерывается, а поведение может смениться
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DamageLim = 10.0f;

	// если существо игрок, то вход в это состояние включает в камере размытие в движении
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player", meta = (DisplayName = "MoBlur")) float MotionBlur = 0.0f;

	// если существо игрок, то вход в это состояние включает в мире замедление времени
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player", meta = (UIMin="0.0", UIMax="1.0", DisplayName="TimeSpeed")) float TimeDilation = 1.0f;

	// звук, который играет, пока существо находится в данном состоянии
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio") TWeakObjectPtr<USoundBase> Sound;

	//поскольку MotionGain- многозначная сущность, то для ослабления движения его нужно подобработать
	float GetMoveWeaken() const { return FMath::Min(1.0f, FMath::Abs(MotionGain)); }
	float GetJumpAlongImpulse() const { return MotionGain; }
	float GetJumpUpImpulse() const { return JumpImpulse; }
	bool IsJump() { return (GetJumpAlongImpulse() > 1 || GetJumpUpImpulse() > 0); }
	FWholeBodyDynamicsModel() { Thorax.Leading = true; Pelvis.Leading = false; }

	FGirdleDynModels& Girdle(bool Tho) { return Tho ? Thorax : Pelvis; }

	static FWholeBodyDynamicsModel& Default()
	{	static FWholeBodyDynamicsModel D;
		return D;
	}

	bool Correction() { return (Pelvis.Correction() + Thorax.Correction()) > 0; }
};

//сборка для позиционирования ноги в анимации
USTRUCT(BlueprintType) struct FLegPos
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DownUp = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float BackFront = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float LeftRight = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Pitch = 0.5f;

	float& Length() { return Pitch; }
	float AbsFront() const { return FMath::Abs(BackFront); }
	float NormFront() const { return BackFront/Pitch; }
	float AbsAside() const { return FMath::Abs(LeftRight); }
	FLinearColor& LC4() { return *((FLinearColor*)this); }
	FVector3f& V3() { return *((FVector3f*)this); }
	FVector3f NormV3() { return *((FVector3f*)this)/Pitch; }
	void Reset(float Lngth) { DownUp = Lngth; BackFront = 0; LeftRight = 0.0f; Length() = Lngth; }
};


//каналы отображения отладочных линий для векторных данных
UENUM() enum class ELimbDebug : uint8
{
	NONE,
	MeshOrigin,
	LimbAxisX,
	LimbAxisY,
	LimbAxisZ,
	LimbForces,
	LimbTorques,
	GirdleGuideDir,
	GirdleStepWeight,
	LimbStepped,
	MainDirs,
	IntegralBumpedNormal,
	LimbNormals,
	GirdleConstrainMode,
	FeetShoulders,
	CentralConstrForceLin,
	CentralConstrForceAng,
	LineTrace,
	GirdleCrouch,
	AILookDir,
	GirdleStandHardness,
	PhyCenterDislocation,
	PhyCenterDisrotation,
	FairStepPredictedPos,
	FairStepPath

};
