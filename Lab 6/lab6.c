/* File  : lab6.c
   Author: Aaron Bruner
   Class : ECE - 4310 : Introduction to Computer Vision
   Term  : Fall 2022

   Description: The purpose of this assignment is to calculate motion using accelerometers and gyroscopes.

   Required Files:
    * acc_gyro.txt

   Bugs:
    * Currently none
*/

#pragma region definitions

#define secPerRow 0.05
#define sampleRate 20 // Hz
#define gravity 9.81  // m/s^2
#define MIN_WINDOW 1
#define MAX_WINDOW 6
#define ACCELEROMETERS_THRESHOLD 0.01
#define GYROSCOPE_ROLL_THRESHOLD 0.05
#define GYROSCOPE_PITCH_AND_YAW_THRESHOLD 0.005

#define SQR(x) ((x)*(x))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>

struct motionData {
    double time;
    double accX;
    double accY;
    double accZ;
    double pitch;
    double roll;
    double yaw;
};

struct motionData* readCSV(char* motionDataDir, int* fileRows);
double calcVariance(struct motionData *data, int index, int rows, int selection);
void integrate(struct motionData* data, int start, int end, double* outputArr);
void PrintData(FILE* fp, double distance[6], double start_time, double end_time, int start_index, int end_index);

char* defaultMotionDataDir = "acc_gyro.txt";
bool isMoving = false;

#pragma endregion


int main(int argc, char* argv[])
{
    struct motionData* motionData, *rover;
    FILE* resultFPT;
    bool isMoving = false;
    int fileRows, startIndex = 0, endIndex = 0;
    double start_time = 0.0, end_time = 0.0;

    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
    /*                              STEP 1: Read in source data                                    */
    /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */

    // Input is only ./lab6
    if (argc == 1) {
        motionData = readCSV(defaultMotionDataDir, &fileRows);
    }
    // Input is anything else
    else
    {
        printf("Incorrect number of arguments...\nUsage: ./lab6\n");
        exit(0);
    }

    resultFPT = fopen("result.txt", "w");

    double varianceData[MAX_WINDOW];
    double dist_arr[6] = { 0,0,0,0,0,0 };
    rover = motionData;

    for (int a = 0; a < fileRows; a++, rover++)
    {
        for (int b = MIN_WINDOW; b < MAX_WINDOW; b++)
        {
            varianceData[b] = calcVariance(motionData, a, fileRows, b);
        }

        isMoving = (calcVariance(motionData, a, fileRows, 1) > ACCELEROMETERS_THRESHOLD || calcVariance(motionData, a, fileRows, 2) > ACCELEROMETERS_THRESHOLD || calcVariance(motionData, a, fileRows, 3) > ACCELEROMETERS_THRESHOLD ||
            calcVariance(motionData, a, fileRows, 4) > GYROSCOPE_PITCH_AND_YAW_THRESHOLD || calcVariance(motionData, a, fileRows, 5) > GYROSCOPE_ROLL_THRESHOLD || calcVariance(motionData, a, fileRows, 6) > GYROSCOPE_PITCH_AND_YAW_THRESHOLD) ? true : false;
        //isMoving = (varianceData[1] > ACCELEROMETERS_THRESHOLD || varianceData[2] > ACCELEROMETERS_THRESHOLD || varianceData[3] > ACCELEROMETERS_THRESHOLD ||
        //    varianceData[4] > GYROSCOPE_THRESHOLD || varianceData[5] > GYROSCOPE_THRESHOLD || varianceData[6] > GYROSCOPE_THRESHOLD) ? true : false;

        if (isMoving && start_time == 0.0)
        {
            start_time = rover->time;
            startIndex = a;
        }
        else if (!isMoving && start_time != 0.0 && end_time == 0.0)
        {
            isMoving = false;
            end_time = rover->time;
            endIndex = a;
        }

        if (start_time != 0.0 && end_time != 0.0)
        {
            integrate(motionData, startIndex, endIndex, dist_arr);

            PrintData(resultFPT, dist_arr, start_time, end_time, startIndex, endIndex);
            start_time = 0.0;
            end_time = 0.0;
        }

    }

    fclose(resultFPT);

    return 0;
}

/// <summary>
/// Read in motion data values from CSV file. Except the delimiter is a space
/// 
/// Need to read the header row first.
/// </summary>
/// <param name="motionDataDir">File directory for CSV file</param>
/// <returns>An array of structures which contain the time, accX, accY, accZ, pitch, roll and yaw.</returns>
struct motionData* readCSV(char* motionDataDir, int *fileRows)
{
    char header[UCHAR_MAX];
    size_t headerSize = UCHAR_MAX;
    double time, accX, accY, accZ, pitch, roll, yaw;
    int i = 0;
    (*fileRows) = 0;
    struct motionData* data;
    FILE* FPT;

    // Open the file for reading
    FPT = fopen(motionDataDir, "r");
    FPT == NULL ? printf("Failed to open %s.\n", motionDataDir), exit(0) : false;

    // Read in header data before reading rows
    fgets(header, headerSize, FPT);

    // Determine the number of rows in the file
    while ((i = fscanf(FPT, "%lf %lf %lf %lf %lf %lf %lf\n", &time, &accX, &accY, &accZ, &pitch, &roll, &yaw)) && !feof(FPT))
        if (i == 7) (*fileRows) += 1;
    // Number of rows + 1 since last row isn't counted
    (*fileRows)++;
    
    // Allocate space for array of structures
    data = calloc((*fileRows), sizeof(struct motionData));

    // Return to the beginning of the file
    rewind(FPT);
    // Read in header data before reading rows
    fgets(header, headerSize, FPT);
    
    // Scan in all columns and rows
    for (i = 0; i <= (*fileRows) && !feof(FPT); i++)
        fscanf(FPT, "%lf %lf %lf %lf %lf %lf %lf\n", &data[i].time, &data[i].accX, &data[i].accY, &data[i].accZ, &data[i].pitch, &data[i].roll, &data[i].yaw);

    fclose(FPT);

    return data;
}

double calcVariance(struct motionData *data, int index, int rows, int selection)
{
    int   i;
    int   local_var_window;
    double mean = 0;
    double var;

    if (index + 10 <= rows)
    {
        local_var_window = index + 10;
    }
    else
    {
        local_var_window = rows;
    }
    for (i = index; i < local_var_window; i++)
    {
        switch (selection)
        {
        case 1:
            mean += data[i].accX;
            break;
        case 2:
            mean += data[i].accY;
            break;
        case 3:
            mean += data[i].accZ;
            break;
        case 4:
            mean += data[i].pitch;
            break;
        case 5:
            mean += data[i].yaw;
            break;
        case 6:
            mean += data[i].roll;
            break;
        }
    }
    mean = mean / (10 + 1);

    for (i = index; i < local_var_window; i++)
    {
        switch (selection)
        {
        case 1:
            var += SQR(data[i].accX - mean);
            break;
        case 2:
            var += SQR(data[i].accY - mean);
            break;
        case 3:
            var += SQR(data[i].accZ - mean);
            break;
        case 4:
            var += SQR(data[i].pitch - mean);
            break;
        case 5:
            var += SQR(data[i].yaw - mean);
            break;
        case 6:
            var += SQR(data[i].roll - mean);
            break;
        }
    }
    var = var / (10 + 1);

    return var;
}

void PrintData(FILE* fp, double distance[6], double start_time, double end_time, int start_index, int end_index)
{
    fprintf(fp, "########################################################\n");
    fprintf(fp, "X Movement: %f [m]\nY Movement: %f [m]\nZ Movement: %f [m]\n", distance[0], distance[1], distance[2]);
    fprintf(fp, "Pitch Movement: %f [rad]\nRoll Movement: %f [rad]\nYaw Movement: %f [rad]\n", distance[3], distance[4], distance[5]);
    fprintf(fp, "Start Time: %0.2f\t\tEnd Time: %0.2f\n", start_time, end_time);
    fprintf(fp, "Start Index: %d\t\tEnd Index: %d\n", start_index, end_index);
    fprintf(fp, "\n\n");

    return;
}

void integrate(struct motionData *data, int start, int end, double *outputArr)
{
    int i;
    double prev_velocity = 0.0;
    double velocity = 0.0;
    double distance = 0.0;
    double result = 0.0;

    for (int a = 1; a < 7; a++)
    {
        if (a < 4)
        {
            prev_velocity = velocity = distance =0;
            for (i = start; i <= end; i++)
            {
                prev_velocity = velocity;
                switch (a)
                {
                case 1:
                    velocity += data[i].accX * gravity * secPerRow;
                    break;
                case 2:
                    velocity += data[i].accY * gravity * secPerRow;
                    break;
                case 3:
                    velocity += data[i].accZ * gravity * secPerRow;
                    break;
                }
                distance += ((velocity + prev_velocity) / 2) * secPerRow;
            }
            outputArr[a-1] = distance;
        }
        else
        {
            result = 0;
            for (i = start; i <= end; i++)
            {
                switch (a)
                {
                case 4:
                    result += data[i].pitch * secPerRow;
                    break;
                case 5:
                    result += data[i].yaw * secPerRow;
                    break;
                case 6:
                    result += data[i].roll * secPerRow;
                    break;
                }
            }
            outputArr[a-1] = result;
        }
    }

    return;
}