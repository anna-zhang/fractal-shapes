#include <cmath>
#include <cstdio>
#include "FIELD_2D.h"
#include "VEC3F.h"

#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif

#if _WIN32
#include <gl/glut.h>
#elif __APPLE__
#include <GLUT/glut.h>
#endif

#include <iostream>
#include <fstream>
#include "QUICKTIME_MOVIE.h"

using namespace std;

const int totalTop = 15; // total top roots
const int totalBottom = 15; // total bottom roots

int currentTop = 1; // actual number of top roots
int currentBottom = 0; // actual number of bottom roots

// VEC3F topRoots[totalTop]; // array to hold top roots
// VEC3F bottomRoots[totalBottom]; // array to hold bottom roots

// resolution of the field
int xRes = 800;
int yRes = 800;

// the field being drawn and manipulated
FIELD_2D field(xRes, yRes);

// the resolution of the OpenGL window -- independent of the field resolution
//int xScreenRes = 700;
//int yScreenRes = 700;
int xScreenRes = 500;
int yScreenRes = 500;

// Text for the title bar of the window
string windowLabel("Field Viewer");

// mouse tracking variables
int xMouse         = -1;
int yMouse         = -1;
int mouseButton    = -1;
int mouseState     = -1;
int mouseModifiers = -1;

// current grid cell the mouse is pointing at
int xField = -1;
int yField = -1;

// animate the current runEverytime()?
bool animate = false;

// draw the grid over the field?
bool drawingGrid = false;

// print out what the mouse is pointing at?
bool drawingValues = false;

// currently capturing frames for a movie?
bool captureMovie = false;

// the current viewer eye position
VEC3F eyeCenter(0.5, 0.5, 1);

// current zoom level into the field
float zoom = 1.0;

// Quicktime movie to capture to
QUICKTIME_MOVIE movie;

// forward declare the caching function here so that we can
// put it at the bottom of the file
void runOnce();

// forward declare the timestepping function here so that we can
// put it at the bottom of the file
void runEverytime();

vector<VEC3F> topRoots;

///////////////////////////////////////////////////////////////////////
// Figure out which field element is being pointed at, set xField and
// yField to them
///////////////////////////////////////////////////////////////////////
void refreshMouseFieldIndex(int x, int y)
{
  // make the lower left the origin
  y = yScreenRes - y;

  float xNorm = (float)x / xScreenRes;
  float yNorm = (float)y / yScreenRes;

  float halfZoom = 0.5 * zoom;
  float xWorldMin = eyeCenter[0] - halfZoom;
  float xWorldMax = eyeCenter[0] + halfZoom;

  // get the bounds of the field in screen coordinates
  //
  // if non-square textures are ever supported, change the 0.0 and 1.0 below
  float xMin = (0.0 - xWorldMin) / (xWorldMax - xWorldMin);
  float xMax = (1.0 - xWorldMin) / (xWorldMax - xWorldMin);

  float yWorldMin = eyeCenter[1] - halfZoom;
  float yWorldMax = eyeCenter[1] + halfZoom;

  float yMin = (0.0 - yWorldMin) / (yWorldMax - yWorldMin);
  float yMax = (1.0 - yWorldMin) / (yWorldMax - yWorldMin);

  float xScale = 1.0;
  float yScale = 1.0;

  if (xRes < yRes)
    xScale = (float)yRes / xRes;
  if (xRes > yRes)
    yScale = (float)xRes / yRes;

  // index into the field after normalizing according to screen
  // coordinates
  xField = xScale * xRes * ((xNorm - xMin) / (xMax - xMin));
  yField = yScale * yRes * ((yNorm - yMin) / (yMax - yMin));

  // clamp to something inside the field
  xField = (xField < 0) ? 0 : xField;
  xField = (xField >= xRes) ? xRes - 1 : xField;
  yField = (yField < 0) ? 0 : yField;
  yField = (yField >= yRes) ? yRes - 1 : yField;
}

///////////////////////////////////////////////////////////////////////
// Print a string to the GL window
///////////////////////////////////////////////////////////////////////
void printGlString(string output)
{
  glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
  for (unsigned int x = 0; x < output.size(); x++)
    glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, output[x]);
}

///////////////////////////////////////////////////////////////////////
// dump the field contents to a GL texture for drawing
///////////////////////////////////////////////////////////////////////
void updateTexture(FIELD_2D& texture)
{
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, 
      texture.xRes(), 
      texture.yRes(), 0, 
      GL_LUMINANCE, GL_FLOAT, 
      texture.data());

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glEnable(GL_TEXTURE_2D);
}

///////////////////////////////////////////////////////////////////////
// draw a grid over everything
///////////////////////////////////////////////////////////////////////
void drawGrid()
{
  glColor4f(0.1, 0.1, 0.1, 1.0);

  float dx = 1.0 / xRes;
  float dy = 1.0 / yRes;

  if (xRes < yRes)
    dx *= (float)xRes / yRes;
  if (xRes > yRes)
    dy *= (float)yRes / xRes;

  glBegin(GL_LINES);
  for (int x = 0; x < field.xRes() + 1; x++)
  {
    glVertex3f(x * dx, 0, 1);
    glVertex3f(x * dx, 1, 1);
  }
  for (int y = 0; y < field.yRes() + 1; y++)
  {
    glVertex3f(0, y * dy, 1);
    glVertex3f(1, y * dy, 1);
  }
  glEnd();
}

///////////////////////////////////////////////////////////////////////
// GL and GLUT callbacks
///////////////////////////////////////////////////////////////////////
void glutDisplay()
{
  // Make ensuing transforms affect the projection matrix
  glMatrixMode(GL_PROJECTION);

  // set the projection matrix to an orthographic view
  glLoadIdentity();
  float halfZoom = zoom * 0.5;

  glOrtho(-halfZoom, halfZoom, -halfZoom, halfZoom, -10, 10);

  // set the matrix mode back to modelview
  glMatrixMode(GL_MODELVIEW);

  // set the lookat transform
  glLoadIdentity();
  gluLookAt(eyeCenter[0], eyeCenter[1], 1,  // eye
            eyeCenter[0], eyeCenter[1], 0,  // center 
            0, 1, 0);   // up

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  float xLength = 1.0;
  float yLength = 1.0;

  if (xRes < yRes)
    xLength = (float)xRes / yRes;
  if (yRes < xRes)
    yLength = (float)yRes / xRes;

  glEnable(GL_TEXTURE_2D); 
  glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(0.0, 0.0, 0.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(0.0, yLength, 0.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(xLength, yLength, 0.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(xLength, 0.0, 0.0);
  glEnd();
  glDisable(GL_TEXTURE_2D); 

  // draw the grid, but only if the user wants
  if (drawingGrid)
    drawGrid();

  // if there's a valid field index, print it
  if (xField >= 0 && yField >= 0 &&
      xField < field.xRes() && yField < field.yRes())
  {
    glLoadIdentity();

    // must set color before setting raster position, otherwise it won't take
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

    // normalized screen coordinates (-0.5, 0.5), due to the glLoadIdentity
    float halfZoom = 0.5 * zoom;
    glRasterPos3f(-halfZoom* 0.95, -halfZoom* 0.95, 0);

    // build the field value string
    char buffer[256];
    string fieldValue("(");
    sprintf(buffer, "%i", xField);
    fieldValue = fieldValue + string(buffer);
    sprintf(buffer, "%i", yField);
    fieldValue = fieldValue + string(", ") + string(buffer) + string(") = ");
    sprintf(buffer, "%f", field(xField, yField));
    fieldValue = fieldValue + string(buffer);

    // draw the grid, but only if the user wants
    if (drawingValues)
      printGlString(fieldValue);
  }

  // if we're recording a movie, capture a frame
  if (captureMovie)
    movie.addFrameGL();

  glutSwapBuffers();
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void printCommands()
{
  cout << "=============================================================== " << endl;
  cout << " Field viewer code for MAT 200C " << endl;
  cout << "=============================================================== " << endl;
  cout << " q           - quit" << endl;
  cout << " v           - type the value of the cell under the mouse" << endl;
  cout << " g           - throw a grid over everything" << endl;
  cout << " m           - start/stop capturing a movie" << endl;
  cout << " r           - read in a PNG file " << endl;
  cout << " w           - write out a PNG file " << endl;
  cout << " left mouse  - pan around" << endl;
  cout << " right mouse - zoom in and out " << endl;
  cout << " shift left mouse - draw on the grid " << endl;
}

///////////////////////////////////////////////////////////////////////
// Map the arrow keys to something here
///////////////////////////////////////////////////////////////////////
void glutSpecial(int key, int x, int y)
{
  switch (key)
  {
    case GLUT_KEY_LEFT:
      break;
    case GLUT_KEY_RIGHT:
      break;
    case GLUT_KEY_UP:
      break;
    case GLUT_KEY_DOWN:
      break;
    default:
      break;
  }
}

///////////////////////////////////////////////////////////////////////
// Map the keyboard keys to something here
///////////////////////////////////////////////////////////////////////
void glutKeyboard(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 'a':
      animate = !animate;
      break;
    case 'g':
      drawingGrid = !drawingGrid;
      break;
    case '?':
      printCommands();
      break;
    case 'v':
      drawingValues = !drawingValues;
      break;
    case 'm':
      // if we were already capturing a movie
      if (captureMovie)
      {
        // write out the movie
        movie.writeMovie("movie.mov");

        // reset the movie object
        movie = QUICKTIME_MOVIE();

        // stop capturing frames
        captureMovie = false;
      }
      else
      {
        cout << " Starting to capture movie. " << endl;
        captureMovie = true;
      }
      break;
    case 'r':
      field.readPNG("bunny.png");
      xRes = field.xRes();
      yRes = field.yRes();
      break;
    case 'w':
      field.writePNG("output.png");
      break;
    case 'q':
      exit(0);
      break;
    default:
      break;
  }
}

///////////////////////////////////////////////////////////////////////
// Do something if the mouse is clicked
///////////////////////////////////////////////////////////////////////
void glutMouseClick(int button, int state, int x, int y)
{
  int modifiers = glutGetModifiers();
  mouseButton = button;
  mouseState = state;
  mouseModifiers = modifiers;

  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && modifiers & GLUT_ACTIVE_SHIFT)
  {
    // figure out which cell we're pointing at
    refreshMouseFieldIndex(x,y);
    
    // set the cell
    field(xField, yField) = 1;
    
    // make sure nothing else is called
    return;
  }

  xMouse = x;  
  yMouse = y;
}

///////////////////////////////////////////////////////////////////////
// Do something if the mouse is clicked and moving
///////////////////////////////////////////////////////////////////////
void glutMouseMotion(int x, int y)
{
  if (mouseButton == GLUT_LEFT_BUTTON && 
      mouseState == GLUT_DOWN && 
      mouseModifiers & GLUT_ACTIVE_SHIFT)
  {
    // figure out which cell we're pointing at
    refreshMouseFieldIndex(x,y);
    
    // set the cell
    field(xField, yField) = 1;
    
    // make sure nothing else is called
    return;
  }

  float xDiff = x - xMouse;
  float yDiff = y - yMouse;
  float speed = 0.001;
  
  if (mouseButton == GLUT_LEFT_BUTTON) 
  {
    eyeCenter[0] -= xDiff * speed;
    eyeCenter[1] += yDiff * speed;
  }
  if (mouseButton == GLUT_RIGHT_BUTTON)
    zoom -= yDiff * speed;

  xMouse = x;
  yMouse = y;
}

///////////////////////////////////////////////////////////////////////
// Do something if the mouse is not clicked and moving
///////////////////////////////////////////////////////////////////////
void glutPassiveMouseMotion(int x, int y)
{
  refreshMouseFieldIndex(x,y);
}

///////////////////////////////////////////////////////////////////////
// animate and display new result
///////////////////////////////////////////////////////////////////////
void glutIdle()
{
  if (animate)
  {
    runEverytime();
  }
  updateTexture(field);
  glutPostRedisplay();
}

//////////////////////////////////////////////////////////////////////////////
// open the GLVU window
//////////////////////////////////////////////////////////////////////////////
int glvuWindow()
{
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE| GLUT_RGBA);
  glutInitWindowSize(xScreenRes, yScreenRes); 
  glutInitWindowPosition(10, 10);
  glutCreateWindow(windowLabel.c_str());

  // set the viewport resolution (w x h)
  glViewport(0, 0, (GLsizei) xScreenRes, (GLsizei) yScreenRes);

  // set the background color to gray
  glClearColor(0.1, 0.1, 0.1, 0);

  // register all the callbacks
  glutDisplayFunc(&glutDisplay);
  glutIdleFunc(&glutIdle);
  glutKeyboardFunc(&glutKeyboard);
  glutSpecialFunc(&glutSpecial);
  glutMouseFunc(&glutMouseClick);
  glutMotionFunc(&glutMouseMotion);
  glutPassiveMotionFunc(&glutPassiveMouseMotion);

  // enter the infinite GL loop
  glutMainLoop();

  // Control flow will never reach here
  return EXIT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// Do a complex multiply
///////////////////////////////////////////////////////////////////////
VEC3F complexMultiply(VEC3F left, VEC3F right)
{
  float a = left[0];
  float b = left[1];
  float c = right[0];
  float d = right[1];
  return VEC3F(a * c - b * d, a * d + b * c, 0.0);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void writePPM(const string &filename, int &xRes, int &yRes, const float *values)
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

//////////////////////////////////////////////////////////////////////////////////
// Returns true if there is a shape in the image
//////////////////////////////////////////////////////////////////////////////////
bool renderImage(int &xRes, int &yRes, const string &filename, int frame_num)
{
  // allocate the final image
  const int totalCells = xRes * yRes;
  float *ppmOut = new float[3 * totalCells];

  float xLength = 4.5;
  float yLength = 4.5;

  int maxIterations = 100;
  float escapeRadius = 200.0; // to match the js version

  float dx = xLength / xRes;
  float dy = yLength / yRes;
  float xHalf = xLength * 0.5;
  float yHalf = yLength * 0.5;

  // get the pixel values of the roots, changing from x[-2 x 2], y[-2 x 2] to [xRes x yRes]
  float root1x_pixel = (topRoots[0][0] + 2.0f) * (xRes / 4.0f);
  float root1y_pixel = (topRoots[0][1] + 2.0f) * (yRes / 4.0f);
  float root2x_pixel = (topRoots[1][0] + 2.0f) * (xRes / 4.0f);
  float root2y_pixel = (topRoots[1][1] + 2.0f) * (yRes / 4.0f);

  // cout << "root1x_pixel: " << root1x_pixel << endl;
  // cout << "root1y_pixel: " << root1y_pixel << endl;
  // cout << "root2x_pixel: " << root2x_pixel << endl;
  // cout << "root2y_pixel: " << root2y_pixel << endl;

  bool shape = false;

  // cout << " Row: "; flush(cout);
  for (int y = 0; y < yRes; y++)
  {
    // cout << y << " "; flush(cout);
    for (int x = 0; x < xRes; x++)
    {
      // getting the center coordinate here is a little sticky
      VEC3F center;
      center[0] = -xHalf + x * dx;
      center[1] = -yHalf + y * dy;

      VEC3F iterate = center; // iterate is q
      VEC3F p; // hold calculated polynomial

      float magnitude = iterate.magnitude();
      int totalIterations = 0;
      while (magnitude < escapeRadius && totalIterations < maxIterations)
      {
        VEC3F g = VEC3F(1.0, 0.0, 0.0); // holds current polynomial on top
        VEC3F diff;

        // compute the top: iterate through top roots
        for (int x = 0; x < totalTop; x++)
        {
          if (x == currentTop) 
          {
            break;
          }
          // add (q-root) onto g, the polynomial on the top
          diff = (iterate - topRoots[x]);
          g = complexMultiply(g, diff);
        }

        // compute the polynomial
        p = g;
        iterate = p;

        magnitude = iterate.magnitude();
        totalIterations++;

        // exit conditions
        if (magnitude > escapeRadius)
          break;
        if (magnitude < 1e-7)
          break;
      }

      // cout << "totalIterations: " << totalIterations << endl;
      int pixelIndex = x + (yRes - y) * xRes; // calculate pixel index for pixel values array that represents the final output image

      // color accordingly
      if (totalIterations == maxIterations)
      {
        // cout << "white" << endl;
        field(x, y) = 1.0; // did not escape, color white
        // set, in final image
        ppmOut[3 * pixelIndex] = 255.0f;
        ppmOut[3 * pixelIndex + 1] = 255.0f;
        ppmOut[3 * pixelIndex + 2] = 255.0f;
        shape = true; // there is a fractal shape
      }
      else
      {
        // cout << "black" << endl;
        field(x, y) = 0.0; // escaped, color black
        // set, in final image
        ppmOut[3 * pixelIndex] = 0.0f;
        ppmOut[3 * pixelIndex + 1] = 0.0f;
        ppmOut[3 * pixelIndex + 2] = 0.0f;
      }

      // red square around roots to be able to see root positions
      if (((x > (root1x_pixel - 10.0f)) && (x < (root1x_pixel + 10.0f)) && (y > (root1y_pixel - 10.0f)) && (y < (root1y_pixel + 10.0f))) || ((x > (root2x_pixel - 10.0f)) && (x < (root2x_pixel + 10.0f)) && (y > (root2y_pixel - 10.0f)) && (y < (root2y_pixel + 10.0f))))
      {
        // cout << "near root" << endl;
        ppmOut[3 * pixelIndex] = 255.0f;
        ppmOut[3 * pixelIndex + 1] = 0.0f;
        ppmOut[3 * pixelIndex + 2] = 0.0f;
      }
    }
  }

  if (shape)
  { // fractal shape exists
    writePPM(filename, xRes, yRes, ppmOut); // output fractal shape
    delete[] ppmOut;
    return true;
  }
  else
  { // no fractal shape
    delete[] ppmOut;
    return false;
  }
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{

  // every root on screen is between x[-2.0, 2.0], y[-2.0, 2.0]
  // top right is (2.0, 2.0), bottom right is (2.0, -2.0), bottom left is (-2.0, 2.0), top left is (-2.0, 2.0)

  // detect mode
  int mode = 0; // 0 for full space exploration, 1 for single exploration; default full space exploration
  if (argc > 1)
  {
    if (strcmp(argv[1], "-single") == 0)
    {
      mode = 1; // set to single exploration mode
    }
  }
  
  if (mode == 1) // single exploration
  {
    // format: ./mandelbrot; -single (# of top roots n) (root0_x) (root0_y) ... (ootn-1_x) (rootn-1_y)
    int num_top_roots = atoi(argv[2]);
    currentTop = num_top_roots; // number of roots
    // cout << "num_top_roots: " << num_top_roots << endl;
    // cout << "currentTop: " << currentTop << endl;
    for (int i = 0; i < num_top_roots; i++)
    {
      topRoots.push_back(VEC3F(atof(argv[2 * i + 3]), atof(argv[2 * i + 4]), 0.0));
      // cout << "topRoots" << i << ": " << topRoots[i] << endl; 
    }
    // compute fractal
    runOnce();

    // initialize GLUT and GL
    glutInit(&argc, argv);

    // open the GL window
    glvuWindow();
  }
  else // full space exploration
  {
    // create and open a text file to store root information alongside output image file names
    ofstream rootInfo("shapes/root_info.txt");

    int gridSize_x = 8; // default grid size if not specified by user
    int gridSize_y = 8; // default grid size if not specified by user

    // format: ./mandelbrot -full (grid size x) (grid size y)
    if (argc == 4)
    {
      // set grid size to be user-specified
      gridSize_x = atoi(argv[2]); 
      gridSize_y = atoi(argv[3]);
    }

    // In case the field is rectangular, make sure to center the eye
    if (xRes < yRes)
    {
      float xLength = (float)xRes / yRes;
      eyeCenter[0] = xLength * 0.5;
    }
    if (yRes < xRes)
    {
      float yLength = (float)yRes / xRes;
      eyeCenter[1] = yLength * 0.5;
    }

    if (rootInfo.is_open()) // make sure the text file can be opened
    {
      // two root case, 7x7 interior grid
      // iterate root 0 from top to bottom
      int image_num = 0; // current output image number
      currentTop = 2; // # of roots
      for (float root0_y = 2.0; root0_y >= 0; root0_y -= 4.0/gridSize_y)
      {
        // iterate root 0 from left to right
        for (float root0_x = -2.0; root0_x <= 2.0; root0_x += 4.0/gridSize_x)
        {
          // iterate root 1 from top to bottom
          for (float root1_y = root0_y; root1_y >= 0; root1_y -= 4.0/gridSize_y)
          {
            float root0_x_start = 0;
            if (root1_y == root0_y)
            {
              root0_x_start = root0_x;
            }
            else
            {
              root0_x_start = -2.0;
            }
            // iterate root 1 from left to right
            for (float root1_x = root0_x_start; root1_x <= 2.0; root1_x += 4.0/gridSize_x)
            {
              char buffer[256]; // hold location to put image file
              sprintf(buffer, "./shapes/frame.%06i.ppm", image_num);

              topRoots.clear(); // clear from last iteration
              topRoots.push_back(VEC3F(root0_x, root0_y, 0.0));
              topRoots.push_back(VEC3F(root1_x, root1_y, 0.0));

              bool shape = renderImage(xRes, yRes, buffer, image_num); // compute shape, if any
              if (shape)
              { // only save root information if shape exists
                rootInfo << buffer << ": " << "topRoots0" << topRoots[0] << ", topRoots1" << topRoots[1] << endl; // write root info to text file
                image_num++;
              }
            }
          }
        }
      }
      rootInfo.close(); // close file after done writing
    }
    else 
    {
      cout << "Unable to open file." << endl;
      return 0;
    }
  }

  return 1;
}

///////////////////////////////////////////////////////////////////////
// This function is called every frame -- do something interesting
// here.
///////////////////////////////////////////////////////////////////////
void runEverytime()
{
}

///////////////////////////////////////////////////////////////////////
// This is called once at the beginning so you can precache
// something here
///////////////////////////////////////////////////////////////////////
void runOnce()
{
  float xLength = 4.5;
  float yLength = 4.5;

  int maxIterations = 100;
  float escapeRadius = 200.0; // to match the js version

  float dx = xLength / xRes;
  float dy = yLength / yRes;
  float xHalf = xLength * 0.5;
  float yHalf = yLength * 0.5;

  cout << " Row: "; flush(cout);
  for (int y = 0; y < yRes; y++)
  {
    cout << y << " "; flush(cout);
    for (int x = 0; x < xRes; x++)
    {
      // getting the center coordinate here is a little sticky
      VEC3F center;
      center[0] = -xHalf + x * dx;
      center[1] = -yHalf + y * dy;

      VEC3F iterate = center; // iterate is q
      VEC3F p; // hold calculated polynomial

      float magnitude = iterate.magnitude();
      int totalIterations = 0;
      while (magnitude < escapeRadius && totalIterations < maxIterations)
      {
        VEC3F g = VEC3F(1.0, 0.0, 0.0); // holds current polynomial on top
        VEC3F diff;

        // compute the top: iterate through top roots
        for (int x = 0; x < totalTop; x++)
        {
          if (x == currentTop) 
          {
            break;
          }
          // add (q-root) onto g, the polynomial on the top
          diff = (iterate - topRoots[x]);
          g = complexMultiply(g, diff);
        }

        // compute the polynomial
        p = g;
        iterate = p;

        magnitude = iterate.magnitude();
        totalIterations++;

        // exit conditions
        if (magnitude > escapeRadius)
          break;
        if (magnitude < 1e-7)
          break;
      }

      // color accordingly
      if (totalIterations == maxIterations)
      {
        field(x, y) = 1.0; // did not escape, color white
      }
      else
      {
        field(x, y) = 0.0; // escaped, color black
      }
    }
  }
}
