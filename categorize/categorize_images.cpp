#include <cmath>
#include <cstdio>
#include "../VEC3F.h"
#include <random>

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <map>

using namespace std;

// forward declare the same shape function here
bool sameShape(int refFrameNum, int uncategorizedFrameNum, int categoryNum, float cutoffScore, ofstream& shapeMatches, bool allInfo);

// forward declare the create category function here
bool createCategory(int frameNum, int numShapeCategories, ofstream& shapeMatches);

// forward declare the categorize all images function here so that we can put it at the bottom of the file
bool categorizeAll(int totalFrames, ofstream& imageCategories, float cutoffScore);

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

///////////////////////////////////////////////////////////////////////////////////////////
// Program usage: ./categorize (-all or -specific) [specific flag parameters]
///////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    if (argc > 2)
    {
        if (strcmp(argv[1], "-all") == 0) 
        {
            // Categorize all images
            // Program usage: ./categorize -all (# of images) (cutoff score)
            if (argc != 4)
            {
                // error
                cout << "Program usage: ./categorize -all (# of images) (cutoff score)" << endl;
                return 1;
            }


            // create and open a text file to store categories of images
            ofstream imageCategories("categories/image_groups.txt");
            if (imageCategories.is_open()) // make sure the text file can be opened
            {
                imageCategories << "Categorization information for: "; // save where the categorization information came from in the text file (the command)
                for (int i = 0; i < argc; i++)
                {
                    imageCategories << argv[i] << " ";
                }
                imageCategories << endl;

                // categorize all images
                bool categorizeSuccess = categorizeAll(atoi(argv[2]), imageCategories, atof(argv[3])); 
                if (!categorizeSuccess)
                {
                    // error
                    cout << "Failed to categorize all images." << endl;
                    return 1;
                }
            }
            else
            {
                // error
                cout << "Text file to hold image categories cannot be opened" << endl;
                return 1;
            }
            
        }
        else if (strcmp(argv[1], "-specific") == 0) 
        {
            // Calculate match between two specific images
            // Program usage: ./categorize -specific (first image #) (second image #) (cutoff score)
            if (argc != 5)
            {
                // error
                cout << "Program usage: ./categorize -specific (first image #) (second image #) (cutoff score)" << endl;
                return 1;
            }

            int firstImgNum = atoi(argv[2]); // get the first image's frame #
            int secondImgNum = atoi(argv[3]); // get the second image's frame #
            float cutoffScore = atof(argv[4]); // get the cutoff score for shape matches
            cout << "Calculating match between " << firstImgNum << " and " << secondImgNum << " with a cutoff score of " << cutoffScore << endl;

            // create and open a text file to store categories of images
            ofstream shapeMatchInfo("categories/shape_match.txt");
            if (shapeMatchInfo.is_open()) // make sure the text file can be opened
            {
                shapeMatchInfo << "Shape match information for: "; // save where the shape match information came from in the text file (the command)
                for (int i = 0; i < argc; i++)
                {
                    shapeMatchInfo << argv[i] << " ";
                }
                shapeMatchInfo << endl;

                // create new shape category with the first image as reference
                if (!createCategory(firstImgNum, 0, shapeMatchInfo))
                {
                    // Error
                    cout << "Error creating shape category " << 0 << endl;
                    return 1; 
                }

                // check if the two shapes match
                bool match = sameShape(firstImgNum, secondImgNum, 0, cutoffScore, shapeMatchInfo, true);
                if (match)
                {
                    // the shapes in the two images match
                    cout << "Match found: " << firstImgNum << " and " << secondImgNum << endl;
                }
                else
                {
                    // the shapes in the two images do not match
                    cout << "No match: " << firstImgNum << " and " << secondImgNum << endl;
                }
            }
            else
            {
                // error
                cout << "Text file to hold shape match information cannot be opened" << endl;
                return 1;
            }
        }
        else if (strcmp(argv[1], "-group") == 0)
        {
            // Calculate match scores for a set of images
            // Program usage: ./categorize -group (cutoff score) (first image #) (second image #) (etc.)
            if (argc < 5)
            {
                // error
                cout << "Program usage: ./categorize -group (cutoff score) (first image #) (second image #) (etc.)" << endl;
                return 1;
            }

            float cutoffScore = atof(argv[2]); // get the cutoff score for shape matches
            int refImageNum = atoi(argv[3]); // get the first frame number for an image and use that image as the category's reference image in order to calculate match scores 

            // create and open a text file to store categories of images
            ofstream categoryMatchInfo("categories/category_match.txt");
            if (categoryMatchInfo.is_open()) // make sure the text file can be opened
            {
                categoryMatchInfo << "Shape match information for: "; // save where the shape match information came from in the text file (the command)
                for (int i = 0; i < argc; i++)
                {
                    categoryMatchInfo << argv[i] << " ";
                }
                categoryMatchInfo << endl;

                // create new shape category with the first image as reference
                if (!createCategory(refImageNum, 0, categoryMatchInfo))
                {
                    // error
                    cout << "Error creating shape category " << 0 << endl;
                    return 1; 
                }

                for (int i = 4; i < argc; i++) // compare following images with the reference image #
                {
                    categoryMatchInfo << "Comparison with frame number " << argv[i] << endl;

                    // check if the two shapes match
                    bool match = sameShape(refImageNum, atoi(argv[i]), 0, cutoffScore, categoryMatchInfo, true);
                    if (match)
                    {
                        // the shapes in the two images match
                        cout << "Match found: " << refImageNum << " and " << argv[i] << endl;
                    }
                    else
                    {
                        // the shapes in the two images do not match
                        cout << "No match: " << refImageNum << " and " << argv[i] << endl;
                    }

                    categoryMatchInfo << endl; // spacer
                }
            }
            else
            {
                // error
                cout << "Text file to hold group shape match information cannot be opened" << endl;
                return 1;
            }
        } 
    }
    else
    {
        // error
        cout << "Program usage: ./categorize (-all or -specific) [specific flag parameters]" << endl;
        return 1;
    }
    return 0;    
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns true if the uncategorized image matches the reference image, false otherwise
// refFrameNum is the number of the reference image file
// uncategorizedFrameNum is the number of the uncategorized image file
// cutoffScore is the cutoff score for same shape matches; the value should be between 0 and 1
// shapeMatches is the text file to write image category info to
// allInfo specifies whether data from failed shape categorizations should be written to shapeMatches
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool sameShape(int refFrameNum, int uncategorizedFrameNum, int categoryNum, float cutoffScore, ofstream& shapeMatches, bool allInfo)
{
    // build reference image frame's filename
    char reference_buffer[256];
    sprintf(reference_buffer, "../shapes/frame.%06i.ppm", refFrameNum);

    // try reading the reference image's frame
    int refWidth, refHeight;
    unsigned char* reference_pixels = NULL;
    bool refReadSuccess = readPPM(reference_buffer, reference_pixels, refWidth, refHeight); // attempt to read the reference image 

    // build uncategorized image to compare it to's filename
    char img_buffer[256]; // the filename for the uncategorized image
    sprintf(img_buffer, "../shapes/frame.%06i.ppm", uncategorizedFrameNum);

    // try reading the next sequential frame as the image to categorize
    int imgWidth, imgHeight;
    unsigned char* img_pixels = NULL; // the pixel info for the uncategorized image
    bool imgReadSuccess = readPPM(img_buffer, img_pixels, imgWidth, imgHeight); // attempt to read the uncategorized image to compare it against the reference image

    int height = refHeight; // height of images to compare
    int width = refWidth; // width of images to compare

    // Make sure the reference image and the uncategorized image have the same dimensions
    if ((refWidth != imgWidth) || (refHeight != imgHeight))
    {
        // Error
        cout << "Error: reference image and uncategorized image have different dimensions." << endl;
        return false;
    }


    int refWhitePixels = 0; // total number of white pixels in the reference image
    int imgWhitePixels = 0; // total number of white pixels in the uncategorized image
    int refBlackPixels = 0; // total number of black pixels in the reference image
    int imgBlackPixels = 0; // total number of black pixels in the uncategorized image

    int pixelWhiteMatches = 0; // hold number of white pixel matches between the reference image and the image to categorize
    int reflectedPixelWhiteMatches = 0; // hold number of white pixel matches between the reference image and the image to categorize reflected across the y axis
    int possibleWhiteMatches = 0; // hold number of possible white pixel matches, either refWhitePixels or imgWhitePixels depending on which is larger; for categorization based on number of white pixel matches

    int pixelBlackMatches = 0; // hold number of black pixel matches between the reference image and the image to categorize
    int reflectedPixelBlackMatches = 0; // hold number of black pixel matches between the reference image and the image to categorize reflected across the y axis
    int possibleBlackMatches = 0; // hold number of possible black pixel matches, either refBlackPixels or imgBlackPixels depending on which is larger; for categorization based on number of black pixel matches
    
    if (refReadSuccess && imgReadSuccess) // successfully read reference image and uncategorized image
    {
        // iterate through every pixel in the image and see if the pixels in the reference image and the uncategorized image match up
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                int pixelIndex = x + (height - y) * width; // calculate pixel index for pixel values array that represents the final output image

                int reflectedPixelIndex = (width - x) + (height - y) * width; // calculate pixel index for pixel value that represents the image reflected across the y axis

                if (float(reference_pixels[3 * pixelIndex]) == 255.0)
                {
                    // white pixel in reference image
                    refWhitePixels += 1;
                }

                if (float(img_pixels[3 * pixelIndex]) == 255.0)
                {
                    // white pixel in uncategorized image
                    imgWhitePixels += 1;
                }

                if (float(reference_pixels[3 * pixelIndex]) == 0.0)
                {
                    // black pixel in reference image
                    refBlackPixels += 1;
                }

                if (float(img_pixels[3 * pixelIndex]) == 0.0)
                {
                    // black pixel in uncategorized image
                    imgBlackPixels += 1;
                }

                if ((float(reference_pixels[3 * pixelIndex]) == float(img_pixels[3 * pixelIndex])) && (float(reference_pixels[3 * pixelIndex + 1]) == float(img_pixels[3 * pixelIndex + 1])) && (float(reference_pixels[3 * pixelIndex + 2]) == float(img_pixels[3 * pixelIndex + 2])))
                {
                    // the red, green, and blue values of the pixel are the same, so the two images have the same value at that pixel
                    if ((reference_pixels[3 * pixelIndex] == 255.0) && (reference_pixels[3 * pixelIndex + 1] == 255.0) && (reference_pixels[3 * pixelIndex + 2] == 255.0))
                    {
                        pixelWhiteMatches += 1; // white pixel, for categorization based on number of white pixel matches
                    }

                    // the red, green, and blue values of the pixel are the same, so the two images have the same value at that pixel
                    if ((reference_pixels[3 * pixelIndex] == 0.0) && (reference_pixels[3 * pixelIndex + 1] == 0.0) && (reference_pixels[3 * pixelIndex + 2] == 0.0))
                    {
                        pixelBlackMatches += 1; // black pixel, for categorization based on number of black pixel matches
                    }
                }

                if ((float(reference_pixels[3 * pixelIndex]) == float(img_pixels[3 * reflectedPixelIndex])) && (float(reference_pixels[3 * pixelIndex + 1]) == float(img_pixels[3 * reflectedPixelIndex + 1])) && (float(reference_pixels[3 * pixelIndex + 2]) == float(img_pixels[3 * reflectedPixelIndex + 2])))
                {
                    // the red, green, and blue values of the reference image pixel and the reflected uncategorized image pixel are the same, same value at those corresponding pixels
                    if ((reference_pixels[3 * pixelIndex] == 255.0) && (reference_pixels[3 * pixelIndex + 1] == 255.0) && (reference_pixels[3 * pixelIndex + 2] == 255.0))
                    {
                        reflectedPixelWhiteMatches += 1; // white pixel, for categorization based on number of white pixel matches
                    }

                    // the red, green, and blue values of the reference image pixel and the reflected uncategorized image pixel are the same, same value at those corresponding pixels
                    if ((reference_pixels[3 * pixelIndex] == 0.0) && (reference_pixels[3 * pixelIndex + 1] == 0.0) && (reference_pixels[3 * pixelIndex + 2] == 0.0))
                    {
                        reflectedPixelBlackMatches += 1; // black pixel, for categorization based on number of black pixel matches
                    }
                }
            }
        }
        
        // determine categorization based on 0.5 * (number of white pixel matches / total possible white pixel matches) + 0.5 * (number of black pixel matches / total possible black pixel matches)
        possibleWhiteMatches = refWhitePixels;
        if (imgWhitePixels > refWhitePixels)
        {
            possibleWhiteMatches = imgWhitePixels; // get the largest number of white pixels, either the amount from the reference image or the uncategorized image
        }

        possibleBlackMatches = refBlackPixels;
        if (imgBlackPixels > refBlackPixels)
        {
            possibleBlackMatches = imgBlackPixels; // get the largest number of black pixels, either the amount from the reference image or the uncategorized image
        }

        float whiteRatio = float(pixelWhiteMatches) / float(possibleWhiteMatches); // get the ratio of how many pixels matched up between the two images compared to the total number of pixels; only white pixel matches
        float blackRatio = float(pixelBlackMatches) / float(possibleBlackMatches); // get the ratio of how many pixels matched up between the two images compared to the total number of pixels; only black pixel matches
        float ratio = (0.5 * whiteRatio) + (0.5 * blackRatio); // calculate overall ratio of matches by evenly weighting the ratio of white pixel matches and the ratio of black pixel matches
        // float ratio = whiteRatio; 

        float whiteReflectedRatio = float(reflectedPixelWhiteMatches) / float(possibleWhiteMatches); // get the ratio of how many pixels matched up between the reference image and the reflected uncategorized image compared to the total number of pixels; only white pixel matches
        float blackReflectedRatio = float(reflectedPixelBlackMatches) / float(possibleBlackMatches); // get the ratio of how many pixels matched up between the reference image and the reflected uncategorized image compared to the total number of pixels; only black pixel matches
        float reflectedRatio = (0.5 * whiteReflectedRatio) + (0.5 * blackReflectedRatio); // calculate overall ratio of matches by evenly weighting the ratio of reflected white pixel matches and the ratio of reflected black pixel matches
        // float reflectedRatio = whiteReflectedRatio; 

        if (ratio > cutoffScore || reflectedRatio > cutoffScore) // will need to adjust ratio
        {
            // this image is part of the reference image's category
            // build copy of uncategorized image frame's filename
            char uncategorized_copy[256]; // filename for the copy of the uncategorized image that is now categorized in the same category as the reference image
            sprintf(uncategorized_copy, "categories/shape.%03i/frame.%06i.ppm", categoryNum, uncategorizedFrameNum);
            writePPM(uncategorized_copy, width, height, img_pixels); // make a copy of the newly categorized image and place it in the shape category's folder

            shapeMatches << img_buffer << " "; // save the categorized image
            if (ratio > cutoffScore)
            {
                // reference image and uncategorized image match
                shapeMatches << "with ratio: " << ratio << ", whiteRatio: " << whiteRatio << ", blackRatio: " << blackRatio << endl; // save the scores for the match
                // shapeMatches << "with reflectedRatio: " << reflectedRatio << ", whiteReflectedRatio: " << whiteReflectedRatio << ", blackReflectedRatio: " << blackReflectedRatio << endl; // save the scores for the match
            }
            else if (reflectedRatio > cutoffScore)
            {
                // reference image and reflected uncategorized image match
                shapeMatches << "with reflectedRatio: " << reflectedRatio << ", whiteReflectedRatio: " << whiteReflectedRatio << ", blackReflectedRatio: " << blackReflectedRatio << endl; // save the scores for the match with uncategorized image reflection
            }
            delete[] reference_pixels; // clean up
            delete[] img_pixels; // clean up
            return true;
        }
        else 
        {
            // images don't match
            if (allInfo)
            {
                // write failed shape categorization info to text file
                shapeMatches << "with ratio: " << ratio << ", whiteRatio: " << whiteRatio << ", blackRatio: " << blackRatio << endl; // save the scores for the failed match
                shapeMatches << "with reflectedRatio: " << reflectedRatio << ", whiteReflectedRatio: " << whiteReflectedRatio << ", blackReflectedRatio: " << blackReflectedRatio << endl; // save the scores for the failed match with uncategorized image reflection

                // for error checking to make sure the right images are being read in
                // build copy of uncategorized image frame's filename
                char uncategorized_copy[256]; // filename for the copy of the uncategorized image that is now categorized in the same category as the reference image
                sprintf(uncategorized_copy, "categories/shape.%03i/frame.%06i.ppm", categoryNum, uncategorizedFrameNum);
                writePPM(uncategorized_copy, width, height, img_pixels); // make a copy of the newly categorized image and place it in the shape category's folder
            }
            delete[] reference_pixels; // clean up
            delete[] img_pixels; // clean up
            return false; 
        }
    }
    else
    {
        // error
        cout << "Error reading images to compare" << endl;
        delete[] reference_pixels; // clean up
        delete[] img_pixels; // clean up
        return false;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns true if the shape in the image is considered a shape to filter out white sprinkles that aren't visually interesting
// frameNum is the image number with the shape in consideration
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool consideredShape(int frameNum)
{
    // build reference image frame's filename
    char reference_buffer[256];
    sprintf(reference_buffer, "../shapes/frame.%06i.ppm", frameNum);

    // try reading the reference image's frame
    int width, height;
    unsigned char* reference_pixels = NULL;
    bool referenceReadSuccess = readPPM(reference_buffer, reference_pixels, width, height); // attempt to read the reference image 
    if (!referenceReadSuccess)
    {
        // error
        cout << "Failed to read " << reference_buffer << endl;
        return false; // false for not considered a shape since it can't be read
    }

    int numWhitePixels = 0; // number of white pixels in the image
    int totalPixels = width * height; // total number of pixels in the image
    for (int k = 0; k < width; k++) // iterate through image array to count number of white pixels
    {
        for (int l = 0; l < height; l++)
        {
            int pixelIndex = k + (height - l) * width; // calculate pixel index in image
            if ((float(reference_pixels[3 * pixelIndex]) == 255.0) && (float(reference_pixels[3 * pixelIndex + 1]) == 255.0) && (float(reference_pixels[3 * pixelIndex + 2]) == 255.0)) 
            {
                // white pixel in image
                numWhitePixels += 1;
            }
        }
    }
    float whiteRatio = float(numWhitePixels) / float(totalPixels); // get ratio of white pixels to all pixels in the image
    if (whiteRatio < 0.001)
    {
        // consider the image as a sprinkle of white pixels with no real shape
        // add it to shape0 category (the shapeless category)
        // build copy of reference image frame's filename
        char reference_copy[256]; // the filename for the copy of the reference image
        sprintf(reference_copy, "categories/shape.%03i/frame.%06i.ppm", 0, frameNum);
        writePPM(reference_copy, width, height, reference_pixels); // make a copy of the reference image and place it in the folder for the new shape category
        delete[] reference_pixels; // clean up
        return false;
    }
    else
    {
        // the shape in the image is an interesting shape
        delete[] reference_pixels; // clean up
        return true;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns true if new category creation is successful, false if encountered error
// frameNum is the frame number of the image to use as the reference image
// numShapeCategories is the number used for the new shape category, since index 0
// shapeMatches is the text file to write image category info to
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool createCategory(int frameNum, int numShapeCategories, ofstream& shapeMatches)
{
    // build reference image frame's filename
    char reference_buffer[256];
    sprintf(reference_buffer, "../shapes/frame.%06i.ppm", frameNum);

    // try reading the reference image's frame
    int width, height;
    unsigned char* reference_pixels = NULL;
    bool referenceReadSuccess = readPPM(reference_buffer, reference_pixels, width, height); // attempt to read the reference image 
    if (!referenceReadSuccess)
    {
        // error
        cout << "Failed to read " << reference_buffer << endl;
        delete[] reference_pixels; // clean up
        return false; // false for not considered a shape since it can't be read
    }

    char folder_name_buffer[256]; // hold folder name for the new shape category
    sprintf(folder_name_buffer, "categories/shape.%03i", numShapeCategories);
    int createFolderSuccess = mkdir(folder_name_buffer, S_IRUSR | S_IWUSR | S_IXUSR); // create a folder for this new shape category; returns 0 if successful, -1 otherwise

    if (createFolderSuccess != 0)
    {
        // Error
        cout << "Error creating shape category folder " << folder_name_buffer << endl;
    }

    shapeMatches << folder_name_buffer << endl; // save the shape category folder in the text file to mark a new shape category

    // build copy of reference image frame's filename
    char reference_copy[256]; // the filename for the copy of the reference image
    sprintf(reference_copy, "categories/shape.%03i/frame.%06i.ppm", numShapeCategories, frameNum);
    writePPM(reference_copy, width, height, reference_pixels); // make a copy of the reference image and place it in the folder for the new shape category

    shapeMatches << "Reference image: " << reference_copy << endl; // save the shape category reference image filename in the text file

    delete[] reference_pixels; // clean up
    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns true if categorization is successful, false otherwise
// totalFrames is the number of total frames to go through
// imageCategories is the text file to write image category info to
// cutoffScore is the cutoff score for same shape matches; the value should be between 0 and 1
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool categorizeAll(int totalFrames, ofstream& imageCategories, float cutoffScore)
{
    int numShapeCategories = 0; // number of shape categories

    // create a map that stores whether an image has been categorized indexed by image name
    map<int, bool> allImages; // key is the integer # of the frame and value is whether it has been categorized (true if categorized, false if uncategorized)
    for (int i = 0; i < totalFrames; i++)
    {
        allImages[i] = false; // initialize all images in map to uncategorized (set to false)
    }

    char shapeless_folder_buffer[256]; // hold folder name for the shapeless shape category (holding images with sprinkles of white pixels)
    sprintf(shapeless_folder_buffer, "categories/shape.%03i", 0); // consider as shape0
    if (mkdir(shapeless_folder_buffer, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
    {
        // create a folder for this new shape category; returns 0 if successful, -1 otherwise
        // error if != 0
        cout << "Error creating " << shapeless_folder_buffer << endl;
        return false;
    }

    float sameCutoff = cutoffScore; // if ratio between images is greater than sameCutoff, then they are categorized as the same shape; otherwise, they are categorized as different shapes
    imageCategories << "Match cutoff score: " << sameCutoff << endl; // save the cutoff score for matches in the categorization text file

    for (int i = 0; i < totalFrames; i++)
    {
        if (allImages[i] == false) // go to the next uncategorized image and set that as the reference image for a new shape category
        {
            bool shapeExists = consideredShape(i); // see if the image has an interesting shape in it; true if there is a shape, false if it's just some white sprinkles
            if (!shapeExists)
            {
                allImages[i] = true; // set this image as categorized as it's already copied into category0, the no-shape category
                continue; // done categorizing this image
            }

            numShapeCategories += 1; // increment number of shape categories
            // create new shape category with this image as reference
            if (!createCategory(i, numShapeCategories, imageCategories))
            {
                // Error
                cout << "Error creating shape category " << numShapeCategories << endl;
                return false; 
            }
            int numShapesInCategory = 1; // hold the number of shapes in this category, initialize to 1 since the category's reference image counts as an image in the category
            allImages[i] = true; // remember that this image has been categorized

            // use that image as the reference image and go through all remaining uncategorized images
            for (int j = i; j < totalFrames; j++)
            {
                if (allImages[j] == false) // only go through remaining uncategorized images
                {
                    // compare two images and get either match or no match, passing in text file name, file number 1 and file number 2
                    // returns whether the two frames match or not
                    if (sameShape(i, j, numShapeCategories, sameCutoff, imageCategories, false))
                    {
                        allImages[j] = true; // remember that this image has been categorized
                        numShapesInCategory += 1; // increment the number of shapes in this category
                        // imageCategories << folder_name_buffer << ": " << i << " and " << j << " with ratio: " << ratio << ", reflectedRatio: " << reflectedRatio << ", reference image had possibleWhiteMatches: " << possibleWhiteMatches << endl; // save the category pair in a text file for now
                    }
                }
            }

            imageCategories << "Number of shapes in category " << numShapeCategories << ": " << numShapesInCategory << endl; // remember number of shapes in this category
            imageCategories << endl; // spacer
        }
    }

    imageCategories.close(); // close file after done writing
    return true;
}