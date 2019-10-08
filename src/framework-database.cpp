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

#include "framework-database.hpp"

int Database::getViewpointCount(){
    return viewpoints.size();
}

void Database::computeModels(){
    for(unsigned int i(0); i<viewpoints.size(); i++){
        viewpoints[i]->computeModel();
    }
}

void Database::computeCorrelations(){
    for(unsigned int i(0); i<transforms.size(); i++){
        transforms[i]->resetCorrelation();
    }
    for(unsigned int i(0); i<structures.size(); i++){
        structures[i]->computeCorrelation(transforms);
    }
}

void Database::computeCentroids(){
    for(unsigned int i(0); i<viewpoints.size(); i++){
        viewpoints[i]->computeCentroid();
    }
}

void Database::computePoses(){
    for(unsigned int i(0); i<transforms.size(); i++){
        transforms[i]->computePose(viewpoints[i].get(),viewpoints[i+1].get());
    }
}

void Database::computeFrame(){
    viewpoints[0]->resetFrame();
    for(unsigned int i(0); i<transforms.size(); i++){
        transforms[i]->computeFrame(viewpoints[i].get(),viewpoints[i+1].get());
    }
}

void Database::computeOptimal(){
    for(unsigned int i(0); i<structures.size(); i++){
        structures[i]->computeOptimalPosition();
    }
}

void Database::computeRadius(){
    for(unsigned int i(0); i<structures.size(); i++){
        structures[i]->computeRadius(viewpoints);
    }
}

void Database::computeStatistics(){
    unsigned int count(0);

    disparityMean=0.;
    radiusMean=0.;
    disparityMax=0.;
    for(unsigned int i(0); i<structures.size(); i++){
        for(unsigned int j(0); j<structures[i]->getFeatureCount(); j++){
            disparityMean+=structures[i]->getDisparity(j);
            radiusMean+=structures[i]->getRadius(j);
            count++;
            if(disparityMax<structures[i]->getDisparity(j)){
                disparityMax=structures[i]->getDisparity(j);
            }
        }
    }
    disparityMean/=double(count);
    radiusMean/=double(count);

    disparitySD=0.;
    radiusSD=0.;
    for(unsigned int i(0); i<structures.size(); i++){
        for(unsigned int j(0); j<structures[i]->getFeatureCount(); j++){
            double component(structures[i]->getDisparity(j)-disparityMean);
            disparitySD+=component*component;
            component=structures[i]->getRadius(j)-radiusMean;
            radiusSD+=component*component;
        }
    }
    disparitySD=std::sqrt(disparitySD/double(count));
    radiusSD=std::sqrt(radiusSD/double(count));
}


//Issue index of following elements will be modifed, can't be use in computeFilter as this
//void Database::deleteAndUnlinkStructure(int i){
//	for(auto f : *structures[i]->getFeatures()){
//		f->structure = NULL;
//	}
//	std::swap(structures[i],structures[structures.size()-1]);
//  structures.resize(structures.size()-1);
//}

void Database::computeFilter(double dispTolerence, double radTolerence){
    unsigned int i(0);
    unsigned int j(structures.size());
    while ( i<j ){
        //if ( structures[i]->computeFilter(dispTolerence, triTolerence, disparitySD) == false ){
        if (structures[i]->computeFilter(disparitySD,radiusMean,radiusSD,dispTolerence,radTolerence)==false){
        	for(auto f : *structures[i]->getFeatures()){
        		f->structure = NULL;
        	}
            std::swap(structures[i],structures[--j]);
        } else {
            i++;
        }
    }
    structures.resize(j);
}

double Database::computeError(){
    return disparityMax;
}

void Database::exportModel(std::string path){
    std::fstream exportStream;
    exportStream.open(path+"/model.xyz",std::ios::out);
    if (exportStream.is_open() == false){
        std::cerr << "unable to create model exportation file" << std::endl;
        return;
    }
    for(unsigned int i(0); i<structures.size(); i++){
        Eigen::Vector3d * position(structures[i]->getPosition());
        exportStream << (*position)(0) << " " << (*position)(1) << " " << (*position)(2) << std::endl;
    }
    exportStream.close();
}

void Database::exportOdometry(std::string path){
    std::fstream exportStream;
    exportStream.open(path+"/odometry.xyz",std::ios::out);
    if (exportStream.is_open() == false){
        std::cerr << "unable to create odometry exportation file" << std::endl;
        return;
    }
    for(unsigned int i(0); i<viewpoints.size(); i++){
        Eigen::Vector3d * position(viewpoints[i]->getPosition());
        exportStream << (*position)(0) << " " << (*position)(1) << " " << (*position)(2) << std::endl;
    }
    exportStream.close();
}

static cv::Point f2i(Eigen::Vector2f value){
	return cv::Point(value[0],value[1]);
}


//Do  cv::waitKey(0); if you want to stop after it.
void Database::displayViewpointStructures(Viewpoint *viewpoint){
	cv::RNG rng(12345);
	cv::Rect myROI(0, 0, viewpoint->getImage()->cols, viewpoint->getImage()->rows);
	cv::Mat res(myROI.width,myROI.height, CV_8UC3, cv::Scalar(0,0,0));
	res = *viewpoint->getImage();
	for(auto f : *viewpoint->getFeatures()){
		if(!f.structure) continue;
		cv::Scalar color = cv::Scalar(rng.uniform(0,255), rng.uniform(0, 255), rng.uniform(0, 255));

		auto features = f.structure->getFeatures();
		for(uint32_t idx = 1;idx < features->size();idx++){
			cv::line(res, f2i((*features)[idx-1]->position),  f2i((*features)[idx]->position), color, 2);
		}
	}

	cv::namedWindow( "miaou", cv::WINDOW_KEEPRATIO );
	imshow( "miaou", res);
}

//
//  development related features
//

void Database::_exportState(std::string path){
    std::fstream stream;

    for(unsigned int i(0); i<viewpoints.size(); i++){
        stream.open( path + "/viewpoint_direction_" + std::to_string(i) + ".xyz", std::ios::out );
        if ( stream.is_open() == false ) { std::cerr << "debug : error on stream" << std::endl; return; }
        for(unsigned int j(0); j<viewpoints[i]->features.size(); j++ ){
            stream << viewpoints[i]->features[j].direction(0) << " "
                   << viewpoints[i]->features[j].direction(1) << " "
                   << viewpoints[i]->features[j].direction(2) << " 255 0 0 " << std::endl;
        }
        stream.close();
    }

    for(unsigned int i(0); i<viewpoints.size(); i++){
        stream.open( path + "/viewpoint_model_" + std::to_string(i) + ".xyz", std::ios::out );
        if ( stream.is_open() == false ) { std::cerr << "debug : error on stream" << std::endl; return; }
        for(unsigned int j(0); j<viewpoints[i]->features.size(); j++ ){
            stream << viewpoints[i]->features[j].model(0) << " "
                   << viewpoints[i]->features[j].model(1) << " "
                   << viewpoints[i]->features[j].model(2) << " 255 0 0 " << std::endl;
        }
        stream << viewpoints[i]->centroid(0) << " "
               << viewpoints[i]->centroid(1) << " "
               << viewpoints[i]->centroid(2) << " 0 255 0" << std::endl;
        stream.close();
    }

    stream.open( path + "/model.xyz", std::ios::out );
    for(unsigned int i(0); i<viewpoints.size(); i++){
        stream << viewpoints[i]->position(0) << " "
               << viewpoints[i]->position(1) << " "
               << viewpoints[i]->position(2) << " 0 0 255" << std::endl;
    }
    for(unsigned int i(0); i<structures.size(); i++){
        stream << structures[i]->position(0) << " "
               << structures[i]->position(1) << " "
               << structures[i]->position(2) << " 255 0 0" << std::endl;
    }

    stream.close();

}

void Database::_exportMatch(std::string path){
    std::fstream stream;
    for(unsigned int i(0); i<structures.size(); i++){
        for(unsigned int j(0); j<structures[i]->features.size(); j++){
            for(unsigned int k(0); k<structures[i]->features.size(); k++){
                int jindex = structures[i]->features[j]->viewpoint->getIndex();
                int kindex = structures[i]->features[k]->viewpoint->getIndex();
                if ( jindex - kindex == 1 )
                    stream.open( path + "/match_" + std::to_string(kindex) + "_" + std::to_string(jindex), std::ios::app );
                    stream << structures[i]->features[k]->position(0) << " "
                           << structures[i]->features[k]->position(1) << " "
                           << structures[i]->features[j]->position(0) << " "
                           << structures[i]->features[j]->position(1) << std::endl;
                    stream.close();
            }
        }
    }
}

