#include "stdafx.h"

using namespace Magick;
using namespace std;

#include "ShapeIndexer.h"

bool FindUnvisitedPixel(const PixelPacket *pc, const bool *pixelvisited, int cols, int rows, int *px, int *py);

static int foo = 0;
static Image *shapeLibrary1 = NULL;
static Image *shapeLibrary2 = NULL;
static Image *shapeLibrary3 = NULL;
static Image *shapeLibrary4 = NULL;

int main(int argc, char* argv[])
{
    string s;

	// C:\\Data\\Programs\\ImageMagick\\
	// E:\\Programs\\Dev\\ImageMagick\\

    Magick::InitializeMagick("C:\\Data\\Programs\\ImageMagick\\");

    if (argc > 1)
    {
        s = string(argv[1]);
    }
    else
    {
        s = "button.gif";
    }

    Image img(s.c_str());
    //img.quantizeColorSpace(YUVColorspace);
	img.segment(1.0, 1.5); // segments the image with the parameter values 1.0 and 1.5
	img.modifyImage(); // ensures that no other references to this image exist

	int cols = img.columns();
	int rows = img.rows();
	const PixelPacket *pc = img.getConstPixels(0, 0, cols, rows); // gets a pointer to the pixels of the image

	int numSegments = CountSegments(pc, cols, rows); // counts the number of segments
	cout << "Number of segments is: " << numSegments << "\r\n"; // prints to the console

	IterateThroughEachShape(pc, cols, rows);

	return 0;
}

void IterateThroughEachShape(const PixelPacket *px, int cols, int rows)
{
	int x = 0, y = 0, shapecount=0;
	vector<MyPoint> mystack; // a stack to keep track of pixels in the same segment
	bool *pixelvisited = new bool[cols*rows]; // creates an array to keep track of which pixels have been visited
	bool *shapelayer = new bool[cols*rows]; // a bool array that marks the positions corresponding to the current shape

	memset(pixelvisited, 0, cols*rows*sizeof(bool));
	

	while(FindUnvisitedPixel(px, pixelvisited, cols, rows, &x, &y))
	{
		shapecount++;
		MyPoint pt1 = GetPoint(x, y);
		mystack.push_back(pt1); // push the starting point into the stack

		memset(shapelayer, 0, cols*rows*sizeof(bool));

		pixelvisited[y*cols + x] = true; // mark the starting pixel as visited
		Color mycolor = GetPxColor(px, cols, x, y);

		while(!mystack.empty())
		{
			MyPoint pt = mystack.back(); mystack.pop_back(); // pop out an element from the stack
			int x = pt.x;
			int y = pt.y;
		
			VisitPixelAndMarkShape(x,   y-1, px, pixelvisited, shapelayer, cols, rows, mystack, mycolor); // north
			VisitPixelAndMarkShape(x+1, y-1, px, pixelvisited, shapelayer, cols, rows, mystack, mycolor); // north-east
			VisitPixelAndMarkShape(x+1, y,   px, pixelvisited, shapelayer, cols, rows, mystack, mycolor); // east
			VisitPixelAndMarkShape(x+1, y+1, px, pixelvisited, shapelayer, cols, rows, mystack, mycolor); // south-east
			VisitPixelAndMarkShape(x,   y+1, px, pixelvisited, shapelayer, cols, rows, mystack, mycolor); // south
			VisitPixelAndMarkShape(x-1, y+1, px, pixelvisited, shapelayer, cols, rows, mystack, mycolor); // south-west
			VisitPixelAndMarkShape(x-1, y,   px, pixelvisited, shapelayer, cols, rows, mystack, mycolor); // west
			VisitPixelAndMarkShape(x-1, y-1, px, pixelvisited, shapelayer, cols, rows, mystack, mycolor); // north-west
		}

		// at this point, the shape is ready.
		FindShapeLibraryDescriptor(px, shapelayer, rows, cols, 0, 0);
		
	}

	delete[] pixelvisited;
	delete[] shapelayer;
}

int FindShapeLibraryDescriptor(const PixelPacket *pc, const bool *shapelayer, int rows, int cols, int x, int y)
{
	// step 1: find the bounds of the shape
	int shapeminx = INT_MAX, shapeminy = INT_MAX, shapemaxx = INT_MIN, shapemaxy = INT_MIN; 
	for (int y=0; y<rows; y++)
	{
		for (int x=0; x<cols; x++)
		{
			if (*(shapelayer + y*cols + x))
			{
				shapeminx = min(shapeminx, x);
				shapemaxx = max(shapemaxx, x);
				shapeminy = min(shapeminy, y);
				shapemaxy = max(shapemaxy, y);
			}
		}
	}

	// step 2: create an image and blt it to that
	int shpwidth = shapemaxx - shapeminx;
	int shpheight = shapemaxy - shapeminy;
	Image shp(Geometry(shpwidth, shpheight), ColorMono(false));
	shp.modifyImage();
	PixelPacket *shpPix = shp.getPixels(0, 0, shpwidth, shpheight);
	for (int y=0; y<shpheight; y++)
	{
		for (int x=0; x<shpwidth; x++)
		{
			if ((*(shapelayer + (y+shapeminy)*cols + (x+shapeminx))))
			{
				*(shpPix +y*shpwidth + x)= ColorMono(true);
			}
		}
	}
	
	// step 3: resize the image to standard size
	shp.transform(Geometry(32, 32));
	// bug ImageMagick ignores the gravity parameter
	//shp.extent(Geometry(32, 32), ColorMono(false), GravityType::SouthGravity);

	Image shp2(Geometry(32, 32), ColorMono(false));
	shp2.composite(shp, GravityType::CenterGravity, CompositeOperator::OverCompositeOp);

	// step 2a: debug ... write it to a file so we can see how it looks
	char szFilename[200];
	sprintf(szFilename, "MyShape_%d.png", foo);
	foo++;
	shp2.write(szFilename);

	// step 4: go through the shape library, trying to find max overlap
	if (shapeLibrary1 == NULL){	shapeLibrary1 = new Image("shapeLibrary1.png");	}
	if (shapeLibrary2 == NULL){	shapeLibrary2 = new Image("shapeLibrary2.png");	}
	if (shapeLibrary3 == NULL){	shapeLibrary3 = new Image("shapeLibrary3.png");	}
	if (shapeLibrary4 == NULL){	shapeLibrary4 = new Image("shapeLibrary4.png");	}

	const PixelPacket *shplib1 = shapeLibrary1->getConstPixels(0, 0, shapeLibrary1->columns(), shapeLibrary1->rows());
	const PixelPacket *shplib2 = shapeLibrary2->getConstPixels(0, 0, shapeLibrary2->columns(), shapeLibrary2->rows());
	const PixelPacket *shplib3 = shapeLibrary3->getConstPixels(0, 0, shapeLibrary3->columns(), shapeLibrary3->rows());
	const PixelPacket *shplib4 = shapeLibrary4->getConstPixels(0, 0, shapeLibrary4->columns(), shapeLibrary4->rows());

	const PixelPacket *shppx = shp2.getConstPixels(0, 0, shp2.columns(), shp2.rows());

	int byte1 = GetShapeLibraryByte(shplib1, shppx, shp2.columns(), shp2.rows());
	int byte2 = GetShapeLibraryByte(shplib2, shppx, shp2.columns(), shp2.rows());
	int byte3 = GetShapeLibraryByte(shplib3, shppx, shp2.columns(), shp2.rows());
	int byte4 = GetShapeLibraryByte(shplib4, shppx, shp2.columns(), shp2.rows());
	
	printf("the best match was: (%d, %d, %d, %d)\n", byte1, byte2, byte3, byte4);

	return 1;
}

int GetShapeLibraryByte(const PixelPacket* shapeLibrary, const PixelPacket* shape, int shapeCols, int shapeRows)
{
	int libmaxX = 0;
	int libmaxY = 0;
	int libcounterMax = 0;

	for (int liby=0; liby<8; liby++)
	{
		for (int libx=0; libx<8; libx++)
		{
			int overlap = FindOverlap(shapeLibrary, libx, liby, shape, shapeCols, shapeRows);

			if (overlap > libcounterMax)
			{
				libmaxX = libx;
				libmaxY = liby;
				libcounterMax = overlap;
			}
		}
	}

	printf("inside byte: %d, %d\n", libmaxX, libmaxY);
	return (libmaxX << 4) | libmaxY;
}

int FindOverlap(const PixelPacket* shapeLibrary, int libx, int liby, const PixelPacket* shape, int shpCols, int shpRows)
{
	int startx = (32 - shpCols)/2;
	int starty = (32 - shpRows)/2;
	int counter = 0;

	//printf("libx: %d, liby: %d\n", libx, liby);

	//Image tim(Geometry(32, 32), ColorMono(true));
	//tim.modifyImage();
	//PixelPacket *ptix = tim.getPixels(0, 0, 32, 32);

	for (int y=0; y<shpRows; y++)
	{
		for (int x=0; x<shpCols; x++)
		{
			if (((shapeLibrary + (liby*32 + starty + y)*256 + (libx*32 + startx + x))->opacity <= 0)) // shape library
			{
				if((shape + y*shpCols + x)->red > 0) // XOR with shape
				{
					counter++;
					//*(ptix + y*32 + x) = ColorMono(false);
				}
				else
				{
					counter--;
					//*(ptix + y*32 + x) = ColorMono(true);
				}
			}
			else if((shape +  y*shpCols + x)->red > 0) // negate if shape isnt but shape is
			{
				counter--;
			}

		}
	}

	//char szShapeMatch[200];
	//sprintf(szShapeMatch, "Shape(%d)_Lib(%d, %d).png", foo, libx, liby);
	//tim.write(szShapeMatch);

	return counter;
}

bool FindUnvisitedPixel(const PixelPacket *pc, const bool *pixelvisited, int cols, int rows, int *px, int *py)
{
	int x, y;
	bool reply = false;

	// iterate through the whole image till we find a pixel that has not been visited yet.
	for (y=0; (!reply) && y<rows; y++)
	{
		for (x=0; (!reply) && x<cols; x++)
		{
			if (!pixelvisited[y*cols + x])
			{
				reply = true;
				*px = x;
				*py = y;
			}
		}
	}
	
	return reply;
}

bool IsInBounds(int x, int y, int cols, int rows)
{
	return ( (x >= 0) && (x<cols) && (y >= 0) && (y<rows) );
}

Color GetPxColor(const PixelPacket *px, int cols, int x, int y)
{
	return *(px + y*cols + x);
}

MyPoint GetPoint(int x, int y)
{
	MyPoint pt;
	pt.x = x;
	pt.y = y;
	return pt;
}

void VisitPixelAndMarkShape(int x, int y, const PixelPacket *px, bool *pixelvisited, bool *shapelayer, int cols, int rows, vector<MyPoint>& mystack, const Color& mycolor)
{
	// if the pixel is within bounds, hasn't been visited yet, and is the same color as me
	if (IsInBounds(x, y, cols, rows) && (!pixelvisited[y*cols+x]) && mycolor == GetPxColor(px, cols, x, y))
	{
		mystack.push_back(GetPoint(x, y)); // add to the stack
		pixelvisited[y*cols + x] = true; // mark as visited
		shapelayer[y*cols + x] = true;
	}
}

void VisitPixel(int x, int y, const PixelPacket *px, bool *pixelvisited, int cols, int rows, vector<MyPoint>& mystack, const Color& mycolor)
{
	// if the pixel is within bounds, hasn't been visited yet, and is the same color as me
	if (IsInBounds(x, y, cols, rows) && (!pixelvisited[y*cols+x]) && mycolor == GetPxColor(px, cols, x, y))
	{
		mystack.push_back(GetPoint(x, y)); // add to the stack
		pixelvisited[y*cols + x] = true; // mark as visited
	}
}

void FloodFill(const PixelPacket *px, bool *pixelvisited, int cols, int rows, int x, int y)
{
	vector<MyPoint> mystack; // a stack to keep track of pixels in the same segment
	MyPoint pt1 = GetPoint(x, y);
	mystack.push_back(pt1);
	pixelvisited[y*cols + x] = true; // mark the starting pixel as visited
	Color mycolor = GetPxColor(px, cols, x, y);

	while(!mystack.empty())
	{
		MyPoint pt = mystack.back(); mystack.pop_back(); // pop out an element from the stack
		int x = pt.x;
		int y = pt.y;
		
		VisitPixel(x, y-1, px, pixelvisited, cols, rows, mystack, mycolor);   // north
		VisitPixel(x+1, y-1, px, pixelvisited, cols, rows, mystack, mycolor); // north-east
		VisitPixel(x+1, y, px, pixelvisited, cols, rows, mystack, mycolor);   // east
		VisitPixel(x+1, y+1, px, pixelvisited, cols, rows, mystack, mycolor); // south-east
		VisitPixel(x, y+1, px, pixelvisited, cols, rows, mystack, mycolor);   // south
		VisitPixel(x-1, y+1, px, pixelvisited, cols, rows, mystack, mycolor); // south-west
		VisitPixel(x-1, y, px, pixelvisited, cols, rows, mystack, mycolor);   // west
		VisitPixel(x-1, y-1, px, pixelvisited, cols, rows, mystack, mycolor); // north-west
	}
}

int CountSegments(const PixelPacket *pc, int cols, int rows)
{
	int x=0, y=0;
	bool *pixelvisited = new bool[cols*rows]; // creates an array to keep track of which pixels have been visited
	memset(pixelvisited, 0, cols*rows*sizeof(bool));
	int count = 0;

	// till no pixels are unvisited
	while(FindUnvisitedPixel(pc, pixelvisited, cols, rows, &x, &y))
	{
		count++;
		FloodFill(pc, pixelvisited, cols, rows, x, y); // mark that pixel and all pixels in the same segment as visited
	}

	return count;
}