/* File  : lab2.c
   Author: Aaron Bruner
   Class : ECE - 4310 : Introduction to Computer Vision
   Term  : Fall 2022

   Description: The purpose of this lab was to design and implement a matched filter (normalized cross-correlation) to recognize letters in an image of text.
                The ground truth file lists all the letters and image pixel coordinates of text in the image. The pixel coordinates are for the center point of each letter.

   Required Files:
    * parenthood.ppm
    * parenthood_e_template.ppm
    * parenthood_gt.txt

   Bugs:
    * 
*/

#define True 1
#define False 0
#define DEBUG False

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct groundTruth {
    char letter;
    int  x; // COLUMN
    int  y; // ROW
};

int getPixelValue(int rows, int columns, int COLS, unsigned char* image);
unsigned char* readImage( int* ROWS, int* COLS, char* source);
unsigned char* createImage(int size);

char* sourceImageDir    = "parenthood.ppm";
char* templateImageDir  = "parenthood_e_template.ppm";
char* groundTruthDir    = "parenthood_gt.txt";

int main(int argc, char* argv[])
{
    unsigned char* sourceImage, *templateImage, *zeroMeanImage, *MSF_Normalized, *result;
    int* templateMSF, *MSF;
    char sourceHeader[320], templateHeader[320];
    int i = 0, mean = 0, sourceROWS, sourceCOLS, templateROWS, templateCOLS, filterRow, filterCol;
    int r, c, dr, dc, wr, wc, average, min, max;
    FILE* fpt;

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    /*                      STEP 1: Read in source, template and ground truth                      */
    /*      * User provides no arguments (argc == 1) then we default to specified files            */
    /*      * User provides 4 arguments  (argc == 4) then we open provided files                   */
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    printf("Step 1:\n");
    if (argc == 1) {
        printf("Performing matched filter on images [%s] and [%s] using ground truth [%s]\n", sourceImageDir, templateImageDir, groundTruthDir);

        printf("\t* Reading in source image...");
        sourceImage = readImage(&sourceROWS, &sourceCOLS, sourceImageDir);
        printf("\t[SUCCESS]\n");
        printf("\t* Reading in template image...");
        templateImage = readImage(&templateROWS, &templateCOLS, templateImageDir);
        printf("\t[SUCCESS]\n");

        // Read in CSV/TXT file
        printf("\t* Opening ground truth file...");
        fpt = fopen(groundTruthDir, "r");
        if (fpt == NULL)
        {
            printf("Failed to open %s\n", groundTruthDir); exit(0);
        }
        else
        {
            printf("\t[SUCCESS]\n");
        }
    }
    else if (argc == 4)
    {
        printf("Performing matched filter on images [%s] and [%s] using ground truth [%s]\n", argv[1], argv[2], argv[3]);

        printf("\t* Reading in source image...");
        sourceImage = readImage(&sourceROWS, &sourceCOLS, argv[1]);
        printf("\t[SUCCESS]\n");
        printf("\t* Reading in template image...");
        templateImage = readImage(&templateROWS, &templateCOLS, argv[2]);
        printf("\t[SUCCESS]\n");

        // Read in CSV/TXT file
        printf("\t* Opening ground truth file...");
        fpt = fopen(argv[3], "r");
        if (fpt == NULL)
        {
            printf("Failed to open %s\n", argv[3]); exit(0);
        }
        else
        {
            printf("\t[SUCCESS]\n");
        }
    }

    int temp1, temp2, fileRows = 0;                       // Number of rows in the ground truth file
    char temp;
    while ((i = fscanf(fpt, "%c %d %d\n", &temp, &temp1, &temp2)) && !feof(fpt))
        if (i == 3) fileRows += 1;
    printf("\t* Found %d number of rows in the ground truth file\n", fileRows);

    struct groundTruth* truth;
    truth = calloc(fileRows, sizeof(struct groundTruth));

    rewind(fpt); // Return to the beginning of the file
    printf("\t* Scanning in values from ground truth file...");
    for (i = 0; i <= fileRows && !feof(fpt); i++)
    {
        fscanf(fpt, "%c %d %d\n", &truth[i].letter, &truth[i].x, &truth[i].y);
    }
    fclose(fpt);
    printf("\t[Read in %d rows]\n", i - 1);

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    /*                  STEP 2: Calculate the matched-spatial filter (MSF) image.                  */
    /*                          a) Zero-Mean Center the template                                   */
    /*                          b) Convolve with image                                             */
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */

    templateMSF = (int*)calloc(templateCOLS * templateROWS, sizeof(int));

    printf("Step 2:\n"); printf("Calculate the mean of the template image...\n");
    for (i = 0; i < templateCOLS * templateROWS; i++) // Sum all pixels
        mean += templateImage[i];
    mean /= templateCOLS * templateROWS;
    printf("\t* Mean pixel value in the template image = %d\n", mean);

    // Zero Mean Template Image
    printf("\t* Generating the zero mean template image\n");
    for (i = 0; i < templateCOLS * templateROWS; i++)
        templateMSF[i] = templateImage[i] - mean;

    // MSF[r,c] = SIG(+Wr/2 -> dr=Wr/2) SIG(+Wc/2 -> dc=Wc/2)[ I[r + dr,c + dc] * T[dr + Wr/2,dc + Wc/2] ]
    printf("\t\t* Allocating space for MSF image...");
    MSF = (int*)calloc(sourceCOLS * sourceROWS, sizeof(int));
    printf("\t[SUCCESS]\n");
    printf("\t\t* Convolving source and zero-mean centered image...");
    wr = templateROWS; wc = templateCOLS; dr = wr/2; dc = wc/2;
    for (r = dr; r < sourceROWS - dr; r++)
    {
        for (c = dc; c < sourceCOLS - dc; c++, average = 0)
        {
            for (filterRow = -dr; filterRow < templateROWS - dr; filterRow++)
            {
                for (filterCol = -dc; filterCol < templateCOLS - dc; filterCol++)
                {
                    average += sourceImage[(r + filterRow) * sourceCOLS + (c + filterCol)] *
                            templateMSF[(filterRow + dr) * templateCOLS + (filterCol + dc)];
                }
            }
            MSF[r * sourceCOLS + c] = (int)average;
        }
    }
    printf("\t[SUCCESS]\n");

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    /*                        STEP 3: Normalize the MSF image to 8-bit                             */
    /*                          a) Find the min and max of MSF                                     */
    /*                          b) Create the 8-bit representation of the MSF                      */
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */

    printf("Step 3:\n"); printf("Finding the minimum pixel and maximum pixel of the MSF...\n");
    min = max = MSF[0];
    printf("\t* Calculating the minimum and maximum pixel in MSF image...");
    for (i = 1; i < sourceROWS * sourceCOLS; i++)
    {
        if (MSF[i] > max)
            max = MSF[i];
        if (MSF[i] < min)
            min = MSF[i];
    }
    printf("\t[SUCCESS]\n");
    printf("\t\t* Minimum determined to be: %d\n\t\t* Maximum determined to be: %d\n", min, max);
    printf("\t* Normalizing the MSF image to 8-bit...");
    printf("\n\t\t* Creating space for normalized image...");
    MSF_Normalized = createImage(sourceCOLS * sourceROWS);
    printf("\t[SUCCESS]\n");

    for (i = 0; i < sourceROWS * sourceCOLS; i++)
    {
        MSF_Normalized[i] = (MSF[i] - min) * 255 / (max - min);
    }

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    /*                 STEP 4: Looping through the following steps for a range of T                */
    /*           a) Threshold at T the normalized MSF image to create a binary image               */
    /*           b) Loop through the ground truth letter locations                                 */
    /*                  i. Check a 9 x 15 pixel area centered at the ground truth location. If     */
    /*                     any pixel in the MSF image is greater than the threshold, consider      */
    /*                     the letter “detected”.If none of the pixels in the 9 x 15 area are      */
    /*                     greater than the threshold, consider the letter “not detected”          */
    /*           c) Categorize and count the detected letters as FP (“detected” but the letter is  */
    /*              not ‘e’) and TP (“detected” and the letter is ‘e’)                             */
    /*           d) Output the total FP and TP for each T                                          */
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    
    int found = False, T = 150; // Threshold
    
    printf("Step 4:\n"); printf("Creating a binary image using the threshold...");
    printf("\t* Creating result image"); result = createImage(sourceCOLS * sourceROWS); printf("\t[SUCCESS]\n");

    for (r = 7; r < sourceROWS - 7; r += 2 * 7 + 1) {
        for (c = 4; c < sourceCOLS - 4; c += 2 * 4 + 1) {
            for (filterRow = -7; filterRow <= 7; filterRow++) {
                for (filterCol = -4; filterCol <= 4; filterCol++) {
                    if ((int)MSF_Normalized[(r + filterCol) * sourceCOLS + (c + filterCol)] >= T) {
                        found = True;

                    }
                }
            }
            result[r * sourceCOLS + c] = found == True ? 255 : 0;
            found = False;
        }
    }

    fpt = fopen("result.ppm", "w");
    fprintf(fpt, "P5 %d %d 255\n", sourceCOLS, sourceROWS);
    fwrite(result, sourceCOLS * sourceROWS, 1, fpt);
    fclose(fpt);

    fpt = fopen("MSF_Normalized.ppm", "w");
    fprintf(fpt, "P5 %d %d 255\n", sourceCOLS, sourceROWS);
    fwrite(MSF_Normalized, sourceCOLS * sourceROWS, 1, fpt);
    fclose(fpt);

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    
    // Clean Up : Release all memory we allocated (I forgot to do this in lab 1 :clown:)
    free(sourceImage);
    free(templateImage);
    free(result);
    free(truth);
}

/// <summary>
/// getPixelValue only has one purpose. It is designed to get the integer pixel value at a row and column
/// </summary>
/// <param name="rows"></param>
/// <param name="columns"></param>
/// <param name="COLS"> Number of columns in the source image </param>
/// <param name="image"> Image we're interested in getting the pixel value of</param>
/// <returns></returns>
int getPixelValue(int rows, int columns, int COLS, unsigned char* image) 
{
    return image[columns + rows * COLS];
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
    FILE *fpt = fopen(source, "rb");
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
    
    unsigned char* destination = createImage((*ROWS)*(*COLS)); // Create an empty image that is large enough for ROWS x COLS bytes
    
    fread(destination, 1, (*ROWS) * (*COLS), fpt);
    fclose(fpt);

    return destination;
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