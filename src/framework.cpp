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

#include "framework.hpp"

#include "framework-stillcompute.hpp"
#include <yaml-cpp/yaml.h>
#include <unistd.h>
#include "framework-utiles.hpp"
#include "ThreadPool.h"
#include "framework-frontend.hpp"

int main(int argc, char *argv[]){
    //profile("boot");
    assert(argc == 2);
    std::cout << "Hello world!" << std::endl;

    YAML::Node config = YAML::LoadFile(argv[1]);

    auto database = Database(
        config["algorithm"]["bootstrap"].as<unsigned long>(),
        config["algorithm"]["error"].as<double>(),
        config["algorithm"]["structure"].as<unsigned long>(),
        config["algorithm"]["disparity"].as<double>(),
        config["algorithm"]["radius_min"].as<double>(),
        config["algorithm"]["radius_max"].as<double>()
    );
    ThreadPool threadpool(8);

    bool inlinerEnabled = false;
    Frontend *frontend = NULL;
    auto frontendType = config["frontend"]["type"].as<std::string>();
    if(frontendType == "IMAGE"){
        ViewPointSource *source = NULL;
        auto sourceType = config["frontend"]["source"]["type"].as<std::string>();
        if(sourceType == "FOLDER") {
            source = new ViewPointSourceFs(config["frontend"]["source"]["path"].as<std::string>());
        }
        auto mask = cv::imread(config["frontend"]["source"]["mask"].as<std::string>(), cv::IMREAD_GRAYSCALE);
        frontend = new FrontendPicture(source, mask, &threadpool, &database);
    }

    if(frontendType == "CLOUDPOINT"){
        frontend = new FrontendCloudpoint(&database, config["frontend"]["model"].as<std::string>(), config["frontend"]["odometry"].as<std::string>());
        inlinerEnabled = true;
    }


    // pipeline major iteration
    int loopMajor(1);

    while(true){

        if(!frontend->next()) continue; //image drop

        database.sanityCheck(inlinerEnabled);

        //
        // geometry estimation solver
        //

        // check for at least two pushed viewpoints
        if ( database.getViewpointCount() < 3 ) {
            continue;
        }

        // algorithm variable
        double loopError( 1.0 );
        double pushError( 0.0 );
        bool loopFlag( true );
        int loopMinor( 0 );

        // algorithm loop
        while ( loopFlag == true ) {

            // algorithm core
            database.computeModels();
            database.computeCentroids();
            database.computeCorrelations();
            database.computePoses();
            database.computeFrames();
            database.computeOptimals();
            database.computeRadii();
            database.computeStatistics();
            database.computeFilters();

            // development feature - begin
            database._exportState(config["export"]["path"].as<std::string>(),loopMajor,loopMinor);
            // development feature - end

            // algorithm error management
            loopError = database.getError();
            if (fabs( loopError - pushError ) < database.getConfigError()) {
                loopFlag = false;
            } else {
                pushError=loopError;
            }

            // update minor iterator
            loopMinor ++;

            // display information
            std::cout << "step : " << std::setw(6) << loopMajor << " | iteration : " << std::setw(3) << loopMinor << " | error : " << loopError << std::endl;

        }
        database.sanityCheck(inlinerEnabled);
        // major iteration exportation : model and odometry
        database.exportModel   (config["export"]["path"].as<std::string>(),loopMajor);
        database.exportOdometry(config["export"]["path"].as<std::string>(),loopMajor);

        // update major iterator
        loopMajor ++;

    }

    // system message
    return 0;

}

