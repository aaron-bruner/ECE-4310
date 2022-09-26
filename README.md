# ECE-4310 [Introduction to Computer Vision]
Course material for ECE-4310 at Clemson University for the Fall semester of 2022.

## Course Description
The purpose of a computer vision system is to take data (usually in the form of one or more images) and
produce information. For example, a computer vision system might inspect bottles for proper volumes,
identify abnormal tissue in a medical image, recognize a fingerprint, or tell an automated door when it is
safe to close. This course teaches the mainstream theories of computer vision used to build such systems.
Several examples (such as optical character recognition) are implemented in assignments.
Upon successful completion of the course, students will be able to apply mainstream computer vision
theories in the engineering (design, implementation, testing and debugging) of modern devices and
systems.

## Topics Covered
- Machine vision sensors and paradigms (1 week)
- Image processing basics (histograms, smoothing, convolution, edge detection) (2 weeks)
- GUI event-driven programming (1.5 weeks)
- Segmentation, region properties and algorithms (2 weeks)
- Matched filters, ROC curves and evaluation (2 weeks)
- Active contours (snakes), energy minimization (1.5 weeks)
- Tsai’s camera calibration model and method, system latency (2 weeks)
- Accelerometers and gyroscopes, motion data, activity recognition (1 week)
- Object modeling and recognition (1 week)
- Range cameras, 3D data, and surface segmentation (1 week)

###### Lab 1 [Convolution, Separable Filters, Sliding Windows]
The purpose of this lab is to implement three versions of a 7x7 mean filter. The first
version should use basic 2D convolution. The second version should use separable filters
(1x7 and 7x1). The third version should use separable filters and a sliding window.

###### Lab 2 [OCR - Optical Character Recognition]
The purpose of this lab is to implement a matched filter (normalized crosscorrelation) to recognize letters in an image of text.

###### Lab 3 [Letters]
The purpose of this lab is to implement thinning, branchpoint and endpoint detection
to recognize letters in an image of text.

###### Lab 4 [Region Interaction]
The purpose of this project is to implement interactive region growing. Your program
will build upon two pieces of code given at the [web site](http://cecas.clemson.edu/~ahoover/ece431/). The plus program allows the
user to load and display an image, and demonstrates several GUI and event handling
techniques. The region growing code demonstrates growing a region based upon several
predicates. These pieces must be integrated into a new program that allows the user to
click any location in an image and visualize the results of growing a region there.

###### Lab 5 [Active Contours]
The purpose of this project is to implement the active contour algorithm. The program
must load a grayscale PPM image and a list of contour points. The contour points must
be processed through the active contour algorithm using the options given below. The
program must output a copy of the image with the initial contour drawn on top of it, and a
second image with the final contour drawn on top of it. The program must also output a
list of the final contour pixel coordinates.

###### Lab 6 [Motion Tracking]
The purpose of this lab is to calculate motion using accelerometers and gyroscopes.
A file of data recorded using an iPhone is available at the course website. There are 7
columns in the file, containing the following data:

- time 
- x_acc 
- y_acc 
- z_acc 
- pitch 
- roll 
- yaw

###### Lab 7 [Camera Calibration]
The purpose of this lab is to complete one of the following options:
(a) calibrate the camera network in the Riggs 13/15/17 lab
(b) calibrate your personal webcam or smartphone using the MATLAB camera calibrator

###### Lab 8 [Range Image Segmentation]
The purpose of this lab is to segment a range image based upon surface normals. A
range image of a chair is given at the course website (note that the reflectance image is
only for visualization and will not be used for the lab; make sure you work with the range
image). Some C-code is also provided to convert the pixels into 3D coordinates. The
segmentation process will use the image grid for grouping pixels, but will use the 3D
coordinates for calculating surface normals for region predicates.
