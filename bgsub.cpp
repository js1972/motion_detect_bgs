#include <opencv2/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/imgproc.hpp>

#include <stdio.h>

#include <iostream>
#include <sstream>

#include <unistd.h> //for sleep

using namespace cv;
using namespace std;

//global variables
Mat frame; //current frame
Mat bgMaskMOG2; //bg mask generated by MOG2 method
Mat segmented_frame;
Mat gray;

Ptr<BackgroundSubtractorMOG2> pMOG2; //MOG2 Background subtractor
int keyboard;

//function declarations
void help();
void processVideo(char* filename);
static void refineSegments(const Mat& img, Mat& mask, Mat& dst);

void help()
{
  cout
  << "----------------------------------------------------------------------------" << endl
  << "This program shows how to use background subtraction methods provided by "    << endl
  << "OpenCV."                                                                      << endl 
                                                                                    << endl
  << "Usage:"                                                                       << endl
  << "./BackgroundSubtraction <video filename>"                                     << endl
  << "for example: ./BackgroundExample video.avi"                                   << endl
  << endl;
}

int main(int argc, char* argv[])
{
  if(argc != 2)
  {
    help();
    return EXIT_FAILURE;
  }

  //create GUI windows
  namedWindow("Frame");
  namedWindow("FG Mask MOG2");

  //create Background Subtractor objects
  pMOG2 = createBackgroundSubtractorMOG2();
  //pMOG2->setVarThreshold(10);

  processVideo(argv[1]);

  destroyAllWindows();

  return EXIT_SUCCESS;
}

void processVideo(char* filename)
{
  //create the capture object
  VideoCapture cap(filename);
  
  //VideoCapture capture(videoFilename);
  if(!cap.isOpened())
  {
    cerr << "Unable to open video file: " << filename << endl;
    exit(EXIT_FAILURE);
  }
  //read input data. ESC or 'q' for quitting
  while ((char)keyboard != 'q' && (char)keyboard != 27)
  {
    //read the current frame
    if(!cap.read(frame))
    {
      cerr << "Unable to read next frame." << endl;
      cerr << "Exiting..." << endl;
      exit(EXIT_FAILURE);
    }

    //remove noise
    cvtColor(frame, gray, COLOR_BGR2GRAY);
    GaussianBlur(gray, gray, Size(21, 21), 0);

    //update the background model
    pMOG2->apply(gray, bgMaskMOG2);
    refineSegments(frame, bgMaskMOG2, segmented_frame);

    //get the frame number and write it on the current frame
    //stringstream ss;
    //rectangle(frame, cv::Point(10, 2), cv::Point(100, 20),
    //          cv::Scalar(255, 255, 255), -1);
    //ss << cap.get(CAP_PROP_POS_FRAMES);
    //string frameNumberString = ss.str();
    //putText(frame, frameNumberString.c_str(), cv::Point(15, 15),
    //        FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,0,0));

    //show the current frame and the fg masks
    imshow("Frame", frame);
    imshow("FG Mask MOG2", bgMaskMOG2);
    imshow("Segmented", segmented_frame);

    //get the input from the keyboard
    keyboard = waitKey(30);
  }
  //delete capture object
  cap.release();
}

static void refineSegments(const Mat& img, Mat& mask, Mat& dst) {
  int niters = 5;
  vector<vector<Point> > contours;
  vector<Vec4i> hierarchy;
  Mat temp;

  dilate(mask, temp, Mat(), Point(-1,-1), niters);
  erode(temp, temp, Mat(), Point(-1,-1), niters*2);
  dilate(temp, temp, Mat(), Point(-1,-1), niters);

  findContours(temp, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);
  dst = Mat::zeros(img.size(), CV_8UC3);
  if(contours.size() == 0)
      return;

  for(int i = 0; i < contours.size(); i++) {
    if(contourArea(contours[i]) < 600) {
      continue;
    }

    // get a bounding box so we can work out where the contour is and ignore it
    // if its where the camera time display is.
    Rect bbox = boundingRect(contours[i]);
    rectangle(dst, bbox.tl(), bbox.br(), Scalar(0,255,0), 2);
    cout << "Bounding box coords - tl: " << bbox.tl() << ", br: " << bbox.br() << endl;

    //rectangle(dst, Point(0, 0), Point(350, 150), Scalar(255, 0, 0), FILLED);

    // ignore the area where hikvision cameras show the date/time
    if (bbox.br().x > 300 && bbox.br().y > 150) {
      putText(dst, "Motion Detected", Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,255),2);
    }
  }

  // iterate through all the top-level contours,
  // draw each connected component with its own random color
  int idx = 0, largestComp = 0;
  double maxArea = 0;
  for (; idx >= 0; idx = hierarchy[idx][0]) {
    const vector<Point>& c = contours[idx];
    double area = fabs(contourArea(Mat(c)));
    if (area < 500) {
      continue;
    }
    if(area > maxArea) {
        maxArea = area;
        largestComp = idx;
    }
  }

  Scalar color( 0, 0, 255 );
  drawContours(dst, contours, largestComp, color, FILLED, LINE_8, hierarchy);
}