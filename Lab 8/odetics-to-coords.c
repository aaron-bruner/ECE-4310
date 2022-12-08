#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define ROWS	128
#define COLS	128
#define RangeImageSourceDir "chair-range.ppm"

/*
**	This routine converts the data in an Odetics range image into 3D
**	cartesian coordinate data.  The range image is 8-bit, and comes
**	already separated from the intensity image.
*/

void outputImage(unsigned char* source, char* fileName, int col, int row);
unsigned char* readImage(int* COLS, int* ROWS, char* source);
unsigned char* createImage(int size);

void main(int argc, char* argv[])
{
	int	r, c;
	double cp[7];
	double xangle, yangle, dist;
	double ScanDirectionFlag, SlantCorrection;
	unsigned char RangeImage[128 * 128];
	double P[3][128 * 128];
	int ImageTypeFlag;
	char Filename[160], Outfile[160];
	FILE* fpt;

	RangeImage = argc == 1 ? readImage(COLS, ROWS, RangeImageSourceDir) : exit(0);
	ScanDirectionFlag = 1; // The slant type can be assumed to be scan direction downward

	cp[0] = 1220.7;             /* horizontal mirror angular velocity in rpm */
	cp[1] = 32.0;               /* scan time per single pixel in microseconds */
	cp[2] = (COLS / 2) - 0.5;   /* middle value of columns */
	cp[3] = 1220.7 / 192.0;     /* vertical mirror angular velocity in rpm */
	cp[4] = 6.14;               /* scan time (with retrace) per line in milliseconds */
	cp[5] = (ROWS / 2) - 0.5;   /* middle value of rows */
	cp[6] = 10.0;               /* standoff distance in range units (3.66cm per r.u.) */

	cp[0] = cp[0] * 3.1415927 / 30.0;   /* convert rpm to rad/sec */
	cp[3] = cp[3] * 3.1415927 / 30.0;   /* convert rpm to rad/sec */
	cp[0] = 2.0 * cp[0];                /* beam ang. vel. is twice mirror ang. vel. */
	cp[3] = 2.0 * cp[3];                /* beam ang. vel. is twice mirror ang. vel. */
	cp[1] /= 1000000.0;                 /* units are microseconds : 10^-6 */
	cp[4] /= 1000.0;                    /* units are milliseconds : 10^-3 */

	/*  start with semi-spherical coordinates from laser-range-finder:  */
	/*              (r,c,RangeImage[r*COLS+c])                          */
	/*  convert those to axis-independant spherical coordinates:        */
	/*                  (xangle,yangle,dist)                            */
	/*  then convert the spherical coordinates to cartesian:            */
	/*                  (P => X[] Y[] Z[])                              */

	if (ImageTypeFlag != 3)
	{
		for (r = 0; r < ROWS; r++)
		{
			for (c = 0; c < COLS; c++)
			{
				SlantCorrection = cp[3] * cp[1] * ((double)c - cp[2]);
				xangle = cp[0] * cp[1] * ((double)c - cp[2]);
				yangle = (cp[3] * cp[4] * (cp[5] - (double)r)) +	/* Standard Transform Part */
					SlantCorrection * ScanDirectionFlag;			/*  + slant correction */
				dist = (double)RangeImage[r * COLS + c] + cp[6];
				P[2][r * COLS + c] = sqrt((dist * dist) / (1.0 + (tan(xangle) * tan(xangle))
					+ (tan(yangle) * tan(yangle))));
				P[0][r * COLS + c] = tan(xangle) * P[2][r * COLS + c];
				P[1][r * COLS + c] = tan(yangle) * P[2][r * COLS + c];
			}
		}
	}

	/*----------------------------------------------------------------------------------------*/
	// The image should first be masked by thresholding at a distance that removes the
	// background and leaves only the floor and the chair.Specify the threshold chosen in your
	// report.
	unsigned char* threasholdImage[ROWS * COLS];
	/*----------------------------------------------------------------------------------------*/

	sprintf(Outfile, "%s.coords", Filename);
	fpt = fopen(Outfile, "w");
	fwrite(P[0], 8, 128 * 128, fpt);
	fwrite(P[1], 8, 128 * 128, fpt);
	fwrite(P[2], 8, 128 * 128, fpt);
	fclose(fpt);
}

/// <summary>
/// The readImage function is designed to take a file name as the source and reads all of the data into a new image.
/// </summary>
/// <param name="ROWS"> Number of rows in the source image </param>
/// <param name="COLS"> Number of columns in the source image </param>
/// <param name="source"> File name that we're needing to open and read data from </param>
/// <returns> The function returns an array of values which makes up our image </returns>
unsigned char* readImage(int* COLS, int* ROWS, char* source)
{
	int BYTES, readHeaderReturn;
	static char header[80];

	// Open image for reading
	FILE* fpt = fopen(source, "rb");
	if (fpt == NULL) {
		printf("Failed to open file (%s) for reading.\n", source);
		exit(0);
	}

	/* read image header (simple 8-bit greyscale PPM only) */
	if (fscanf(fpt, "%s %d %d %d\n", header, &*COLS, &*ROWS, &BYTES) != 4 || strcmp(header, "P5") != 0 || BYTES != 255)
	{
		fclose(fpt);
		printf("Image header corrupted.\n");
		exit(0);
	}

	// Create an empty image that is large enough for ROWS x COLS bytes
	unsigned char* destination = createImage((*ROWS) * (*COLS));

	fread(destination, 1, (*ROWS) * (*COLS), fpt);
	fclose(fpt);

	return destination;
}

/// <summary>
/// Output the image to the fileName provided.
/// </summary>
/// <param name="source">The image needing to be output to the directory fileName</param>
/// <param name="fileName">Directory where the image needs to be printed to</param>
/// <param name="col">Number of columns in source image</param>
/// <param name="row">Number of rows in source image</param>
void outputImage(unsigned char* source, char* fileName, int col, int row)
{

	FILE* FPT = fopen(fileName, "w"); FPT == NULL ? printf("Unable to open %s for writing.\n", source), exit(0) : false;
	fprintf(FPT, "P5 %d %d 255\n", col, row);
	fwrite(source, col * row, 1, FPT);
	fclose(FPT);

	return;
}

/// <summary>
/// createImage allocates memory for our image array.
/// </summary>
/// <param name="size"> Number of bytes that are needing to be allocated for our image </param>
/// <returns> An array with 'size' number of bytes allocated for our image use</returns>
unsigned char* createImage(int size)
{
	unsigned char* newImage = (unsigned char*)calloc(size, sizeof(unsigned char));
	if (newImage == NULL) {
		printf("Unable to allocate %d bytes of memory.\n", size);
		exit(0);
	}

	return newImage;
}