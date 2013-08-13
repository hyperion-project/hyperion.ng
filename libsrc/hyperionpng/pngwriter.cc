//**********  pngwriter.cc   **********************************************
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
//  Documentation:             The header file (pngwriter.h) is commented, but for a
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

#define NO_FREETYPE
#include "pngwriter.h"

// Default Constructor
////////////////////////////////////////////////////////////////////////////
pngwriter::pngwriter()
{

   filename_ = new char[255];
   textauthor_ = new char[255];
   textdescription_ = new char[255];
   texttitle_  = new char[255];
   textsoftware_ = new char[255];

   strcpy(filename_, "out.png");
   width_ = 250;
   height_ = 250;
   backgroundcolour_ = 65535;
   compressionlevel_ = -2;
   filegamma_ = 0.5;
   transformation_ = 0;

   strcpy(textauthor_, "PNGwriter Author: Paul Blackburn");
   strcpy(textdescription_, "http://pngwriter.sourceforge.net/");
   strcpy(textsoftware_, "PNGwriter: An easy to use graphics library.");
   strcpy(texttitle_, "out.png");

   int kkkk;

   bit_depth_ = 16; //Default bit depth for new images
   colortype_=2;
   screengamma_ = 2.2;

   graph_ = (png_bytepp)malloc(height_ * sizeof(png_bytep));
   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   for (kkkk = 0; kkkk < height_; kkkk++)
	 {
		graph_[kkkk] = (png_bytep)malloc(6*width_ * sizeof(png_byte));
	if(graph_[kkkk] == NULL)
	  {
		 std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	  }
	 }

   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   int tempindex;
   for(int hhh = 0; hhh<width_;hhh++)
	 {
	for(int vhhh = 0; vhhh<height_;vhhh++)
	  {
		 //graph_[vhhh][6*hhh + i] where i goes from 0 to 5
		 tempindex = 6*hhh;
		 graph_[vhhh][tempindex] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+1] = (char)(backgroundcolour_%256);
		 graph_[vhhh][tempindex+2] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+3] = (char)(backgroundcolour_%256);
		 graph_[vhhh][tempindex+4] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+5] = (char)(backgroundcolour_%256);
	  }
	 }

};

//Copy Constructor
//////////////////////////////////////////////////////////////////////////
pngwriter::pngwriter(const pngwriter &rhs)
{
   width_ = rhs.width_;
   height_ = rhs.height_;
   backgroundcolour_ = rhs.backgroundcolour_;
   compressionlevel_ = rhs.compressionlevel_;
   filegamma_ = rhs.filegamma_;
   transformation_ = rhs.transformation_;;

   filename_ = new char[strlen(rhs.filename_)+1];
   textauthor_ = new char[strlen(rhs.textauthor_)+1];
   textdescription_ = new char[strlen(rhs.textdescription_)+1];
   textsoftware_ = new char[strlen(rhs.textsoftware_)+1];
   texttitle_ = new char[strlen(rhs.texttitle_)+1];

   strcpy(filename_, rhs.filename_);
   strcpy(textauthor_, rhs.textauthor_);
   strcpy(textdescription_, rhs.textdescription_);
   strcpy(textsoftware_,rhs.textsoftware_);
   strcpy(texttitle_, rhs.texttitle_);

   int kkkk;

   bit_depth_ = rhs.bit_depth_;
   colortype_= rhs.colortype_;
   screengamma_ = rhs.screengamma_;

   graph_ = (png_bytepp)malloc(height_ * sizeof(png_bytep));
   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   for (kkkk = 0; kkkk < height_; kkkk++)
	 {
		graph_[kkkk] = (png_bytep)malloc(6*width_ * sizeof(png_byte));
	if(graph_[kkkk] == NULL)
	  {
		 std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	  }
	 }

   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }
   int tempindex;
   for(int hhh = 0; hhh<width_;hhh++)
	 {
	for(int vhhh = 0; vhhh<height_;vhhh++)
	  {
		 //   graph_[vhhh][6*hhh + i ] i=0 to 5
		 tempindex=6*hhh;
		 graph_[vhhh][tempindex] = rhs.graph_[vhhh][tempindex];
		 graph_[vhhh][tempindex+1] = rhs.graph_[vhhh][tempindex+1];
		 graph_[vhhh][tempindex+2] = rhs.graph_[vhhh][tempindex+2];
		 graph_[vhhh][tempindex+3] = rhs.graph_[vhhh][tempindex+3];
		 graph_[vhhh][tempindex+4] = rhs.graph_[vhhh][tempindex+4];
		 graph_[vhhh][tempindex+5] = rhs.graph_[vhhh][tempindex+5];
	  }
	 }

};

//Constructor for int colour levels, char * filename
//////////////////////////////////////////////////////////////////////////
pngwriter::pngwriter(int x, int y, int backgroundcolour, char * filename)
{
   width_ = x;
   height_ = y;
   backgroundcolour_ = backgroundcolour;
   compressionlevel_ = -2;
   filegamma_ = 0.6;
   transformation_ = 0;

   textauthor_ = new char[255];
   textdescription_ = new char[255];
   texttitle_ = new char[strlen(filename)+1];
   textsoftware_ = new char[255];
   filename_ = new char[strlen(filename)+1];

   strcpy(textauthor_, "PNGwriter Author: Paul Blackburn");
   strcpy(textdescription_, "http://pngwriter.sourceforge.net/");
   strcpy(textsoftware_, "PNGwriter: An easy to use graphics library.");
   strcpy(texttitle_, filename);
   strcpy(filename_, filename);

   if((width_<0)||(height_<0))
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **: Constructor called with negative height or width. Setting width and height to 1." << std::endl;
	width_ = 1;
	height_ = 1;
	 }

   if(backgroundcolour_ >65535)
	 {
	std::cerr << " PNGwriter::pngwriter - WARNING **: Constructor called with background colour greater than 65535. Setting to 65535."<<std::endl;
	backgroundcolour_ = 65535;
	 }

   if(backgroundcolour_ <0)
	 {
	std::cerr << " PNGwriter::pngwriter - WARNING **: Constructor called with background colour lower than 0. Setting to 0."<<std::endl;
	backgroundcolour_ = 0;
	 }

   int kkkk;

   bit_depth_ = 16; //Default bit depth for new images
   colortype_=2;
   screengamma_ = 2.2;

   graph_ = (png_bytepp)malloc(height_ * sizeof(png_bytep));
   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   for (kkkk = 0; kkkk < height_; kkkk++)
	 {
		graph_[kkkk] = (png_bytep)malloc(6*width_ * sizeof(png_byte));
	if(graph_[kkkk] == NULL)
	  {
		 std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	  }
	 }

   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   int tempindex;
   for(int hhh = 0; hhh<width_;hhh++)
	 {
	for(int vhhh = 0; vhhh<height_;vhhh++)
	  {
		 //graph_[vhhh][6*hhh + i] i = 0  to 5
		 tempindex = 6*hhh;
		 graph_[vhhh][tempindex] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+1] = (char)(backgroundcolour_%256);
		 graph_[vhhh][tempindex+2] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+3] = (char)(backgroundcolour_%256);
		 graph_[vhhh][tempindex+4] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+5] = (char)(backgroundcolour_%256);
	  }
	 }

};

//Constructor for double levels, char * filename
/////////////////////////////////////////////////////////////////////////
pngwriter::pngwriter(int x, int y, double backgroundcolour, char * filename)
{
   width_ = x;
   height_ = y;
   compressionlevel_ = -2;
   filegamma_ = 0.6;
   transformation_ = 0;
   backgroundcolour_ = int(backgroundcolour*65535);

   textauthor_ = new char[255];
   textdescription_ = new char[255];
   texttitle_ = new char[strlen(filename)+1];
   textsoftware_ = new char[255];
   filename_ = new char[strlen(filename)+1];

   strcpy(textauthor_, "PNGwriter Author: Paul Blackburn");
   strcpy(textdescription_, "http://pngwriter.sourceforge.net/");
   strcpy(textsoftware_, "PNGwriter: An easy to use graphics library.");
   strcpy(texttitle_, filename);
   strcpy(filename_, filename);

   if((width_<0)||(height_<0))
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **: Constructor called with negative height or width. Setting width and height to 1." << std::endl;
	width_ = 1;
	height_ = 1;
	 }

   if(backgroundcolour_ >65535)
	 {
	std::cerr << " PNGwriter::pngwriter - WARNING **: Constructor called with background colour greater than 1.0. Setting to 1.0."<<std::endl;
	backgroundcolour_ = 65535;
	 }

   if(backgroundcolour_ < 0)
	 {
	std::cerr << " PNGwriter::pngwriter - WARNING **: Constructor called with background colour lower than 0.0. Setting to 0.0."<<std::endl;
	backgroundcolour_ = 0;
	 }

   int kkkk;

   bit_depth_ = 16; //Default bit depth for new images
   colortype_=2;
   screengamma_ = 2.2;

   graph_ = (png_bytepp)malloc(height_ * sizeof(png_bytep));
   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   for (kkkk = 0; kkkk < height_; kkkk++)
	 {
		graph_[kkkk] = (png_bytep)malloc(6*width_ * sizeof(png_byte));
	if(graph_[kkkk] == NULL)
	  {
		 std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	  }
	 }

   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   int tempindex;
   for(int hhh = 0; hhh<width_;hhh++)
	 {
	for(int vhhh = 0; vhhh<height_;vhhh++)
	  {
		 // graph_[vhhh][tempindex + i] where i = 0 to 5
		 tempindex = 6*hhh;
		 graph_[vhhh][tempindex] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+1] = (char)(backgroundcolour_%256);
		 graph_[vhhh][tempindex+2] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+3] = (char)(backgroundcolour_%256);
		 graph_[vhhh][tempindex+4] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+5] = (char)(backgroundcolour_%256);
	  }
	 }

};

//Destructor
///////////////////////////////////////
pngwriter::~pngwriter()
{
   delete [] filename_;
   delete [] textauthor_;
   delete [] textdescription_;
   delete [] texttitle_;
   delete [] textsoftware_;

   for (int jjj = 0; jjj < height_; jjj++) free(graph_[jjj]);
   free(graph_);
};

//Constructor for int levels, const char * filename
//////////////////////////////////////////////////////////////
pngwriter::pngwriter(int x, int y, int backgroundcolour, const char * filename)
{
   width_ = x;
   height_ = y;
   backgroundcolour_ = backgroundcolour;
   compressionlevel_ = -2;
   filegamma_ = 0.6;
   transformation_ = 0;

   textauthor_ = new char[255];
   textdescription_ = new char[255];
   texttitle_ = new char[strlen(filename)+1];
   textsoftware_ = new char[255];
   filename_ = new char[strlen(filename)+1];

   strcpy(textauthor_, "PNGwriter Author: Paul Blackburn");
   strcpy(textdescription_, "http://pngwriter.sourceforge.net/");
   strcpy(textsoftware_, "PNGwriter: An easy to use graphics library.");
   strcpy(texttitle_, filename);
   strcpy(filename_, filename);

   if((width_<0)||(height_<0))
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **: Constructor called with negative height or width. Setting width and height to 1." << std::endl;
	height_ = 1;
	width_ = 1;
	 }

   if(backgroundcolour_ >65535)
	 {
	std::cerr << " PNGwriter::pngwriter - WARNING **: Constructor called with background colour greater than 65535. Setting to 65535."<<std::endl;
	backgroundcolour_ = 65535;
	 }

   if(backgroundcolour_ <0)
	 {
	std::cerr << " PNGwriter::pngwriter - WARNING **: Constructor called with background colour lower than 0. Setting to 0."<<std::endl;
	backgroundcolour_ = 0;
	 }

   int kkkk;

   bit_depth_ = 16; //Default bit depth for new images
   colortype_=2;
   screengamma_ = 2.2;

   graph_ = (png_bytepp)malloc(height_ * sizeof(png_bytep));
   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   for (kkkk = 0; kkkk < height_; kkkk++)
	 {
		graph_[kkkk] = (png_bytep)malloc(6*width_ * sizeof(png_byte));
	if(graph_[kkkk] == NULL)
	  {
		 std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	  }
	 }

   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   int tempindex;
   for(int hhh = 0; hhh<width_;hhh++)
	 {
	for(int vhhh = 0; vhhh<height_;vhhh++)
	  {
		 //graph_[vhhh][6*hhh + i] where i = 0 to 5
		 tempindex=6*hhh;
		 graph_[vhhh][tempindex] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+1] = (char)(backgroundcolour_%256);
		 graph_[vhhh][tempindex+2] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+3] = (char)(backgroundcolour_%256);
		 graph_[vhhh][tempindex+4] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+5] = (char)(backgroundcolour_%256);
	  }
	 }

};

//Constructor for double levels, const char * filename
/////////////////////////////////////////////////////////////////////////
pngwriter::pngwriter(int x, int y, double backgroundcolour, const char * filename)
{
   width_ = x;
   height_ = y;
   compressionlevel_ = -2;
   backgroundcolour_ = int(backgroundcolour*65535);
   filegamma_ = 0.6;
   transformation_ = 0;

   textauthor_ = new char[255];
   textdescription_ = new char[255];
   texttitle_ = new char[strlen(filename)+1];
   textsoftware_ = new char[255];
   filename_ = new char[strlen(filename)+1];

   strcpy(textauthor_, "PNGwriter Author: Paul Blackburn");
   strcpy(textdescription_, "http://pngwriter.sourceforge.net/");
   strcpy(textsoftware_, "PNGwriter: An easy to use graphics library.");
   strcpy(texttitle_, filename);
   strcpy(filename_, filename);

   if((width_<0)||(height_<0))
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **: Constructor called with negative height or width. Setting width and height to 1." << std::endl;
	width_ = 1;
	height_ = 1;
	 }

   if(backgroundcolour_ >65535)
	 {
	std::cerr << " PNGwriter::pngwriter - WARNING **: Constructor called with background colour greater than 65535. Setting to 65535."<<std::endl;
	backgroundcolour_ = 65535;
	 }

   if(backgroundcolour_ <0)
	 {
	std::cerr << " PNGwriter::pngwriter - WARNING **: Constructor called with background colour lower than 0. Setting to 0."<<std::endl;
	backgroundcolour_ = 0;
	 }

   int kkkk;

   bit_depth_ = 16; //Default bit depth for new images
   colortype_=2;
   screengamma_ = 2.2;

   graph_ = (png_bytepp)malloc(height_ * sizeof(png_bytep));
   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   for (kkkk = 0; kkkk < height_; kkkk++)
	 {
		graph_[kkkk] = (png_bytep)malloc(6*width_ * sizeof(png_byte));
	if(graph_[kkkk] == NULL)
	  {
		 std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	  }
	 }

   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   int tempindex;
   for(int hhh = 0; hhh<width_;hhh++)
	 {
	for(int vhhh = 0; vhhh<height_;vhhh++)
	  {
		 //etc
		 tempindex = 6*hhh;
		 graph_[vhhh][tempindex] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+1] = (char)(backgroundcolour_%256);
		 graph_[vhhh][tempindex+2] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+3] = (char)(backgroundcolour_%256);
		 graph_[vhhh][tempindex+4] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+5] = (char)(backgroundcolour_%256);
	  }
	 }

};

// Overloading operator =
/////////////////////////////////////////////////////////
pngwriter & pngwriter::operator = (const pngwriter & rhs)
{
   if( this==&rhs)
	 {
	return *this;
	 }

   width_ = rhs.width_;
   height_ = rhs.height_;
   backgroundcolour_ = rhs.backgroundcolour_;
   compressionlevel_ = rhs.compressionlevel_;
   filegamma_ = rhs.filegamma_;
   transformation_ = rhs.transformation_;

   filename_ = new char[strlen(rhs.filename_)+1];
   textauthor_ = new char[strlen(rhs.textauthor_)+1];
   textdescription_ = new char[strlen(rhs.textdescription_)+1];
   textsoftware_ = new char[strlen(rhs.textsoftware_)+1];
   texttitle_ = new char[strlen(rhs.texttitle_)+1];

   strcpy(textauthor_, rhs.textauthor_);
   strcpy(textdescription_, rhs.textdescription_);
   strcpy(textsoftware_,rhs.textsoftware_);
   strcpy(texttitle_, rhs.texttitle_);
   strcpy(filename_, rhs.filename_);

   int kkkk;

   bit_depth_ = rhs.bit_depth_;
   colortype_= rhs.colortype_;
   screengamma_ = rhs.screengamma_;

   graph_ = (png_bytepp)malloc(height_ * sizeof(png_bytep));
   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   for (kkkk = 0; kkkk < height_; kkkk++)
	 {
		graph_[kkkk] = (png_bytep)malloc(6*width_ * sizeof(png_byte));
	if(graph_[kkkk] == NULL)
	  {
		 std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	  }
	 }

   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::pngwriter - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   int tempindex;
   for(int hhh = 0; hhh<width_;hhh++)
	 {
	for(int vhhh = 0; vhhh<height_;vhhh++)
	  {
		 tempindex=6*hhh;
		 graph_[vhhh][tempindex] = rhs.graph_[vhhh][tempindex];
		 graph_[vhhh][tempindex+1] = rhs.graph_[vhhh][tempindex+1];
		 graph_[vhhh][tempindex+2] = rhs.graph_[vhhh][tempindex+2];
		 graph_[vhhh][tempindex+3] = rhs.graph_[vhhh][tempindex+3];
		 graph_[vhhh][tempindex+4] = rhs.graph_[vhhh][tempindex+4];
		 graph_[vhhh][tempindex+5] = rhs.graph_[vhhh][tempindex+5];
	  }
	 }

   return *this;
}

///////////////////////////////////////////////////////////////
void pngwriter::plot(int x, int y, int red, int green, int blue)
{
   int tempindex;

   if(red > 65535)
	 {
	red = 65535;
	 }
   if(green > 65535)
	 {
	green = 65535;
	 }
   if(blue > 65535)
	 {
	blue = 65535;
	 }

   if(red < 0)
	 {
	red = 0;
	 }
   if(green < 0)
	 {
	green = 0;
	 }
   if(blue < 0)
	 {
	blue = 0;
	 }

   if((bit_depth_ == 16))
	 {
	//	if( (height_-y >-1) && (height_-y <height_) && (6*(x-1) >-1) && (6*(x-1)+5<6*width_) )
	if( (y<=height_) && (y>0) && (x>0) && (x<=width_) )
	  {
		 //graph_[height_-y][6*(x-1) + i] where i goes from 0 to 5
		 tempindex= 6*x-6;
		 graph_[height_-y][tempindex] = (char) floor(((double)red)/256);
		 graph_[height_-y][tempindex+1] = (char)(red%256);
		 graph_[height_-y][tempindex+2] = (char) floor(((double)green)/256);
		 graph_[height_-y][tempindex+3] = (char)(green%256);
		 graph_[height_-y][tempindex+4] = (char) floor(((double)blue)/256);
		 graph_[height_-y][tempindex+5] = (char)(blue%256);
	  };

	/*
	 if(!( (height_-y >-1) && (height_-y <height_) && (6*(x-1) >-1) && (6*(x-1)+5<6*width_) ))
	 {
	 std::cerr << " PNGwriter::plot-- Plotting out of range! " << y << "   " << x << std::endl;
	 }
	 */
	 }

   if((bit_depth_ == 8))
	 {
	//	 if( (height_-y >-1) && (height_-y <height_) && (3*(x-1) >-1) && (3*(x-1)+5<3*width_) )
	if( (y<height_+1) && (y>0) && (x>0) && (x<width_+1) )
	  {
		 //	     graph_[height_-y][3*(x-1) + i] where i goes from 0 to 2
		 tempindex = 3*x-3;
		 graph_[height_-y][tempindex] = (char)(floor(((double)red)/257.0));
		 graph_[height_-y][tempindex+1] = (char)(floor(((double)green)/257.0));
		 graph_[height_-y][tempindex+2] = (char)(floor(((double)blue)/257.0));

	  };

	/*
	 if(!( (height_-y >-1) && (height_-y <height_) && (6*(x-1) >-1) && (6*(x-1)+5<6*width_) ))
	 {
	 std::cerr << " PNGwriter::plot-- Plotting out of range! " << y << "   " << x << std::endl;
	 }
	 */
	 }
};

void pngwriter::plot(int x, int y, double red, double green, double blue)
{
   this->plot(x,y,int(red*65535),int(green*65535),int(blue*65535));
};

///////////////////////////////////////////////////////////////
int pngwriter::read(int x, int y, int colour)
{
   int temp1,temp2;

   if((colour !=1)&&(colour !=2)&&(colour !=3))
	 {
	std::cerr << " PNGwriter::read - WARNING **: Invalid argument: should be 1, 2 or 3, is " << colour << std::endl;
	return 0;
	 }

   if( ( x>0 ) && ( x <= (this->width_) ) && ( y>0 ) && ( y <= (this->height_) ) )
	 {

	if(bit_depth_ == 16)
	  {
		 temp2=6*(x-1);
		 if(colour == 1)
		   {
		  temp1 = (graph_[(height_-y)][temp2])*256 + graph_[height_-y][temp2+1];
		  return temp1;
		   }

		 if(colour == 2)
		   {
		  temp1 = (graph_[height_-y][temp2+2])*256 + graph_[height_-y][temp2+3];
		  return temp1;
		   }

		 if(colour == 3)
		   {
		  temp1 = (graph_[height_-y][temp2+4])*256 + graph_[height_-y][temp2+5];
		  return temp1;
		   }
	  }

	if(bit_depth_ == 8)
	  {
		 temp2=3*(x-1);
		 if(colour == 1)
		   {
		  temp1 = graph_[height_-y][temp2];
		  return temp1*256;
		   }

		 if(colour == 2)
		   {
		  temp1 =  graph_[height_-y][temp2+1];
		  return temp1*256;
		   }

		 if(colour == 3)
		   {
		  temp1 =  graph_[height_-y][temp2+2];
		  return temp1*256;
		   }
	  }
	 }
   else
	 {
	return 0;
	 }

   std::cerr << " PNGwriter::read - WARNING **: Returning 0 because of bitdepth/colour type mismatch."<< std::endl;
   return 0;
}

///////////////////////////////////////////////////////////////
int pngwriter::read(int xxx, int yyy)
{
   int temp1,temp2,temp3,temp4,temp5;

   if(
	  ( xxx>0 ) &&
	  ( xxx <= (this->width_) ) &&
	  ( yyy>0 ) &&
	  ( yyy <= (this->height_) )
	  )
	 {
	if(bit_depth_ == 16)
	  {
		 //	temp1 = (graph_[(height_-yyy)][6*(xxx-1)])*256 + graph_[height_-yyy][6*(xxx-1)+1];
		 temp5=6*xxx;
		 temp1 = (graph_[(height_-yyy)][temp5-6])*256 + graph_[height_-yyy][temp5-5];
		 temp2 = (graph_[height_-yyy][temp5-4])*256 + graph_[height_-yyy][temp5-3];
		 temp3 = (graph_[height_-yyy][temp5-2])*256 + graph_[height_-yyy][temp5-1];
		 temp4 =  int((temp1+temp2+temp3)/3.0);
	  }
	else if(bit_depth_ == 8)
	  {
		 //	temp1 = graph_[height_-yyy][3*(xxx-1)];
		 temp5 = 3*xxx;
		 temp1 = graph_[height_-yyy][temp5-3];
		 temp2 =  graph_[height_-yyy][temp5-2];
		 temp3 =  graph_[height_-yyy][temp5-1];
		 temp4 =  int((temp1+temp2+temp3)/3.0);
	  }
	else
	  {
		 std::cerr << " PNGwriter::read - WARNING **: Invalid bit depth! Returning 0 as average value." << std::endl;
		 temp4 = 0;
	  }

	return temp4;

	 }
   else
	 {
	return 0;
	 }
}

/////////////////////////////////////////////////////
double  pngwriter::dread(int x, int y, int colour)
{
   return double(this->read(x,y,colour))/65535.0;
}

double  pngwriter::dread(int x, int y)
{
   return double(this->read(x,y))/65535.0;
}

///////////////////////////////////////////////////////
void pngwriter::clear()
{
   int pen = 0;
   int pencil = 0;
   int tempindex;

   if(bit_depth_==16)
	 {
	for(pen = 0; pen<width_;pen++)
	  {
		 for(pencil = 0; pencil<height_;pencil++)
		   {
		  tempindex=6*pen;
		  graph_[pencil][tempindex] = 0;
		  graph_[pencil][tempindex+1] = 0;
		  graph_[pencil][tempindex+2] = 0;
		  graph_[pencil][tempindex+3] = 0;
		  graph_[pencil][tempindex+4] = 0;
		  graph_[pencil][tempindex+5] = 0;
		   }
	  }
	 }

   if(bit_depth_==8)
	 {
	for(pen = 0; pen<width_;pen++)
	  {
		 for(pencil = 0; pencil<height_;pencil++)
		   {
		  tempindex=3*pen;
		  graph_[pencil][tempindex] = 0;
		  graph_[pencil][tempindex+1] = 0;
		  graph_[pencil][tempindex+2] = 0;
		   }
	  }
	 }

};

/////////////////////////////////////////////////////
void pngwriter::pngwriter_rename(char * newname)
{
   delete [] filename_;
   delete [] texttitle_;

   filename_ = new char[strlen(newname)+1];
   texttitle_ = new char[strlen(newname)+1];

   strcpy(filename_,newname);
   strcpy(texttitle_,newname);
};

///////////////////////////////////////////////////////
void pngwriter::pngwriter_rename(const char * newname)
{
   delete [] filename_;
   delete [] texttitle_;

   filename_ = new char[strlen(newname)+1];
   texttitle_ = new char[strlen(newname)+1];

   strcpy(filename_,newname);
   strcpy(texttitle_,newname);
};

///////////////////////////////////////////////////////
void pngwriter::pngwriter_rename(long unsigned int index)
{
   char buffer[255];

   //   %[flags][width][.precision][modifiers]type
   //
   if((index > 999999999)||(index < 0))
	 {
	std::cerr << " PNGwriter::pngwriter_rename - ERROR **: Numerical name is out of 0 - 999 999 999 range (" << index <<")." << std::endl;
	return;
	 }

   if( 0>  sprintf(buffer, "%9.9lu.png",index))
	 {
	std::cerr << " PNGwriter::pngwriter_rename - ERROR **: Error creating numerical filename." << std::endl;
	return;
	 }

   delete [] filename_;
   delete [] texttitle_;

   filename_ = new char[strlen(buffer)+1];
   texttitle_ = new char[strlen(buffer)+1];

   strcpy(filename_,buffer);
   strcpy(texttitle_,buffer);

};

///////////////////////////////////////////////////////
void pngwriter::settext(char * title, char * author, char * description, char * software)
{
   delete [] textauthor_;
   delete [] textdescription_;
   delete [] texttitle_;
   delete [] textsoftware_;

   textauthor_ = new char[strlen(author)+1];
   textdescription_ = new char[strlen(description)+1];
   textsoftware_ = new char[strlen(software)+1];
   texttitle_ = new char[strlen(title)+1];

   strcpy(texttitle_, title);
   strcpy(textauthor_, author);
   strcpy(textdescription_, description);
   strcpy(textsoftware_, software);
};

///////////////////////////////////////////////////////
void pngwriter::settext(const char * title, const char * author, const char * description, const char * software)
{
   delete [] textauthor_;
   delete [] textdescription_;
   delete [] texttitle_;
   delete [] textsoftware_;

   textauthor_ = new char[strlen(author)+1];
   textdescription_ = new char[strlen(description)+1];
   textsoftware_ = new char[strlen(software)+1];
   texttitle_ = new char[strlen(title)+1];

   strcpy(texttitle_, title);
   strcpy(textauthor_, author);
   strcpy(textdescription_, description);
   strcpy(textsoftware_, software);
};

///////////////////////////////////////////////////////
void pngwriter::close()
{
   FILE            *fp;
   png_structp     png_ptr;
   png_infop       info_ptr;

   fp = fopen(filename_, "wb");
   if( fp == NULL)
	 {
	std::cerr << " PNGwriter::close - ERROR **: Error creating file (fopen() returned NULL pointer)." << std::endl;
	perror(" PNGwriter::close - ERROR **");
	return;
	 }

   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   info_ptr = png_create_info_struct(png_ptr);
   png_init_io(png_ptr, fp);
   if(compressionlevel_ != -2)
	 {
	png_set_compression_level(png_ptr, compressionlevel_);
	 }
   else
	 {
	png_set_compression_level(png_ptr, PNGWRITER_DEFAULT_COMPRESSION);
	 }

   png_set_IHDR(png_ptr, info_ptr, width_, height_,
		bit_depth_, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

   if(filegamma_ < 1.0e-1)
	 {
	filegamma_ = 0.5;  // Modified in 0.5.4 so as to be the same as the usual gamma.
	 }

   png_set_gAMA(png_ptr, info_ptr, filegamma_);

   time_t          gmt;
   png_time        mod_time;
   png_text        text_ptr[5];
   time(&gmt);
   png_convert_from_time_t(&mod_time, gmt);
   png_set_tIME(png_ptr, info_ptr, &mod_time);
   text_ptr[0].key = (char *)"Title";
   text_ptr[0].text = texttitle_;
   text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
   text_ptr[1].key = (char *)"Author";
   text_ptr[1].text = textauthor_;
   text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
   text_ptr[2].key = (char *)"Description";
   text_ptr[2].text = textdescription_;
   text_ptr[2].compression = PNG_TEXT_COMPRESSION_NONE;
   text_ptr[3].key = (char *)"Creation Time";
   text_ptr[3].text = png_convert_to_rfc1123(png_ptr, &mod_time);
   text_ptr[3].compression = PNG_TEXT_COMPRESSION_NONE;
   text_ptr[4].key = (char *)"Software";
   text_ptr[4].text = textsoftware_;
   text_ptr[4].compression = PNG_TEXT_COMPRESSION_NONE;
   png_set_text(png_ptr, info_ptr, text_ptr, 5);

   png_write_info(png_ptr, info_ptr);
   png_write_image(png_ptr, graph_);
   png_write_end(png_ptr, info_ptr);
   png_destroy_write_struct(&png_ptr, &info_ptr);
   fclose(fp);
}

//////////////////////////////////////////////////////
void pngwriter::line(int xfrom, int yfrom, int xto, int yto, int red, int green,int  blue)
{
   //  Bresenham Algorithm.
   //
   int dy = yto - yfrom;
   int dx = xto - xfrom;
   int stepx, stepy;

   if (dy < 0)
	 {
	dy = -dy;  stepy = -1;
	 }
   else
	 {
	stepy = 1;
	 }

   if (dx < 0)
	 {
	dx = -dx;  stepx = -1;
	 }
   else
	 {
	stepx = 1;
	 }
   dy <<= 1;     // dy is now 2*dy
   dx <<= 1;     // dx is now 2*dx

   this->plot(xfrom,yfrom,red,green,blue);

   if (dx > dy)
	 {
	int fraction = dy - (dx >> 1);

	while (xfrom != xto)
	  {
		 if (fraction >= 0)
		   {
		  yfrom += stepy;
		  fraction -= dx;
		   }
		 xfrom += stepx;
		 fraction += dy;
		 this->plot(xfrom,yfrom,red,green,blue);
	  }
	 }
   else
	 {
	int fraction = dx - (dy >> 1);
	while (yfrom != yto)
	  {
		 if (fraction >= 0)
		   {
		  xfrom += stepx;
		  fraction -= dy;
		   }
		 yfrom += stepy;
		 fraction += dx;
		 this->plot(xfrom,yfrom,red,green,blue);
	  }
	 }

}

void pngwriter::line(int xfrom, int yfrom, int xto, int yto, double red, double green,double  blue)
{
   this->line( xfrom,
		   yfrom,
		   xto,
		   yto,
		   int (red*65535),
		   int (green*65535),
		   int (blue*65535)
		   );
}

///////////////////////////////////////////////////////////////////////////////////////////
void pngwriter::square(int xfrom, int yfrom, int xto, int yto, int red, int green, int blue)
{
   this->line(xfrom, yfrom, xfrom, yto, red, green, blue);
   this->line(xto, yfrom, xto, yto, red, green, blue);
   this->line(xfrom, yfrom, xto, yfrom, red, green, blue);
   this->line(xfrom, yto, xto, yto, red, green, blue);
}

void pngwriter::square(int xfrom, int yfrom, int xto, int yto, double red, double green, double blue)
{
   this->square( xfrom,  yfrom,  xto,  yto, int(red*65535), int(green*65535), int(blue*65535));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void pngwriter::filledsquare(int xfrom, int yfrom, int xto, int yto, int red, int green, int blue)
{
   for(int caca = xfrom; caca <xto+1; caca++)
	 {
	this->line(caca, yfrom, caca, yto, red, green, blue);
	 }
}

void pngwriter::filledsquare(int xfrom, int yfrom, int xto, int yto, double red, double green, double blue)
{
   this->filledsquare( xfrom,  yfrom,  xto,  yto, int(red*65535), int(green*65535), int(blue*65535));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void pngwriter::circle(int xcentre, int ycentre, int radius, int red, int green, int blue)
{
   int x = 0;
   int y = radius;
   int p = (5 - radius*4)/4;

   circle_aux(xcentre, ycentre, x, y, red, green, blue);
   while (x < y)
	 {
	x++;
	if (p < 0)
	  {
		 p += 2*x+1;
	  }
	else
	  {
		 y--;
		 p += 2*(x-y)+1;
	  }
	circle_aux(xcentre, ycentre, x, y, red, green, blue);
	 }
}

void pngwriter::circle(int xcentre, int ycentre, int radius, double red, double green, double blue)
{
   this->circle(xcentre,ycentre,radius, int(red*65535), int(green*65535), int(blue*65535));
}

////////////////////////////////////////////////////////////

void pngwriter::circle_aux(int xcentre, int ycentre, int x, int y, int red, int green, int blue)
{
   if (x == 0)
	 {
	this->plot( xcentre, ycentre + y, red, green, blue);
	this->plot( xcentre, ycentre - y, red, green, blue);
	this->plot( xcentre + y, ycentre, red, green, blue);
	this->plot( xcentre - y, ycentre, red, green, blue);
	 }
   else
	 if (x == y)
	   {
	  this->plot( xcentre + x, ycentre + y, red, green, blue);
	  this->plot( xcentre - x, ycentre + y, red, green, blue);
	  this->plot( xcentre + x, ycentre - y, red, green, blue);
	  this->plot( xcentre - x, ycentre - y, red, green, blue);
	   }
   else
	 if (x < y)
	   {
	  this->plot( xcentre + x, ycentre + y, red, green, blue);
	  this->plot( xcentre - x, ycentre + y, red, green, blue);
	  this->plot( xcentre + x, ycentre - y, red, green, blue);
	  this->plot( xcentre - x, ycentre - y, red, green, blue);
	  this->plot( xcentre + y, ycentre + x, red, green, blue);
	  this->plot( xcentre - y, ycentre + x, red, green, blue);
	  this->plot( xcentre + y, ycentre - x, red, green, blue);
	  this->plot( xcentre - y, ycentre - x, red, green, blue);
	   }

}

////////////////////////////////////////////////////////////
void pngwriter::filledcircle(int xcentre, int ycentre, int radius, int red, int green, int blue)
{
   for(int jjj = ycentre-radius; jjj< ycentre+radius+1; jjj++)
	 {
	this->line(xcentre - int(sqrt((double)(radius*radius) - (-ycentre + jjj)*(-ycentre + jjj ))), jjj,
		   xcentre + int(sqrt((double)(radius*radius) - (-ycentre + jjj)*(-ycentre + jjj ))),jjj,red,green,blue);
	 }
}

void pngwriter::filledcircle(int xcentre, int ycentre, int radius, double red, double green, double blue)
{
   this->filledcircle( xcentre, ycentre,  radius, int(red*65535), int(green*65535), int(blue*65535));
}

////////////////Reading routines/////////////////////
/////////////////////////////////////////////////

// Modified with Mikkel's patch
void pngwriter::readfromfile(char * name)
{
   FILE            *fp;
   png_structp     png_ptr;
   png_infop       info_ptr;
   unsigned char   **image;
   unsigned long   width, height;
   int bit_depth, color_type, interlace_type;
   //   png_uint_32     i;
   //
   fp = fopen (name,"rb");
   if (fp==NULL)
	 {
	std::cerr << " PNGwriter::readfromfile - ERROR **: Error opening file \"" << std::flush;
	std::cerr << name <<std::flush;
	std::cerr << "\"." << std::endl << std::flush;
	perror(" PNGwriter::readfromfile - ERROR **");
	return;
	 }

   if(!check_if_png(name, &fp))
	 {
	std::cerr << " PNGwriter::readfromfile - ERROR **: Error opening file " << name << ". This may not be a valid png file. (check_if_png() failed)." << std::endl;
	// fp has been closed already if check_if_png() fails.
	return;
	 }

//   Code as it was before Sven's patch
/*   if(!read_png_info(fp, &png_ptr, &info_ptr))
	 {
	std::cerr << " PNGwriter::readfromfile - ERROR **: Error opening file " << name << ". read_png_info() failed." << std::endl;
	// fp has been closed already if read_png_info() fails.
	return;
	 }

   if(!read_png_image(fp, png_ptr, info_ptr, &image, &width, &height))
	 {
	std::cerr << " PNGwriter::readfromfile - ERROR **: Error opening file " << name << ". read_png_image() failed." << std::endl;
	// fp has been closed already if read_png_image() fails.
	return;
	 }

   //stuff should now be in image[][].

 */ //End of code as it was before Sven's patch.

   //Sven's patch starts here
   ////////////////////////////////////

   /*

   if(!read_png_info(fp, &png_ptr, &info_ptr))
	 {

	std::cerr << " PNGwriter::readfromfile - ERROR **: Error opening file " << name << ". read_png_info() failed." << std::endl;
	// fp has been closed already if read_png_info() fails.
	return;
	 }

   // UPDATE: Query info struct to get header info BEFORE reading the image

   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
   bit_depth_ = bit_depth;
   colortype_ = color_type;

   if(color_type == PNG_COLOR_TYPE_PALETTE)
	 {
	png_set_expand(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
	 }

   if(!read_png_image(fp, png_ptr, info_ptr, &image, &width, &height))
	 {
	std::cerr << " PNGwriter::readfromfile - ERROR **: Error opening file " << name << ". read_png_image() failed." << std::endl;
	// fp has been closed already if read_png_image() fails.
	return;
	 }

   //stuff should now be in image[][].
   */
   //Sven's patch ends here.
   ////////////////////////////////

   // Mikkel's patch starts here
   // ///////////////////////////////////

   if(!read_png_info(fp, &png_ptr, &info_ptr))
	 {

	std::cerr << " PNGwriter::readfromfile - ERROR **: Error opening file " << name << ". read_png_info() failed." << std::endl;
	// fp has been closed already if read_png_info() fails.
	  return;
	 }

   //Input transformations
   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
   bit_depth_ = bit_depth;
   colortype_ = color_type;


   // Changes palletted image to RGB
   if(color_type == PNG_COLOR_TYPE_PALETTE /*&& bit_depth<8*/)
	 {
	// png_set_expand(png_ptr);
	png_set_palette_to_rgb(png_ptr);  // Just an alias of png_set_expand()
	transformation_ = 1;
	 }

   // Transforms grescale images of less than 8 bits to 8 bits.
   if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth<8)
	 {
	// png_set_expand(png_ptr);
	png_set_gray_1_2_4_to_8(png_ptr);  // Just an alias of the above.
	transformation_ = 1;
	 }

   // Completely strips the alpha channel.
   if(color_type & PNG_COLOR_MASK_ALPHA)
	 {
	png_set_strip_alpha(png_ptr);
	transformation_ = 1;
	 }

   // Converts greyscale images to RGB.
   if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) // Used to be RGB, fixed it.
	 {
	png_set_gray_to_rgb(png_ptr);
	transformation_ = 1;
	 }

	 // If any of the above were applied,
   if(transformation_)
	 {
	 // png_set_gray_to_rgb(png_ptr);   //Is this really needed here?

	 // After setting the transformations, libpng can update your png_info structure to reflect any transformations
	 // you've requested with this call. This is most useful to update the info structure's rowbytes field so you can
	 // use it to allocate your image memory. This function will also update your palette with the correct screen_gamma
	 // and background if these have been given with the calls above.

	png_read_update_info(png_ptr, info_ptr);

	// Just in case any of these have changed?
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
	bit_depth_ = bit_depth;
	colortype_ = color_type;
	 }

   if(!read_png_image(fp, png_ptr, info_ptr, &image, &width, &height))
	 {
	std::cerr << " PNGwriter::readfromfile - ERROR **: Error opening file " << name << ". read_png_image() failed." << std::endl;
	// fp has been closed already if read_png_image() fails.
	return;
	 }

   //stuff should now be in image[][].

   // Mikkel's patch ends here
   // //////////////////////////////
   //
   if( image == NULL)
	 {
	std::cerr << " PNGwriter::readfromfile - ERROR **: Error opening file " << name << ". Can't assign memory (after read_png_image(), image is NULL)." << std::endl;
	fclose(fp);
	return;
	 }

   //First we must get rid of the image already there, and free the memory.
   int jjj;
   for (jjj = 0; jjj < height_; jjj++) free(graph_[jjj]);
   free(graph_);

   //Must reassign the new size of the read image
   width_ = width;
   height_ = height;

   //Graph now is the image.
   graph_ = image;

   rowbytes_ = png_get_rowbytes(png_ptr, info_ptr);

	// This was part of the original source, but has been moved up.
/*
   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
   bit_depth_ = bit_depth;
   colortype_ = color_type;
*/
  // if(color_type == PNG_COLOR_TYPE_PALETTE /*&& bit_depth<8*/   )
 /*    {
	png_set_expand(png_ptr);
	 }

   if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth<8)
	 {
	png_set_expand(png_ptr);
	 }

   if(color_type & PNG_COLOR_MASK_ALPHA)
	 {
	png_set_strip_alpha(png_ptr);
	 }

   if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	 {
	png_set_gray_to_rgb(png_ptr);
	 }

*/

   if((bit_depth_ !=16)&&(bit_depth_ !=8))
	 {
	std::cerr << " PNGwriter::readfromfile() - WARNING **: Input file is of unsupported type (bad bit_depth). Output will be unpredictable.\n";
	 }

// Thanks to Mikkel's patch, PNGwriter should now be able to handle these color types:

/*
color_type     - describes which color/alpha channels are present.
					 PNG_COLOR_TYPE_GRAY                        (bit depths 1, 2, 4, 8, 16)
					 PNG_COLOR_TYPE_GRAY_ALPHA                        (bit depths 8, 16)
					 PNG_COLOR_TYPE_PALETTE                        (bit depths 1, 2, 4, 8)
					 PNG_COLOR_TYPE_RGB                        (bit_depths 8, 16)
					 PNG_COLOR_TYPE_RGB_ALPHA                        (bit_depths 8, 16)

					 PNG_COLOR_MASK_PALETTE
					 PNG_COLOR_MASK_COLOR

color types.  Note that not all combinations are legal
#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_PALETTE  (PNG_COLOR_MASK_COLOR (2) | PNG_COLOR_MASK_PALETTE (1) )
#define PNG_COLOR_TYPE_RGB        (PNG_COLOR_MASK_COLOR (2) )
#define PNG_COLOR_TYPE_RGB_ALPHA  (PNG_COLOR_MASK_COLOR (2) | PNG_COLOR_MASK_ALPHA (4) )
#define PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA (4) )

aliases
#define PNG_COLOR_TYPE_RGBA  PNG_COLOR_TYPE_RGB_ALPHA
#define PNG_COLOR_TYPE_GA  PNG_COLOR_TYPE_GRAY_ALPHA

 These describe the color_type field in png_info.
 color type masks
#define PNG_COLOR_MASK_PALETTE    1
#define PNG_COLOR_MASK_COLOR      2
#define PNG_COLOR_MASK_ALPHA      4


					 */


   if(colortype_ !=2)
	 {
	std::cerr << " PNGwriter::readfromfile() - WARNING **: Input file is of unsupported type (bad color_type). Output will be unpredictable.\n";
	 }

   screengamma_ = 2.2;
   double          file_gamma,screen_gamma;
   screen_gamma = screengamma_;
   if (png_get_gAMA(png_ptr, info_ptr, &file_gamma))
	 {
	png_set_gamma(png_ptr,screen_gamma,file_gamma);
	 }
   else
	 {
	png_set_gamma(png_ptr, screen_gamma,0.45);
	 }

   filegamma_ = file_gamma;

   fclose(fp);
}

///////////////////////////////////////////////////////

void pngwriter::readfromfile(const char * name)
{
   this->readfromfile((char *)(name));
}

/////////////////////////////////////////////////////////
int pngwriter::check_if_png(char *file_name, FILE **fp)
{
   char    sig[PNG_BYTES_TO_CHECK];

   if ( /*(*fp = fopen(file_name, "rb")) */  *fp == NULL) // Fixed 10 10 04
	 {
	//   exit(EXIT_FAILURE);
	std::cerr << " PNGwriter::check_if_png - ERROR **: Could not open file  " << file_name << " to read." << std::endl;
	perror(" PNGwriter::check_if_png - ERROR **");
	return 0;
	 }

   if (fread(sig, 1, PNG_BYTES_TO_CHECK, *fp) != PNG_BYTES_TO_CHECK)
	 {
	//exit(EXIT_FAILURE);
	std::cerr << " PNGwriter::check_if_png - ERROR **: File " << file_name << " does not appear to be a valid PNG file." << std::endl;
	perror(" PNGwriter::check_if_png - ERROR **");
	fclose(*fp);
	return 0;
	 }

   if (png_sig_cmp( (png_bytep) sig, (png_size_t)0, PNG_BYTES_TO_CHECK) /*png_check_sig((png_bytep) sig, PNG_BYTES_TO_CHECK)*/ )
	 {
	std::cerr << " PNGwriter::check_if_png - ERROR **: File " << file_name << " does not appear to be a valid PNG file. png_check_sig() failed." << std::endl;
	fclose(*fp);
	return 0;
	 }



   return 1; //Success
}

///////////////////////////////////////////////////////
int pngwriter::read_png_info(FILE *fp, png_structp *png_ptr, png_infop *info_ptr)
{
   *png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (*png_ptr == NULL)
	 {
	std::cerr << " PNGwriter::read_png_info - ERROR **: Could not create read_struct." << std::endl;
	fclose(fp);
	return 0;
	//exit(EXIT_FAILURE);
	 }
   *info_ptr = png_create_info_struct(*png_ptr);
   if (*info_ptr == NULL)
	 {
	png_destroy_read_struct(png_ptr, (png_infopp)NULL, (png_infopp)NULL);
	std::cerr << " PNGwriter::read_png_info - ERROR **: Could not create info_struct." << std::endl;
	//exit(EXIT_FAILURE);
	fclose(fp);
	return 0;
	 }
   if (setjmp((*png_ptr)->jmpbuf)) /*(setjmp(png_jmpbuf(*png_ptr)) )*//////////////////////////////////////
	 {
	png_destroy_read_struct(png_ptr, info_ptr, (png_infopp)NULL);
	std::cerr << " PNGwriter::read_png_info - ERROR **: This file may be a corrupted PNG file. (setjmp(*png_ptr)->jmpbf) failed)." << std::endl;
	fclose(fp);
	return 0;
	//exit(EXIT_FAILURE);
	 }
   png_init_io(*png_ptr, fp);
   png_set_sig_bytes(*png_ptr, PNG_BYTES_TO_CHECK);
   png_read_info(*png_ptr, *info_ptr);

   return 1;
}

////////////////////////////////////////////////////////////
int pngwriter::read_png_image(FILE *fp, png_structp png_ptr, png_infop info_ptr,
				  png_bytepp *image, png_uint_32 *width, png_uint_32 *height)
{
   unsigned int i,j;

   *width = png_get_image_width(png_ptr, info_ptr);
   *height = png_get_image_height(png_ptr, info_ptr);

   if( width == NULL)
	 {
	std::cerr << " PNGwriter::read_png_image - ERROR **: png_get_image_width() returned NULL pointer." << std::endl;
	fclose(fp);
	return 0;
	 }

   if( height == NULL)
	 {
	std::cerr << " PNGwriter::read_png_image - ERROR **: png_get_image_height() returned NULL pointer." << std::endl;
	fclose(fp);
	return 0;
	 }

   if ((*image = (png_bytepp)malloc(*height * sizeof(png_bytep))) == NULL)
	 {
	std::cerr << " PNGwriter::read_png_image - ERROR **: Could not allocate memory for reading image." << std::endl;
	fclose(fp);
	return 0;
	//exit(EXIT_FAILURE);
	 }
   for (i = 0; i < *height; i++)
	 {
	(*image)[i] = (png_bytep)malloc(png_get_rowbytes(png_ptr, info_ptr));
	if ((*image)[i] == NULL)
	  {
		 for (j = 0; j < i; j++) free((*image)[j]);
		 free(*image);
		 fclose(fp);
		 std::cerr << " PNGwriter::read_png_image - ERROR **: Could not allocate memory for reading image." << std::endl;
		 return 0;
		 //exit(EXIT_FAILURE);
	  }
	 }
   png_read_image(png_ptr, *image);

   return 1;
}

///////////////////////////////////
int pngwriter::getheight(void)
{
   return height_;
}

int pngwriter::getwidth(void)
{
   return width_;
}


int pngwriter::getbitdepth(void)
{
   return bit_depth_;
}

int pngwriter::getcolortype(void)
{
   return colortype_;
}

double pngwriter::getgamma(void)
{
   return filegamma_;
}

void pngwriter::setgamma(double gamma)
{
   filegamma_ = gamma;
}

// The algorithms HSVtoRGB and RGBtoHSV were found at http://www.cs.rit.edu/~ncs/
//  which is a page that belongs to Nan C. Schaller, though
//  these algorithms appear to be the work of Eugene Vishnevsky.
//////////////////////////////////////////////
void pngwriter::HSVtoRGB( double *r, double *g, double *b, double h, double s, double v )
{
   // r,g,b values are from 0 to 1
   // h = [0,1], s = [0,1], v = [0,1]
   // if s == 0, then h = -1 (undefined)
   //
   h = h*360.0;

   int i;
   double f, p, q, t;
   if( s == 0 )
	 {
	// achromatic (grey)
	*r = *g = *b = v;
	return;
	 }

   h /= 60;                        // sector 0 to 5
   i = int(floor( h ));
   f = h - i;                      // factorial part of h
   p = v * ( 1 - s );
   q = v * ( 1 - s * f );
   t = v * ( 1 - s * ( 1 - f ) );

   switch( i )
	 {
	  case 0:
	*r = v;
	*g = t;
	*b = p;
	break;
	  case 1:
	*r = q;
	*g = v;
	*b = p;
	break;
	  case 2:
	*r = p;
	*g = v;
	*b = t;
	break;
	  case 3:
	*r = p;
	*g = q;
	*b = v;
	break;
	  case 4:
	*r = t;
	*g = p;
	*b = v;
	break;
	  default:                // case 5:
	*r = v;
	*g = p;
	*b = q;
	break;
	 }
}

void pngwriter::RGBtoHSV( float r, float g, float b, float *h, float *s, float *v )
{

   float min=0.0; //These values are not used.
   float max=1.0;
   float delta;

   if( (r>=g)&&(r>=b) )
	 {
	max = r;
	 }
   if( (g>=r)&&(g>=b) )
	 {
	max = g;
	 }
   if( (b>=g)&&(b>=r) )
	 {
	max = b;
	 }

   if( (r<=g)&&(r<=b) )
	 {
	min = r;
	 }
   if( (g<=r)&&(g<=b) )
	 {
	min = g;
	 }
   if( (b<=g)&&(b<=r) )
	 {
	min = b;
	 }

   *v = max;                               // v

   delta = max - min;

   if( max != 0 )
	 *s = delta / max;               // s
   else
	 {

	r = g = b = 0;                // s = 0, v is undefined
	*s = 0;
	*h = -1;
	return;
	 }

   if( r == max )
	 *h = ( g - b ) / delta;         // between yellow & magenta
   else if( g == max )
	 *h = 2 + ( b - r ) / delta;     // between cyan & yellow
   else
	 *h = 4 + ( r - g ) / delta;     // between magenta & cyan

   *h *= 60;                               // degrees
   if( *h < 0 )
	 *h += 360;

}

//
//////////////////////////////////////////////////////////////////////////////////
void pngwriter::plotHSV(int x, int y, double hue, double saturation, double value)
{
   double red,green,blue;
   double *redp;
   double *greenp;
   double *bluep;

   redp = &red;
   greenp = &green;
   bluep = &blue;

   HSVtoRGB(redp,greenp,bluep,hue,saturation,value);
   plot(x,y,red,green,blue);
}

void pngwriter::plotHSV(int x, int y, int hue, int saturation, int value)
{
   plotHSV(x, y, double(hue)/65535.0, double(saturation)/65535.0,  double(value)/65535.0);
}

//
//////////////////////////////////////////////////////////////////////////////////
double pngwriter::dreadHSV(int x, int y, int colour)
{
   if( (x>0)&&(x<=width_)&&(y>0)&&(y<=height_) )
	 {

	float * huep;
	float * saturationp;
	float * valuep;
	float red,green,blue;
	float hue, saturation, value;

	red = float(dread(x,y,1));
	green = float(dread(x,y,2));
	blue = float(dread(x,y,3));

	huep = &hue;
	saturationp = &saturation;
	valuep = &value;

	RGBtoHSV( red,  green,  blue, huep,  saturationp, valuep );

	if(colour == 1)
	  {
		 return double(hue)/360.0;
	  }

	else if(colour == 2)
	  {
		 return saturation;
	  }

	else if(colour == 3)
	  {
		 return value;
	  }

	std::cerr << " PNGwriter::dreadHSV - ERROR **: Called with wrong colour argument: should be 1, 2 or 3; was: " << colour << "." << std::endl;
	 }
   return 0.0;
}

//
//////////////////////////////////////////////////////////////////////////////////
int pngwriter::readHSV(int x, int y, int colour)
{
   if( (x>0)&&(x<=width_)&&(y>0)&&(y<=height_) )
	 {

	float * huep;
	float * saturationp;
	float * valuep;
	float red,green,blue;
	float hue, saturation, value;

	red = float(dread(x,y,1));
	green = float(dread(x,y,2));
	blue = float(dread(x,y,3));

	huep = &hue;
	saturationp = &saturation;
	valuep = &value;

	RGBtoHSV( red,  green,  blue, huep,  saturationp, valuep );

	if(colour == 1)
	  {
		 return int(65535*(double(hue)/360.0));
	  }

	else if(colour == 2)
	  {
		 return int(65535*saturation);
	  }

	else if(colour == 3)
	  {
		 return int(65535*value);
	  }

	std::cerr << " PNGwriter::readHSV - ERROR **: Called with wrong colour argument: should be 1, 2 or 3; was: " << colour << "." << std::endl;
	return 0;
	 }
   else
	 {
	return 0;
	 }
}

void pngwriter::setcompressionlevel(int level)
{
   if( (level < -1)||(level > 9) )
	 {
	std::cerr << " PNGwriter::setcompressionlevel - ERROR **: Called with wrong compression level: should be -1 to 9, was: " << level << "." << std::endl;
	 }
   compressionlevel_ = level;
}

// An implementation of a Bezier curve.
void pngwriter::bezier(  int startPtX, int startPtY,
			 int startControlX, int startControlY,
			 int endPtX, int endPtY,
			 int endControlX, int endControlY,
			 double red, double green, double blue)
{

   double cx = 3.0*(startControlX - startPtX);
   double bx = 3.0*(endControlX - startControlX) - cx;
   double ax = double(endPtX - startPtX - cx - bx);

   double cy = 3.0*(startControlY - startPtY);
   double by = 3.0*(endControlY - startControlY) - cy;
   double ay = double(endPtY - startPtY - cy - by);

   double x,y,newx,newy;
   x = startPtX;
   y = startPtY;

   for(double t = 0.0; t<=1.005; t += 0.005)
	 {
	newx = startPtX + t*(double(cx) + t*(double(bx) + t*(double(ax))));
	newy = startPtY + t*(double(cy) + t*(double(by) + t*(double(ay))));
	this->line(int(x),int(y),int(newx),int(newy),red,green,blue);
	x = newx;
	y = newy;
	 }
}

//int version of bezier
void pngwriter::bezier(  int startPtX, int startPtY,
			 int startControlX, int startControlY,
			 int endPtX, int endPtY,
			 int endControlX, int endControlY,
			 int  red, int  green, int blue)
{
   this->bezier(   startPtX,  startPtY,
		   startControlX, startControlY,
		   endPtX, endPtY,
		   endControlX,  endControlY,
		   double(red)/65535.0,  double(green)/65535.0,  double(blue)/65535.0);
}

/*
int pngwriter::getcompressionlevel(void)
{
   return png_get_compression_level(png_ptr);
}
*/

double pngwriter::version(void)
{
   const char *a = "Jeramy Webb (jeramyw@gmail.com), Mike Heller (mkheller@gmail.com)"; // For their generosity ;-)
   char b = a[27];
   b++;
   return (PNGWRITER_VERSION);
}

void pngwriter::write_png(void)
{
   this->close();
}

#ifndef NO_FREETYPE

// Freetype-based text rendering functions.
///////////////////////////////////////////
void pngwriter::plot_text( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double red, double green, double blue)
{
   FT_Library  library;
   FT_Face     face;
   FT_Matrix   matrix;      // transformation matrix
   FT_Vector   pen;

   FT_UInt glyph_index;
   FT_Error error;

   FT_Bool use_kerning;
   FT_UInt previous = 0;

   /* Set up transformation Matrix */
   matrix.xx = (FT_Fixed)( cos(angle)*0x10000);   /* It would make more sense to do this (below), but, bizzarely, */
   matrix.xy = (FT_Fixed)(-sin(angle)*0x10000);   /* if one does, FT_Load_Glyph fails consistently.               */
   matrix.yx = (FT_Fixed)( sin(angle)*0x10000);  //   matrix.yx = - matrix.xy;
   matrix.yy = (FT_Fixed)( cos(angle)*0x10000);  //   matrix.yy = matrix.xx;

   /* Place starting coordinates in adequate form. */
   pen.x = x_start*64 ;
   pen.y =   (int)(y_start/64.0);

   /*Count the length of the string */
   int num_chars = strlen(text);

   /* Initialize FT Library object */
   error = FT_Init_FreeType( &library );
   if (error) { std::cerr << " PNGwriter::plot_text - ERROR **: FreeType: Could not init Library."<< std::endl; return;}

   /* Initialize FT face object */
   error = FT_New_Face( library,face_path,0,&face );
   if ( error == FT_Err_Unknown_File_Format ) { std::cerr << " PNGwriter::plot_text - ERROR **: FreeType: Font was opened, but type not supported."<< std::endl; return; } else if (error){ std::cerr << " PNGwriter::plot_text - ERROR **: FreeType: Could not find or load font file."<< std::endl; return; }

   /* Set the Char size */
   error = FT_Set_Char_Size( face,          /* handle to face object           */
				 0,             /* char_width in 1/64th of points  */
				 fontsize*64,   /* char_height in 1/64th of points */
				 100,           /* horizontal device resolution    */
				 100 );         /* vertical device resolution      */

   /* A way of accesing the glyph directly */
   FT_GlyphSlot  slot = face->glyph;  // a small shortcut

   /* Does the font file support kerning? */
   use_kerning = FT_HAS_KERNING( face );

   int n;
   for ( n = 0; n < num_chars; n++ )
	 {
	/* Convert character code to glyph index */
	glyph_index = FT_Get_Char_Index( face, text[n] );

	/* Retrieve kerning distance and move pen position */
	if ( use_kerning && previous&& glyph_index )
	  {
		 FT_Vector  delta;
		 FT_Get_Kerning( face,
				 previous,
				 glyph_index,
				 ft_kerning_default, //FT_KERNING_DEFAULT,
				 &delta );

		 /* Transform this kerning distance into rotated space */
		 pen.x += (int) (((double) delta.x)*cos(angle));
		 pen.y +=  (int) (((double) delta.x)*( sin(angle)));
	  }

	/* Set transform */
	FT_Set_Transform( face, &matrix, &pen );

/*set char size*/

	if (error) { std::cerr << " PNGwriter::plot_text - ERROR **: FreeType: Set char size error." << std::endl; return;};

	/* Retrieve glyph index from character code */
	glyph_index = FT_Get_Char_Index( face, text[n] );

	/* Load glyph image into the slot (erase previous one) */
	error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
	if (error) { std::cerr << " PNGwriter::plot_text - ERROR **: FreeType: Could not load glyph (in loop). (FreeType error "<< std::hex << error <<")." << std::endl; return;}

	/* Convert to an anti-aliased bitmap */
	//	error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );
	error = FT_Render_Glyph( face->glyph, ft_render_mode_normal );
	if (error) { std::cerr << " PNGwriter::plot_text - ERROR **: FreeType: Render glyph error." << std::endl; return;}

	/* Now, draw to our target surface */
	my_draw_bitmap( &slot->bitmap,
			slot->bitmap_left,
			y_start + slot->bitmap_top,
			red,
			green,
			blue );

	/* Advance to the next position */
	pen.x += slot->advance.x;
	pen.y += slot->advance.y;

	/* record current glyph index */
	previous = glyph_index;
	 }

   /* Free the face and the library objects */
   FT_Done_Face    ( face );
   FT_Done_FreeType( library );
}

void pngwriter::plot_text_utf8( char * face_path, int fontsize, int x_start, int y_start, double angle,  char * text, double red, double green, double blue)
{
   FT_Library  library;
   FT_Face     face;
   FT_Matrix   matrix;      // transformation matrix
   FT_Vector   pen;

   FT_UInt glyph_index;
   FT_Error error;

   FT_Bool use_kerning;
   FT_UInt previous = 0;

   /* Set up transformation Matrix */
   matrix.xx = (FT_Fixed)( cos(angle)*0x10000);   /* It would make more sense to do this (below), but, bizzarely, */
   matrix.xy = (FT_Fixed)(-sin(angle)*0x10000);   /* if one does, FT_Load_Glyph fails consistently.               */
   matrix.yx = (FT_Fixed)( sin(angle)*0x10000);  //   matrix.yx = - matrix.xy;
   matrix.yy = (FT_Fixed)( cos(angle)*0x10000);  //   matrix.yy = matrix.xx;

   /* Place starting coordinates in adequate form. */
   pen.x = x_start*64 ;
   pen.y = (int)(y_start/64.0);

   /*Count the length of the string */
   int num_bytes=0;
   while(text[num_bytes]!=0)
	 {
	num_bytes++;
	 }

	 /*
   std::cout << "Num bytes is: "<< num_bytes << std::endl;
   */

   //The array of ucs4 glyph indexes, which will by at most the number of bytes in the utf-8 file.
   long * ucs4text;
   ucs4text = new long[num_bytes+1];

   unsigned char u,v,w,x,y,z;

   int num_chars=0;

   long iii=0;

   while(iii<num_bytes)
	 {
	z = text[iii];

	if(z<=127)
	  {
		 ucs4text[num_chars] = z;
	  }

	if((192<=z)&&(z<=223))
	  {
		 iii++; y = text[iii];
		 ucs4text[num_chars] = (z-192)*64 + (y -128);
	  }

	if((224<=z)&&(z<=239))
	  {
		 iii++; y = text[iii];
		 iii++; x = text[iii];
		 ucs4text[num_chars] = (z-224)*4096 + (y -128)*64 + (x-128);
	  }

	if((240<=z)&&(z<=247))
	  {
		 iii++; y = text[iii];
		 iii++; x = text[iii];
		 iii++; w = text[iii];
		 ucs4text[num_chars] = (z-240)*262144 + (y -128)*4096 + (x-128)*64 + (w-128);
	  }

	if((248<=z)&&(z<=251))
	  {
		 iii++; y = text[iii];
		 iii++; x = text[iii];
		 iii++; w = text[iii];
		 iii++; v = text[iii];
		 ucs4text[num_chars] = (z-248)*16777216 + (y -128)*262144 + (x-128)*4096 + (w-128)*64 +(v-128);
	  }

	if((252==z)||(z==253))
	  {
		 iii++; y = text[iii];
		 iii++; x = text[iii];
		 iii++; w = text[iii];
		 iii++; v = text[iii];
		 u = text[iii];
		 ucs4text[num_chars] = (z-252)*1073741824 + (y -128)*16777216   + (x-128)*262144 + (w-128)*4096 +(v-128)*64 + (u-128);
	  }

	if((z==254)||(z==255))
	  {
		 std::cerr << " PNGwriter::plot_text_utf8 - ERROR **: Problem with character: invalid UTF-8 data."<< std::endl;
	  }
	// std::cerr << "\nProblem at " << iii << ".\n";
	//
	iii++;
	num_chars++;
	 }

   // num_chars now contains the number of characters in the string.
   /*
   std::cout << "Num chars is: "<< num_chars << std::endl;
   */

   /* Initialize FT Library object */
   error = FT_Init_FreeType( &library );
   if (error) { std::cerr << " PNGwriter::plot_text_utf8 - ERROR **: FreeType: Could not init Library."<< std::endl; return;}

   /* Initialize FT face object */
   error = FT_New_Face( library,face_path,0,&face );
   if ( error == FT_Err_Unknown_File_Format ) { std::cerr << " PNGwriter::plot_text_utf8 - ERROR **: FreeType: Font was opened, but type not supported."<< std::endl; return; } else if (error){ std::cerr << " PNGwriter::plot_text - ERROR **: FreeType: Could not find or load font file."<< std::endl; return; }

   /* Set the Char size */
   error = FT_Set_Char_Size( face,          /* handle to face object           */
				 0,             /* char_width in 1/64th of points  */
				 fontsize*64,   /* char_height in 1/64th of points */
				 100,           /* horizontal device resolution    */
				 100 );         /* vertical device resolution      */

   /* A way of accesing the glyph directly */
   FT_GlyphSlot  slot = face->glyph;  // a small shortcut

   /* Does the font file support kerning? */
   use_kerning = FT_HAS_KERNING( face );

   int n;
   for ( n = 0; n < num_chars; n++ )
	 {
	/* Convert character code to glyph index */
	glyph_index = FT_Get_Char_Index( face, ucs4text[n] );

	/* Retrieve kerning distance and move pen position */
	if ( use_kerning && previous&& glyph_index )
	  {
		 FT_Vector  delta;
		 FT_Get_Kerning( face,
				 previous,
				 glyph_index,
				 ft_kerning_default, //FT_KERNING_DEFAULT,
				 &delta );

		 /* Transform this kerning distance into rotated space */
		 pen.x += (int) (((double) delta.x)*cos(angle));
		 pen.y +=  (int) (((double) delta.x)*( sin(angle)));
	  }

	/* Set transform */
	FT_Set_Transform( face, &matrix, &pen );

/*set char size*/

	if (error) { std::cerr << " PNGwriter::plot_text_utf8 - ERROR **: FreeType: Set char size error." << std::endl; return;};

	/* Retrieve glyph index from character code */
	glyph_index = FT_Get_Char_Index( face, ucs4text[n] );

	/* Load glyph image into the slot (erase previous one) */
	error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
	if (error) { std::cerr << " PNGwriter::plot_text_utf8 - ERROR **: FreeType: Could not load glyph (in loop). (FreeType error "<< std::hex << error <<")." << std::endl; return;}

	/* Convert to an anti-aliased bitmap */
	error = FT_Render_Glyph( face->glyph, ft_render_mode_normal );
	if (error) { std::cerr << " PNGwriter::plot_text_utf8 - ERROR **: FreeType: Render glyph error." << std::endl; return;}

	/* Now, draw to our target surface */
	my_draw_bitmap( &slot->bitmap,
			slot->bitmap_left,
			y_start + slot->bitmap_top,
			red,
			green,
			blue );

	/* Advance to the next position */
	pen.x += slot->advance.x;
	pen.y += slot->advance.y;

	/* record current glyph index */
	previous = glyph_index;
	 }

   /* Free the face and the library objects */
   FT_Done_Face    ( face );
   FT_Done_FreeType( library );

   delete[] ucs4text;
}

void pngwriter::plot_text( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, int red, int green, int blue)
{
   plot_text( face_path, fontsize, x_start, y_start,  angle,  text,  ((double) red)/65535.0,  ((double) green)/65535.0,  ((double) blue)/65535.0   );
}

void pngwriter::plot_text_utf8( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, int red, int green, int blue)
{
   plot_text_utf8( face_path, fontsize, x_start, y_start,  angle,  text,  ((double) red)/65535.0,  ((double) green)/65535.0,  ((double) blue)/65535.0   );
}

void pngwriter::my_draw_bitmap( FT_Bitmap * bitmap, int x, int y, double red, double green, double blue)
{
   double temp;
   for(int j=1; j<bitmap->rows+1; j++)
	 {
	for(int i=1; i< bitmap->width + 1; i++)
	  {
		 temp = (double)(bitmap->buffer[(j-1)*bitmap->width + (i-1)] )/255.0;

		 if(temp)
		   {
		  this->plot(x + i,
				 y  - j,
				 temp*red + (1-temp)*(this->dread(x+i,y-j,1)),
				 temp*green + (1-temp)*(this->dread(x+i,y-j,2)),
				 temp*blue + (1-temp)*(this->dread(x+i,y-j,3))
				 );
		   }
	  }
	 }
}



//////////// Get text width

//put in freetype section

int pngwriter::get_text_width(char * face_path, int fontsize, char * text)
{

   FT_Library  library;
   FT_Face     face;
   FT_Matrix   matrix;      // transformation matrix
   FT_Vector   pen;

   FT_UInt glyph_index;
   FT_Error error;

   FT_Bool use_kerning;
   FT_UInt previous = 0;

   /* Set up transformation Matrix */
   matrix.xx = (FT_Fixed)( 1.0*0x10000);   /* It would make more sense to do this (below), but, bizzarely, */
   matrix.xy = (FT_Fixed)( 0.0*0x10000);   /* if one does, FT_Load_Glyph fails consistently.               */
   matrix.yx = (FT_Fixed)( 0.0*0x10000);  //   matrix.yx = - matrix.xy;
   matrix.yy = (FT_Fixed)( 1.0*0x10000);  //   matrix.yy = matrix.xx;

   /* Place starting coordinates in adequate form. */
   pen.x = 0;
   pen.y = 0;

   /*Count the length of the string */
   int num_chars = strlen(text);

   /* Initialize FT Library object */
   error = FT_Init_FreeType( &library );
   if (error) { std::cerr << " PNGwriter::get_text_width - ERROR **: FreeType: Could not init Library."<< std::endl; return 0;}

   /* Initialize FT face object */
   error = FT_New_Face( library,face_path,0,&face );
   if ( error == FT_Err_Unknown_File_Format ) { std::cerr << " PNGwriter::get_text_width - ERROR **: FreeType: Font was opened, but type not supported."<< std::endl; return 0; } else if (error){ std::cerr << " PNGwriter::get_text_width - ERROR **: FreeType: Could not find or load font file." << std::endl; return 0; }

   /* Set the Char size */
   error = FT_Set_Char_Size( face,          /* handle to face object           */
				 0,             /* char_width in 1/64th of points  */
				 fontsize*64,   /* char_height in 1/64th of points */
				 100,           /* horizontal device resolution    */
				 100 );         /* vertical device resolution      */

   /* A way of accesing the glyph directly */
   FT_GlyphSlot  slot = face->glyph;  // a small shortcut

   /* Does the font file support kerning? */
   use_kerning = FT_HAS_KERNING( face );

   int n;
   for ( n = 0; n < num_chars; n++ )
	 {
	/* Convert character code to glyph index */
	glyph_index = FT_Get_Char_Index( face, text[n] );

	/* Retrieve kerning distance and move pen position */
	if ( use_kerning && previous&& glyph_index )
	  {
		 FT_Vector  delta;
		 FT_Get_Kerning( face,
				 previous,
				 glyph_index,
				 ft_kerning_default, //FT_KERNING_DEFAULT,
				 &delta );

		 /* Transform this kerning distance into rotated space */
		 pen.x += (int) ( delta.x);
		 pen.y +=  0;
	  }

	/* Set transform */
	FT_Set_Transform( face, &matrix, &pen );

/*set char size*/

	if (error) { std::cerr << " PNGwriter::get_text_width - ERROR **: FreeType: Set char size error." << std::endl; return 0;};

	/* Retrieve glyph index from character code */
	glyph_index = FT_Get_Char_Index( face, text[n] );

	/* Load glyph image into the slot (erase previous one) */
	error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
	if (error) { std::cerr << " PNGwriter::get_text_width - ERROR **: FreeType: Could not load glyph (in loop). (FreeType error "<< std::hex << error <<")." << std::endl; return 0;}

	/* Convert to an anti-aliased bitmap */
	//	error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );
	error = FT_Render_Glyph( face->glyph, ft_render_mode_normal );
	if (error) { std::cerr << " PNGwriter::get_text_width - ERROR **: FreeType: Render glyph error." << std::endl; return 0;}

	/* Now, draw to our target surface */
/*	my_draw_bitmap( &slot->bitmap,
			slot->bitmap_left,
			slot->bitmap_top,
			red,
			green,
			blue );
*/
	/* Advance to the next position */
	pen.x += slot->advance.x;
//	std::cout << ((double) pen.x)/64.0 << std::endl;
	pen.y += slot->advance.y;

	/* record current glyph index */
	previous = glyph_index;
	 }


   /* Free the face and the library objects */
   FT_Done_Face    ( face );
   FT_Done_FreeType( library );

   return (int)( ((double)pen.x)/64.0 );
}


int pngwriter::get_text_width_utf8(char * face_path, int fontsize,  char * text)
{
   FT_Library  library;
   FT_Face     face;
   FT_Matrix   matrix;      // transformation matrix
   FT_Vector   pen;

   FT_UInt glyph_index;
   FT_Error error;

   FT_Bool use_kerning;
   FT_UInt previous = 0;

   /* Set up transformation Matrix */
   matrix.xx = (FT_Fixed)( 0x10000);   /* It would make more sense to do this (below), but, bizzarely, */
   matrix.xy = (FT_Fixed)( 0*0x10000);   /* if one does, FT_Load_Glyph fails consistently.               */
   matrix.yx = (FT_Fixed)( 0*0x10000);  //   matrix.yx = - matrix.xy;
   matrix.yy = (FT_Fixed)( 0x10000);  //   matrix.yy = matrix.xx;

   /* Place starting coordinates in adequate form. */
   pen.x = 0 ;
   pen.y = 0;

   /*Count the length of the string */
   int num_bytes=0;
   while(text[num_bytes]!=0)
	 {
	num_bytes++;
	 }

	 /*
   std::cout << "Num bytes is: "<< num_bytes << std::endl;
   */

   //The array of ucs4 glyph indexes, which will by at most the number of bytes in the utf-8 file.
   long * ucs4text;
   ucs4text = new long[num_bytes+1];

   unsigned char u,v,w,x,y,z;

   int num_chars=0;

   long iii=0;

   while(iii<num_bytes)
	 {
	z = text[iii];

	if(z<=127)
	  {
		 ucs4text[num_chars] = z;
	  }

	if((192<=z)&&(z<=223))
	  {
		 iii++; y = text[iii];
		 ucs4text[num_chars] = (z-192)*64 + (y -128);
	  }

	if((224<=z)&&(z<=239))
	  {
		 iii++; y = text[iii];
		 iii++; x = text[iii];
		 ucs4text[num_chars] = (z-224)*4096 + (y -128)*64 + (x-128);
	  }

	if((240<=z)&&(z<=247))
	  {
		 iii++; y = text[iii];
		 iii++; x = text[iii];
		 iii++; w = text[iii];
		 ucs4text[num_chars] = (z-240)*262144 + (y -128)*4096 + (x-128)*64 + (w-128);
	  }

	if((248<=z)&&(z<=251))
	  {
		 iii++; y = text[iii];
		 iii++; x = text[iii];
		 iii++; w = text[iii];
		 iii++; v = text[iii];
		 ucs4text[num_chars] = (z-248)*16777216 + (y -128)*262144 + (x-128)*4096 + (w-128)*64 +(v-128);
	  }

	if((252==z)||(z==253))
	  {
		 iii++; y = text[iii];
		 iii++; x = text[iii];
		 iii++; w = text[iii];
		 iii++; v = text[iii];
		 u = text[iii];
		 ucs4text[num_chars] = (z-252)*1073741824 + (y -128)*16777216   + (x-128)*262144 + (w-128)*4096 +(v-128)*64 + (u-128);
	  }

	if((z==254)||(z==255))
	  {
		 std::cerr << " PNGwriter::get_text_width_utf8 - ERROR **: Problem with character: invalid UTF-8 data."<< std::endl;
	  }
	// std::cerr << "\nProblem at " << iii << ".\n";
	//
	iii++;
	num_chars++;
	 }

   // num_chars now contains the number of characters in the string.
   /*
   std::cout << "Num chars is: "<< num_chars << std::endl;
   */

   /* Initialize FT Library object */
   error = FT_Init_FreeType( &library );
   if (error) { std::cerr << " PNGwriter::get_text_width_utf8 - ERROR **: FreeType: Could not init Library."<< std::endl; return 0;}

   /* Initialize FT face object */
   error = FT_New_Face( library,face_path,0,&face );
   if ( error == FT_Err_Unknown_File_Format ) { std::cerr << " PNGwriter::get_text_width_utf8 - ERROR **: FreeType: Font was opened, but type not supported."<< std::endl; return 0; } else if (error){ std::cerr << " PNGwriter::plot_text - ERROR **: FreeType: Could not find or load font file."<< std::endl; return 0; }

   /* Set the Char size */
   error = FT_Set_Char_Size( face,          /* handle to face object           */
				 0,             /* char_width in 1/64th of points  */
				 fontsize*64,   /* char_height in 1/64th of points */
				 100,           /* horizontal device resolution    */
				 100 );         /* vertical device resolution      */

   /* A way of accesing the glyph directly */
   FT_GlyphSlot  slot = face->glyph;  // a small shortcut

   /* Does the font file support kerning? */
   use_kerning = FT_HAS_KERNING( face );

   int n;
   for ( n = 0; n < num_chars; n++ )
	 {
	/* Convert character code to glyph index */
	glyph_index = FT_Get_Char_Index( face, ucs4text[n] );

	/* Retrieve kerning distance and move pen position */
	if ( use_kerning && previous&& glyph_index )
	  {
		 FT_Vector  delta;
		 FT_Get_Kerning( face,
				 previous,
				 glyph_index,
				 ft_kerning_default, //FT_KERNING_DEFAULT,
				 &delta );

		 /* Transform this kerning distance into rotated space */
		 pen.x += (int) (delta.x);
		 pen.y +=  0;
	  }

	/* Set transform */
	FT_Set_Transform( face, &matrix, &pen );

/*set char size*/

	if (error) { std::cerr << " PNGwriter::get_text_width_utf8 - ERROR **: FreeType: Set char size error." << std::endl; return 0;};

	/* Retrieve glyph index from character code */
	glyph_index = FT_Get_Char_Index( face, ucs4text[n] );

	/* Load glyph image into the slot (erase previous one) */
	error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
	if (error) { std::cerr << " PNGwriter::get_text_width_utf8 - ERROR **: FreeType: Could not load glyph (in loop). (FreeType error "<< std::hex << error <<")." << std::endl; return 0;}

	/* Convert to an anti-aliased bitmap */
	error = FT_Render_Glyph( face->glyph, ft_render_mode_normal );
	if (error) { std::cerr << " PNGwriter::get_text_width_utf8 - ERROR **: FreeType: Render glyph error." << std::endl; return 0;}

	/* Now, draw to our target surface */
/*	my_draw_bitmap( &slot->bitmap,
			slot->bitmap_left,
			y_start + slot->bitmap_top,
			red,
			green,
			blue );
*/
	/* Advance to the next position */
	pen.x += slot->advance.x;
	pen.y += slot->advance.y;

	/* record current glyph index */
	previous = glyph_index;
	 }

   /* Free the face and the library objects */
   FT_Done_Face    ( face );
   FT_Done_FreeType( library );

   delete[] ucs4text;

   return (int) (((double) pen.x)/64.0);
}

///////////////
#endif
#ifdef NO_FREETYPE

void pngwriter::plot_text( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, int red, int green, int blue)
{
   std::cerr << " PNGwriter::plot_text - ERROR **:  PNGwriter was compiled without Freetype support! Recompile PNGwriter with Freetype support (once you have Freetype installed, that is. Websites: www.freetype.org and pngwriter.sourceforge.net)." << std::endl;
   return;
}

void pngwriter::plot_text( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double red, double green, double blue)
{
   std::cerr << " PNGwriter::plot_text - ERROR **:  PNGwriter was compiled without Freetype support! Recompile PNGwriter with Freetype support (once you have Freetype installed, that is. Websites: www.freetype.org and pngwriter.sourceforge.net)." << std::endl;
   return;

}

void pngwriter::plot_text_utf8( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, int red, int green, int blue)
{
   std::cerr << " PNGwriter::plot_text_utf8 - ERROR **:  PNGwriter was compiled without Freetype support! Recompile PNGwriter with Freetype support (once you have Freetype installed, that is. Websites: www.freetype.org and pngwriter.sourceforge.net)." << std::endl;
   return;
}

void pngwriter::plot_text_utf8( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double red, double green, double blue)
{
   std::cerr << " PNGwriter::plot_text_utf8 - ERROR **:  PNGwriter was compiled without Freetype support! Recompile PNGwriter with Freetype support (once you have Freetype installed, that is. Websites: www.freetype.org and pngwriter.sourceforge.net)." << std::endl;
   return;
}

//////////// Get text width
int pngwriter::get_text_width(char * face_path, int fontsize, char * text)
{
   std::cerr << " PNGwriter::get_text_width - ERROR **:  PNGwriter was compiled without Freetype support! Recompile PNGwriter with Freetype support (once you have Freetype installed, that is. Websites: www.freetype.org and pngwriter.sourceforge.net)." << std::endl;
   return 0;
}


int pngwriter::get_text_width_utf8(char * face_path, int fontsize,  char * text)
{
   std::cerr << " PNGwriter::get_text_width_utf8 - ERROR **:  PNGwriter was compiled without Freetype support! Recompile PNGwriter with Freetype support (once you have Freetype installed, that is. Websites: www.freetype.org and pngwriter.sourceforge.net)." << std::endl;
   return 0;
}

///////////////
#endif

/////////////////////////////////////
int pngwriter::bilinear_interpolation_read(double x, double y, int colour)
{

   int inty, intx;
   inty = (int) ceil(y);
   intx = (int) ceil(x);

   //inty = (int) floor(y) +1;
   // intx = (int) floor(x) +1;
   //
   bool attop, atright;
   attop = inty==this->height_;
   atright =  intx==this->width_;
/*
   if( intx==this->width_ +1)
	 {
	intx--;
	//	std::cout << "intx--" << std::endl;

	 }
  */
   /*
   if(inty == this->height_ +1)
	 {
	inty--;
	//	std::cout << "inty--" << std::endl;
	 }
   */

   if( (!attop)&&(!atright) )
	 {

	double f,g,f1,g1;
	f = 1.0 + x - ((double) intx);
	g = 1.0 + y - ((double) inty);
	f1 = 1.0 - f;
	g1 = 1.0 - g;

	return (int) (
			  f1*g1*this->read(intx, inty,colour)
			  + f*g1*this->read(intx+1,inty,colour)
			  +f1*g*this->read(intx,inty+1,colour)
			  + f*g*(this->read(intx+1,inty+1,colour))
			  );
	 }

   if( (atright)&&(!attop))
	 {

	double f,g,f1,g1;
	f = 1.0 + x - ((double) intx);
	g = 1.0 + y - ((double) inty);
	f1 = 1.0 - f;
	g1 = 1.0 - g;

	return (int) (
			  f1*g1*this->read(intx, inty,colour)
			  + f*g1*( 2*(this->read(intx,inty,colour)) - (this->read(intx-1,inty,colour)) )
			  +f1*g*this->read(intx,inty+1,colour)
			  + f*g*(2*(this->read(intx,inty+1,colour)) - (this->read(intx-1,inty+1,colour)))
			  );
	 }

   if((attop)&&(!atright))
	 {
	double f,g,f1,g1;
	f = 1.0 + x - ((double) intx);
	g = 1.0 + y - ((double) inty);
	f1 = 1.0 - f;
	g1 = 1.0 - g;

	return (int) (
			  f1*g1*this->read(intx, inty,colour)
			  + f*g1*this->read(intx+1,inty,colour)
			  +f1*g*( 2*(this->read(intx,inty,colour))  - this->read(intx,inty-1,colour) )
			  + f*g*( 2*(this->read(intx+1,inty,colour))  - this->read(intx+1,inty-1,colour))
			  );
	 }

   double f,g,f1,g1;
   f = 1.0 + x - ((double) intx);
   g = 1.0 + y - ((double) inty);
   f1 = 1.0 - f;
   g1 = 1.0 - g;

   return (int) (
		 f1*g1*this->read(intx, inty,colour)
		 + f*g1*( 2*(this->read(intx,inty,colour)) - (this->read(intx-1,inty,colour)) )
		 +f1*g*( 2*(this->read(intx,inty,colour))  - this->read(intx,inty-1,colour) )
		 + f*g*( 2*( 2*(this->read(intx,inty,colour)) - (this->read(intx-1,inty,colour)) )  - ( 2*(this->read(intx,inty-1,colour)) - (this->read(intx-1,inty-1,colour)) ))
		 );

   /*
	return (int) (
	f1*g1*this->read(intx, inty,colour)
	+ f*g1*this->read(intx+1,inty,colour)
	+f1*g*this->read(intx,inty+1,colour)
	+ f*g*this->read(intx+1, inty+1,colour)
	);
	* */

};

double pngwriter::bilinear_interpolation_dread(double x, double y, int colour)
{
   return double(this->bilinear_interpolation_read(x,y,colour))/65535.0;
};

void pngwriter::plot_blend(int x, int y, double opacity, int red, int green, int blue)
{
   this->plot(x, y,
		  (int)(  opacity*red   +  this->read(x,y,1)*(1.0-opacity)),
		  (int)( opacity*green +  this->read(x,y,2)*(1.0-opacity)),
		  (int)( opacity*blue  +  this->read(x,y,3)*(1.0-opacity))
		  );
};

void pngwriter::plot_blend(int x, int y, double opacity, double red, double green, double blue)
{
   this->plot_blend(x, y, opacity, (int)  (65535*red), (int)  (65535*green),  (int)  (65535*blue));
};

void pngwriter::invert(void)
{
   //   int temp1, temp2, temp3;
   double temp11, temp22, temp33;

   for(int jjj = 1; jjj <= (this->height_); jjj++)
	 {
	for(int iii = 1; iii <= (this->width_); iii++)
	  {
		 /*	     temp11 = (this->read(iii,jjj,1));
		  temp22 = (this->read(iii,jjj,2));
		  temp33 = (this->read(iii,jjj,3));
		  *
		  this->plot(iii,jjj,
		  ((double)(65535 - temp11))/65535.0,
		  ((double)(65535 - temp22))/65535.0,
		  ((double)(65535 - temp33))/65535.0
		  );
		  *
		  */
		 temp11 = (this->read(iii,jjj,1));
		 temp22 = (this->read(iii,jjj,2));
		 temp33 = (this->read(iii,jjj,3));

		 this->plot(iii,jjj,
			(int)(65535 - temp11),
			(int)(65535 - temp22),
			(int)(65535 - temp33)
			);

	  }
	 }
}

void pngwriter::resize(int width, int height)
{

   for (int jjj = 0; jjj < height_; jjj++) free(graph_[jjj]);
   free(graph_);

   width_ = width;
   height_ = height;
   backgroundcolour_ = 0;

   graph_ = (png_bytepp)malloc(height_ * sizeof(png_bytep));
   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::resize - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   for (int kkkk = 0; kkkk < height_; kkkk++)
	 {
	graph_[kkkk] = (png_bytep)malloc(6*width_ * sizeof(png_byte));
	if(graph_[kkkk] == NULL)
	  {
		 std::cerr << " PNGwriter::resize - ERROR **:  Not able to allocate memory for image." << std::endl;
	  }
	 }

   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::resize - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   int tempindex;
   for(int hhh = 0; hhh<width_;hhh++)
	 {
	for(int vhhh = 0; vhhh<height_;vhhh++)
	  {
		 //graph_[vhhh][6*hhh + i] where i goes from 0 to 5
		 tempindex = 6*hhh;
		 graph_[vhhh][tempindex] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+1] = (char)(backgroundcolour_%256);
		 graph_[vhhh][tempindex+2] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+3] = (char)(backgroundcolour_%256);
		 graph_[vhhh][tempindex+4] = (char) floor(((double)backgroundcolour_)/256);
		 graph_[vhhh][tempindex+5] = (char)(backgroundcolour_%256);
	  }
	 }
}

void pngwriter::boundary_fill(int xstart, int ystart, double boundary_red,double boundary_green,double boundary_blue,double fill_red, double fill_green, double fill_blue)
{
   if( (
	(this->dread(xstart,ystart,1) != boundary_red) ||
	(this->dread(xstart,ystart,2) != boundary_green) ||
	(this->dread(xstart,ystart,3) != boundary_blue)
	)
	   &&
	   (
	(this->dread(xstart,ystart,1) != fill_red) ||
	(this->dread(xstart,ystart,2) != fill_green) ||
	(this->dread(xstart,ystart,3) != fill_blue)
	)
	   &&
	   (xstart >0)&&(xstart <= width_)&&(ystart >0)&&(ystart <= height_)
	   )
	 {
	this->plot(xstart, ystart, fill_red, fill_green, fill_blue);
	boundary_fill(xstart+1,  ystart,  boundary_red, boundary_green, boundary_blue, fill_red,  fill_green,  fill_blue) ;
	boundary_fill(xstart,  ystart+1,  boundary_red, boundary_green, boundary_blue, fill_red,  fill_green,  fill_blue) ;
	boundary_fill(xstart,  ystart-1,  boundary_red, boundary_green, boundary_blue, fill_red,  fill_green,  fill_blue) ;
	boundary_fill(xstart-1,  ystart,  boundary_red, boundary_green, boundary_blue, fill_red,  fill_green,  fill_blue) ;
	 }
}

//no int version needed
void pngwriter::flood_fill_internal(int xstart, int ystart,  double start_red, double start_green, double start_blue, double fill_red, double fill_green, double fill_blue)
{
   if( (
	(this->dread(xstart,ystart,1) == start_red) &&
	(this->dread(xstart,ystart,2) == start_green) &&
	(this->dread(xstart,ystart,3) == start_blue)
	)
	   &&
	   (
	(this->dread(xstart,ystart,1) != fill_red) ||
	(this->dread(xstart,ystart,2) != fill_green) ||
	(this->dread(xstart,ystart,3) != fill_blue)
	)
	   &&
	   (xstart >0)&&(xstart <= width_)&&(ystart >0)&&(ystart <= height_)
	   )
	 {
	this->plot(xstart, ystart, fill_red, fill_green, fill_blue);
	flood_fill_internal(  xstart+1,  ystart,   start_red,  start_green,  start_blue,  fill_red,  fill_green,  fill_blue);
	flood_fill_internal(  xstart-1,  ystart,   start_red,  start_green,  start_blue,  fill_red,  fill_green,  fill_blue);
	flood_fill_internal(  xstart,  ystart+1,   start_red,  start_green,  start_blue,  fill_red,  fill_green,  fill_blue);
	flood_fill_internal(  xstart,  ystart-1,   start_red,  start_green,  start_blue,  fill_red,  fill_green,  fill_blue);
	 }

}

//int version
void pngwriter::boundary_fill(int xstart, int ystart, int boundary_red,int boundary_green,int boundary_blue,int fill_red, int fill_green, int fill_blue)
{

   this->boundary_fill( xstart, ystart,
			((double) boundary_red)/65535.0,
			((double) boundary_green)/65535.0,
			((double) boundary_blue)/65535.0,
			((double) fill_red)/65535.0,
			((double) fill_green)/65535.0,
			((double) fill_blue)/65535.0
			);
}

void pngwriter::flood_fill(int xstart, int ystart, double fill_red, double fill_green, double fill_blue)
{
   flood_fill_internal(  xstart,  ystart,  this->dread(xstart,ystart,1),this->dread(xstart,ystart,2),this->dread(xstart,ystart,3),   fill_red,  fill_green, fill_blue);
}

//int version
void pngwriter::flood_fill(int xstart, int ystart, int fill_red, int fill_green, int fill_blue)
{
   this->flood_fill( xstart,  ystart,
			 ((double)  fill_red)/65535.0,
			 ((double) fill_green)/65535.0,
			 ((double)  fill_blue)/65535.0
			 );
}

void pngwriter::polygon( int * points, int number_of_points, double red, double green, double blue)
{
   if( (number_of_points<1)||(points ==NULL))
	 {
	std::cerr << " PNGwriter::polygon - ERROR **:  Number of points is zero or negative, or array is NULL." << std::endl;
	return;
	 }

   for(int k=0;k< number_of_points-1; k++)
	 {
	this->line(points[2*k],points[2*k+1],points[2*k+2],points[2*k+3], red, green, blue);
	 }
}

//int version
void pngwriter::polygon( int * points, int number_of_points, int red, int green, int blue)
{
   this->polygon(points, number_of_points,
		 ((double) red)/65535.0,
		 ((double) green)/65535.0,
		 ((double) blue)/65535.0
		 );
}

void pngwriter::plotCMYK(int x, int y, double cyan, double magenta, double yellow, double black)
{
/*CMYK to RGB:
 *  -----------
 *  red   = 255 - minimum(255,((cyan/255)    * (255 - black) + black))
 *  green = 255 - minimum(255,((magenta/255) * (255 - black) + black))
 *  blue  = 255 - minimum(255,((yellow/255)  * (255 - black) + black))
 * */

   if(cyan<0.0)
	 {
	cyan = 0.0;
	 }
   if(magenta<0.0)
	 {
	magenta = 0.0;
	 }
   if(yellow<0.0)
	 {
	yellow = 0.0;
	 }
   if(black<0.0)
	 {
	black = 0.0;
	 }

   if(cyan>1.0)
	 {
	cyan = 1.0;
	 }
   if(magenta>1.0)
	 {
	magenta = 1.0;
	 }
   if(yellow>1.0)
	 {
	yellow = 1.0;
	 }
   if(black>1.0)
	 {
	black = 1.0;
	 }

   double  red, green, blue, minr, ming, minb, iblack;

   iblack = 1.0 - black;

   minr = 1.0;
   ming = 1.0;
   minb = 1.0;

   if( (cyan*iblack + black)<1.0 )
	 {
	minr = cyan*iblack + black;
	 }

   if( (magenta*iblack + black)<1.0 )
	 {
	ming = magenta*iblack + black;
	 }

   if( (yellow*iblack + black)<1.0 )
	 {
	minb = yellow*iblack + black;
	 }

   red = 1.0 - minr;
   green = 1.0 - ming;
   blue = 1.0 - minb;

   this->plot(x,y,red, green, blue);

}

//int version
void pngwriter::plotCMYK(int x, int y, int cyan, int magenta, int yellow, int black)
{
   this->plotCMYK( x, y,
		   ((double) cyan)/65535.0,
		   ((double) magenta)/65535.0,
		   ((double) yellow)/65535.0,
		   ((double) black)/65535.0
		   );
}

double pngwriter::dreadCMYK(int x, int y, int colour)
{
/*
 * Black   = minimum(1-Red,1-Green,1-Blue)
 *     Cyan    = (1-Red-Black)/(1-Black)
 *     Magenta = (1-Green-Black)/(1-Black)
 *     Yellow  = (1-Blue-Black)/(1-Black)
 *
 * */
   if((colour !=1)&&(colour !=2)&&(colour !=3)&&(colour !=4))
	 {
	std::cerr << " PNGwriter::dreadCMYK - WARNING **: Invalid argument: should be 1, 2, 3 or 4, is " << colour << std::endl;
	return 0;
	 }

   double black, red, green, blue, ired, igreen, iblue, iblack;
   //add error detection here
   // not much to detect, really
   red = this->dread(x, y, 1);
   green = this->dread(x, y, 2);
   blue = this->dread(x, y, 3);

   ired = 1.0 - red;
   igreen = 1.0 - green;
   iblue = 1.0 - blue;

   black = ired;

   //black is the mimimum of inverse RGB colours, and if they are all equal, it is the inverse of red.
   if( (igreen<ired)&&(igreen<iblue) )
	 {
	black = igreen;
	 }

   if( (iblue<igreen)&&(iblue<ired) )
	 {
	black = iblue;
	 }

   iblack = 1.0 - black;

   if(colour == 1)
	 {
	return ((ired-black)/(iblack));
	 }

   if(colour == 2)
	 {
	return ((igreen-black)/(iblack));
	 }

   if(colour == 3)
	 {
	return ((iblue-black)/(iblack));
	 }

   if(colour == 4)
	 {
	return black;
	 }

   return 0.0;
}

int pngwriter::readCMYK(int x, int y, int colour)
{
/*
 * Black   = minimum(1-Red,1-Green,1-Blue)
 *     Cyan    = (1-Red-Black)/(1-Black)
 *     Magenta = (1-Green-Black)/(1-Black)
 *     Yellow  = (1-Blue-Black)/(1-Black)
 *
 * */
   if((colour !=1)&&(colour !=2)&&(colour !=3)&&(colour !=4))
	 {
	std::cerr << " PNGwriter::readCMYK - WARNING **: Invalid argument: should be 1, 2, 3 or 4, is " << colour << std::endl;
	return 0;
	 }

   double black, red, green, blue, ired, igreen, iblue, iblack;
   //add error detection here
   // not much to detect, really
   red = this->dread(x, y, 1);
   green = this->dread(x, y, 2);
   blue = this->dread(x, y, 3);

   ired = 1.0 - red;
   igreen = 1.0 - green;
   iblue = 1.0 - blue;

   black = ired;

   //black is the mimimum of inverse RGB colours, and if they are all equal, it is the inverse of red.
   if( (igreen<ired)&&(igreen<iblue) )
	 {
	black = igreen;
	 }

   if( (iblue<igreen)&&(iblue<ired) )
	 {
	black = iblue;
	 }

   iblack = 1.0 - black;

   if(colour == 1)
	 {
	return (int)( ((ired-black)/(iblack))*65535);
	 }

   if(colour == 2)
	 {
	return (int)( ((igreen-black)/(iblack))*65535);
	 }

   if(colour == 3)
	 {
	return (int)( ((iblue-black)/(iblack))*65535);
	 }

   if(colour == 4)
	 {
	return (int)( (black)*65535);
	 }

   return 0;

}

void pngwriter::scale_k(double k)
{
   if(k <= 0.0)
	 {
	std::cerr << " PNGwriter::scale_k - ERROR **:  scale_k() called with negative or zero scale factor. Was: " << k << "." << std::endl;
	 }

   // Calculate the new scaled height and width
   int scaledh, scaledw;
   scaledw = (int) ceil(k*width_);
   scaledh = (int) ceil(k*height_);

   // Create image storage.
   pngwriter temp(scaledw,scaledh,0,"temp");

   int red, green, blue;

   double spacingx = ((double)width_)/(2*scaledw);
   double spacingy = ((double)height_)/(2*scaledh);

   double readx, ready;

   for(int x = 1; x<= scaledw; x++)
	 {
	for(int y = 1; y <= scaledh; y++)
	  {
		 readx = (2*x-1)*spacingx;
		 ready = (2*y-1)*spacingy;
		 red = this->bilinear_interpolation_read(readx, ready, 1);
		 green = this->bilinear_interpolation_read(readx, ready, 2);
		 blue = this->bilinear_interpolation_read(readx, ready, 3);
		 temp.plot(x, y, red, green, blue);

	  }
	 }

   // From here on, the process is the same for all scale functions.
   //Get data out of temp and into this's storage.

   //Resize this instance
   // Delete current storage.
   for (int jjj = 0; jjj < height_; jjj++) free(graph_[jjj]);
   free(graph_);

   //New image will have bit depth 16, regardless of original bit depth.
   bit_depth_ = 16;

   // New width and height will be the scaled width and height
   width_ = scaledw;
   height_ = scaledh;
   backgroundcolour_ = 0;

   graph_ = (png_bytepp)malloc(height_ * sizeof(png_bytep));
   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::scale_k - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   for (int kkkk = 0; kkkk < height_; kkkk++)
	 {
	graph_[kkkk] = (png_bytep)malloc(6*width_ * sizeof(png_byte));
	if(graph_[kkkk] == NULL)
	  {
		 std::cerr << " PNGwriter::scale_k - ERROR **:  Not able to allocate memory for image." << std::endl;
	  }
	 }

   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::scale_k - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   //This instance now has a new, resized storage space.

   //Copy the temp date into this's storage.
   int tempindex;
   for(int hhh = 0; hhh<width_;hhh++)
	 {
	for(int vhhh = 0; vhhh<height_;vhhh++)
	  {
		 tempindex=6*hhh;
		 graph_[vhhh][tempindex] = temp.graph_[vhhh][tempindex];
		 graph_[vhhh][tempindex+1] = temp.graph_[vhhh][tempindex+1];
		 graph_[vhhh][tempindex+2] = temp.graph_[vhhh][tempindex+2];
		 graph_[vhhh][tempindex+3] = temp.graph_[vhhh][tempindex+3];
		 graph_[vhhh][tempindex+4] = temp.graph_[vhhh][tempindex+4];
		 graph_[vhhh][tempindex+5] = temp.graph_[vhhh][tempindex+5];
	  }
	 }

   // this should now contain the new, scaled image data.
   //
}

void pngwriter::scale_kxky(double kx, double ky)
{
   if((kx <= 0.0)||(ky <= 0.0))
	 {
	std::cerr << " PNGwriter::scale_kxky - ERROR **:  scale_kxky() called with negative or zero scale factor. Was: " << kx << ", " << ky << "." << std::endl;
	 }

   int scaledh, scaledw;
   scaledw = (int) ceil(kx*width_);
   scaledh = (int) ceil(ky*height_);

   pngwriter temp(scaledw, scaledh, 0, "temp");

   int red, green, blue;

   double spacingx = ((double)width_)/(2*scaledw);
   double spacingy = ((double)height_)/(2*scaledh);

   double readx, ready;

   for(int x = 1; x<= scaledw; x++)
	 {
	for(int y = 1; y <= scaledh; y++)
	  {
		 readx = (2*x-1)*spacingx;
		 ready = (2*y-1)*spacingy;
		 red = this->bilinear_interpolation_read(readx, ready, 1);
		 green = this->bilinear_interpolation_read(readx, ready, 2);
		 blue = this->bilinear_interpolation_read(readx, ready, 3);
		 temp.plot(x, y, red, green, blue);

	  }
	 }
   // From here on, the process is the same for all scale functions.
   //Get data out of temp and into this's storage.

   //Resize this instance
   // Delete current storage.
   for (int jjj = 0; jjj < height_; jjj++) free(graph_[jjj]);
   free(graph_);

   //New image will have bit depth 16, regardless of original bit depth.
   bit_depth_ = 16;

   // New width and height will be the scaled width and height
   width_ = scaledw;
   height_ = scaledh;
   backgroundcolour_ = 0;

   graph_ = (png_bytepp)malloc(height_ * sizeof(png_bytep));
   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::scale_kxky - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   for (int kkkk = 0; kkkk < height_; kkkk++)
	 {
	graph_[kkkk] = (png_bytep)malloc(6*width_ * sizeof(png_byte));
	if(graph_[kkkk] == NULL)
	  {
		 std::cerr << " PNGwriter::scale_kxky - ERROR **:  Not able to allocate memory for image." << std::endl;
	  }
	 }

   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::scale_kxky - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   //This instance now has a new, resized storage space.

   //Copy the temp date into this's storage.
   int tempindex;
   for(int hhh = 0; hhh<width_;hhh++)
	 {
	for(int vhhh = 0; vhhh<height_;vhhh++)
	  {
		 tempindex=6*hhh;
		 graph_[vhhh][tempindex] = temp.graph_[vhhh][tempindex];
		 graph_[vhhh][tempindex+1] = temp.graph_[vhhh][tempindex+1];
		 graph_[vhhh][tempindex+2] = temp.graph_[vhhh][tempindex+2];
		 graph_[vhhh][tempindex+3] = temp.graph_[vhhh][tempindex+3];
		 graph_[vhhh][tempindex+4] = temp.graph_[vhhh][tempindex+4];
		 graph_[vhhh][tempindex+5] = temp.graph_[vhhh][tempindex+5];
	  }
	 }

   // this should now contain the new, scaled image data.
   //
   //
}

void pngwriter::scale_wh(int finalwidth, int finalheight)
{
   if((finalwidth <= 0)||(finalheight <= 0))
	 {
	std::cerr << " PNGwriter::scale_wh - ERROR **: Negative or zero final width or height not allowed." << std::endl;
	 }

//   double kx;
//   double ky;

//   kx = ((double)finalwidth)/((double)width_);
//   ky = ((double)finalheight)/((double)height_);

   pngwriter temp(finalwidth, finalheight, 0, "temp");

   int red, green, blue;

   double spacingx = ((double)width_)/(2*finalwidth);
   double spacingy = ((double)height_)/(2*finalheight);

   double readx, ready;

   for(int x = 1; x<= finalwidth; x++)
	 {
	for(int y = 1; y <= finalheight; y++)
	  {
		 readx = (2*x-1)*spacingx;
		 ready = (2*y-1)*spacingy;
		 red = this->bilinear_interpolation_read(readx, ready, 1);
		 green = this->bilinear_interpolation_read(readx, ready, 2);
		 blue = this->bilinear_interpolation_read(readx, ready, 3);
		 temp.plot(x, y, red, green, blue);

	  }
	 }

   // From here on, the process is the same for all scale functions.
   //Get data out of temp and into this's storage.

   //Resize this instance
   // Delete current storage.
   for (int jjj = 0; jjj < height_; jjj++) free(graph_[jjj]);
   free(graph_);

   //New image will have bit depth 16, regardless of original bit depth.
   bit_depth_ = 16;

   // New width and height will be the scaled width and height
   width_ = finalwidth;
   height_ = finalheight;
   backgroundcolour_ = 0;

   graph_ = (png_bytepp)malloc(height_ * sizeof(png_bytep));
   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::scale_wh - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   for (int kkkk = 0; kkkk < height_; kkkk++)
	 {
	graph_[kkkk] = (png_bytep)malloc(6*width_ * sizeof(png_byte));
	if(graph_[kkkk] == NULL)
	  {
		 std::cerr << " PNGwriter::scale_wh - ERROR **:  Not able to allocate memory for image." << std::endl;
	  }
	 }

   if(graph_ == NULL)
	 {
	std::cerr << " PNGwriter::scale_wh - ERROR **:  Not able to allocate memory for image." << std::endl;
	 }

   //This instance now has a new, resized storage space.

   //Copy the temp date into this's storage.
   int tempindex;
   for(int hhh = 0; hhh<width_;hhh++)
	 {
	for(int vhhh = 0; vhhh<height_;vhhh++)
	  {
		 tempindex=6*hhh;
		 graph_[vhhh][tempindex] = temp.graph_[vhhh][tempindex];
		 graph_[vhhh][tempindex+1] = temp.graph_[vhhh][tempindex+1];
		 graph_[vhhh][tempindex+2] = temp.graph_[vhhh][tempindex+2];
		 graph_[vhhh][tempindex+3] = temp.graph_[vhhh][tempindex+3];
		 graph_[vhhh][tempindex+4] = temp.graph_[vhhh][tempindex+4];
		 graph_[vhhh][tempindex+5] = temp.graph_[vhhh][tempindex+5];
	  }
	 }

   // this should now contain the new, scaled image data.
   //
   //
}

// Blended functions
//
void pngwriter::plotHSV_blend(int x, int y, double opacity, double hue, double saturation, double value)
{
   double red,green,blue;
   double *redp;
   double *greenp;
   double *bluep;

   redp = &red;
   greenp = &green;
   bluep = &blue;

   HSVtoRGB(redp,greenp,bluep,hue,saturation,value);
   plot_blend(x,y,opacity, red,green,blue);

}

void pngwriter::plotHSV_blend(int x, int y, double opacity, int hue, int saturation, int value)
{
   plotHSV_blend(x, y, opacity, double(hue)/65535.0, double(saturation)/65535.0,  double(value)/65535.0);
}

void pngwriter::line_blend(int xfrom, int yfrom, int xto, int yto,  double opacity, int red, int green,int  blue)
{
   //  Bresenham Algorithm.
   //
   int dy = yto - yfrom;
   int dx = xto - xfrom;
   int stepx, stepy;

   if (dy < 0)
	 {
	dy = -dy;  stepy = -1;
	 }
   else
	 {
	stepy = 1;
	 }

   if (dx < 0)
	 {
	dx = -dx;  stepx = -1;
	 }
   else
	 {
	stepx = 1;
	 }
   dy <<= 1;     // dy is now 2*dy
   dx <<= 1;     // dx is now 2*dx

   this->plot_blend(xfrom,yfrom,opacity, red,green,blue);

   if (dx > dy)
	 {
	int fraction = dy - (dx >> 1);

	while (xfrom != xto)
	  {
		 if (fraction >= 0)
		   {
		  yfrom += stepy;
		  fraction -= dx;
		   }
		 xfrom += stepx;
		 fraction += dy;
		 this->plot_blend(xfrom,yfrom,opacity, red,green,blue);
	  }
	 }
   else
	 {
	int fraction = dx - (dy >> 1);
	while (yfrom != yto)
	  {
		 if (fraction >= 0)
		   {
		  xfrom += stepx;
		  fraction -= dy;
		   }
		 yfrom += stepy;
		 fraction += dx;
		 this->plot_blend(xfrom,yfrom, opacity, red,green,blue);
	  }
	 }

}

void pngwriter::line_blend(int xfrom, int yfrom, int xto, int yto, double opacity, double red, double green,double  blue)
{
   this->line_blend( xfrom,
			 yfrom,
			 xto,
			 yto,
			 opacity,
			 int (red*65535),
			 int (green*65535),
			 int (blue*65535)
			 );

}

void pngwriter::square_blend(int xfrom, int yfrom, int xto, int yto, double opacity, int red, int green,int  blue)
{
   this->line_blend(xfrom, yfrom, xfrom, yto, opacity, red, green, blue);
   this->line_blend(xto, yfrom, xto, yto, opacity, red, green, blue);
   this->line_blend(xfrom, yfrom, xto, yfrom, opacity, red, green, blue);
   this->line_blend(xfrom, yto, xto, yto, opacity,  red, green, blue);
}

void pngwriter::square_blend(int xfrom, int yfrom, int xto, int yto, double opacity, double red, double green,double  blue)
{
   this->square_blend( xfrom,  yfrom,  xto,  yto, opacity, int(red*65535), int(green*65535), int(blue*65535));
}

void pngwriter::filledsquare_blend(int xfrom, int yfrom, int xto, int yto, double opacity, int red, int green,int  blue)
{
   for(int caca = xfrom; caca <xto+1; caca++)
	 {
	this->line_blend(caca, yfrom, caca, yto, opacity, red, green, blue);
	 }

}

void pngwriter::filledsquare_blend(int xfrom, int yfrom, int xto, int yto, double opacity, double red, double green,double  blue)
{
   this->filledsquare_blend( xfrom,  yfrom,  xto,  yto, opacity, int(red*65535), int(green*65535), int(blue*65535));
}

void pngwriter::circle_aux_blend(int xcentre, int ycentre, int x, int y, double opacity, int red, int green, int blue)
{
   if (x == 0)
	 {
	this->plot_blend( xcentre, ycentre + y, opacity, red, green, blue);
	this->plot_blend( xcentre, ycentre - y, opacity, red, green, blue);
	this->plot_blend( xcentre + y, ycentre, opacity, red, green, blue);
	this->plot_blend( xcentre - y, ycentre, opacity, red, green, blue);
	 }
   else
	 if (x == y)
	   {
	  this->plot_blend( xcentre + x, ycentre + y, opacity, red, green, blue);
	  this->plot_blend( xcentre - x, ycentre + y, opacity, red, green, blue);
	  this->plot_blend( xcentre + x, ycentre - y, opacity, red, green, blue);
	  this->plot_blend( xcentre - x, ycentre - y, opacity, red, green, blue);
	   }
   else
	 if (x < y)
	   {
	  this->plot_blend( xcentre + x, ycentre + y, opacity, red, green, blue);
	  this->plot_blend( xcentre - x, ycentre + y, opacity, red, green, blue);
	  this->plot_blend( xcentre + x, ycentre - y, opacity, red, green, blue);
	  this->plot_blend( xcentre - x, ycentre - y, opacity, red, green, blue);
	  this->plot_blend( xcentre + y, ycentre + x, opacity, red, green, blue);
	  this->plot_blend( xcentre - y, ycentre + x, opacity, red, green, blue);
	  this->plot_blend( xcentre + y, ycentre - x, opacity, red, green, blue);
	  this->plot_blend( xcentre - y, ycentre - x, opacity, red, green, blue);
	   }

}
//

void pngwriter::circle_blend(int xcentre, int ycentre, int radius, double opacity, int red, int green, int blue)
{
   int x = 0;
   int y = radius;
   int p = (5 - radius*4)/4;

   circle_aux_blend(xcentre, ycentre, x, y, opacity, red, green, blue);
   while (x < y)
	 {
	x++;
	if (p < 0)
	  {
		 p += 2*x+1;
	  }
	else
	  {
		 y--;
		 p += 2*(x-y)+1;
	  }
	circle_aux_blend(xcentre, ycentre, x, y, opacity, red, green, blue);
	 }

}

void pngwriter::circle_blend(int xcentre, int ycentre, int radius, double opacity, double red, double green, double blue)
{
   this->circle_blend(xcentre,ycentre,radius, opacity,  int(red*65535), int(green*65535), int(blue*65535));
}

void pngwriter::filledcircle_blend(int xcentre, int ycentre, int radius, double opacity, int red, int green, int blue)
{
   for(int jjj = ycentre-radius; jjj< ycentre+radius+1; jjj++)
	 {
	this->line_blend(xcentre - int(sqrt((double)(radius*radius) - (-ycentre + jjj)*(-ycentre + jjj ))), jjj,
			 xcentre + int(sqrt((double)(radius*radius) - (-ycentre + jjj)*(-ycentre + jjj ))),jjj, opacity, red,green,blue);
	 }

}

void pngwriter::filledcircle_blend(int xcentre, int ycentre, int radius, double opacity, double red, double green, double blue)
{
   this->filledcircle_blend( xcentre, ycentre,  radius, opacity, int(red*65535), int(green*65535), int(blue*65535));
}

void pngwriter::bezier_blend(  int startPtX, int startPtY,
				   int startControlX, int startControlY,
				   int endPtX, int endPtY,
				   int endControlX, int endControlY,
				   double opacity,
				   double red, double green, double blue)
{

   double cx = 3.0*(startControlX - startPtX);
   double bx = 3.0*(endControlX - startControlX) - cx;
   double ax = double(endPtX - startPtX - cx - bx);

   double cy = 3.0*(startControlY - startPtY);
   double by = 3.0*(endControlY - startControlY) - cy;
   double ay = double(endPtY - startPtY - cy - by);

   double x,y,newx,newy;
   x = startPtX;
   y = startPtY;

   for(double t = 0.0; t<=1.005; t += 0.005)
	 {
	newx = startPtX + t*(double(cx) + t*(double(bx) + t*(double(ax))));
	newy = startPtY + t*(double(cy) + t*(double(by) + t*(double(ay))));
	this->line_blend(int(x),int(y),int(newx),int(newy),opacity, red,green,blue);
	x = newx;
	y = newy;
	 }
}

void pngwriter::bezier_blend(  int startPtX, int startPtY,
				   int startControlX, int startControlY,
				   int endPtX, int endPtY,
				   int endControlX, int endControlY,
				   double opacity,
				   int red, int green, int blue)
{
   this->bezier_blend(   startPtX,  startPtY,
			 startControlX, startControlY,
			 endPtX, endPtY,
			 endControlX,  endControlY,
			 opacity,
			 double(red)/65535.0,  double(green)/65535.0,  double(blue)/65535.0);

}

/////////////////////////////
#ifndef NO_FREETYPE

// Freetype-based text rendering functions.
///////////////////////////////////////////
void pngwriter::plot_text_blend( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double opacity, double red, double green, double blue)
{
   FT_Library  library;
   FT_Face     face;
   FT_Matrix   matrix;      // transformation matrix
   FT_Vector   pen;

   FT_UInt glyph_index;
   FT_Error error;

   FT_Bool use_kerning;
   FT_UInt previous = 0;

   /* Set up transformation Matrix */
   matrix.xx = (FT_Fixed)( cos(angle)*0x10000);   /* It would make more sense to do this (below), but, bizzarely, */
   matrix.xy = (FT_Fixed)(-sin(angle)*0x10000);   /* if one does, FT_Load_Glyph fails consistently.               */
   matrix.yx = (FT_Fixed)( sin(angle)*0x10000);  //   matrix.yx = - matrix.xy;
   matrix.yy = (FT_Fixed)( cos(angle)*0x10000);  //   matrix.yy = matrix.xx;

   /* Place starting coordinates in adequate form. */
   pen.x = x_start*64 ;
   pen.y =   (int)(y_start/64.0);

   /*Count the length of the string */
   int num_chars = strlen(text);

   /* Initialize FT Library object */
   error = FT_Init_FreeType( &library );
   if (error) { std::cerr << " PNGwriter::plot_text_blend - ERROR **: FreeType: Could not init Library."<< std::endl; return;}

   /* Initialize FT face object */
   error = FT_New_Face( library,face_path,0,&face );
   if ( error == FT_Err_Unknown_File_Format ) { std::cerr << " PNGwriter::plot_text_blend - ERROR **: FreeType: Font was opened, but type not supported."<< std::endl; return; } else if (error){ std::cerr << " PNGwriter::plot_text - ERROR **: FreeType: Could not find or load font file."<< std::endl; return; }

   /* Set the Char size */
   error = FT_Set_Char_Size( face,          /* handle to face object           */
				 0,             /* char_width in 1/64th of points  */
				 fontsize*64,   /* char_height in 1/64th of points */
				 100,           /* horizontal device resolution    */
				 100 );         /* vertical device resolution      */

   /* A way of accesing the glyph directly */
   FT_GlyphSlot  slot = face->glyph;  // a small shortcut

   /* Does the font file support kerning? */
   use_kerning = FT_HAS_KERNING( face );

   int n;
   for ( n = 0; n < num_chars; n++ )
	 {
	/* Convert character code to glyph index */
	glyph_index = FT_Get_Char_Index( face, text[n] );

	/* Retrieve kerning distance and move pen position */
	if ( use_kerning && previous&& glyph_index )
	  {
		 FT_Vector  delta;
		 FT_Get_Kerning( face,
				 previous,
				 glyph_index,
				 ft_kerning_default, //FT_KERNING_DEFAULT,
				 &delta );

		 /* Transform this kerning distance into rotated space */
		 pen.x += (int) (((double) delta.x)*cos(angle));
		 pen.y +=  (int) (((double) delta.x)*( sin(angle)));
	  }

	/* Set transform */
	FT_Set_Transform( face, &matrix, &pen );

/*set char size*/

	if (error) { std::cerr << " PNGwriter::plot_text_blend - ERROR **: FreeType: Set char size error." << std::endl; return;};

	/* Retrieve glyph index from character code */
	glyph_index = FT_Get_Char_Index( face, text[n] );

	/* Load glyph image into the slot (erase previous one) */
	error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
	if (error) { std::cerr << " PNGwriter::plot_text_blend - ERROR **: FreeType: Could not load glyph (in loop). (FreeType error "<< std::hex << error <<")." << std::endl; return;}

	/* Convert to an anti-aliased bitmap */
	//	error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );
	error = FT_Render_Glyph( face->glyph, ft_render_mode_normal );
	if (error) { std::cerr << " PNGwriter::plot_text_blend - ERROR **: FreeType: Render glyph error." << std::endl; return;}

	/* Now, draw to our target surface */
	my_draw_bitmap_blend( &slot->bitmap,
				  slot->bitmap_left,
				  y_start + slot->bitmap_top,
				  opacity,
				  red,
				  green,
				  blue );

	/* Advance to the next position */
	pen.x += slot->advance.x;
	pen.y += slot->advance.y;

	/* record current glyph index */
	previous = glyph_index;
	 }

   /* Free the face and the library objects */
   FT_Done_Face    ( face );
   FT_Done_FreeType( library );
}

void pngwriter::plot_text_utf8_blend( char * face_path, int fontsize, int x_start, int y_start, double angle,  char * text, double opacity, double red, double green, double blue)
{
   FT_Library  library;
   FT_Face     face;
   FT_Matrix   matrix;      // transformation matrix
   FT_Vector   pen;

   FT_UInt glyph_index;
   FT_Error error;

   FT_Bool use_kerning;
   FT_UInt previous = 0;

   /* Set up transformation Matrix */
   matrix.xx = (FT_Fixed)( cos(angle)*0x10000);   /* It would make more sense to do this (below), but, bizzarely, */
   matrix.xy = (FT_Fixed)(-sin(angle)*0x10000);   /* if one does, FT_Load_Glyph fails consistently.               */
   matrix.yx = (FT_Fixed)( sin(angle)*0x10000);  //   matrix.yx = - matrix.xy;
   matrix.yy = (FT_Fixed)( cos(angle)*0x10000);  //   matrix.yy = matrix.xx;

   /* Place starting coordinates in adequate form. */
   pen.x = x_start*64 ;
   pen.y = (int)(y_start/64.0);

   /*Count the length of the string */
   int num_bytes=0;
   while(text[num_bytes]!=0)
	 {
	num_bytes++;
	 }

	 /*
   std::cout << "Num bytes is: "<< num_bytes << std::endl;
   */

   //The array of ucs4 glyph indexes, which will by at most the number of bytes in the utf-8 file.
   long * ucs4text;
   ucs4text = new long[num_bytes+1];

   unsigned char u,v,w,x,y,z;

   int num_chars=0;

   long iii=0;

   while(iii<num_bytes)
	 {
	z = text[iii];

	if(z<=127)
	  {
		 ucs4text[num_chars] = z;
	  }

	if((192<=z)&&(z<=223))
	  {
		 iii++; y = text[iii];
		 ucs4text[num_chars] = (z-192)*64 + (y -128);
	  }

	if((224<=z)&&(z<=239))
	  {
		 iii++; y = text[iii];
		 iii++; x = text[iii];
		 ucs4text[num_chars] = (z-224)*4096 + (y -128)*64 + (x-128);
	  }

	if((240<=z)&&(z<=247))
	  {
		 iii++; y = text[iii];
		 iii++; x = text[iii];
		 iii++; w = text[iii];
		 ucs4text[num_chars] = (z-240)*262144 + (y -128)*4096 + (x-128)*64 + (w-128);
	  }

	if((248<=z)&&(z<=251))
	  {
		 iii++; y = text[iii];
		 iii++; x = text[iii];
		 iii++; w = text[iii];
		 iii++; v = text[iii];
		 ucs4text[num_chars] = (z-248)*16777216 + (y -128)*262144 + (x-128)*4096 + (w-128)*64 +(v-128);
	  }

	if((252==z)||(z==253))
	  {
		 iii++; y = text[iii];
		 iii++; x = text[iii];
		 iii++; w = text[iii];
		 iii++; v = text[iii];
		 u = text[iii];
		 ucs4text[num_chars] = (z-252)*1073741824 + (y -128)*16777216   + (x-128)*262144 + (w-128)*4096 +(v-128)*64 + (u-128);
	  }

	if((z==254)||(z==255))
	  {
		 std::cerr << " PNGwriter::plot_text_utf8_blend - ERROR **: Problem with character: invalid UTF-8 data."<< std::endl;
	  }
	// std::cerr << "\nProblem at " << iii << ".\n";
	//
	iii++;
	num_chars++;
	 }

   // num_chars now contains the number of characters in the string.
   /*
   std::cout << "Num chars is: "<< num_chars << std::endl;
   */

   /* Initialize FT Library object */
   error = FT_Init_FreeType( &library );
   if (error) { std::cerr << " PNGwriter::plot_text_utf8_blend - ERROR **: FreeType: Could not init Library."<< std::endl; return;}

   /* Initialize FT face object */
   error = FT_New_Face( library,face_path,0,&face );
   if ( error == FT_Err_Unknown_File_Format ) { std::cerr << " PNGwriter::plot_text_utf8_blend - ERROR **: FreeType: Font was opened, but type not supported."<< std::endl; return; } else if (error){ std::cerr << " PNGwriter::plot_text - ERROR **: FreeType: Could not find or load font file."<< std::endl; return; }

   /* Set the Char size */
   error = FT_Set_Char_Size( face,          /* handle to face object           */
				 0,             /* char_width in 1/64th of points  */
				 fontsize*64,   /* char_height in 1/64th of points */
				 100,           /* horizontal device resolution    */
				 100 );         /* vertical device resolution      */

   /* A way of accesing the glyph directly */
   FT_GlyphSlot  slot = face->glyph;  // a small shortcut

   /* Does the font file support kerning? */
   use_kerning = FT_HAS_KERNING( face );

   int n;
   for ( n = 0; n < num_chars; n++ )
	 {
	/* Convert character code to glyph index */
	glyph_index = FT_Get_Char_Index( face, ucs4text[n] );

	/* Retrieve kerning distance and move pen position */
	if ( use_kerning && previous&& glyph_index )
	  {
		 FT_Vector  delta;
		 FT_Get_Kerning( face,
				 previous,
				 glyph_index,
				 ft_kerning_default, //FT_KERNING_DEFAULT,
				 &delta );

		 /* Transform this kerning distance into rotated space */
		 pen.x += (int) (((double) delta.x)*cos(angle));
		 pen.y +=  (int) (((double) delta.x)*( sin(angle)));
	  }

	/* Set transform */
	FT_Set_Transform( face, &matrix, &pen );

/*set char size*/

	if (error) { std::cerr << " PNGwriter::plot_text_utf8_blend - ERROR **: FreeType: Set char size error." << std::endl; return;};

	/* Retrieve glyph index from character code */
	glyph_index = FT_Get_Char_Index( face, ucs4text[n] );

	/* Load glyph image into the slot (erase previous one) */
	error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
	if (error) { std::cerr << " PNGwriter::plot_text_utf8_blend - ERROR **: FreeType: Could not load glyph (in loop). (FreeType error "<< std::hex << error <<")." << std::endl; return;}

	/* Convert to an anti-aliased bitmap */
	error = FT_Render_Glyph( face->glyph, ft_render_mode_normal );
	if (error) { std::cerr << " PNGwriter::plot_text_utf8_blend - ERROR **: FreeType: Render glyph error." << std::endl; return;}

	/* Now, draw to our target surface */
	my_draw_bitmap_blend( &slot->bitmap,
				  slot->bitmap_left,
				  y_start + slot->bitmap_top,
				  opacity,
				  red,
				  green,
				  blue );

	/* Advance to the next position */
	pen.x += slot->advance.x;
	pen.y += slot->advance.y;

	/* record current glyph index */
	previous = glyph_index;
	 }

   /* Free the face and the library objects */
   FT_Done_Face    ( face );
   FT_Done_FreeType( library );

   delete[] ucs4text;
}

void pngwriter::plot_text_blend( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double opacity, int red, int green, int blue)
{
   plot_text_blend( face_path, fontsize, x_start, y_start,  angle,  text, opacity,   ((double) red)/65535.0,  ((double) green)/65535.0,  ((double) blue)/65535.0   );
}

void pngwriter::plot_text_utf8_blend( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double opacity,  int red, int green, int blue)
{
   plot_text_utf8_blend( face_path, fontsize, x_start, y_start,  angle,  text, opacity,  ((double) red)/65535.0,  ((double) green)/65535.0,  ((double) blue)/65535.0   );
}

void pngwriter::my_draw_bitmap_blend( FT_Bitmap * bitmap, int x, int y, double opacity, double red, double green, double blue)
{
   double temp;
   for(int j=1; j<bitmap->rows+1; j++)
	 {
	for(int i=1; i< bitmap->width + 1; i++)
	  {
		 temp = (double)(bitmap->buffer[(j-1)*bitmap->width + (i-1)] )/255.0;

		 if(temp)
		   {
		  this->plot_blend(x + i,
				   y  - j,
				   opacity,
				   temp*red + (1-temp)*(this->dread(x+i,y-j,1)),
				   temp*green + (1-temp)*(this->dread(x+i,y-j,2)),
				   temp*blue + (1-temp)*(this->dread(x+i,y-j,3))
				   );
		   }
	  }
	 }
}

#endif
#ifdef NO_FREETYPE

void pngwriter::plot_text_blend( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double opacity, int red, int green, int blue)
{
   std::cerr << " PNGwriter::plot_text_blend - ERROR **:  PNGwriter was compiled without Freetype support! Recompile PNGwriter with Freetype support (once you have Freetype installed, that is. Websites: www.freetype.org and pngwriter.sourceforge.net)." << std::endl;
   return;
}

void pngwriter::plot_text_blend( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double opacity,  double red, double green, double blue)
{
   std::cerr << " PNGwriter::plot_text_blend - ERROR **:  PNGwriter was compiled without Freetype support! Recompile PNGwriter with Freetype support (once you have Freetype installed, that is. Websites: www.freetype.org and pngwriter.sourceforge.net)." << std::endl;
   return;

}

void pngwriter::plot_text_utf8_blend( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double opacity,  int red, int green, int blue)
{
   std::cerr << " PNGwriter::plot_text_utf8_blend - ERROR **:  PNGwriter was compiled without Freetype support! Recompile PNGwriter with Freetype support (once you have Freetype installed, that is. Websites: www.freetype.org and pngwriter.sourceforge.net)." << std::endl;
   return;
}

void pngwriter::plot_text_utf8_blend( char * face_path, int fontsize, int x_start, int y_start, double angle, char * text, double opacity, double red, double green, double blue)
{
   std::cerr << " PNGwriter::plot_text_utf8_blend - ERROR **:  PNGwriter was compiled without Freetype support! Recompile PNGwriter with Freetype support (once you have Freetype installed, that is. Websites: www.freetype.org and pngwriter.sourceforge.net)." << std::endl;
   return;
}

#endif

///////////////////////////

void pngwriter::boundary_fill_blend(int xstart, int ystart, double opacity, double boundary_red,double boundary_green,double boundary_blue,double fill_red, double fill_green, double fill_blue)
{
   if( (
	(this->dread(xstart,ystart,1) != boundary_red) ||
	(this->dread(xstart,ystart,2) != boundary_green) ||
	(this->dread(xstart,ystart,3) != boundary_blue)
	)
	   &&
	   (
	(this->dread(xstart,ystart,1) != fill_red) ||
	(this->dread(xstart,ystart,2) != fill_green) ||
	(this->dread(xstart,ystart,3) != fill_blue)
	)
	   &&
	   (xstart >0)&&(xstart <= width_)&&(ystart >0)&&(ystart <= height_)
	   )
	 {
	this->plot_blend(xstart, ystart, opacity,  fill_red, fill_green, fill_blue);
	boundary_fill_blend(xstart+1,  ystart, opacity,  boundary_red, boundary_green, boundary_blue, fill_red,  fill_green,  fill_blue) ;
	boundary_fill_blend(xstart,  ystart+1, opacity,  boundary_red, boundary_green, boundary_blue, fill_red,  fill_green,  fill_blue) ;
	boundary_fill_blend(xstart,  ystart-1, opacity,  boundary_red, boundary_green, boundary_blue, fill_red,  fill_green,  fill_blue) ;
	boundary_fill_blend(xstart-1,  ystart, opacity,  boundary_red, boundary_green, boundary_blue, fill_red,  fill_green,  fill_blue) ;
	 }
}

//no int version needed
void pngwriter::flood_fill_internal_blend(int xstart, int ystart, double opacity,  double start_red, double start_green, double start_blue, double fill_red, double fill_green, double fill_blue)
{
   if( (
	(this->dread(xstart,ystart,1) == start_red) &&
	(this->dread(xstart,ystart,2) == start_green) &&
	(this->dread(xstart,ystart,3) == start_blue)
	)
	   &&
	   (
	(this->dread(xstart,ystart,1) != fill_red) ||
	(this->dread(xstart,ystart,2) != fill_green) ||
	(this->dread(xstart,ystart,3) != fill_blue)
	)
	   &&
	   (xstart >0)&&(xstart <= width_)&&(ystart >0)&&(ystart <= height_)
	   )
	 {
	this->plot_blend(xstart, ystart, opacity, fill_red, fill_green, fill_blue);
	flood_fill_internal_blend(  xstart+1,  ystart, opacity,   start_red,  start_green,  start_blue,  fill_red,  fill_green,  fill_blue);
	flood_fill_internal_blend(  xstart-1,  ystart,opacity,   start_red,  start_green,  start_blue,  fill_red,  fill_green,  fill_blue);
	flood_fill_internal_blend(  xstart,  ystart+1, opacity,  start_red,  start_green,  start_blue,  fill_red,  fill_green,  fill_blue);
	flood_fill_internal_blend(  xstart,  ystart-1, opacity,  start_red,  start_green,  start_blue,  fill_red,  fill_green,  fill_blue);
	 }

}

//int version
void pngwriter::boundary_fill_blend(int xstart, int ystart, double opacity, int boundary_red,int boundary_green,int boundary_blue,int fill_red, int fill_green, int fill_blue)
{

   this->boundary_fill_blend( xstart, ystart,
				  opacity,
				  ((double) boundary_red)/65535.0,
				  ((double) boundary_green)/65535.0,
				  ((double) boundary_blue)/65535.0,
				  ((double) fill_red)/65535.0,
				  ((double) fill_green)/65535.0,
				  ((double) fill_blue)/65535.0
				  );
}

void pngwriter::flood_fill_blend(int xstart, int ystart, double opacity, double fill_red, double fill_green, double fill_blue)
{
   flood_fill_internal_blend(  xstart,  ystart, opacity,  this->dread(xstart,ystart,1),this->dread(xstart,ystart,2),this->dread(xstart,ystart,3),   fill_red,  fill_green, fill_blue);
}

//int version
void pngwriter::flood_fill_blend(int xstart, int ystart, double opacity, int fill_red, int fill_green, int fill_blue)
{
   this->flood_fill_blend( xstart,  ystart,
			   opacity,
			   ((double)  fill_red)/65535.0,
			   ((double) fill_green)/65535.0,
			   ((double)  fill_blue)/65535.0
			   );
}

void pngwriter::polygon_blend( int * points, int number_of_points, double opacity,  double red, double green, double blue)
{
   if( (number_of_points<1)||(points ==NULL))
	 {
	std::cerr << " PNGwriter::polygon_blend - ERROR **:  Number of points is zero or negative, or array is NULL." << std::endl;
	return;
	 }

   for(int k=0;k< number_of_points-1; k++)
	 {
	this->line_blend(points[2*k],points[2*k+1],points[2*k+2],points[2*k+3], opacity,  red, green, blue);
	 }
}

//int version
void pngwriter::polygon_blend( int * points, int number_of_points, double opacity, int red, int green, int blue)
{
   this->polygon_blend(points, number_of_points,
			   opacity,
			   ((double) red)/65535.0,
			   ((double) green)/65535.0,
			   ((double) blue)/65535.0
			   );
}

void pngwriter::plotCMYK_blend(int x, int y, double opacity, double cyan, double magenta, double yellow, double black)
{
/*CMYK to RGB:
 *  -----------
 *  red   = 255 - minimum(255,((cyan/255)    * (255 - black) + black))
 *  green = 255 - minimum(255,((magenta/255) * (255 - black) + black))
 *  blue  = 255 - minimum(255,((yellow/255)  * (255 - black) + black))
 * */

   if(cyan<0.0)
	 {
	cyan = 0.0;
	 }
   if(magenta<0.0)
	 {
	magenta = 0.0;
	 }
   if(yellow<0.0)
	 {
	yellow = 0.0;
	 }
   if(black<0.0)
	 {
	black = 0.0;
	 }

   if(cyan>1.0)
	 {
	cyan = 1.0;
	 }
   if(magenta>1.0)
	 {
	magenta = 1.0;
	 }
   if(yellow>1.0)
	 {
	yellow = 1.0;
	 }
   if(black>1.0)
	 {
	black = 1.0;
	 }

   double  red, green, blue, minr, ming, minb, iblack;

   iblack = 1.0 - black;

   minr = 1.0;
   ming = 1.0;
   minb = 1.0;

   if( (cyan*iblack + black)<1.0 )
	 {
	minr = cyan*iblack + black;
	 }

   if( (magenta*iblack + black)<1.0 )
	 {
	ming = magenta*iblack + black;
	 }

   if( (yellow*iblack + black)<1.0 )
	 {
	minb = yellow*iblack + black;
	 }

   red = 1.0 - minr;
   green = 1.0 - ming;
   blue = 1.0 - minb;

   this->plot_blend(x,y,opacity, red, green, blue);

}

//int version
void pngwriter::plotCMYK_blend(int x, int y, double opacity, int cyan, int magenta, int yellow, int black)
{
   this->plotCMYK_blend( x, y,
			 opacity,
			 ((double) cyan)/65535.0,
			 ((double) magenta)/65535.0,
			 ((double) yellow)/65535.0,
			 ((double) black)/65535.0
			 );
}

void pngwriter::laplacian(double k, double offset)
{

   // Create image storage.
   pngwriter temp(width_,height_,0,"temp");

   double red, green, blue;

   for(int x = 1; x <= width_; x++)
	 {
	for(int y = 1; y <= height_; y++)
	  {
		 red =
		   8.0*this->dread(x,y,1) -
		   ( this->dread(x+1, y-1, 1) +
		 this->dread(x,   y-1, 1) +
		 this->dread(x-1, y-1, 1) +
		 this->dread(x-1, y,   1) +
		 this->dread(x+1, y,   1) +
		 this->dread(x+1, y+1, 1) +
		 this->dread(x,   y+1, 1) +
		 this->dread(x-1, y+1, 1) );

		 green =
		   8.0*this->dread(x,y,2) -
		   ( this->dread(x+1, y-1, 2) +
		 this->dread(x,   y-1, 2) +
		 this->dread(x-1, y-1, 2) +
		 this->dread(x-1, y,   2) +
		 this->dread(x+1, y,   2) +
		 this->dread(x+1, y+1, 2) +
		 this->dread(x,   y+1, 2) +
		 this->dread(x-1, y+1, 2));

		 blue =
		   8.0*this->dread(x,y,3) -
		   ( this->dread(x+1, y-1, 3) +
		 this->dread(x,   y-1, 3) +
		 this->dread(x-1, y-1, 3) +
		 this->dread(x-1, y,   3) +
		 this->dread(x+1, y,   3) +
		 this->dread(x+1, y+1, 3) +
		 this->dread(x,   y+1, 3) +
		 this->dread(x-1, y+1, 3));

		 temp.plot(x,y,offset+k*red,offset+k*green,offset+k*blue);

	  }
	 }

   for(int xx = 1; xx <= width_; xx++)
	 {
	for(int yy = 1; yy <= height_; yy++)
	  {
		 this->plot(xx,yy,  temp.read(xx,yy,1), temp.read(xx,yy,2), temp.read(xx,yy,3));
	  }
	 }
}



// drwatop(), drawbottom() and filledtriangle() were contributed by Gurkan Sengun
// ( <gurkan@linuks.mine.nu>, http://www.linuks.mine.nu/ )
void pngwriter::drawtop(long x1,long y1,long x2,long y2,long x3, int red, int green, int blue)
{
   // This swaps x2 and x3
   // if(x2>x3) x2^=x3^=x2^=x3;
   if(x2>x3)
   {
   x2^=x3;
   x3^=x2;
   x2^=x3;
   }

   long posl = x1*256;
   long posr = posl;

   long cl=((x2-x1)*256)/(y2-y1);
   long cr=((x3-x1)*256)/(y2-y1);

   for(int y=y1; y<y2; y++)
	 {
	this->line(posl/256, y, posr/256, y, red, green, blue);
	posl+=cl;
	posr+=cr;
	 }
}

// drwatop(), drawbottom() and filledtriangle() were contributed by Gurkan Sengun
// ( <gurkan@linuks.mine.nu>, http://www.linuks.mine.nu/ )
void pngwriter::drawbottom(long x1,long y1,long x2,long x3,long y3, int red, int green, int blue)
{
   //Swap x1 and x2
   //if(x1>x2) x2^=x1^=x2^=x1;
   if(x1>x2)
   {
   x2^=x1;
   x1^=x2;
   x2^=x1;
	}

   long posl=x1*256;
   long posr=x2*256;

   long cl=((x3-x1)*256)/(y3-y1);
   long cr=((x3-x2)*256)/(y3-y1);

   for(int y=y1; y<y3; y++)
	 {
	this->line(posl/256, y, posr/256, y, red, green, blue);

	posl+=cl;
	posr+=cr;
	 }
}

// drwatop(), drawbottom() and filledtriangle() were contributed by Gurkan Sengun
// ( <gurkan@linuks.mine.nu>, http://www.linuks.mine.nu/ )
void pngwriter::filledtriangle(int x1,int y1,int x2,int y2,int x3,int y3, int red, int green, int blue)
{
   if((x1==x2 && x2==x3) || (y1==y2 && y2==y3)) return;

   if(y2<y1)
	 {
	// x2^=x1^=x2^=x1;
	x2^=x1;
	x1^=x2;
	x2^=x1;
	// y2^=y1^=y2^=y1;
	y2^=y1;
	y1^=x2;
	y2^=y1;
	 }

   if(y3<y1)
	 {
	//x3^=x1^=x3^=x1;
	x3^=x1;
	x1^=x3;
	x3^=x1;
	//y3^=y1^=y3^=y1;
	y3^=y1;
	y1^=y3;
	y3^=y1;
	 }

   if(y3<y2)
	 {
	//x2^=x3^=x2^=x3;
	x2^=x3;
	x3^=x2;
	x2^=x3;
	//y2^=y3^=y2^=y3;
	y2^=y3;
	y3^=y2;
	y2^=y3;
	 }

   if(y2==y3)
	 {
	this->drawtop(x1, y1, x2, y2, x3, red, green, blue);
	 }
   else
	 {
	if(y1==y3 || y1==y2)
	  {
		 this->drawbottom(x1, y1, x2, x3, y3, red, green, blue);
	  }
	else
	  {
		 int new_x = x1 + (int)((double)(y2-y1)*(double)(x3-x1)/(double)(y3-y1));
		 this->drawtop(x1, y1, new_x, y2, x2, red, green, blue);
		 this->drawbottom(x2, y2, new_x, x3, y3, red, green, blue);
	  }
	 }

}

//Double (bug found by Dave Wilks. Was: (int) red*65535, should have been (int) (red*65535).
void pngwriter::filledtriangle(int x1,int y1,int x2,int y2,int x3,int y3, double red, double green, double blue)
{
   this->filledtriangle(x1, y1, x2, y2, x3, y3, (int) (red*65535), (int) (green*65535),  (int) (blue*65535));
}

//Blend, double. (bug found by Dave Wilks. Was: (int) red*65535, should have been (int) (red*65535).
void pngwriter::filledtriangle_blend(int x1,int y1,int x2,int y2,int x3,int y3, double opacity, double red, double green, double blue)
{
   this->filledtriangle_blend( x1, y1, x2, y2, x3, y3,  opacity,  (int) (red*65535), (int) (green*65535),  (int) (blue*65535));
}

//Blend, int
void pngwriter::filledtriangle_blend(int x1,int y1,int x2,int y2,int x3,int y3, double opacity, int red, int green, int blue)
{
   if((x1==x2 && x2==x3) || (y1==y2 && y2==y3)) return;

   /*if(y2<y1)
	 {
	x2^=x1^=x2^=x1;
	y2^=y1^=y2^=y1;
	 }

   if(y3<y1)
	 {
	x3^=x1^=x3^=x1;
	y3^=y1^=y3^=y1;
	 }

   if(y3<y2)
	 {
	x2^=x3^=x2^=x3;
	y2^=y3^=y2^=y3;
	 }
	 */

	 if(y2<y1)
	 {
	// x2^=x1^=x2^=x1;
	x2^=x1;
	x1^=x2;
	x2^=x1;
	// y2^=y1^=y2^=y1;
	y2^=y1;
	y1^=x2;
	y2^=y1;
	 }

   if(y3<y1)
	 {
	//x3^=x1^=x3^=x1;
	x3^=x1;
	x1^=x3;
	x3^=x1;
	//y3^=y1^=y3^=y1;
	y3^=y1;
	y1^=y3;
	y3^=y1;
	 }

   if(y3<y2)
	 {
	//x2^=x3^=x2^=x3;
	x2^=x3;
	x3^=x2;
	x2^=x3;
	//y2^=y3^=y2^=y3;
	y2^=y3;
	y3^=y2;
	y2^=y3;
	 }


   if(y2==y3)
	 {
	this->drawtop_blend(x1, y1, x2, y2, x3, opacity, red, green, blue);
	 }
   else
	 {
	if(y1==y3 || y1==y2)
	  {
		 this->drawbottom_blend(x1, y1, x2, x3, y3, opacity, red, green, blue);
	  }
	else
	  {
		 int new_x = x1 + (int)((double)(y2-y1)*(double)(x3-x1)/(double)(y3-y1));
		 this->drawtop_blend(x1, y1, new_x, y2, x2, opacity,  red, green, blue);
		 this->drawbottom_blend(x2, y2, new_x, x3, y3, opacity, red, green, blue);
	  }
	 }

}

//Blend, int
void pngwriter::drawbottom_blend(long x1,long y1,long x2,long x3,long y3, double opacity, int red, int green, int blue)
{
   //Swap x1 and x2
   if(x1>x2)
   {
   x2^=x1;
   x1^=x2;
   x2^=x1;
   }

   long posl=x1*256;
   long posr=x2*256;

   long cl=((x3-x1)*256)/(y3-y1);
   long cr=((x3-x2)*256)/(y3-y1);

   for(int y=y1; y<y3; y++)
	 {
	this->line_blend(posl/256, y, posr/256, y, opacity, red, green, blue);

	posl+=cl;
	posr+=cr;
	 }

}

//Blend, int
void pngwriter::drawtop_blend(long x1,long y1,long x2,long y2,long x3, double opacity, int red, int green, int blue)
{
   // This swaps x2 and x3
   if(x2>x3)
   {
   x2^=x3;
   x3^=x2;
   x2^=x3;
}

   long posl = x1*256;
   long posr = posl;

   long cl=((x2-x1)*256)/(y2-y1);
   long cr=((x3-x1)*256)/(y2-y1);

   for(int y=y1; y<y2; y++)
	 {
	this->line_blend(posl/256, y, posr/256, y, opacity, red, green, blue);
	posl+=cl;
	posr+=cr;
	 }

}

void pngwriter::triangle(int x1, int y1, int x2, int y2, int x3, int y3, int red, int green, int blue)
{
   this->line(x1, y1, x2, y2, red, green, blue);
   this->line(x2, y2, x3, y3, red, green, blue);
   this->line(x3, y3, x1, y1, red, green, blue);
}

void pngwriter::triangle(int x1, int y1, int x2, int y2, int x3, int y3, double red, double green, double blue)
{

   this->line(x1, y1, x2, y2, ((int)65535*red), ((int)65535*green), ((int)65535*blue));
   this->line(x2, y2, x3, y3, ((int)65535*red), ((int)65535*green), ((int)65535*blue));
   this->line(x3, y3, x1, y1, ((int)65535*red), ((int)65535*green), ((int)65535*blue));

}





void pngwriter::arrow( int x1,int y1,int x2,int y2,int size, double head_angle, double red, double green, double blue)
{

   this->line(x1, y1, x2, y2, red, green, blue);
   //   double th = 3.141592653589793 + (head_angle)*3.141592653589793/180.0;  //degrees
   double th = 3.141592653589793 + head_angle;
   double costh = cos(th);
   double sinth = sin(th);
   double t1, t2, r;
   t1 = ((x2-x1)*costh - (y2-y1)*sinth);
   t2 = ((x2-x1)*sinth + (y2-y1)*costh);
   r = sqrt(t1*t1 + t2*t2);

   double advancex  = size*t1/r;
   double advancey  = size*t2/r;
   this->line(x2, y2, int(x2 + advancex), int(y2 + advancey), red, green, blue);
   t1 = (x2-x1)*costh + (y2-y1)*sinth;
   t2 =   (y2-y1)*costh - (x2-x1)*sinth;

   advancex  = size*t1/r;
   advancey  = size*t2/r;
   this->line(x2, y2, int(x2 + advancex), int(y2 + advancey), red, green, blue);
}

void pngwriter::filledarrow( int x1,int y1,int x2,int y2,int size, double head_angle, double red, double green, double blue)
{
   int p1x, p2x, p3x, p1y, p2y, p3y;

   this->line(x1, y1, x2, y2, red, green, blue);
   double th = 3.141592653589793 + head_angle;
   double costh = cos(th);
   double sinth = sin(th);
   double t11, t12, t21, t22, r1, r2;
   t11 = ((x2-x1)*costh - (y2-y1)*sinth);
   t21 = ((x2-x1)*sinth + (y2-y1)*costh);
   t12 = (x2-x1)*costh + (y2-y1)*sinth;
   t22 =   (y2-y1)*costh - (x2-x1)*sinth;

   r1 = sqrt(t11*t11 + t21*t21);
   r2 = sqrt(t12*t12 + t22*t22);

   double advancex1  = size*t11/r1;
   double advancey1  = size*t21/r1;
   double advancex2  = size*t12/r2;
   double advancey2  = size*t22/r2;

   p1x = x2;
   p1y = y2;

   p2x = int(x2 + advancex1);
   p2y = int(y2 + advancey1);

   p3x = int(x2 + advancex2);
   p3y = int(y2 + advancey2);


   this->filledtriangle( p1x,  p1y,  p2x,  p2y,  p3x,  p3y, red, green,  blue);

}

void pngwriter::arrow( int x1,int y1,int x2,int y2,int size, double head_angle, int red, int green, int blue)
{
   this->arrow(  x1, y1, x2, y2, size,  head_angle,  (double (red))/65535.0,  (double (green))/65535.0,  (double (blue))/65535.0 );
}

void pngwriter::filledarrow( int x1,int y1,int x2,int y2,int size, double head_angle, int red, int green, int blue)
{
   this->filledarrow(  x1, y1, x2, y2, size,  head_angle, (double (red))/65535.0,  (double (green))/65535.0, (double (blue))/65535.0 );
}


void pngwriter::cross( int x, int y, int xwidth, int yheight, int red, int green, int blue)
{
   this->line(int(x - xwidth/2.0), y, int(x + xwidth/2.0), y, red, green, blue);
   this->line(x, int(y - yheight/2.0), x, int(y + yheight/2.0), red, green, blue);
}

void pngwriter::maltesecross( int x, int y, int xwidth, int yheight, int x_bar_height, int y_bar_width, int red, int green, int blue)
{
   this->line(int(x - xwidth/2.0), y, int(x + xwidth/2.0), y, red, green, blue);
   this->line(x, int(y - yheight/2.0), x, int(y + yheight/2.0), red, green, blue);
   // Bars on ends of vertical line
   this->line(int(x - y_bar_width/2.0), int(y + yheight/2.0), int(x + y_bar_width/2.0), int(y + yheight/2.0), red, green, blue);
   this->line(int(x - y_bar_width/2.0), int(y - yheight/2.0), int(x + y_bar_width/2.0), int(y - yheight/2.0), red, green, blue);
   // Bars on ends of horizontal line.
   this->line(int(x - xwidth/2.0), int(y - x_bar_height/2.0), int(x - xwidth/2.0), int(y + x_bar_height/2.0), red, green, blue);
   this->line(int(x + xwidth/2.0), int(y - x_bar_height/2.0), int(x + xwidth/2.0), int(y + x_bar_height/2.0), red, green, blue);
}

void pngwriter::cross( int x, int y, int xwidth, int yheight, double red, double green, double blue)
{
   this->cross( x, y, xwidth, yheight, int(65535*red), int(65535*green), int(65535*blue));
}

void pngwriter::maltesecross( int x, int y, int xwidth, int yheight, int x_bar_height, int y_bar_width, double red, double green, double blue)
{
   this->maltesecross( x, y, xwidth, yheight, x_bar_height, y_bar_width, int(65535*red), int(65535*green), int(65535*blue));
}


void pngwriter::filleddiamond( int x, int y, int width, int height, int red, int green, int blue)
{
   this->filledtriangle( int(x - width/2.0), y, x, y, x, int(y + height/2.0), red, green, blue);
   this->filledtriangle( int(x + width/2.0), y, x, y, x, int(y + height/2.0), red, green, blue);
   this->filledtriangle( int(x - width/2.0), y, x, y, x, int(y - height/2.0), red, green, blue);
   this->filledtriangle( int(x + width/2.0), y, x, y, x, int(y - height/2.0), red, green, blue);
}

void pngwriter::diamond( int x, int y, int width, int height, int red, int green, int blue)
{
   this->line( int(x - width/2.0), y, x, int(y + height/2.0), red, green, blue);
   this->line( int(x + width/2.0), y, x, int(y + height/2.0), red, green, blue);
   this->line( int(x - width/2.0), y, x, int(y - height/2.0), red, green, blue);
   this->line( int(x + width/2.0), y, x, int(y - height/2.0), red, green, blue);
}


void pngwriter::filleddiamond( int x, int y, int width, int height, double red, double green, double blue)
{
   this->filleddiamond(  x, y,  width,  height, int(red*65535), int(green*65535), int(blue*65535) );
}

void pngwriter::diamond( int x, int y, int width, int height, double red, double green, double blue)
{
   this->diamond(  x,  y,  width,  height, int(red*65535), int(green*65535), int(blue*65535) );
}

