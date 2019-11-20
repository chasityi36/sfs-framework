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

# include "framework-structure.hpp"

Eigen::Vector3d * Structure::getPosition(){
    return &position;
}

int Structure::getFeaturesCount(){
    return features.size();
}

bool Structure::getBootstrap(int lastViewpointIndex){
    if (features.size()==2){
        for(auto & feature: features){
            if(feature->getViewpoint()->getIndex()==lastViewpointIndex){
                return true;
            }
        }
    }
    return false;
}

bool Structure::getLastViewpointCreated(unsigned int lastViewpointIndex){
    if(originalViewpoint->getIndex()==lastViewpointIndex){
        return true;
    }
    return false;
}

bool Structure::getHasLastViewpoint(int lastViewpointIndex){
    for(auto & feature: features){
        if(feature->getViewpoint()->getIndex()==lastViewpointIndex){
            return true;
        }
    }
    return false;
}

void Structure::setFeaturesState(){
    for(auto & element: features){
        element->setStructurePtr(NULL);
    }
}

void Structure::addFeature(Feature * feature){
    feature->reset();
    feature->setStructurePtr(this);
    features.push_back(feature);
}

void Structure::computeModel(){
    for(auto & element: features){
        element->computeModel();
    }
}

void Structure::computeCentroid(std::vector<std::shared_ptr<Transform>> & transforms){
    int i_index(0), j_index(0);
    for(unsigned int i(0); i<features.size(); i++){
        i_index=features[i]->getViewpoint()->getIndex();
        for(unsigned int j(0); j<features.size(); j++){
            j_index=features[j]->getViewpoint()->getIndex();
            if (i_index-j_index==1){
                transforms[j_index]->pushCentroid(features[j]->getModel(),features[i]->getModel());
            }
        }
    }
}

void Structure::computeCorrelation(std::vector<std::shared_ptr<Transform>> & transforms){
    int i_index(0), j_index(0);
    for(unsigned int i(0); i<features.size(); i++){
        i_index=features[i]->getViewpoint()->getIndex();
        for(unsigned int j(0); j<features.size(); j++){
            j_index=features[j]->getViewpoint()->getIndex();
            if (i_index-j_index==1){
                transforms[j_index]->pushCorrelation(features[j]->getModel(), features[i]->getModel());
            }
        }
    }
}

void Structure::computeOptimalPosition(unsigned int ignoreViewpoint){
    Eigen::Matrix3d wacc(Eigen::Matrix3d::Zero());
    Eigen::Vector3d vacc(Eigen::Vector3d::Zero());
    Eigen::Matrix3d weight;
    Eigen::Vector3d vector;
    for(auto & element: features){
        if(element->getViewpoint()->getIndex()<=ignoreViewpoint){
            vector=(*element->getViewpoint()->getOrientation())*(*element->getDirection());
            weight=Eigen::Matrix3d::Identity()-vector*vector.transpose();
            vacc+=weight*(*element->getViewpoint()->getPosition());
            wacc+=weight;
        }
    }
    position=wacc.inverse()*vacc;
}

void Structure::computeRadius(){
    Eigen::Vector3d fvector;
    Eigen::Vector3d fposition;
    double radius(0.);
    for(auto & element: features){
        fvector=(*element->getViewpoint()->getOrientation())*(*element->getDirection());
        radius=fvector.dot(position-(*element->getViewpoint()->getPosition()));
        fposition=(*element->getViewpoint()->getPosition())+fvector*radius;
        element->setRadius(radius,(fposition-position).norm());
    }
}

bool Structure::filterRadiusClamp(double clampValue, int indexRange){
    for(auto & feature: features){
        if(feature->getViewpoint()->getIndex()>=indexRange){
            if(feature->getRadius()<clampValue){
                return false;
            }
        }
    }
    return true;
}

bool Structure::filterRadiusStatistics(double meanValue, double stdValue, int indexRange){
    for(auto & feature: features){
        if(feature->getViewpoint()->getIndex()>=indexRange){
            if(fabs(feature->getRadius()-meanValue)>stdValue){
                return false;
            }
        }
    }
    return true;
}

bool Structure::filterDisparityStatistics(double stdValue, int indexRange){
    for(auto & feature: features){
        if(feature->getViewpoint()->getIndex()>=indexRange){
            if(feature->getDisparity()>stdValue){
                return false;
            }
        }
    }
    return true;
}

bool Structure::filterTriangulation(double const minAngle, double const maxAngle){
    Eigen::Vector3d iVector;
    Eigen::Vector3d jVector;
    double angleValue(0.);
    for(unsigned int i(0); i<features.size(); i++){
        iVector=(*features[i]->getViewpoint()->getPosition())-position;
        iVector/=iVector.norm();
        for(unsigned int j(i+1); j<features.size(); j++){
            jVector=(*features[j]->getViewpoint()->getPosition())-position;
            jVector/=jVector.norm();
            angleValue=acos(iVector.dot(jVector));
            if((angleValue<minAngle)||(angleValue>maxAngle)){
                return false;
            }
        }
    }
    return true;
}

