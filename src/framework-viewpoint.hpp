/*
 *  sfs-framework
 *
 *      Nils Hamel - nils.hamel@bluewin.ch
 *      Charles Papon - charles.papon.90@gmail.com
 *      Copyright (c) 2019 DHLAB, EPFL & HES-SO Valais-Wallis
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <Eigen/Dense>
#include <opencv4/opencv2/core/types.hpp>
#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/features2d.hpp>
#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include "framework-feature.hpp"

class Viewpoint {
//private:
public:
    std::string uid;
    int index;
	cv::Mat image;
    int width;
    int height;
	std::vector<Feature> features;
	std::vector<cv::KeyPoint> cvFeatures;
	cv::Mat cvDescriptor;
    Eigen::Matrix3d orientation;
    Eigen::Vector3d position;
public:
    time_t time;
    int microsecond;
	void setImage(cv::Mat &image){this->image = image;}
	cv::Mat* getImage(){return &this->image;}
	void setCvFeatures(std::vector<cv::KeyPoint> &image){this->cvFeatures = cvFeatures;}
	std::vector<cv::KeyPoint>* getCvFeatures(){return &this->cvFeatures;}
	void setCvDescriptor(cv::Mat &image){this->cvDescriptor = cvDescriptor;}
	cv::Mat* getCvDescriptor(){return &this->cvDescriptor;}
	void setPosition(Eigen::Vector3d position){this->position = position;}
	std::vector<Feature> *getFeatures() { return &features; }
    int getIndex();
    Eigen::Matrix3d * getOrientation();
    Eigen::Vector3d * getPosition();
    double getDisparity(int featID);
    void resetFrame();
    void setIndex(int newIndex);
    void setImageDimension(int newWidth, int newHeight);
    void setPose(Eigen::Matrix3d newOrientation, Eigen::Vector3d newPosition);
    void allocateFeaturesFromCvFeatures();
    double getSecondFrom(Viewpoint *ref);
    Feature *getFeatureFromCvIndex(uint32_t i){ return &features[i]; }
//	GETSET(cv::Mat, Image);
//	GETSET(std::vector<cv::KeyPoint> , Features);
//	GETSET(cv::Mat, Descriptor);
};
