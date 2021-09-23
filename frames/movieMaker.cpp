#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include "../QUICKTIME_MOVIE.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////////
// Read in a raw PPM file of the "P6" style.
//
// Input: "filename" is the name of the file you want to read in
// Output: "pixels" will point to an array of pixel values
//         "width" will be the width of the image
//         "height" will be the height of the image
//
// The PPM file format is:
//
//   P6
//   <image width> <image height>
//   255
//   <raw, 8-bit binary stream of RGB values>
//
// Open one in a text editor to see for yourself.
//
//////////////////////////////////////////////////////////////////////////////////
bool readPPM(const char* filename, unsigned char*& pixels, int& width, int& height)
{
  // try to open the file
  FILE* file;
  file = fopen(filename, "rb");
  if (file == NULL)
  {
    cout << " Couldn't open file " << filename << "! " << endl;
    return false; 
  }

  // get the dimensions
  unsigned char newline;
  fscanf(file, "P6\n%d %d\n255%c", &width, &height, &newline);
  if (newline != '\n') {
    cout << " The header of " << filename << " may be improperly formatted." << endl;
    cout << " The program will continue, but you may want to check your input. " << endl;
  }
  int totalPixels = width * height;

  // allocate three times as many pixels since there are R,G, and B channels
  pixels = new unsigned char[3 * totalPixels];
  fread(pixels, 1, 3 * totalPixels, file);
  fclose(file);
  
  // output some success information
  cout << " Successfully read in " << filename << " with dimensions: " 
       << width << " " << height << endl;
  return true;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
  QUICKTIME_MOVIE movie;

  bool readSuccess = true;
  int frameNumber = 0;
  while (readSuccess)
  {
    // build the next frame's filename
    char buffer[256];
    sprintf(buffer, "frame.%04i.ppm", frameNumber);
  
    // try reading the next sequential frame
    int width, height;
    unsigned char* pixels = NULL;
    readSuccess = readPPM(buffer, pixels, width, height); 

    // if it exists, add it
    if (readSuccess)
      // cout << "here" << endl;
      movie.addFrame(pixels, width, height);

    if (pixels) delete[] pixels;
    frameNumber++;
  }

  // write out the compiled movie
  movie.writeMovie("movie.mov");

  return 0;
}
