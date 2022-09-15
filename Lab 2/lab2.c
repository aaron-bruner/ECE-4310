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
    unsigned char* sourceImage, *templateImage, *zeroMeanImage, *MSF;
    char sourceHeader[320], templateHeader[320];
    int i = 0, mean = 0, sourceROWS, sourceCOLS, templateROWS, templateCOLS, filterRow, filterCol;
    int r, c, dr, dc, wr, wc, average;
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

    printf("Step 2:\n"); printf("Calculate the mean of the template image...\n");
    for (i = 0; i < templateCOLS * templateROWS; i++) // Sum all pixels
        mean += templateImage[i];
    mean /= templateCOLS * templateROWS;
    printf("\t* Mean pixel value in the template image = %d\n", mean);

    // Zero Mean Template Image
    printf("\t* Generating the zero mean template image\n");
    for (i = 0; i < templateCOLS * templateROWS; i++)
        templateImage[i] -= mean;

    // MSF[r,c] = SIG(+Wr/2 -> dr=Wr/2) SIG(+Wc/2 -> dc=Wc/2)[ I[r + dr,c + dc] * T[dr + Wr/2,dc + Wc/2] ]
    printf("\t\t* Allocating space for MSF image...");
    MSF = createImage(sourceCOLS * sourceROWS);
    printf("\t[SUCCESS]\n");
    printf("\t\t* Convolving source and zero-mean centered image...");
    wr = templateROWS; wc = templateCOLS; dr = wr/2; dc = wc/2;
    for (r = dr; r < sourceROWS - dr; r++)
    {
        for (c = dc; c < sourceCOLS - dc; c++, MSF[r * sourceCOLS + c] = average)
        {
            average = 0;
            for (filterRow = -wr; filterRow < wr; filterRow++)
            {
                for (filterCol = -wc; filterCol < wc; filterCol++)
                {
                    average += getPixelValue(r + filterRow, c + filterCol, sourceImage) *
                            getPixelValue(filterRow + dr, filterCol + dc, templateImage);
                }
            }
            MSF[r * sourceCOLS + c] = average;
        }
    }
    printf("\t[SUCCESS]\n");

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    /*                        STEP 3: Normalize the MSF image to 8-bit                             */
    /*                          a) Find the min and max of MSF                                     */
    /*                          b) 
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */


    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */


    printf("\t* Finding the minimum pixel and maximum pixel of the MSF...");
    
    // Clean Up : Release all memory we allocated (I forgot to do this in lab 1 :clown:)
    free(sourceImage);
    free(templateImage);
    //free(output);
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