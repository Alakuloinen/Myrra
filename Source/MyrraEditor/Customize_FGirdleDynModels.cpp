#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"

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
	TSharedPtr<IPropertyHandle> HandleJumpImp = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, JumpImpulse));
	TSharedPtr<IPropertyHandle> HandleAutoMove = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, MoveWithNoExtGain));
	TSharedPtr<IPropertyHandle> HandlePreJump = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, PreJump));
	TSharedPtr<IPropertyHandle> HandleFlyFixed = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, FlyFixed));
	TSharedPtr<IPropertyHandle> HandleSpineStiffness = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, SpineStiffness));

	TSharedPtr<IPropertyHandle> HandleSound = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Sound));

	HandleUse = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Use));
	HandleUse->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FMyrDynModelTypeCustomization::OnUsedChanged));

	//для заголовка таким вот тупым образом распознать цифры и к ним добавить эквиваленты этих цифр, как если бы они были константами фаз атаки
	FString FullHead = StructPropertyHandle->GetPropertyDisplayName().ToString();
	auto P = StructPropertyHandle->GetParentHandle();
	auto A = P->AsArray();
	if (A.IsValid())
	{
		uint32 RealNum = FullHead[0] - L'0';
		uint32 nphas;
		A->GetNumElements(nphas);
		switch (nphas)
		{
		case 6: 
			FullHead = TXTENUM(EActionPhase, (EActionPhase)RealNum);
			break;
		case 3:
			if (RealNum == 0) FullHead = TEXT("Transit To");
			if (RealNum == 1) FullHead = TEXT("Persist In");
			if (RealNum == 1) FullHead = TEXT("Exit");
			break;
		}
	}


	auto Sep = FString::Printf(TEXT("%s"), std::wstring(150, L'█').c_str());
	FLinearColor SepCol(0.02, 0.01, 0.02, 1);

	//столбец заголовка (левый)
	HeaderRow.NameContent()
	[
		//прямо в левой половине заводим контейнер из нескольких ячеек
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				//вместо стандартного виджета создать текст, взяв строку из имени свойства - чтобы текст можно было покрасить цветом
				//в данном случае это просто номер элемента в массиве
				SNew(STextBlock).Text(FText::FromString(FullHead))
				.ColorAndOpacity(FLinearColor(1, 0, 1, 1))
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
			]
			//разделитель для красоты
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
				[SNew(STextBlock).Text(FText::FromString(Sep)).ColorAndOpacity(SepCol).ShadowOffset(FVector2D(5, 0))]
		]

		//флаг использовать
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left)
		[	HandleUse->CreatePropertyNameWidget()	]

		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left)
		[	HandleUse->CreatePropertyValueWidget()]

		//разделитель для красоты
		+ SHorizontalBox::Slot().FillWidth(10).VAlign(VAlign_Center)
		[	SNew(STextBlock).Text(FText::FromString(Sep)).ColorAndOpacity(SepCol).ShadowOffset(FVector2D(5, 0)) ]

		// название и виджет ассета звука без громоздкого ярлыка (надо как-то добавить фильтр)
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)
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
		+ SHorizontalBox::Slot().FillWidth(20)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)	[SNew(STextBlock).Text(FText::FromString(Sep)).ColorAndOpacity(SepCol).ShadowOffset(FVector2D(5, 0))]
			+ SHorizontalBox::Slot().AutoWidth() [HandleAnimRate->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().AutoWidth() [HandleAnimRate->CreatePropertyValueWidget()]
		]
		+ SHorizontalBox::Slot().FillWidth(25)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)	[SNew(STextBlock).Text(FText::FromString(Sep)).ColorAndOpacity(SepCol).ShadowOffset(FVector2D(5, 0))]
			+ SHorizontalBox::Slot().AutoWidth() [HandleHealthAdd->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().AutoWidth() [HandleHealthAdd->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().AutoWidth() [HandleStaminaAdd->CreatePropertyValueWidget()]
		]
		+ SHorizontalBox::Slot().FillWidth(13).HAlign(HAlign_Right) [	HandleMotionGain->CreatePropertyNameWidget()	]
		+ SHorizontalBox::Slot().FillWidth(5)						[	HandleMotionGain->CreatePropertyValueWidget()	]
		+ SHorizontalBox::Slot().FillWidth(5)						[	HandleJumpImp->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(10).HAlign(HAlign_Right)	[	HandleSpineStiffness->CreatePropertyNameWidget()]
		+ SHorizontalBox::Slot().FillWidth(5)						[	HandleSpineStiffness->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[	HandleAutoMove->CreatePropertyNameWidget()		]
		+ SHorizontalBox::Slot().FillWidth(2)						[	HandleAutoMove->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[	HandlePreJump->CreatePropertyNameWidget()		]
		+ SHorizontalBox::Slot().FillWidth(2)						[	HandlePreJump->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[	HandleFlyFixed->CreatePropertyNameWidget()		]
		+ SHorizontalBox::Slot().FillWidth(2)						[	HandleFlyFixed->CreatePropertyValueWidget()		]

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
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	//подбираем те свойства, которые мы хотим видеть в таблице ПОД заголовком
	TSharedPtr<IPropertyHandle> HandleThorax = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Thorax));
	TSharedPtr<IPropertyHandle> HandlePelvis = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, Pelvis));

	//просто добавляем строки с содержимым по умолчанию для данного типа свойств
	IDetailPropertyRow& PropertyThoraxRow = StructBuilder.AddProperty(HandleThorax.ToSharedRef());
	IDetailPropertyRow& PropertyPelvisRow = StructBuilder.AddProperty(HandlePelvis.ToSharedRef());

	//попытки выключать виджеты, если галочка юз снята
	if(PropertyThoraxRow.CustomNameWidget())
		ThoN = PropertyThoraxRow.CustomNameWidget()->Widget;
	else PropertyThoraxRow.GetDefaultWidgets(ThoN, ThoV);

	if (PropertyThoraxRow.CustomValueWidget())
		ThoV = PropertyThoraxRow.CustomValueWidget()->Widget;
	else PropertyThoraxRow.GetDefaultWidgets(ThoV, ThoV);

	if (PropertyPelvisRow.CustomNameWidget())
		PelN = PropertyPelvisRow.CustomNameWidget()->Widget;
	else PropertyPelvisRow.GetDefaultWidgets(PelN, PelV);

	if (PropertyPelvisRow.CustomValueWidget())
		PelV = PropertyPelvisRow.CustomValueWidget()->Widget;
	else PropertyPelvisRow.GetDefaultWidgets(PelV, PelV);
}

//==============================================================================================================
//обработчик события
//==============================================================================================================
void FMyrDynModelTypeCustomization::OnUsedChanged()
{
	if (!HandleUse.IsValid()) return;
	bool Val; HandleUse->GetValue(Val);
	if (Val)
	{
		ThoN->SetVisibility(EVisibility::Visible);
		ThoV->SetVisibility(EVisibility::Visible);
	}
	else
	{
		ThoN->SetVisibility(EVisibility::Collapsed);
		ThoV->SetVisibility(EVisibility::Collapsed);

	}
	if (PropertyUtilities.IsValid())
		PropertyUtilities->RequestRefresh();
}

//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№


TSharedRef<IPropertyTypeCustomization> FMyrGirdleModelTypeCustomization::MakeInstance()
{
	return MakeShareable(new FMyrGirdleModelTypeCustomization);
}

//==============================================================================================================
// инициализировать то, что будет рисоваться в строке заголовка свойства-структуры
//==============================================================================================================
void FMyrGirdleModelTypeCustomization::CustomizeHeader(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//сохранить родительский хэндл
	MainHandle = StructPropertyHandle;

	//найти объявленное где-то в исходном коде перечисление 
	EnumDyMoPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ELDY"), true);

	//это свойство надо оживить подвязкой обработчика изменений
	HandleUsed = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Use));
	HandleUsed->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FMyrGirdleModelTypeCustomization::OnUsedChanged));

	//строка разделителя
	auto Sep = FString::Printf(TEXT("%s"), std::wstring(150, L'▖').c_str());
	FLinearColor SepCol(0.05, 0.05, 0.01, 1);

	//столбец заголовка (левый)
	HeaderRow.NameContent()
		.HAlign(EHorizontalAlignment::HAlign_Fill)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(2).VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				//текст названия пояса, thprax,pelvis
				SNew(STextBlock)
				.Text(StructPropertyHandle->GetPropertyDisplayName())
				.ColorAndOpacity(FLinearColor(255, 255, 0, 255))
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
			]
			+ SHorizontalBox::Slot().FillWidth(1)
			[	SNew(STextBlock).Text(FText::FromString(Sep)).ColorAndOpacity(SepCol).ShadowOffset(FVector2D(5, 0)) ]
				
		]
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left)
		[
			//чекбокс свойства "Used"
			SNew(SProperty, HandleUsed).ShouldDisplayName(true)
		]
		+ SHorizontalBox::Slot().FillWidth(10).HAlign(HAlign_Left).VAlign(VAlign_Center)
			[
				//наверно, можно черту-разделитель ка-то более просто сделать, но поиск не дал результата
				SNew(STextBlock).Text(FText::FromString(Sep)).ColorAndOpacity(SepCol).ShadowOffset(FVector2D(5, 0))
			]
		//чекбокс свойства "Leading"
		+ SHorizontalBox::Slot().AutoWidth()
		[	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Leading))->CreatePropertyNameWidget()	]
		+ SHorizontalBox::Slot().AutoWidth()
		[	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Leading))->CreatePropertyValueWidget()	]

	]

	//столбец значений (правый) - растягиваем до конца окна
	.ValueContent()
		.MinDesiredWidth(2000.0f)
		.MaxDesiredWidth(2000.0f).VAlign(VAlign_Center)
		[
			//наверно, можно черту-разделитель ка-то более просто сделать, но поиск не дал результата
			SNew(STextBlock).Text(FText::FromString(Sep)).ColorAndOpacity(SepCol).ShadowOffset(FVector2D(5, 0))

		];
}

#define GETP(S) StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, S))
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

	FString EntNames[6];
	if (StructPropertyHandle->GetProperty()->GetNameCPP() == TEXT("Thorax"))
	{	EntNames[0] = TEXT("Thorax"); EntNames[1] = TEXT("Pectus"); EntNames[2] = TEXT("Head");	} else
	{	EntNames[0] = TEXT("Pelvis"); EntNames[1] = TEXT("Lumbus"); EntNames[2] = TEXT("Tail");	} 
	EntNames[3] = TEXT("On Floor");
	EntNames[4] = TEXT("In Air");
	EntNames[5] = TEXT("Other");



	//подбираем те свойства, которые мы хотим видеть в теле таблицы, ПОД заголовком
	HandleLimbs[0] = GETP(Center);
	HandleLimbs[1] = GETP(Spine);
	HandleLimbs[2] = GETP(Tail);

	TSharedPtr<IPropertyHandle> HandleF[5];
	HandleF[0] = GETP(Crouch);
	HandleF[1] = GETP(ForcesMultiplier);
	HandleF[2] = GETP(CommonDampingIfNeeded);
	HandleF[3] = GETP(LegsSpreadMin);
	HandleF[4] = GETP(LegsSpreadMax);

	TSharedPtr<IPropertyHandle> HandleB[5];
	HandleB[0] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HardVertical));
	HandleB[1] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, HardCourseAlign));
	HandleB[2] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, FixOnFloor));
	HandleB[3] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, LegsSwingLock));
	HandleB[4] = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, NormalAlign));

	TSharedPtr<IPropertyHandle> HAnchMo[8][2];
	HAnchMo[0][0] = GETP(FixOnFloor);			HAnchMo[0][1] = GETP(FixOnTraj);
	HAnchMo[1][0] = GETP(TightenYawOnFloor);	HAnchMo[1][1] = GETP(TightenYawInAir);
	HAnchMo[2][0] = GETP(TightenLeanOnFloor);	HAnchMo[2][1] = GETP(TightenLeanInAir);
	HAnchMo[3][0] = GETP(LockYawOnFloor);		HAnchMo[3][1] = GETP(LockYawInAir);
	HAnchMo[4][0] = GETP(LockPitchOnFloor);		HAnchMo[4][1] = GETP(LockPitchInAir);
	HAnchMo[5][0] = GETP(LockRollOnFloor);		HAnchMo[5][1] = GETP(LockRollInAir);
	HAnchMo[6][0] = GETP(VerticalOnFloor);		HAnchMo[6][1] = GETP(VerticalInAir);
	HAnchMo[7][0] = GETP(LockLegsSwingOnFloor);	HAnchMo[7][1] = GETP(LockLegsSwingInAir);

	TSharedPtr<SHorizontalBox> AnchBits[3];


	//формируем строки и цвета по начальным значениям 
	OnChanged();

	//раскладка по строкам свойств битов для частей тела
	for (int i = 0; i < 3; i++)
	{
		//вроде как подвязка обработчика события изменения значения свойства
		HandleLimbs[i].Get()->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FMyrGirdleModelTypeCustomization::OnChanged));

		RowWidgets[i] = StructBuilder.AddCustomRow(FText::FromString(EntNames[i]))

			//левая часть - типа имя свойства
			.NameContent().HAlign(HAlign_Fill)
			[
				//биты физических состояний члеников спины, пояса и головы/хвоста
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().MaxWidth(80)  [ SNew(STextBlock).Text(FText::FromString(EntNames[i])).ColorAndOpacity(FLinearColor(0.5, 0.5, 0.5, 1))]
				+ SHorizontalBox::Slot().MaxWidth(25)  [ HandleLimbs[i]->CreatePropertyValueWidget() ]
				+ SHorizontalBox::Slot().FillWidth(10)  [ SAssignNew(Mnemo[i], STextBlock).Text(FText::FromString(Digest[i])).ColorAndOpacity(DigestColor[i])]
			]
			//правая часть 
			.ValueContent().HAlign(HAlign_Fill)
			[
				//в одной ячейке стандартный виджет свойства, комбобокс с битами, поуже, во второй - строка расшифровки
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().MaxWidth(80) [ SNew(STextBlock).Text(FText::FromString(EntNames[i+3])).ColorAndOpacity(FLinearColor(0.5, 0.5, 0.5, 1))]
				+ SHorizontalBox::Slot().FillWidth(12)
				[
					SAssignNew(AnchBits[i], SHorizontalBox)
				]

				//мелкий столбик - втиснуть свойство не относящееся к битам части тела, но удобно занимающее лишнее место
				+ SHorizontalBox::Slot().HAlign(HAlign_Right).FillWidth(1)
					[	HandleF[i]->CreatePropertyNameWidget()	]
				+ SHorizontalBox::Slot().MaxWidth(50)
					[	HandleF[i]->CreatePropertyValueWidget()	]

	
			];
	}

	//наполнить заготовленную ячейку флагами
	for (int j = 0; j < 2; j++)
	{
		for (int i = 0; i < 8; i++)
		{
			AnchBits[j]->AddSlot();
			AnchBits[j]->GetSlot(2 * i).FillWidth(3)[HAnchMo[i][j]->CreatePropertyNameWidget()];
			AnchBits[j]->GetSlot(2 * i).SetHorizontalAlignment(HAlign_Right);
			AnchBits[j]->AddSlot();
			AnchBits[j]->GetSlot(2 * i + 1).FillWidth(1)[HAnchMo[i][j]->CreatePropertyValueWidget()];
			AnchBits[j]->GetSlot(2 * i + 1).SetHorizontalAlignment(HAlign_Left);
		}
	}
	AnchBits[2]->AddSlot().AutoWidth()[HandleF[3]->CreatePropertyNameWidget()];
	AnchBits[2]->AddSlot().AutoWidth()[HandleF[3]->CreatePropertyValueWidget()];
	AnchBits[2]->AddSlot().AutoWidth()[HandleF[4]->CreatePropertyValueWidget()];
}

//==============================================================================================================
//превратить набор битов в строку подсказку, что отображается напротив - удобнее вместо всплывающей
//==============================================================================================================
FLinearColor FMyrGirdleModelTypeCustomization::MakeDigestString(int32 Bits, FString& OutStr, char TypeCode)
{
	//накопитель для цвета строки, изначально черный, чтоб не засветить
	FLinearColor ReCo(0.0, 0.0, 0.0, 1.0);

	//подразумевается, что указатель на перечисление уже был привязан к реальному перечислителю битов
	if (!Bits)
	{	OutStr = TEXT("Passive");
		ReCo += FLinearColor(0.3, 0.3, 0.3);
	}
	else
	{
		if (Bits & LDY_GRAVITY) ReCo += FLinearColor(0.3, 0.3, 0.3);
		if (Bits & LDY_ROTATE) ReCo += FLinearColor(0.5, 0.5, 0.5);
		if (Bits & LDY_PULL) ReCo += FLinearColor(0.6, 0.6, 0.6);
		if (Bits & LDY_TO_NORMAL) ReCo *= FLinearColor(0.8, 1.0, 0.8);
		if (Bits & LDY_TO_COURSE) ReCo *= FLinearColor(1.0, 0.8, 0.8);
		if (Bits & LDY_TO_VERTICAL) ReCo *= FLinearColor(0.8, 0.8, 1.0);
		if (Bits & LDY_TO_ATTACK) ReCo *= FLinearColor(1.0, 1.0, 0.8);

		//если найден взведеным очередной бит, добавить в строку то, как этот бит назван
		OutStr = TEXT("");
		for (int i = 0; i < EnumDyMoPtr->GetMaxEnumValue(); i++)
			if (Bits & (1 << i))
				OutStr += EnumDyMoPtr->GetDisplayNameTextByIndex(i).ToString() + " ";
	}
	return ReCo;
}

//==============================================================================================================
//обработчик события
//==============================================================================================================
void FMyrGirdleModelTypeCustomization::OnChanged()
{
	//преобразоват в строку
	int32 Val;
	for (int i = 0; i < 3; i++)
	{
		if (!HandleLimbs[i].IsValid()) continue;

		//тип переменной в виде строки
		auto TyNa = HandleLimbs[i]->GetProperty()->GetCPPType().GetCharArray();

		//добыть значение и расчленить в строку
		if (HandleLimbs[i].Get()->GetValue(Val) != FPropertyAccess::Result::Success)
			HandleLimbs[i].Get()->GetValue((uint8&)Val);
		DigestColor[i] = MakeDigestString(Val, Digest[i], TyNa[0]);

		//вывод в сохраненное текстовое поле
		if (!Mnemo[i].IsValid()) continue;
		Mnemo[i]->SetText(FText::FromString(Digest[i]));
		Mnemo[i]->SetColorAndOpacity(DigestColor[i]);
	}
	if (PropertyUtilities.IsValid())
		PropertyUtilities->RequestRefresh();
}

void FMyrGirdleModelTypeCustomization::OnUsedChanged()
{
	if(!HandleUsed.IsValid()) return;
	bool Val; HandleUsed->GetValue(Val);
	if(Val)
	{
		for (int i = 0; i < 5; i++)
		{
			RowWidgets[i].WholeRowContent().Widget->SetVisibility(EVisibility::Visible);
			RowWidgets[i].ValueWidget.Widget->SetVisibility(EVisibility::Visible);
			RowWidgets[i].NameWidget.Widget->SetVisibility(EVisibility::Visible);
		}
	}
	else
	{
		for (int i = 0; i < 5; i++)
		{
			RowWidgets[i].WholeRowContent().Widget->SetVisibility(EVisibility::Collapsed);
			RowWidgets[i].ValueWidget.Widget->SetVisibility(EVisibility::Collapsed);
			RowWidgets[i].NameWidget.Widget->SetVisibility(EVisibility::Collapsed);
		}
	}
	if (PropertyUtilities.IsValid())
		PropertyUtilities->RequestRefresh();
}
