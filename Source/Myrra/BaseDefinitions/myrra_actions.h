#pragma once
#include "CoreMinimal.h"
//#include "myrra_emotions.h"
#include "myrra_actions.generated.h"


//обобщение атак и прочих действий существа - для замены существующей тесной системы атак
//ЭТО МАРАЗАМ, ТОВАРИЩИ - чтобы в анимации узел Blend Poses by Enum работал привильно, нужно объявлять ВСЕ значения в числовом интервале
//просто иначе последняя константа, равная 255 никогда не видится блюпринтом
UENUM(BlueprintType) enum class EAction : uint8
{
	ATTACK_PAW_1 = 0,				//атака передней конечностью
	ATTACK_PAW_2 = 1,				//атака передней конечностью 
	ATTACK_PAW_3 = 2,				//атака передней конечностью 
	ATTACK_PAW_4 = 3,				//атака передней конечностью 
	ATTACK_PAW_5 = 4,				//атака передней конечностью 
	ATTACK_PAW_6 = 5,				//атака передней конечностью 
	ATTACK_PAW_7 = 6, 				//атака передней конечностью
	ATTACK_PAW_8 = 7,				//атака передней конечностью 
	ATTACK_PAW_9 = 8, 				//атака передней конечностью
	ATTACK_PAW_10 = 9,				//атака передней конечностью

	ATTACK_MOUTH_1 = 10,			//атака ртом, кусь, возможно, поедание
	ATTACK_MOUTH_2 = 11,			//атака ртом, кусь, возможно, поедание
	ATTACK_MOUTH_3 = 12,			//атака ртом, кусь, возможно, поедание
	ATTACK_MOUTH_4 = 13,			//атака ртом, кусь, возможно, поедание
	ATTACK_MOUTH_5 = 14,			//атака ртом, кусь, возможно, поедание
	ATTACK_MOUTH_6 = 15,			//атака ртом, кусь, возможно, поедание
	ATTACK_MOUTH_7 = 16,			//атака ртом, кусь, возможно, поедание
	ATTACK_MOUTH_8 = 17,			//атака ртом, кусь, возможно, поедание
	ATTACK_MOUTH_9 = 18,			//атака ртом, кусь, возможно, поедание
	ATTACK_MOUTH_10 = 19,			//атака ртом, кусь, возможно, поедание

	ATTACK20 = 20 UMETA(Hidden),
	ATTACK21 = 21 UMETA(Hidden),
	ATTACK22 = 22 UMETA(Hidden),
	ATTACK23 = 23 UMETA(Hidden),
	ATTACK24 = 24 UMETA(Hidden),
	ATTACK25 = 25 UMETA(Hidden),
	ATTACK26 = 26 UMETA(Hidden),
	ATTACK27 = 27 UMETA(Hidden),
	ATTACK28 = 28 UMETA(Hidden),
	ATTACK29 = 29 UMETA(Hidden),
	ATTACK30 = 30 UMETA(Hidden),
	ATTACK31 = 31 UMETA(Hidden),
	ATTACK32 = 32 UMETA(Hidden),
	ATTACK33 = 33 UMETA(Hidden),
	ATTACK34 = 34 UMETA(Hidden),
	ATTACK35 = 35 UMETA(Hidden),
	ATTACK36 = 36 UMETA(Hidden),
	ATTACK37 = 37 UMETA(Hidden),
	ATTACK38 = 38 UMETA(Hidden),
	ATTACK39 = 39 UMETA(Hidden),
	ATTACK40 = 40 UMETA(Hidden),
	ATTACK41 = 41 UMETA(Hidden),
	ATTACK42 = 42 UMETA(Hidden),
	ATTACK43 = 43 UMETA(Hidden),
	ATTACK44 = 44 UMETA(Hidden),
	ATTACK45 = 45 UMETA(Hidden),
	ATTACK46 = 46 UMETA(Hidden),
	ATTACK47 = 47 UMETA(Hidden),
	ATTACK48 = 48 UMETA(Hidden),
	ATTACK49 = 49 UMETA(Hidden),
	ATTACK50 = 50 UMETA(Hidden),
	ATTACK51 = 51 UMETA(Hidden),
	ATTACK52 = 52 UMETA(Hidden),
	ATTACK53 = 53 UMETA(Hidden),
	ATTACK54 = 54 UMETA(Hidden),
	ATTACK55 = 55 UMETA(Hidden),
	ATTACK56 = 56 UMETA(Hidden),
	ATTACK57 = 57 UMETA(Hidden),
	ATTACK58 = 58 UMETA(Hidden),
	ATTACK59 = 59 UMETA(Hidden),
	ATTACK60 = 60 UMETA(Hidden),
	ATTACK61 = 61 UMETA(Hidden),
	ATTACK62 = 62 UMETA(Hidden),
	ATTACK63 = 63 UMETA(Hidden),
	ATTACK64 = 64 UMETA(Hidden),
	ATTACK65 = 65 UMETA(Hidden),
	ATTACK66 = 66 UMETA(Hidden),
	ATTACK67 = 67 UMETA(Hidden),
	ATTACK68 = 68 UMETA(Hidden),
	ATTACK69 = 69 UMETA(Hidden),
	ATTACK70 = 70 UMETA(Hidden),
	ATTACK71 = 71 UMETA(Hidden),
	ATTACK72 = 72 UMETA(Hidden),
	ATTACK73 = 73 UMETA(Hidden),
	ATTACK74 = 74 UMETA(Hidden),
	ATTACK75 = 75 UMETA(Hidden),
	ATTACK76 = 76 UMETA(Hidden),
	ATTACK77 = 77 UMETA(Hidden),
	ATTACK78 = 78 UMETA(Hidden),
	ATTACK79 = 79 UMETA(Hidden),
	ATTACK80 = 80 UMETA(Hidden),
	ATTACK81 = 81 UMETA(Hidden),
	ATTACK82 = 82 UMETA(Hidden),
	ATTACK83 = 83 UMETA(Hidden),
	ATTACK84 = 84 UMETA(Hidden),
	ATTACK85 = 85 UMETA(Hidden),
	ATTACK86 = 86 UMETA(Hidden),
	ATTACK87 = 87 UMETA(Hidden),
	ATTACK88 = 88 UMETA(Hidden),
	ATTACK89 = 89 UMETA(Hidden),
	ATTACK90 = 90 UMETA(Hidden),
	ATTACK91 = 91 UMETA(Hidden),
	ATTACK92 = 92 UMETA(Hidden),
	ATTACK93 = 93 UMETA(Hidden),
	ATTACK94 = 94 UMETA(Hidden),
	ATTACK95 = 95 UMETA(Hidden),
	ATTACK96 = 96 UMETA(Hidden),
	ATTACK97 = 97 UMETA(Hidden),
	ATTACK98 = 98 UMETA(Hidden),
	ATTACK99 = 99 UMETA(Hidden),

	PICK_AT_START1 = 100,	// подобрать объект, для кошки плавно наклониться сразу по нажатию
	PICK_AT_START2 = 101,	// подобрать объект, для кошки плавно наклониться сразу по нажатию
	PICK_AT_START3 = 102,	// подобрать объект, для кошки плавно наклониться сразу по нажатию

	ATTACK103 = 103 UMETA(Hidden),
	ATTACK104 = 104 UMETA(Hidden),
	ATTACK105 = 105 UMETA(Hidden),
	ATTACK106 = 106 UMETA(Hidden),
	ATTACK107 = 107 UMETA(Hidden),
	ATTACK108 = 108 UMETA(Hidden),
	ATTACK109 = 109 UMETA(Hidden),
	ATTACK110 = 110 UMETA(Hidden),
	ATTACK111 = 111 UMETA(Hidden),
	ATTACK112 = 112 UMETA(Hidden),
	ATTACK113 = 113 UMETA(Hidden),
	ATTACK114 = 114 UMETA(Hidden),
	ATTACK115 = 115 UMETA(Hidden),
	ATTACK116 = 116 UMETA(Hidden),
	ATTACK117 = 117 UMETA(Hidden),
	ATTACK118 = 118 UMETA(Hidden),
	ATTACK119 = 119 UMETA(Hidden),
	ATTACK120 = 120 UMETA(Hidden),
	ATTACK121 = 121 UMETA(Hidden),
	ATTACK122 = 122 UMETA(Hidden),
	ATTACK123 = 123 UMETA(Hidden),
	ATTACK124 = 124 UMETA(Hidden),
	ATTACK125 = 125 UMETA(Hidden),
	ATTACK126 = 126 UMETA(Hidden),
	ATTACK127 = 127 UMETA(Hidden),
	ATTACK128 = 128 UMETA(Hidden),
	ATTACK129 = 129 UMETA(Hidden),
	ATTACK130 = 130 UMETA(Hidden),
	ATTACK131 = 131 UMETA(Hidden),
	ATTACK132 = 132 UMETA(Hidden),
	ATTACK133 = 133 UMETA(Hidden),
	ATTACK134 = 134 UMETA(Hidden),
	ATTACK135 = 135 UMETA(Hidden),
	ATTACK136 = 136 UMETA(Hidden),
	ATTACK137 = 137 UMETA(Hidden),
	ATTACK138 = 138 UMETA(Hidden),
	ATTACK139 = 139 UMETA(Hidden),
	ATTACK140 = 140 UMETA(Hidden),
	ATTACK141 = 141 UMETA(Hidden),
	ATTACK142 = 142 UMETA(Hidden),
	ATTACK143 = 143 UMETA(Hidden),
	ATTACK144 = 144 UMETA(Hidden),
	ATTACK145 = 145 UMETA(Hidden),
	ATTACK146 = 146 UMETA(Hidden),
	ATTACK147 = 147 UMETA(Hidden),
	ATTACK148 = 148 UMETA(Hidden),

	Bump_GoodWall_Fast = 149,
	Bump_GoodWall_Slow = 150,
	Bump_BadWall_Slow = 151,
	Bump_BadWall_Fast = 152,
	Bump_Rear_Slow = 153,
	Bump_Rear_Fast = 154,

	//действие при желании залезть и обнаружении пригодной для лазанья поверхности, которую касается передняя часть тела или виртуальные ноги
	Bump_Climb_GoodWall = 155,

	//действие при желании залезть и обнаружении неподходящей поверхности под ногами или в контакте с верхом
	Bump_Climb_BadWall = 156,

	RaiseFront = 157,

	RaiseBack = 158,

	ATTACK159 = 159 UMETA(Hidden),
	ATTACK160 = 160 UMETA(Hidden),
	ATTACK161 = 161 UMETA(Hidden),
	ATTACK162 = 162 UMETA(Hidden),
	ATTACK163 = 163 UMETA(Hidden),
	ATTACK164 = 164 UMETA(Hidden),
	ATTACK165 = 165 UMETA(Hidden),
	ATTACK166 = 166 UMETA(Hidden),
	ATTACK167 = 167 UMETA(Hidden),
	ATTACK168 = 168 UMETA(Hidden),
	ATTACK169 = 169 UMETA(Hidden),
	ATTACK170 = 170 UMETA(Hidden),
	ATTACK171 = 171 UMETA(Hidden),
	ATTACK172 = 172 UMETA(Hidden),
	ATTACK173 = 173 UMETA(Hidden),
	ATTACK174 = 174 UMETA(Hidden),
	ATTACK175 = 175 UMETA(Hidden),
	ATTACK176 = 176 UMETA(Hidden),
	ATTACK177 = 177 UMETA(Hidden),
	ATTACK178 = 178 UMETA(Hidden),
	ATTACK179 = 179 UMETA(Hidden),

	SELF_EXPRESS1 = 180,		//самодействие выражающее реальную угрозу, например шипение и разворот боком
	SELF_EXPRESS2 = 181,		//самодействие выражающее реальную угрозу, например шипение и разворот боком
	SELF_EXPRESS3 = 182,		//самодействие выражающее реальную угрозу, например шипение и разворот боком

	ATTACK183 = 183 UMETA(Hidden),
	ATTACK184 = 184 UMETA(Hidden),
	ATTACK185 = 185 UMETA(Hidden),
	ATTACK186 = 186 UMETA(Hidden),
	ATTACK187 = 187 UMETA(Hidden),
	ATTACK188 = 188 UMETA(Hidden),
	ATTACK189 = 189 UMETA(Hidden),

	SELF_PRANCE1 = 190,				//встать на дыбы
	SELF_PRANCE_QUICK_AT_BUMP = 191,//встать на дыбы резко при ударении головой по лазибельной поверхности
	SELF_PRANCE3 = 192,				//встать на дыбы

	ATTACK193 = 193 UMETA(Hidden),
	ATTACK194 = 194 UMETA(Hidden),

	SELF_PRANCE_BACK = 195,			//подпрыгнуть задом
	RECOIL_AT_BUMP = 196,			//отпрянуть в ответ на уткнутие
	JUMP_BACK_AT_BUMP = 197,		//подпрыгнуть задом

	ATTACK198 = 198 UMETA(Hidden),
	ATTACK199 = 199 UMETA(Hidden),

	SELF_WRIGGLE1 = 200,			//извиваться лежа, чтобы встать
	SELF_TWIST_IN_FALL = 201,		//выкрутиться в падении, чтобы ускорить вращение
	SELF_STABILIZE_IN_FALL = 203,	//замедлить вращение в падении, выставив ноги вниз

	ATTACK203 = 203 UMETA(Hidden),
	ATTACK204 = 204 UMETA(Hidden),

	SELF_YAWN = 205,			//зевать
	SELF_LICK = 206,			//зевать
	SELF_YAWN3 = 207,			//зевать
		
	SELF_TONGUE1 = 208,			//движение языком
	SELF_TONGUE2 = 209,			//движение языком

	SELF_STRETCH1 = 210,			//постянуться, разминая мышцы (зевнуть и тп)
	SELF_STRETCH2 = 211,			//постянуться, разминая мышцы (зевнуть и тп)
	SELF_STRETCH3 = 212,			//постянуться, разминая мышцы (зевнуть и тп)

	ATTACK213 = 213 UMETA(Hidden),
	ATTACK214 = 214 UMETA(Hidden),
	
	SELF_DYING1 = 215,			//умирание

	ATTACK216 = 216 UMETA(Hidden),
	ATTACK217 = 217 UMETA(Hidden),
	ATTACK218 = 218 UMETA(Hidden),
	ATTACK219 = 219 UMETA(Hidden),

	RELAX_SITDOWN1 = 220,			//войти в состояние отдыха (сидя)
	RELAX_SITDOWN2 = 221,			//войти в состояние отдыха (сидя)
	RELAX_SLEEP = 222,				//войти в отдых сон (особая хрень, запускает телепорт в сновидение)
	RELAX_SLEEP_KINEMATIC = 223,	//войти в отдых сон (особая хрень, запускает телепорт в сновидение)

	ATTACK224 = 224 UMETA(Hidden),
	ATTACK225 = 225 UMETA(Hidden),
	ATTACK226 = 226 UMETA(Hidden),
	ATTACK227 = 227 UMETA(Hidden),
	ATTACK228 = 228 UMETA(Hidden),
	ATTACK229 = 229 UMETA(Hidden),


	ATTACK_PARRY1 = 230,			//специальная атака, ни на что другое, кроме как на парирование и немедленную контратаку, не пригодная
	ATTACK_PARRY2 = 231,			//специальная атака, ни на что другое, кроме как на парирование и немедленную контратаку, не пригодная
	ATTACK_PARRY3 = 232,			//специальная атака, ни на что другое, кроме как на парирование и немедленную контратаку, не пригодная

	ATTACK233 = 233 UMETA(Hidden),
	ATTACK234 = 234 UMETA(Hidden),
	ATTACK235 = 235 UMETA(Hidden),
	ATTACK236 = 236 UMETA(Hidden),

	COMPLAIN_TIRED = 237,			//действие, чтоб показать, что сатмина на нуле

	ATTACK238 = 238 UMETA(Hidden),
	ATTACK239 = 239 UMETA(Hidden),

	JUMP_FORTH1 = 240,				//прыгнуть вперед
	JUMP_FORTH2 = 241,				//прыгнуть вперед
	JUMP_FORTH3 = 242,				//прыгнуть вперед

	ATTACK243 = 243 UMETA(Hidden),
	ATTACK244 = 244 UMETA(Hidden),

	JUMP_BACK1 = 245,				//прыгнуть назад
	JUMP_BACK2 = 246,				//прыгнуть назад

	Sprint = 247,				
	Move = 248,
	Walk = 249,
	Run = 250,
	Crouch = 251,
	Climb = 252,
	Fly = 253,
	Soar = 254,
	NONE = 255
};

//фазы действия, более подробно чем ранее для атак, не для всех все будут выполняться
UENUM(BlueprintType) enum class EActionPhase : uint8
{
	ASCEND = 0,		// начало подготовки действия с нуля, нормальная анимация вперед
	READY = 1,		// достижение подготовкой точки ожидания спуска, очень медленная анимация назад
	RUSH = 2,		// действие готовится, но спуск гарантирован, подготовки не будет 
	STRIKE = 3,		// активное действие после сигнала спуска
	FINISH = 4,		// активность кончилась, анимация продолжается до нуля
	DESCEND = 5,	// возврат назад к нулю или к READY
	NUM = 6,
	NONE = 255		// нет фазы, используется совместно с нулевой атакой
};


//костыль, из-за того, что в аргументах TMap нельзя уточнять, как будет показываться свойство
USTRUCT(BlueprintType) struct FActionPhaseSet
{
	GENERATED_USTRUCT_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = EActionPhase)) int32 Phases;
};


//для отладки и не только - результат предложения сделать атаку
UENUM() enum class EResult : uint8
{
	NO_WILL = 0,
	WRONG_ACTOR,
	INCOMPLETE_DATA,
	OUT_OF_RADIUS,		// цель за пределами радиуса/кольца, где это действие имеет эффект
	OUT_OF_ANGLE,		// цель за пределами угла, где это действие имеет эффект
	OUT_OF_SENTIMENT,	// не атаковали, потому что не те чувства к цели испытываем
	OUT_OF_VELOCITY,	// не атаковали, потому скорость цели не та
	OUT_OF_CHANCE,		// не атаковали потому что вероятность не выпала
	OUT_OF_BEHAVE_STATE,
	OUT_OF_STAYING_MUTE,
	TOO_WEAK_AI,
	FORBIDDEN_IN_RELAX,
	OUT_OF_STAMINA,
	OUT_OF_HEALTH,
	OUT_OF_SLEEPINESS,
	OUT_OF_REPEATS,
	ALREADY_RUNNING,	// если мы пытаемся с нуля началь атаку, а существо уже какую-то атаку выполняет
	ATE_INSTEAD_OF_RUNNING,	// опция съесть то что в зубах и не начинать атаку
	STARTED,			// атака началась
	STROKE,				// атака достигла кульминации
	RUSHED_TO_STRIKE,	// подтверждена дарока к боевой фазе, сам переход случится в закладке анимации
	GONE_TO_STRIKE_AGAIN,// ушёл в режим descend чтобы повторно ударить по короткому пути
	WRONG_PHASE,		// мы уже атакуем в какой-то фазе, новую атаку вызвать нельзя
	UNSAFE_PARRY,		// нас вызвали парировать, но атака слишком мощна, так что мы лучше увернемся
	UNSAFE_EVADE,		// нас вызвали эффектно увернуться, но атака слишком мощна
	LOW_STAMINA,		// слишком мало запаса сил для атаки	
	FAILED_TO_START,	// что-то пошло не так и попытка не состоялась
	FAILED_TO_STRIKE,	// что-то пошло не так и попытка не состоялась
	NO_USEFUL_FOUND,
	LOW_PRIORITY,
	INJURED,
	NO_GOAL,
	OKAY_FOR_NOW,

	

};

inline bool ActOk(EResult R) { return (R == EResult::OKAY_FOR_NOW); }
inline bool StrOk(EResult R) { return (R == EResult::STROKE || R == EResult::RUSHED_TO_STRIKE || R == EResult::GONE_TO_STRIKE_AGAIN); }


//кнопки мыши и клавиши
//надо переименовать на суть, убрать названия клавиш
enum PLAYER_CAUSE
{
	PCK_LBUTTON,
	PCK_MBUTTON,
	PCK_RBUTTON,
	PCK_PARRY,
	PCK_USE,
	PCK_SPACE,
	PCK_ALT,
	PCK_CTRL,
	PCK_SHIFT,
	PCK_SOAR,
	PCK_MAXIMUM
};

//Имена атак, взываемые игроком по кнопкам
USTRUCT(BlueprintType) struct FBehaveStatePlayerTriggeredActions
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAction ActionLButton = EAction::ATTACK_PAW_1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAction ActionMButton = EAction::Climb;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAction ActionRButton = EAction::ATTACK_MOUTH_1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAction ActionQParry = EAction::ATTACK_PARRY1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAction ActionEUse = EAction::PICK_AT_START1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAction ActionSpace = EAction::JUMP_FORTH1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAction ActionAlt = EAction::JUMP_BACK1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAction ActionCtrl = EAction::Crouch;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAction ActionShift = EAction::Run;

	//обращение по индексу
	EAction ByIndex(int i) { return ((EAction*)this)[i]; }
};
