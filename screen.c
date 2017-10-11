#include "stdafx.h"
#include "screen.h"

#ifndef EBULENT
#define DEBUG
#endif

// buffer de construction d'une image 132x64
BYTE	matrix[MATRIX_SZ];

// met un pixel en noir
//void	setPixel(WORD y, WORD x) {
//	if (y < HEIGHTPX && x < WIDTHPX) {
//	    #ifdef EBULENT
//			WORD page = y / 8;					// numéro de la page sur l'écran Kent/Ebulent
//			WORD block = page * WIDTHPX + x;	// numéro de l'octet
//			BYTE c = 0x01 << (y % 8);
//		    matrix[block] &= ~c;
//	    #else
//		    WORD pos = y * WIDTHPX + x;
//		    WORD block = pos / 8;
//		    BYTE c = 0x01 << (7 - pos % 8);
//		    matrix[block] |= c;
//	    #endif
//	}
//#ifdef DEBUG
//	else {
//		printf("\n***tentative d'ecriture hors zone.");
//		return;
//	}
//#endif
//}


WORD startBlock;
BYTE pixelMask;

void	setRow(WORD y) {
	//if (y >= HEIGHTPX) return;
	#ifdef EBULENT
		startBlock = (y >> 3) * WIDTHPX;
		pixelMask = 0x01 << (y & 0x7);
		pixelMask = ~pixelMask;
	#else
		startBlock = y * WIDTHPX;
	#endif
}

void	setPixelInRow(WORD x) {
	//if (x >= WIDTHPX) return;
	#ifdef EBULENT
		matrix[startBlock + x] &= pixelMask;
	#else
		WORD pos = startBlock + x;
		WORD block = pos / 8;
		pixelMask = 0x01 << (7 - pos % 8);
		matrix[block] |= pixelMask;
	#endif
}


void	clearScreen() {
	#ifdef EBULENT
		memset(matrix, 0xff, MATRIX_SZ);
	#else
		memset(matrix, 0, MATRIX_SZ);
	#endif
}


// Colore une zone rectangulaire de (Y1, X1) à  (Y2, X2) compris
void	setArea(WORD y1, WORD x1, WORD y2, WORD x2) {
	WORD i, j;

	for (i = y1; i <= y2; i++) {
		setRow(i);
		for (j = x1; j <= x2; j++)
			//setPixel(i, j);
			setPixelInRow(j);
	}
}

