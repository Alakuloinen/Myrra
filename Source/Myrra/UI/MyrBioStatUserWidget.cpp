// Fill out your copyright notice in the Description page of Project Settings.

#include "MyrBioStatUserWidget.h"
#include "../Control/MyrPlayerController.h"			// контроллр игрока
#include "../Control/MyrDaemon.h"					// пешка-дух для управления существами, регистрируется как протагонист в Game Mode
#include "../Creature/MyrPhyCreature.h"				// существо, 
#include "../MyrraGameModeBase.h"					// базис загруженного уровня, нужен, чтобы откорчевать оттуда протагониста, 
#include "../MyrraGameInstance.h"					// ядро игры 
#include "../AssetStructures/MyrCreatureGenePool.h"	// генофонд существа, для доступа к диапазонам ролевых параметров
#include "GameFramework/InputSettings.h"
#include "Components/Button.h"


//==============================================================================================================
//для блюпринта, обработчика нажатия клавиш, чтобы выходить из меню по нажатию
//==============================================================================================================
bool UMyrBioStatUserWidget::KeyToEscape(FKey KeyHappened, FName ActionMap, bool EscapeToo)
{
	TArray<FInputActionKeyMapping> OutMappings;
	auto InputSettings = UInputSettings::GetInputSettings();
	InputSettings->GetActionMappingByName(ActionMap, OutMappings);
	for (auto KeyM : OutMappings)
	{
		if (KeyM.Key == KeyHappened)
			return true;
		if (EscapeToo && KeyM.Key == EKeys::Escape)
			return true;
	}
	return false;
}

//==============================================================================================================
//вывести для худа называние клавиш, которыми делается следующее действие (или перенести в виджет)
//==============================================================================================================
UFUNCTION(BlueprintCallable) bool UMyrBioStatUserWidget::GetKeysForCreatureAction(EAction Action, FString& OutString) const
{
	OutString = TEXT("");
	if(MyrOwner)
		if (MyrOwner->Daemon)
		{
			//отсыскать как та или иная кнопка называна в Input Settings - бардак, унифицировать бы
			FName ActionMapping;
			switch (MyrOwner->Daemon->GetButtonUsedForThisAction(Action))
			{	case PCK_ALT:		ActionMapping = TEXT("Evade");		break;
				case PCK_LBUTTON:	ActionMapping = TEXT("Attack_L");	break;
				case PCK_MBUTTON:	ActionMapping = TEXT("Attack_M");	break;
				case PCK_PARRY:		ActionMapping = TEXT("Counter");	break;
				case PCK_RBUTTON:	ActionMapping = TEXT("Attack_R");	break;
				case PCK_SPACE:		ActionMapping = TEXT("Jump");		break;
				case PCK_USE:		ActionMapping = TEXT("Pick");		break;
			}

			//где искать назначения клавиш
			auto InputSettings = UInputSettings::GetInputSettings();
			if (!ActionMapping.IsNone())
			{

				//клавиш может быть несколько альтернатив
				TArray<FInputActionKeyMapping> OutMappings;
				InputSettings->GetActionMappingByName(ActionMapping, OutMappings);
				if (OutMappings.Num() > 0)
				{	for (auto K : OutMappings)
					{
						//перебор модификаторов
						if (K.bAlt) OutString+=EKeys::LeftAlt.ToString() + TEXT("+");
						if (K.bCmd) OutString += TEXT("Cmd+");
						if (K.bCtrl) OutString += EKeys::LeftControl.ToString() + TEXT("+");
						if (K.bShift) OutString += EKeys::LeftShift.ToString() + TEXT("+");
						OutString += K.Key.ToString();

						//здесь надо переводимую строку "или", но пока лень
						OutString += TEXT(".");
					}
				}
			}
		}
	return false;
}

//==============================================================================================================
//для блюпринта меню, сделанного на этом классе, хз
//==============================================================================================================
void UMyrBioStatUserWidget::SetProgressBarFillType(UProgressBar* PB, TEnumAsByte<EProgressBarFillType::Type> Newft)
{ 
	PB->BarFillType = Newft;
	if( Newft==EProgressBarFillType::RightToLeft) PB->SetRenderTransformAngle(180); else
		PB->SetRenderTransformAngle(0);
}


//==============================================================================================================
//получить существо, параметры которого тут исследуются
//==============================================================================================================
AMyrPhyCreature* URoleParameter::GetPlayer() const
{
	AMyrraGameModeBase* GameMode = Cast<AMyrraGameModeBase>(GetWorld()->GetAuthGameMode());
	if (!GameMode) return nullptr;
	AMyrPhyCreature* Me = GameMode->Protagonist->GetOwnedCreature();
	return Me;
}


//==============================================================================================================
//для блюпринта, выдать все данные по параметру, чтобы нарисовать их
//==============================================================================================================
UFUNCTION(BlueprintCallable) void URoleParameter::GetCurLevelStatistics(float& WholeProgress, uint8& CurLevel, float& PreogressFromLevel, int32& XPFromLevel, ELevelShift& Result) const
{
	if(auto Me = GetPlayer())
	{	FPhene& Ph = Me->Phenotype.ByEnum(IndexOfParam);
		WholeProgress		= Ph.R_WholeXP();
		CurLevel			= Ph.Level();
		Result				= Ph.GetStatus(); 
		PreogressFromLevel	= Ph.R_Progress();
		XPFromLevel			= Ph.Progress();
	}
}

//==============================================================================================================
//по нажатию кнопки - установить целое число уровней, которое "просится", если есть заимствование, то тут же опустошить и заимствоваанный
//==============================================================================================================
void URoleParameter::UpGradeLevel()
{
	if(auto Me = GetPlayer())
	{	FPhene& Ph = Me->Phenotype.ByEnum(IndexOfParam);
		auto& IndexOfDonor = GetWorld()->GetGameInstance<UMyrraGameInstance>()->DonorRoleAxis;
		if (IndexOfDonor != EPhene::NUM)
		{	Ph.Uplevel(Me->Phenotype.ByEnum(IndexOfDonor).Donate());
			IndexOfDonor = EPhene::NUM;
		}
		else Ph.Uplevel();
	}
}

//==============================================================================================================
//установить источником для позитивного влиания при апгрейде уровня
//==============================================================================================================
bool URoleParameter::MakeAsDonor()
{	auto& Buf = GetWorld()->GetGameInstance<UMyrraGameInstance>()->DonorRoleAxis;
	if (Buf == IndexOfParam) Buf = EPhene::NUM;
	else Buf = IndexOfParam;
	return (Buf == IndexOfParam);
}

//==============================================================================================================
// мнемоническая строка список воздействий по их битам
//==============================================================================================================
#define REVEAL(Type,Val)	St = In.ToStr(Val); \
							if(St!=TEXT("None") && St!=TEXT("__") && St!=TEXT("All")) InflNames.Add(FText::FromString(St));	\
							else for(uint32 Nu = 1; Nu<255; Nu *= 2)	 \
								if (In.Has((Type)Nu))				\
								{	FString St2 = In.ToStr((Type)Nu);	St = St + TEXT(", ") + St2; }

void UMyrBPMath::InfluencesToMnemo(const FInflu& In, FText& Out)
{
	auto W = GEngine->GetCurrentPlayWorld();					if (!W) return;
	auto GI = Cast<UMyrraGameInstance>(W->GetGameInstance());	if (!GI) return;
	TArray<FText> InflNames;
	FString St;
	REVEAL(EInfluWhat, In.What)
	REVEAL(EInfluWhere, In.Where)
	REVEAL(EInfluHow, In.How)
	REVEAL(EInfluWhy, In.Why)
	Out = FText::Join(FText::FromString(TEXT(", ")), InflNames);
}
