// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Myrra.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "MyrBioStatUserWidget.generated.h"

UCLASS() class MYRRA_API UMyrBPMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	//перевод однобитного дробного с фиксированной запятой в обычный флоат
	UFUNCTION(BlueprintPure) static float RatioToF(FRatio R) { return R; }

	//перевод однобитного биполярного дробного с фиксированной запятой в обычный флоат
	UFUNCTION(BlueprintPure) static float BipolarToF(FBipolar R) { return R; }

	//рандом для эмоций и ИИ
	UFUNCTION(BlueprintPure) static float AIRand() { return FUrGestalt::RandVar / RAND_MAX; }


	//структура эмоции в обычный цвет
	UFUNCTION(BlueprintPure) static FColor PathiaToColor(FPathia R) { return FColor(R.Rage, R.Love, R.Fear); }

	//выдать ближайшую эмоцию словесную для интерфейса
	UFUNCTION(BlueprintPure) static float EmotionToMnemo(const FPathia In, EPathia& Out1, EPathia& Out2) { return In.GetFullArchetype(Out1, Out2); }

	//структура рефлекс (биты воздействий + эмоция) в цвет (переводится только эмоция)
	UFUNCTION(BlueprintPure) static FLinearColor EmoReflexToColor(const FReflex& In) { return (FLinearColor)In.Emotion; }

	//биты воздействий в текстовую строку, переводятся взведенные биты воздействий в виде имен через запятую
	UFUNCTION(BlueprintPure) static void InfluencesToMnemo(const FInflu& In, FText& Out);
	UFUNCTION(BlueprintPure) static void EmoStimulusToMnemo(const FReflex& In, FText& Out) { InfluencesToMnemo(In.Condition, Out); };

	//структура ррефлекс (биты воздействий + эмоция) проверяется бит указывающий на латентность (фазу накопления опыта)
	UFUNCTION(BlueprintPure) static bool IsLatent(FReflex R) { return R.IsLatent(); }

	//перевод эмоции в две строки текста ближайших опорных эмоций-мнемоник
	UFUNCTION(BlueprintPure) static float EmotionToMnemoText(const FPathia In, FText& Out1, FText& Out2)
	{	EPathia O1; EPathia O2;
		float Wei = In.GetFullArchetype(O1, O2);
		Out1 = UEnum::GetDisplayValueAsText(O1);
		Out2 = UEnum::GetDisplayValueAsText(O2);
		return Wei;
	}
};

//###################################################################################################################
//базовый класс для разных элементов интерфейса, связанных с существом
//###################################################################################################################
UCLASS() class MYRRA_API UMyrBioStatUserWidget : public UUserWidget
{
	GENERATED_BODY()

//---------------------------------------------------------------------
public:	// общие характеристики / кэши вычисляемых параметров
//---------------------------------------------------------------------

	//единый указатель на объект источник данных - остальное по идее берется в обработчиках
	//хотя переменные для здоровья, усталости и эмоций (будут в релизе) пусть таки останутся
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class AMyrPhyCreature* MyrOwner;

	//единый указатель на объект источник данных - остальное по идее берется в обработчиках
	//хотя переменные для здоровья, усталости и эмоций (будут в релизе) пусть таки останутся
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class AMyrAIController* MyrAIC;

	//если это существо управляется игроком, а виджет - худ , важно сразу же подцепить нужный контроллер в нужном типе
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	class AMyrPlayerController* MyrPlayerController;

//---------------------------------------------------------------------
public:	// функции
//---------------------------------------------------------------------


	//для блюпринта меню, сделанного на этом классе, обработчика нажатия клавиш, чтобы выходить по нажатию
	UFUNCTION(BlueprintCallable) bool KeyToEscape(FKey KeyHappened, FName ActionMap, bool EscapeToo = true);

	//вывести для худа называние клавиш, которыми делается следующее действие (или перенести в виджет)
	UFUNCTION(BlueprintCallable) bool GetKeysForCreatureAction(EAction Action, FString& OutString) const;

	//для блюпринта меню, сделанного на этом классе, хз
	UFUNCTION(BlueprintCallable) void SetProgressBarFillType(class UProgressBar* PB, TEnumAsByte<EProgressBarFillType::Type> Newft);

	//вызов божественных сущностей
	UFUNCTION(BlueprintCallable) class UMyrraGameInstance* GetMyrraGameInstance() const { return (UMyrraGameInstance*)GetGameInstance(); }
	UFUNCTION(BlueprintCallable) class AMyrraGameModeBase* GetMyrraGameMode() const { return (AMyrraGameModeBase*)GetWorld()->GetAuthGameMode(); }
	UFUNCTION(BlueprintCallable) class AMyrPlayerController* GetMyrPlayerController() const { return (AMyrPlayerController*)GetOwningPlayer<APlayerController>(); }

	UFUNCTION(BlueprintImplementableEvent)	void OnSetOwnerCreature(AMyrPhyCreature* Creature);
	UFUNCTION(BlueprintCallable)	void SetOwnerCreature(AMyrPhyCreature* Creature) { MyrOwner = Creature; OnSetOwnerCreature(Creature); }

	//// показан
	UFUNCTION(BlueprintImplementableEvent)	void OnJustShowed();

	//эти функции имплементируются в блюпринте как обработчики событий, по ним на экране появляются всякие рюшки
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnSaved"))	void OnSaved();				// когда только произошло сохранение (вывести надпись)
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnDeath"))	void OnDeath();				// когда связанное с этим виджетом существо умерло (эффектно скрыть)
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnEat"))		void OnEat(AActor* Victim);	// когда обладатель виджета съел кого-то
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnGrab"))		void OnGrab(AActor* Victim);// когда обладатель виджета схватил кого-то
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnUnGrab"))	void OnUnGrab(AActor* Victim);// когда обладатель виджета отпустил кого-то
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnExpGain"))	void OnExpGain(EPhene OnWhat, ELevelShift Verdict);// когда выклюёвывается новое очко опыта

	UFUNCTION(BlueprintImplementableEvent)	void OnQuestChanged(class UMyrQuestProgress* Quest);// когда берется, изменяется или кончается квест

	//параметр задает откуда брать, из какого списка
	UFUNCTION(BlueprintImplementableEvent)	void OnExpressionStart(bool Relax);// начало самовыражения - вывести меню
	UFUNCTION(BlueprintImplementableEvent)	void OnExpressionRelease(bool Relax);// начало самовыражения - вывести меню
	UFUNCTION(BlueprintImplementableEvent)	void OnExpressionSelect(bool Relax, int i);// начало самовыражения - вывести меню

	//новый универсальный вариант
	UFUNCTION(BlueprintImplementableEvent)	void OnTriggerNotify(EWhyTrigger ExactReaction, AActor* Obj, bool Can);

	// события старта действий, чтоб отображать названия и не делать это в тике
	UFUNCTION(BlueprintImplementableEvent)	void OnAction(int WhatKind, bool Start);

	// отобразить инфу по актору в фокусе камеры
	UFUNCTION(BlueprintImplementableEvent)	void SetGoalActor(AActor* GoAc);


	UFUNCTION(BlueprintCallable) FLinearColor EmoToColor(FPathia E) const { return (FLinearColor)E; }



};


//###################################################################################################################
// отдельный атрибут для прокачки и отображения. УОбъект - для уобства работы с виджетом списком
//массив этих уобъектов заводится в Game Instance так как нужен только для отображения
//###################################################################################################################
UCLASS(Blueprintable, BlueprintType, hidecategories = (Object), meta = (BlueprintSpawnableComponent))
class URoleParameter : public UObject
{
	GENERATED_BODY()

public:

	//человеческое имя для атрибута и пояснения (вводится отдельно, потому что нужно только для отображения)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Name;

	//пояснение - внизу или всплывающее, как получится (вводится отдельно, потому что нужно только для отображения)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Explanation;

	//индекс в массиве Р структуры FRoleParams, которая должна находиться в рассматриваемом существе
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EPhene IndexOfParam;

public:

	//получить существо, параметры которого тут исследуются
	UFUNCTION(BlueprintCallable) class AMyrPhyCreature* GetPlayer() const;

	//чтобы инициализировать в блюпринте одним квадратиком
	UFUNCTION(BlueprintCallable) void Init(FText iName, FText iExplanation, EPhene iIndexOfParam) { Name = iName; Explanation = iExplanation; IndexOfParam = iIndexOfParam; }

	//для блюпринта, выдать все данные по параметру, чтобы нарисовать их
	UFUNCTION(BlueprintCallable) void GetCurLevelStatistics(float& WholeProgress, uint8& CurLevel, float& PreogressFromLevel, int32 &XPFromLevel, ELevelShift& Result) const;

	//по нажатию кнопки - установить целое число уровней, которое "просится", если есть заимствование, то тут же опустошить и заимствоваанный
	UFUNCTION(BlueprintCallable) void UpGradeLevel();

	//установить источником для позитивного влиания при апгрейде уровня
	UFUNCTION(BlueprintCallable) bool MakeAsDonor();
};