#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <string> 
#include <sstream>
#include <stdio.h>
#include <opencv2/cudacodec.hpp>

#define UP    0
#define DOWN  1
#define LEFT  2
#define RIGHT 3
#define MAX_X 824
#define MIN_X 520
#define MAX_Y 623
#define MIN_Y 92
#define WIDTH 304
#define HEIGHT 531

using namespace cv;
using namespace std;

int theObject[2] = {0,0};
int theOtherObject[2] = {0,0}; 	//because, we are going to need a progressing state change
int x2 = theOtherObject[1];
int y2 = theOtherObject[2];
int deltax[9] = {0}; 		//let's try this method first off; pre-populate the array with all zero values
int deltay[9] = {0};
int deltaidx = 0;
int headingx[9] = {0};
int headingy[9] = {0};

//generic declarations - program wide
int i;
int foo;
int goingDown;
int goingRight;
int current_x, current_y;
int paddle_x;
int x_direction = LEFT;

cv::Rect objectBoundingRectangle = cv::Rect(0,0,0,0);

string intToString(int number) {
    std::stringstream ss;
    ss << number;
    return ss.str();
}
 
void searchForMovement(cv::Mat thresholdImage, cv::Mat &cameraFeed) 
{
    bool objectDetected = false;
    int xpos, ypos, direction_change;

    vector< vector<Point> > contours;
    vector<Vec4i> hierarchy;

    cv::Mat temp;

    thresholdImage.copyTo(temp);

    //cv::Rect roi(x,y,w,h)
    cv::Rect roi(MIN_X,MIN_Y,WIDTH,HEIGHT);

    cv::Mat roi_temp = temp(roi); 
    cv::findContours(roi_temp,contours,hierarchy,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE );
    if(contours.size()>0)
	objectDetected = true;
    else 
	objectDetected = false;
 
    if(objectDetected){

        vector< vector<Point> > largestContourVec;
        largestContourVec.push_back(contours.at(contours.size()-1));
        objectBoundingRectangle = boundingRect(largestContourVec.at(0));

        xpos = objectBoundingRectangle.x+objectBoundingRectangle.width/2;
        ypos = objectBoundingRectangle.y+objectBoundingRectangle.height/2;
        theObject[0] = xpos , theObject[1] = ypos;
    }

    //make some temp x and y variables so we dont have to type out so much

    int x1 = theObject[0]+520;
    int y1 = theObject[1]+92;
    if(y1 != y2) //movement. we give no craps about sideways motion, really
    { if(x1 < x2) //from right to left 
      { deltax[deltaidx] = x2-x1;
        headingx[deltaidx] = LEFT;
	if(y1 < y2) //from down to up
	{ deltay[deltaidx] = y2-y1;
          headingy[deltaidx] = UP; }
	else //from up to down
	{ deltay[deltaidx] = y1-y2;
          headingy[deltaidx] = DOWN; }
      }	
      else //from left to right
      { deltax[deltaidx] = x1-x2;
        headingx[deltaidx] = RIGHT;
        if(y1 < y2) //from down to up
        { deltay[deltaidx] = y2-y1;
          headingy[deltaidx] = UP; }
        else //from up to down
        { deltay[deltaidx] = y1-y2;
          headingy[deltaidx] = DOWN; }
      }
    }
    if(deltaidx == 7) //then that's the last value I need into the delta arrays; from now on, we're just shifting
    { direction_change = 0; //assuming we're not changing direction some time in this 8 frames
      for(i = 1; i<8; i++)
      { if ((headingx[i-1] != headingx[i])    //we bounced off of something in the horizontal
           || (headingy[i-1] != headingy[i])) //we bounced off of something in the vertical
	{ direction_change = 1; }
      }
     
      if(direction_change == 0) //you're moving in a straight line
      { int j;
	for(j=0;j<8;j++)
	{ deltax[8] = deltax[j]+deltax[8];
	  deltay[8] = deltay[j]+deltay[8]; }		
	  deltax[8] = deltax[8] >> 3; //because division is expensive; shifting is cheap
	  deltay[8] = deltay[8] >> 3;
	for(i=1;i<8;i++)
	{ deltax[i-1] = deltax[i];    //shift the whole array one position to the left
	  deltay[i-1] = deltay[i];
          headingx[i-1] = headingx[i];
          headingy[i-1] = headingy[i]; }  //as the loop fires, it'll reassign delta*[7] and then we'll check again
          x_direction = headingx[0];
          if(headingy[0] == DOWN)
           goingDown = 1;
          else
           goingDown = 0;
      }           
      else //then let's just flush the array
      { for(i = 0;i<9; i++)
        { deltax[i] = 0;
          deltay[i] = 0;
          headingx[i] = 0;
          headingy[i] = 0;
          deltaidx = 0; }
      }  
    }
 else
 { deltaidx++; }
 x2 = x1;
 y2 = y1;

 

    //let's draw some lines to actually figure out where anything is actually AT!
    line(cameraFeed,Point(520,92),Point(824,92),Scalar(0,0,255),2);    //top
    line(cameraFeed,Point(520,623),Point(824,623),Scalar(0,0,255),2);  //bottom
    line(cameraFeed,Point(520,92),Point(520,623),Scalar(0,0,255),2);   //left
    line(cameraFeed,Point(824,92),Point(824,623),Scalar(0,0,255),2);   //right

    //write the position of the object to the screen
    if(goingDown) {
     putText(cameraFeed,"[DOWN]",Point(50,50),1,1,Scalar(255,0,0),2); 
     putText(cameraFeed,"[ MOVE RIGHT OR LEFT ]",Point(50,100),1,1,Scalar(255,0,0),2);
     if(x_direction == LEFT)
      putText(cameraFeed,"[ BALL MOVING LEFT - XPOS: " +intToString(x1) + " ]",Point(50,150),1,1,Scalar(255,0,0),2);
     else
      putText(cameraFeed,"[ BALL MOVING RIGHT- XPOS: " + intToString(x1) + " ]",Point(50,150),1,1,Scalar(255,0,0),2);
    }
    else {
     putText(cameraFeed,"[ UP ]",Point(50,50),1,1,Scalar(255,0,0),2);
     putText(cameraFeed,"[   MOVE TO CENTER   ]",Point(50,100),1,1,Scalar(255,0,0),2);
    }
}

int main() {

    cv::Mat frame0, frame1, result;
    cv::Mat frame0_warped, frame1_warped;

    cv::cuda::GpuMat gpu_frame0, gpu_frame1, gpu_grayImage0, gpu_grayImage1, gpu_differenceImage, gpu_thresholdImage;
    cv::cuda::GpuMat gpu_frame0_warped, gpu_frame1_warped;

    int toggle, frame_count;
    // Add a camera stream here

    // MP4 file pipeline
    std::string pipeline = "filesrc location=/home/nvidia/EE555_Lab8/pong_video.mp4 ! qtdemux name=demux ! h264parse ! omxh264dec ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";

    std::cout << "Using pipeline: " << pipeline << std::endl;
 
    // Create OpenCV capture object, ensure it works.
    cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);

    if (!cap.isOpened()) {
        std::cout << "Connection failed" << std::endl;
        return -1;
    }

    // Add serial init check here

    
Point2f inputQuad[4];
Point2f outputQuad[4];

cv::Mat lambda (2, 4, CV_32FC1);

inputQuad[0] = Point2f(520,80);
inputQuad[1] = Point2f(880,77);
inputQuad[2] = Point2f(923,672);
inputQuad[3] = Point2f(472,655);

outputQuad[0] = Point2f(437,0);
outputQuad[1] = Point2f(842,0);
outputQuad[2] = Point2f(842,719);
outputQuad[3] = Point2f(437,719);

 lambda = cv::getPerspectiveTransform(inputQuad,outputQuad);

    // Capture the first frame with GStreamer
    cap >> frame0;
    gpu_frame0.upload(frame0);
    cv::cuda::warpPerspective(gpu_frame0,gpu_frame0_warped,lambda,frame0.size());
    gpu_frame0_warped.download(frame0_warped);

    // Convert the frames to gray scale (monochrome)
    //cv::cvtColor(frame0,grayImage0,cv::COLOR_BGR2GRAY);
    cv::cuda::cvtColor(gpu_frame0_warped,gpu_grayImage0,cv::COLOR_BGR2GRAY);

    // Initialize 
    toggle = 0;
    frame_count = 0;

    while (frame_count < 2000) {

        if (toggle == 0) {
           // Get a new frame from file
           cap >> frame1;
	   gpu_frame1.upload(frame1);
	   cv::cuda::warpPerspective(gpu_frame1,gpu_frame1_warped,lambda,frame1.size());
	   gpu_frame1_warped.download(frame1_warped);

           cv::cuda::cvtColor(gpu_frame1_warped,gpu_grayImage1,cv::COLOR_BGR2GRAY);
           toggle = 1;
        } 
        else {
	   cap >> frame0;
           gpu_frame0.upload(frame0);
           cv::cuda::warpPerspective(gpu_frame0,gpu_frame0_warped,lambda,frame0.size());
	   gpu_frame0_warped.download(frame0_warped);

	   cv::cuda::cvtColor(gpu_frame0_warped,gpu_grayImage0,cv::COLOR_BGR2GRAY);
           toggle = 0;
	}
 
	// Compute the absolute value of the difference
	cv::cuda::absdiff(gpu_grayImage0, gpu_grayImage1, gpu_differenceImage);


	// Threshold the difference image
        cv::cuda::threshold(gpu_differenceImage, gpu_thresholdImage, 50, 255, cv::THRESH_BINARY);

        gpu_thresholdImage.download(result);

	// Find the location of any moving object and show the final frame
	if (toggle == 0) {
                searchForMovement(result,frame0_warped);
	        imshow("Frame", frame0_warped);
	}
	else {
                searchForMovement(result,frame1_warped);
	        imshow("Frame", frame1_warped);
	}

	frame_count++;

        cv::waitKey(1); //needed to show frame
    }
}
