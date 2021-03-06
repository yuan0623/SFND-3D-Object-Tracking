
#include <iostream>
#include <algorithm>
#include <numeric>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "camFusion.hpp"
#include "dataStructures.h"

using namespace std;


// Create groups of Lidar points whose projection into the camera falls into the same bounding box
void clusterLidarWithROI(std::vector<BoundingBox> &boundingBoxes, std::vector<LidarPoint> &lidarPoints, float shrinkFactor, cv::Mat &P_rect_xx, cv::Mat &R_rect_xx, cv::Mat &RT)
{
    // loop over all Lidar points and associate them to a 2D bounding box
    cv::Mat X(4, 1, cv::DataType<double>::type);
    cv::Mat Y(3, 1, cv::DataType<double>::type);

    for (auto it1 = lidarPoints.begin(); it1 != lidarPoints.end(); ++it1)
    {
        // assemble vector for matrix-vector-multiplication
        X.at<double>(0, 0) = it1->x;
        X.at<double>(1, 0) = it1->y;
        X.at<double>(2, 0) = it1->z;
        X.at<double>(3, 0) = 1;

        // project Lidar point into camera
        Y = P_rect_xx * R_rect_xx * RT * X;
        cv::Point pt;
        // pixel coordinates
        pt.x = Y.at<double>(0, 0) / Y.at<double>(2, 0); 
        pt.y = Y.at<double>(1, 0) / Y.at<double>(2, 0); 

        vector<vector<BoundingBox>::iterator> enclosingBoxes; // pointers to all bounding boxes which enclose the current Lidar point
        for (vector<BoundingBox>::iterator it2 = boundingBoxes.begin(); it2 != boundingBoxes.end(); ++it2)
        {
            // shrink current bounding box slightly to avoid having too many outlier points around the edges
            cv::Rect smallerBox;
            smallerBox.x = (*it2).roi.x + shrinkFactor * (*it2).roi.width / 2.0;
            smallerBox.y = (*it2).roi.y + shrinkFactor * (*it2).roi.height / 2.0;
            smallerBox.width = (*it2).roi.width * (1 - shrinkFactor);
            smallerBox.height = (*it2).roi.height * (1 - shrinkFactor);

            // check wether point is within current bounding box
            if (smallerBox.contains(pt))
            {
                enclosingBoxes.push_back(it2);
            }

        } // eof loop over all bounding boxes

        // check wether point has been enclosed by one or by multiple boxes
        if (enclosingBoxes.size() == 1)
        { 
            // add Lidar point to bounding box
            enclosingBoxes[0]->lidarPoints.push_back(*it1);
        }

    } // eof loop over all Lidar points
}

/* 
* The show3DObjects() function below can handle different output image sizes, but the text output has been manually tuned to fit the 2000x2000 size. 
* However, you can make this function work for other sizes too.
* For instance, to use a 1000x1000 size, adjusting the text positions by dividing them by 2.
*/
void show3DObjects(std::vector<BoundingBox> &boundingBoxes, cv::Size worldSize, cv::Size imageSize, bool bWait)
{
    // create topview image
    cv::Mat topviewImg(imageSize, CV_8UC3, cv::Scalar(255, 255, 255));

    for(auto it1=boundingBoxes.begin(); it1!=boundingBoxes.end(); ++it1)
    {
        // create randomized color for current 3D object
        cv::RNG rng(it1->boxID);
        cv::Scalar currColor = cv::Scalar(rng.uniform(0,150), rng.uniform(0, 150), rng.uniform(0, 150));

        // plot Lidar points into top view image
        int top=1e8, left=1e8, bottom=0.0, right=0.0; 
        float xwmin=1e8, ywmin=1e8, ywmax=-1e8;
        for (auto it2 = it1->lidarPoints.begin(); it2 != it1->lidarPoints.end(); ++it2)
        {
            // world coordinates
            float xw = (*it2).x; // world position in m with x facing forward from sensor
            float yw = (*it2).y; // world position in m with y facing left from sensor
            xwmin = xwmin<xw ? xwmin : xw;
            ywmin = ywmin<yw ? ywmin : yw;
            ywmax = ywmax>yw ? ywmax : yw;

            // top-view coordinates
            int y = (-xw * imageSize.height / worldSize.height) + imageSize.height;
            int x = (-yw * imageSize.width / worldSize.width) + imageSize.width / 2;

            // find enclosing rectangle
            top = top<y ? top : y;
            left = left<x ? left : x;
            bottom = bottom>y ? bottom : y;
            right = right>x ? right : x;

            // draw individual point
            cv::circle(topviewImg, cv::Point(x, y), 4, currColor, -1);
        }

        // draw enclosing rectangle
        cv::rectangle(topviewImg, cv::Point(left, top), cv::Point(right, bottom),cv::Scalar(0,0,0), 2);

        // augment object with some key data
        char str1[200], str2[200];
        sprintf(str1, "id=%d, #pts=%d", it1->boxID, (int)it1->lidarPoints.size());
        putText(topviewImg, str1, cv::Point2f(left-250, bottom+50), cv::FONT_ITALIC, 2, currColor);
        sprintf(str2, "xmin=%2.2f m, yw=%2.2f m", xwmin, ywmax-ywmin);
        putText(topviewImg, str2, cv::Point2f(left-250, bottom+125), cv::FONT_ITALIC, 2, currColor);  
    }

    // plot distance markers
    float lineSpacing = 2.0; // gap between distance markers
    int nMarkers = floor(worldSize.height / lineSpacing);
    for (size_t i = 0; i < nMarkers; ++i)
    {
        int y = (-(i * lineSpacing) * imageSize.height / worldSize.height) + imageSize.height;
        cv::line(topviewImg, cv::Point(0, y), cv::Point(imageSize.width, y), cv::Scalar(255, 0, 0));
    }

    // display image
    string windowName = "3D Objects";
    cv::namedWindow(windowName, 1);
    cv::imshow(windowName, topviewImg);

    if(bWait)
    {
        cv::waitKey(0); // wait for key to be pressed
    }
}

struct toSort{
    cv::DMatch DMatch;
    double euclideanDistance;
};
// associate a given bounding box with the keypoints it contains
bool Compare(toSort a, toSort b) {
    double d1 = a.euclideanDistance; // f1 = g1 + h1
    double d2 = b.euclideanDistance; // f2 = g2 + h2
    return d1 < d2;
}

void clusterKptMatchesWithROI(BoundingBox &boundingBox, std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr, std::vector<cv::DMatch> &kptMatches)
{
    // ...

    std::vector<cv::DMatch> kptMatches_bb;
    std::vector<toSort> toSortVec;
    for(auto kptMatch_itr = kptMatches.begin(); kptMatch_itr != kptMatches.end(); kptMatch_itr++)
    {
        auto current_kpt_index = kptMatch_itr->trainIdx;
        auto previous_kpt_index = kptMatch_itr->queryIdx;
        if(boundingBox.roi.contains(kptsCurr[current_kpt_index].pt) && boundingBox.roi.contains(kptsPrev[previous_kpt_index].pt))
        {
            toSort myToSort;
            myToSort.DMatch = *kptMatch_itr;
            cv::KeyPoint kpInnerCurr = kptsCurr.at(current_kpt_index);
            cv::KeyPoint kpInnerPrev = kptsPrev.at(previous_kpt_index);
            myToSort.euclideanDistance = std::sqrt(std::pow((kpInnerCurr.pt.x - kpInnerPrev.pt.x),2)+std::pow((kpInnerCurr.pt.y - kpInnerPrev.pt.y),2));
            toSortVec.push_back(myToSort);

        }


    }
    // filter out the outliers
    std::sort(toSortVec.begin(),toSortVec.end(),Compare);

    double med_dis = toSortVec[floor(kptMatches_bb.size()/2)].euclideanDistance;
    double q1_dis = toSortVec[floor(kptMatches_bb.size()/4)].euclideanDistance;
    double q3_dis = toSortVec[floor(kptMatches_bb.size()/4*3)].euclideanDistance;
    double IQR_dis = q3_dis - q1_dis;
    //std::cout<<"q1: "<<q1_dis<<" "<<"q3: "<<q3_dis<<" "<<IQR_dis<<std::endl;
    for(auto toSortVec_itr = toSortVec.begin(); toSortVec_itr !=toSortVec.end();toSortVec_itr++)
    {
        if(toSortVec_itr->euclideanDistance<q1_dis-IQR_dis && toSortVec_itr->euclideanDistance>q3_dis+IQR_dis)
        {
            std::cout<<"I found the outliers"<<std::endl;


        }
        else
        {
            kptMatches_bb.push_back(toSortVec_itr->DMatch);
        }
    }

    boundingBox.kptMatches = kptMatches_bb;

}



// Compute time-to-collision (TTC) based on keypoint correspondences in successive images
void computeTTCCamera(std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr, 
                      std::vector<cv::DMatch> kptMatches, double frameRate, double &TTC, cv::Mat *visImg)
{
    // ...
    // compute distance ratios between all matched keypoints
    vector<double> distRatios; // stores the distance ratios for all keypoints between curr. and prev. frame
    for (auto it1 = kptMatches.begin(); it1 != kptMatches.end() - 1; ++it1)
    { // outer kpt. loop

        // get current keypoint and its matched partner in the prev. frame
        cv::KeyPoint kpOuterCurr = kptsCurr.at(it1->trainIdx);
        cv::KeyPoint kpOuterPrev = kptsPrev.at(it1->queryIdx);

        for (auto it2 = kptMatches.begin() + 1; it2 != kptMatches.end(); ++it2)
        { // inner kpt.-loop

            double minDist = 100.0; // min. required distance

            // get next keypoint and its matched partner in the prev. frame
            cv::KeyPoint kpInnerCurr = kptsCurr.at(it2->trainIdx);
            cv::KeyPoint kpInnerPrev = kptsPrev.at(it2->queryIdx);

            // compute distances and distance ratios
            double distCurr = cv::norm(kpOuterCurr.pt - kpInnerCurr.pt);
            double distPrev = cv::norm(kpOuterPrev.pt - kpInnerPrev.pt);

            if (distPrev > std::numeric_limits<double>::epsilon() && distCurr >= minDist)
            { // avoid division by zero

                double distRatio = distCurr / distPrev;
                distRatios.push_back(distRatio);
            }
        } // eof inner loop over all matched kpts
    }     // eof outer loop over all matched kpts

    // only continue if list of distance ratios is not empty
    if (distRatios.size() == 0)
    {
        TTC = NAN;
        return;
    }


    // STUDENT TASK (replacement for meanDistRatio)
    std::sort(distRatios.begin(), distRatios.end());
    long medIndex = floor(distRatios.size() / 2.0);
    double medDistRatio = distRatios.size() % 2 == 0 ? (distRatios[medIndex - 1] + distRatios[medIndex]) / 2.0 : distRatios[medIndex]; // compute median dist. ratio to remove outlier influence

    double dT = 1 / frameRate;
    TTC = -dT / (1 - medDistRatio);



}


void computeTTCLidar(std::vector<LidarPoint> &lidarPointsPrev,
                     std::vector<LidarPoint> &lidarPointsCurr, double frameRate, double &TTC)
{

    // auxiliary variables
    double dT = 1/frameRate;        // time between two measurements in seconds
    double laneWidth = 4.0; // assumed width of the ego lane

    // find median of distance to Lidar points within ego lane
    double minXPrev = 1e9, minXCurr = 1e9;
    std::vector<double> XPrev;
    std::vector<double> XCur;
    for (auto it = lidarPointsPrev.begin(); it != lidarPointsPrev.end(); ++it)
    {

        if (abs(it->y) <= laneWidth / 2.0)
        { // 3D point within ego lane?
            //minXPrev = minXPrev > it->x ? it->x : minXPrev;
            XPrev.push_back(it->x);
        }
    }

    for (auto it = lidarPointsCurr.begin(); it != lidarPointsCurr.end(); ++it)
    {

        if (abs(it->y) <= laneWidth / 2.0)
        { // 3D point within ego lane?
            //minXCurr = minXCurr > it->x ? it->x : minXCurr;
            XCur.push_back(it->x);
        }
    }
    std::sort(XPrev.begin(),XPrev.end());
    std::sort(XCur.begin(),XCur.end());
    long medIndexPrev = floor(XPrev.size() / 2.0);
    long medIndexCur = floor(XCur.size()/2.0);
    double medPrev = XPrev.size() % 2 == 0 ? (XPrev[medIndexPrev - 1] + XPrev[medIndexPrev]) / 2.0 : XPrev[medIndexPrev]; // compute median of x in previous frame
    double medCur = XCur.size() % 2 == 0 ? (XCur[medIndexCur - 1] + XCur[medIndexCur]) / 2.0 : XCur[medIndexCur]; // compute median of x in previous frame


    // compute TTC from both measurements
    TTC = medCur * dT / (medPrev - medCur);

}


void matchBoundingBoxes(std::vector<cv::DMatch> &matches, std::map<int, int> &bbBestMatches, DataFrame &prevFrame, DataFrame &currFrame)
{
    // ...
    const int size_p = prevFrame.boundingBoxes.size();
    const int size_c = currFrame.boundingBoxes.size();
    //int count[size_p][size_c] = {};//initialize a null matrix with all values "0" of bounding boxes size prev x curr
    cv::Mat count = cv::Mat::zeros(size_p, size_c, CV_32S);
    for (auto matchpair : matches)
    {
        //take one matched keypoint at a time find the corresponsing point in current and prev frame
        //once done check to which bounding box in prev and curr frame the point belong too
        //once found store the value and increment the count
        //cv::KeyPoint kpOuterPrev = kptsPrev.at(it1->queryIdx);
        cv::KeyPoint prevkp1 = prevFrame.keypoints.at(matchpair.queryIdx);
        auto prevkp = prevkp1.pt;//previous frame take keypint
        cv::KeyPoint currkp1 = currFrame.keypoints.at(matchpair.trainIdx);
        auto currkp = currkp1.pt;//current frame take keypint
        for (int prevbb = 0; prevbb < size_p; prevbb++)//loop through all the prev frame bb
        {
            if (prevFrame.boundingBoxes[prevbb].roi.contains(prevkp))//check if the "previous frame take keypint" belongs to this box
            {//if it does
                for (int currbb = 0; currbb < size_c; currbb++)//loop through all the curr frame bb
                {
                    if (currFrame.boundingBoxes[currbb].roi.contains(currkp))//check if the "current frame take keypint" belongs to this box
                    {//if it does
                        //count[prevbb][currbb] = count[prevbb][currbb] + 1;//do a +1 if match is found
                        count.at<int>(prevbb, currbb) = count.at<int>(prevbb, currbb) + 1;
                    }
                }
            }
        }
    }
    //for each prev bb find and compare the max count of corresponding curr bb.
    //the curr bb with max no. of matches (max count) is the bbestmatch
    for (int i = 0; i < size_p; i++)//loop through prev bounding box
    {
        int id = -1;//initialize id as the matrix starts from 0 x 0 we do not want to take 0 as the initializing value
        int maxvalue = 0;//initialize max value
        for (int j = 0; j < size_c; j++)//loop through all curr bounding boxes to see which prev + curr bb pair has maximum count
        {
            if (count.at<int>(i,j) > maxvalue)
            {
                maxvalue = count.at<int>(i,j);//input value for comparison
                id = j;//id
            }
        }
        bbBestMatches[i] = id;//once found for 1 prev bounding box; input the matched pair in bbBestMatches

    }
}
