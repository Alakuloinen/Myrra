#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h" //для SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

TSharedRef<IPropertyTypeCustomization> FGirdleFlagsCustomization::MakeInstance()
{	return MakeShareable(new FGirdleFlagsCustomization);}

#define GETP(S) StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleFlags, S))

void FGirdleFlagsCustomization::CustomizeHeader(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//столбец заголовка (левый)
	HeaderRow.NameContent()	[	StructPropertyHandle->CreatePropertyNameWidget() ]
		
	//столбец значений (правый) - здесь ничего не будет, чтобы визуально не отвлекал от содержимого строк внутри
	.ValueContent()
		[
			//в остальные 4 ячейки левой половины заголовка засунуть 4 float свойства, друг за другом
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(5)[SNew(STextBlock).Text(StructPropertyHandle->GetPropertyDisplayName())]
			+ SHorizontalBox::Slot().AutoWidth()[GETP(FixOn)->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().FillWidth(5)[GETP(FixOn)->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().AutoWidth()[GETP(TightenYaw)->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().FillWidth(5)[GETP(TightenYaw)->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().AutoWidth()[GETP(TightenLean)->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().FillWidth(5)[GETP(TightenLean)->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().AutoWidth()[GETP(LockRoll)->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().FillWidth(5)[GETP(LockRoll)->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().AutoWidth()[GETP(LockPitch)->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().FillWidth(5)[GETP(LockPitch)->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().AutoWidth()[GETP(LockYaw)->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().FillWidth(5)[GETP(LockYaw)->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().AutoWidth()[GETP(Vertical)->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().FillWidth(5)[GETP(Vertical)->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().AutoWidth()[GETP(LockLegsSwing)->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().FillWidth(5)[GETP(LockLegsSwing)->CreatePropertyNameWidget()]
		];


}

void FGirdleFlagsCustomization::CustomizeChildren(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

TSharedRef<IPropertyTypeCustomization> FMyrDynModelTypeCustomization::MakeInstance()
{	return MakeShareable(new FMyrDynModelTypeCustomization);}

#define GETP(S) StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWholeBodyDynamicsModel, S))

//==============================================================================================================
// инициализировать то, что будет рисоваться в строке заголовка свойства-структуры
//==============================================================================================================
void FMyrDynModelTypeCustomization::CustomizeHeader(
	TSharedRef<class IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//получить рычаги влияния на те свойства, которые мы хотим видеть прямо в строке заголовка

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

	//GETP(TimeDilation)->CreatePropertyValueWidget

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
		[	SNew(SProperty, GETP(Use)).ShouldDisplayName(true) ]


		//разделитель для красоты
		+ SHorizontalBox::Slot().FillWidth(5).VAlign(VAlign_Center)
		[	SNew(STextBlock).Text(FText::FromString(Sep)).ColorAndOpacity(SepCol).ShadowOffset(FVector2D(5, 0)) ]

		//замедление времени
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Left)
			[SNew(SProperty, GETP(TimeDilation)).ShouldDisplayName(true)]

		//размытие в движении
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Left)
			[SNew(SProperty, GETP(MotionBlur)).ShouldDisplayName(true)]

		//максимальный урон
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Left)
			[SNew(SProperty, GETP(DamageLim)).ShouldDisplayName(true)]

		// название и виджет ассета звука без громоздкого ярлыка (надо как-то добавить фильтр)
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)
		[	GETP(Sound)->CreatePropertyNameWidget()	]
		+ SHorizontalBox::Slot().FillWidth(10)
		[	SNew(SObjectPropertyEntryBox).PropertyHandle(GETP(Sound)).DisplayThumbnail(false) ]


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
			+ SHorizontalBox::Slot().AutoWidth() [GETP(AnimRate)->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().AutoWidth() [GETP(AnimRate)->CreatePropertyValueWidget()]
		]
		+ SHorizontalBox::Slot().FillWidth(25)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)	[SNew(STextBlock).Text(FText::FromString(Sep)).ColorAndOpacity(SepCol).ShadowOffset(FVector2D(5, 0))]
			+ SHorizontalBox::Slot().AutoWidth() [GETP(HealthAdd)->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().AutoWidth() [GETP(HealthAdd)->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().AutoWidth() [GETP(StaminaAdd)->CreatePropertyValueWidget()]
		]
		+ SHorizontalBox::Slot().FillWidth(13).HAlign(HAlign_Right) [	GETP(MotionGain)->CreatePropertyNameWidget()	]
		+ SHorizontalBox::Slot().FillWidth(5)						[	GETP(MotionGain)->CreatePropertyValueWidget()	]
		+ SHorizontalBox::Slot().FillWidth(5)						[	GETP(JumpImpulse)->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(10).HAlign(HAlign_Right)	[	GETP(SpineStiffness)->CreatePropertyNameWidget()]
		+ SHorizontalBox::Slot().FillWidth(5)						[	GETP(SpineStiffness)->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[	GETP(MoveWithNoExtGain)->CreatePropertyNameWidget()		]
		+ SHorizontalBox::Slot().FillWidth(2)						[	GETP(MoveWithNoExtGain)->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[	GETP(PreJump)->CreatePropertyNameWidget()		]
		+ SHorizontalBox::Slot().FillWidth(2)						[	GETP(PreJump)->CreatePropertyValueWidget()		]
		+ SHorizontalBox::Slot().FillWidth(5).HAlign(HAlign_Right)	[	GETP(FlyFixed)->CreatePropertyNameWidget()		]
		+ SHorizontalBox::Slot().FillWidth(2)						[	GETP(FlyFixed)->CreatePropertyValueWidget()		]

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
	EnumDyMoPtr = FindObject<UEnum>(nullptr, TEXT("/Script/Myrra.ELDY"), true);

	//это свойство надо оживить подвязкой обработчика изменений
	HandleUsed = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGirdleDynModels, Use));
	HandleUsed->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FMyrGirdleModelTypeCustomization::OnUsedChanged));

	
	//строка разделителя
	auto Sep = FString::Printf(TEXT("%s"), std::wstring(150, L'▖').c_str());
	FLinearColor SepCol(0.07, 0.07, 0.01, 1);

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
			//разделитель для красоты
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

	FString EntNames[3];
	if (StructPropertyHandle->GetProperty()->GetNameCPP() == TEXT("Thorax"))
	{	EntNames[0] = TEXT("Thorax"); EntNames[1] = TEXT("Pectus"); EntNames[2] = TEXT("Head");	} else
	{	EntNames[0] = TEXT("Pelvis"); EntNames[1] = TEXT("Lumbus"); EntNames[2] = TEXT("Tail");	} 


	//подбираем те свойства, которые мы хотим видеть в теле таблицы, ПОД заголовком
	HandleLimbs[0] = GETP(Center);
	HandleLimbs[1] = GETP(Spine);
	HandleLimbs[2] = GETP(Tail);

	HandleFlags[0] = GETP(OnGnd);
	HandleFlags[1] = GETP(InAir);


	TSharedPtr<IPropertyHandle> HandleF[6];
	HandleF[0] = GETP(Crouch);
	HandleF[1] = GETP(ForcesMultiplier);

	//формируем строки и цвета по начальным значениям 
	OnChanged();

	for (int i = 0; i < 3; i++)
	{
		//вроде как подвязка обработчика события изменения значения свойства
		HandleLimbs[i].Get()->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FMyrGirdleModelTypeCustomization::OnChanged));

		StructBuilder.AddCustomRow(FText::FromString(EntNames[0]))
			.NameContent().HAlign(HAlign_Fill)
			[
				//1. название членика, 2. маленький контрол битов, 3. выписка битов длинной строкой
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().MaxWidth(80)[SNew(STextBlock).Text(FText::FromString(EntNames[i])).ColorAndOpacity(FLinearColor(0.5, 0.5, 0.5, 1))]
				+ SHorizontalBox::Slot().MaxWidth(25)[HandleLimbs[i]->CreatePropertyValueWidget()]
				+ SHorizontalBox::Slot().FillWidth(10)[SAssignNew(Mnemo[i], STextBlock).Text(FText::FromString(Digest[i])).ColorAndOpacity(DigestColor[i])]
			]
			.ValueContent().HAlign(HAlign_Fill)
			[
				i < 2 ?

				//1. набор битов закреплений для земли и воздуха, 
				StructBuilder.GenerateStructValueWidget(HandleFlags[i].ToSharedRef()) 

				:

				//1. раскоряка ног, 2. жесткость связи пояса со спиной, 3. общая вязкость
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().HAlign(HAlign_Right).FillWidth(1)[GETP(LegsSpreadMin)->CreatePropertyNameWidget()]
				+ SHorizontalBox::Slot().MaxWidth(50)[GETP(LegsSpreadMin)->CreatePropertyValueWidget()]
				+ SHorizontalBox::Slot().MaxWidth(50)[GETP(LegsSpreadMax)->CreatePropertyValueWidget()]

				+ SHorizontalBox::Slot().HAlign(HAlign_Right)[GETP(JunctionStiffness)->CreatePropertyNameWidget()]
				+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)[StructBuilder.GenerateStructValueWidget(GETP(JunctionStiffness).ToSharedRef())]

				+ SHorizontalBox::Slot().HAlign(HAlign_Right).AutoWidth()[GETP(CommonDampingIfNeeded)->CreatePropertyNameWidget()]
				+ SHorizontalBox::Slot().HAlign(HAlign_Left).MaxWidth(50)[GETP(CommonDampingIfNeeded)->CreatePropertyValueWidget()]

				+ SHorizontalBox::Slot().HAlign(HAlign_Right).AutoWidth()[GETP(Crouch)->CreatePropertyNameWidget()]
				+ SHorizontalBox::Slot().HAlign(HAlign_Left).MaxWidth(50)[GETP(Crouch)->CreatePropertyValueWidget()]

				+ SHorizontalBox::Slot().HAlign(HAlign_Right).AutoWidth()[GETP(ForcesMultiplier)->CreatePropertyNameWidget()]
				+ SHorizontalBox::Slot().HAlign(HAlign_Left).MaxWidth(50)[GETP(ForcesMultiplier)->CreatePropertyValueWidget()]

				+ SHorizontalBox::Slot().HAlign(HAlign_Right).AutoWidth()[GETP(FairSteps)->CreatePropertyNameWidget()]
				+ SHorizontalBox::Slot().HAlign(HAlign_Left).MaxWidth(20)[GETP(FairSteps)->CreatePropertyValueWidget()]
			];
	}

/*	//вроде как подвязка обработчика события изменения значения свойства
	HandleLimbs[2].Get()->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FMyrGirdleModelTypeCustomization::OnChanged));

	StructBuilder.AddCustomRow(FText::FromString(EntNames[2]))
		.NameContent().HAlign(HAlign_Fill)
		[
			//1. название членика, 2. маленький контрол битов, 3. выписка битов длинной строкой
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().MaxWidth(80)[SNew(STextBlock).Text(FText::FromString(EntNames[2])).ColorAndOpacity(FLinearColor(0.5, 0.5, 0.5, 1))]
			+ SHorizontalBox::Slot().MaxWidth(25)[GETP(Tail)->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().FillWidth(10)[SAssignNew(Mnemo[2], STextBlock).Text(FText::FromString(Digest[2])).ColorAndOpacity(DigestColor[2])]
		]
		.ValueContent().HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().HAlign(HAlign_Right).FillWidth(1)	[GETP(LegsSpreadMin)->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().MaxWidth(50)						[GETP(LegsSpreadMin)->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().MaxWidth(50)						[GETP(LegsSpreadMax)->CreatePropertyValueWidget()]

			+ SHorizontalBox::Slot().HAlign(HAlign_Right)[GETP(JunctionStiffness)->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)[StructBuilder.GenerateStructValueWidget(GETP(JunctionStiffness).ToSharedRef())]

			+ SHorizontalBox::Slot().HAlign(HAlign_Right).FillWidth(1)	[GETP(CommonDampingIfNeeded)->CreatePropertyNameWidget()]
			+ SHorizontalBox::Slot().HAlign(HAlign_Right).MaxWidth(50)	[GETP(CommonDampingIfNeeded)->CreatePropertyValueWidget()]
		];

	//формируем строки и цвета по начальным значениям 
	OnChanged();*/

	//раскладка по строкам свойств битов для частей тела
/*	for (int i = 0; i < 3; i++)
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
				//+ SHorizontalBox::Slot().FillWidth(10)	[SNew(SSpinBox<float>).MaxFractionalDigits(3)]
			]
			//правая часть 
			.ValueContent().HAlign(HAlign_Fill)
			[
				//в одной ячейке стандартный виджет свойства, комбобокс с битами, поуже, во второй - строка расшифровки
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().MaxWidth(80) [ SNew(STextBlock).Text(FText::FromString(EntNames[i+3])).ColorAndOpacity(FLinearColor(0.5, 0.5, 0.5, 1))]
				+ SHorizontalBox::Slot().FillWidth(12)[	SAssignNew(AnchBits[i], SHorizontalBox)	]

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
	//третья строка
	AnchBits[2]->AddSlot().AutoWidth()[GETP(LegsSpreadMin)->CreatePropertyNameWidget()];
	AnchBits[2]->AddSlot().AutoWidth()[GETP(LegsSpreadMin)->CreatePropertyValueWidget()];
	AnchBits[2]->AddSlot().AutoWidth()[GETP(LegsSpreadMax)->CreatePropertyValueWidget()];

	AnchBits[2]->AddSlot().AutoWidth().HAlign(HAlign_Right)[GETP(JunctionStiffness)->CreatePropertyNameWidget()];
	AnchBits[2]->AddSlot().AutoWidth().HAlign(HAlign_Right)[StructBuilder.GenerateStructValueWidget(GETP(JunctionStiffness).ToSharedRef())];
	
	*/
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
