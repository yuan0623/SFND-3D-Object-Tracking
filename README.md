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
The `matchBoundingBoxes` functon is implemented. 
I take one matched keypoint at a time find the corresponsing point in current and prev frame, then I check which bounding box in prev and curr frame the point belong to, once found store the value and increment the count
check line 283-332 in `camFusion_student.cpp`.

### FP.2 Compute Lidar-based TTC
First, I find closest distance to Lidar points within ego lane, then I use the equation that I learned from the TTC section to compute the TTC.
check line 238-280 in `camFusion_student.cpp`.

### FP.3 Associate Keypoint Correspondences with Bounding Boxes
First, I take one match pair, then if the previous keypoint and current keypoint are within the same bounding box, then we add this match to a vector. After finishing the loop, I assign the obtained vector to `boundingBox.kptMatches`. Then I filter out the outliers based on their distance.
check line 145-178 in `camFusion_student.cpp`.

### FP.4 Compute Camera-based TTC
This section is heavily based on the previous practice code. First I compute distance ratios between all matched keypoints, then I use median of computed distance ratio, and apply the equation I learned to compute TTC.
check line 182-235 in `camFusion_student.cpp`.

### FP.5 Performance Evaluation 1
Based on the given sequence, I don't spot the lidar measurement has any obvious faulty-measurement due to the robustness of my code. If you don't believe it, feel free to run the code. All TTC estimation is reasonable. 

### FP.6 Performance Evaluation 2
1. The detector/descriptor combination has been listed [here](https://docs.google.com/spreadsheets/d/1uxNoxjb7APyzbs_wWEZHTDqBL1qvrkhQDfxyXEYi0ek/edit?usp=sharing). From this spread sheet, we know FAST/BRIEF combination is the best because it is the most efficient combination and the TTC is reasonalbly good.
2. From the image sequcene provided, I spot one faulty measurement. I think it's caused by too many faulty-match.
