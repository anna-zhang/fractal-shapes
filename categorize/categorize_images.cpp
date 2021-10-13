#include <cmath>
#include <cstdio>
#include "../VEC3F.h"
#include <random>

#include <iostream>
#include <fstream>
#include <sys/stat.h>
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

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void writePPM(const string &filename, int &xRes, int &yRes, unsigned char *values)
{
  int totalCells = xRes * yRes;
  unsigned char *pixels = new unsigned char[3 * totalCells];
  for (int i = 0; i < 3 * totalCells; i++)
    pixels[i] = values[i];

  FILE *fp;
  fp = fopen(filename.c_str(), "wb");
  if (fp == NULL)
  {
    cout << " Could not open file \"" << filename.c_str() << "\" for writing." << endl;
    cout << " Make sure you're not trying to write from a weird location or with a " << endl;
    cout << " strange filename. Bailing ... " << endl;
    exit(0);
  }

  fprintf(fp, "P6\n%d %d\n255\n", xRes, yRes);
  fwrite(pixels, 1, totalCells * 3, fp);
  fclose(fp);
  delete[] pixels;
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
    int numShapeCategories = 0; // number of shape categories

    // create a map that stores whether an image has been categorized indexed by image name
    map<int, bool> allImages; // key is the integer # of the frame and value is whether it has been categorized (true if categorized, false if uncategorized)
    for (int i = 0; i < totalFrames; i++)
    {
        allImages[i] = false; // initialize all images in map to uncategorized (set to false)
    }

    for (int i = 0; i < totalFrames; i++)
    {
        if (allImages[i] == false) // go to the next uncategorized image and set that as the reference image for a new shape category
        {
            // build reference image frame's filename
            char reference_buffer[256];
            sprintf(reference_buffer, "../shapes/frame.%06i.ppm", i);

            // try reading the reference image's frame
            int width, height;
            unsigned char* reference_pixels = NULL;
            referenceReadSuccess = readPPM(reference_buffer, reference_pixels, width, height); // attempt to read the reference image 
            if (!referenceReadSuccess)
            {
                // error
                cout << "Failed to read " << reference_buffer << endl;
                return 1;
            }

            numShapeCategories += 1; // one more shape category created
            char folder_name_buffer[256]; // hold folder name for the new shape category
            sprintf(folder_name_buffer, "categories/shape.%03i", numShapeCategories);
            int createFolderSuccess = mkdir(folder_name_buffer, S_IRUSR | S_IWUSR | S_IXUSR); // create a folder for this new shape category; returns 0 if successful, -1 otherwise

            // build copy of reference image frame's filename
            char reference_copy[256]; // the filename for the copy of the reference image
            sprintf(reference_copy, "categories/shape.%03i/frame.%06i.ppm", numShapeCategories, i);
            writePPM(reference_copy, width, height, reference_pixels); // make a copy of the reference image and place it in the folder for the new shape category

            allImages[i] = true; // remember that this image has been categorized

            if (referenceReadSuccess && (createFolderSuccess == 0)) // reference image is successfully read and shape category folder is successfully created
            {
                // use that images as the reference image and go through all remaining uncategorized images
                for (int j = i; j < totalFrames; j++)
                {
                    if (allImages[j] == false) // only go through remaining uncategorized images
                    {
                        // build image to compare it to's filename
                        char img_buffer[256]; // the filename for the uncategorized image
                        sprintf(img_buffer, "../shapes/frame.%06i.ppm", j);

                        // try reading the next sequential frame as the image to categorize
                        int width, height;
                        unsigned char* img_pixels = NULL; // the pixel info for the uncategorized image
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

                                    if ((float(reference_pixels[3 * pixelIndex]) == float(img_pixels[3 * pixelIndex])) && (float(reference_pixels[3 * pixelIndex + 1]) == float(img_pixels[3 * pixelIndex + 1])) && (float(reference_pixels[3 * pixelIndex + 2]) == float(img_pixels[3 * pixelIndex + 2])))
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
                                
                                // build copy of uncategorized image frame's filename
                                char uncategorized_copy[256]; // filename for the copy of the uncategorized image that is now categorized in the same category as the reference image
                                sprintf(uncategorized_copy, "categories/shape.%03i/frame.%06i.ppm", numShapeCategories, j);
                                writePPM(uncategorized_copy, width, height, img_pixels); // make a copy of the newly categorized image and place it in the shape category's folder
                            }
                        }

                        delete[] img_pixels; // clean up
                    }
                }
            }

            delete[] reference_pixels; // clean up
        }
    }

    imageCategories.close(); // close file after done writing
    return 1;
}