#pragma once
#include "CoreMinimal.h"
#include "myrra_emotions.h"
#include "myrra_gameplay.h"
#include "myrra_ai.generated.h"

//###################################################################################################################
//пространственно-иерархическое отношение между текущим (Me) и целевым (You) объектом на текущем уровне
//пока вообще отменено и не используется
//###################################################################################################################
UENUM(BlueprintType) enum class EYouAndMe : uint8
{
	Touching,			// цель непосредственно касается нас

	InAir,				// цель или мы в воздухе - нас разделяет воздух, и по опорам цели не достигнуть

	OnTheSameFloor,		// цель на той же опоре, что и мы (монолитный пол или один и тот же сегмент древа)
	OnTheSameTree,		// цель на том же компоненте, что и мы, но на ином сегменте - нужно идти не на прямую а по цепи/поддереву
	OnTheSameStairs,	// цель на том же статик меше, но на разных колизионный телах оного

	ThruMyTree,			// цель на неясной опоре, а мы на дереве, и надо нам пройти свое дерево, чтобы дойти до цели
	ThruMyParentFloor,	// цель на неясной опоре, но, чтобы выйти на эту иерархию, должны спуститься на материнский компонент

	OnChildFloor,		// цель на дочернем компоненте к нашей опоре (оно на почке нашей земли)
	OnTreeChildFloor,	// цель на дочернем компоненте к нашей опоре, но наша опора - древа, к сегменту которого прикреплена опора цели
	OnParentFloor,		// цель на материнском компоненте к нашей опоре (это мы на почке)
	OnParentTree,		//
	OnFloorOblique,		// цель на дочернем компоненте к дочернего компонента нашей опоры
	OnAdjacentTrees,	// мы на дереве, и цель на дереве, но между деревьями есть переходы/перепрыги

	MeFleeingByObstacle,	// мы убегаем от цели, цель всё равно где, но перед нами препятствие и его надо обойти
	MeChasingByObstacle,	// мы стремимся к цели, но перед нами препятствие и его надо обойти

	MeChasingYouUnreachable,// мы стремимся к цели, но цель не достать
	MeBypassingObstacleSlidingToYou,// мы обходим препятствие касаеясь его

	MeClimbingToYouOnTheSameBody,	// мы лезем по одному членику с целью


	OnObstacleToJump,	// цель на препятствии и прямо сейчас можно попробовать прыгнуть
	OnObstacleToClimb,	// цель на препятствии и прямо сейчас можно попробовать карабкаться

	MeFreezingOnBranch,	// мы на дереве, но противник расположен так, что безопаснее остаться на месте
	Unrelated
};


//###################################################################################################################
// результат восприятия новой цели и попытки ее вписать в систему текущих
//###################################################################################################################
UENUM(BlueprintType) enum class EGoalAcceptResult : uint8
{
	AdoptFirst,
	AdoptSecond,
	ReplaceFirst,
	ReplaceSecond,
	ModifyFirst,
	ModifySecond,
	Discard,
	HaveDisappeared
};

//###################################################################################################################
// результат восприятия новой цели 
//###################################################################################################################
UENUM(BlueprintType) enum class EHowSensed : uint8
{
	ROUTINE,			// никак не воспринято, просто рутинно перечитывается - нужно по умолчанию для MesureGoal
	SEEN,			// зрение - ныне не используется, так как по зрению своя система трассировок
	HEARD,			// слух - основной
	PERCUTED,		// ударен в ходе атаки
	ATTACKSTART,	// начал атаку
	ATTACKSTRIKE,	// перевел свою атаку в ударную фазу
	DIED,			// умер
	EXPRESSED,		// крикнул или прочим образом вызвал эмоцию
	CALL_OF_HOME	// зов статического объекта, откуда мы появились и где можно исчезнуть от врага
};

//для отладки и не только - результат необходимости противостоять атаке
UENUM() enum class EAttackEscapeResult : uint8
{
	NO_WILL,
	OUT_OF_AREA,			// вне опасной зоны
	NO_DANGER,				// ложная тревога
	NO_RIVAL,				// ложная тревога
	WE_PARRIED,				// активно контратаковали
	WE_PARRIED_BY_SELFACTION,// активно контратаковали
	WE_STARTED_PARRY,		// начали контратаку, но не стукнули
	WE_RETRACTED,			// мы временно отсутпили
	WE_GONNA_SUFFER,		// мы обречены словить эту атаку
	WE_GONNA_RUN,			// мы обречены, но ещё есть время убежать
	WE_KEEP_GOOD_SELFACTION,// наше текущее самодействие достаточно хорошо, чтобы противостоять атаки
	WE_KEEP_GOOD_RELAXACTION,// наше текущее самодействие достаточно хорошо, чтобы противостоять атаки
	TOO_POSITIVE		// атака слишком приятна, чтобы ее избегать
};


//сопосб разрешения трассировки пути к цели
UENUM() enum class ERouteResult : uint8
{
	Towards_Directly,
	Simple_Walkaround,
	Towards_Base,
	GiveUp_For_Unreachable,
	Away_Directly
};

//сопосб разрешения трассировки пути к цели
UENUM() enum class EMoveResult : uint8
{
	Walk_To_Goal,
	Walk_To_Goal_TOGGLE,
	Walk_To_Goal_Quietly_Turn,
	Run_To_Goal,
	Crouch_To_Goal,
	Crouch_To_Goal_TOGGLE,
	Crouch_To_Goal_SLOWLY,
	Soar_To_Goal,
	Fly_To_Goal,

	Walk_From_Goal,
	Walk_From_Goal_TOGGLE,
	Walk_From_Goal_Quietly_Turn,
	Run_From_Goal,
	Crouch_From_Goal,
	Crouch_From_Goal_TOGGLE,
	Crouch_From_Goal_SLOWLY,
	Soar_From_Goal,
	Fly_From_Goal,

	NONE
};


//###################################################################################################################
// психологическое отношение одного персонажа к другому персонажу или вообще объекту - элемент ИИ
// поля даны в минимальной битности, так как отношения и в реальности обычно ступенчатые, а в колебания в пределах
// ступеньки для принятия решения не существенны. однако даны функции перевода во float для более сложных
// алгоритмов изменения (которые получаются вероятностными, ибо приведенные в байт могут оказаться нулём)
//###################################################################################################################
USTRUCT(BlueprintType) struct FGestaltRelation
{
	GENERATED_USTRUCT_BODY()

	// текущее отношение к данному экземпляру
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Rage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Love;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Fear;

	//биты-флаги особых состояний
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 InGoal0 : 1;		// этот объект находится в оперативном внимании, в слоте целей 0
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 InGoal1 : 1;		// этот объект находится в оперативном внимании, в слоте целей 1
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 NowSeen : 1;		// этот объект сейчас виден (видимость фиксируется в момент растмения и затмения, так что нужен флаг) но зачем в гештальте неясно
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Unreachable : 1;	// этот объект недостижим (например, строго вверху), надо отбежать или бегать кругом, но не стоять на месте 
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 NeverBefore : 1;	// этот объект был занесен в память первый раз, с ним еще не проводилось никаких действий и переживаний
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 ItDamagedUs : 1;	// этот объект атаковал нас враждебностью и доставлял боль хоть раз
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Authority : 1;	// этот объект представляет собой непререкаемый авторитет, уважение и т.п.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Reserved5 : 1;	// 

	//получить эмоцию извне
	void		FromFColor(FColor c) { Rage = c.R; Love = c.G; Fear = c.B; }
	void		FromFLinearColor(FLinearColor c) { Rage = c.R*255; Love = c.G*255; Fear = c.B*255; }

	//выдать эмоцию наружу
	FColor			ToFColor() const { return FColor(Rage, Love, Fear, 255); }
	FLinearColor	ToFLinearColor() const { return FLinearColor(Rage/255.0f, Love/255.0f, Fear/255.0f, 1.0f); }

	//хз, наверно уже не нужно
	bool IsHome() const { return (Rage == 255 && Love == 255 && Fear == 255);  }

	//выдать полноформатную эмоцию и сохранить эмоцию в образе
	FEmotion GetEmotion() const { return FEmotion ( Rage/255.0, Love/255.0, Fear/255.0); }
	void SaveEmotionToLongMem(FLinearColor E, float Alpha) { Rage = E.R*255; Love = E.G*255; Fear = E.B*255; }

	void SaveEmotionToLongTermMem(FEmotionMemory& EM) { FromFLinearColor(FMath::Lerp(ToFLinearColor(), EM.Emotion, EM.GetSure())); }

	FGestaltRelation() {
		InGoal0 = 0;
		InGoal1 = 0;
		NowSeen = 0;
		NeverBefore = 0;
		ItDamagedUs = 0;
	}
};

//###################################################################################################################
// ◘ новая попытка собрать данные по одной цели оперативная цель
//###################################################################################################################
USTRUCT(BlueprintType) struct FGoal
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) UPrimitiveComponent* Object = nullptr;// указатель на объект в сцене (существо и ит.п.)

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) EYouAndMe Relativity;			// дальность цели иерархическая, для поиска маршрута
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) ERouteResult RouteResult;		// решение по сособу обхода

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) uint8 TicksNoEvents = 0;			// тактов, в течение которых не происходит приемов сигналов и атак
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) uint8 NoticeEvents = 0;			// актов замечания
	uint16 Ticks = 0;																// давность присутствия цели - определяет ее важность и скучность

	//курс непосредственно на цель, расстояние до цели 
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) FVector3f LookAtDir;				// направление взгляда на цель, всегда НАПРЯМУЮ, единичный вектор
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) float LookAtDist = 0.0f;			// расстояние до цели (возможно, тогда не нужен вектор, просто радиус домножать)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) float LookAtDistInv = 0.0f;		// инверсное расстояние до цели (возможно, тогда не нужен вектор, просто радиус домножать)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) float LookAlign = 0.0f;			// степень прямости взора (тупо скалярное произведение, но чтобы не пересчитывать)

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) FVector ClosestPoint;			// абсолютные координаты ближайшей точки маршрута на цель (это может быть и сама цель)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) FVector3f MoveToDir;				// направление движения на ближайшую точку маршрута до цели (это может быть и сама цель)

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) float Attraction = 0.0;			// степень тяги к(+) или от(-) цели = из кривых от расстояния,эмоций - кэш, чтобы при восприятии сразу знать, уходить или приходить
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) float Weight = 0.0;				// вес цели сравнительно с другой. так как подсчёт стал очень большим, лучше кешировать
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) float Visibility = 0.0;			// видимость (опознаваемость) для стелса, кешируется, чтобы можно было накручивать кучу формул
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) float Audibility = 0.0;			// слышимость - из-за временной дискретности актов звука надо набирать статистику

	//контейнер для регистрации событий, влияющих на логику мира (и эмоции наши), относящиеся конкретно к этой цели
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) FEmotionMemory EventMemory;

	//хз нужно ли в таком виде или заводить отдельную переменную уверенности
	float& Sure() { return EventMemory.Emotion.A; }
};

