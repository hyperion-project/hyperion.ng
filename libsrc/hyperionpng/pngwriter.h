//**********  pngwriter.h   **********************************************
//  Author:                    Paul Blackburn
//
//  Email:                     individual61@users.sourceforge.net
//
//  Version:                   0.5.4   (19 / II / 2009)
//
//  Description:               Library that allows plotting a 48 bit
//                             PNG image pixel by pixel, which can 
//                             then be opened with a graphics program.
//  
//  License:                   GNU General Public License
//                             Copyright 2002, 2003, 2004, 2005, 2006, 2007,
//                             2008, 2009 Paul Blackburn
//                             
//  Website: Main:             http://pngwriter.sourceforge.net/
//           Sourceforge.net:  http://sourceforge.net/projects/pngwriter/
//           Freshmeat.net:    http://freshmeat.net/projects/pngwriter/
//           
//  Documentation:             This header file is commented, but for a
//                             quick reference document, and support,
//                             take a look at the website.
//
//*************************************************************************


/*
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software
 *     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * */

#ifndef PNGWRITER_H
#define PNGWRITER_H 1

#define PNGWRITER_VERSION 0.54

#include <png.h>

// REMEMBER TO ADD -DNO_FREETYPE TO YOUR COMPILATION FLAGS IF PNGwriter WAS
// COMPILED WITHOUT FREETYPE SUPPORT!!!
// 
// RECUERDA AGREGAR -DNO_FREETYPE A TUS OPCIONES DE COMPILACION SI PNGwriter 
// FUE COMPILADO SIN SOPORTE PARA FREETYPE!!!
// 
#ifndef NO_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif



#ifdef OLD_CPP // For compatibility with older compilers.
#include <iostream.h>
#include <math.h>
#include <wchar.h>
#include <string.h>
using namespace std;
#endif // from ifdef OLD_CPP

#ifndef OLD_CPP // Default situation.
#include <iostream>
#include <cmath>
#include <cwchar>
#include <string>
#endif // from ifndef OLD_CPP


//png.h must be included before FreeType headers.
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>




#define PNG_BYTES_TO_CHECK (4)
#define PNGWRITER_DEFAULT_COMPRESSION (6)

class pngwriter 
{
 private:
   
   char * filename_;   
   char * textauthor_;   
   char * textdescription_;   
   char * texttitle_;   
   char * textsoftware_;   


   
   int height_;
   int width_;
   int  backgroundcolour_;
   int bit_depth_;
   int rowbytes_;
   int colortype_;
   int compressionlevel_;
   bool transformation_; // Required by Mikkel's patch
   
   unsigned char * * graph_;
   double filegamma_;
   double screengamma_;
   void circle_aux(int xcentre, int ycentre, int x, int y, int red, int green, int blue);
   void circle_aux_blend(int xcentre, int ycentre, int x, int y, double opacity, int red, int green, int blue);
   int check_if_png(char *file_name, FILE **fp);
   int read_png_info(FILE *fp, png_structp *png_ptr, png_infop *info_ptr);
   int read_png_image(FILE *fp, png_structp png_ptr, png_infop info_ptr,
 		       png_bytepp *image, png_uint_32 *width, png_uint_32 *height);
   void flood_fill_internal( int xstart, int ystart,  double start_red, double start_green, double start_blue, double fill_red, double fill_green, double fill_blue);
   void flood_fill_internal_blend( int xstart, int ystart, double opacity,  double start_red, double start_green, double start_blue, double fill_red, double fill_green, double fill_blue);

#ifndef NO_FREETYPE
   void my_draw_bitmap( FT_Bitmap * bitmap, int x, int y, double red, double green, double blue);
   void my_draw_bitmap_blend( FT_Bitmap * bitmap, int x, int y,double opacity,  double red, double green, double blue);
#endif
   
   /* The algorithms HSVtoRGB and RGBtoHSV were found at http://www.cs.rit.edu/~ncs/
    * which is a page that belongs to Nan C. Schaller, though
    * these algorithms appear to be the work of Eugene Vishnevsky. 
    * */
   void HSVtoRGB( double *r, double *g, double *b, double h, double s, double v ); 
   void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v );

   /* drwatop(), drawbottom() and filledtriangle() were contributed by Gurkan Sengun
    * ( <gurkan@linuks.mine.nu>, http://www.linuks.mine.nu/ )
    * */
   void drawtop(long x1,long y1,long x2,long y2,long x3, int red, int green, int blue);
   void drawbottom(long x1,long y1,long x2,long x3,long y3, int red, int green, int blue);
   void drawbottom_blend(long x1,long y1,long x2,long x3,long y3, double opacity, int red, int green, int blue);
   void drawtop_blend(long x1,long y1,long x2,long y2,long x3, double opacity, int red, int green, int blue);
   
 public:

   /* General Notes
    * It is important to remember that all functions that accept an argument of type "const char *" will also
    * accept "char *", this is done so you can have a changing filename (to make many PNG images in series 
    * with a different name, for example), and to allow you to use string type objects which can be easily 
    * turned into const char * (if theString is an object of type string, then it can be used as a const char *
    * by saying theString.c_str()).
    * It is also important to remember that whenever a function has a colour coeffiecient as its argument, 
    * that argument can be either an int from 0 to 65535 or a double from 0.0 to 1.0. 
    * It is important to make sure that you are calling the function with the type that you want.
    * Remember that 1 is an int, while 1.0 is a double, and will thus determine what version of the function 
    * will be used. Similarly, do not make the mistake of calling for example plot(x, y, 0.0, 0.0, 65535),
    * because
    * there is no plot(int, int, double, double, int).
    * Also, please note that plot() and read() (and the functions that use them internally) 
    * are protected against entering, for example, a colour coefficient that is over 65535
    * or over 1.0. Similarly, they are protected against negative coefficients. read() will return 0
    * when called outside the image range. This is actually useful as zero-padding should you need it.
    * */

   /* Compilation
    * A typical compilation would look like this:
    * 
    * g++ my_program.cc -o my_program freetype-config --cflags \
    *          -I/usr/local/include  -L/usr/local/lib -lpng -lpngwriter -lz -lfreetype
    * 
    * If you did not compile PNGwriter with FreeType support, then remove the
    * FreeType-related flags and add -DNO_FREETYPE above.
    * */
   
   /* Constructor
    * The constructor requires the width and the height of the image, the background colour for the
    * image and the filename of the file (a pointer or simple "myfile.png"). The background colour
    * can only be initialized to a shade of grey (once the object has been created you can do whatever 
    * you want, though), because generally one wants either a white (65535 or 1.0) or a black (0 or 0.0)
    * background to start with.
    * The default constructor creates a PNGwriter instance that is 250x250, white background,
    * and filename "out.png".
    * Tip: The filename can be given as easily as:
    * pngwriter mypng(300, 300, 0.0, "myfile.png");    
    * Tip: If you are going to create a PNGwriter instance for reading in a file that already exists, 
    * then width and height can be 1 pixel, and the size will be automatically adjusted once you use
    * readfromfile().
    * */
   pngwriter();   
   pngwriter(const pngwriter &rhs);
   pngwriter(int width, int height, int backgroundcolour, char * filename);   
   pngwriter(int width, int height, double backgroundcolour, char * filename);    
   pngwriter(int width, int height, int backgroundcolour, const char * filename);   
   pngwriter(int width, int height, double backgroundcolour, const char * filename);    

   /* Destructor
    * */
   ~pngwriter();  

   /* Assignment Operator
    * */
   pngwriter & operator = (const pngwriter & rhs);
      
   /*  Plot
    * With this function a pixel at coordinates (x, y) can be set to the desired colour. 
    * The pixels are numbered starting from (1, 1) and go to (width, height). 
    * As with most functions in PNGwriter, it has been overloaded to accept either int arguments 
    * for the colour coefficients, or those of type double. If they are of type int, 
    * they go from 0 to 65535. If they are of type double, they go from 0.0 to 1.0.
    * Tip: To plot using red, then specify plot(x, y, 1.0, 0.0, 0.0). To make pink, 
    * just add a constant value to all three coefficients, like this:
    * plot(x, y, 1.0, 0.4, 0.4). 
    * Tip: If nothing is being plotted to your PNG file, make sure that you remember
    * to close() the instance before your program is finished, and that the x and y position
    * is actually within the bounds of your image. If either is not, then PNGwriter will 
    * not complain-- it is up to you to check for this!
    * Tip: If you try to plot with a colour coefficient out of range, a maximum or minimum
    * coefficient will be assumed, according to the given coefficient. For example, attempting
    * to plot plot(x, y, 1.0,-0.2,3.7) will set the green coefficient to 0 and the red coefficient
    * to 1.0.
    * */
   void  plot(int x, int y, int red, int green, int blue); 
   void  plot(int x, int y, double red, double green, double blue); 
                                                          
   /* Plot HSV
    * With this function a pixel at coordinates (x, y) can be set to the desired colour, 
    * but with the colour coefficients given in the Hue, Saturation, Value colourspace. 
    * This has the advantage that one can determine the colour that will be plotted with 
    * only one parameter, the Hue. The colour coefficients must go from 0 to 65535 and
    * be of type int, or be of type double and go from 0.0 to 1.0.
    * */
   void plotHSV(int x, int y, double hue, double saturation, double value);
   void plotHSV(int x, int y, int hue, int saturation, int value); 

   /* Read
    * With this function we find out what colour the pixel (x, y) is. If "colour" is 1,
    * it will return the red coefficient, if it is set to 2, the green one, and if 
    * it set to 3, the blue colour coefficient will be returned,
    * and this returned value will be of type int and be between 0 and 65535.
    * Note that if you call read() on a pixel outside the image range, the value returned
    * will be 0.
    * */
   int read(int x, int y, int colour);  

   /* Read, Average
    * Same as the above, only that the average of the three colour coefficients is returned.
    */
   int read(int x, int y);              
   
  /* dRead
   * With this function we find out what colour the pixel (x, y) is. If "colour" is 1,
   * it will return the red coefficient, if it is set to 2, the green one, and if 
   * it set to 3, the blue colour coefficient will be returned,
   * and this returned value will be of type double and be between 0.0 and 1.0.
   * Note that if you call dread() outside the image range, the value returned will be 0.0
   * */
   double dread(int x, int y, int colour);   
   
   /* dRead, Average
    * Same as the above, only that the average of the three colour coefficients is returned.
    */
   double dread(int x, int y);             
   
   /* Read HSV
    * With this function we find out what colour the pixel (x, y) is, but in the Hue, 
    * Saturation, Value colourspace. If "colour" is 1,
    * it will return the Hue coefficient, if it is set to 2, the Saturation one, and if 
    * it set to 3, the Value colour coefficient will be returned, and this returned
    * value will be of type int and be between 0 and 65535. Important: If you attempt
    * to read the Hue of a pixel that is a shade of grey, the value returned will be
    * nonsensical or even NaN. This is just the way the RGB -> HSV algorithm works: 
    * the Hue of grey is not defined. You might want to check whether the pixel
    * you are reading is grey before attempting a readHSV().
    * Tip: This is especially useful for categorizing sections of the image according 
    * to their colour. 
    * */
   int readHSV(int x, int y, int colour);  
    
  /* dRead HSV
   * With this function we find out what colour the pixel (x, y) is, but in the Hue, 
   * Saturation, Value colourspace. If "colour" is 1,
   * it will return the Hue coefficient, if it is set to 2, the Saturation one, and if 
   * it set to 3, the Value colour coefficient will be returned,
   * and this returned value will be of type double and be between 0.0 and 1.0.
   * */
   double dreadHSV(int x, int y, int colour);    
   
   /* Clear
    * The whole image is set to black.
    * */ 
   void clear(void);    
   
   /* Close
    * Close the instance of the class, and write the image to disk.
    * Tip: If you do not call this function before your program ends, no image
    * will be written to disk.
    * */
   void close(void); 

   /* Rename
    * To rename the file once an instance of pngwriter has been created.
    * Useful for assigning names to files based upon their content.
    * Tip: This is as easy as calling pngwriter_rename("newname.png")
    * If the argument is a long unsigned int, for example 77, the filename will be changed to
    * 0000000077.png
    * Tip: Use this to create sequences of images for movie generation.
    * */
   void pngwriter_rename(char * newname);               
   void pngwriter_rename(const char * newname);            
   void pngwriter_rename(long unsigned int index);            

   /* Figures
    * These functions draw basic shapes. Available in both int and double versions.
    * The line functions use the fast Bresenham algorithm. Despite the name, 
    * the square functions draw rectangles. The circle functions use a fast 
    * integer math algorithm. The filled circle functions make use of sqrt().
    * */
   void line(int xfrom, int yfrom, int xto, int yto, int red, int green,int  blue);
   void line(int xfrom, int yfrom, int xto, int yto, double red, double green,double  blue);
  
   void triangle(int x1, int y1, int x2, int y2, int x3, int y3, int red, int green, int blue);
   void triangle(int x1, int y1, int x2, int y2, int x3, int y3, double red, double green, double blue);
   
   void square(int xfrom, int yfrom, int xto, int yto, int red, int green,int  blue);
   void square(int xfrom, int yfrom, int xto, int yto, double red, double green,double  blue);
   
   void filledsquare(int xfrom, int yfrom, int xto, int yto, int red, int green,int  blue);
   void filledsquare(int xfrom, int yfrom, int xto, int yto, double red, double green,double  blue);

   void circle(int xcentre, int ycentre, int radius, int red, int green, int blue);
   void circle(int xcentre, int ycentre, int radius, double red, double green, double blue);
   
   void filledcircle(int xcentre, int ycentre, int radius, int red, int green, int blue);
   void filledcircle(int xcentre, int ycentre, int radius, double red, double green, double blue);

   
   /* Read From File
    * Open the existing PNG image, and copy it into this instance of the class. It is important to mention 
    * that PNG variants are supported. Very generally speaking, most PNG files can now be read (as of version 0.5.4), 
    * but if they have an alpha channel it will be completely stripped. If the PNG file uses GIF-style transparency 
    * (where one colour is chosen to be transparent), PNGwriter will not read the image properly, but will not 
    * complain. Also, if any ancillary chunks are included in the PNG file (chroma, filter, etc.), it will render 
    * with a slightly different tonality. For the vast majority of PNGs, this should not be an issue. Note: 
    * If you read an 8-bit PNG, the internal representation of that instance of PNGwriter will be 8-bit (PNG 
    * files of less than 8 bits will be upscaled to 8 bits). To convert it to 16-bit, just loop over all pixels, 
    * reading them into a new instance of PNGwriter. New instances of PNGwriter are 16-bit by default.
    * */

   void readfromfile(char * name);  
   void readfromfile(const char * name); 

   /* Get Height
    * When you open a PNG with readfromfile() you can find out its height with this function.
    * */
   int getheight(void);
   
   /* Get Width
    * When you open a PNG with readfromfile() you can find out its width with this function.
    * */
   int getwidth(void);

   /* Set Compression Level
    * Set the compression level that will be used for the image. -1 is to use the  default,
    * 0 is none, 9 is best compression. 
    * Remember that this will affect how long it will take to close() the image. A value of 2 or 3
    * is good enough for regular use, but for storage or transmission you might want to take the time
    * to set it at 9.
    * */
    void setcompressionlevel(int level);

   /* Get Bit Depth
    * When you open a PNG with readfromfile() you can find out its bit depth with this function.
    * Mostly for troubleshooting uses.
    * */
   int getbitdepth(void);
   
   /* Get Colour Type
    * When you open a PNG with readfromfile() you can find out its colour type (libpng categorizes 
    * different styles of image data with this number).
    * Mostly for troubleshooting uses.
    * */
   int getcolortype(void);
   
   /* Set Gamma Coeff
    * Set the image's gamma (file gamma) coefficient. This is experimental, but use it if your image's colours seem too bright
    * or too dark. The default value of 0.5 should be fine. The standard disclaimer about Mac and PC gamma
    * settings applies.
    * */
   void setgamma(double gamma);

   
   /* Get Gamma Coeff
    * Get the image's gamma coefficient. This is experimental.
    * */
   double getgamma(void);

   /* Bezier Curve
    * (After Frenchman Pierre BŽzier from Regie Renault)
    * A collection of formulae for describing curved lines 
    * and surfaces, first used in 1972 to model automobile surfaces. 
    *             (from the The Free On-line Dictionary of Computing)
    * See http://www.moshplant.com/direct-or/bezier/ for one of many
    * available descriptions of bezier curves.
    * There are four points used to define the curve: the two endpoints
    * of the curve are called the anchor points, while the other points,
    * which define the actual curvature, are called handles or control points.
    * Moving the handles lets you modify the shape of the curve. 
    * */ 
     
   void bezier(  int startPtX, int startPtY,                                                                            
              int startControlX, int startControlY,                                                                                     
              int endPtX, int endPtY,                                                                                                   
              int endControlX, int endControlY,                                                                                         
              double red, double green, double blue);

   void bezier(  int startPtX, int startPtY,                                                                            
              int startControlX, int startControlY,                                                                                     
              int endPtX, int endPtY,                                                                                                   
              int endControlX, int endControlY,                                                                                         
              int red, int green, int blue);

   /* Set Text
    * Sets the text information in the PNG header. If it is not called, the default is used.
    */
   void settext(char * title, char * author, char * description, char * software);
   void settext(const char * title, const char * author, const char * description, const char * software);


   /* Version Number
    * Returns the PNGwriter version number.
    */
  static double version(void);  

   /* Write PNG
    * Writes the PNG image to disk. You can still change the PNGwriter instance after this.
    * Tip: This is exactly the same as close(), but easier to remember.
    * Tip: To make a sequence of images using only one instance of PNGwriter, alter the image, change its name,
    * write_png(), then alter the image, change its name, write_png(), etc.
    */
   void write_png(void);

   /* Plot Text
    * Uses the Freetype2 library to set text in the image. face_path is the file path to a 
    * TrueType font file (.ttf) (FreeType2 can also handle other types). fontsize specifices the approximate
    * height of the rendered font in pixels. x_start and y_start specify the placement of the
    * lower, left corner of the text string. angle is the text angle in radians. text is the text to be rendered.
    * The colour coordinates can be doubles from 0.0 to 1.0 or ints from 0 to 65535.
    * Tip: PNGwriter installs a few fonts in /usr/local/share/pngwriter/fonts to get you started.
    * Tip: Remember to add -DNO_FREETYPE to your compilation flags if PNGwriter was compiled without FreeType support.
    * */
   void plot_text(char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double red, double green, double blue);
   void plot_text(char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, int red, int green, int blue);

   
   /* Plot UTF-8 Text 
    * Same as the above, but the text to be plotted is encoded in UTF-8. Why would you want this? To be able to plot
    * all characters available in a large TrueType font, for example: for rendering Japenese, Chinese and other 
    * languages not restricted to the standard 128 character ASCII space.
    * Tip: The quickest way to get a string into UTF-8 is to write it in an adequate text editor, and save it as a file
    * in UTF-8 encoding, which can then be read in in binary mode.
    * */
   void plot_text_utf8(char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double red, double green, double blue);
   void plot_text_utf8(char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, int red, int green, int blue);


   /* Bilinear Interpolation of Image
    * Given a floating point coordinate (x from 0.0 to width, y from 0.0 to height),
    * this function will return the interpolated colour intensity specified by 
    * colour (where red = 1, green = 2, blue = 3).
    * bilinear_interpolate_read() returns an int from 0 to 65535, and 
    * bilinear_interpolate_dread() returns a double from 0.0 to 1.0.
    * Tip: Especially useful for enlarging an image.
    * */
   int bilinear_interpolation_read(double x, double y, int colour);
   double bilinear_interpolation_dread(double x, double y, int colour);
   
   /* Plot Blend
    * Plots the colour given by red, green blue, but blended with the existing pixel
    * value at that position. opacity is a double that goes from 0.0 to 1.0. 
    * 0.0 will not change the pixel at all, and 1.0 will plot the given colour. 
    * Anything in between will be a blend of both pixel levels. Please note: This is neither 
	* alpha channel nor PNG transparency chunk support. This merely blends the plotted pixels.
    * */
   
   void plot_blend(int x, int y, double opacity, int red, int green, int blue);
   void plot_blend(int x, int y, double opacity, double red, double green, double blue);

   
   /* Invert
    * Inverts the image in RGB colourspace.
    * */
   void invert(void);

   /* Resize Image
    * Resizes the PNGwriter instance. Note: All image data is set to black (this is 
    * a resizing, not a scaling, of the image).
    * */
   void resize(int width, int height);

   /* Boundary Fill
    * All pixels adjacent to the start pixel will be filled with the fill colour, until the boundary colour is encountered. 
    * For example, calling boundary_fill() with the boundary colour set to red, on a pixel somewhere inside a red circle,
    * will fill the entire circle with the desired fill colour. If, on the other hand, the circle is not the boundary colour,
    * the rest of the image will be filled. 
    * The colour components are either doubles from 0.0 to 1.0 or ints from 0 to 65535.
    * */
   void boundary_fill(int xstart, int ystart, double boundary_red,double boundary_green,double boundary_blue,double fill_red, double fill_green, double fill_blue) ;
   void boundary_fill(int xstart, int ystart, int boundary_red,int boundary_green,int boundary_blue,int fill_red, int fill_green, int fill_blue) ;

   /* Flood Fill
    * All pixels adjacent to the start pixel will be filled with the fill colour, if they are the same colour as the
    * start pixel. For example, calling flood_fill() somewhere in the interior of a solid blue rectangle will colour
    * the entire rectangle the fill colour. The colour components are either doubles from 0.0 to 1.0 or ints from 0 to 65535.
    * */
   void flood_fill(int xstart, int ystart, double fill_red, double fill_green, double fill_blue) ;
   void flood_fill(int xstart, int ystart, int fill_red, int fill_green, int fill_blue) ;

   /* Polygon
    * This function takes an array of integer values containing the coordinates of the vertexes of a polygon. 
    * Note that if you want a closed polygon, you must repeat the first point's coordinates for the last point.
    * It also requires the number of points contained in the array. For example, if you wish to plot a triangle,
    * the array will contain 6 elements, and the number of points is 3. Be very careful about this; if you specify the wrong number
    * of points, your program will either segfault or produce points at nonsensical coordinates.
    * The colour components are either doubles from 0.0 to 1.0 or ints from 0 to 65535.
    * */
   void polygon(int * points, int number_of_points, double red, double green, double blue);
   void polygon(int * points, int number_of_points, int red, int green, int blue);
     
   /* Plot CMYK
    * Plot a point in the Cyan, Magenta, Yellow, Black colourspace. Please note that this colourspace is 
    * lossy, i.e. it cannot reproduce all colours on screen that RGB can. The difference, however, is 
    * barely noticeable. The algorithm used is a standard one. The colour components are either
    * doubles from 0.0 to 1.0 or ints from 0 to 65535.
    * */
   void plotCMYK(int x, int y, double cyan, double magenta, double yellow, double black);
   void plotCMYK(int x, int y, int cyan, int magenta, int yellow, int black);
   
   /* Read CMYK, Double version
    * Get a pixel in the Cyan, Magenta, Yellow, Black colourspace. if 'colour' is 1, the Cyan component will be returned
    * as a double from 0.0 to 1.0. If 'colour is 2, the Magenta colour component will be returned, and so on, up to 4.
    * */
   double dreadCMYK(int x, int y, int colour);

   /* Read CMYK
    * Same as the above, but the colour components returned are an int from 0 to 65535.
    * */
   int readCMYK(int x, int y, int colour);

   /* Scale Proportional
    * Scale the image using bilinear interpolation. If k is greater than 1.0, the image will be enlarged.
    * If k is less than 1.0, the image will be shrunk. Negative or null values of k are not allowed.
    * The image will be resized and the previous content will be replaced by the scaled image.
    * Tip: use getheight() and getwidth() to find out the new width and height of the scaled image.
    * Note: After scaling, all images will have a bit depth of 16, even if the original image had
    * a bit depth of 8.
    * */
   void scale_k(double k);
   
   /* Scale Non-Proportional
    * Scale the image using bilinear interpolation, with different horizontal and vertical scale factors.
    * */
   void scale_kxky(double kx, double ky);
   
   /* Scale To Target Width and Height
    * Scale the image in such a way as to meet the target width and height. 
    * Tip: if you want to keep the image proportional, scale_k() might be more appropriate.
    * */
   void scale_wh(int finalwidth, int finalheight);


   /* Blended Functions
    * All these functions are identical to their non-blended types. They take an extra argument, opacity, which is
    * a double from 0.0 to 1.0 and represents how much of the original pixel value is retained when plotting the 
    * new pixel. In other words, if opacity is 0.7, then after plotting, the new pixel will be 30% of the
    * original colour the pixel was, and 70% of the new colour, whatever that may be. As usual, each function
    * is available in int or double versions.  Please note: This is neither alpha channel nor PNG transparency chunk support. This merely blends the plotted pixels.
    * */
  
   // Start Blended Functions
  
   void plotHSV_blend(int x, int y, double opacity, double hue, double saturation, double value);
   void plotHSV_blend(int x, int y, double opacity, int hue, int saturation, int value);

   void line_blend(int xfrom, int yfrom, int xto, int yto,  double opacity, int red, int green,int  blue);
   void line_blend(int xfrom, int yfrom, int xto, int yto, double opacity, double red, double green,double  blue);
  
   void square_blend(int xfrom, int yfrom, int xto, int yto, double opacity, int red, int green,int  blue);
   void square_blend(int xfrom, int yfrom, int xto, int yto, double opacity, double red, double green,double  blue);
   
   void filledsquare_blend(int xfrom, int yfrom, int xto, int yto, double opacity, int red, int green,int  blue);
   void filledsquare_blend(int xfrom, int yfrom, int xto, int yto, double opacity, double red, double green,double  blue);

   void circle_blend(int xcentre, int ycentre, int radius, double opacity, int red, int green, int blue);
   void circle_blend(int xcentre, int ycentre, int radius, double opacity, double red, double green, double blue);
   
   void filledcircle_blend(int xcentre, int ycentre, int radius, double opacity, int red, int green, int blue);
   void filledcircle_blend(int xcentre, int ycentre, int radius, double opacity, double red, double green, double blue);

   void bezier_blend(  int startPtX, int startPtY,                                                                            
		       int startControlX, int startControlY,                                                                                     
		       int endPtX, int endPtY,                                                                                                   
		       int endControlX, int endControlY,
		       double opacity,
		       double red, double green, double blue);
   
   void bezier_blend(  int startPtX, int startPtY,                                                                            
		       int startControlX, int startControlY,                                                                                     
		       int endPtX, int endPtY,                                                                                                   
		       int endControlX, int endControlY,
		       double opacity,
		       int red, int green, int blue);
   
   void plot_text_blend(char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double opacity, double red, double green, double blue);
   void plot_text_blend(char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double opacity, int red, int green, int blue);

   void plot_text_utf8_blend(char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double opacity, double red, double green, double blue);
   void plot_text_utf8_blend(char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double opacity, int red, int green, int blue);

   void boundary_fill_blend(int xstart, int ystart, double opacity, double boundary_red,double boundary_green,double boundary_blue,double fill_red, double fill_green, double fill_blue) ;
   void boundary_fill_blend(int xstart, int ystart, double opacity, int boundary_red,int boundary_green,int boundary_blue,int fill_red, int fill_green, int fill_blue) ;

   void flood_fill_blend(int xstart, int ystart, double opacity, double fill_red, double fill_green, double fill_blue) ;
   void flood_fill_blend(int xstart, int ystart, double opacity, int fill_red, int fill_green, int fill_blue) ;

   void polygon_blend(int * points, int number_of_points, double opacity, double red, double green, double blue);
   void polygon_blend(int * points, int number_of_points, double opacity,  int red, int green, int blue);
     
   void plotCMYK_blend(int x, int y, double opacity, double cyan, double magenta, double yellow, double black);
   void plotCMYK_blend(int x, int y, double opacity, int cyan, int magenta, int yellow, int black);

   // End of Blended Functions
  
   /* Laplacian
    * This function applies a discrete laplacian to the image, multiplied by a constant factor. 
    * The kernel used in this case is:
    *                1.0  1.0  1.0
    *                1.0 -8.0  1.0
    *                1.0  1.0  1.0
    * Basically, this works as an edge detector. The current pixel is assigned the sum of all neighbouring
    * pixels, multiplied by the corresponding kernel element. For example, imagine a pixel and its 8 neighbours:
    *                1.0    1.0    0.0  0.0
    *                1.0  ->1.0<-  0.0  0.0
    *                1.0    1.0    0.0  0.0
    * This represents a border between white and black, black is on the right. Applying the laplacian to 
    * the pixel specified above pixel gives:
    *                1.0*1.0  + 1.0*1.0  + 0.0*1.0 +
    *                1.0*1.0  + 1.0*-8.0 + 0.0*1.0 +
    *                1.0*1.0  + 1.0*1.0  + 0.0*1.0   = -3.0
    * Applying this to the pixel to the right of the pixel considered previously, we get a sum of 3.0.
    * That is, after passing over an edge, we get a high value for the pixel adjacent to the edge. Since
    * PNGwriter limits the colour components if they are off-scale, and the result of the laplacian
    * may be negative, a scale factor and an offset value are included. This might be useful for
    * keeping things within range or for bringing out more detail in the edge detection. The 
    * final pixel value will be given by:
    *   final value =  laplacian(original pixel)*k + offset
    * Tip: Try a value of 1.0 for k to start with, and then experiment with other values.
    * */
    void laplacian(double k, double offset);

   /* Filled Triangle
    * Draws the triangle specified by the three pairs of points in the colour specified
    * by the colour coefficients. The colour components are either doubles from 0.0 to 
    * 1.0 or ints from 0 to 65535.
    * */
   void filledtriangle(int x1,int y1,int x2,int y2,int x3,int y3, int red, int green, int blue);
   void filledtriangle(int x1,int y1,int x2,int y2,int x3,int y3, double red, double green, double blue);

   /* Filled Triangle, Blended
    * Draws the triangle specified by the three pairs of points in the colour specified
    * by the colour coefficients, and blended with the background. See the description for Blended Functions.
    * The colour components are either doubles from 0.0 to 1.0 or ints from 0 to 65535.
    * */
   void filledtriangle_blend(int x1,int y1,int x2,int y2,int x3,int y3, double opacity, int red, int green, int blue);
   void filledtriangle_blend(int x1,int y1,int x2,int y2,int x3,int y3, double opacity, double red, double green, double blue);

   /* Arrow,  Filled Arrow
    * Plots an arrow from (x1, y1) to (x2, y2) with the arrowhead at the second point, given the size in pixels
    * and the angle in radians of the arrowhead. The plotted arrow consists of one main line, and two smaller 
    * lines originating from the second point. Filled Arrow plots the same, but the arrowhead is a solid triangle.
    * Tip: An angle of 10 to 30 degrees looks OK.
    * */
   
   void arrow( int x1,int y1,int x2,int y2,int size, double head_angle, double red, double green, double blue);
   void arrow( int x1,int y1,int x2,int y2,int size, double head_angle, int red, int green, int blue);
   
   void filledarrow( int x1,int y1,int x2,int y2,int size, double head_angle, double red, double green, double blue);
   void filledarrow( int x1,int y1,int x2,int y2,int size, double head_angle, int red, int green, int blue);

   /* Cross, Maltese Cross
    * Plots a simple cross at x, y, with the specified height and width, and in the specified colour.
    * Maltese cross plots a cross, as before, but adds bars at the end of each arm of the cross.
    * The size of these bars is specified with x_bar_height and y_bar_width.
    * The cross will look something like this:
    * 
    *    -----  <-- ( y_bar_width)
    *      |
    *      |
    *  |-------|  <-- ( x_bar_height )
    *      |
    *      |
    *    -----
    * */
   
   void cross( int x, int y, int xwidth, int yheight, double red, double green, double blue);
   void cross( int x, int y, int xwidth, int yheight, int red, int green, int blue);

   void maltesecross( int x, int y, int xwidth, int yheight, int x_bar_height, int y_bar_width, double red, double green, double blue);
   void maltesecross( int x, int y, int xwidth, int yheight, int x_bar_height, int y_bar_width, int red, int green, int blue);

   /* Diamond and filled diamond
    * Plots a diamond shape, given the x, y position, the width and height, and the colour.
    * Filled diamond plots a filled diamond.
    * */
   
   void filleddiamond( int x, int y, int width, int height, int red, int green, int blue);
   void diamond(int x, int y, int width, int height, int red, int green, int blue);
   
   void filleddiamond( int x, int y, int width, int height, double red, double green, double blue);
   void diamond(int x, int y, int width, int height, double red, double green, double blue);

   /* Get Text Width, Get Text Width UTF8
    * Returns the approximate width, in pixels, of the specified *unrotated* text. It is calculated by adding 
    * each letter's width and kerning value (as specified in the TTF file). Note that this will not 
    * give the position of the farthest pixel, but it will give a pretty good idea of what area the 
    * text will occupy. Tip: The text, when plotted unrotated, will fit approximately in a box with its lower left corner at 
    * (x_start, y_start) and upper right at (x_start + width, y_start + size), where width is given by get_text_width()
    * and size is the specified size of the text to be plotted. Tip: Text plotted at position
    * (x_start, y_start), rotated with a given 'angle', and of a given 'size'
    * whose width is 'width', will fit approximately inside a rectangle whose corners are at
    *     1  (x_start, y_start)
    *     2  (x_start + width*cos(angle), y_start + width*sin(angle))
    *     3  (x_start + width*cos(angle) - size*sin(angle), y_start + width*sin(angle) + size*cos(angle))
    *     4  (x_start - size*sin(angle), y_start + size*cos(angle))  
    * */
   
   int get_text_width(char * face_path, int fontsize,  char * text);

   int get_text_width_utf8(char * face_path, int fontsize, char * text);
   
   
};


#endif

