#pragma once
#include "CoreMinimal.h"
#include "myrra_behave.generated.h"

//###################################################################################################################
//навести порвдок в флагах состояния для всего тела
//состояния делятся на макросостояния и микросостояния. (старшие и младшие биты)
// - макросостояния переключаются отдельным автоматом, чтоб не плодить паралальных ветвей
// - микросостояния также управляются отдельным автоматом и могут прерываться (устанавливаться в дефолт) переключением макросостояния
//###################################################################################################################
UENUM(BlueprintType) enum class EMacroState : uint8
{
	NO,
	LAX = 1 << 3,
	LAND = 2 << 3,
	WALL = 3 << 3,
	AIR = 4 << 3,
	WATER = 5 << 3
};

UENUM(BlueprintType, meta = (Bitflags)) enum class EBehaveState : uint8
{
	NO = 0,									// запрет перехода
	RESERVED1 = 1	UMETA(Hidden),			// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED2 = 2	UMETA(Hidden),			// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED3 = 3	UMETA(Hidden),			// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED4 = 4	UMETA(Hidden),			// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED5 = 5	UMETA(Hidden),			// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED6 = 6	UMETA(Hidden),			// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED7 = 7	UMETA(Hidden),			// ----- заполнитель для порядка следования, без этого полная хня -------

	dead = (uint8)EMacroState::LAX + 0,		// без сознания, рэгдолл со степенью перехода в состояние релакс, ничего не возможно
	relax = (uint8)EMacroState::LAX + 1,	// релакс, сидение, лежание, разные подтипы, большинство действий недоступно, требуется выход
	held = (uint8)EMacroState::LAX + 2,		// удерживаемый кем-то в зубах
	dying = (uint8)EMacroState::LAX + 3,	// состояние между окончательной смертью и жизнью в обе стороны
	lie = (uint8)EMacroState::LAX + 4,		// просто лежание на боку - от падения или намерненный релакс
	RESERVED13 = 13	UMETA(Hidden),			// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED14 = 14	UMETA(Hidden),			// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED15 = 15	UMETA(Hidden),			// ----- заполнитель для порядка следования, без этого полная хня -------

	stand = (uint8)EMacroState::LAND + 0,	// стояние (если скорость долго на нуле) + дополнительные анимации скучания				
	walk = (uint8)EMacroState::LAND + 1,	// ходьба (от нуля скорости до небыстрого бега) основной режим движения, многие действия возможны на ходу
	run = (uint8)EMacroState::LAND + 2,		// быстрый бег, отдельная кнопка, невозможны многие действия
	crouch = (uint8)EMacroState::LAND + 3,	// подкрадывание, отдельная кнопка, атаки действуют по другому, чувствительность и расход энергии другие)
	sidewalk = (uint8)EMacroState::LAND + 4,//движение боком - просто чтобы иметь другую динамику частей тела
	turn = (uint8)EMacroState::LAND + 5,	// разворот на месте
	RESERVED22 = 22	UMETA(Hidden),			// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED23 = 23	UMETA(Hidden),			// ----- заполнитель для порядка следования, без этого полная хня -------

	stuck = (uint8)EMacroState::WALL + 0,	// уткнулись в стену, стоя на горизонтали, прерываем движение, облокачиваемся, можем взбираться
	hang = (uint8)EMacroState::WALL + 1,	// висение на вертикали, одним поясом
	climb = (uint8)EMacroState::WALL + 2,	// карабканье по подходящей опоре (в движении и висении)
	slide = (uint8)EMacroState::WALL + 3,	// соскальзывание (падение с касанием опоры под большим углом) не в группе "воздух" так как есть хватка
	mount = (uint8)EMacroState::WALL + 4,	// забираться, вспрыгивать, одолеть забор, уступ, аналог soar, только на небольшую высоту
	RESERVED29 = 29	UMETA(Hidden),		// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED30 = 30	UMETA(Hidden),		// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED31 = 31	UMETA(Hidden),		// ----- заполнитель для порядка следования, без этого полная хня -------

	fall = (uint8)EMacroState::AIR + 0,	// падение (убытие опоры в прыжке или достижение некоторой скорости вниз)
	soar = (uint8)EMacroState::AIR + 1,	// взмывание (начальная стадия прыжка, пока вверх) сюда же, возможно, полёт летающих
	fly = (uint8)EMacroState::AIR + 2,	// активный полёт
	tumble = (uint8)EMacroState::AIR + 3,//"кувыркаться", при падении на бок или кверх ногами, но включается активный переворот
	land = (uint8)EMacroState::AIR + 4,	// "приземление", управляемое падение на лапки
	RESERVED37 = 37	UMETA(Hidden),		// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED38 = 38	UMETA(Hidden),		// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED39 = 39	UMETA(Hidden),		// ----- заполнитель для порядка следования, без этого полная хня -------

	drawn = (uint8)EMacroState::WATER + 0,	// утопание, пассивное падение на дно
	swim = (uint8)EMacroState::WATER + 1,	// активное плавание по поверхности
	dive = (uint8)EMacroState::WATER + 2,	// активное плавание в глубине
	RESERVED43 = 43	UMETA(Hidden),		// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED44 = 44	UMETA(Hidden),		// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED45 = 45	UMETA(Hidden),		// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED46 = 46	UMETA(Hidden),		// ----- заполнитель для порядка следования, без этого полная хня -------
	RESERVED47 = 47	UMETA(Hidden),		// ----- заполнитель для порядка следования, без этого полная хня -------

	TRANSIT_PREPARE = 254,	// оставить текущее состояние, но добавить/запомнить целевое	
	TRANSIT_AT_ONCE = 255	// немедленно перейти к ранее запрашиваемому состоянию
};

//макрос для ветвления по переходным состояниям
#define FRINGE(FROM,TO) (uint16)(((uint8)(FROM)) | (((uint8)(TO))<<8))
#define TRANS(FROM,TO) (uint16)(((uint8)(EBehaveState::##FROM)) | (((uint8)(EBehaveState::##TO))<<8))
#define STABLE(AT) (uint16)(((uint8)(EBehaveState::##AT)) | (((uint8)(EBehaveState::NO))<<8))


//получить макросостояние (248 = 0xb11111000) просто вытирая три послених бита
#define MACRO(state) ((EMacroState)((uint8)(state)&248))

//операция сравнения для быстрого выявления макросостояния (если все биты)
#define IS_MACRO(state, macrostate) ((((uint8)(state)) & ((uint8)(macrostate))) == ((uint8)(EMacroState::##macrostate)))


//###################################################################################################################
//аггрегатор двух состояний - условно старого и нового, используется в хранении и в сообщениях об изменении состояния
//###################################################################################################################
USTRUCT(BlueprintType) struct FBehaveFringe
{
	GENERATED_USTRUCT_BODY()


	//собственно, поле, содержащее состояния
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EBehaveState CurrentState;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EBehaveState UpcomingState;

	//стабильный, значит нет процесса перехода между состояниями
	bool Stable() const { return (UpcomingState == EBehaveState::NO); }

	//утилиты для хэша и прочего
	uint16&			Integra(void) { return *((uint16*)this); }
	uint16			cIntegra(void) const { return *((uint16*)this); }
	bool			operator==	(const FBehaveFringe& o) const { return (o.cIntegra() == cIntegra()); }
	bool			operator!=	(const FBehaveFringe& o) const { return (o.cIntegra() != cIntegra()); }
	friend uint32	GetTypeHash(const FBehaveFringe& o) { return GetTypeHash(o.cIntegra()); }

	//получить макросостояние (248 = 0xb11111000) просто вытирая три послених бита
	EMacroState		Macro() { return (EMacroState)(((uint8)CurrentState) & 248); }

	//конструктор, можно не задавать второе состояние, в таком случае статус сразу стабильный
	FBehaveFringe(EBehaveState At, EBehaveState To = EBehaveState::NO) : CurrentState(At), UpcomingState(To) {}
	FBehaveFringe() : CurrentState(EBehaveState::NO), UpcomingState(EBehaveState::NO) {}

	void Retract() { UpcomingState = EBehaveState::NO; }
	void Achieve() { CurrentState = UpcomingState; UpcomingState = EBehaveState::NO; }
	void ModGoal(EBehaveState To) { if (To != CurrentState) UpcomingState = To; else EBehaveState::NO; }

};


