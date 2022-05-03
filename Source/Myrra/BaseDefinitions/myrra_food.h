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
	Sleepiness,
	Pain,
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Reserve = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RateOfConsume = 0.0f;

	//другое применение переменной скорости - резерв до того, как еду надкусили
	float& MaxReserve() { return RateOfConsume; }

	//еда еще полная, не надкушенная
	bool StillFull() const { return FMath::Abs(RateOfConsume) < FMath::Abs(Reserve); }

	//пометить, что еда теперь надкушенная
	void MarkAsBitten() { MaxReserve() = Reserve; }

	//еда пустая, вызщывать только для надкушенной - изначальный резерв и текущий имеют разные знаки, значит резерв ушел в "минус"
	bool Empty() { return (MaxReserve() * Reserve <= 0 );	}


	//передать из источика по тому же каналу новую порцию эффекта, тру = источник исчерпан, надо его удалить
	bool Transfer(FDigestivePotion& Other, float NewPart)
	{
		//внимание, здесь через жопу - когда запас полон, в переменной RateOfConsume хранится собственно расход в секунду
		//и он в начале присваивается желудку; так что скорость всегда должна быть меньше по модулю, чем суммарный запас
		//но после первого куся в эту переменную вносится первое, максимальное значение резерва, и тогда эта переменная будет
		//всегда больше, чем сам резерв (по модулю) - этим можно различать полный и початый кусок еды
		if (Other.StillFull())
		{
			//скорость поглощения наша появляется лишь при первом кусе этой еды
			RateOfConsume = FMath::Max(RateOfConsume, Other.RateOfConsume);

			//пометить как надкушенную, теперь можно проверять на опустошенность
			Other.MarkAsBitten();

			//пока не было кусей, резерв еды максимальный, и можно брать долю непосредственно от него
			//берем минимальное на случай если пользователь ввел долю больше единицы
			Reserve += FMath::Min(Other.Reserve, Other.Reserve * NewPart);
			Other.Reserve -= Other.Reserve * NewPart;

			//если первая доля больше или равна единице, то еда уже тут будет опустошена
			return Other.Empty();
		}
		//от этого запаса уже успели откусить какую-то часть
		else
		{
			//раз так, то он уже может быть пустым, заслуживает удаления
			if (Other.Empty()) return true;

			//наш резерв пополняется на постоянную долю, зависящую от изначальной полноты этой еды
			//однако такой доли может уже не остаться, в этом случае загребётся лишь то, что осталось
			Reserve += FMath::Min(Other.MaxReserve() * NewPart, Other.Reserve);

			//на этот же кусок уменьшается резерв еды, может зайти в минус
			Other.Reserve -= Other.MaxReserve() * NewPart;
			return Other.Empty();
		}
	}

	//потратить часть данного эффекта на реальную характеристику (определенную заранее извне)
	bool SpendToParam(float* Param, float DeltaTime)
	{
		//когда нет нужной характеристики, тратится в пустую
		if (Param)
		{
			float Quant = FMath::Sign(Reserve) * RateOfConsume * DeltaTime;
			*Param += Quant;
			if (*Param > 1) *Param = 1;
			else if (*Param < 0) *Param = 0;
			Reserve -= Quant;
			return (Reserve * Quant < 0);
		}

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
				Other->Effects.RemoveAt(i);
		return Empty();
	}
};