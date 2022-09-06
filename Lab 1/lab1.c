/* File  : lab1.c
   Author: Aaron Bruner
   Class : ECE - 4310 : Introduction to Computer Vision
   Term  : Fall 2022

   Description: The purpose of this lab was to design and implement three different filter types; 2D Convolution, Separable and Sliding Window.
                Using the image bridge.ppm with all three filters we should get the same result which can be validated using the terminal command
                'make diff.' More information regarding the topics discussed here can be found here:
                https://towardsdatascience.com/a-basic-introduction-to-separable-convolutions-b99ec3102728
*/

#define True 1
#define False 0
#define DEBUG False

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int getPixelValue(int rows, int columns, int COLS, unsigned char* image);
unsigned char* readImage( int* ROWS, int* COLS, char* source);
unsigned char* createImage(int size);
unsigned char* runConvolution(int ROWS, int COLS, int filterSize, unsigned char* sourceImage);
unsigned char* runSeparableFilter(int ROWS, int COLS, int filterSize, unsigned char* sourceImage);
unsigned char* runSlidingWindow(int ROWS, int COLS, int filterSize, unsigned char* sourceImage);

clock_t start, end;
double time_spent = 0.0;

int main(int argc, char* argv[])
{
    unsigned char* sourceImage, * convolutionImage, * separableImage, * slidingImage;
    char header[320];
    int filterSize = 0, ROWS, COLS;
    FILE* fpt;

    // Ensure that the provided CLA are correct and check if the optional filter size is provided
    if (argc < 2) {
        printf("Usage: avg [filename.ppm] (-s) (odd filter size)\n\nExample: avg bridge.ppm -s 3\n");
        exit(0);
    }
    else if (argc >= 2) // Check to see if optional flags are available
    {
        if (argc == 4 && strcmp(argv[2], "-s") == 0) // User specified a filter specific filter size
        {
            if (atoi(argv[3]) % 2 != 0) // Odd
            {
                filterSize = atoi(argv[3]) / 2;
                // We divide by two so that we have the filter size we need. An example is shown below:
                // 0/2 = 0      3/2 = 1     5/2 = 2     7/2 = 3     and so on. Since they integers we don't get the decimals
            }
            else // Even
            {
                printf("Error: Filter size must be an odd number.\n");
                exit(0);
            }
        }
        else // Unless they specify a filter size then we default to 7x7
        {
            filterSize = 3;
        }
    }

    // Once we've allocated space for our image data we can read it in
    sourceImage = readImage(&ROWS, &COLS, argv[1]);

    /// ----------------------------
    ///  Perform the 2D Convolution
    /// ----------------------------
    convolutionImage = runConvolution(ROWS, COLS, filterSize, sourceImage);

    // Export image to file
    fpt = fopen("convolution.ppm", "w");
    fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
    fwrite(convolutionImage, COLS * ROWS, 1, fpt);
    fclose(fpt);

    /// ----------------------------
    ///     Run Separable Filter
    /// ----------------------------
    separableImage = runSeparableFilter(ROWS, COLS, filterSize, sourceImage);

    // Export image to file
    fpt = fopen("separableFilter.ppm", "w");
    fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
    fwrite(separableImage, COLS * ROWS, 1, fpt);
    fclose(fpt);

    // ----------------------------
    //     Run Sliding Window
    // ----------------------------
    slidingImage = runSlidingWindow(ROWS, COLS, filterSize, sourceImage);

    // Export image to file
    fpt = fopen("slidingWindow.ppm", "w");
    fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
    fwrite(slidingImage, COLS * ROWS, 1, fpt);
    fclose(fpt);
}

unsigned char* runConvolution(int ROWS, int COLS, int filterSize, unsigned char* sourceImage)
{
    int average = 0;
    long int timeSpent = 0.0;
    struct timespec	start, end;
    unsigned char* convolutionImage = createImage(ROWS*COLS);
    
    for (int i = 0; i < 10; i++)
    {
        if (i==0) printf("\n\t\tPerforming 2D Convolution\nTime Spend for 10 iterations: ");
        clock_gettime(CLOCK_REALTIME, &start);

        for (int imageRow = filterSize; imageRow < ROWS - filterSize; imageRow++)
        {
            for (int imageColumn = filterSize; imageColumn < COLS - filterSize; imageColumn++, average = 0)
            {
                for (int filterRow = -filterSize; filterRow <= filterSize; filterRow++)
                {
                    for (int column = -filterSize; column <= filterSize; column++) {
                        average = average + getPixelValue(imageRow + filterRow, imageColumn + column, COLS, sourceImage);
                    }
                }
                convolutionImage[imageColumn + imageRow * COLS] = average / ((filterSize * 2 + 1) * (filterSize * 2 + 1));
            }
        }
        clock_gettime(CLOCK_REALTIME, &end);
        printf(" [#%d](%ld %ld) |", i+1, (long int)end.tv_sec, end.tv_nsec);
        timeSpent += end.tv_nsec - start.tv_nsec;
    }
    printf("\nAverage time spent performing 2D Convolution : %ld ns\n", (timeSpent/10) < 0 ? -(timeSpent/10) : timeSpent/10);
    printf("| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - |\n");

    return convolutionImage;
}

unsigned char* runSeparableFilter(int ROWS, int COLS, int filterSize, unsigned char* sourceImage)
{
    int average = 0;
    long int timeSpent = 0.0;
    struct timespec	start, end;
    //unsigned char * separableImageRow = createImage(ROWS * COLS);
    //              --------------------------------------------------------
    // We cannot use unsigned char* because the average we're storing can be values that are greater than 255
    int * separableImageRow = (int *)calloc(ROWS * COLS, sizeof(int));
    unsigned char * separableImageCol = createImage(ROWS * COLS);
    if (separableImageRow == NULL) {
        printf("Unable to allocate %d bytes of memory for separableImageRow.\n", ROWS * COLS);
        exit(0);
    }

    // Run 10 times so we can get an average time over 10 iterations
    for (int i = 0; i < 10; i++)
    {
        if (i == 0) printf("\n\t\tPerforming Separable Filter\nTime Spend for 10 iterations: ");
        clock_gettime(CLOCK_REALTIME, &start);

        // Generate an image with averages of the horizontal rows
        for (int imageRow = 0; imageRow < ROWS; imageRow++)
        {
            for (int imageColumn = filterSize; imageColumn < COLS - filterSize; imageColumn++, average = 0)
            {
                for (int filterColumn = -filterSize; filterColumn <= filterSize; filterColumn++) {
                    average = average + sourceImage[(imageColumn + filterColumn) + imageRow * COLS];
                }
                separableImageRow[imageColumn + imageRow * COLS] = average;
            }
        }

        // This is essentially the normal 2D convolution since the rows are already averaged
        // However, instead of needing to average the rows we only need to worry about the columns
        for (int imageRow = filterSize; imageRow < ROWS - filterSize; imageRow++)
        {
            for (int imageColumn = 0; imageColumn < COLS; imageColumn++, average = 0)
            {
                for (int filterRow = -filterSize; filterRow <= filterSize; filterRow++) {
                    average = average + separableImageRow[imageColumn + (imageRow + filterRow) * COLS];
                }
                separableImageCol[imageColumn + imageRow * COLS] = average / ((filterSize * 2 + 1) * (filterSize * 2 + 1));
            }
        }

        clock_gettime(CLOCK_REALTIME, &end);
        printf(" [#%d](%ld %ld) |", i + 1, (long int)end.tv_sec, end.tv_nsec);
        timeSpent += end.tv_nsec - start.tv_nsec;
    }
    printf("\nAverage time spent performing Separable Filter : %ld ns\n", (timeSpent / 10) < 0 ? -(timeSpent / 10) : timeSpent / 10);
    printf("| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - |\n");

    return separableImageCol;
}

unsigned char* runSlidingWindow(int ROWS, int COLS, int filterSize, unsigned char* sourceImage)
{
    int average = 0;
    long int timeSpent = 0.0;
    struct timespec	start, end;
    //unsigned char * slidingWindowImageRow = createImage(ROWS * COLS);
    //              --------------------------------------------------------
    // We cannot use unsigned char* because the average we're storing can be values that are greater than 255
    int* slidingWindowImageRow = (int*)calloc(ROWS * COLS, sizeof(int));
    unsigned char* slidingWindowImageColumn = createImage(ROWS * COLS);

    for (int i = 0; i < 10; i++)
    {
        if (i == 0) printf("\n\t\tPerforming Sliding Window Filter\nTime Spend for 10 iterations: ");
        clock_gettime(CLOCK_REALTIME, &start);

        // Generate an image with averages of the horizontal rows
        for (int imageRow = 0; imageRow < ROWS; imageRow++)
        {
            for (int imageColumn = filterSize; imageColumn < COLS - filterSize; imageColumn++)
            {
                if (imageColumn == filterSize)
                {
                    average = 0;
                    for (int filterColumn = -filterSize; filterColumn <= filterSize; filterColumn++)
                    {
                        average += sourceImage[(imageColumn + filterColumn) + imageRow * COLS];
                    }
                }
                else
                {
                    average -= sourceImage[(imageColumn - (filterSize + 1)) + imageRow * COLS];
                    average += sourceImage[(imageColumn + filterSize)       + imageRow * COLS];
                }

                slidingWindowImageRow[imageColumn + imageRow * COLS] = average;
            }
        }

        for (int imageColumn = filterSize; imageColumn < COLS - filterSize; imageColumn++)
        {
            for (int imageRow = filterSize; imageRow < ROWS - filterSize; imageRow++)
            {
                if (imageRow == filterSize)
                {
                    average = 0;
                    for (int filterRow = -filterSize; filterRow <= filterSize; filterRow++)
                    {
                        average += slidingWindowImageRow[imageColumn + (imageRow + filterRow) * COLS];
                    }
                }
                else
                {
                    average -= slidingWindowImageRow[imageColumn + (imageRow - (filterSize + 1)) * COLS];
                    average += slidingWindowImageRow[imageColumn + (imageRow + filterSize)       * COLS];
                }
                slidingWindowImageColumn[imageColumn + imageRow * COLS] = average / ((filterSize * 2 + 1) * (filterSize * 2 + 1));
            }
        }

        clock_gettime(CLOCK_REALTIME, &end);
        printf(" [#%d](%ld %ld) |", i + 1, (long int)end.tv_sec, end.tv_nsec);
        timeSpent += end.tv_nsec - start.tv_nsec;
    }
    printf("\nAverage time spent performing Sliding Window Filter : %ld ns\n", (timeSpent / 10) < 0 ? -(timeSpent / 10) : timeSpent / 10);
    printf("| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - |\n");

    return slidingWindowImageColumn;
}

int getPixelValue(int rows, int columns, int COLS, unsigned char* image) 
{
    return image[columns + rows * COLS];
}

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

unsigned char* createImage(int size)
{
    unsigned char* newImage = (unsigned char*)calloc(size, sizeof(unsigned char));
    if (newImage == NULL) {
        printf("Unable to allocate %d bytes of memory.\n", size);
        exit(0);
    }

    return newImage;
}