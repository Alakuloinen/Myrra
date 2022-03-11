#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"
#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "DetailWidgetRow.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Runtime/AppFramework/Public/Widgets/Colors/SColorPicker.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h" //для SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

TSharedRef<IPropertyTypeCustomization> FMyrDynModelTypeCustomization::MakeInstance()
{	return MakeShareable(new FMyrDynModelTypeCustomization);}


//==============================================================================================================
// инициализировать то, что будет рисоваться в строке заголовка свойства-структуры
//==============================================================================================================
void FMyrDynModelTypeCustomization::CustomizeHeader(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//получить рычаги влияния на те свойства, которые мы хотим видеть прямо в строке заголовка
	TSharedPtr<IPropertyHandle> HandleAnimRate = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, AnimRate));
	TSharedPtr<IPropertyHandle> HandleHealthAdd = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, HealthAdd));
	TSharedPtr<IPropertyHandle> HandleStaminaAdd = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, StaminaAdd));
	TSharedPtr<IPropertyHandle> HandleMotionGain = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, MotionGain));
	TSharedPtr<IPropertyHandle> HandleSound = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Sound));

	//для заголовка таким вот тупым образом распознать цифры и к ним добавить эквиваленты этих цифр, как если бы они были константами фаз атаки
	FString FullHead = StructPropertyHandle->GetPropertyDisplayName().ToString();
	if (FullHead == TEXT("0")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::ASCEND); }
	if (FullHead == TEXT("1")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::READY); }
	if (FullHead == TEXT("2")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::RUSH); }
	if (FullHead == TEXT("3")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::STRIKE); }
	if (FullHead == TEXT("4")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::FINISH); }
	if (FullHead == TEXT("5")) { FullHead += TEXT(", in attacks ");  FullHead += TXTENUM(EActionPhase, EActionPhase::DESCEND); }

	//столбец заголовка (левый)
	HeaderRow.NameContent()
	[
		//прямо в левой половине заводим контейнер из нескольких ячеек
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(20)
		[
			//вместо стандартного виджета создать текст, взяв строку из имени свойства - чтобы текст можно было покрасить цветом
			//в данном случае это просто номер элемента в массиве
			SNew(STextBlock).Text(FText::FromString(FullHead))
			.ColorAndOpacity(FLinearColor(1, 0, 1, 1))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		]
		//последние ячейка - под название и виджет ассета звука без громоздкого ярлыка (надо как-то добавить фильтр)
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)
		[	HandleSound->CreatePropertyNameWidget()	]
		+ SHorizontalBox::Slot().FillWidth(10)
		[	SNew(SObjectPropertyEntryBox).PropertyHandle(HandleSound).DisplayThumbnail(false) ]
	]

	//столбец значений (правый) - здесь ничего не будет, чтобы визуально не отвлекал от содержимого строк внутри
	.ValueContent()
	.MinDesiredWidth(1200.0f)
	.MaxDesiredWidth(1800.0f)
	[
		//в остальные 4 ячейки левой половины заголовка засунуть 4 float свойства, друг за другом
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(10)	[	SNew(SProperty, HandleAnimRate).ShouldDisplayName(true)		]
		+ SHorizontalBox::Slot().FillWidth(10)	[	SNew(SProperty, HandleHealthAdd).ShouldDisplayName(true)	]
		+ SHorizontalBox::Slot().FillWidth(10)	[	SNew(SProperty, HandleStaminaAdd).ShouldDisplayName(true)	]
		+ SHorizontalBox::Slot().FillWidth(10)	[	SNew(SProperty, HandleMotionGain).ShouldDisplayName(true)	]

	];
}

//==============================================================================================================
// инициализировать то, что будет рисоваться в строках ниже заголовка структуры
//==============================================================================================================
void FMyrDynModelTypeCustomization::CustomizeChildren(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//подбираем те свойства, которые мы хотим видеть в таблице ПОД заголовком
	TSharedPtr<IPropertyHandle> HandleThorax = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Thorax));
	TSharedPtr<IPropertyHandle> HandlePelvis = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Pelvis));

	//просто добавляем строки с содержимым по умолчанию для данного типа свойств
	IDetailPropertyRow& PropertyThoraxRow = StructBuilder.AddProperty(HandleThorax.ToSharedRef());
	IDetailPropertyRow& PropertyPelvisRow = StructBuilder.AddProperty(HandlePelvis.ToSharedRef());
}


//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№


TSharedRef<IPropertyTypeCustomization> FMyrGirdleModelTypeCustomization::MakeInstance()
{	return MakeShareable(new FMyrGirdleModelTypeCustomization); }

//==============================================================================================================
// инициализировать то, что будет рисоваться в строке заголовка свойства-структуры
//==============================================================================================================
void FMyrGirdleModelTypeCustomization::CustomizeHeader(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//получить рычаги влияния на те свойства, которые мы хотим видеть прямо в строке заголовка
	TSharedPtr<IPropertyHandle> Handle0 = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HardVertical));
	TSharedPtr<IPropertyHandle> Handle1 = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, FixOnFloor));
	TSharedPtr<IPropertyHandle> Handle2 = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, LegsSwingLock));
	TSharedPtr<IPropertyHandle> Handle3 = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HardCourseAlign));
	TSharedPtr<IPropertyHandle> Handle4 = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HeavyLegs));

	//сохранить родительский хэндл
	MainHandle = StructPropertyHandle;

	//найти объявленное где-то в исходном коде перечисление 
	EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ELDY"), true);

	//столбец заголовка (левый)
	HeaderRow.NameContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(10)
			[
				//текст названия пояса, thprax,pelvis
				SNew(STextBlock)
				.Text(StructPropertyHandle->GetPropertyDisplayName())
				.ColorAndOpacity(FLinearColor(255, 255, 0, 255))
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
			]
			+ SHorizontalBox::Slot().FillWidth(4)
			[
				//чекбокс свойства "Leading"
				SNew(SProperty, StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Leading))).ShouldDisplayName(true)
			]

			//создать поле с текстом
			//.StrikeBrush(&FSlateColorBrush(FLinearColor(0, 255, 0, 255)))
		]

	//столбец значений (правый) - растягиваем до конца окна
	.ValueContent()
	.MinDesiredWidth(1800.0f)
	.MaxDesiredWidth(1800.0f)
	[
		//наверно, можно черту-разделитель ка-то более просто сделать, но поиск не дал результата
		SNew(STextBlock)
		.Text(FText::FromString("================================================================================================================================="))
		.ColorAndOpacity(FLinearColor(255, 0, 255, 255))

	];
}

//==============================================================================================================
// инициализировать то, что будет рисоваться в строках ниже заголовка структуры
//==============================================================================================================
void FMyrGirdleModelTypeCustomization::CustomizeChildren(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//сохранить указатель на это нечто, чтобы с помощью него перериосвывать при изменении значений
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//подбираем те свойства, которые мы хотим видеть в теле таблицы, ПОД заголовком
	HandleLimbs[0] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Center));
	HandleLimbs[1] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Left));
	HandleLimbs[2] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Right));
	HandleLimbs[3] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Spine));
	HandleLimbs[4] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Tail));

	TSharedPtr<IPropertyHandle> HandleF[5];
	HandleF[0] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Crouch));
	HandleF[1] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, LegsSpread));
	HandleF[2] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, ForcesMultiplier));
	HandleF[3] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, FeetBrakeDamping));
	HandleF[4] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, CommonDampingIfNeeded));

	TSharedPtr<IPropertyHandle> HandleB[5];
	HandleB[0] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HardVertical));
	HandleB[1] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HardCourseAlign));
	HandleB[2] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, FixOnFloor));
	HandleB[3] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, LegsSwingLock));
	HandleB[4] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HeavyLegs));

	//раскладка по строкам свойств битов для частей тела
	for (int i = 0; i < 5; i++)
	{
		//значение, имеющееся в свойстве
		int32 Val;	HandleLimbs[i].Get()->GetValue(Val);
		DigestColor[i] = MakeDigestString(Val, Digest[i]);

		//вроде как подвязка обработчика события изменения значения свойства
		HandleLimbs[i].Get()->SetOnPropertyValueChanged (FSimpleDelegate::CreateSP(SharedThis(this), &FMyrGirdleModelTypeCustomization::OnChanged));

		//строка для свойства
		StructBuilder.AddCustomRow(HandleLimbs[i]->GetPropertyDisplayName())

			//левая часть - имя свойства и всё
			.NameContent()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("===")))
				.ColorAndOpacity(FLinearColor(0.5, 0.5, 0.5, 1))
			]
			//правая часть, помимо собственно битов частей тела тут ещё куча всего, так как много места 
			.ValueContent().HAlign(HAlign_Fill)
			[
				//в одной ячейке стандартный виджет свойства, комбобокс с битами, поуже, во второй - строка расшифровки
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().MaxWidth(100)
				[
					HandleLimbs[i]->CreatePropertyNameWidget()
				]
				+ SHorizontalBox::Slot().MaxWidth(20)
				[
					HandleLimbs[i]->CreatePropertyValueWidget()
				]
				//поясняющая строка, максимальной длины, которая должна обновляться
				+ SHorizontalBox::Slot().FillWidth(10)
				[
					SAssignNew(Mnemo[i], STextBlock).Text(FText::FromString(Digest[i])).ColorAndOpacity(DigestColor[i])
				]
				//мелкий столбик - втиснуть свойство не относящееся к битам части тела, но удобно занимающее лишнее место
				+ SHorizontalBox::Slot().MaxWidth(200).FillWidth(2)
				[
					SNew(SProperty, HandleF[i]).ShouldDisplayName(true)
				]
				//два мелких столбика для флагов - разделены по столбцам, чтобы виджет-галочка занимала не больше, чем она сама
				+ SHorizontalBox::Slot()
				.MaxWidth(150)
				.FillWidth(2)
				.HAlign(HAlign_Right)
				[
					HandleB[i]->CreatePropertyNameWidget()
				]
				+ SHorizontalBox::Slot()
					.MaxWidth(20)
				[
					HandleB[i]->CreatePropertyValueWidget()
				]
			];
	}

}

//==============================================================================================================
//превратить набор битов в строку подсказку, что отображается напротив - удобнее вместо всплывающей
//==============================================================================================================
FLinearColor FMyrGirdleModelTypeCustomization::MakeDigestString(int32 Bits, FString& OutStr)
{
	//подразумевается, что указатель на перечисление уже был привязан к реальному перечислителю битов
	if (!Bits)
	{
		OutStr = TEXT("Passive");
		return FLinearColor(0.3, 0.3, 0.3);
	}
	else
	{
		//очистить от прдыдущего
		OutStr = TEXT("");

		//если найден взведеным очередной бит, добавить в строку то, как этот бит назван
		for (int i=0; i<EnumPtr->GetMaxEnumValue(); i++)
			if(Bits & (1<<i))	OutStr += EnumPtr->GetDisplayNameTextByIndex(i).ToString() + " ";

		//гравитация, важный признак, сделать строки сгравитацией визуально более яркими
		if (Bits & LDY_GRAVITY)
		{
			if(Bits == LDY_GRAVITY) FLinearColor(0.4, 0.4, 0.4);
			else return FLinearColor(0.8, 0.8, 0.8);
		}
		else return FLinearColor(0.5, 0.5, 0.5);

	}
	return FLinearColor(0.5, 0.5, 0.5);
}

//==============================================================================================================
//обработчик события
//==============================================================================================================
void FMyrGirdleModelTypeCustomization::OnChanged()
{
	//преобразоват в строку
	for (int i = 0; i < 5; i++)
	{
		if (!HandleLimbs[i].IsValid()) continue;
		if (!Mnemo[i].IsValid()) continue;
		int32 Val;
		HandleLimbs[i].Get()->GetValue(Val);
		DigestColor[i] = MakeDigestString(Val, Digest[i]);
		Mnemo[i]->SetText(FText::FromString(Digest[i]));
		Mnemo[i]->SetColorAndOpacity(DigestColor[i]);
	}
	if (PropertyUtilities.IsValid())
		PropertyUtilities->RequestRefresh();
}

//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№

TSharedRef<IPropertyTypeCustomization> FMyrLogicEventDataTypeCustomization::MakeInstance()
{	return MakeShareable(new FMyrLogicEventDataTypeCustomization);}

//==============================================================================================================
// инициализировать то, что будет рисоваться в строке заголовка свойства-структуры
//==============================================================================================================
void FMyrLogicEventDataTypeCustomization::CustomizeHeader(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//сохранить указатель на это нечто, чтобы с помощью него перериосвывать при изменении значений
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//получить рычаги влияния на те свойства, которые мы хотим видеть прямо в строке заголовка
	HandleColorForMe =			StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyrLogicEventData, EmotionDeltaForMe));
	HandleColorForGoal =		StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyrLogicEventData, EmotionDeltaForGoal));
	TSharedPtr<IPropertyHandle> HandleTimesToUseAfterCatch =	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyrLogicEventData, TimesToUseAfterCatch));
	TSharedPtr<IPropertyHandle> HandleMultEmotionByInputScalar =StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyrLogicEventData, MultEmotionByInputScalar));

	//вроде как подвязка обработчика события изменения значения свойства
	auto D = FSimpleDelegate::CreateSP(SharedThis(this), &FMyrLogicEventDataTypeCustomization::OnChanged);
	HandleColorForGoal.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, R))->SetOnPropertyValueChanged(D);
	HandleColorForGoal.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, G))->SetOnPropertyValueChanged(D);
	HandleColorForGoal.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, B))->SetOnPropertyValueChanged(D);
	HandleColorForGoal.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, A))->SetOnPropertyValueChanged(D);
	HandleColorForMe.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, R))->SetOnPropertyValueChanged(D);
	HandleColorForMe.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, G))->SetOnPropertyValueChanged(D);
	HandleColorForMe.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, B))->SetOnPropertyValueChanged(D);
	HandleColorForMe.Get()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, A))->SetOnPropertyValueChanged(D);

	OnChanged();

	//столбец заголовка (левый)
	HeaderRow.NameContent()
	[
		SNew(SHorizontalBox)
	]
	
	//столбец значений (правый) 
	.ValueContent()
	.MinDesiredWidth(1600.0f)
	.MaxDesiredWidth(1800.0f)
	[
		SNew(SHorizontalBox)

		//квадратик с цветом
		+ SHorizontalBox::Slot().FillWidth(1)						[SAssignNew(Me, SBorder).VAlign(VAlign_Center).HAlign(HAlign_Center).BorderImage(&bme)]

		//название эмоции
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)  [SNew(STextBlock).Text(FText::FromString("Emotion (Me)")).MinDesiredWidth(0)]

		//компонентв эмоции с подписями
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString("R"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[HandleColorForMe->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, R))->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString("L"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[HandleColorForMe->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, G))->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString("F"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[HandleColorForMe->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, B))->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString("*"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[HandleColorForMe->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, A))->CreatePropertyValueWidget()]

		+ SHorizontalBox::Slot().FillWidth(1)						[ SAssignNew(Goal, SBorder).VAlign(VAlign_Center).HAlign(HAlign_Center).BorderImage(&bgo)]

		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)  [ SNew(STextBlock).Text(FText::FromString("Emotion (Goal)")).MinDesiredWidth(0)]

		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[ SNew(STextBlock).Text(FText::FromString("R"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[ HandleColorForGoal->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, R))->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[ SNew(STextBlock).Text(FText::FromString("L"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[ HandleColorForGoal->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, G))->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[ SNew(STextBlock).Text(FText::FromString("F"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[ HandleColorForGoal->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, B))->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(0.5).HAlign(HAlign_Right)[ SNew(STextBlock).Text(FText::FromString("*"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[ HandleColorForGoal->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColor, A))->CreatePropertyValueWidget()]

		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[ SNew(STextBlock).Text(FText::FromString("Aftershocks:"))]
		+ SHorizontalBox::Slot().FillWidth(2)						[	HandleTimesToUseAfterCatch->CreatePropertyValueWidget()	]
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[ SNew(STextBlock).Text(FText::FromString("Scale By Input:"))]
		+ SHorizontalBox::Slot().FillWidth(1)						[	HandleMultEmotionByInputScalar->CreatePropertyValueWidget()	]

	];

}

//==============================================================================================================
// инициализировать то, что будет рисоваться в строках ниже заголовка структуры
//==============================================================================================================
void FMyrLogicEventDataTypeCustomization::CustomizeChildren(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//подбираем те свойства, которые мы хотим видеть в таблице ПОД заголовком
	TSharedPtr<IPropertyHandle> XPE = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyrLogicEventData, ExperienceIncrements));

	//просто добавляем строки с содержимым по умолчанию для данного типа свойств
	StructBuilder.AddProperty(XPE.ToSharedRef());

}

void FMyrLogicEventDataTypeCustomization::OnChanged()
{
	bgo.TintColor = GetColorForGoal();
	bme.TintColor = GetColorForMe();
	PropertyUtilities->RequestRefresh();

}


