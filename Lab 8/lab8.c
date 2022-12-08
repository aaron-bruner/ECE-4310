#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define x 0
#define y 1
#define z 2
#define true 1
#define false 0
#define ROWS	128
#define COLS	128
#define RangeImageSourceDir "chair-range.ppm"
#define SQR(x) ((x)*(x))

/*
**	This routine converts the data in an Odetics range image into 3D
**	cartesian coordinate data.  The range image is 8-bit, and comes
**	already separated from the intensity image.
*/

void outputImage(unsigned char* source, char* fileName, int col, int row);
unsigned char* readImage(int* cols, int* rows, char* source);
unsigned char* createImage(int size);

void main(int argc, char* argv[])
{
	int	r, c, row, col, index = 0, distance;
	double cp[7];
	double	x0, y0, z0, x1, y1, z1, x2, y2, z2;
	double	ax, ay, az, bx, by, bz, cx, cy, cz;
	double xangle, yangle, dist;
	double ScanDirectionFlag, SlantCorrection;
	unsigned char *RangeImage/*[128 * 128]*/, *thresholdImage;
	double P[3][128 * 128];
	double C[3][128 * 128];
	int ImageTypeFlag;
	char Filename[160], Outfile[160];
	int* indices, i, j, seed;
	int RegionSize, /** RegionPixels,*/ TotalRegions;
	double avg, var;
	FILE* fpt;

	RangeImage = readImage(&c, &r, RangeImageSourceDir);
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
				P[z][r * COLS + c] = sqrt((dist * dist) / (1.0 + (tan(xangle) * tan(xangle))
					+ (tan(yangle) * tan(yangle))));
				P[x][r * COLS + c] = tan(xangle) * P[z][r * COLS + c];
				P[y][r * COLS + c] = tan(yangle) * P[z][r * COLS + c];
			}
		}
	}

	/*----------------------------------------------------------------------------------------*/
	// The image should first be masked by thresholding at a distance that removes the
	// background and leaves only the floor and the chair.Specify the threshold chosen in your
	// report.
	thresholdImage = createImage(ROWS*COLS);
	for (index = 0; index < ROWS*COLS; index++) thresholdImage[index] = RangeImage[index];
	for (index = 0; index < ROWS*COLS; index++) thresholdImage[index] = thresholdImage[index] > 128 ? 0 : 255;
	outputImage(thresholdImage, "output-threshold.ppm", COLS, ROWS);
	/*----------------------------------------------------------------------------------------*/

	/*----------------------------------------------------------------------------------------*/
	// Surface normals should be calculated using the cross product method as discussed in
	// class
	distance = 4;
	for (row = 0; row < ROWS - distance; row++)
	{
		for (col = 0; col < COLS - distance; col++)
		{
			//Create a and b vector for cross product
			ax = P[x][row * COLS + (col + distance)] - P[0][row * COLS + col];
			ay = P[y][row * COLS + (col + distance)] - P[1][row * COLS + col];
			az = P[z][row * COLS + (col + distance)] - P[2][row * COLS + col];

			bx = P[x][(row + distance) * COLS + col] - P[0][row * COLS + col];
			by = P[y][(row + distance) * COLS + col] - P[1][row * COLS + col];
			bz = P[z][(row + distance) * COLS + col] - P[2][row * COLS + col];

			C[x][row * COLS + col] = ay * bz - az * by;
			C[y][row * COLS + col] = az * bx - ax * bz;
			C[z][row * COLS + col] = ax * by - ay * bx;
		}
	}
	/*----------------------------------------------------------------------------------------*/

	/*----------------------------------------------------------------------------------------*/
	//Region growing should be used to segment regions, using the queue - based C code
	//previously provided.The region predicate should be that a pixel can join the region if its
	//orientation is within a threshold of the average orientation of pixels already in the region.
	//The angular difference should be calculated using the dot product.The region growing
	//code must be modified to recalculate the average after every new pixel joins the region.
	//Specify the angle threshold chosen in your report.You may take advantage of the fact
	//that for this image, the surfaces all have strong orientation differences.
	//
	//Seed pixels for region growing should be found by identifying a complete 5x5 window of
	//unlabeled(and not masked out in the first step) of still - unlabeled region.If any pixel
	//within the 5x5 window is masked out or already labeled in a region, then the pixel cannot
	//seed a new region.Region growing ends when there are no more possible seed pixels.
	for (i = 2; i < ROWS - 2; i++)
	{
		for (j = 2; j < COLS - 2; j++, seed = false)
		{
			for (row = -2; row <= 2; row++)
			{
				for (col = -2; col <= 2; col++)
				{
					if ((thresholdImage[(i + row) * COLS + (j + col)] == 255) || Outfile[(i + row) * COLS + (j + col)] != 0)
					{
						seed = true;
					}
				}
			}
			if (!seed)
			{
				TotalRegions += 30;
				RegionGrow(RangeImage, Outfile, ROWS, COLS, i, j, 0, TotalRegions, indices, &RegionSize, C);
				if (RegionSize < 100)
				{
					for (int k = 0; k < RegionSize; k++)
					{
						Outfile[indices[k]] = 0;
					}
					TotalRegions -= 30;
				}
				else
				{
					printf("Region Number: %d \t Number of Pixels: %d\n", (TotalRegions / 30) - 1, RegionSize);
				}
			}

		}
	}
	/*----------------------------------------------------------------------------------------*/

	//sprintf(Outfile, "%s.coords", Filename);
	//fpt = fopen(Outfile, "w");
	//fwrite(P[0], 8, 128 * 128, fpt);
	//fwrite(P[1], 8, 128 * 128, fpt);
	//fwrite(P[2], 8, 128 * 128, fpt);
	//fclose(fpt);
}

/// <summary>
/// The readImage function is designed to take a file name as the source and reads all of the data into a new image.
/// </summary>
/// <param name="ROWS"> Number of rows in the source image </param>
/// <param name="COLS"> Number of columns in the source image </param>
/// <param name="source"> File name that we're needing to open and read data from </param>
/// <returns> The function returns an array of values which makes up our image </returns>
unsigned char* readImage(int* cols, int* rows, char* source)
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
	if (fscanf(fpt, "%s %d %d %d\n", header, &*cols, &*rows, &BYTES) != 4 || strcmp(header, "P5") != 0 || BYTES != 255)
	{
		fclose(fpt);
		printf("Image header corrupted.\n");
		exit(0);
	}

	// Create an empty image that is large enough for ROWS x COLS bytes
	unsigned char* destination = createImage((*rows) * (*cols));

	fread(destination, 1, (*rows) * (*cols), fpt);
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

/*
** Given an image, a starting point, and a label, this routine
** paint-fills (8-connected) the area with the given new label
** according to the given criteria (pixels close to the average
** intensity of the growing region are allowed to join).
*/

#define MAX_QUEUE 10000	/* max perimeter size (pixels) of border wavefront */

void RegionGrow(unsigned char* image,	/* image data */
	unsigned char* labels,	/* segmentation labels */
	int rows, int cols,	/* size of image */
	int r, int c,		/* pixel to paint from */
	int paint_over_label,	/* image label to paint over */
	int new_label,		/* image label for painting */
	int* indices,		/* output:  indices of pixels painted */
	int* count,		/* output:  count of pixels painted */
	double** C)
{
	int	r2, c2;
	int	queue[MAX_QUEUE] = 0, qh, qt;
	int	average[3], total[3];	/* average and total intensity in growing region */

	*count = 0;
	if (labels[r * cols + c] != paint_over_label)
		return;
	labels[r * cols + c] = new_label;
	
	// Need to have an average and total for X, Y and Z
	//average = total = (int)image[r * cols + c];
	average[x] = total[x] = (int)C[x][r * cols + c];
	average[y] = total[y] = (int)C[y][r * cols + c];
	average[z] = total[z] = (int)C[z][r * cols + c];

	if (indices != NULL)
		indices[0] = r * cols + c;
	queue[0] = r * cols + c;
	qh = 1;	/* queue head */
	qt = 0;	/* queue tail */
	(*count) = 1;
	while (qt != qh)
	{
		for (r2 = -1; r2 <= 1; r2++)
			for (c2 = -1; c2 <= 1; c2++)
			{
				if (r2 == 0 && c2 == 0)
					continue;
				if ((queue[qt] / cols + r2) < 0 || (queue[qt] / cols + r2) >= rows ||
					(queue[qt] % cols + c2) < 0 || (queue[qt] % cols + c2) >= cols)
					continue;
				if (labels[(queue[qt] / cols + r2) * cols + queue[qt] % cols + c2] != paint_over_label)
					continue;

				/* test criteria to join region */
				if (abs((int)(image[(queue[qt] / cols + r2) * cols + queue[qt] % cols + c2])
					- average) > 50)
					continue;

				dot_product = (average[x] * C[x][curr_pos]) + (average[y] * C[y][curr_pos]) + (average[z] * C[z][curr_pos]);
				mag1 = sqrt(SQR(average[x]) + SQR(average[y]) + SQR(average[z]));
				mag2 = sqrt(SQR(C[x][(queue[qt] / cols + r2) * cols + queue[qt] % cols + c2]) + 
							SQR(C[y][(queue[qt] / cols + r2) * cols + queue[qt] % cols + c2]) + 
							SQR(C[z][(queue[qt] / cols + r2) * cols + queue[qt] % cols + c2]));

				angle = acos(dot_product / (mag1 * mag2));

				if (angle > ANGLE_THRESH)
					continue;

				labels[(queue[qt] / cols + r2) * cols + queue[qt] % cols + c2] = new_label;
				if (indices != NULL)
					indices[*count] = (queue[qt] / cols + r2) * cols + queue[qt] % cols + c2;

				//total += image[(queue[qt] / cols + r2) * cols + queue[qt] % cols + c2];
				total[x] += C[x][(queue[qt] / cols + r2) * cols + queue[qt] % cols + c2];
				total[y] += C[y][(queue[qt] / cols + r2) * cols + queue[qt] % cols + c2];
				total[z] += C[z][(queue[qt] / cols + r2) * cols + queue[qt] % cols + c2];

				(*count)++;
				queue[qh] = (queue[qt] / cols + r2) * cols + queue[qt] % cols + c2;
				qh = (qh + 1) % MAX_QUEUE;
				if (qh == qt)
				{
					printf("Max queue size exceeded\n");
					exit(0);
				}
			}
		qt = (qt + 1) % MAX_QUEUE;
	}
}