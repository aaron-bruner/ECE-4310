
#define SQR(x) ((x)*(x))	/* macro for square */
#ifndef M_PI			/* in case M_PI not found in math.h */
#define M_PI 3.1415927
#endif
#ifndef M_E
#define M_E 2.718282
#endif

#define MAX_FILENAME_CHARS	320

char	filename[MAX_FILENAME_CHARS];

HWND	MainWnd;

        // Display flags
int		ShowPixelCoords;
int		color; // User defined color (RGB)
int		Play; // Grow region in play (1 ms)
int		Step; // or in step (J key active)
int     reset; // Halt region grow
int		absoluteDifference; // Predicate one: the absolute difference of the pixel intensity
                            // to the average intensity of pixels already in the region
char diff[50];              // Need a string for return from dialog box
int		centroidDistance;   // Predicate two: the distance of the pixel to the centroid of pixels
                            // already in the region
char dist[50];              // Need a string for return from dialog box
int		mouse_x; // Mouse down X
int		mouse_y; // Mouse down Y
int		JKeyDown;

        // Image data
unsigned char	*OriginalImage;
int				ROWS,COLS;

#define TIMER_SECOND	1			/* ID of timer used for animation */

        // Drawing flags
int		TimerRow,TimerCol;
int		ThreadRow,ThreadCol;
int		ThreadRunning;
int     thredCount;

        // Function prototypes
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
void PaintImage();
void AnimationThread(void *);		/* passes address of window */
void regionGrowSetup();
void regionGrow();