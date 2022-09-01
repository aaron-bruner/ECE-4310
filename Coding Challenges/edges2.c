
/*
** Demonstrates how to calculate a simple gradient in the
** other direction.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(int argc, char* argv[])

{
	FILE* fpt;
	unsigned char* image, * output, * output_horizontal, * output_vertical;
	char		header[80];
	int		ROWS, COLS, BYTES;
	int		i, j, b, r, c;

	/* tell user how to use program if incorrect arguments */
	if (argc != 3)
	{
		printf("Usage:  edges2 [filename] [amount]\n");
		exit(0);
	}
	b = atoi(argv[2]);
	if (b < 0 || b > 255)
	{
		printf("amount must be 0...255\n");
		exit(0);
	}
	/* open image for reading */
	fpt = fopen(argv[1], "rb");
	if (fpt == NULL)
	{
		printf("Unable to open %s for reading\n", argv[1]);
		exit(0);
	}
	/* read image header (simple 8-bit greyscale PPM only) */
	i = fscanf(fpt, "%s %d %d %d ", header, &COLS, &ROWS, &BYTES);
	if (i != 4 || strcmp(header, "P5") != 0 || BYTES != 255)
	{
		printf("%s is not an 8-bit PPM greyscale (P5) image\n", argv[1]);
		fclose(fpt);
		exit(0);
	}
	/* allocate dynamic memory for image */
	image = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
	if (image == NULL)
	{
		printf("Unable to allocate %d x %d memory\n", COLS, ROWS);
		exit(0);
	}
	/* read image data from file */
	fread(image, 1, ROWS * COLS, fpt);
	fclose(fpt);

	/* find edges by testing single pixel gradient */
	output_horizontal = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
	output_vertical = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
	output = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
	for (r = 0; r < ROWS; r++)
	{
		for (c = 0; c < COLS - 1; c++)
		{
			if (abs((int)(image[r * COLS + c + 1]) - (int)(image[r * COLS + c])) > b)
				output_vertical[r * COLS + c] = 0;	/* this is edge */
			else
				output_vertical[r * COLS + c] = 255;	/* this is not an edge */
		}
	}
	for (r = 0; r < ROWS; r++)
	{
		for (c = 0; c < COLS - 1; c++)
		{
			if (abs((int)(image[r * COLS + c + 1]) - (int)(image[r * COLS + c])) > b)
				output_horizontal[r * COLS + c] = 0;	/* this is edge */
			else
				output_horizontal[r * COLS + c] = 255;	/* this is not an edge */
		}
	}

	for (r = 0; r < ROWS; r++)
	{
		for (c = 0; c < COLS - 1; c++)
		{
			output[r * COLS + c] = (unsigned char)sqrt((double)output_vertical[r * COLS + c]* (double)output_vertical[r * COLS + c] + (double)output_horizontal[r * COLS + c]* (double)output_horizontal[r * COLS + c]);
		}
	}

	/* write out edges2.ppm, the output result */
	fpt = fopen("edges2.ppm", "wb");
	if (fpt == NULL)
	{
		printf("Unable to open edges2.ppm for writing\n");
		exit(0);
	}
	fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
	fwrite(output, 1, ROWS * COLS, fpt);
	fclose(fpt);
}
