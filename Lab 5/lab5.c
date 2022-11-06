/* File  : lab5.c
   Author: Aaron Bruner
   Class : ECE - 4310 : Introduction to Computer Vision
   Term  : Fall 2022

   Description: This project must implement the active contour algorithm. The program
                must load a grayscale PPM image and a list of contour points. The contour points must
                be processed through the active contour algorithm using the options given below. The
                program must output a copy of the image with the initial contour drawn on top of it, and a
                second image with the final contour drawn on top of it. The program must also output a
                list of the final contour pixel coordinates.

   Required Files:
    * hawk.ppm
    * hawk_init.txt

   Bugs:
    * Currently none
*/

//#define DEBUG False
#define BLACK 0
#define WHITE 255

#define SQR(x) ((x)*(x))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

struct contourPoints {
    //char letter;
    int  x; // COLUMN
    int  y; // ROW
};

void sobel(int* gradientImage, float* gradientImageTwo, unsigned char* sourceImage, int COLS, int ROWS);
void normalize(unsigned char* normImage, int* srcImage, int max, int min, int COLS, int ROWS);
float* normalizeBinary(float* energy);
void MaxMin(int* srcImage, int *max, int *min, int COLS, int ROWS);
void MaxMinSobel(float* srcImage, float *max, float *min, int COLS, int ROWS);
void outputImage(unsigned char* source, char* fileName, int col, int row);
unsigned char* readImage(int* ROWS, int* COLS, char* source);
unsigned char* createImage(int size);

struct contourPoints* readCSV(char* contourPointsDir, int* fileRows);

char* sourceImageDir = "hawk.ppm";
char* contoursPointsDir = "hawk_init.txt";

int main(int argc, char* argv[])
{
    unsigned char* sourceImage, *sourceWithContours, *normalizedImage;
    int* gradientImage;
    struct contourPoints* contours;
    struct contourPoints* newContours;

    int r, c, x, y, i = 0, j = 0, fileRows = 0, sourceROWS, sourceCOLS, max = 0, min = 0, counter = 0, location, COLS = 7;

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    /*                 STEP 1: Read in source image and contour pixels                             */
    /*      * User provides no arguments (argc == 1) then we default to specified files            */
    /*      * User provides 2 arguments  (argc == 3) then we open provided files                   */
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */

    if (argc == 1) {
        sourceImage = readImage(&sourceROWS, &sourceCOLS, sourceImageDir);
        contours = readCSV(contoursPointsDir, &fileRows);
    }
    else if (argc == 3)
    {
        sourceImage = readImage(&sourceROWS, &sourceCOLS, argv[1]);
        contours = readCSV(argv[2], &fileRows);
    }
    else
    {
        printf("Incorrect number of arguments...\nUsage: ./lab5 (sourceImage.ppm) (ContourPoints.txt)\n");
        exit(0);
    }

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    /*          STEP 2: Print plus signs on source image at contour locations                      */
    /* The image with the initial contours drawn as an arrow at each location
    *
    *  _   _   _   X-3 _   _   _
    *  _   _   _   X-2 _   _   _
    *  _   _   _   X-1 _   _   _
    *  Y-3 Y-2 Y-1 YX  Y+1 Y+2 Y+3
    *  _   _   _   X+1 _   _   _
    *  _   _   _   X+2 _   _   _
    *  _   _   _   X+3 _   _   _
    *
    *  Create a copy of the original image */
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    sourceWithContours = createImage(sourceCOLS * sourceROWS);
    for (i = 0; i < sourceROWS * sourceCOLS; i++) sourceWithContours[i] = sourceImage[i];
    for (i = 0; i < fileRows; i++)
    {
        for (j = -3; j < 4; j++)
        {
            sourceWithContours[(contours[i].y + j) * sourceCOLS + contours[i].x] = BLACK; // Vertical Line
            sourceWithContours[contours[i].y * sourceCOLS + (contours[i].x + j)] = BLACK; // Horizontal Line
        }
    }
    outputImage(sourceWithContours, "hawk_sourceArrows.ppm", sourceCOLS, sourceROWS);

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    /*                 STEP 3: Get the Sobel edge gradient magnitude image                         */
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    gradientImage = calloc(sourceROWS * sourceCOLS, sizeof(int));
    float* sobelImage = calloc(sourceROWS * sourceCOLS, sizeof(float));
    sobel(gradientImage, sobelImage, sourceImage, sourceCOLS, sourceROWS);

    // Find maximum and minimum values
    MaxMin(gradientImage, &max, &min, sourceCOLS, sourceROWS);
    float maxtwo = 0, mintwo = 0;
    MaxMinSobel(sobelImage, &maxtwo, &mintwo, sourceCOLS, sourceROWS);

    // Normalize the image using min and max values
    normalizedImage = createImage(sourceROWS * sourceCOLS);
    normalize(normalizedImage, gradientImage, max, min, sourceCOLS, sourceROWS);
    outputImage(normalizedImage, "hawk_normalized.ppm", sourceCOLS, sourceROWS);

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    /*                  STEP 4: Internal and external energy                                       */
    /*  You must experiment with different window sizes and weightings of each energy term, to     */
    /*  find which gives the best result. Each energy term can be normalized by rescaling from     */
    /*  min-max value to 0-1, to assist with weighting. The active contour algorithm should run    */
    /*  for a maximum of 30 iterations, but you should experiment with fewer iterations.           */
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */

    // Initialize our energy variables
    float* inEnergyOne = calloc(SQR(COLS), sizeof(float));
    float* inEnergyTwo = calloc(SQR(COLS), sizeof(float));
    float* exEnergy    = calloc(SQR(COLS), sizeof(float));
    float* totalEnergy = calloc(SQR(COLS), sizeof(float));
    float avgDist = 0, * normOne, * normTwo, * normEx;
    newContours = calloc(fileRows, sizeof(struct contourPoints));

    float* invertedSobel = (float*)calloc(sourceCOLS * sourceROWS, sizeof(float));
    unsigned char* result = createImage(sourceCOLS * sourceROWS);

    //INVERT
    for (int h = 0; h < sourceCOLS * sourceROWS; h++)
    {
        invertedSobel[h] = (float)maxtwo - sobelImage[h];
    }

    int a = 0;

    do
    {
        // Calculate the average distance
        float temp1 = 0.0, temp2 = 0.0;
        avgDist = 0;

        //printf("[%d]\n", counter);

        for (a = 0; a < fileRows - 1; a++)
        {
            temp1 = SQR(contours[a].x - contours[a+1].x);
            temp2 = SQR(contours[a].y - contours[a+1].y);
            //printf("\t%f %f\t\t%d %d\n", temp1, temp2, contours[a].x, contours[a].y);

            avgDist += sqrt(temp1 + temp2);
            newContours[a].x = newContours[a].y = 0;

            //avgDist += sqrt(SQR(contours[a].y - contours[a == (fileRows - 1) ? 0 : (a + 1)].y) + SQR(contours[a].x - contours[a == (fileRows - 1) ? 0 : (a + 1)].x));
            //avgDist = (a == (fileRows - 1)) ? avgDist / fileRows : avgDist;
        }
        temp1 = SQR(contours[a].x - contours[0].x);
        temp2 = SQR(contours[a].y - contours[0].y);
        newContours[a].x = newContours[a].y = 0;

        avgDist += sqrt(temp1+temp2);
        avgDist = avgDist / fileRows;

        //printf("\t%f %f\t\t%d %d\t%f\n", temp1, temp2, contours[a].x, contours[a].y, avgDist);

        for (int b = 0; b < fileRows; b++)
        {
            // Empty energy variables
            for (int c = 0; c < fileRows; c++)
            {
                inEnergyOne[c] = inEnergyTwo[c] = exEnergy[c] = totalEnergy[c] = 0;
            }

            // Calculate the energy
            for (r = -3; r <= 3; r++)
            {
                for (c = -3; c <= 3; c++)
                {
                    inEnergyOne[(r + 3) * 7 + (c + 3)] = SQR((contours[b].y + r) - contours[b == (fileRows - 1) ? 0 : (b + 1)].y) + SQR((contours[b].x + c) - contours[b == (fileRows - 1) ? 0 : (b + 1)].x);
                    inEnergyTwo[(r + 3) * 7 + (c + 3)] = SQR(sqrt(inEnergyOne[(r + 3) * 7 + (c + 3)]) - avgDist);
                       exEnergy[(r + 3) * 7 + (c + 3)] = SQR(invertedSobel[(contours[b].y + r) * sourceCOLS + (contours[b].x + c)]);
                    //printf("[%d] %f (%f | %d) %f\n", (r+3)*7+(c+3), inEnergyOne[(r + 3) * 7 + (c + 3)], inEnergyTwo[(r + 3) * 7 + (c + 3)], (r + 3) * 7 + (c + 3), exEnergy[(r + 3) * 7 + (c + 3)]);
                }
            }

            // Normalize the energy
            normOne = normalizeBinary(inEnergyOne);
            normTwo = normalizeBinary(inEnergyTwo);
            normEx  = normalizeBinary(exEnergy);

            // Get Total Energy and location
            min = location = 0;
            for (int d = 0; d < SQR(COLS); d++)
            {
                totalEnergy[d] = 3 * (normOne[d] + normTwo[d]) + (2 * normEx[d]);
                (d == 0) ? min = totalEnergy[d] : (totalEnergy[d] < min ? min = totalEnergy[d], location = d : false);
            }

            //printf("Total Energy [%d] %f\n", b, totalEnergy[b]);

            // Now that we have the total energy and location we can find new contour positions
            double temp = (double)location / 7;
            temp > 3 ? newContours[b].x = contours[b].y + abs(temp - 3) : (temp < 3 ? newContours[b].x = contours[b].y - abs(temp - 3) : (newContours[b].x = contours[b].y));
                  temp = (int)(location % 7);
            temp > 3 ? newContours[b].y = contours[b].x + abs(temp - 3) : (temp < 3 ? newContours[b].y = contours[b].x - abs(temp - 3) : (newContours[b].y = contours[b].x));

            //printf("COL : %d\tROW : %d\n", newContours[b].x, newContours[b].y);

        }

        // Apply changes made above
        for (int e = 0; e < fileRows; e++)
        {
            contours[e].x = newContours[e].y;
            contours[e].y = newContours[e].x;
        }
        
        counter++;

        if (counter == 2 || counter == 5 || counter == 10 || counter == 15 || counter == 20 || counter == 25 || counter == 30)
        {
            for (int u = 0; u < sourceROWS * sourceCOLS; u++) result[u] = sourceImage[u];
            for (int f = 0; f < fileRows; f++)
            {
                for (int g = -3; g <= 3; g++)
                {
                    result[(contours[f].y + g) * sourceCOLS + contours[f].x] = BLACK; // Vertical Line
                    result[contours[f].y * sourceCOLS + (contours[f].x + g)] = BLACK; // Horizontal Line
                }
            }
            switch (counter)
            {
            case 2:
                outputImage(result, "hawk_final_2.ppm", sourceCOLS, sourceROWS);
            case 5:
                outputImage(result, "hawk_final_5.ppm", sourceCOLS, sourceROWS);
            case 10:
                outputImage(result, "hawk_final_10.ppm", sourceCOLS, sourceROWS);
            case 15:
                outputImage(result, "hawk_final_15.ppm", sourceCOLS, sourceROWS);
            case 20:
                outputImage(result, "hawk_final_20.ppm", sourceCOLS, sourceROWS);
            case 25:
                outputImage(result, "hawk_final_25.ppm", sourceCOLS, sourceROWS);
            case 30:
                outputImage(result, "hawk_final_30.ppm", sourceCOLS, sourceROWS);
            }
        }

    } while (counter < 30);

    FILE *fpt;
    fpt = fopen("coordinates.txt", "w");
    // Output final coordinates
    for (int p = 0; p < fileRows; p++)
    {
        if (p == 0) fprintf(fpt, "Columns Rows\n");
        fprintf(fpt, "%d %d\n", contours[p].x, contours[p].y);
    }
    fclose(fpt);

    return 0;
}

float* normalizeBinary(float * energy)
{
    int i = 0, r = 0, c = 0, cols = 7;
    float* result = calloc(SQR(cols), sizeof(float));
    float min = energy[0], max = energy[0];

    for (i = 1; i < SQR(cols); i++)
    {
        max < energy[i] ? max = energy[i] : max;
        min > energy[i] ? min = energy[i] : min;
    }

    for (r = 0; r < cols; r++)
    {
        for (c = 0; c < cols; c++)
        {
            result[r * cols + c] = (energy[r * cols + c] - min) * 255 / (max - min);
        }
    }

    return result;
}

/// <summary>
/// Normalize the source image using the maximum and minimum pixel values provided
/// </summary>
/// <param name="normImage"></param>
/// <param name="srcImage"></param>
/// <param name="max"></param>
/// <param name="min"></param>
/// <param name="COLS"></param>
/// <param name="ROWS"></param>
void normalize(unsigned char* normImage, int* srcImage, int max, int min, int COLS, int ROWS)
{
    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            normImage[r * COLS + c] = (srcImage[r * COLS + c] - min) * 255 / (max - min);
        }
    }

    return;
}

/// <summary>
/// Find the maximum and minimum pixel value for the source image
/// </summary>
/// <param name="srcImage"></param>
/// <param name="max"></param>
/// <param name="min"></param>
void MaxMin(int* srcImage, int *max, int *min, int COLS, int ROWS)
{
    int i = 0;

    (*min) = (*max) = srcImage[0];

    for (i = 0; i < COLS * ROWS; i++)
    {
        (*max) < srcImage[i] ? (*max) = srcImage[i] : (*max);
        (*min) > srcImage[i] ? (*min) = srcImage[i] : (*min);
    }
    return;
}

void MaxMinSobel(float* srcImage, float* max, float* min, int COLS, int ROWS)
{
    int i = 0;

    (*min) = (*max) = srcImage[0];

    for (i = 0; i < COLS * ROWS; i++)
    {
        (*max) < srcImage[i] ? (*max) = srcImage[i] : (*max);
        (*min) > srcImage[i] ? (*min) = srcImage[i] : (*min);
    }
    return;
}

/// <summary>
/// 
/// Level Edge | Vertical Edge | Sobel Template |
/// -1  -2  -1 | -1   0   1    |  w1   w2   w3  |
///  0   0   0 | -2   0   2    |  w4   w5   w6  |
///  1   2   1 | -1   0   1    |  w7   w8   w9  |
/// </summary>
/// <param name="gradientImage"></param>
/// <param name="sourceImage"></param>
/// <param name="sourceROWS"></param>
/// <param name="sourceCOLS"></param>
void sobel(int* gradientImage, float* gradientImageTwo, unsigned char* sourceImage, int COLS, int ROWS)
{
    int levelEdge[9]    = { -1, -2, -1,  0, 0, 0,  1, 2, 1 };
    int verticalEdge[9] = { -1,  0,  1, -2, 0, 2, -1, 0, 1 };
    int x, y, r, c, i, j;

    // Copy source image into convolution image
    for (i = 0; i < COLS * ROWS; i++) gradientImage[i] = sourceImage[i];

    // Apply Sobel Filter
    for (r = 1; r < ROWS - 1; r++)
    {
        for (c = 1; c < COLS - 1; c++, x = 0, y = 0)
        {
            for (i = -1; i <= 1; i++)
            {
                for (j = -1; j <= 1; j++)
                {
                    y +=    levelEdge[(i + 1) * 3 + (j + 1)] * sourceImage[(r + i) * COLS + (c + j)];
                    x += verticalEdge[(i + 1) * 3 + (j + 1)] * sourceImage[(r + i) * COLS + (c + j)];
                }
            }
               gradientImage[r * COLS + c] = sqrt(SQR(x) + SQR(y));
            gradientImageTwo[r * COLS + c] = sqrt(SQR(x) + SQR(y));
        }
    }

    return;
}

/// <summary>
/// Read in integer values from CSV file. Except the delimiter is a space
/// </summary>
/// <param name="contourPointsDir">File directory for CSV file</param>
/// <returns>An array of structures which contain the columns and rows from the file</returns>
struct contourPoints* readCSV(char* contourPointsDir, int *fileRows)
{
    int i = 0, r = 0, c = 0;
    struct contourPoints* contours;
    FILE* FPT;

    // Open the file for reading
    FPT = fopen(contourPointsDir, "r");
    FPT == NULL ? printf("Failed to open %s.\n", contourPointsDir), exit(0) : false;

    // Determine the number of rows in the file
    while ((i = fscanf(FPT, "%d %d\n", &c, &r)) && !feof(FPT))
        if (i == 2) (*fileRows) += 1;
    // Number of rows + 1 since last row isn't counted
    (*fileRows)++;
    
    // Allocate space for array of structures
    contours = calloc((*fileRows), sizeof(struct contourPoints));

    // Return to the beginning of the file
    rewind(FPT);
    
    // Scan in all columns and rows
    for (i = 0; i <= (*fileRows) && !feof(FPT); i++)
        fscanf(FPT, "%d %d\n", &contours[i].x, &contours[i].y);
    fclose(FPT);

    return contours;
}

/// <summary>
/// The readImage function is designed to take a file name as the source and reads all of the data into a new image.
/// </summary>
/// <param name="ROWS"> Number of rows in the source image </param>
/// <param name="COLS"> Number of columns in the source image </param>
/// <param name="source"> File name that we're needing to open and read data from </param>
/// <returns> The function returns an array of values which makes up our image </returns>
unsigned char* readImage(int* ROWS, int* COLS, char* source)
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