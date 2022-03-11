#pragma once
#include "CoreMinimal.h"
//#include "Templates/SharedPointer.h"
#include "myrra_emotions.h"
#include "myrra_roleparams.h"
#include "Engine/DataTable.h"			//чтобы эмоциональные отклики вынести в таблицу
#include "myrra_gameplay.generated.h"


//события, значимые для геймплея и сюжета
//навести порядок, переименовать во что-то более адекватное
UENUM(BlueprintType) enum class EMyrLogicEvent : uint8
{
	NO,

	SelfSpawnedAtLevel,							// это существо только появилось на уровне
	SelfRelaxStart,								// войти в релакс
	SelfRelaxEnd,								// выйти из релакса
	SelfSleepStart,								// войти в сон
	SelfDied,									// когда померли на данном уровне


	ObjNoticedFirstTime,	// для ИИ, когда заметил цель первый раз
	ObjRecognizedFirstTime,	// для ИИ, когда уверенность в опознавании достигла 1 первый раз
	ObjDroppedOtherForYou,	// для ИИ, выбили другую цель ради этой, ибо формально посчитали, что важнее
	ObjKnowYouButUnsure,	// для ИИ, заметили не впервый раз, но ещё не поняли до конца кто это

	SelfHarmBegin,	// начать атаку
	SelfGraceBegin,	// начать атаку

		 MePatient_PlannedHurt,					// агент нанес пациенту увечье намеренно атакуя
	MePatient_FirstPlannedHurt,					// агент намеренно покалечил пациента, первый раз
		 MePatient_PlannedHurtByAuthority,		// агент пациента не первый раз бьет, но агент хозяин или авторитет для пациента, для него это нормально
	MePatient_FirstPlannedHurtByAuthority,		// агент намеренно покалечил пациента, первый раз, но агент для пациента авторитет
		 MePatient_PlannedHurtTowardsAuthority,	// агент нас не первый раз бьет, но это мы авторитет, а он на нас руку поднимает
	MePatient_FirstPlannedHurtTowardsAuthority,	// агент намеренно нас покалечил, первый раз, но это мы авторитет, а он на нас руку поднимает
		 MePatient_PlannedHurtByFriend,			// агент намеренно нас покалечил, а мы к нему хорошо относились!
	MePatient_FirstPlannedHurtByFriend,			// агент намеренно нас покалечил, первый раз, а мы к нему хорошо относились!
	 MePatient_PlannedHurtByBigFearful,			// агент намеренно нас покалечил, и он здоровый и страшный
	 MePatient_PlannedHurtByLittlePoor,			// агент намеренно нас покалечил, и он мелкий и убогий
	 MePatient_PlannedHurtTillMyDeath,			// агент намеренно наносит нам фатальные повреждения, зная, что наше здоровье на исходе
	MePatient_HurtParried,						//*агент нас атаковал, но мы хотя бы попытались парировать
		 MePatient_CasualHurt,					// агент нанес нам увечье случайно
	MePatient_FirstCasualHurt,					// агент нанес нам увечье случайно первый раз, до этого не наносил ни разу
		 MePatient_CasualHurtByFriend,			// агент случайно нас покалечил, а мы к нему хорошо относились!
	MePatient_FirstCasualHurtByFriend,			// агент случайно нас покалечил, первый раз, а мы к нему хорошо относились!
		 MePatient_CasualHurtTowardsAuthority,	// агент нас не первый раз бьет, но это мы авторитет, а он на нас руку поднимает
	MePatient_FirstCasualHurtTowardsAuthority,	// агент намеренно нас покалечил, первый раз, но это мы авторитет, а он на нас руку поднимает

		 MeAgent_PlannedHurt,					// объект нанес на увечье намеренно нас атакуя
	MeAgent_FirstPlannedHurt,					// объект намеренно нас покалечил, первый раз
		 MeAgent_PlannedHurtByAuthority,		// объект нас не первый раз бьет, но это наш хозяин или авторитет
	MeAgent_FirstPlannedHurtByAuthority,		// объект намеренно нас покалечил, первый раз, но это наш царь, хозяин и бог
		 MeAgent_PlannedHurtTowardsAuthority,	// объект нас не первый раз бьет, но это мы авторитет, а он на нас руку поднимает
	MeAgent_FirstPlannedHurtTowardsAuthority,	// объект намеренно нас покалечил, первый раз, но это мы авторитет, а он на нас руку поднимает
		 MeAgent_PlannedHurtByFriend,			// объект намеренно нас покалечил, а мы к нему хорошо относились!
	MeAgent_FirstPlannedHurtByFriend,			// объект намеренно нас покалечил, первый раз, а мы к нему хорошо относились!
	 MeAgent_PlannedHurtByBigFearful,			// объект намеренно нас покалечил, и он здоровый и страшный
	 MeAgent_PlannedHurtByLittlePoor,			// объект намеренно нас покалечил, и он мелкий и убогий
	 MeAgent_PlannedHurtTillMyDeath,			// объект намеренно наносит нам фатальные повреждения, зная, что наше здоровье на исходе
	MeAgent_HurtParried,						//*объект нас атаковал, но мы хотя бы попытались парировать
		 MeAgent_CasualHurt,					// объект нанес нам увечье случайно
	MeAgent_FirstCasualHurt,					// объект нанес нам увечье случайно первый раз, до этого не наносил ни разу
		 MeAgent_CasualHurtByFriend,			// объект случайно нас покалечил, а мы к нему хорошо относились!
	MeAgent_FirstCasualHurtByFriend,			// объект случайно нас покалечил, первый раз, а мы к нему хорошо относились!
		 MeAgent_CasualHurtTowardsAuthority,	// объект нас не первый раз бьет, но это мы авторитет, а он на нас руку поднимает
	MeAgent_FirstCasualHurtTowardsAuthority,	// объект намеренно нас покалечил, первый раз, но это мы авторитет, а он на нас руку поднимает

	MePatient_PlannedGrace,						// нам намеренно сделали добродетель
	MePatient_PlannedGraceByEnemy,				// нам сделал добродетель тот, к кому мы плохо относимся (но не факт что он к нам, или он при смерти)
	MePatient_PlannedGraceByAuthority,			// нам сделал добродетель наш авторитет или хозяин
	MePatient_CasualGrace,						// нам случайно кто-то привнес добродетель
	MePatient_CasualGraceByEnemy,				// нам случайно сделал добродетель тот, к кому мы плохо относимся

	MeAgent_PlannedGrace,						// мы намеренно сотворили кому-то добродетель
	MeAgent_PlannedGraceByEnemy,				// мы намеренно сделали добро тому, кто считал нас врагом
	MeAgent_PlannedGraceByAuthority,			// мы хозяин и намеренно поощрили своего раба
	MeAgent_CasualGrace,						// мы случайно кому-то делали добро
	MeAgent_CasualGraceByEnemy,					// мы случайно сделали хорошо тому, кто считал нас врагом

	//простые действия с объектами
	ObjGrab,									// взять объект в руки или в рот
	ObjUnGrab,									// бросить или утерять взятый объект
	ObjEat,										// съесть любой объект, живой или нет
	ObjOpenDoor,								// открыть дверь, указывается компонент-дверь
	ObjUnlockDoor,								// отпереть дверь, подействовав на триггер
	ObjAffectTrigger,							// мы просто пересекли триггер-объём, который призван что-то обозначать
	ObjEnterLocation,							// мы пересекли триггер-объём, который символизирует локацию - больше не для эмоций, а для триггера глобальной логики
	ObjExitLocation,							// мы вышли за триггер-объём, который символизирует локацию - больше не для эмоций, а для триггера глобальной логики
	ObjEnterQuietPlace,							// мы пересекли триггер-объём тихого места - для успокоения эмоций
	ObjExitQuietPlace,							// мы вышли за триггер-объём тихого места - для успокоения эмоций

	SelfJump,									// просто прыжок, сам факт

	//тут хз пока, как это должно работать
	MePatient_AffectByExpression,				// когда на нас подействовали чужим выражением (внимание, конкретное выражение задаётся действием, а не общим списком откликов)
	MeAgent_AffectByExpression,					// мы запустили самовыражение
	MePatient_GuessDeceivedByExpression,		// когда мы понимаем, что экпрессия агента не соответствовала актуальной эмоции, а значит он мухлевал
	MeAgent_GuessDeceivedByExpression,			// когда экспрессор понимает, что экпрессия его не соответствовала актуальной эмоции, а значит он мухлевал

	SelfStableAndBoring,							//когда ничего не происходит долгое время

	ObjNowUnreachable,							// объект стал недоступный
	ObjNowReachable								// объект был недоступный, стал доступный

};

//###################################################################################################################
// то, что записывается глобально и по чему ищутся критерии для старта квестов - 
//внимание, само EMyrLogicEvent - ключ к этому в мультикарте. или может нах?
//формировать каждый раз динамическую карту срабатываний для текущих квестов?
//###################################################################################################################
USTRUCT(BlueprintType) struct FMyrLogicMainRecord
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) APawn* Instigator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UObject* Victim;
};

//###################################################################################################################
// 2 цели: регистрация событий, от которых зависит прокачка навыков и общий механизм изменения эмоций при этих событиях
// по идее, список/карта этих событий должна быть индивидуальна для класса существ или для даже отдельного существа 
// видимо, завести больше разных цветов-эмоций для разных ситуаций (первый раз, не первый раз)
//###################################################################################################################
USTRUCT(BlueprintType) struct FMyrLogicEventData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	//приоритет - если последний записанный случай имел больший приоритет, этот новый записываться не будет, пока старый не отыграет свой хвост афтершоков
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Priority = 0;
	
	//непосредственное применение к своим эмоциям / пока неясно, как смешивать свои эмоции и чувства к целям, нужны абсолютные веса
	//скорее всего, смешивать с коэффициентом в альфа-канале каждый раз
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor EmotionDeltaForMe;

	//если событие связано с целью, то в ИИ фигурирет эмоция по отношению к цели, ее постепенно изменять на дельту
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor EmotionDeltaForGoal;

	//после первой решистрации применять подмешивание новой эмоции еще данное количество тиков (или меньше, если придёт новое событие)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 TimesToUseAfterCatch = 0;	

	//при егистрации может вводиться аргумент, специфичный для действия, и он влияет на применение эмоций 
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool MultEmotionByInputScalar = false;

	//какие ролевые праметры увеличивает (или уменьшает) событие
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<EPhene, int> ExperienceIncrements;	

	//выдать готовую цветовую суть-настройку эмоции сразу переведенную во флоат
	FLinearColor Emo(bool ForGoal) { return (ForGoal ? EmotionDeltaForGoal : EmotionDeltaForMe).ReinterpretAsLinear(); }

	//остаточный коэффициент воздействия этой эмоции, когда она уже лежит грузом на душе какое-то число тактов
	float Taper(int LastEventTicks) { return (float)LastEventTicks / (TimesToUseAfterCatch + 1); }
};

//###################################################################################################################
//стек последних событий, плозая потому что все кроме посленего события расположены случайно
//###################################################################################################################
USTRUCT(BlueprintType) struct FEmotionMemory
{
	GENERATED_USTRUCT_BODY()

	EMyrLogicEvent Events[8];	// идентификаторы предыдущих событий в той же последовательности, пока впустую
	uint8 Mults[8];				// коэффициенты внесенных событий (хз зачем весь стек)
	FMyrLogicEventData* LastInfo = nullptr; // указатель на полную сборку последнего события

	//счётчик-декремент до нуля пройденных доп-тактов влияния последней эмоции (для афтершоков)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 LastEventTicks = 0;	

	//накопленная эмоция, результат работы
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEmotion Emotion;

	////////////////////////////////////
	uint64& AsOne() { return *((uint64*)Events); }
	uint64& AsOneMult() { return *((uint64*)Events); }

	//конструктадор
	FEmotionMemory() { for(int i=1;i<8;i++) Events[i] = EMyrLogicEvent::NO; }

	//последний
	EMyrLogicEvent Last() const { return Events[0]; }
	uint8& LastMult() { return Mults[0]; }
	float LastMultReal() const { return (float)Mults[0] / 255.0f; }

	//содержит во всей памяти
	int ContainsDeep(EMyrLogicEvent Q)	{ int R=0; for(int i=1;i<8;i++) if(Events[i] == Q) R++; return R; }

	//элементарный акт смешивания эмоции, полуфабрикат для других операций,
	//тут не нужно знать, какое конкретное событияе вызвало данный прилив эмоций
	void Change(FMyrLogicEventData* EventInfo, bool ForGoal, float ExtFactor)
	{	FLinearColor NewEmo = EventInfo->Emo(ForGoal);
		Emotion.EquiColor = FMath::Lerp ( Emotion.EquiColor, NewEmo,
			NewEmo.A * (EventInfo->MultEmotionByInputScalar ? ExtFactor : 1.0f));
	}
	
	//запомнить сюда новое событие и тут же свершить прилив реальной эмоции (ForGoal - ибо не знаем, в чём лежит эта структура)
	void AddEvent(EMyrLogicEvent New, FMyrLogicEventData* EventData, float Mult, bool ForGoal)
	{	if(!EventData) return;
		if(LastInfo)
			if(EventData->Priority < LastInfo->Priority) return;
		AsOne() = AsOne() >> 8;						// одной операцией передвигаем все байты вглубь, освобождая 0 байт
		AsOneMult() = AsOneMult() >> 8;				// одной операцией передвигаем все байты вглубь, освобождая 0 байт
		Events[0] = New;							// кладём на освободившееся место новое событие
		LastEventTicks = EventData->TimesToUseAfterCatch;	// взводим счётчик
		LastMult() = (uint8)FMath::Clamp(Mult, 0.0f, 1.0f)*255;	// сохраняем коэффициент силы
		Change(EventData, ForGoal, Mult);			// свершить прилив эмоций
		LastInfo = EventData;						// сохранить последнее
	}

	//рутинное тактовое изменение эмоции слогласно последнему положенному в память событию
	//внимание, кода шлейф памяти о последнем потрясении кончился, она просто выдает фолс, а дальше надо вовне решать
	bool Tick(bool ForGoal)
	{	
		if (!LastInfo) return false;
		if (LastEventTicks > 0)						// последнее событие еще не исчерпала волнительность
		{	float Mult = LastMultReal()				// коэффициент актуальности посленего события
				* LastInfo->Taper(LastEventTicks);	// из начальной актуальности и гаснущего шлейфа
			Change(LastInfo, ForGoal, Mult);		// применить прилив реальной эмоции
			LastEventTicks--;						// стало на такт меньше
			return true;
		}
		else return false;
	}

	//стереть факт фиксации события
	void Erase() { Events[0] = EMyrLogicEvent::NO; LastInfo = nullptr; }

};

//еда - возможно, перенести в отдельный файл


//###################################################################################################################
// пищевой эффект при съедании существа или объекта
//###################################################################################################################
USTRUCT(BlueprintType) struct FDigestivity
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DeltaEnergy = 0.0f;						// сколько энергии
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DeltaHealth = 0.0f;						// сколько здоровья
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DeltaStamina = 0.0f;						// сколько сил
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Time = 0;									// время действия эффекта
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AActor> WhatRemains;				// какой объект создавать на месте съеденного
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class USoundBase* SoundAtEat;					// звук при еде
};



//...для типа triggervolume
//###################################################################################################################
//причина использования триггера
//###################################################################################################################
UENUM(BlueprintType) enum class EWhyTrigger : uint8
{
	CameraDist,					//изменить расстояние камеры, обычно приблизить, строку надо целиком превести в число

	UnlockDoorLightButton,		//открыть дверь, если привязана кнопка, то она из несветящейся становится светящейся
	UnlockDoorDimButton,		//открыть дверь, в строке указывается имя компонента двери в том же акторе
	UnlockDoorFlashButton,		//открыть дверь, в строке указывается имя компонента двери в том же акторе

	SpawnAtComeIn,				//генернуть что-то, видимо, существо (в параметре указать имя специального компонента спавнера)
	SpawnAtComeOut,				//генернуть что-то, видимо, существо (в параметре указать имя специального компонента спавнера)
	SpawnAtInDestroyAtOut,		//генернуть что-то, например растение при входе в и уничтожить на выходе

	Attract,					//привлекать с помощью ИИ существ определенного вида (пока неясно, как формировать эмоцию, видимо в паре со Spawner, пусть и нерабочим
	Destroy,					//удалить актора-пересекателя со сцены (пока неясно, как фильтровать поддающихся этой участи)

	Eat,						//съесть неживое - указывается компонент свитчабл, который с каждым кусем +1 индекс воплощения

	Quiet,						//успокоитель

	NONE
};

//###################################################################################################################
//дополнительное увеомление на экран при встрече триггера
//###################################################################################################################
UENUM(BlueprintType) enum class ETriggerNotify : uint8
{
	NONE,
	CanClimb,
	CanEat,
	CanSleep
};


//###################################################################################################################
//для компонента триггера: причина срабатывания и указания что делать в виде строки
//в зависимости от причины строка интерпретируется по своему
//###################################################################################################################
USTRUCT(BlueprintType) struct FTriggerReason
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EWhyTrigger Why;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Value;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ETriggerNotify Notify;
};



//###################################################################################################################
//цеплялка в составе квеста, копируется в лист ожидания
//здесь не проявляются внутренние условия стадийности, если оно скопировалось, значит, условия уже выполнены
//###################################################################################################################
USTRUCT(BlueprintType) struct FMyrQuestTrigger
{
	GENERATED_USTRUCT_BODY()

	//человеко-понятное имя пославшего это событие, можно ввести в редакторе
	//не указатель, так как объект может быть еще не высран на уровне
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Instigator;

	//тип пославшего это событие - если конкретное имя не указано, сравнивается тип
	//тип вводится напрямую, посольку все классы изначально загружены, можно выбрать в редакторе из списка
	//тип стандартный (не существо) чтоб не тянуть сюда заголовки лишние
	//а сохранять все равно как getName
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<APawn> InstigatorType;

	//человеко-понятное имя объекта, с которым пославший сотворил это событие, можно ввести в редакторе
	//это может быть и компонент, и актор
	//не указатель, так как объект может быть еще не высран на уровне
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Destination;

	//тип объекта, с которым пославший сотворил это событие - если конкретное имя не указано, сравнивается тип
	//тип вводится напрямую, посольку все классы изначально загружены, можно выбрать в редакторе из списка
	//а сохранять все равно как getName
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UClass* DestinationType;

	//сюда еще кучу всякого можно внести, но нах, лучше полагаться на множество уже ранее определенных эвентов
	//если эта сборка уже выставлена в лист ожидания, значит внктри квеста всё посчитано

	//квест, который выставил эту цеплялку (он сам себя подвязывает, когда кладет в лист ожидания)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) class UMyrQuest* OwningQuest;
	
	//пока неяснол, как адресовать части квеста, связанные с цеплялками, пусть пока будет так
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 QuestTransID;

};

//###################################################################################################################
//элемент карты для проверки применимости квестов - под ключом из типа MyrLogicEvent
//###################################################################################################################
USTRUCT(BlueprintType) struct FMyrQuestsToStart
{
	GENERATED_USTRUCT_BODY()

	//предполагается, что на каждое элементарное действие MyrLogicEvent не будет накапливаться много квестов
	//поэтому отыскиваться они будут перебором простого массива внутри
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FMyrQuestTrigger> AvailableQuestTriggers;
};





