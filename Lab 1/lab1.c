#define True 1
#define False 0
#define DEBUG False

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int getPixelValue(int rows, int columns, int COLS, unsigned char* image);
// int readHeader(char* imageFile, int* numberOfRows, int* numberOfCols);
void readImage(unsigned char* destination, int* ROWS, int* COLS, char* source);
unsigned char* runConvolution(int ROWS, int COLS, int filterSize, unsigned char* sourceImage);
void separableFilter(void);
void slidingWindow(void);
unsigned char* createImage(int size);

clock_t start, end;
double time_spent = 0.0;

int main(int argc, char* argv[])
{
    unsigned char *sourceImage, *convolutionImage, *separableImage, *slidingImage;
    char header[320];
    int filterSize = 0, ROWS, COLS, BYTES, readHeaderReturn;
    FILE *fpt;

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
    readImage(sourceImage, &ROWS, &COLS, argv[1]);
    
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

}

unsigned char* runConvolution(int ROWS, int COLS, int filterSize, unsigned char* sourceImage)
{
    int average = 0;
    long int timeSpent = 0.0;
    struct timespec	start, end;
    unsigned char* convolutionImage = createImage(ROWS*COLS);
    
    for (int i = 0; i < 10; i++)
    {
        if (i==0) printf("Performing 2D Convolution\nTime Spend for 10 iterations: ");
        clock_gettime(CLOCK_REALTIME, &start);

        for (int r = filterSize; r < ROWS - filterSize; r++)
        {
            for (int c = filterSize; c < COLS - filterSize; c++)
            {
                average = 0;
                for (int row = -filterSize; row <= filterSize; row++)
                {
                    for (int column = -filterSize; column <= filterSize; column++) {
                        average = average + getPixelValue(r + row, c + column, COLS, sourceImage);
                    }
                }
                convolutionImage[r * COLS + c] = average / ((filterSize*2+1)*(filterSize*2+1));
            }
        }
        clock_gettime(CLOCK_REALTIME, &end);
        printf(" [#%d](%ld %ld) |", i+1, (long int)end.tv_sec, end.tv_nsec);
        timeSpent += end.tv_nsec - start.tv_nsec;
    }
    printf("\nAverage time spent performing 2D Convolution : %ld ns\n", (timeSpent/10) < 0 ? -(timeSpent/10) : timeSpent/10);

    return convolutionImage;
}

void separableFilter(void)
{

}

void slidingWindow(void)
{

}

int getPixelValue(int rows, int columns, int COLS, unsigned char* image) 
{
    //if (DEBUG) printf("DEBUG: IN getPixelValue\n");
    return image[columns + rows * COLS];
}

// int readHeader(char* imageFile, int* numberOfRows, int* numberOfCols)
// {
//     static char header[80];
//     int BYTES;
//
//     /* open image for reading */
//     FILE *fpt = fopen(imageFile, "r");
//     if (fpt == NULL) {
//         return 1;
//     }
//     
//     /* read image header (simple 8-bit greyscale PPM only) */
//    if (fscanf(fpt, "%s %d %d %d\n", header, &(*numberOfCols), &(*numberOfRows), &BYTES) != 4 || strcmp(header, "P5") != 0 || BYTES != 255)
//    {
//        fclose(fpt);
//        return 2;
//    }
//    return 0;
// }


void readImage(unsigned char* destination, int* ROWS, int* COLS, char* source)
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
    if (fscanf(fpt, "%s %d %d %d\n", header, &(*COLS), &(*ROWS), &BYTES) != 4 || strcmp(header, "P5") != 0 || BYTES != 255)
    {
        fclose(fpt);
        printf("Image header corrupted.\n");
        exit(0);
    }

    destination = createImage((*ROWS)*(*COLS)); // Create an empty image that is large enough for ROWS x COLS bytes
    
    fread(destination, 1, (*ROWS) * (*COLS), fpt);
    fclose(fpt);
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