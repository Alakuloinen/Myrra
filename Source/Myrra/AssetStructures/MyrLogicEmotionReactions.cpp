// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetStructures/MyrLogicEmotionReactions.h"

//==============================================================================================================
//забить место под новый
//==============================================================================================================
bool UMyrLogicEmotionReactions::Add(FMyrLogicEventData*& Dst, EMyrLogicEvent E)
{
	if (!MyrLogicEvents.Find(E))
		return ((Dst = &MyrLogicEvents.Add(E))!=0);
	else return false;
}



//==============================================================================================================
//конструктырь - свежий ассет в лоб забить чёткими вариантами откликов
//==============================================================================================================
UMyrLogicEmotionReactions::UMyrLogicEmotionReactions()
{
	FMyrLogicEventData* D = nullptr;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//другое существо случайно нанесло нам увечье
	if (Add(D, EMyrLogicEvent::MePatient_CasualHurt))
	{	D->EmotionDeltaForMe = FColor(255, 125, 25, 2);			// максимум гнева, саможалости и чуть-чуть страха 
		D->EmotionDeltaForGoal = FColor(200, 0, 200, 2);		// ненависть от несправедливости, ну и страх из непредсказуемости, но случайно это не намеренно, так что не максимум
		D->TimesToUseAfterCatch = 17;							// относительно легко отходим, но больно
		D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// типа тяжелое эмоциональное испытание делает нас выносливее
		D->ExperienceIncrements.Add(EPhene::Care, -1);		// типа мы озлобляемся и становимя несобранными

		if (Add(D, EMyrLogicEvent::MePatient_FirstCasualHurt))
		{	D->EmotionDeltaForMe = FColor(255, 105, 25, 3);			// максимум злости, саможалости, это всё забивает страх
			D->EmotionDeltaForGoal = FColor(128, 0, 255, 3);		// максимум страха к объекту, а вот злость первый раз от ступора малая
			D->TimesToUseAfterCatch = 85;							// непривычно, больно, обидно, поэтому долго отходим
			D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья
			D->ExperienceIncrements.Add(EPhene::Charm, 2);		// типа нас больше жалеют, но это разово
			D->ExperienceIncrements.Add(EPhene::Care, -1);		// типа мы озлобляемся и становимся несобранными
		}
	}

	//мы случайно нанесли увечья другому существу
	if (Add(D, EMyrLogicEvent::MeAgent_CasualHurt))
	{	D->EmotionDeltaForMe = FColor(100, 25, 150, 2);			// смешанные чувства к себе но больше опаски
		D->EmotionDeltaForGoal = FColor(128, 120, 25, 2);		// из-за чуства вины подниматеся любовь, но и ненависть, что под руку попало
		D->TimesToUseAfterCatch = 12;							// не очень лиегко отходим
		D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья
		D->ExperienceIncrements.Add(EPhene::Strength, 1);	// мы сделали это с силой, стали сильнее
		D->ExperienceIncrements.Add(EPhene::Care, +0.1);		// мы хотим лучше следить за собой

		if (Add(D, EMyrLogicEvent::MeAgent_FirstCasualHurt))
		{	D->EmotionDeltaForMe = FColor(50, 95, 150, 2);		// саможалость и страх, но не терминальный, всё равно драться
			D->EmotionDeltaForGoal = FColor(20, 150, 200, 1);	// неясно, чего ждать, поэтому вина (страх, жалость) + мало гнева
			D->TimesToUseAfterCatch = 17;						// непривычно, поэтому долго отходим, но из-за непопалановости быстро вытесняем
			D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья
			D->ExperienceIncrements.Add(EPhene::Charm, -2);	// привлекательность снижается
			D->ExperienceIncrements.Add(EPhene::Care, -1);	// не по плану - оттого несобранность
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//другое существо, которое мы считали другом, нанесло нам урон, хотя не планировало, а случайно
	if (Add(D, EMyrLogicEvent::MePatient_CasualHurtByFriend))
	{	D->EmotionDeltaForMe = FColor(160, 125, 205, 4);			// страх, злоба и чуть саможалости к себе
		D->EmotionDeltaForGoal = FColor(220, 120, 10, 4);		// гнев на бывшего друга, но не первый раз, так что несильно и любви остается
		D->TimesToUseAfterCatch = 55;							// серьёзная штука, долго отходим
		D->ExperienceIncrements.Add(EPhene::Charm, 1);		// типа нас больше жалеют
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// типа тяжелое эмоциональное испытание делает нас выносливее

		if (Add(D, EMyrLogicEvent::MePatient_FirstCasualHurtByFriend))
		{	D->EmotionDeltaForMe = FColor(255, 25, 25, 8);		// максимум гнева, саможалости и чуть-чуть страха 
			D->EmotionDeltaForGoal = FColor(100, 100, 100, 8);	// серое непонимание к объекту
			D->TimesToUseAfterCatch = 95;						// непривычно, и вообще серьезно долго отходим
			D->ExperienceIncrements.Add(EPhene::Stamina, 2);// типа тяжелое эмоциональное испытание делает нас выносливее
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//нас случайно ударило другое существо, но мы для него авторитет, получается оно полуобъективно виновато
	if (Add(D, EMyrLogicEvent::MePatient_CasualHurtTowardsAuthority))
	{	D->EmotionDeltaForMe = FColor(30, 100, 25, 2);		// спокойствие+саможалость, сам дурак, но опаска за свое будущее
		D->EmotionDeltaForGoal = FColor(250, 0, 90, 3);		// дурак опасный, гнев не сильный, но опаска - вдруг он свергнет
		D->TimesToUseAfterCatch = 10;						// не слишком серьезная печаль
		D->ExperienceIncrements.Add(EPhene::Charm, 1);	// типа нас больше жалеют
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);// типа тяжелое эмоциональное испытание делает нас выносливее

		if (Add(D, EMyrLogicEvent::MePatient_FirstCasualHurtTowardsAuthority))
		{	D->EmotionDeltaForMe = FColor(200, 200, 25, 2);		// саможалость плюс гнев на себя, что такое допустил
			D->EmotionDeltaForGoal = FColor(255, 100, 10, 2);	// дурак, который гневит, но которого даже жалко
			D->TimesToUseAfterCatch = 85;						// непривычная дерзость долго горит
			D->ExperienceIncrements.Add(EPhene::Charm, 2);	// типа нас больше жалеют
			D->ExperienceIncrements.Add(EPhene::Stamina, 2);// типа тяжелое эмоциональное испытание делает нас выносливее
		}
	}

	//мы случайно ударили другое существо, но оно для нас авторитет 
	if (Add(D, EMyrLogicEvent::MeAgent_CasualHurtTowardsAuthority))
	{	D->EmotionDeltaForMe = FColor(50, 150, 200, 2);		// саможалость и сильный страх возмездия
		D->EmotionDeltaForGoal = FColor(50, 200, 200, 2);	// невольный пиетет, сочувствие и сильный страх
		D->TimesToUseAfterCatch = 30;						// серьезный шаг, надолго запоминается
		D->ExperienceIncrements.Add(EPhene::Charm, -3);	// предательство понижает обояние
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);// типа тяжелое эмоциональное испытание делает нас выносливее

		if (Add(D, EMyrLogicEvent::MeAgent_FirstCasualHurtTowardsAuthority))
		{	D->EmotionDeltaForMe = FColor(100, 50, 255, 3);		// самодосада, вина, и сильныйстрах
			D->EmotionDeltaForGoal = FColor(50, 200, 255, 3);	// невольный пиетет, сочувствие и сильный страх
			D->TimesToUseAfterCatch = 40;						// серьезный шаг, надолго запоминается
			D->ExperienceIncrements.Add(EPhene::Charm, -5);	// предательство понижает обояние
			D->ExperienceIncrements.Add(EPhene::Stamina, 2);// типа тяжелое эмоциональное испытание делает нас выносливее
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//другое существо в общем случае намеренно нанесло нам увечье
	if (Add(D, EMyrLogicEvent::MePatient_PlannedHurt))
	{	D->EmotionDeltaForMe = FColor(200, 50, 200, 3);			// гнев+страх=подавленность, а самолюбие от лузерства снижается
		D->EmotionDeltaForGoal = FColor(235, 0, 100, 3);			// ненависть не максимальная, так как рутинный бой
		D->TimesToUseAfterCatch = 20;							// долгое последействие
		D->ExperienceIncrements.Add(EPhene::Care, -1);		// типа мы озлобляемся и становимя несобранными
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// накопление большей выносливости
		D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья

		if (Add(D, EMyrLogicEvent::MePatient_FirstPlannedHurt))
		{	D->EmotionDeltaForMe = FColor(80, 158, 95, 4);			// саможалость, чуть страха (неожиданно), быстро растёт
			D->EmotionDeltaForGoal = FColor(245, 10, 200, 5);		// не самый сильный страх (рутинный бой), но гнев в первый раз серьезный
			D->TimesToUseAfterCatch = 60;							// очень долгое последействие
			D->ExperienceIncrements.Add(EPhene::Care, -1);		// типа мы озлобляемся и становимя несобранными
			D->ExperienceIncrements.Add(EPhene::Stamina, 2);	// накопление большей выносливости
			D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья
		}
	}

	//мы намеренноо попали и ударили другое существо
	if (Add(D, EMyrLogicEvent::MeAgent_PlannedHurt))
	{	D->EmotionDeltaForMe = FColor(30, 235, 25, 1);			// самоудовлетворенность в отдаленной перспективе
		D->EmotionDeltaForGoal = FColor(50, 10, 25, 1);			// в сторону безразличия к побежденному
		D->TimesToUseAfterCatch = 30;							// все по плану не стоит задерживаться
		D->ExperienceIncrements.Add(EPhene::Strength, 2);	// силовое воздействие
		D->ExperienceIncrements.Add(EPhene::Agility, 2);	// попали, значит ловкость
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// накопление выносливости вяско тратим силы
		D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья жертве

		if (Add(D, EMyrLogicEvent::MeAgent_FirstPlannedHurt))
		{	D->EmotionDeltaForMe = FColor(0, 215, 100, 3);			// самоудовлетворенность но с опаской
			D->EmotionDeltaForGoal = FColor(50, 10, 25, 3);			// в сторону безразличия к побежденному
			D->TimesToUseAfterCatch = 55;							// все по плану не стоит задерживаться
			D->ExperienceIncrements.Add(EPhene::Strength, 5);	// силовое воздействие
			D->ExperienceIncrements.Add(EPhene::Agility, 5);	// попали, значит ловкость
			D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// объект раз бьет намеренно, но это наш хозяин или авторитет
	if (Add(D, EMyrLogicEvent::MePatient_PlannedHurtByAuthority))
	{	D->EmotionDeltaForMe = FColor(255, 0, 100, 1);			// вроде заслужил, а вроде еще более обидно 
		D->EmotionDeltaForGoal = FColor(55, 205, 215, 1);		// страх и обожание к авторитету, но и злобы всё больше
		D->TimesToUseAfterCatch = 15;							// долгое последействие
		D->ExperienceIncrements.Add(EPhene::Care, 1);		// он нас мотивирует
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// накопление большей выносливости через дисциплину
		D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья

		if (Add(D, EMyrLogicEvent::MePatient_FirstPlannedHurtByAuthority))
		{	D->EmotionDeltaForMe = FColor(100, 100, 0, 2);			// саможалость и чуточку гнева к себе
			D->EmotionDeltaForGoal = FColor(255, 255, 0, 2);			// обожание к объекту и злоба
			D->TimesToUseAfterCatch = 35;							// первый раз мало действует
			D->ExperienceIncrements.Add(EPhene::Care, 1);		// он нас мотивирует
			D->ExperienceIncrements.Add(EPhene::Stamina, 2);	// накопление большей выносливости через дисциплину
			D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья
		}
	}

	// мы намеренно ударили своего раба
	if (Add(D, EMyrLogicEvent::MeAgent_PlannedHurtByAuthority))
	{	D->EmotionDeltaForMe = FColor(100, 250, 50, 1);			// выброс адренолина, самоудовлетворение
		D->EmotionDeltaForGoal = FColor(100, 70, 10, 1);			// неприязнь но с ценностью
		D->TimesToUseAfterCatch = 5;								// нечего долго париться
		D->ExperienceIncrements.Add(EPhene::Strength, 2);	// силовое
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// накопление большей выносливости через дисциплину

		if (Add(D, EMyrLogicEvent::MeAgent_FirstPlannedHurtByAuthority))
		{	D->EmotionDeltaForMe = FColor(50, 250, 50, 3);			// выброс адренолина, самоудовлетворение
			D->EmotionDeltaForGoal = FColor(200, 70, 10, 2);			// неприязнь но с ценностью
			D->TimesToUseAfterCatch = 15;							// долгое последействие
			D->ExperienceIncrements.Add(EPhene::Strength, 1);	// силовое
			D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// накопление большей выносливости через дисциплину
			D->ExperienceIncrements.Add(EPhene::Care, -1);		// вывод из себя
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// агент нас бьет, но это мы авторитет, а он на нас руку намеренно поднимает
	if (Add(D, EMyrLogicEvent::MePatient_PlannedHurtTowardsAuthority))
	{	D->EmotionDeltaForMe = FColor(180, 70, 100, 1);			// для себя гнев и жалость ради успокоения, и страх, что власть рушится
		D->EmotionDeltaForGoal = FColor(255, 0, 0, 3);			// это уже не наглость, а что-то крамольное
		D->TimesToUseAfterCatch = 95;							// долгое последействие
		D->ExperienceIncrements.Add(EPhene::Care, 1);		// он нас мотивирует
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// накопление большей выносливости через дисциплину
		D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья

		if (Add(D, EMyrLogicEvent::MePatient_FirstPlannedHurtTowardsAuthority))
		{	D->EmotionDeltaForMe = FColor(255, 100, 30, 2);			// саможалость и много гнева
			D->EmotionDeltaForGoal = FColor(180, 25, 0, 1);			// злости от ступора немного и даже небольшое уважение
			D->TimesToUseAfterCatch = 10;							// первый раз мало действует
			D->ExperienceIncrements.Add(EPhene::Strength, 1);	// сила кажется что расход
			D->ExperienceIncrements.Add(EPhene::Care, 2);		// он нас мотивирует на собранность
			D->ExperienceIncrements.Add(EPhene::Stamina, 2);	// накопление большей выносливости через дисциплину
		}
	}

	//мы намеренно бъём нашего властелина
	if (Add(D, EMyrLogicEvent::MeAgent_PlannedHurtTowardsAuthority))
	{	D->EmotionDeltaForMe = FColor(50, 250, 150, 3);			// постепенно накапливаеся гордость а страх падает до среднего
		D->EmotionDeltaForGoal = FColor(255, 20, 20, 3);			// серьезный шаг продиктован большой ненавистью, страх отступает потому что уже сделано
		D->TimesToUseAfterCatch = 40;							// в первый раз поэтому долго
		D->ExperienceIncrements.Add(EPhene::Strength, 2);	// сила кажется что расход
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// накопление большей выносливости через дисциплину
		D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья

		if (Add(D, EMyrLogicEvent::MeAgent_FirstPlannedHurtTowardsAuthority))
		{	D->EmotionDeltaForMe = FColor(100, 1, 200, 3);			// вначале страх и смешанные чувства с самоуничижением
			D->EmotionDeltaForGoal = FColor(255, 255, 255, 1);		// безумие смешанных чувств к цели
			D->TimesToUseAfterCatch = 85;							// серьезное событие долго
			D->ExperienceIncrements.Add(EPhene::Charm, -3);		// намеренное предательство
			D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// накопление большей выносливости
			D->ExperienceIncrements.Add(EPhene::Strength, 2);	// сила кажется что расход
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// агент намеренно нас покалечил, а мы к нему хорошо относились, считали другом
	if (Add(D, EMyrLogicEvent::MePatient_PlannedHurtByFriend))
	{	D->EmotionDeltaForMe = FColor(100, 255, 155, 5);			// саможалость и страх за себя мощно возрпстает
		D->EmotionDeltaForGoal = FColor(255, 25, 55, 3);			// гнев и остаток любви к бывшему другу
		D->TimesToUseAfterCatch = 35;							// мощно вдаряет, но мало действует
		D->ExperienceIncrements.Add(EPhene::Care, -2);		// типа мы сильно озлобляемся и становимся несобранными
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// накопление большей выносливости
		D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья

		if (Add(D, EMyrLogicEvent::MePatient_FirstPlannedHurtByFriend))
		{	D->EmotionDeltaForMe = FColor(100, 255, 155, 2);			// саможалость и страх за себя мощно возрпстает
			D->EmotionDeltaForGoal = FColor(255, 25, 55, 1);			// гнев и остаток любви к бывшему другу - пока медленно
			D->TimesToUseAfterCatch = 125;							// первый раз и необычное событие долго
			D->ExperienceIncrements.Add(EPhene::Care, -2);		// типа мы сильно озлобляемся и становимся несобранными
			D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// накопление большей выносливости
			D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья
		}
	}

	// мы друг ему и мы его покалечили - странно, где это вообщеработает 
	if (Add(D, EMyrLogicEvent::MeAgent_PlannedHurtByFriend))
	{	D->EmotionDeltaForMe = FColor(100, 1, 100, 3);			// ненависть к себе но в цедлом отчуждение
		D->EmotionDeltaForGoal = FColor(255, 255, 255, 1);		// безумие смешанных чувств к цели
		D->TimesToUseAfterCatch = 15;							// мощно вдаряет, но мало действует
		D->ExperienceIncrements.Add(EPhene::Charm, -3);		// предательство
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// накопление большей выносливости
		D->ExperienceIncrements.Add(EPhene::Strength, 1);	// сила кажется что расход
		D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья

		if (Add(D, EMyrLogicEvent::MeAgent_FirstPlannedHurtByFriend))
		{	D->EmotionDeltaForMe = FColor(100, 1, 100, 5);			// ненависть к себе но в цедлом отчуждение
			D->EmotionDeltaForGoal = FColor(255, 255, 255, 3);		// безумие смешанных чувств к цели
			D->TimesToUseAfterCatch = 95;							// первый раз и необычное событие долго
			D->ExperienceIncrements.Add(EPhene::Charm, -4);		// предательство
			D->ExperienceIncrements.Add(EPhene::Care, -1);		// утрата собранности и ума
			D->ExperienceIncrements.Add(EPhene::Stamina, 2);	// накопление большей выносливости
			D->ExperienceIncrements.Add(EPhene::Strength, 1);	// сила кажется что расход
			D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// объект намеренно нас покалечил, и он здоровый и страшный
	if (Add(D, EMyrLogicEvent::MePatient_PlannedHurtByBigFearful))
	{	D->EmotionDeltaForMe = FColor(0, 100, 255, 1);			// страх и жалость к себе
		D->EmotionDeltaForGoal = FColor(0, 30, 255, 3);			// страх и чуть восхищения к объекту, сильно растет
		D->TimesToUseAfterCatch = 25;							// очень надолго запоминается
		D->ExperienceIncrements.Add(EPhene::Care, -1);		// от психотравмы становимся менее собранными
	}

	// мы намеренно покалечили объект, и мы для него оргромны
	if (Add(D, EMyrLogicEvent::MeAgent_PlannedHurtByBigFearful))
	{	D->EmotionDeltaForMe = FColor(60, 100, 50, 1);			// смутные чувства к себе
		D->EmotionDeltaForGoal = FColor(100, 30, 10, 1);			// досада на мелочь, но слабая
		D->TimesToUseAfterCatch = 13;							// мелочь
		D->ExperienceIncrements.Add(EPhene::Care, -1);		// менее собранными
		D->ExperienceIncrements.Add(EPhene::Charm, -1);		// обижать мелких некрасиво
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// объект намеренно нас покалечил, и он мелкий и убогий
	if (Add(D, EMyrLogicEvent::MePatient_PlannedHurtByLittlePoor))
	{	D->EmotionDeltaForMe = FColor(255, 255, 0, 1);			// саможалость к себе
		D->EmotionDeltaForGoal = FColor(255, 0, 0, 1);			// чистая ненависть к врагу
		D->TimesToUseAfterCatch = 25;								// милюзга надолго не занимает
		D->ExperienceIncrements.Add(EPhene::Care, 1);		// он нас мотивирует
		D->ExperienceIncrements.Add(EPhene::Stamina, -1);	// выносливость падает, так как мелочь морально изматывает
	}
	// мы намеренно покалечили того, для кого мы мелкие, а он для нас большой
	if (Add(D, EMyrLogicEvent::MeAgent_PlannedHurtByLittlePoor))
	{	D->EmotionDeltaForMe = FColor(0, 255, 100, 3);			// радость и гордость, но тревога, что можно огрести
		D->EmotionDeltaForGoal = FColor(255, 100, 150, 2);		// ненависть с восхищением и страхом, сильная эмоция
		D->TimesToUseAfterCatch = 30;							// серьёзный прорыв надолго запоминается
		D->ExperienceIncrements.Add(EPhene::Care, 1);		// он нас мотивирует на собранность
		D->ExperienceIncrements.Add(EPhene::Strength, 2);	// силовое достижение
		D->ExperienceIncrements.Add(EPhene::Agility, 1);	// он нас мотивирует
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// он нас мотивирует
		D->ExperienceIncrements.Add(EPhene::Charm, 1);		// он нас мотивирует
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// объект намеренно наносит нам фатальные повреждения, зная, что наше здоровье на исходе
	if (Add(D, EMyrLogicEvent::MePatient_PlannedHurtTillMyDeath))
	{	D->EmotionDeltaForMe = FColor(20, 20, 255, 1);			// страх с нотками саможалости и гнева
		D->EmotionDeltaForGoal = FColor(255, 0, 255, 1);			// чистая ненависть к врагу
		D->TimesToUseAfterCatch = 5;								// милюзга надолго не занимает
		D->ExperienceIncrements.Add(EPhene::Care, 1);		// осознанность растёт от осознания бытия на волоске
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// выносливость растёт от осознания бытия на волоске
		D->ExperienceIncrements.Add(EPhene::Strength, 1);	// сила растёт от осознания бытия на волоске
	}
	//мы намеренно наносим смертельный удар
	if (Add(D, EMyrLogicEvent::MeAgent_PlannedHurtTillMyDeath))
	{	D->EmotionDeltaForMe = FColor(150, 200, 0, 1);			// робкий переход счастью и радости
		D->EmotionDeltaForGoal = FColor(30, 20, 10, 1);			// робкий переход к полной отчужденности
		D->TimesToUseAfterCatch = 10;							// средне
		D->ExperienceIncrements.Add(EPhene::Care, 2);		// осознанность растёт от результативности
		D->ExperienceIncrements.Add(EPhene::Strength, 1);	// сила
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//*агент нас атаковал, но мы хотя бы попытались парировать
	if (Add(D, EMyrLogicEvent::MePatient_HurtParried))
	{	D->EmotionDeltaForMe = FColor(40, 150, 50, 1);			// успокоение и самолюбие
		D->EmotionDeltaForGoal = FColor(125, 10, 30, 2);			// ненависть, но с успокоением
		D->TimesToUseAfterCatch = 60;							// удача ненадолго, но запоминается
		D->ExperienceIncrements.Add(EPhene::Agility, 2);	// удачный уверт серьезно повышает ловкость
		D->ExperienceIncrements.Add(EPhene::Strength, 2);	// сила растёт - результативная силовая контратака
	}

	//мы атаковали, а он отбил нашу атаку своей контратакой
	if (Add(D, EMyrLogicEvent::MeAgent_HurtParried))
	{	D->EmotionDeltaForMe = FColor(150, 40, 50, 1);			// злоба и самоуничижение
		D->EmotionDeltaForGoal = FColor(195, 40, 150, 2);		// чистая ненависть с тревогой и уважением
		D->TimesToUseAfterCatch = 40;							// неудача ненадолго, но запоминается
		D->ExperienceIncrements.Add(EPhene::Care, -1);		// осознание помутняется
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// выносилвость растёт просто от затраты сил
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//этот объект сделал для нас намеренное благодеяние
	if (Add(D, EMyrLogicEvent::MePatient_PlannedGrace))
	{	D->EmotionDeltaForMe = FColor(10, 200, 50, 2);			// удовольствие с сомнениями
		D->EmotionDeltaForGoal = FColor(0, 255, 30, 1);			// любовь, очень медленно ибо сомнительно
		D->TimesToUseAfterCatch = 70;							// благодарность долго не забывается
		D->ExperienceIncrements.Add(EPhene::Care, 2);		// собранность усиливается
		D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья
	}
	//мы намеренно облгодеяли этого объекта
	if (Add(D, EMyrLogicEvent::MeAgent_PlannedGrace))
	{	D->EmotionDeltaForMe = FColor(50, 255, 50, 1);			// гордость с подозрениями
		D->EmotionDeltaForGoal = FColor(10, 200, 30, 3);			// охлаждение любви, трата энергии
		D->TimesToUseAfterCatch = 40;							// благодарность долго не забывается
		D->ExperienceIncrements.Add(EPhene::Charm, 3);		// логично, хорошее отношение повышается
		D->ExperienceIncrements.Add(EPhene::Care, 1);		// осознанность от намеренного добра 
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//этот объект сделал для нас случайное благодеяние
	if (Add(D, EMyrLogicEvent::MePatient_CasualGrace))
	{	D->EmotionDeltaForMe = FColor(10, 255, 200, 1);			// удовольствие c сильным страхом от нелогичности
		D->EmotionDeltaForGoal = FColor(0, 200, 130, 3);			// любовь с сильными опасениями
		D->TimesToUseAfterCatch = 30;							// странная благодарность долго сидит
		D->ExperienceIncrements.Add(EPhene::Strength, 1);	// чудесное придание сил
		D->MultEmotionByInputScalar = true;						// глубина переживания зависит от уровня увечья
	}
	//мы случайно облгодеяли этого объекта, промахнулись
	if (Add(D, EMyrLogicEvent::MeAgent_CasualGrace))
	{	D->EmotionDeltaForMe = FColor(150, 180, 10, 2);			// досада на себя, что растрачиваем, но всё же хороше настроение
		D->EmotionDeltaForGoal = FColor(190, 100, 30, 1);		// злоба, что выкрал чужое добро, но слабо растущая
		D->TimesToUseAfterCatch = 5;								// побыстрее забыть конфуз
		D->ExperienceIncrements.Add(EPhene::Charm, 3);		// логично
	}



	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//*просто прыжок сам с собой
	if (Add(D, EMyrLogicEvent::SelfJump))
	{	D->EmotionDeltaForMe = FColor(0, 200, 0, 1);				// успокоение и самогордость, и вообще это весело
		D->EmotionDeltaForGoal = FColor(0, 0, 0, 1);				// цели нет, а та что есть вызывает медитативное отстранение
		D->TimesToUseAfterCatch = 10;							// мало, чтобы не запрыгать себе всё прочее
		D->MultEmotionByInputScalar = true;
		D->ExperienceIncrements.Add(EPhene::Strength, 1);	// силовое упражнение
		D->ExperienceIncrements.Add(EPhene::Stamina, 1);	// выносливость
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//*просто прыжок сам с собой
	if (Add(D, EMyrLogicEvent::ObjEat))
	{	D->EmotionDeltaForMe = FColor(0, 250, 0, 1);				// успокоение и самогордость, и вообще это весело
		D->EmotionDeltaForGoal = FColor(0, 0, 0, 1);				// цели нет, а та что есть вызывает медитативное отстранение
		D->TimesToUseAfterCatch = 10;							// мало, чтобы не запрыгать себе всё прочее
		D->MultEmotionByInputScalar = true;
		D->ExperienceIncrements.Add(EPhene::Strength, 1);	// прибавка сил
		D->ExperienceIncrements.Add(EPhene::Stamina, -1);	// минус выносливость, ибо толстеем
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//*скукота
	if (Add(D, EMyrLogicEvent::SelfStableAndBoring))
	{
		D->EmotionDeltaForMe = FColor(0, 0, 0, 1);				// успокоение 
		D->EmotionDeltaForGoal = FColor(10, 30, 20, 1);			// немного обиды на возможную цель
		D->TimesToUseAfterCatch = 255;							// максимально, чтобы часто не заспамливать себя
		D->MultEmotionByInputScalar = true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//вход в место успокоения
	if (Add(D, EMyrLogicEvent::ObjEnterQuietPlace))
	{
		D->EmotionDeltaForMe = FColor(0, 0, 0, 4);				// успокоение 
		D->EmotionDeltaForGoal = FColor(0, 0, 30, 2);			// немного тревожности вовне
		D->TimesToUseAfterCatch = 255;							// максимально, чтобы часто не заспамливать себя
		D->MultEmotionByInputScalar = true;
	}

}

#define ADD(C,V,S,D) Add(EEmoCause::##C, EEmotio::##V, S, D)

UMyrEmoReactionList::UMyrEmoReactionList()
{
	ADD ( VoiceOfRatio,		Peace,			2,		255);		//движение к спокойному бодрствованию
	ADD ( Burnout,			Void,			2,		120);		//движение к полному эмоциональному выгоранию
	ADD ( Pain,				Insanity,		10,		120);		//реакция на боль - сумасшествие
	ADD ( MeJumped,			Pride,			2,		30);		//ПРЫГНУЛИ И НА ДУШЕ ХОРОШО, но не надолго
}
