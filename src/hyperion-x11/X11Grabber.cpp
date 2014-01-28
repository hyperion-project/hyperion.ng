
#include <X11/Xlib.h>

int main(int argc, char ** argv)
{
	Display * dspl;
	Drawable drwbl;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;

	XImage * xImage = XGetImage(dspl, drwbl, x, y, width, height, AllPlanes, ZPixmap);
}
