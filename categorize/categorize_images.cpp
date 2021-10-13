#include <cmath>
#include <cstdio>
#include "../VEC3F.h"
#include <random>

#include <iostream>
#include <fstream>
#include <map>

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

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    // create and open a text file to store categories of images
    ofstream imageCategories("categories/image_groups.txt");

    bool referenceReadSuccess = false; // store whether the reference image has been read successfully

    if (argc != 2)
    {
        // error
        cout << "Program usage: ./categorize (# of images total)" << endl;
        return 0;
    }

    int totalFrames = atoi(argv[1]); // number of total frames to go through

    // create a map that stores whether an image has been categorized indexed by image name
    map<int, bool> allImages; // key is the integer # of the frame and value is whether it has been categorized (true if categorized, false if uncategorized)
    for (int i = 0; i < totalFrames; i++)
    {
        allImages[i] = false; // initialize all images in map to uncategorized (set to false)
    }

    for (int i = 0; i < totalFrames; i++)
    {
        if (allImages[i] == false)
        {
            // build reference image frame's filename
            char reference_buffer[256];
            sprintf(reference_buffer, "../shapes/frame.%06i.ppm", i);

            // try reading the reference image's frame
            int width, height;
            unsigned char* reference_pixels = NULL;
            referenceReadSuccess = readPPM(reference_buffer, reference_pixels, width, height); // attempt to read the reference image 
            allImages[i] = true; // remember that this image has been categorized

            if (referenceReadSuccess) // reference image is successfully read
            {
                // use that images as the reference image and go through all remaining uncategorized images
                for (int j = i; j < totalFrames; j++)
                {
                    if (allImages[j] == false)
                    {
                        // build image to compare it to's filename
                        char img_buffer[256];
                        sprintf(img_buffer, "../shapes/frame.%06i.ppm", j);

                        // try reading the next sequential frame as the image to categorize
                        int width, height;
                        unsigned char* img_pixels = NULL;
                        bool readSuccess = readPPM(img_buffer, img_pixels, width, height); // attempt to read the uncategorized image to compare it against the reference image
                        int totalPixels = width * height; // total number of pixels in the image

                        int pixelMatches = 0; // hold number of pixel matches between the reference image and the image to categorize
                        
                        if (readSuccess) // uncategorized image is successfully read
                        {
                            // iterate through every pixel in the image and see if the pixels in the reference image and the uncategorized image match up
                            for (int x = 0; x < width; x++)
                            {
                                for (int y = 0; y < height; y++)
                                {
                                    int pixelIndex = x + (height - y) * width; // calculate pixel index for pixel values array that represents the final output image

                                    if ((reference_pixels[3 * pixelIndex] == img_pixels[3 * pixelIndex]) && (reference_pixels[3 * pixelIndex + 1] == img_pixels[3 * pixelIndex + 1]) && (reference_pixels[3 * pixelIndex + 2] == img_pixels[3 * pixelIndex + 2]))
                                    {
                                        // the red, green, and blue values of the pixel are the same, so the two images have the same value at that pixel
                                        pixelMatches += 1;
                                    }
                                }
                            }
                            
                            float ratio = float(pixelMatches) / float(totalPixels); // get the ratio of how many pixels matched up between the two images compared to the total number of pixels

                            if (ratio > 0.99) // will need to adjust ratio
                            {
                                // this image is part of the reference image's category
                                cout << "ratio: " << ratio << endl;
                                allImages[j] = true; // remember that this image has been categorized
                                imageCategories << i << " and " << j << endl; // save the category pair in a text file for now
                            }
                        }
                    }
                }
            }
        }
    }
    return 1;
}