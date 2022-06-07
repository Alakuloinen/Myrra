#include "PropertyCustomizations.h"
#include "../Myrra/Myrra.h"	
#include "SlateBasics.h"
#include "SlateCore.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h" //для SProperty
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"


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
		HandleLimbs[i].Get()->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FMyrGirdleModelTypeCustomization::OnChanged));

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
					+ SHorizontalBox::Slot().MaxWidth(40)
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
		for (int i = 0; i < EnumPtr->GetMaxEnumValue(); i++)
			if (Bits & (1 << i))	OutStr += EnumPtr->GetDisplayNameTextByIndex(i).ToString() + " ";

		//гравитация, важный признак, сделать строки сгравитацией визуально более яркими
		if (Bits & LDY_GRAVITY)
		{
			if (Bits == LDY_GRAVITY) FLinearColor(0.4, 0.4, 0.4);
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
