/*
 * Module de création d'une image BMP pour GRPID à partir d'une séquence
 * encodée (cf doc).
 *
 *
 * Le caractère # (CTAG) fait office de balise et permet de définir un formatage
 * du texte.
 * Par principe, une balise "ouverte" le restera jusqu'à lecture d'une balise
 * opposée. 
 * Ex : #m a pour effet d'écrire le texte en mode inversé. Tout caractère qui
 * suit cette balise sera inversé. Il faudra alors spécifier #w pour revenir en
 * mode d'écriture normal.
 *
 * Par défaut le texte sera aligne à gauche et l'écriture se fera toujours de haut
 * en bas (la zone la plus haute vers la zone la plus basse).
 * L'écriture dans chaque zone se fera de gauche à droite.
 *
 * La matrix de pixels aura pour origine la position supérieure gauche de l'écran.
 *
 */


#include "stdafx.h"

#include "interpreter.h"
#include "font_tools.h"
#include "screen.h"

#ifndef EBULENT
#define DEBUG
#endif


// "pinceau" qui contient les infos d'écriture (centrage, police, ..)
static Brush 	myBrush;

// buffer de stockage des lignes de texte formaté
#define MAX_BUFFER_LG	50
static char	buffer[MAX_BUFFER_LG];

// tableau de pointeur sur les différentes lignes à afficher et leur format
#define MAX_SCREEN_ZONES	2
#define MAX_SCREEN_LINES	2
static char * lines[MAX_SCREEN_ZONES][MAX_SCREEN_LINES];

// nombre de zones à afficher
static BYTE zonesNbr;

// nombre de lignes à afficher dans chaque zone
static BYTE linesNbr[MAX_SCREEN_ZONES];

// police de référence dans chaque zone
static BYTE refFont[MAX_SCREEN_ZONES];

/* Décode un tag donné et modifie la brush en conséquence
 * Le pointeur string doit renvoie vers la balise à décoder (ex : #p) dans la chaine.
 * On utilise un pointeur pour avoir le restant de la chaine dans le
 * cas oÃ¹ une balise ferait appel à des paramètres  lire supplémentaires (comme la police)
 *
 * On retourne la position de fin de la balise (ex, #p1 retournera 3)
 *
 */
int decodeTag(const char *string, Brush *myBrush) {
	int offset = 0;

	if (string[0] != CTAG)
		return offset;

	switch (string[1]) {
		// écriture simple
		case CNORMAL:
			myBrush->inverseWriting = FALSE;
			offset = 2;
			break;
		// écriture inversée
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
		// alignement à gauche
		case CALIGNL:
			myBrush->centerText = FALSE;
			offset = 2;
			break;
		// retour à la ligne
		case CNEWLINE:
			offset = 2;
			break;
		// décallage d'un pixel
		case C1PIXEL:
			myBrush->cursorX++;
			offset = 2;
			break;
		// changement de police
		case CFONT:
			// on vérifie que la font est dispo puis on la sélectionne
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


/* Vérifie et décode les informations contenues dans le début de la chaine
 * La chaine doit impérativement commencer par (mode affichage | id police)
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

	// séparation des zones à afficher en chaines distinctes:
	// la séparation se fait par la recherche de CTAG|CAUTO
   	zonesNbr = 0;
   	pc = buffer;
	while (zonesNbr < MAX_SCREEN_ZONES) {
		// lecture de la police de référence pour la zone
		if (pc[0] != CTAG || pc[1] != CFONT)
		return -1;
		pc += decodeTag(pc, &myBrush);
		refFont[zonesNbr] = myBrush.fontId;
		// sauvegarde du début de la chaîne pour la zone
		lines[zonesNbr++][0] = pc;
		pc = strstr(pc, delimiter_zone);
		if (pc == NULL) break;
		*pc = '\0';
		pc += strlen(delimiter_zone);
	}

	// séparation des lignes à afficher dans chaque zone en chaines distinctes:
	// la séparation se fait par la recherche de CTAG|CNEWLINE
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


/* Lit les chaines de caractères et leurs balises d'instruction
 * 
 * En mode écriture inversées, on "colorie" la zone à plusieurs niveau :
 *	- Dans la fonction writeCharacter pour inverser la couleur du caractère
 *	- Dans la fonction writeString pour les contours de caractères (zones ne
	  contenant pas de pixels pour le caractères concernées mais étant situées
	  entre l'ascendant inférieur et supérieur
 *	- Dans la fonction parseData pour les zones à colorier en dehors du texte
      (zones dues au centrage du texte par exemple)
 */
int parseData() {
	WORD i, j;

	if (myBrush.mode == AUTO) {

		BYTE currZone, currLine, heightLine;
		WORD widthZone, leftZone = BORDERX;
		
		// Construction zone par zone de gauche à droite
		for (currZone = 0; currZone < zonesNbr; currZone++) {

			// calcul de la hauteur disponible par ligne dans la zone
			heightLine = HEIGHTPX / linesNbr[currZone];

			// sélection de la police de référence pour la zone
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
				// La largeur des zones de gauche est fixée par le texte à afficher
				widthZone = 0;
				for (currLine = 0; currLine < linesNbr[currZone]; currLine++) {
					WORD lenStr = calcLenOfString(lines[currZone][currLine]);
					if (widthZone < lenStr)
						widthZone = lenStr;
				}
				// Restauration de la police de référence pour la zone
				selectFont(myBrush.fontId);
			}
			else
				// La dernière zone occupe tout l'espace restant
				widthZone = WIDTHPX - leftZone;

			// On construit ligne par ligne de haut en bas
			for (currLine = 0; currLine < linesNbr[currZone]; currLine++) {

				char *pc;

				myBrush.areaYStart 	= heightLine * currLine;
				myBrush.areaYEnd 	= myBrush.areaYStart + heightLine - 1;

				// lecture du mode d'alignement éventuel (centré ou aligné à gauche)
				// cette balise est obligatoirement présente en début de sous chaine
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

				// si mode inversé, on colorie les zones hors du texte pour 
				// ne pas laisser de blanc autour du texte centré
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


/* Ecrit un chaine dans la matrice de l'écran
 * A ce niveau myBrush contient les coordonnées du curseur
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
		 * l'écriture se fait de haut en bas ; pour trouver l'origine sur 
		 * une zone (HEIGHTPX/linesNbr), on recherche le centre de cette zone 
		 * auquel on soustrait la moitié de la taille de la police. Ceci pour 
		 * faire coincider le centre de la zone et le centre de l'ascendant et donc
		 * centrer chaque écriture par rapport à sa zone. On optimise ainsi l'espace
		 * et les changements de police dans une mÃªme zone.
		 * 
		 * On écrira le caractère de haut en bas, l'origine se trouvant dans le 
		 * coin supérieur gauche
		 */

		absoluteCenterY = heightLine / 2 - getFontSize(myBrush.fontId) / 2;

		myBrush.cursorY = absoluteCenterY + myBrush.areaYStart + BORDERY;

		if (string[loop] == ' ')		
			myBrush.cursorX += getSpaceWidth();

		else if (myBrush.invisible)
			myBrush.cursorX += calcLenOfChar(string[loop]);

		// si une balise a décalé le curseur et qu'on est en position inversée, on colorie
		if (myBrush.cursorX - myBrush.prevCursorX > 0 && myBrush.inverseWriting)
			setArea(	myBrush.areaYStart, myBrush.prevCursorX, \
						myBrush.areaYEnd, myBrush.cursorX -1);

		if (string[loop] != ' ' && myBrush.invisible != TRUE) 
			writeCharacter(string[loop], &myBrush.cursorY, &myBrush.cursorX, myBrush.inverseWriting);

		// si mode inversé, on colorie le contour exterieur du caractère
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

// passage de la chaine de caractère en paramètre
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

	// parse la chaine reçue et contenant les informations de codage + caractères
	if (parseData() != 0) {
#ifdef DEBUG
		printf("\nUne erreur est survenue dans le traitement de la chaine\n.");
#endif
		return -1;
	}

	// créer l'image BMP : module temporaire
	convertToBmp();
	
	return 0;
}

#endif
