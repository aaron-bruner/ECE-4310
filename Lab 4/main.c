/* File  : main.c
   Author: Aaron Bruner
   Class : ECE - 4310 : Introduction to Computer Vision
   Term  : Fall 2022

   Description: The purpose of this project is to implement an interactive region growing. The program
                will build upon two pieces of code given at the web site. The plus program allows the
                user to load and display an image, and demonstrates several GUI and event handling
                techniques. The region growing code demonstrates growing a region based upon several
                predicates. These pieces must be integrated into a new program that allows the user to
                click any location in an image and visualize the results of growing a region there.

   Required Files:
    * main.c
    * globals.h
    * resources.h
    * interface.rc
   Optional Images:
    * bridge.ppm
    * targets.ppm

   Bugs:
    * None that I am aware of.
*/

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <process.h>	/* needed for multithreading */
#include "resource.h"
#include "globals.h"


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow)

{
    MSG msg;
    HWND hWnd;
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, "ID_PLUS_ICON");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = "ID_MAIN_MENU";
    wc.lpszClassName = "PLUS";

    if (!RegisterClass(&wc))
        return (FALSE);

    hWnd = CreateWindow("PLUS", "plus program",
        WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
        CW_USEDEFAULT, 0, 400, 400, NULL, NULL, hInstance, NULL);
    if (!hWnd)
        return (FALSE);

    ShowScrollBar(hWnd, SB_BOTH, FALSE);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    MainWnd = hWnd;

    ShowPixelCoords = 0;
    color = 0; // User defined color (RGB)
    Play = 0; // Grow region in play (1 ms)
    Step = 0; // or in step (J key active)
    reset = 0; // Halt region grow
    absoluteDifference = 0; // Predicate one: the absolute difference of the pixel intensity
                            // to the average intensity of pixels already in the region
    centroidDistance = 0;   // Predicate two: the distance of the pixel to the centroid of pixels
                            // already in the region
    mouse_x = 0; // Mouse down X
    mouse_y = 0; // Mouse down Y
    JKeyDown = 0;
    thredCount = 0;
    //BigDots = 0;

    strcpy(filename, "");
    OriginalImage = NULL;
    ROWS = COLS = 0;

    InvalidateRect(hWnd, NULL, TRUE);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (msg.wParam);
}

// https://learn.microsoft.com/en-us/windows/win32/dlgbox/using-dialog-boxes#creating-a-modeless-dialog-box
// Absolute difference
LRESULT CALLBACK WndProcDiff(HWND hWnd, UINT uMsg,
    WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            GetDlgItemText(hWnd, IDC_ABS_DIFFERENCE, diff, 256);
            centroidDistance = atoi(diff);
            EndDialog(hWnd, wParam);
            break;

        case IDCANCEL:
            EndDialog(hWnd, wParam);
            break;
        }
        break;
    }
    return (0L);
}

// https://learn.microsoft.com/en-us/windows/win32/dlgbox/using-dialog-boxes#creating-a-modeless-dialog-box
// Centroid Distance
LRESULT CALLBACK WndProcDist(HWND hWnd, UINT uMsg,
    WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            GetDlgItemText(hWnd, IDC_CENTROID_DISTANCE, dist, 256);
            centroidDistance = atoi(dist);
            EndDialog(hWnd, wParam);
            break;

        case IDCANCEL:
            EndDialog(hWnd, wParam);
            break;
        }
        break;
    }
    return (0L);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg,
    WPARAM wParam, LPARAM lParam)
{
    HMENU hMenu;
    OPENFILENAME ofn;
    FILE* fpt;
    HDC hDC;
    char header[320], text[320];
    int BYTES, xPos, yPos;

    switch (uMsg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_SHOWPIXELCOORDS:
            ShowPixelCoords = (ShowPixelCoords + 1) % 2;
            PaintImage();
            break;
        case ID_FILE_LOAD:
            if (OriginalImage != NULL) {
                free(OriginalImage);
                OriginalImage = NULL;
            }
            memset(&(ofn), 0, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.lpstrFile = filename;
            filename[0] = 0;
            ofn.nMaxFile = MAX_FILENAME_CHARS;
            ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
            ofn.lpstrFilter = "PPM files\0*.ppm\0All files\0*.*\0\0";
            if (!(GetOpenFileName(&ofn)) || filename[0] == '\0')
                break; /* user cancelled load */
            if ((fpt = fopen(filename, "rb")) == NULL) {
                MessageBox(NULL, "Unable to open file", filename, MB_OK | MB_APPLMODAL);
                break;
            }
            fscanf(fpt, "%s %d %d %d", header, &COLS, &ROWS, &BYTES);
            if (strcmp(header, "P5") != 0 || BYTES != 255) {
                MessageBox(NULL, "Not a PPM (P5 greyscale) image", filename, MB_OK | MB_APPLMODAL);
                fclose(fpt);
                break;
            }
            OriginalImage = (unsigned char*)calloc(ROWS * COLS, 1);
            header[0] = fgetc(fpt); /* whitespace character after header */
            fread(OriginalImage, 1, ROWS * COLS, fpt);
            fclose(fpt);
            SetWindowText(hWnd, filename);
            PaintImage();
            break;

        case ID_FILE_QUIT:
            DestroyWindow(hWnd);
            break;

        // New Cases
        case ID_COLOR_RED:
            color = 1;
            break;

        case ID_COLOR_GREEN:
            color = 2;
            break;

        case ID_COLOR_BLUE:
            color = 3;
            break;

        // https://learn.microsoft.com/en-us/windows/win32/dlgbox/using-dialog-boxes#creating-a-modal-dialog-box
        case ID_REGIONGROW_ABSOLUTEDIFFERENCE:
            DialogBox(NULL, MAKEINTRESOURCE(IDD_DIFFERENCE), hWnd, WndProcDiff);
            break;

        case ID_REGIONGROW_CENTROIDDISTANCE:
            DialogBox(NULL, MAKEINTRESOURCE(IDD_DISTANCE), hWnd, WndProcDist);
            break;

        case ID_GROWSTYLE_PLAY:
            Play = (Play + 1) % 2;
            Step = 0;
            break;

        case ID_GROWSTYLE_STEP:
            Step = (Step + 1) % 2;
            Play = 0;
            break;

        case ID_DISPLAY_SHOWORIGINALIMAGE:
            PaintImage();
            reset = 1;
            Play = Step = 0;
            break;
        }
        break;
    case WM_SIZE:
        /* could be used to detect when window size changes */
        PaintImage();
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
        break;
    case WM_PAINT:
        PaintImage();
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
        break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        mouse_x = LOWORD(lParam);
        mouse_y = HIWORD(lParam);
        //_beginthread(ExplosionThread, 0, MainWnd);	/* start up a child thread to do other work while this thread continues GUI */
        if (Step != 0 || Play != 0)
        {
            _beginthread(regionGrowSetup, 0, MainWnd);
            thredCount++;
        }
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
        break;
    case WM_MOUSEMOVE:
        if (ShowPixelCoords == 1) {
            xPos = LOWORD(lParam);
            yPos = HIWORD(lParam);
            if (xPos >= 0 && xPos < COLS && yPos >= 0 && yPos < ROWS) {
                sprintf(text, "%d,%d=>%d     ", xPos, yPos, OriginalImage[yPos * COLS + xPos]);
                hDC = GetDC(MainWnd);
                TextOut(hDC, 0, 0, text, strlen(text)); /* draw text on the window */
                if (color == 0) // CLEMSON ORANGE
                    SetPixel(hDC, xPos, yPos, RGB(245, 102, 0)); /* color the cursor position CLEMSON ORANGE! */
                if (color == 1) // RED
                    SetPixel(hDC, xPos, yPos, RGB(255, 0, 0)); /* color the cursor position red */
                if (color == 2) // GREEN
                    SetPixel(hDC, xPos, yPos, RGB(0, 255, 0)); /* color the cursor position green */
                if (color == 3) // BLUE
                    SetPixel(hDC, xPos, yPos, RGB(0, 0, 255)); /* color the cursor position blue */
                ReleaseDC(MainWnd, hDC);
            }
        }
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
        break;
    case WM_KEYDOWN:
        if (wParam == 'j' || wParam == 'J')
            JKeyDown = 1;

        if (wParam == 's' || wParam == 'S')
            PostMessage(MainWnd, WM_COMMAND, ID_SHOWPIXELCOORDS, 0); /* send message to self */
        if ((TCHAR)wParam == '1') {
            TimerRow = TimerCol = 0;
            SetTimer(MainWnd, TIMER_SECOND, 10, NULL); /* start up 10 ms timer */
        }
        if ((TCHAR)wParam == '2') {
            KillTimer(MainWnd, TIMER_SECOND); /* halt timer, stopping generation of WM_TIME events */
            PaintImage(); /* redraw original image, erasing animation */
        }
        if ((TCHAR)wParam == '3') {
            ThreadRunning = 1;
            _beginthread(AnimationThread, 0, MainWnd); /* start up a child thread to do other work while this thread continues GUI */
        }
        if ((TCHAR)wParam == '4') {
            ThreadRunning = 0; /* this is used to stop the child thread (see its code below) */
        }
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
        break;
    case WM_TIMER:
        /* this event gets triggered every time the timer goes off */
        hDC = GetDC(MainWnd);
        SetPixel(hDC, TimerCol, TimerRow, RGB(0, 0, 255)); /* color the animation pixel blue */
        ReleaseDC(MainWnd, hDC);
        TimerRow++;
        TimerCol += 2;
        break;
    case WM_HSCROLL:
        /* this event could be used to change what part of the image to draw */
        PaintImage(); /* direct PaintImage calls eliminate flicker; the alternative is InvalidateRect(hWnd,NULL,TRUE); UpdateWindow(hWnd); */
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
        break;
    case WM_VSCROLL:
        /* this event could be used to change what part of the image to draw */
        PaintImage();
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
        break;
    }

    hMenu = GetMenu(MainWnd);
    if (ShowPixelCoords == 1)
        CheckMenuItem(hMenu, ID_SHOWPIXELCOORDS, MF_CHECKED); /* you can also call EnableMenuItem() to grey(disable) an option */
    else
        CheckMenuItem(hMenu, ID_SHOWPIXELCOORDS, MF_UNCHECKED);

    // Need to switch on and off check boxes as options are changed
    (Play == 1) ? CheckMenuItem(hMenu, ID_GROWSTYLE_PLAY, MF_CHECKED) : CheckMenuItem(hMenu, ID_GROWSTYLE_PLAY, MF_UNCHECKED);
    (Step == 1) ? CheckMenuItem(hMenu, ID_GROWSTYLE_STEP, MF_CHECKED) : CheckMenuItem(hMenu, ID_GROWSTYLE_STEP, MF_UNCHECKED);
    (color == 0) ? CheckMenuItem(hMenu, ID_COLOR_RED, MF_UNCHECKED), CheckMenuItem(hMenu, ID_COLOR_GREEN, MF_UNCHECKED), CheckMenuItem(hMenu, ID_COLOR_BLUE, MF_UNCHECKED) : color;
    (color == 1) ? CheckMenuItem(hMenu, ID_COLOR_RED, MF_CHECKED), CheckMenuItem(hMenu, ID_COLOR_GREEN, MF_UNCHECKED), CheckMenuItem(hMenu, ID_COLOR_BLUE, MF_UNCHECKED)
                    : CheckMenuItem(hMenu, ID_COLOR_RED, MF_UNCHECKED);
    (color == 2) ? CheckMenuItem(hMenu, ID_COLOR_RED, MF_UNCHECKED), CheckMenuItem(hMenu, ID_COLOR_GREEN, MF_CHECKED), CheckMenuItem(hMenu, ID_COLOR_BLUE, MF_UNCHECKED)
                    : CheckMenuItem(hMenu, ID_COLOR_GREEN, MF_UNCHECKED);
    (color == 3) ? CheckMenuItem(hMenu, ID_COLOR_RED, MF_UNCHECKED), CheckMenuItem(hMenu, ID_COLOR_GREEN, MF_UNCHECKED), CheckMenuItem(hMenu, ID_COLOR_BLUE, MF_CHECKED)
                    : CheckMenuItem(hMenu, ID_COLOR_BLUE, MF_UNCHECKED);

    DrawMenuBar(hWnd);

    return (0L);
}

// Source - reg-grow.c
void regionGrowSetup()
{
    unsigned char* image, * labels;
    //FILE* fpt;
    //char		header[80];
    int		/*ROWS, COLS, BYTES,*/ r, c, r2, c2;
    int* indices, i;
    int		RegionSize, /** RegionPixels,*/ TotalRegions;
    double		avg, var;
    void		RegionGrow();

    r = mouse_y;
    c = mouse_x;

    /* Allocate memory for images.  Read image (raw grey). */
    image = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
    image = OriginalImage;
    /* segmentation image = labels; calloc initializes all labels to 0 */
    labels = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
    /* used to quickly erase small grown regions */
    indices = (int*)calloc(ROWS * COLS, sizeof(int));


    TotalRegions = 0;
    avg = var = 0.0;	/* compute average and variance in 7x7 window */
    for (r2 = -3; r2 <= 3; r2++)
        for (c2 = -3; c2 <= 3; c2++)
            avg += (double)(image[(r + r2) * COLS + (c + c2)]);
    avg /= 49.0;
    for (r2 = -3; r2 <= 3; r2++)
        for (c2 = -3; c2 <= 3; c2++)
            var += SQR(avg - (double)image[(r + r2) * COLS + (c + c2)]);
    var = sqrt(var) / 49.0;
    if (var < 1.0)	/* condition for seeding a new region is low var */
    {
        // printf("%d,%d avg=%lf var=%lf\n",r,c,avg,var);
        TotalRegions++;
        RegionGrow(image, labels, ROWS, COLS, r, c, 0, TotalRegions,
            indices, &RegionSize);
        if (RegionSize < 100)
        {	/* erase region (relabel pixels back to 0) */
            for (i = 0; i < RegionSize; i++)
                labels[indices[i]] = 0;
            TotalRegions--;
        }
    }
    (thredCount > 0) ? thredCount-- : thredCount;
}

/*
* Source : reg-grow.c
* 
** Given an image, a starting point, and a label, this routine
** paint-fills (8-connected) the area with the given new label
** according to the given criteria (pixels close to the average
** intensity of the growing region are allowed to join).
*/

#define MAX_QUEUE 10000	/* max perimeter size (pixels) of border wavefront */

void RegionGrow(unsigned char* image,	/* image data */
    unsigned char* labels,	/* segmentation labels */
    int ROWS, int COLS,	/* size of image */
    int r, int c,		/* pixel to paint from */
    int paint_over_label,	/* image label to paint over */
    int new_label,		/* image label for painting */
    int* indices,		/* output:  indices of pixels painted */
    int* count)		/* output:  count of pixels painted */
{
    int	myself = thredCount, orig_x, orig_y, r2, c2;
    int	queue[MAX_QUEUE], qh, qt;
    int	average, total;	/* average and total intensity in growing region */
    double len = 0;
    HDC hDC = GetDC(MainWnd);

    *count = 0;
    if (labels[r * COLS + c] != paint_over_label)
        return;
    labels[r * COLS + c] = new_label;
    average = total = (int)image[r * COLS + c];
    if (indices != NULL)
        indices[0] = r * COLS + c;
    queue[0] = r * COLS + c;
    qh = 1;	/* queue head */
    qt = 0;	/* queue tail */
    orig_x = r; // Need to record the original center ROWS
    orig_y = c; //				  and original center COLS
    (*count) = 1;
    while (qt != qh)
    {
        // End all threads if the image gets reset
        while (reset == 1 && (thredCount == myself))
        {
            reset = 0;
            _endthread();
        }

        if ((*count) % 50 == 0)	/* recalculate average after each 50 pixels join */
        {
            average = total / (*count);
            // printf("new avg=%d\n",average);
        }

        // Only run if Step or Play is active
        if (Step != 0 || Play != 0)
        {
        for (r2 = -1; r2 <= 1; r2++)
            for (c2 = -1; c2 <= 1; c2++)
            {
                if (r2 == 0 && c2 == 0)
                    continue;
                if ((queue[qt] / COLS + r2) < 0 || (queue[qt] / COLS + r2) >= ROWS ||
                    (queue[qt] % COLS + c2) < 0 || (queue[qt] % COLS + c2) >= COLS)
                    continue;
                if (labels[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2] != paint_over_label)
                    continue;
                /* test criteria to join region */
                len = sqrt(SQR((queue[qt] / COLS + r2) - orig_x) + SQR(((queue[qt] % COLS + c2) - orig_y)));
                if ((abs((int)(image[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2])
                    - average) > centroidDistance) || (len > centroidDistance))
                    continue;

                (color == 0) ? SetPixel(hDC, queue[qt] % COLS + c2, queue[qt] / COLS + r2, RGB(245, 102, 0)) : color; // CLEMSON ORANGE
                (color == 1) ? SetPixel(hDC, queue[qt] % COLS + c2, queue[qt] / COLS + r2, RGB(255,   0, 0)) : color; // RED
                (color == 2) ? SetPixel(hDC, queue[qt] % COLS + c2, queue[qt] / COLS + r2, RGB(  0, 255, 0)) : color; // GREEN
                (color == 3) ? SetPixel(hDC, queue[qt] % COLS + c2, queue[qt] / COLS + r2, RGB(  0, 0, 255)) : color; // BLUE

                labels[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2] = new_label;
                if (indices != NULL)
                    indices[*count] = (queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2;
                total += image[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2];
                (*count)++;
                queue[qh] = (queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2;
                qh = (qh + 1) % MAX_QUEUE;
                if (qh == qt)
                {
                    printf("Max queue size exceeded\n");
                    exit(0);
                }
            }
        }
        else
        {
            // Turned off mid grow
            return;
        }
        qt = (qt + 1) % MAX_QUEUE;
        if (Play == 1)
        {
            _sleep(1); // Play will sleep for 1 ms and then continue
        }
        else if (Step == 1)
        {
            // For step we wait until J is pressed to continue
            while (JKeyDown == 0) _sleep(1);
            JKeyDown = 0;
        }
    }
    reset = 0;
}

void PaintImage()
{
    PAINTSTRUCT Painter;
    HDC hDC;
    BITMAPINFOHEADER bm_info_header;
    BITMAPINFO* bm_info;
    int i, r, c, DISPLAY_ROWS, DISPLAY_COLS;
    unsigned char* DisplayImage;

    if (OriginalImage == NULL)
        return; /* no image to draw */

    /* Windows pads to 4-byte boundaries.  We have to round the size up to 4 in each dimension, filling with black. */
    DISPLAY_ROWS = ROWS;
    DISPLAY_COLS = COLS;
    if (DISPLAY_ROWS % 4 != 0)
        DISPLAY_ROWS = (DISPLAY_ROWS / 4 + 1) * 4;
    if (DISPLAY_COLS % 4 != 0)
        DISPLAY_COLS = (DISPLAY_COLS / 4 + 1) * 4;
    DisplayImage = (unsigned char*)calloc(DISPLAY_ROWS * DISPLAY_COLS, 1);
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            DisplayImage[r * DISPLAY_COLS + c] = OriginalImage[r * COLS + c];

    BeginPaint(MainWnd, &Painter);
    hDC = GetDC(MainWnd);
    bm_info_header.biSize = sizeof(BITMAPINFOHEADER);
    bm_info_header.biWidth = DISPLAY_COLS;
    bm_info_header.biHeight = -DISPLAY_ROWS;
    bm_info_header.biPlanes = 1;
    bm_info_header.biBitCount = 8;
    bm_info_header.biCompression = BI_RGB;
    bm_info_header.biSizeImage = 0;
    bm_info_header.biXPelsPerMeter = 0;
    bm_info_header.biYPelsPerMeter = 0;
    bm_info_header.biClrUsed = 256;
    bm_info_header.biClrImportant = 256;
    bm_info = (BITMAPINFO*)calloc(1, sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD));
    bm_info->bmiHeader = bm_info_header;
    for (i = 0; i < 256; i++) {
        bm_info->bmiColors[i].rgbBlue = bm_info->bmiColors[i].rgbGreen = bm_info->bmiColors[i].rgbRed = i;
        bm_info->bmiColors[i].rgbReserved = 0;
    }

    SetDIBitsToDevice(hDC, 0, 0, DISPLAY_COLS, DISPLAY_ROWS, 0, 0,
        0, /* first scan line */
        DISPLAY_ROWS, /* number of scan lines */
        DisplayImage, bm_info, DIB_RGB_COLORS);
    ReleaseDC(MainWnd, hDC);
    EndPaint(MainWnd, &Painter);

    free(DisplayImage);
    free(bm_info);
}

void AnimationThread(HWND AnimationWindowHandle)
{
    HDC hDC;
    char text[300];

    ThreadRow = ThreadCol = 0;
    while (ThreadRunning == 1) {
        hDC = GetDC(MainWnd);
        // Color = 0 (idk, shouldn't be 0)
        (color == 0) ? SetPixel(hDC, ThreadCol, ThreadRow, RGB(245, 102, 0)) : color; /* color the animation pixel CLEMSON ORANGE! */
        // Color = 1 (Red)
        (color == 1) ? SetPixel(hDC, ThreadCol, ThreadRow, RGB(255, 0, 0)) : color; /* color the animation pixel red */
        // Color = 2 (Green)
        (color == 2) ? SetPixel(hDC, ThreadCol, ThreadRow, RGB(0, 255, 0)) : color; /* color the animation pixel green */
        // Color = 3 (Blue)
        (color == 3) ? SetPixel(hDC, ThreadCol, ThreadRow, RGB(0, 0, 255)) : color; /* color the animation pixel green */
        sprintf(text, "%d,%d     ", ThreadRow, ThreadCol);
        TextOut(hDC, 300, 0, text, strlen(text)); /* draw text on the window */
        ReleaseDC(MainWnd, hDC);
        ThreadRow += 3;
        ThreadCol++;
        Sleep(100); /* pause 100 ms */
    }
}