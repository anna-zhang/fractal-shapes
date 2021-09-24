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

  int orientation = 0; // 0 for horizontal scan, 1 for vertical scan; default horizontal scan
  // format: ./movieMaker [orientation: either -h or -v]
  if (argc == 2)
  {
    if (strcmp(argv[1], "-v") == 0)
    {
      orientation = 1;
    }
  }


  bool readSuccess = true;
  int frameNumber = 0;

  if (orientation == 0) // horizontal scan
  {
    while (readSuccess)
    {
      // build the next frame's filename
      char buffer[256];
      sprintf(buffer, "frame.%06i.ppm", frameNumber);
    
      // try reading the next sequential frame
      int width, height;
      unsigned char* pixels = NULL;
      readSuccess = readPPM(buffer, pixels, width, height); 

      // if it exists, add it
      if (readSuccess)
        movie.addFrame(pixels, width, height);

      if (pixels) delete[] pixels;
      frameNumber++;
    }
  }
  else // vertical scan
  {
    // root0 sweeping vertical, root1 sweeping vertical
    int column = 0;
    for (int y = 0; y < 11; y++)
    {
      for (int z = 0; z < 11; z++)
      {
        for (int i = 0; i < 11; i++)
        {
          frameNumber = i + 1331 * z + 121 * y;
          for (int j = 0; j < 11; j++)
          {
            // build the next frame's filename
            char buffer[256];
            sprintf(buffer, "frame.%06i.ppm", frameNumber);
          
            // try reading the next sequential frame
            int width, height;
            unsigned char* pixels = NULL;
            readSuccess = readPPM(buffer, pixels, width, height); 

            // if it exists, add it
            if (readSuccess)
              // cout << "here" << endl;
              movie.addFrame(pixels, width, height);
            else break;

            if (pixels) delete[] pixels;
            frameNumber += 11;
          }
        }
      }
    }
    
    // // root0 sweeping horizontal, root1 sweeping vertical: output 09-23-2021_10x10_hv.mov
    // int column = 0;
    // for (int z = 0; z < 121; z++)
    // {
    //   for (int i = 0; i < 11; i++)
    //   {
    //     frameNumber = i + 121 * z;
    //     for (int j = 0; j < 11; j++)
    //     {
    //       // build the next frame's filename
    //       char buffer[256];
    //       sprintf(buffer, "frame.%06i.ppm", frameNumber);
        
    //       // try reading the next sequential frame
    //       int width, height;
    //       unsigned char* pixels = NULL;
    //       readSuccess = readPPM(buffer, pixels, width, height); 

    //       // if it exists, add it
    //       if (readSuccess)
    //         // cout << "here" << endl;
    //         movie.addFrame(pixels, width, height);
    //       else break;

    //       if (pixels) delete[] pixels;
    //       frameNumber += 11;
    //     }
    //   }
    // }  
  }
  

  // write out the compiled movie
  movie.writeMovie("movie.mov");

  return 0;
}
