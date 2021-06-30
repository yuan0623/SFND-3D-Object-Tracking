# SFND 3D Object Tracking

## Dependencies for Running Locally
* cmake >= 2.8
  * All OSes: [click here for installation instructions](https://cmake.org/install/)
* make >= 4.1 (Linux, Mac), 3.81 (Windows)
  * Linux: make is installed by default on most Linux distros
  * Mac: [install Xcode command line tools to get make](https://developer.apple.com/xcode/features/)
  * Windows: [Click here for installation instructions](http://gnuwin32.sourceforge.net/packages/make.htm)
* Git LFS
  * Weight files are handled using [LFS](https://git-lfs.github.com/)
* OpenCV >= 4.1
  * This must be compiled from source using the `-D OPENCV_ENABLE_NONFREE=ON` cmake flag for testing the SIFT and SURF detectors.
  * The OpenCV 4.1.0 source code can be found [here](https://github.com/opencv/opencv/tree/4.1.0)
* gcc/g++ >= 5.4
  * Linux: gcc / g++ is installed by default on most Linux distros
  * Mac: same deal as make - [install Xcode command line tools](https://developer.apple.com/xcode/features/)
  * Windows: recommend using [MinGW](http://www.mingw.org/)

## Basic Build Instructions

1. Clone this repo.
2. Make a build directory in the top level project directory: `mkdir build && cd build`
3. Compile: `cmake .. && make`
4. Run it: `./3D_object_tracking`.  (If you have run-time error, you may need to re-download the `yolov3.weights`. just run `wget "https://pjreddie.com/media/files/yolov3.weights"`, and copy `yolov3.weights` into `dat/yolo/`)

## rubric

### FP.1 Match 3D Objects

check line 252-301 in `camFusion_student.cpp`.

### FP.2 Compute Lidar-based TTC

check line 218-249 in `camFusion_student.cpp`.

### FP.3 Associate Keypoint Correspondences with Bounding Boxes

check line 139-158 in `camFusion_student.cpp`.

### FP.4 Compute Camera-based TTC

check line 162-215 in `camFusion_student.cpp`.

### FP.5 Performance Evaluation 1
1. when there are points on the edge of the bounding box, the TTC tend to be wrong. This is because the bounding box does not represent the true shape of the object. The points on the edge may represent something else.
2. when the points are within the bounding box, but they are on the other object, the TTC tend to be wrong. This is because the outlier is not taken out.

### FP.6 Performance Evaluation 2
1. The detector/descriptor combination has been listed [here](https://docs.google.com/spreadsheets/d/1uxNoxjb7APyzbs_wWEZHTDqBL1qvrkhQDfxyXEYi0ek/edit?usp=sharing). From this spread sheet, we know FAST/BRIEF combination is the best because it is the most efficient combination and the TTC is reasonalbly good.
2. From the image sequcene provided, I didn't spot any faulty detection. I think the camera detection is robust in the provided case. However, if the lighting is too bad, then the camera-based TTC is going to be way off.
