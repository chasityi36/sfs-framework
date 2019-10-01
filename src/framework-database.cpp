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

void Database::setPath(std::string recordPath, std::string modelPath){
    ios.setRecordPath( recordPath );
    ios.setModelPath( modelPath );
}

void Database::computeModels(){
    for(unsigned int i(0); i<viewpoints.size(); i++){
        viewpoints[i]->computeModel();
        viewpoints[i]->computeCentroid();
    }
}

void Database::computeCorrelations(){
    for(unsigned int i(0); i<transforms.size(); i++){
        transforms[i]->resetCorrelation();
    }
    for(unsigned int i(0); i<structures.size(); i++){
        structures[i]->computeCorrelation(viewpoints,transforms);
    }
}

void Database::computePoses(){
    for(unsigned int i(1); i<transforms.size(); i++){
        transforms[i]->computePose(viewpoints[i-1].get(),viewpoints[i].get());
    }
}

void Database::computeFrame(){
    viewpoints[0]->resetFrame();
    for(unsigned int i(0); i<transforms.size(); i++){
        transforms[i]->computeFrame(viewpoints[i].get(),viewpoints[i+1].get());
    }
}

void Database::computeRadius(){
    for(unsigned int i(0); i<structures.size(); i++){
        structures[i]->computeRadius(viewpoints);
    }
}

double Database::computeError(){
    double error(0.);
    double candidate(0.);
    for(unsigned int i(0); i<structures.size(); i++){
        candidate=structures[i]->getError(viewpoints);
        if (candidate>error) error=candidate;
    }
    return error;
}

void Database::exportModel(){
    std::string * exportPath(ios.getModelPath());
    std::fstream exportStream;
    exportStream.open((*exportPath)+"/model.xyz",std::ios::out);
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

void Database::exportOdometry(){
    std::string * exportPath(ios.getModelPath());
    std::fstream exportStream;
    exportStream.open((*exportPath)+"/odometry.xyz",std::ios::out);
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
