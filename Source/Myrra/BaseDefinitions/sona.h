#pragma once

#if __has_include("CoreMinimal.h")
#include "CoreMinimal.h"
#include "sona.generated.h"
#endif

//координаты семы на карте слоя
struct SEMPOS
{
	unsigned short w;
	unsigned short h;
};

enum CONS_ART_TAG
{
	//место артикуляции			//		код грея
	empty = 0,					//		   0 0 0	
	glottal = 1,				//		   0 0 1	'
	uvular = 3,					//		   0 1 1	q
	velar = 2,					//		   0 1 0	k
	palatal = 6,				//		   1 1 0	c
	retroflex = 7,				//		   1 1 1	ch
	sibilant = 5,				//		   1 0 1	ts
	dental = 4,					//		   1 0 0	t

	plain = 0 << 4,				//		   0 0 0	
	trill = 1 << 4,				//		   0 0 1	
	nasal = 3 << 4,				//		   0 1 1	
	voiced_plosive = 2 << 4,	//		   0 1 0	
	voiced_aspirated = 6 << 4,	//		   1 1 0	
	voiced_fricative = 7 << 4,	//		   1 1 1	
	fricative = 5 << 4,			//		   1 0 1	
	aspirated = 4 << 4,			//		   1 0 0	

};

//попытка хранить речь в слогах CVC
struct SYLLABE
{
	enum CONS_ART_LOC
	{ 
		labial,		
		dental,		//01
		velar,		//10
		glottal		//11
	};

	enum CONS_ART_MAN
	{
		plosive,
		fricative,
		trill
	};
	CONS_ART_LOC initiale : 6;
	unsigned char mediale : 4;
	CONS_ART_LOC finale : 6;
};

//слову как осмысленной единице должен соответствовать один пиксель на текстуре

//лист дерева для поиска и уложения слов по критерию близости
struct MEMULA
{
	unsigned char fragment[8];		//фрагмент текста, 64 бита, 8 букв
	//а может в одном байте кодировать целый слог? 8 гласных, 32 согласных

};