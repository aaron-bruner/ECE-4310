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
    * Currently none
*/

#define True 1
#define False 0
#define DEBUG False
#define T 255 // Upper limit for thresholding

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct groundTruth {
    char letter;
    int  x; // COLUMN
    int  y; // ROW
};

unsigned char* readImage( int* ROWS, int* COLS, char* source);
unsigned char* createImage(int size);

char* sourceImageDir    = "parenthood.ppm";
char* templateImageDir  = "parenthood_e_template.ppm";
char* groundTruthDir    = "parenthood_gt.txt";

int main(int argc, char* argv[])
{
    unsigned char* sourceImage, *templateImage, *zeroMeanImage, *MSF_Normalized, *thresholdImage;
    int* templateMSF, *MSF;
    struct groundTruth* truth;
    char sourceHeader[320], templateHeader[320], temp, letter;
    int temp1, temp2, fileRows = 0; // Number of rows in the ground truth file
    int i = 0, j = 0, mean = 0, sourceROWS, sourceCOLS, templateROWS, templateCOLS, filterRow, filterCol;
    int r, c, dr, dc, wr, wc, average, min, max, found = False, TP, FP, maxTP, maxT;
    FILE* fpt, *TPFPfpt;

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    /*                      STEP 1: Read in source, template and ground truth                      */
    /*      * User provides no arguments (argc == 1) then we default to specified files            */
    /*      * User provides 4 arguments  (argc == 4) then we open provided files                   */
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */

    printf("Step 1:\n");
    if (argc == 1) {
        printf("Performing matched filter on images [%s] and [%s] using ground truth [%s]\n", sourceImageDir, templateImageDir, groundTruthDir);

        printf("\t* Reading in source image...");
        sourceImage = readImage(&sourceROWS, &sourceCOLS, sourceImageDir); printf("\t[SUCCESS]\n");
        printf("\t* Reading in template image...");
        templateImage = readImage(&templateROWS, &templateCOLS, templateImageDir); printf("\t[SUCCESS]\n");

        // Read in CSV/TXT file
        printf("\t* Opening ground truth file...");
        fpt = fopen(groundTruthDir, "r");
        fpt == NULL ? printf("Failed to open %s\n", groundTruthDir), exit(0) : printf("\t[SUCCESS]\n");
    }
    else if (argc == 4)
    {
        printf("Performing matched filter on images [%s] and [%s] using ground truth [%s]\n", argv[1], argv[2], argv[3]);

        printf("\t* Reading in source image...");
        sourceImage = readImage(&sourceROWS, &sourceCOLS, argv[1]); printf("\t[SUCCESS]\n");
        printf("\t* Reading in template image...");
        templateImage = readImage(&templateROWS, &templateCOLS, argv[2]); printf("\t[SUCCESS]\n");

        // Read in CSV/TXT file
        printf("\t* Opening ground truth file...");
        fpt = fopen(argv[3], "r");
        fpt == NULL ? printf("Failed to open %s\n", argv[3]), exit(0) : printf("\t[SUCCESS]\n");
    }
    else
    {
        printf("Incorrect number of arguments...\nUsage: ./lab2 (sourceImage.ppm) (templateImage.ppm) (groundTruth.txt)\n");
        exit(0);
    }

    while ((i = fscanf(fpt, "%c %d %d\n", &temp, &temp1, &temp2)) && !feof(fpt))
        if (i == 3) fileRows += 1;
    printf("\t* Found %d number of rows in the ground truth file\n", fileRows);

    printf("\t* Allocating space for ground truth file...");
    truth = calloc(fileRows, sizeof(struct groundTruth)); printf("\t[SUCCESS]\n");

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
    printf("\t\t* Allocating space for template MSF image...");
    templateMSF = (int*)calloc(templateCOLS * templateROWS, sizeof(int)); printf("\t[SUCCESS]\n");
    for (i = 0; i < templateCOLS * templateROWS; i++)
        templateMSF[i] = templateImage[i] - mean;

    // MSF[r,c] = SIG(+Wr/2 -> dr=Wr/2) SIG(+Wc/2 -> dc=Wc/2)[ I[r + dr,c + dc] * T[dr + Wr/2,dc + Wc/2] ]
    printf("\t\t* Allocating space for MSF image...");
    MSF = (int*)calloc(sourceCOLS * sourceROWS, sizeof(int)); printf("\t[SUCCESS]\n");
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
        // https://en.wikipedia.org/wiki/Normalization_(image_processing)
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
    
    printf("Step 4:\n"); printf("Creating a binary image using the threshold...");
    printf("\n\t* Allocating space for result image"); 
    thresholdImage = createImage(sourceCOLS * sourceROWS); printf("\t[SUCCESS]\n");
    char outStr[20];
    TPFPfpt = fopen("TPFP.txt", "w"); TPFPfpt == NULL ? (printf("Failed to open TPFP.txt.\n"), exit(0)) : TPFPfpt;

    printf("\t* Loop over the MSF image with threshold values from 0 to %d incrementing by 10...\n", T);
    for (i = 0; i <= T; i++, TP = 0, FP = 0)
    {
        // Part a: Generate binary image using threshold
        for (int pixel = 0; pixel < sourceCOLS * sourceROWS; pixel++)
        {
            thresholdImage[pixel] = MSF_Normalized[pixel] >= i ? (unsigned char)255 : (unsigned char)0;
        }
        
        // Part b: Looping through the ground truth letter locations
        for (j = 0; j <= fileRows; j++, found = False)
        {
            for (filterRow = truth[j].y - dr; filterRow <= truth[j].y + dr; filterRow++)
            {
                for (filterCol = truth[j].x - dc; filterCol <= truth[j].x + dc; filterCol++)
                {
                    // i) found a 255 pixel within a 9x15 area of x,y                           | Check to see if it's already true
                    found = (thresholdImage[filterRow * sourceCOLS + filterCol] == 255) ? True : (found == True) ? True : False;
                }
            }
            // Part c: If we find a 255 pixel and it's actually e then TP. Otherwise we find something that's not e it's a FP
            truth[j].letter == 'e' ? (found == True ? TP++ : TP) : (found == True ? FP++ : FP);
        }
        // Part d: Outputting the total FP and TP for each T value ranging from 0 to 255
        fprintf(TPFPfpt, "Threshold [%3d] : TP = %4d\t| FP = %4d\n", i, TP, FP);
        
        // Find the threshold that gives us the larges amount of TP
        if (T == 0)
        {
            maxTP = TP;
            maxT = i;
        }
        else if (TP >= maxTP)
        {
            maxTP = TP;
            maxT = i;
        }
    }

    // Generating the ideal image using the threshold value with the most TP
    printf("\t* Generating the ideal OCR image using threshold value [%d]...", maxT);
    for (int pixel = 0; pixel < sourceCOLS * sourceROWS; pixel++)
    {
        thresholdImage[pixel] = MSF_Normalized[pixel] >= maxT ? (unsigned char)255 : (unsigned char)0;
    }
    printf("\t[SUCCESS]\n");
    printf("\t* Sending result image to idealImage.ppm...");
    fpt = fopen("idealImage.ppm", "w");
    fprintf(fpt, "P5 %d %d 255\n", sourceCOLS, sourceROWS);
    fwrite(thresholdImage, sourceCOLS * sourceROWS, 1, fpt);
    fclose(fpt);
    printf("\t[SUCCESS]\n");

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
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