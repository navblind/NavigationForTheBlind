//ros
#include <ros/ros.h>

//opencv
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <sensor_msgs/Image.h>

//octomap - pavement
#include <nav_msgs/OccupancyGrid.h>

//pointcloud - pavement
#include <pcl_ros/point_cloud.h>
#include <pcl/point_types.h>
#include <sensor_msgs/PointCloud.h>

//looking for home directory
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string>

//file libraries
#include <iostream>
#include <fstream>

//local libraries
#include "pavement_to_marker/pavement_to_marker.h"
#include "pavement_to_cloud/pavement_to_cloud.h"

#define EDGE_MARKER_WIDTH 6
#define EDGE_MARKER_TYPE 8
#define EDGE_MARKER_SHIFT 0

#define EDGE_PAV_VERTICAL_START 150
#define EDGE_PAV_VERTICAL_END 650
#define EDGE_MARKER_VERTICAL_POINT_DIST 50

using namespace cv;

class CvImage
{
  sensor_msgs::ImagePtr toImageMsg() const;

  // Overload mainly intended for aggregate messages that contain
  // a sensor_msgs::Image as a member.
  void toImageMsg(sensor_msgs::Image& ros_image) const;
};

std::string get_directory(std::string file_name, std::string file_num, std::string file_type);
int getLeftPavementPoint(cv::Mat image, int line);
int getRightPavementPoint(cv::Mat image, int line);

int main(int argc, char **argv) {

	//START ros init
	ros::init(argc, argv, "get_video_image_meddle");
	ros::NodeHandle n;

	cv_bridge::CvImage img_bridge;
	sensor_msgs::Image img_msg;
	sensor_msgs::Image img_msg_orig;
	std_msgs::Header header;

	ros::Publisher pub_img = n.advertise<sensor_msgs::Image>("video_image_topic", 1);//output image publisher
	ros::Publisher pub_img_orig = n.advertise<sensor_msgs::Image>("video_image_orig_topic", 1);//output image publisher
	ros::Publisher octomap_pub = n.advertise<nav_msgs::OccupancyGrid>("pavement_map", 1);//map occupancy publisher
    ros::Publisher pub_pav_pointCloud = n.advertise<sensor_msgs::PointCloud> ("pav_pointCloud", 1);//publisher for pavement point cloud
	//END ros init

    //point cloud
    sensor_msgs::PointCloud pointCloud_msg;
    geometry_msgs::Point32 pavPoint;
    pointCloud_msg.header.stamp = ros::Time::now();
    pointCloud_msg.header.frame_id = "map";

    //generate some fake data for our point cloud
    /*for(unsigned int i = 0; i < 50; ++i)
    {
        for(unsigned int j = 0; j < 50; j++)
        {
            for (unsigned int k = 0; k < 50; k++)
            {
                pavPoint.x = i*0.1;
                pavPoint.y = j*0.1;
                pavPoint.z = k*0.1;
                pointCloud_msg.points.push_back(pavPoint);
            }
        }
    }*/

	//init occupancy map
	nav_msgs::OccupancyGrid pavement_octomap;
	pavement_octomap.info.resolution = 0.1;         	// float32
	pavement_octomap.info.width      = 100;           // uint32
	pavement_octomap.info.height     = 1000;           // uint32

	long map[100000];//0 - nothing, 100 - obstacle, 50 - uknown
	for(long i = 0; i < 100000; i++)
	{
		map[i] = 50;
	}
	std::vector<signed char> a(map, map + 100000);
	pavement_octomap.data = a;
		//1m equals to one square - with 30fps video moves 0.03m per frame
		//1m equals 500px at bottom
		//1m equals 250px in the middle
		//1m equals 50px at top
	int elapsedDistanceDiscrete = 0;
	int threeToOneSquare = 0;
	//END init occcupancy map

	//START video directory
	std::string videoDirectory;
	videoDirectory = get_directory("obed", "", "mp4");
	//END video directory

	Mat image;          //Create Matrix to store image
	Mat imageResult;		//Create Matrix to store processed image
	Mat imageOrig;			//Create Matrix to store orig image with detected pavement

	VideoCapture cap;          //initialize capture
	cap.open(videoDirectory.c_str());

	//START edge detection variables
	Point lineStart = Point(100, 100);
	Point lineEnd = Point(300, 300);
	Point lineStartRight = Point(100, 100);
	Point lineEndRight = Point(300, 300);
	int num_of_edge_points = (EDGE_PAV_VERTICAL_END - EDGE_PAV_VERTICAL_START - EDGE_MARKER_VERTICAL_POINT_DIST)/EDGE_MARKER_VERTICAL_POINT_DIST;
	int leftPoint = 0;
	int rightPoint = 0;

	//END edge detection variables

	while (ros::ok())
	{
		cap >> image;
		imageOrig = image;

		cv::blur(image,imageResult,cv::Size(60, 60));//blurr image
		//remove red and blue
		for(int i = 0; i < imageResult.size().width; i++)
		{
			for(int j = 0; j < imageResult.size().height; j++)
			{
				//imageResult.at<cv::Vec3b>(j,i)[0]= 0;
				imageResult.at<cv::Vec3b>(j,i)[1]= 0;
				imageResult.at<cv::Vec3b>(j,i)[2]= 0;
				//increase contrast
				if (imageResult.at<cv::Vec3b>(j,i)[0] > 120)
				{
					imageResult.at<cv::Vec3b>(j,i)[0] = 255;
				}
				else
				{
					imageResult.at<cv::Vec3b>(j,i)[0] = 0;
				}
			}
		}

		/*//START robot moved forward
		threeToOneSquare++;
		if (threeToOneSquare > 2)
		{
			threeToOneSquare = 0;
			elapsedDistanceDiscrete++;
			//START perspective - draw pavement to map
			if (elapsedDistanceDiscrete < 999)//check if end of map
			{
				//1m equals to one sqare - with 30fps video moves 0.03m per frame
				//1m eqals 500px at bottom
				//1m eqals 250px in the middle
				//1m eqals 50px at top
				//map[20 + 100*elapsedDistanceDiscrete] = 100;//demo
				//map[60 + 100*elapsedDistanceDiscrete] = 100;//demo
				leftPoint = getLeftPavementPoint(imageResult, 700);
				rightPoint = getRightPavementPoint(imageResult, 700);
					//to do - check if computed index is valid
				//map[(int)((double)leftPoint/1.2) + 100*elapsedDistanceDiscrete] = 100;
					//to do - check if computed index is valid
				//map[(int)((double)rightPoint/1.2) + 100*elapsedDistanceDiscrete] = 100;
				std::vector<signed char> a(map, map + 100000);
				pavement_octomap.data = a;
			}
			//END perspective - draw pavement to map
		}
		//END robot moved forward*/

		//START draw pavement boundaries
			//left
		lineStart = Point( getLeftPavementPoint(imageResult, EDGE_PAV_VERTICAL_START), EDGE_PAV_VERTICAL_START);
		lineEnd = Point( getLeftPavementPoint(imageResult, EDGE_PAV_VERTICAL_START + EDGE_MARKER_VERTICAL_POINT_DIST), EDGE_PAV_VERTICAL_START + EDGE_MARKER_VERTICAL_POINT_DIST);
		line(imageResult, lineStart, lineEnd, Scalar(0, 255, 0), EDGE_MARKER_WIDTH, EDGE_MARKER_TYPE, EDGE_MARKER_SHIFT);
		line(imageOrig, lineStart, lineEnd, Scalar(0, 255, 0), EDGE_MARKER_WIDTH, EDGE_MARKER_TYPE, EDGE_MARKER_SHIFT);

			//right
		lineStartRight = Point( getRightPavementPoint(imageResult, EDGE_PAV_VERTICAL_START), EDGE_PAV_VERTICAL_START);
		lineEndRight = Point( getRightPavementPoint(imageResult, EDGE_PAV_VERTICAL_START + EDGE_MARKER_VERTICAL_POINT_DIST), EDGE_PAV_VERTICAL_START + EDGE_MARKER_VERTICAL_POINT_DIST);
		line(imageResult, lineStartRight, lineEndRight, Scalar(0, 255, 0), EDGE_MARKER_WIDTH, EDGE_MARKER_TYPE, EDGE_MARKER_SHIFT);
		line(imageOrig, lineStartRight, lineEndRight, Scalar(0, 255, 0), EDGE_MARKER_WIDTH, EDGE_MARKER_TYPE, EDGE_MARKER_SHIFT);

        //putPavementFragmentIntoCloud
        ///vypocet prvych pozicii x
        pavFragment pavFragmentC;
        pavFragmentC.cm.left.start.x = -1000;
        pavFragmentC.cm.right.start.x = 1000;
        pavFragmentC.cm.left.start.y = 50000;
        pavFragmentC.cm.right.start.y = 50000;
        pointCloud_msg.points.clear();
        pavFragmentC.pix.left.start = lineStart;
        pavFragmentC.pix.left.end = lineEnd;
        pavFragmentC.pix.right.start = lineStartRight;
        pavFragmentC.pix.right.end = lineEndRight;
        ROS_ERROR("Trololo!");
        ROS_ERROR("leftStartX %d leftStartY %d leftEndX %d leftEndY %d rightStartX %d rightStartY %d rightEndX %d rightEndY %d", pavFragmentC.pix.left.start.x, pavFragmentC.pix.left.start.y, pavFragmentC.pix.left.end.x, pavFragmentC.pix.left.end.y, pavFragmentC.pix.right.start.x, pavFragmentC.pix.right.start.y, pavFragmentC.pix.right.end.x, pavFragmentC.pix.right.end.y);
        changeToCmX(&pavFragmentC);
        pavFragmentC.cm.left.start.x = pavFragmentC.cm.left.end.x;
        pavFragmentC.cm.right.start.x = pavFragmentC.cm.right.end.x;
        putPavementFragmentIntoCloud(&pointCloud_msg, &pavFragmentC);


        //test line drawing to pointcloud
        /*pointCloud_msg.points.clear();
        pointCm pointCmStart;
        pointCm pointCmEnd;
        pointCmStart.x = 0;
        pointCmStart.y = 0;
        pointCmStart.z = 1;
        pointCmEnd.x = 1000;
        pointCmEnd.y = 1000;
        pointCmEnd.z = 1;
        //putPavementLineIntoCloud(&pointCloud_msg,pointCmStart, pointCmEnd);
        putPavementLineIntoCloud(&pointCloud_msg,pointCmEnd, pointCmStart);
        //pointCmEnd = pointCmStart;
        pointCmStart = pointCmEnd;
        pointCmEnd.x = 1000;
        pointCmEnd.y = 2000;
        pointCmEnd.z = 1;
        //putPavementLineIntoCloud(&pointCloud_msg, pointCmStart, pointCmEnd);
        putPavementLineIntoCloud(&pointCloud_msg, pointCmEnd, pointCmStart);*/

        ///test pavement to cloud functions
        /*cv::Point leftPixStart;
        cv::Point rightPixStart;
        cv::Point leftPixEnd;
        cv::Point rightPixEnd;
        leftPixStart.x = 100;
        leftPixStart.y = 100;
        rightPixStart.x = 300;
        rightPixStart.y = 100;
        leftPixEnd.x = 200;
        leftPixEnd.y = 200;
        rightPixEnd.x = 500;
        rightPixEnd.y = 200;
        int pavementWidthPix = getWidthPix(leftPixStart, rightPixStart);// endLineLeft, endLineRight);
        ROS_ERROR("widthPix %d", pavementWidthPix);
        int pavementWidthCm = getWidthCm(pavementWidthPix, 300);//leftPixStart.y);
        ROS_ERROR("widthCm %d", pavementWidthCm);
        pavementWidthCm = getWidthCm(pavementWidthPix, 50);//leftPixStart.y);
        ROS_ERROR("widthCm %d", pavementWidthCm);
        cv::Point pavCenterPix = getCenterOfPavement(leftPixEnd, rightPixEnd);//endLineLeft, endLineRight);
        ROS_ERROR("pavement center x %d y %d", pavCenterPix.x, pavCenterPix.y);
        cv::Point imgCenterPoint;
        imgCenterPoint.x = PIC_WIDTH_PIX/2;
        imgCenterPoint.y = PIC_HEIGHT_PIX/2;
        int pavCenterFromImgCenterPix = getWidthPix(imgCenterPoint, pavCenterPix);
        ROS_ERROR(" pav center pix dist %d", pavCenterFromImgCenterPix);
        int pavCenterFromImgCenterCm = getWidthCm(abs(pavCenterFromImgCenterPix), rightPixEnd.y);
        ROS_ERROR("pav center cm dist %d", pavCenterFromImgCenterCm);
        pointCm pavCenterCm = getCenterOfPavementCm(pavCenterFromImgCenterCm, pavCenterPix);
        ROS_ERROR("pav center point cm x %d y %d", pavCenterCm.x, pavCenterCm.y);
        pointCm autobus1;
        pointCm autobus2;
        autobus1.x = computeCmX(pavCenterCm, pavementWidthCm, true);
        autobus2.x = computeCmX(pavCenterCm, pavementWidthCm, false);
        ROS_ERROR("cm point left x %d cm point right x %d", autobus1.x, autobus2.x);
        int fragmentLengthCmLeftY = getFragmentLengthCmY(leftPixStart.y, leftPixEnd.y);
        ROS_ERROR("left height y %d", fragmentLengthCmLeftY);
        int fragmentLengthCmRightY = getFragmentLengthCmY(rightPixStart.y, rightPixEnd.y);
        ROS_ERROR("right height y %d", fragmentLengthCmRightY);
        autobus1.y = fragmentLengthCmLeftY;
        putPavementLineIntoCloud(&pointCloud_msg, pointCmStart, autobus1);
        autobus2.y = fragmentLengthCmRightY;
        putPavementLineIntoCloud(&pointCloud_msg, pointCmStart, autobus2);*/

        //sleep(10000);

		for (int i = 0; i < num_of_edge_points; i++)
		{
			lineStart = lineEnd;
			lineEnd = Point(getLeftPavementPoint(imageResult, EDGE_PAV_VERTICAL_START + EDGE_MARKER_VERTICAL_POINT_DIST + EDGE_MARKER_VERTICAL_POINT_DIST*i), EDGE_PAV_VERTICAL_START + EDGE_MARKER_VERTICAL_POINT_DIST + EDGE_MARKER_VERTICAL_POINT_DIST*i);
			line(imageResult, lineStart, lineEnd, Scalar(0, 255, 0), EDGE_MARKER_WIDTH, EDGE_MARKER_TYPE, EDGE_MARKER_SHIFT);
			line(imageOrig, lineStart, lineEnd, Scalar(0, 255, 0), EDGE_MARKER_WIDTH, EDGE_MARKER_TYPE, EDGE_MARKER_SHIFT);

			lineStartRight = lineEndRight;
			lineEndRight = Point( getRightPavementPoint(imageResult, EDGE_PAV_VERTICAL_START + EDGE_MARKER_VERTICAL_POINT_DIST + EDGE_MARKER_VERTICAL_POINT_DIST*i), EDGE_PAV_VERTICAL_START + EDGE_MARKER_VERTICAL_POINT_DIST + EDGE_MARKER_VERTICAL_POINT_DIST*i);
			line(imageResult, lineStartRight, lineEndRight, Scalar(0, 255, 0), EDGE_MARKER_WIDTH, EDGE_MARKER_TYPE, EDGE_MARKER_SHIFT);
			line(imageOrig, lineStartRight, lineEndRight, Scalar(0, 255, 0), EDGE_MARKER_WIDTH, EDGE_MARKER_TYPE, EDGE_MARKER_SHIFT);

            //put pavement fragment into cloud
            pavFragmentC.pix.left.start = lineStart;
            pavFragmentC.pix.left.end = lineEnd;
            pavFragmentC.pix.right.start = lineStartRight;
            pavFragmentC.pix.right.end = lineEndRight;
            ROS_ERROR("leftStartX %d leftStartY %d leftEndX %d leftEndY %d rightStartX %d rightStartY %d rightEndX %d rightEndY %d", pavFragmentC.pix.left.start.x, pavFragmentC.pix.left.start.y, pavFragmentC.pix.left.end.x, pavFragmentC.pix.left.end.y, pavFragmentC.pix.right.start.x, pavFragmentC.pix.right.start.y, pavFragmentC.pix.right.end.x, pavFragmentC.pix.right.end.y);
            putPavementFragmentIntoCloud(&pointCloud_msg, &pavFragmentC);
		}

		//START perspective conversion
		//put pavement to octomap
		std::vector<signed char> a(map, map + 100000);
		pavement_octomap.data = a;
		//END perspective conversion

		//publish processed image
		header.stamp = ros::Time::now();
		img_bridge = cv_bridge::CvImage(header, sensor_msgs::image_encodings::RGB8, imageResult);
		img_bridge.toImageMsg(img_msg);
		pub_img.publish(img_msg);//publish processed image
		img_bridge = cv_bridge::CvImage(header, sensor_msgs::image_encodings::RGB8, imageOrig);
		img_bridge.toImageMsg(img_msg_orig);
		pub_img_orig.publish(img_msg_orig);//publish original image
		//octomap_pub.publish(pavement_octomap);//publish octomap
		//usleep(10000);

        //publish point cloud
        pointCloud_msg.header.stamp = ros::Time::now();
        pub_pav_pointCloud.publish(pointCloud_msg);
    }


    return 0;
}

std::string get_directory(std::string file_name, std::string file_num, std::string file_type)
{
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;
	std::string file_directory;
	file_directory = homedir;
	file_directory = file_directory + "/Videos/MRVKroute/" + file_name + file_num + "." + file_type;
	return file_directory;
}

int getLeftPavementPoint(cv::Mat image, int line)
{
	int isGrass = 0;
	int isEdge = 0;
	for (int i = 0; i < image.size().width; i++)
	{
		if (image.at<cv::Vec3b>(line,i)[0] == 0)
		{
			isGrass = 1;
		}
		else if ((image.at<cv::Vec3b>(line,i)[0] == 255) && (isGrass == 1) && (isEdge == 0))
		{
			isGrass = 0;
			isEdge = 1;
			return i;
		}
	}
	return 0;
}

int getRightPavementPoint(cv::Mat image, int line)
{
	int isGrass = 0;
	int isEdge = 0;
	for (int i = image.size().width; i > 0; i--)
	{
		if (image.at<cv::Vec3b>(line,i)[0] == 0)
		{
			isGrass = 1;
		}
		else if ((image.at<cv::Vec3b>(line,i)[0] == 255) && (isGrass == 1) && (isEdge == 0))
		{
			isGrass = 0;
			isEdge = 1;
			return i;
		}
	}
	return 0;
}
