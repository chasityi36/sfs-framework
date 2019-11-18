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

Database::Database(double initialError, double initialDisparity, double initialRadius){
    configError=initialError;
    configDisparity=initialDisparity;
    configRadius=initialRadius;
}

bool Database::getBootstrap(){
    return viewpoints.size()<DB_LOOP_BOOT_COUNT?true:false;
}

double Database::getConfigError(){
    return configError;
}

/* encapsulation fault - adding disparity in addition to position */
double Database::getError(){
    return (viewpoints.back()->position - viewpoints.front()->position).norm() + maxValue;
}

double Database::getTranslationMeanValue(){
    double meanValue(0.);
    for(auto & element: transforms){
        meanValue+=element->getTranslation()->norm();
    }
    return meanValue/double(transforms.size());
}

void Database::getLocalViewpoints(Eigen::Vector3d position, std::vector<std::shared_ptr<Viewpoint>> *localViewpoints){
    int localCount = MIN(5, viewpoints.size());
    for(auto i = viewpoints.end()-localCount;i != viewpoints.end(); ++i){
        localViewpoints->push_back(*i);
    }
}

void Database::addViewpoint(std::shared_ptr<Viewpoint> viewpoint){
    if(viewpoint->getIndex() > 0) transforms.push_back(std::make_shared<Transform>());
    viewpoints.push_back(viewpoint);
}

void Database::aggregate(std::vector<std::shared_ptr<Viewpoint>> *localViewpoints, Viewpoint *newViewpoint, uint32_t *correlations){
    uint32_t localViewpointsCount = localViewpoints->size();
    Structure** structures = new Structure*[localViewpointsCount];
    uint32_t* structuresOccurences = new uint32_t[localViewpointsCount];
    uint32_t structureNewCount = 0;
    uint32_t structureAggregationCount = 0;
    uint32_t structureFusionCount = 0;
    uint32_t *viewpointsUsage = new uint32_t[viewpoints.size() + 1];
    memset(viewpointsUsage, -1, (viewpoints.size() + 1)*sizeof(uint32_t));
    newViewpoint->setIndex(viewpoints.size());
    for(uint32_t queryIdx = 0;queryIdx < newViewpoint->features.size(); queryIdx++){
        uint32_t *correlationsPtr = correlations + queryIdx*localViewpointsCount; //Used to iterate over the given lines

        uint32_t structuresCount = 0;
        uint32_t matchCount = 0;

        //Collect all the existing structures of the matches and count their occurences
        for(uint32_t localIdx = 0;localIdx < localViewpointsCount;localIdx++){
            uint32_t trainIdx = correlationsPtr[localIdx];
            if(trainIdx != 0xFFFFFFFF){
                matchCount++;
                auto localFeature = (*localViewpoints)[localIdx]->getFeatureFromCvIndex(trainIdx);
                auto localStructure = localFeature->structure;
                if(localStructure){
                    uint32_t cacheIdx;
                    for(cacheIdx = 0;cacheIdx < structuresCount; cacheIdx++){
                        if(structures[cacheIdx] == localStructure){
                            structuresOccurences[cacheIdx]++;
                            break;
                        }
                    }
                    if(cacheIdx == structuresCount){ //No cache hit
                        structures[cacheIdx] = localStructure;
                        structuresOccurences[cacheIdx] = 1;
                        structuresCount++;
                    }
                }
            }
        }

        //Figure out which structure will be used to integrate the newViewpoint feature
        if(matchCount == 0) continue; //No match => no integration
        Structure *structure = NULL;
        switch(structuresCount){
            case 0: {
                // no more needed - direct access to structure
                structure = this->newStructure();
                structureNewCount++;
            }break;
            case 1: {
                if(structuresOccurences[0] < 2) continue; //Not good enough
                structure = structures[0];
                structureAggregationCount++;
            }break;
            default: {
                structureFusionCount++;
                continue;
            }break;
        }

        //Populate pre-existing viewpoint usage from the structure
        for(auto f : structure->features){
            viewpointsUsage[f->viewpoint->index] = queryIdx;
        }

        //Integrate all orphan feature into the common structure
        for(uint32_t localIdx = 0;localIdx < localViewpointsCount;localIdx++){
            uint32_t trainIdx = correlationsPtr[localIdx];
            if(trainIdx != 0xFFFFFFFF){
                auto localFeature = (*localViewpoints)[localIdx]->getFeatureFromCvIndex(trainIdx);
                auto viewpointId = (*localViewpoints)[localIdx]->index;
                if(!localFeature->structure && viewpointsUsage[viewpointId] != queryIdx){
                    viewpointsUsage[viewpointId] = queryIdx;
                    structure->addFeature((*localViewpoints)[localIdx]->getFeatureFromCvIndex(trainIdx));
                }
            }
        }

        auto newFeature = newViewpoint->getFeatureFromCvIndex(queryIdx);
        if(newFeature->structure) throw std::runtime_error("New feature already had a structure");
        if(viewpointsUsage[viewpoints.size()] != queryIdx){
            structure->addFeature(newFeature);
        }
    }
    delete[] viewpointsUsage;
    delete[] structures;
    delete[] structuresOccurences;
    std::cout << "structureNewCount=" << structureNewCount << " structureAggregationCount=" << structureAggregationCount << " structureFusionCount=" << structureFusionCount << std::endl;

}

/* experimental - not used yet */
void Database::prepareStructure(){

    // Last viewpoint index
    unsigned int lastViewpoint(viewpoints.size()-1);

    // Continuous indexation
    unsigned int index(0);

    // Copy structures vector
    std::vector<std::shared_ptr<Structure>> unsorted(structures);

    sortStructTypeA=0;
    sortStructTypeB=0;

    // Type-based detection - Type A
    for(auto & structure: unsorted){
        if(structure->getFeaturesCount()==2){
            if(structure->getHasLastViewpoint(lastViewpoint)==true){
                structures[index++]=structure;
                sortStructTypeA++;
            }
        }
    }

    // Type-based detection - Type B
    for(auto & structure: unsorted){
        if(structure->getFeaturesCount()>2){
            if(structure->getHasLastViewpoint(lastViewpoint)==true){
                structures[index++]=structure;
                sortStructTypeB++;
            }
        }
    }

    // Type-based detection - Type C
    for(auto & structure: unsorted){
        if(structure->getHasLastViewpoint(lastViewpoint)==false){
            structures[index++]=structure;
        }
    }

    // development feature - begin
    if(index!=structures.size()){
        std::cerr << "Fault : " << index << " vs " << structures.size() << std::endl;
    }
    // development feature - end

}

void Database::removeStructure(unsigned int structureIndex){

    // Remove structure
    for(unsigned int i(structureIndex+1); i<structures.size();i++){
        structures[i-1]=structures[i];
    }

    // Resize array
    structures.pop_back();

}

void Database::computeModels(){

    // Compute model for all features of all structures
    //
    // Translation are renormalised taking into account the last estimated one
    // leading to a re-computation of all optimal intersection and radius.
    for(auto & structure: structures){
        structure->computeModel();
    }
}

void Database::computeCentroids(int loopState){

    // Active transformation start index
    unsigned int transformationStart(0);

    // Active structure range lower bound
    unsigned int structureStart(0);

    // Active structure range upper bound
    unsigned int structureRange(structures.size());

    // Check pipeline state
    if(loopState==DB_LOOP_MODE_LAST){

        // Update active transformation
        transformationStart=transforms.size()-1;

        // Update structure range
        structureStart=sortStructTypeA;
        structureRange=sortStructTypeA+sortStructTypeB;

    }

    // Reset active transformations centroid
    for(unsigned int i(transformationStart); i<transforms.size(); i++){
        transforms[i]->resetCentroid();
    }

    // Compute centroid contribution for active structures
    for(unsigned int i(structureStart); i<structureRange; i++) {
        structures[i]->computeCentroid(transforms);
    }

    // Compute active transformation centroid
    for(unsigned int i(transformationStart); i<transforms.size(); i++){
        transforms[i]->computeCentroid();
    }

}

void Database::computeCorrelations(int loopState){

    // Active transformation start index
    unsigned int transformationStart(0);

    // Active structure range lower bound
    unsigned int structureStart(0);

    // Active structure range upper bound
    unsigned int structureRange(structures.size());

    // Check pipeline state
    if(loopState==DB_LOOP_MODE_LAST){

        // Update active transformation
        transformationStart=transforms.size()-1;

        // Update structure range
        structureStart=sortStructTypeA;
        structureRange=sortStructTypeA+sortStructTypeB;

    }

    // Reset active transformation correlation matrix
    for(unsigned int i(transformationStart); i<transforms.size(); i++){
        transforms[i]->resetCorrelation();
    }

    // Compute correlation matrix correlation for active structures
    for(unsigned int i(structureStart); i<structureRange; i++){
        structures[i]->computeCorrelation(transforms);
    }

}

void Database::computePoses(int loopState){

    // Active transformation start index
    unsigned int transformationStart(0);

    // Transformation translation mean value
    double normalValue(0.);

    // Check pipeline state
    if(loopState==DB_LOOP_MODE_LAST){

        // Update active transformation
        transformationStart=transforms.size()-1;

    }

    // Compute transformation (pose)
    for(unsigned int i(transformationStart); i<transforms.size(); i++){
        transforms[i]->computePose();
    }

    // Compute translation mean value
    normalValue=getTranslationMeanValue();

    // Renormalise transformations translation
    for(unsigned int i(0); i<transforms.size(); i++){
        transforms[i]->setTranslationScale(normalValue);
    }

}

void Database::computeFrames(){

    // Compute absolute frame for the viewpoints
    //
    // Need to be done for all viewpoints as the translations are re-normalised
    // at each iteration

    // Assign identity and zero position to first viewpoint
    viewpoints[0]->resetFrame();

    // Compute viewpoint absolute orientation and position
    for(unsigned int i(0); i<transforms.size(); i++){
        transforms[i]->computeFrame(viewpoints[i].get(),viewpoints[i+1].get());
    }
}

void Database::computeOptimals(long loopState){

    // Compute optimal intersection of structure
    //
    // Need to be done for all structures as the translation are re-normalised
    // at each iteration

    // Active viewpoints stop index
    unsigned int viewpointLast(viewpoints.size()-1);

    // Compute optimal structures position for type A
    for(unsigned int i(0); i<sortStructTypeA; i++){
        structures[i]->computeOptimalPosition(viewpointLast);
    }

    // Check pipeline state
    if(loopState==DB_LOOP_MODE_LAST){

        // Update active viewpoints
        viewpointLast=viewpoints.size()-2;

    }

    // Compute optimal structures position for type B and C
    for(unsigned int i(sortStructTypeA); i<structures.size(); i++){
        structures[i]->computeOptimalPosition(viewpointLast);
    }

}

void Database::computeRadii(long loopState){

    // Compute feature radius correction
    //
    // Need to be done after structures optimal position computation. As the
    // optimal position is recomputed at each iteration, the radius correction
    // has to be made for each feature of each structure

    for(unsigned int i(0); i<structures.size(); i++){
        structures[i]->computeRadius();
    }

}

void Database::computeStatistics(long loopState, double(Feature::*getValue)()){

    // Count increment
    unsigned int countValue(0);

    // Standard deviation component
    double componentValue(0.);

    // Active viewpoint start index
    unsigned int ignoreViewpoint(0);

    // Active structure range
    unsigned int structureRange(structures.size());

    // Check pipeline state
    if(loopState==DB_LOOP_MODE_LAST){

        // Update active viewpoints
        ignoreViewpoint=viewpoints.size()-1;

        // Update structures range
        structureRange=sortStructTypeA;

    }

    // Compute mean value
    meanValue=0.;
    for(unsigned int i(0); i<structureRange; i++){
        for(auto & feature: structures[i]->features){ /* encapsulation fault */
            if(feature->getViewpoint()->getIndex()>=ignoreViewpoint){ /* encapsulation fault */
                meanValue+=(feature->*getValue)(); /* encapsulation fault */
                countValue++;
            }
        }
    }
    meanValue/=double(countValue);

    // Compute standard deviation
    stdValue=0.;
    maxValue=0.;
    for(unsigned int i(0); i<structureRange; i++){
        for(auto & feature: structures[i]->features){ /* encapsulation fault */
            if(feature->getViewpoint()->getIndex()>=ignoreViewpoint){ /* encapsulation fault */
                componentValue=(feature->*getValue)()-meanValue; /* encapsulation fault */
                stdValue+=componentValue*componentValue;
                if(maxValue<(feature->*getValue)()){ /* encapsulation fault */
                    maxValue=(feature->*getValue)(); /* encapsulation fault */
                }
            }
        }
    }
    stdValue=std::sqrt(stdValue/(countValue-1));

}

//Issue index of following elements will be modifed, can't be use in computeFilter as this
//void Database::deleteAndUnlinkStructure(int i){
//    for(auto f : *structures[i]->getFeatures()){
//        f->structure = NULL;
//    }
//    std::swap(structures[i],structures[structures.size()-1]);
//  structures.resize(structures.size()-1);
//}

void Database::computeFiltersRadialClamp(int loopState){

    // Continuous indexation
    unsigned int index(0);

    // Copy structures vector
    std::vector<std::shared_ptr<Structure>> unfiltered(structures);

    // Type-range tracking
    unsigned int trackA(sortStructTypeA);
    unsigned int trackB(sortStructTypeB);

    // Apply filter condition on structures
    for(unsigned int i(0); i<unfiltered.size(); i++){
        if(unfiltered[i]->filterRadiusClamp(0.,0)==true){
            structures[index++]=unfiltered[i];
        }else{
            unfiltered[i]->setFeaturesState();
            if(i<(sortStructTypeA+sortStructTypeB)){
                if(i<sortStructTypeA){
                    trackA --;
                }else{
                    trackB --;
                }
            }
        }
    }

    // resize structure vector
    structures.resize(index);

    // Update type-range
    sortStructTypeA=trackA;
    sortStructTypeB=trackB;

    // development feature - begin
    std::cerr << "R:D : " << index << "/" << unfiltered.size() << " (" << trackA << ", " << trackB << ")" << std::endl;
    // development feature - end

}

void Database::computeFiltersRadialStatistics(int loopState){

    // Continuous indexation
    unsigned int index(0);

    // Copy structures vector
    std::vector<std::shared_ptr<Structure>> unfiltered(structures);

    // Type-range tracking
    unsigned int trackA(sortStructTypeA);
    unsigned int trackB(sortStructTypeB);

    // Active structures range
    unsigned int structureRange(unfiltered.size());

    // Check pipeline state
    if(loopState==DB_LOOP_MODE_LAST){

        // Update active structure
        structureRange=sortStructTypeA;

    }

    // Apply filter condition
    for(unsigned int i(0); i<unfiltered.size(); i++){
        if((i>=structureRange)||(unfiltered[i]->filterRadiusStatistics(meanValue, stdValue*configRadius,0)==true)){
            structures[index++]=unfiltered[i];
        }else{
            unfiltered[i]->setFeaturesState();
            if(i<(sortStructTypeA+sortStructTypeB)){
                if(i<sortStructTypeA){
                    trackA --;
                }else{
                    trackB --;
                }
            }
        }
    }

    // Resize structures vector
    structures.resize(index);

    // Update type-range
    sortStructTypeA=trackA;
    sortStructTypeB=trackB;

    // development feature - begin
    std::cerr << "R:D : " << index << "/" << unfiltered.size() << " (" << trackA << ", " << trackB << ")" << std::endl;
    // development feature - end

}

void Database::computeFiltersDisparityStatistics(int loopState){

    // Continuous indexation
    unsigned int index(0);

    // Copy structures vector
    std::vector<std::shared_ptr<Structure>> unfiltered(structures);

    // Type-range tracking
    unsigned int trackA(sortStructTypeA);
    unsigned int trackB(sortStructTypeB);

    // Active structures range
    unsigned int structureRange(unfiltered.size());

    // Check pipeline state
    if(loopState==DB_LOOP_MODE_LAST){

        // Update active structure
        structureRange=sortStructTypeA;

    }

    // Apply filter condition
    for(unsigned int i(0); i<unfiltered.size(); i++){
        if((i>=structureRange)||(unfiltered[i]->filterDisparityStatistics(stdValue*configDisparity,0)==true)){
            structures[index++]=unfiltered[i];
        }else{
            unfiltered[i]->setFeaturesState();
            if(i<(sortStructTypeA+sortStructTypeB)){
                if(i<sortStructTypeA){
                    trackA --;
                }else{
                    trackB --;
                }
            }
        }
    }

    // Resize structures vector
    structures.resize(index);

    // Update type-range
    sortStructTypeA=trackA;
    sortStructTypeB=trackB;

    // development feature - begin
    std::cerr << "R:D : " << index << "/" << unfiltered.size() << " (" << trackA << ", " << trackB << ")" << std::endl;
    // development feature - end

}

void Database::exportModel(std::string path, unsigned int major){
    std::fstream exportStream;
    exportStream.open(path+"/dev/"+std::to_string(major)+"_structure.xyz",std::ios::out);
    if (exportStream.is_open() == false){
        std::cerr << "unable to create model exportation file" << std::endl;
        return;
    }
    for(auto & element: structures){
        Eigen::Vector3d * position(element->getPosition());
        exportStream << (*position)(0) << " " << (*position)(1) << " " << (*position)(2) << " 255 0 0" << std::endl;
    }
    exportStream.close();
}

void Database::exportOdometry(std::string path, unsigned int major){
    std::fstream exportStream;
    exportStream.open(path+"/dev/"+std::to_string(major)+"_odometry.xyz",std::ios::out);
    if (exportStream.is_open() == false){
        std::cerr << "unable to create odometry exportation file" << std::endl;
        return;
    }
    for(auto & element: viewpoints){
        Eigen::Vector3d * position(element->getPosition());
        exportStream << (*position)(0) << " " << (*position)(1) << " " << (*position)(2) << " 255 255 255" << std::endl;
    }
    exportStream.close();
}

//
//  development related features
//

static cv::Point _f2i(Eigen::Vector2f value){
    return cv::Point(value[0],value[1]);
}

struct featureSort
{
    inline bool operator() (const Feature* struct1, const Feature* struct2)
    {
        return (struct1->viewpoint->index < struct2->viewpoint->index);
    }
};

//Do  cv::waitKey(0); if you want to stop after it.
void Database::_displayViewpointStructures(Viewpoint *viewpoint, unsigned int structSizeMin){
    cv::RNG rng(12345);
    cv::Rect myROI(0, 0, viewpoint->getImage()->cols, viewpoint->getImage()->rows);
    cv::Mat res(myROI.width,myROI.height, CV_8UC3, cv::Scalar(0,0,0));
    res = *viewpoint->getImage();
    for(int featureId = 0; featureId < viewpoint->getFeatures()->size(); featureId++){
        auto f = (*viewpoint->getFeatures())[featureId];
        if(!f.structure) continue;
        if(f.structure->features.size() < structSizeMin) continue;
        cv::Scalar color = cv::Scalar(rng.uniform(0,255), rng.uniform(0, 255), rng.uniform(0, 255));

        std::vector<Feature*> features = *(f.structure->getFeatures());
        std::sort(features.begin(), features.end(), featureSort());
        for(uint32_t idx = 1;idx < features.size();idx++){
            cv::line(res, _f2i((features)[idx-1]->position),  _f2i((features)[idx]->position), color, 2);
        }
    }

    rng = cv::RNG(12345);
    for(int featureId = 0; featureId < viewpoint->getFeatures()->size(); featureId++){
        auto f = (*viewpoint->getFeatures())[featureId];
        if(!f.structure) continue;
        if(f.structure->features.size() < structSizeMin) continue;
        cv::Scalar color = cv::Scalar(rng.uniform(0,255), rng.uniform(0, 255), rng.uniform(0, 255));

        auto features = f.structure->getFeatures();
        cv::putText(
            res,
            std::to_string(featureId),
            _f2i((*features)[0]->position) + cv::Point(-5, -5),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            color
        );
    }


    cv::namedWindow( "miaou", cv::WINDOW_KEEPRATIO );
    imshow( "miaou", res);
}

void Database::_sanityCheck(bool inliner){
    //How many structure have a given size (size is the index)
    uint32_t *structureSizes = new uint32_t[viewpoints.size()+1];
    memset(structureSizes, 0, (viewpoints.size()+1)*sizeof(uint32_t));

    //Set<Viewpoint> to identify viewpoint duplication in structures
    uint32_t *viewpointsUsage = new uint32_t[viewpoints.size()];
    memset(viewpointsUsage, -1, (viewpoints.size())*sizeof(uint32_t));

    for(uint32_t structureId = 0;structureId < structures.size();structureId++){
        auto &s = structures[structureId];
        if(s->features.size() < 2) throw std::runtime_error("Structure with less than two feature");
        structureSizes[s->features.size()]++;
        auto inliner = s->features.front()->inliner;
        for(auto &f : s->features){
            if(f->structure != s.get()) throw std::runtime_error("Structure with a feature which isn't pointing that structure");
            if(inliner) if(f->inliner != inliner) throw std::runtime_error("Inliner issue");
            auto viewpointId = f->viewpoint->index;
            if(viewpointsUsage[viewpointId] == structureId) throw std::runtime_error("Same view point twice in a structure");
            viewpointsUsage[viewpointId] = structureId;
        }
    }
    delete []viewpointsUsage;

    std::cout << "Structure family ";
    for(uint32_t size = 0;size <= viewpoints.size(); size++){
        auto count = structureSizes[size];
        if(count) std::cout << size << "=>" << count << " ";
    }
    std::cout << std::endl;
    delete []structureSizes;

    for(auto v : viewpoints){
        for(auto &f : v->features){
            if(f.structure){
                auto sf = &(f.structure->features);
                if(std::find(sf->begin(), sf->end(), &f) == sf->end()) throw std::runtime_error("Feature having a structure without that feature");
            }
        }
    }
}

void Database::_sanityCheckStructure(){

    unsigned int lastViewpoint(viewpoints.size()-1);

    for(unsigned int i(0); i<structures.size(); i++){
        if(structures[i]->getFeaturesCount()==2){
            if(structures[i]->getHasLastViewpoint(lastViewpoint)==true){
                if(i>=sortStructTypeA){
                    std::cerr << "Sanity check on structure : fault on type A : " << i << "/" << sortStructTypeA << std::endl;
                }
            }
        }
    }

    for(unsigned int i(0); i<structures.size(); i++){
        if(structures[i]->getFeaturesCount()>2){
            if(structures[i]->getHasLastViewpoint(lastViewpoint)==true){
                if(i>=sortStructTypeA+sortStructTypeB){
                    std::cerr << "Sanity check on structure : fault on type B : " << i << "/" << sortStructTypeA+sortStructTypeB << std::endl;
                }
            }
        }
    }

    for(unsigned int i(0); i<structures.size(); i++){
        if(structures[i]->getHasLastViewpoint(lastViewpoint)==false){
            if(i<sortStructTypeA+sortStructTypeB){
                std::cerr << "Sanity check on structure : fault on type C : " << i << "/" << sortStructTypeA+sortStructTypeB << std::endl;
            }
        }
    }

}

// Note : this function does not respect encapsulation (development function) - need to be removed
void Database::_exportState(std::string path, int major, int iter){
    int vpcount(255/viewpoints.size());
    std::fstream stream;
    stream.open( path + "/debug/" + std::to_string(major) + "_" + std::to_string(iter) + ".xyz", std::ios::out );
    if (stream.is_open()==false){
        std::cerr << "unable to create state exportation file" << std::endl;
        return;
    }
    for(auto & element: viewpoints){
        stream << element->position(0) << " "
               << element->position(1) << " "
               << element->position(2) << " 0 0 255" << std::endl;
    }
    for(auto & element: structures){
        stream << element->position(0) << " "
               << element->position(1) << " "
               << element->position(2) << " 255 0 255" << std::endl;
        for(unsigned int j(0); j<element->features.size(); j++){
            Eigen::Matrix3d matrix(*element->features[j]->getViewpoint()->getOrientation());
            Eigen::Vector3d vector(*element->features[j]->getViewpoint()->getPosition());
            Eigen::Vector3d Position(matrix*(element->features[j]->direction*element->features[j]->radius)+vector);
            stream << Position(0) << " "
                   << Position(1) << " "
                   << Position(2) << " 255 " << j*vpcount << " 0" << std::endl;
        }
    }
    stream.close();
}

// Note : this function does not respect encapsulation (development function) - need to be removed
void Database::_exportMatchDistribution(std::string path, unsigned int major, std::string type){
    if(viewpoints.size()<DB_LOOP_BOOT_COUNT){
        return;
    }
    std::fstream stream;
    stream.open( path + "/debug/" + std::to_string(major) + "_" + type + ".mat", std::ios::out );
    if(stream.is_open()==false){
        std::cerr << "unable to create match distribution exportation file" << std::endl;
    }
    stream << viewpoints.size() << " -1" << std::endl;
    for(auto & element: structures){
        for(unsigned int i(0); i<element->features.size(); i++){
            for(unsigned int j(0); j<element->features.size(); j++){
                stream << element->features[i]->viewpoint->index+1 << " " << element->features[j]->viewpoint->index+1 << std::endl;
            }
        }
    }
    stream.close();
}

