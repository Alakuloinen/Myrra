#pragma once
#include "CoreMinimal.h"
#include "myrra_food.generated.h"


//еда - возможно, перенести в отдельный файл

//###################################################################################################################
// канал влияния пищи на характеристики, тут их будет больше
//###################################################################################################################
UENUM(BlueprintType) enum class EDigestiveTarget : uint8
{
	Health,
	Stamina,
	Energy,
	Psychedelic,
	NONE
};

//###################################################################################################################
// запись одного эффекта приема пищи, может передаваться от еде к едоку и тратиться едоком на параметры
//###################################################################################################################
USTRUCT(BlueprintType) struct FDigestivePotion
{
	GENERATED_BODY()

		//канал эффекта, определяет какой параметр увеличивать или уменьшать
		UPROPERTY(EditAnywhere, BlueprintReadWrite) EDigestiveTarget Target;

	//длительность воздействия в секундах, примерно, ибо коэффициент деления кванта времени
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Seconds = 1;

	//запас эффекта, если длительность 1, то удельная прибыль/убыль в секунду
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Reserve = 0;

	//передать из источика по тому же каналу новую порцию эффекта, тру = источник исчерпан, надо его удалить
	bool Transfer(FDigestivePotion& Other, float Part)
	{
		Reserve += Other.Reserve * Part;
		Seconds += Other.Seconds * Part;
		Other.Reserve *= 1 - Part;
		Other.Seconds *= 1 - Part;
		return (Other.Reserve <= 0.001);
	}

	//потратить часть данного эффекта на реальную характеристику (определенную заранее извне)
	bool SpendToParam(float* Param, float DeltaTime)
	{
		//знак резерва отражает прибыль/убыль, обратный знак всегда сокращает резерв
		float Q = Reserve * DeltaTime / (float)Seconds;
		Reserve -= Q;

		//перебарщивание эффектом ведёт к отлрицательному эффекту - возможно, лишнее
		//когда нет нужной характеристики, тратится в пустую
		if (Param)
		{
			*Param += Q;
			if (*Param > 1) *Param -= Q;
			else if (*Param < 0) *Param += Q;
		}

		//знак кванта равен знаку резерва, но если знаки различаются, перейден ноль
		if (Reserve * Q <= 0) { Reserve = 0; return true; }
		else return false;
	}
};

//###################################################################################################################
//контейнер эффектов приема пищи, универсальный, хранится как в еде, так и в желудке
//###################################################################################################################
USTRUCT(BlueprintType) struct FDigestiveEffects
{
	GENERATED_BODY()

		//пока массив, хоть он и часто меняется, но тут немного элементов
		UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FDigestivePotion> Effects;

	//весь запас эффектов исчерпан
	bool Empty() const { return (Effects.Num() == 0); }

	//передать из внешнего источника сюда один канал эффектов, тру = исчерпан, в источнике надо удалить
	bool TransferSingle(FDigestivePotion& Src, float Part)
	{
		FDigestivePotion* Found = nullptr;
		for (auto& D : Effects)	if (D.Target == Src.Target) Found = &D;
		if (!Found) {
			Found = &Effects.AddDefaulted_GetRef();
			Found->Target = Src.Target;
		}
		return Found->Transfer(Src, Part);
	}

	//передать всё (или заданную часть) содержимое из источника в текущий котейнер
	bool Transfer(FDigestiveEffects* Other, float Part)
	{
		//с конца удаляем опустошенных
		for (int i = Other->Effects.Num() - 1; i >= 0; i--)
			if (TransferSingle(Other->Effects[i], Part))
				Effects.RemoveAt(i);
		return Empty();
	}
};