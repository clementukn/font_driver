
/* Pour �conomiser la m�moire, le choix a �t� fait de parser la police � la
 * vol�e.
 * On pourrait imaginer parser cette police au d�but puis enregistrer les 
 * donn�es dans une structure de police d�finie (cf doc).
 *
 * font_tools g�re toutes les m�thodes de traitement li�es � la police.
 */

#include "stdafx.h"

#include "font_tools.h"
#include "interpreter.h"
#include "screen.h"

#ifndef EBULENT
#define DEBUG
#endif

// constantes du format police
#define FT_HEADER_SIZE		5
#define FT_CHARH_SIZE		7	// sans la liste des donn�es de pixels


/* polices de caract�res */
#include "font.c"

// pointeur vers la police en cours
const BYTE *font;
// taille en octet de la police en cours
WORD fontBytes;


/* Retourne l'id de la liste fontList correspond � l'idFont demand�
 * l'idFont peut aller de 1 � 9 mais et ne correspond pas forc�ment
 * � l'id de fontList
 */
int 	searchFont(BYTE idFont) {
	int i;

	// recherche de l'id de la police dans la liste
	for (i = 0; i < nbrFonts; i++)
		if (fontList[i][0] == idFont)
			return i;

	return -1;
}


// S�lectionne un police dans la liste en fonction de son id
BOOL	selectFont(BYTE idFont) {
	int i;
	if ((i = searchFont(idFont)) >= 0) {
		font = fontList[i];	
		fontBytes = fontDataSize[i];
		return TRUE;
	} 
	else
		return FALSE;
}


// lit la police et renvoie la valeur de l'ascent
BYTE	getAscent() {
	return font[3];
}


// retourne la largeur en pixel d'un espace
BYTE 	getSpaceWidth() {
	return font[4];
}


WORD 	getFontSize(BYTE idFont) {
	int i;
	if ((i = searchFont(idFont)) >= 0) {
		return fontList[i][2];	
	} 
	else
		return 0;
}


WORD 	getCurrFontSize() {
	return font[2];
}


// Retourne la taille du header en octets
BYTE  	getHeaderSize() {
	return FT_HEADER_SIZE;
}


/* Retourne le nombre d'octets de codage du caract�res
 * posChar indique l'offset de l'adresse de l'en-t�te du cacact�re
 * par rapport � font
 */
WORD 	getCharDataBytes(WORD posChar) {
	BYTE	charWidth, charHeight;
	WORD	bytes;

	charWidth 	= font[posChar + 1];
	charHeight 	= font[posChar + 2];
	bytes = charWidth * charHeight;
	bytes = bytes / 8 + (bytes%8==0 ? 0 : 1);

	return bytes;
}


// retourne la position d'un caract�re donn� dans la police
WORD 	getCharPos(BYTE c) {
	BOOL 	isFound = FALSE;
	WORD 	bytes = 0, cursFont = getHeaderSize(), defFont = 0;

	// calcul la longueur de la lettre
	// On boucle sur le code ASCII de chaque caract�re pour trouver c
	while (!isFound && cursFont < fontBytes) {
		if (font[cursFont] == c)
			isFound = TRUE;
		// sinon calcul le nombre de donn�es cod�es par le caract�res
		// pour lire le suivant
		else {
			if (font[cursFont] == STUB_CHAR)
				defFont = cursFont;
			// r�cup�re le nombre d'octets de codage du caract�re
			bytes = getCharDataBytes(cursFont);
			// on se d�place de en-t�te caract�re + donn�e
			cursFont += FT_CHARH_SIZE + bytes;
		}
	}

	if (isFound) {
#ifdef DEBUG
	printf("\nCaractere %c trouve.", font[cursFont]);
	printf("\n\tlargeur : %d", font[cursFont + 1]);
	printf("\n\thauteur : %d", font[cursFont + 2]);
	printf("\n\tblocks : %d", bytes);
#endif
		return cursFont;
	}
	else {
#ifdef DEBUG
		printf("\nCaractere %c non trouve dans la police.", c);
#endif
		return defFont;
	}
}

/* Calcule et retourne la longueur n�cessaire en px pour �crire une chaine donn�e
 * avec la police courante
 *
 * Attention: cette fonction pour changer la police courante
 */
WORD	calcLenOfString(char *string) {
	WORD cursFont, lenStr, loop;
	Brush brushTemp;

	lenStr = strlen(string);
	loop = 0;
	brushTemp.cursorX = 0;
	while (loop < lenStr) {
		// interprete les balise et pointe vers le texte uniquement
		while (string[loop] == CTAG) {
			loop = loop + decodeTag(string + loop, &brushTemp);
		}
		if (loop >= lenStr) break;

		// si c'est un espace on traite diff�remment
		if (string[loop] == ' ') {
			brushTemp.cursorX += getSpaceWidth();
			loop++;
			continue;
		}

		// recherche le caract�re en cours dans la police
		if ((cursFont = getCharPos(string[loop])) == 0)
			return 0;

		brushTemp.cursorX += font[cursFont + 5];
		loop++;
	}

	return brushTemp.cursorX;
}

/* Calcule et retourne la longueur n�cessaire en px pour �crire un caract�re
 * avec la police courante
 */
WORD	calcLenOfChar(char c)
{
	if (c == ' ')
		return getSpaceWidth();
	else
	{
		WORD cursFont = getCharPos(c);
		return (cursFont) ? font[cursFont + 5] : 0;
	}
}


/* Ecrit un caract�re en allant chercher son empreinte dans la police
 * puis place le curseur � la position du caract�re suivant (comme d�finit
 * dans la police).
 * La fonction retourne FALSE si la lettre n'a pas �t� trouv�e 
 * 
 */
BOOL 	writeCharacter(BYTE c, WORD *posY, WORD *posX, BOOL inverseMode) {
	// on place le curseur de parcours de la police sur le champ 'code ASCII'
	// du 1er caract�re de la police
	WORD 	cursFont;
	WORD	charPosX, charPosY;
	BYTE	charWidth, charHeight;
	BYTE	i, j;

	// recherche du caract�re dans la police
	if ((cursFont = getCharPos(c)) == 0)
		return FALSE;

	// recherche de la position de d�but d'�criture par rapport � 
	// l'origine (posY, posX)
	// A ce niveau cursFont indique le d�but de la structure du caract�re
	// dans la police.
	charPosX 	= *posX + font[cursFont + 3];
	charPosY 	= *posY + font[cursFont + 4];
	charWidth 	= font[cursFont + 1];
	charHeight 	= font[cursFont + 2];

	// si le caract�re sort de l'�cran
	if (*posX >= WIDTHPX || *posY >= HEIGHTPX || (*posX+charWidth) > WIDTHPX || (*posY+getCurrFontSize()) > HEIGHTPX)
		return FALSE;

	// si mode invers� on colore le contour
	if (inverseMode) {
		// contour sup�rieur
		setArea(*posY, *posX, charPosY - 1, *posX + charWidth - 1);
		// contour inf�rieur (entre fin du cara et la fin de la zone du caract�re)
		setArea(	*posY + charHeight + font[cursFont + 4],  /* posY + (taille r�elle + offset) */
					*posX, *posY + getCurrFontSize() - 1,
					*posX + charWidth - 1);
	}

	// on dessine chaque ligne de pixels du caract�re
	// on r�cup�re le bloc de donn�es contenant les infos du pixel en cours
	for (i = 0; i < charHeight; i++) {
		setRow(charPosY);
		for (j = 0; j < charWidth; j++) {
			// on recherche le block d'octet concern� pour extraire pixel par pixel
			BYTE byte = font[cursFont + FT_CHARH_SIZE + (i * charWidth + j) / 8];

			// si mode invers� activ�, on inverse le bit
			if (inverseMode)
				byte = ~byte;

			byte <<= (i * charWidth + j) % 8;
			if (byte & 0x80)
				//setPixel(charPosY, charPosX);
				setPixelInRow(charPosX);

			charPosX++;

		}
		// on incr�mente Y pour dessiner la ligne suivant et 
		// on ram�ne X en position initiale
		charPosY++;
		charPosX = *posX + font[cursFont + 3];
	}

	// en mode invers�, on colorie l'espace entre la fin des pixels 
	// du caract�re jusqu'� l'origine du caract�re suivant
	if (inverseMode)
		setArea(	*posY, *posX + font[cursFont + 3] + charWidth,
					*posY + getCurrFontSize() - 1, *posX + font[cursFont + 5] - 1);

	// on positione X au coordonn�es du caract�re suivant comme
	// d�finit dans la police
	*posX += font[cursFont + 5];

	return TRUE;
}
