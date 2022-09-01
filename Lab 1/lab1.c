#define True 1
#define False 0
#define DEBUG False

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int getPixelValue(int rows, int columns, int COLS, unsigned char* image);
int readHeader(char* imageFile, int* numberOfRows, int* numberOfCols);
void readImage(unsigned char** destination, int size, char* source);
unsigned char* createImage(int size);
int solution(int r, int c, int COLS, unsigned char* image);

int main(int argc, char* argv[])
{
    unsigned char* image;
    char header[80];
    int filterSize = 0, ROWS, COLS, BYTES;

    // Ensure that the provided CLA are correct and check if the optional filter size is provided
    if (argc < 4) {
        printf("Usage: avg [filename.ppm] [r] [c] (-s) (odd filter size)\n\nExample: avg bridge.ppm 12 321 -s 3\n");
        exit(0);
    }
    else if (argc >= 4) // Check to see if optional flags are available
    {
        if (argc == 6 && strcmp(argv[4], "-s") == 0) // User specified a filter specific filter size
        {
            if (DEBUG) printf("User Specified filter value : %s\n", argv[5]);
            if (atoi(argv[5]) % 2 != 0) // Odd
            {
                filterSize = atoi(argv[5]);
            }
            else // Even
            {
                printf("Error: Filter size must be an odd number.\n");
                exit(0);
            }

            //atoi(argv[5]) % 2 != 0 ? filterSize = atoi(argv[5]) : printf("Error: Invalid filter size\n"), exit(0);
        }
        else // Unless they specify a filter size then we default to 3
        {
            filterSize = 3;
        }
    }

    // Read the provided image and validate that it's the correct format 
    int returnMessage = readHeader(argv[1], &ROWS, &COLS);
    if (returnMessage == 1)
    {
        printf("Unable to open %s for reading\n", argv[1]);
        exit(0);
    }
    else if (returnMessage == 2)
    {
        printf("The file [%s] is not an 8-bit PPM greyscale (P5) image\n", argv[1]);
        exit(0);
    }

    // Validate that the CLA for ROW and COL are inside of the image size
    int rowCLA = atoi(argv[2]);
    int colCLA = atoi(argv[3]);

    if (rowCLA < 0 || rowCLA > ROWS) {
        printf("The provided value (%d) is outside of the valid range (0 to %d)\n", rowCLA, ROWS);
        exit(0);
    }
    else if (colCLA < 0 || colCLA > COLS) {
        printf("The provided value (%d) is outside of the valid range (0 to %d)\n", colCLA, COLS);
        exit(0);
    }

    // Create an empty image that is large enough for ROWS x COLS bytes
    image = createImage(ROWS * COLS);

    // Once we've allocated space for our image data we can read it in
    readImage(&image, ROWS * COLS, argv[1]);

    int average = 0;
    /* Determine the average of a 3x3 matrix at pixel [row,column] */
    for (int row = -1; row <= 1; row++)
        for (int column = -1; column <= 1; column++) {
            average = average + getPixelValue(row + rowCLA, column + colCLA, COLS, image);
            if (DEBUG) printf("PROCESS - Pixel value for dr(%d) dc(%d) = %d\n", row, column, getPixelValue(row + rowCLA, column + colCLA, COLS, image));
        }
    printf("The average (3x3) at [%d,%d] is = %d\n", rowCLA, colCLA, average / 9);
    printf("The solution is %d\n", solution(rowCLA, colCLA, COLS, image));
}

int getPixelValue(int rows, int columns, int COLS, unsigned char* image) 
{
    if (DEBUG) printf("DEBUG: IN getPixelValue\n");
    return image[columns + rows * COLS];
}

int readHeader(char* imageFile, int* numberOfRows, int* numberOfCols)
{
    if (DEBUG) printf("DEBUG: IN readHeader\n");
    static char header[80];
    int BYTES;

    /* open image for reading */
    FILE *fpt = fopen(imageFile, "r");
    if (fpt == NULL) {
        return 1;
    }
    
    /* read image header (simple 8-bit greyscale PPM only) */
    if (fscanf(fpt, "%s %d %d %d", header, &(*numberOfCols), &(*numberOfRows), &BYTES) != 4 || strcmp(header, "P5") != 0 || BYTES != 255) 
    {
        fclose(fpt);
        return 2;
    }
    return 0;
}

void readImage(unsigned char** destination, int size, char* source)
{
    if (DEBUG) printf("IN readImage\n");
    // Open image for reading
    FILE *fpt = fopen(source, "r");
    if (fpt == NULL) {
        printf("Failed to open file (%s) for reading.\n", source);
        exit(0);
    }

    fread(*destination, 1, size, fpt);
    fclose(fpt);
}

unsigned char* createImage(int size)
{
    if (DEBUG) printf("IN createImage\n");
    unsigned char* newImage;

    /* allocate dynamic memory for image */
    newImage = (unsigned char*)calloc(size, sizeof(unsigned char));
    if (newImage == NULL) {
        printf("Unable to allocate %d bytes of memory.\n", size);
        exit(0);
    }

    return newImage;
}

int solution(int r, int c, int COLS, unsigned char* image) {
    if (DEBUG) printf("IN solution");
    /* calculate avg pixel value around 3x3 window of r,c */
    int avg = 0;
    for (int dr = -1; dr <= +1; dr++)
    {
        for (int dc = -1; dc <= +1; dc++)
        {
            avg = avg + image[(r + dr) * COLS + c + dc];
            if (DEBUG) printf("SOLUTION - Pixel value for dr(%d) dc(%d) = %d\n", dr, dc, image[(r + dr) * COLS + c + dc]);
        }
    }
    return avg / 9;
}
