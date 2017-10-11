#ifndef SCREEN_H
#define SCREEN_H

#include "stdafx.h"

#ifdef EBULENT		// Ecran EBULENT sur PIC24F
// - codage ordonné par groupes de 8 lignes (page) puis par ligne, depuis le coin (0,0) en haut à gauche jusqu'au coin (131,63) en bas à droite
// - dans chaque page, 1 colonne de 8 bits = 1 octet, codage des bits dans le sens b7->b0.
// - matrix[0].b0 = pixel(0,0), matrix[0].b7 = pixel(7,0), matrix[1].b0 = pixel(0,1), matrix[131].b7 = pixel(7,131), matrix[132].b0 = pixel (8,0)...
// - les pixels noirs sont codés comme des bits à 0 pour les écrans Kent ou Ebulent

#else				// BMP sur PC Windows
// - codage ordonné par ligne puis par colonne, depuis le coin (0,0) en haut à gauche jusqu'au coin (131,63) en bas à droite
// - matrix[0].b7 = pixel(0,0), matrix[0].b0 = pixel(0,7), matrix[1].b7 = pixel(0,8)...
// - les pixels noirs sont codés comme des bits à 1
#endif

// configuration écran en px
#define WIDTHPX 	132U
#define HEIGHTPX  	64U

// buffer de construction d'une image 132x64
#define MATRIX_SZ	(WIDTHPX * HEIGHTPX / 8)
extern BYTE matrix[MATRIX_SZ];

//void 	setPixel(WORD y, WORD x);
void	setRow(WORD y);
void	setPixelInRow(WORD x);
void	setArea(WORD y1, WORD x1, WORD y2, WORD x2);
void	clearScreen();

#endif