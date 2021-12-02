# Fractal Shapes
A tool to explore and categorize two root polynomials

# Usage - CLI
## Modes for exploring two root polynomial:
- **Single shape exploration:** user inputs locations of roots and creates an OpenGL window to preview the shape generated <br/>
```./mandelbrot -single (# of top roots n) (root0_x) (root0_y) … (rootn-1_x) (rootn-1_y)```
- **Random exploration:** generates (# of images to generate) images with either all roots in random locations or one root pinned to the origin and the other roots in random locations<br/>
```./mandelbrot -random (-any or -pinned) (# of roots) (# of images to generate) (-color or -noColor) (-center or -notCentered)``` <br/>
```./mandelbrot -random -any (# of roots) (# of images to generate) (-color or -noColor) (-center or -notCentered)``` generates shapes in which all root locations are randomized <br/>
```./mandelbrot -random -pinned (# of roots) (# of images to generate) (-color or -noColor) (-center or -notCentered)``` generates shapes in which one root location is pinned to the origin and all other root locations are randomized 
- **Regular grid exploration with two roots, one pinned to the origin:** generates shapes with one root at a location that follows a regular grid of size (grid size x) by (grid size y) and the other root pinned to the origin <br/>
```./mandelbrot -pinned (grid size x) (grid size y) (-color or -noColor) (-center or -notCentered)``` <br/>
- **Regular grid exploration with two roots:** generates shapes with roots at locations following a regular grid of size (grid size x) by (grid size y)
```./mandelbrot -full (grid size x) (grid size y) (-color or -noColor) (-center or -notCentered)``` <br/>
```(-color or -noColor)``` specifies whether the images generated should have root locations colored in red<br/>
``` (-center or -notCentered) ``` specifies whether the images generated should have the fractal shapes centered in the middle of the image

## Modes for categorizing images using a pixel-by-pixel approach:
- **Categorize all images:** puts (# of images) images into categories based on (cutoff score)<br/>
```./categorize -all (# of images) (cutoff score)```
- **Calculate match between two specific images:** takes (first image #) as the reference image and computes the match score between (first image #) and (second image #) with (cutoff score) <br/>
```./categorize -specific (first image #) (second image #) (cutoff score)```
- **Calculate match scores for a set of images:** takes (first image #) as the reference image and computes the match score between the following images and the reference image,  finds all images that can be categorized together with (first image #) as the reference image <br/>
```./categorize -group (cutoff score) (first image #) (second image #) (etc.)```