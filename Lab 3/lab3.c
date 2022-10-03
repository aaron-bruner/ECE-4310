/* File  : lab3.c
   Author: Aaron Bruner
   Class : ECE - 4310 : Introduction to Computer Vision
   Term  : Fall 2022

   Description: The purpose of this lab is to implement thinning, branchpoint and endpoint detection to recognize letters in an image of text.

   Required Files:
    * parenthood.ppm
    * parenthood_e_template.ppm
    * parenthood_gt.txt
    * msf_e.ppm

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
void thin(unsigned char* srcImage);
void edgeNonEdgeTransitions(unsigned char* img, int *transitions, int *neighbors, int *passNum4, int r, int c);

char* sourceImageDir    = "parenthood.ppm";
char* templateImageDir  = "parenthood_e_template.ppm";
char* groundTruthDir    = "parenthood_gt.txt";
char* msfeImageDir      = "msf_e.ppm";

int main(int argc, char* argv[])
{
    unsigned char* sourceImage, *thinResult, *templateImage, *msf_eImage, *zeroMeanImage, *MSF_Normalized, *thresholdImage;
    int* templateMSF, *MSF;
    struct groundTruth* truth;
    char sourceHeader[320], templateHeader[320], temp, letter;
    int temp1, temp2, fileRows = 0; // Number of rows in the ground truth file
    int i = 0, j = 0, mean = 0, sourceROWS, sourceCOLS, templateROWS, templateCOLS, filterRow, filterCol, msfe_ROWS, msfe_COLS;
    int r, c, dr = 7, dc = 4, wr = 7, wc = 4, average, min, max, found = False, TP, FP, maxTP, maxT, threshLocation, isE = False;
    FILE* fpt, *TPFPfpt;

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    /*                 STEP 1: Read in source, msf image and ground truth                          */
    /*      * User provides no arguments (argc == 1) then we default to specified files            */
    /*      * User provides 4 arguments  (argc == 5) then we open provided files                   */
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */

    printf("Step 1:\n");
    if (argc == 1) {
        printf("Performing matched filter on images [%s] and [%s] using ground truth [%s] and MSF image [%s]\n", sourceImageDir, templateImageDir, groundTruthDir, msfeImageDir);

        printf("\t* Reading in source image...");
        sourceImage = readImage(&sourceROWS, &sourceCOLS, sourceImageDir); printf("\t[SUCCESS]\n");
        printf("\t* Reading in template image...");
        templateImage = readImage(&templateROWS, &templateCOLS, templateImageDir); printf("\t[SUCCESS]\n");
        printf("\t* Reading in MSF_e image...");
        msf_eImage = readImage(&msfe_ROWS, &msfe_COLS, msfeImageDir); printf("\t[SUCCESS]\n");

        // Read in CSV/TXT file
        printf("\t* Opening ground truth file...");
        fpt = fopen(groundTruthDir, "r");
        fpt == NULL ? printf("Failed to open %s\n", groundTruthDir), exit(0) : printf("\t[SUCCESS]\n");
    }
    else if (argc == 5)
    {
        printf("Performing matched filter on images [%s] and [%s] using ground truth [%s] and MSF image [%s]\n", argv[1], argv[2], argv[4], argv[3]);

        printf("\t* Reading in source image...");
        sourceImage = readImage(&sourceROWS, &sourceCOLS, argv[1]); printf("\t[SUCCESS]\n");
        printf("\t* Reading in template image...");
        templateImage = readImage(&templateROWS, &templateCOLS, argv[2]); printf("\t[SUCCESS]\n");
        printf("\t* Reading in MSF_e image...");
        msf_eImage = readImage(&msfe_ROWS, &msfe_COLS, argv[3]); printf("\t[SUCCESS]\n");

        // Read in CSV/TXT file
        printf("\t* Opening ground truth file...");
        fpt = fopen(argv[4], "r");
        fpt == NULL ? printf("Failed to open %s\n", argv[3]), exit(0) : printf("\t[SUCCESS]\n");
    }
    else
    {
        printf("Incorrect number of arguments...\nUsage: ./lab2 (sourceImage.ppm) (templateImage.ppm) (msf_e.ppm) (groundTruth.txt)\n");
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
    /*    STEP 2: Looping through the following steps for a range of T                             */
    /*      a) Loop through the ground truth letter locations                                      */
    /*          i) Check a 9x15 pixel area centered at the ground truth location. If any pixel in  */
    /*              the msf image is greater then the threshold, consider the letter "detected".   */
    /*              If none of the pixels in the 9 x 15 area are greater than the threshold,       */
    /*              consider the letter "not detected".                                            */
    /*         ii) If the letter is "not detected" continue to the next letter                     */
    /*        iii) Create a 9 x 15 pixel image that is a copy of the area centered at the ground   */
    /*              truth location(center of letter) from the original image.                      */
    /*         iv) Threshold this image at 128 to create a binary image.                           */
    /*          v) Thin the thresholded image down to single-pixel wide components.                */
    /*         vi) Check all remaining pixels to determine if they are branch-points or endpoints. */
    /*        vii) If there are not exactly 1 branch-point and 1 endpoint, do not further consider */
    /*             this letter(it becomes “not detected”)                                          */
    /*      b) Count up the number of FP (letters detected that are not 'e') and TP (number of     */
    /*          letters detected that are 'e').                                                    */
    /*      c) Output the total TP and FP for each T.                                              */
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */

    //char buff[12];

    printf("Step 2:\n");
    thresholdImage = createImage(templateCOLS * templateROWS);
    thinResult = createImage(templateCOLS * templateROWS);
    // Open to write to clear file
    TPFPfpt = fopen("TPFP.txt", "w"); TPFPfpt == NULL ? (printf("Failed to open TPFP.txt.\n"), exit(0)) : TPFPfpt; fclose(TPFPfpt);

    printf("\t* Loop over the MSF image with threshold values from 0 to %d incrementing by 10...\n", T);
    for (i = 0, threshLocation = 0; i <= T; i++, TP = 0, FP = 0, threshLocation = 0)
    {
        // Part a: Looping through the ground truth letter locations
        for (j = 0; j <= fileRows; j++, found = False, isE = False)
        {
            for (filterRow = truth[j].y - dr; filterRow <= truth[j].y + dr; filterRow++)
            {
                for (filterCol = truth[j].x - dc; filterCol <= truth[j].x + dc; filterCol++, threshLocation++)
                {
                    // i) found a pixel > threshold within a 9x15 area of x,y             | Check to see if it's already true
                    found = (msf_eImage[filterRow * sourceCOLS + filterCol] > i) ? True : (found == True) ? True : False;

                    // Part iii)
                    thresholdImage[threshLocation] = sourceImage[filterRow * sourceCOLS + filterCol];
                    //printf("Writing %d to [%d,%d]\n", sourceImage[filterRow * sourceCOLS + filterCol], filterRow, filterCol);
                }
            }
            // Part ii) Skip all of this if we never found a letter 
            // This doesn't mean the letter is 'e' it just means we found something that wasn't space
            if (found)
            {
                // Part iv)
                for (int pixel = 0; pixel < templateCOLS * templateROWS; pixel++)
                {
                    thresholdImage[pixel] = thresholdImage[pixel] > 128 ? (unsigned char)255 : (unsigned char)0;
                }

                // Part v)
                thin(thresholdImage);

                //snprintf(buf, 12, "pre_%d_suff", i); // puts string into buffer
                //fpt = fopen(".ppm", "w");
                //fprintf(fpt, "P5 %d %d 255\n", sourceCOLS, sourceROWS);
                //fwrite(thresholdImage, sourceCOLS * sourceROWS, 1, fpt);
                //fclose(fpt);

                // Part vi)
                branchEndPoints(thresholdImage, &isE);


            }
        }
    }
    fclose(TPFPfpt);

    //printf("\t[SUCCESS]\n");
    //printf("\t* Sending result image to idealImage.ppm...");
    //fpt = fopen("idealImage.ppm", "w");
    //fprintf(fpt, "P5 %d %d 255\n", sourceCOLS, sourceROWS);
    //fwrite(thresholdImage, sourceCOLS * sourceROWS, 1, fpt);
    //fclose(fpt);
    //printf("\t[SUCCESS]\n");

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
}

/// <summary>
/// This is a thinning algorithm designed based on both the lecture notes 'Edge properties' and the
/// Zhang-Suen thinning algorithm found at the link below.
/// https://rosettacode.org/wiki/Zhang-Suen_thinning_algorithm#C
/// 
/// This function uses a helper function edgeNonEdgetransitions which detects if there are any edge pixes
/// and returns them in the variables transitionCount, neighborCount and passNum4 for each row and column.
/// </summary>
/// <param name="srcImage">The source image which is a 9x15 which contains a letter to be thinned</param>
void thin(unsigned char* srcImage)
{
    const int rowE = 15, colE = 9, ON = 255, OFF = 0;
    int erasure = True, transitionCount, neighborCount, passNum4;
    unsigned char* copy = createImage(rowE * colE);

    //Lecture notes: Edge properties | Describes the Zhang-Suen thinning algorithm
    // https://rosettacode.org/wiki/Zhang-Suen_thinning_algorithm#C
    // P9   P2  P3      _   A   _
    // P8   P1  P4      C   X   B
    // P7   P6  P5      _   D   _
    //1. Pass through the image looking at each pixel X.
    do
    {
        for (int i = 0; i < rowE * colE; i++) copy[i] = 0; // Reset the copy image to all black pixels
        erasure = False; // Only repeat if we find a marked pixel

        for (int r = 1; r < rowE; r++)
        {
            for (int c = 1; c < colE; c++, transitionCount = 0, neighborCount = 0)
            {
                //2. Count the number of edge->non-edge transitions in CW (or CCW) order around the pixel X (r,c)
                // Only need to check pixes that are ON
                if (srcImage[r * colE + c] == ON) edgeNonEdgeTransitions(srcImage, &transitionCount, &neighborCount, &passNum4, r, c);

                //5. The edge pixel is marked for erasure if it has
                //      a) exactly 1 edge->non - edge transition,
                //      b) 2 <= edge neighbors <= 6, and
                //      c) passes item #4
                if (transitionCount == 1 && neighborCount >= 3 && neighborCount <= 7 && passNum4 == 1)
                {
                    // Mark pixel for erasure
                    copy[r * colE + c] = ON;
                    erasure = True;
                }
            }
        }

        //6. Once all pixels have been scanned, erase those marked, and repeat
        //      (back to step 1) until no pixels are marked for erasure.
        for (int index = 0; index < colE * rowE; index++)
        {
            copy[index] == ON ? srcImage[index] = OFF : False;
        }
    } while (erasure == True);

    // Free the copy image
    free(copy);
    return;
}

/// <summary>
/// 
/// </summary>
/// <param name="thresholdImage"></param>
/// <param name="isE"></param>
void branchEndPoints(unsigned char *thresholdImage, int *isE)
{


    return;
}

/// <summary>
/// Checking for edge->non-edge transitions in a clockwise motion centered at X which is r,c
/// </summary>
/// <param name="img">The 9 x 15 image we're looping over</param>
/// <param name="transitions">The number of Edge to non-edge transitions</param>
/// <param name="neighbors">Count of neighboring pixels (on)</param>
/// <param name="r"></param>
/// <param name="c"></param>
void edgeNonEdgeTransitions(unsigned char* img, int *transitions, int *neighbors, int *passNum4, int r, int c)
{
    const int COLS = 9, ON = 255, OFF = 0; int last = False;
    // Checking in a clockwise rotation of the following image (r,c)
    // NW  N  NE    |   (-1,-1)  (-1,0)  (-1,+1)    |   (-9,-1)  (-9,0)  (-9,+1)
    //  W  X  E     |   ( 0,-1)  (0, 0)  ( 0,+1)    |   ( 0,-1)  (0, 0)  ( 0,+1)
    // SW  S  SE    |   (+1,-1)  (+1,0)  (+1,+1)    |   (+9,-1)  (+9,0)  (+9,+1)
    //  We'll check North -> North East -> East -> South East -> South -> South West -> West -> North West
    //  Moving up one row = -9  |   Moving down one row = +9    |   Moving left = -1    |   Moving right = +1
    const char N = img[(r-1) * COLS + c], NE = img[(r-1) * COLS + (c+1)],
               E = img[r * COLS + (c+1)], SE = img[(r+1) * COLS + (c+1)],
               S = img[(r+1) * COLS + c], SW = img[(r+1) * COLS + (c-1)],
               W = img[r * COLS + (c-1)], NW = img[(r-1) * COLS + (c-1)];

    //3. Count the number of edge neighbor pixels
    // Check N  edges
    (N  == ON ? (*(neighbors++), last = True) : (last = False));
    // Check NE edges
    (NE == ON ? (*(neighbors++), last = True) : (last == True ? *(transitions++) : (last = False)));
    // Check SE edges
    (SE == ON ? (*(neighbors++), last = True) : (last == True ? *(transitions++) : (last = False)));
    // Check S  edges
    (S  == ON ? (*(neighbors++), last = True) : (last == True ? *(transitions++) : (last = False)));
    // Check SW edges
    (SW == ON ? (*(neighbors++), last = True) : (last == True ? *(transitions++) : (last = False)));
    // Check W  edges
    (W  == ON ? (*(neighbors++), last = True) : (last == True ? *(transitions++) : (last = False)));
    // Check NW edges
    (NW == ON ? (*(neighbors++), last = True) : (last == True ? *(transitions++) : (last = False)));
    // Check NW -> N edges
    (N && last ? *(transitions++) : *(transitions));
    //4. Check that at least one of the North, East, or (West and South) are not edge pixels
    //      A or B or (C and D) != edge
    (N == OFF || E == OFF || (S == OFF && W == OFF)) ? (*passNum4 = True) : (*passNum4 = False);

    return;
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