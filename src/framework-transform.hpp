/*
 *  sfs-framework
 *
 *      Nils Hamel - nils.hamel@bluewin.ch
 *      Charles Papon - charles.papon.90@gmail.com
 *      Copyright (c) 2019-2020 DHLAB, EPFL & HES-SO Valais-Wallis
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

// External includes
#include <iostream>
#include <Eigen/Dense>

// Internal includes
#include "framework-viewpoint.hpp"

// Module object
class Transform {

public: /* Need to be set back to private */
    Eigen::Matrix3d rotation;
    Eigen::Vector3d translation;
    Eigen::Vector3d push;
    Eigen::Vector3d centerFirst;
    Eigen::Vector3d centerSecond;
    Eigen::Matrix3d correlation;
    unsigned int count;
    double scale;

public:
    double getError();
    Eigen::Matrix3d * getRotation();
    Eigen::Vector3d * getTranslation();
    double getScale();
    void setTranslationScale(double scaleFactor);
    void setScale();
    void pushCorrelation(Eigen::Vector3d * firstComponent, Eigen::Vector3d * secondComponent);
    void pushCentroid(Eigen::Vector3d * pushFirst, Eigen::Vector3d * pushSecond);
    void resetCorrelation();
    void resetCentroid();
    void computeCentroid();
    void computePose();
    void computeFrame(Viewpoint * first, Viewpoint * second);

};

