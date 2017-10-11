
#ifndef INTRPRETER_H
#define INTRPRETER_H

#include "stdafx.h"

// définition des caractères spéciaux
#define CTAG 		'#'  // codage d'une instruction (police, centrage, ...)
#define CFONT		'p'	 // changement de police
#define CNORMAL		'w'	 // mode d'écriture simple (bleu sur fond blanc)
#define CINVERSE	'm'	 // mode d'écriture inversé (blanc sur fond bleu)
#define CINVISIBLE	'i'	 // mode d'écriture invisible
#define CVISIBLE	'!'  // mode d'écriture visible
#define CNEWLINE	'n'	 // saut de ligne
#define C1PIXEL		'1'	 // espacement d'1 pixel
#define CCENTERH	'c'	 // centrage horizontal
#define CALIGNL		'l'	 // alignement à gauche

#define CAUTO		'a'	 // mode affichage automatique



#define BORDERX		0	// bordure en pixel sur axe X
#define BORDERY		0	// bordure en pixel sur axe Y


typedef enum {
	AUTO
} DisplayMode;


/* Définition du mode/style d'écriture utilisé 
 * à un instant T
 */
typedef struct {
	// police utilisée
	BYTE	fontId;		

	// mode d'affichage utilisé : 2 lignes, plein écran, ...
	DisplayMode	mode;	

	// écriture en mode inversée activée
	BOOL	inverseWriting;	
	// mode invisible
	BOOL	invisible;
	// centrage activé. Si non, alignement à gauche	
	BOOL	centerText;

	// position du curseur sur l'écran à un instant T
	WORD	cursorX;
	WORD	cursorY;
	// position du curseur précédent (seulement X nécessaire)
	WORD	prevCursorX;

	// délimitation verticale
	WORD areaYStart;
	WORD areaYEnd;

 } Brush;



// déclaration des prototypes
int parseHeader(const char *string);
int parseData();
int decodeTag(const char *string, Brush *myBrush);
int writeString(const char *string, BYTE heightLine);

#endif	