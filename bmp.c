#include "stdafx.h"
#include "screen.h"

void convertToBmp() {
	
	RGBTRIPLE 	image[WIDTHPX * HEIGHTPX];	// A SUPPRIMER
	RGBTRIPLE	imageDbl[HEIGHTPX][WIDTHPX];
	int i, j, nbrCount;
	FILE *dest = fopen("output.bmp", "wb");


	//BM
    BITMAPFILEHEADER    bmfh;  
    BITMAPINFOHEADER    bmih;  

    ((unsigned char *)&bmfh.bfType)[0] = 'B';
    ((unsigned char *)&bmfh.bfType)[1] = 'M';
    bmfh.bfSize = 54 + HEIGHTPX * WIDTHPX * 3;
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;
    bmfh.bfOffBits = 54;

    bmih.biSize = 40;
    bmih.biWidth = 132;
    bmih.biHeight = 64;
    bmih.biPlanes = 1;
    bmih.biBitCount = 24;
    bmih.biCompression = 0;
    bmih.biSizeImage = 0;
    bmih.biXPelsPerMeter = 0;
    bmih.biYPelsPerMeter = 0;
    bmih.biClrUsed = 0;
    bmih.biClrImportant = 0;


    nbrCount = 0;

    for (i = 0; i < WIDTHPX * HEIGHTPX / 8; i++) {
    	// décomposition bit par bit
    	// on transpose l'image
    	for (j = 0; j < 8; j++) {
    		if (((matrix[i] << j) & 0x80)) {
    			imageDbl[63 - nbrCount / 132][nbrCount % 132].rgbtBlue = 0;
    			imageDbl[63 - nbrCount / 132][nbrCount % 132].rgbtGreen = 0;
    			imageDbl[63 - nbrCount / 132][nbrCount % 132].rgbtRed = 0;
    		}
    		else {
    			imageDbl[63 - nbrCount / 132][nbrCount % 132].rgbtBlue = 255;
    			imageDbl[63 - nbrCount / 132][nbrCount % 132].rgbtGreen = 255;
    			imageDbl[63 - nbrCount / 132][nbrCount % 132].rgbtRed = 255;
    		}

			nbrCount++;
    	}
    }

    nbrCount = 0;
    for (i = 0; i < 64; i++)
    	for (j = 0; j < 132; j++)
    		image[nbrCount++] = imageDbl[i][j];

    fwrite(&bmfh, sizeof(BITMAPFILEHEADER), 1, dest);
    fwrite(&bmih, sizeof(BITMAPINFOHEADER), 1, dest);
    fwrite(image, sizeof(RGBTRIPLE), WIDTHPX * HEIGHTPX, dest);
	fclose(dest);
}

