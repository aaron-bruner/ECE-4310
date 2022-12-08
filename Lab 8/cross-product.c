
	/*
	** Coding challenge:  find the vector that is orthogonal
	** to the given three coordinates
	**
	** cross product formula:
	** https://www.mathsisfun.com/algebra/vectors-cross-product.html 
	*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main (int argc, char *argv[])

{
double	x0,y0,z0,x1,y1,z1,x2,y2,z2;
double	ax,ay,az,bx,by,bz,cx,cy,cz;

	/*
	** three coordinates:
	** zero is at the origin
	** one points along the x axis
	** two points along the y axis
	*/
x0=0; y0=0; z0=0;
x1=1; y1=0; z1=0;
x2=0; y2=1; z2=0;

	/* create two vectors from the three coordinates */
ax=x1-x0;
ay=y1-y0;
az=z1-z0;

bx=x2-x0;
by=y2-y0;
bz=z2-z0;

	/* find the cross product of the two vectors */
cx=ay*bz-az*by;
cy=az*bx-ax*bz;
cz=ax*by-ay*bx;

printf("cross vector is %lf %lf %lf\n",cx,cy,cz);

}

