/*
 * Module de cr�ation d'une image BMP pour GRPID � partir d'une s�quence
 * encod�e (cf doc).
 *
 *
 * Le caract�re # (CTAG) fait office de balise et permet de d�finir un formatage
 * du texte.
 * Par principe, une balise "ouverte" le restera jusqu'� lecture d'une balise
 * oppos�e. 
 * Ex : #m a pour effet d'�crire le texte en mode invers�. Tout caract�re qui
 * suit cette balise sera invers�. Il faudra alors sp�cifier #w pour revenir en
 * mode d'�criture normal.
 *
 * Par d�faut le texte sera aligne � gauche et l'�criture se fera toujours de haut
 * en bas (la zone la plus haute vers la zone la plus basse).
 * L'�criture dans chaque zone se fera de gauche � droite.
 *
 * La matrix de pixels aura pour origine la position sup�rieure gauche de l'�cran.
 *
 */


#include "stdafx.h"

#include "interpreter.h"
#include "font_tools.h"
#include "screen.h"

#ifndef EBULENT
#define DEBUG
#endif


// "pinceau" qui contient les infos d'�criture (centrage, police, ..)
static Brush 	myBrush;

// buffer de stockage des lignes de texte format�
#define MAX_BUFFER_LG	50
static char	buffer[MAX_BUFFER_LG];

// tableau de pointeur sur les diff�rentes lignes � afficher et leur format
#define MAX_SCREEN_ZONES	2
#define MAX_SCREEN_LINES	2
static char * lines[MAX_SCREEN_ZONES][MAX_SCREEN_LINES];

// nombre de zones � afficher
static BYTE zonesNbr;

// nombre de lignes � afficher dans chaque zone
static BYTE linesNbr[MAX_SCREEN_ZONES];

// police de r�f�rence dans chaque zone
static BYTE refFont[MAX_SCREEN_ZONES];

/* D�code un tag donn� et modifie la brush en cons�quence
 * Le pointeur string doit renvoie vers la balise � d�coder (ex : #p) dans la chaine.
 * On utilise un pointeur pour avoir le restant de la chaine dans le
 * cas où une balise ferait appel � des param�tres  lire suppl�mentaires (comme la police)
 *
 * On retourne la position de fin de la balise (ex, #p1 retournera 3)
 *
 */
int decodeTag(const char *string, Brush *myBrush) {
	int offset = 0;

	if (string[0] != CTAG)
		return offset;

	switch (string[1]) {
		// �criture simple
		case CNORMAL:
			myBrush->inverseWriting = FALSE;
			offset = 2;
			break;
		// �criture invers�e
		case CINVERSE:
			myBrush->inverseWriting = TRUE;
			offset = 2;
			break;
		case CINVISIBLE:
			myBrush->invisible = TRUE;
			offset = 2;
			break;
		case CVISIBLE:
			myBrush->invisible = FALSE;
			offset = 2;
			break;
		// centrage horizontal
		case CCENTERH:
			myBrush->centerText = TRUE;
			offset = 2;
			break;
		// alignement � gauche
		case CALIGNL:
			myBrush->centerText = FALSE;
			offset = 2;
			break;
		// retour � la ligne
		case CNEWLINE:
			offset = 2;
			break;
		// d�callage d'un pixel
		case C1PIXEL:
			myBrush->cursorX++;
			offset = 2;
			break;
		// changement de police
		case CFONT:
			// on v�rifie que la font est dispo puis on la s�lectionne
			if (searchFont(string[2] - '0') >= 0) {
				myBrush->fontId = string[2] - '0';
				selectFont(myBrush->fontId);
			}
#ifdef DEBUG
			else {
				printf("\nLa police selectionne est inexistante.");
			}
#endif
			offset = 3;
			break;
		case CAUTO:
			myBrush->mode = AUTO;
			offset = 2;
			break;
		default:
			break;
	}

#ifdef DEBUG
	if (!offset) {
		printf("\nLa balise n'est pas correcte.");
	}
#endif
	return offset;
}


/* V�rifie et d�code les informations contenues dans le d�but de la chaine
 * La chaine doit imp�rativement commencer par (mode affichage | id police)
 */
int parseHeader(const char *string) {
	const char delimiter_zone[] = {CTAG, CAUTO, '\0'};
	const char delimiter_line[] = {CTAG, CNEWLINE, '\0'};
	char *pc;
   	int i, j;

	clearScreen();

	// reset du "pinceau"
	memset(&myBrush, 0, sizeof(myBrush));
	
	// lecture du mode d'affichage
	if (string[0] != CTAG || string[1] != CAUTO)
		return -1;
	string += decodeTag(string, &myBrush);

	// recopie de la suite de la chaine dans le buffer de travail
	if (strlen(string) >= MAX_BUFFER_LG)
		return(-1);
	strcpy(buffer, string);

	// s�paration des zones � afficher en chaines distinctes:
	// la s�paration se fait par la recherche de CTAG|CAUTO
   	zonesNbr = 0;
   	pc = buffer;
	while (zonesNbr < MAX_SCREEN_ZONES) {
		// lecture de la police de r�f�rence pour la zone
		if (pc[0] != CTAG || pc[1] != CFONT)
		return -1;
		pc += decodeTag(pc, &myBrush);
		refFont[zonesNbr] = myBrush.fontId;
		// sauvegarde du d�but de la cha�ne pour la zone
		lines[zonesNbr++][0] = pc;
		pc = strstr(pc, delimiter_zone);
		if (pc == NULL) break;
		*pc = '\0';
		pc += strlen(delimiter_zone);
	}

	// s�paration des lignes � afficher dans chaque zone en chaines distinctes:
	// la s�paration se fait par la recherche de CTAG|CNEWLINE
	// (chaque chaine conserves ses balises de formatage)
	for (i = 0; i < zonesNbr; i++) {
		j = 0;
		pc = lines[i][0];
		while (j < MAX_SCREEN_LINES) {
			lines[i][j++] = pc;
   			pc = strstr(pc, delimiter_line);
			if (pc == NULL) break;
			*pc = '\0';
			pc += strlen(delimiter_line);
   		}
		linesNbr[i] = j;
   	}

    return (zonesNbr > 0) ? 0 : -1;
}


/* Lit les chaines de caract�res et leurs balises d'instruction
 * 
 * En mode �criture invers�es, on "colorie" la zone � plusieurs niveau :
 *	- Dans la fonction writeCharacter pour inverser la couleur du caract�re
 *	- Dans la fonction writeString pour les contours de caract�res (zones ne
	  contenant pas de pixels pour le caract�res concern�es mais �tant situ�es
	  entre l'ascendant inf�rieur et sup�rieur
 *	- Dans la fonction parseData pour les zones � colorier en dehors du texte
      (zones dues au centrage du texte par exemple)
 */
int parseData() {
	WORD i, j;

	if (myBrush.mode == AUTO) {

		BYTE currZone, currLine, heightLine;
		WORD widthZone, leftZone = BORDERX;
		
		// Construction zone par zone de gauche � droite
		for (currZone = 0; currZone < zonesNbr; currZone++) {

			// calcul de la hauteur disponible par ligne dans la zone
			heightLine = HEIGHTPX / linesNbr[currZone];

			// s�lection de la police de r�f�rence pour la zone
			myBrush.fontId = refFont[currZone];
			selectFont(myBrush.fontId);

			if (getCurrFontSize() > heightLine) {
#ifdef DEBUG
			printf("\nLa police n'est pas adaptee a l'espace.");
#endif
			return -1;
		}

			// Calcul de la largeur de la zone
			if (currZone < zonesNbr-1)
			{
				// La largeur des zones de gauche est fix�e par le texte � afficher
				widthZone = 0;
				for (currLine = 0; currLine < linesNbr[currZone]; currLine++) {
					WORD lenStr = calcLenOfString(lines[currZone][currLine]);
					if (widthZone < lenStr)
						widthZone = lenStr;
				}
				// Restauration de la police de r�f�rence pour la zone
				selectFont(myBrush.fontId);
			}
			else
				// La derni�re zone occupe tout l'espace restant
				widthZone = WIDTHPX - leftZone;

			// On construit ligne par ligne de haut en bas
			for (currLine = 0; currLine < linesNbr[currZone]; currLine++) {

				char *pc;

				myBrush.areaYStart 	= heightLine * currLine;
				myBrush.areaYEnd 	= myBrush.areaYStart + heightLine - 1;

				// lecture du mode d'alignement �ventuel (centr� ou align� � gauche)
				// cette balise est obligatoirement pr�sente en d�but de sous chaine
				pc = lines[currZone][currLine];
				myBrush.cursorX = 0;
				while (pc[0] == CTAG) {
					int offset = decodeTag(pc, &myBrush);
					if (!offset || myBrush.cursorX > 0)
					break;
					pc += offset;
				}
			
				// calcul de l'alignement horizontal initial
				if (myBrush.centerText) {
					int lenStr = calcLenOfString(pc);

					// Restauration de la police
					selectFont(myBrush.fontId);

				// si mode invers�, on colorie les zones hors du texte pour 
				// ne pas laisser de blanc autour du texte centr�
				if (lenStr >= 0) {
						myBrush.cursorX = widthZone / 2 - lenStr / 2 - 1;

					if (myBrush.inverseWriting) {
						for (i = myBrush.areaYStart; i <= myBrush.areaYEnd; i++) {
							setRow(i);
							for (j = 0; j < WIDTHPX; j++) {
								// si on est en dehors du texte, on colorie
								if (j < myBrush.cursorX || j > myBrush.cursorX + lenStr - 1)
									//setPixel(i, j);
									setPixelInRow(j);
							}
						}
					}
				}
				else
					return -1;
			}
			else
					myBrush.cursorX = leftZone;

			// "dessine" la ligne
				writeString(pc, heightLine);
#ifdef DEBUG
			printf("\nLigne %d ecrite.", currLine);
#endif
		}

			// calcul du bord gauche de la zone suivante
			leftZone += widthZone;
		}
	}
	return 0;
}


/* Ecrit un chaine dans la matrice de l'�cran
 * A ce niveau myBrush contient les coordonn�es du curseur
 */
int	writeString(const char *string, BYTE heightLine) {
	WORD	lenStr, loop, absoluteCenterY;

	selectFont(myBrush.fontId);

	lenStr = strlen(string);
	loop = 0;
	while (loop < lenStr) {
		myBrush.prevCursorX = myBrush.cursorX;

		while (string[loop] == CTAG) {
			int offset = decodeTag(string + loop, &myBrush);
			if (!offset)
				break;
			loop = loop + offset;
		}
		if (loop >= lenStr) break;

		/* calcul de l'origine verticale (y),
		 * l'�criture se fait de haut en bas ; pour trouver l'origine sur 
		 * une zone (HEIGHTPX/linesNbr), on recherche le centre de cette zone 
		 * auquel on soustrait la moiti� de la taille de la police. Ceci pour 
		 * faire coincider le centre de la zone et le centre de l'ascendant et donc
		 * centrer chaque �criture par rapport � sa zone. On optimise ainsi l'espace
		 * et les changements de police dans une même zone.
		 * 
		 * On �crira le caract�re de haut en bas, l'origine se trouvant dans le 
		 * coin sup�rieur gauche
		 */

		absoluteCenterY = heightLine / 2 - getFontSize(myBrush.fontId) / 2;

		myBrush.cursorY = absoluteCenterY + myBrush.areaYStart + BORDERY;

		if (string[loop] == ' ')		
			myBrush.cursorX += getSpaceWidth();

		else if (myBrush.invisible)
			myBrush.cursorX += calcLenOfChar(string[loop]);

		// si une balise a d�cal� le curseur et qu'on est en position invers�e, on colorie
		if (myBrush.cursorX - myBrush.prevCursorX > 0 && myBrush.inverseWriting)
			setArea(	myBrush.areaYStart, myBrush.prevCursorX, \
						myBrush.areaYEnd, myBrush.cursorX -1);

		if (string[loop] != ' ' && myBrush.invisible != TRUE) 
			writeCharacter(string[loop], &myBrush.cursorY, &myBrush.cursorX, myBrush.inverseWriting);

		// si mode invers�, on colorie le contour exterieur du caract�re
		if (myBrush.inverseWriting && myBrush.cursorX > 0) {

			setArea(myBrush.areaYStart, myBrush.prevCursorX, myBrush.cursorY - 1, myBrush.cursorX - 1);

			setArea(	myBrush.cursorY + getCurrFontSize(), myBrush.prevCursorX, \
						myBrush.areaYEnd, myBrush.cursorX - 1);
		}
		loop++;
	}

	return 0;
}

#ifdef CONVERTTOBMP

// passage de la chaine de caract�re en param�tre
int main(int argc, char* argv[]) {
	char *myString;
	int offset = 0;

	
	if (argc > 1)
		myString = argv[1];
	else
		return -1;
	
	// lit le header de la chaine de commande
	offset = parseHeader(myString);

	if (offset < 0) {
#ifdef DEBUG
		printf("\nUne erreur est survenue dans la lecture du header\n.");
#endif
		return -1;
	}

	// parse la chaine re�ue et contenant les informations de codage + caract�res
	if (parseData() != 0) {
#ifdef DEBUG
		printf("\nUne erreur est survenue dans le traitement de la chaine\n.");
#endif
		return -1;
	}

	// cr�er l'image BMP : module temporaire
	convertToBmp();
	
	return 0;
}

#endif
