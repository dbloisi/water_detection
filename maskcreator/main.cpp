/*
 *
 *
 *  Please, report suggestions/comments/bugs to
 *  domenico.bloisi@gmail.com
 *
 */

//C
#include <stdio.h>
//C++
#include <iostream>
#include <fstream>
#include <string>
//OpenCV
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>



#include "imagemanager.h"


using namespace cv;
using namespace std;

/**
 * Global variables
 */
Mat frame;
Mat mask;
Mat gui_frame;
double fps;   //frame per second for the input video sequence
int keyboard;  //input from keyboard
int counter = 0;

vector<Point> points;

int width = -1;
int height = -1;

ofstream outputfile;

/**
 * Function Headers
*/
void help();
void processVideo(char* videoFilename, double _fps=-1.);
void processImages(char* firstFrameFilename);

static void onMouse(int event, int x, int y, int, void*);

/**
* @function help
*/
void help()
{
    cout
    << "--------------------------------------------------------------------------" << endl
                                                                                    << endl
    << "written by Domenico D. Bloisi"                                              << endl
    << "domenico.bloisi@gmail.com"                                                  << endl
    << "--------------------------------------------------------------------------" << endl
    << "You can process both videos (-vid) and images (-img)."                      << endl
                                                                                    << endl
    << "Usage:"                                                                     << endl
    << "maskcreator {-vid <video filename>|-img <image filename> [-w <width> -h <height>]}" << endl
    << "for example: maskcreator -vid video.avi"                                    << endl
    << "or: maskcreator -img /data/images/1.png"                                    << endl
    << "or: maskcreator -vid /data/video/movie.avi -w 480 -h 360"                   << endl
    << "--------------------------------------------------------------------------" << endl
                                                                                    << endl;
}

/**
* @function main
*/
int main(int argc, char* argv[])
{

	//print help information
	help();
	
	namedWindow("frame");
	setMouseCallback("frame", onMouse, NULL);
	
  outputfile.open("training_data_list.txt", ios_base::app);
  if(!outputfile.is_open()) {
      cout << "Unable to create training_data_list.txt" << endl;
      return EXIT_FAILURE;
  }

	//check for the input parameter correctness
	if(argc < 3) {
		cerr <<"Incorrect input list" << endl;
		cerr <<"exiting..." << endl;
		return EXIT_FAILURE;
	}
	if(argc > 6) {
	  for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0) {
            height = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-w") == 0) {
            width = atoi(argv[++i]);
        }
    }
	}
		
	if(strcmp(argv[1], "-vid") == 0) {
		processVideo(argv[2]);
	}
  else if(strcmp(argv[1], "-img") == 0) {
		
		processImages(argv[2]);
  }
	else {
		//error in reading input parameters
		cerr <<"Please, check the input parameters." << endl;
		cerr <<"Exiting..." << endl;
		return EXIT_FAILURE;
	}
	//destroy GUI windows
	destroyAllWindows();
	return EXIT_SUCCESS;
}

/**
* @function processVideo
*/
void processVideo(char* videoFilename, double _fps) {
    //create the capture object
    VideoCapture capture(videoFilename);
    if(!capture.isOpened()){
        //error in opening the video input
        cerr << "Unable to open video file: " << videoFilename << endl;
        exit(EXIT_FAILURE);
    }	
	if(_fps < 0.)    
		fps = capture.get(5); //CV_CAP_PROP_FPS
	else
		fps = _fps;
	
    if(fps != fps) { //check for nan value
		fps = 25.;
	}	
	//std::cout << "Number of cores:" << std::thread::hardware_concurrency() << std::endl;
	
    

	//read input data. ESC or 'q' for quitting
	int sleepTime = 0;
    while( (char)keyboard != 'q' && (char)keyboard != 27 ){
		
		//read the current frame
        if(!capture.read(frame)) {
            cerr << "Unable to read next frame." << endl;
            cerr << "Exiting..." << endl;		
            exit(EXIT_FAILURE);
        }
     
     if(width > 0 && height > 0) {
        Mat resized_frame(height, width, frame.type());
        resize(frame, resized_frame, resized_frame.size());
        frame = resized_frame.clone();
     }
     gui_frame = frame.clone();

		stringstream stream;
		rectangle(gui_frame, cv::Point(10, 2), cv::Point(100,20),cv::Scalar(255,255,255), -1);
        stream << capture.get(1);
		string frameNumberString = stream.str();
		putText(gui_frame, frameNumberString.c_str(), cv::Point(15, 15),FONT_HERSHEY_SIMPLEX, 0.5 , cv::Scalar(0,0,0));
		
		//show the current frame and the fg masks
        imshow("frame", gui_frame);
        
        if(keyboard == 's') {
            if(sleepTime != 0 || !mask.data) {
                keyboard = waitKey(sleepTime);
                continue;
            }
            cout << "saving image and mask..." << endl;
            cout.flush();
            string s(videoFilename);
            int index1 = s.find_last_of("/");
            s = s.substr(index1+1);
            int index2 = s.find_last_of(".");
            s = s.substr(0, index2);
            //raw
            string imageprefix = "/home/bloisi/water_detection/training_data/img/";           
            string imagename = imageprefix + "img_" + s + "_" + frameNumberString + ".png";
            outputfile << imagename;
            if(!imwrite(imagename, frame)) {
                cout << "Unable to save " << imagename << endl;
            }
            else {
                cout << "image " << imagename << " saved." << endl;
            }
            outputfile << " ";
            //mask
            string maskprefix = "/home/bloisi/water_detection/training_data/mask/";
            string maskname = maskprefix + "mask_" + s + "_" + frameNumberString + ".png";
            outputfile << maskname;
            if(!imwrite(maskname, mask)) {
                cout << "Unable to save " << maskname << endl;
            }
            else {
                cout << "mask " << maskname << " saved." << endl;
            }
            outputfile.flush();
            points.clear();
            mask.release();
        }
        else if(keyboard == 'b') {
            sleepTime = 0;
        }
        else if(keyboard == 'f') {
            sleepTime = 30;
        }
        else if(keyboard == 'c') {
            points.clear();
        }
        		
		keyboard = waitKey(sleepTime);
    }
    //delete capture object
    capture.release();
}

/**
* @function processImages
*/
void processImages(char* firstFrameFilename) {	

	string foldername(firstFrameFilename);
	size_t folder_index = foldername.find_last_of("/");
    if(folder_index == string::npos) {
    	folder_index = foldername.find_last_of("\\");
    }
    foldername = foldername.substr(0,folder_index+1);
    
	ImageManager *im = new ImageManager(foldername);

    //read the first file of the sequence
	string s = im->next(1);
	size_t index = s.find_last_of("/"); 
	if (index != string::npos) {
		s.erase(s.begin() + index);
	}

    //frame = imread(foldername+s);
	frame = imread(s);

    if(!frame.data){
        //error in opening the first image
        cerr << "Unable to open first image frame: " << s << endl;
        exit(EXIT_FAILURE);
    }
	
    

    int frameRateCounter = 0;

    //read input data. ESC or 'q' for quitting
	int sleepTime = 30;
    while( (char)keyboard != 'q' && (char)keyboard != 27 ){
    	

        ++frameRateCounter;

        stringstream stream;
        rectangle(frame, cv::Point(10, 2), cv::Point(100,20),cv::Scalar(255,255,255), -1);
        stream << frameRateCounter;
        string frameNumberString = stream.str();
        putText(frame, frameNumberString.c_str(), cv::Point(15, 15),FONT_HERSHEY_SIMPLEX, 0.5 , cv::Scalar(0,0,0));


        rectangle(frame, cv::Point(10, 2), cv::Point(100,20),
                  cv::Scalar(255,255,255), -1);
        putText(frame, frameNumberString.c_str(), cv::Point(15, 15),
                FONT_HERSHEY_SIMPLEX, 0.5 , cv::Scalar(0,0,0));
        //show the current frame and the fg masks
        imshow("frame", frame);

		keyboard = waitKey(sleepTime);
        
        //read next frame
        s = im->next(1);
		index = s.find_last_of("/");
		if (index != string::npos) {
			s.erase(s.begin() + index);
		}

        frame = imread(s);
        if(!frame.data){
            //error in opening the next image in the sequence
            cerr << "Unable to open image frame: " << s << endl;
            exit(EXIT_FAILURE);
        }        
    }	
}

static void onMouse(int event, int x, int y, int flags, void* userdata)
{
     if  ( event == EVENT_LBUTTONDOWN )
     {
          //cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
          points.push_back(Point(x,y));
          
          circle(gui_frame, Point(x,y), 4, Scalar(0,0,255), 2);
          imshow("frame", gui_frame);
     }
     else if  ( event == EVENT_RBUTTONDOWN )
     {
          //cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
          cv::fillConvexPoly(gui_frame,               //Image to be drawn on
                   points,                 //C-Style array of points
                   Scalar(255,0,0),  //Color , BGR form
                   CV_AA,             // connectedness, 4 or 8
                   0);            // Bits of radius to treat as fraction
          mask = Mat::zeros(frame.size(), CV_8UC1);
          cv::fillConvexPoly(mask,               //Image to be drawn on
                   points,                 //C-Style array of points
                   Scalar(1),  //Color , BGR form
                   CV_AA,             // connectedness, 4 or 8
                   0);            // Bits of radius to treat as fraction
          imshow("frame", gui_frame);
          imshow("mask", mask);
          
          
     }
     else if  ( event == EVENT_MBUTTONDOWN )
     {
          //cout << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
     }
     else if ( event == EVENT_MOUSEMOVE )
     {
          //cout << "Mouse move over the window - position (" << x << ", " << y << ")" << endl;

     }
}


