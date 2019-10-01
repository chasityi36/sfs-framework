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

#include <iostream>
#include <cmath>
#include "framework-database.hpp"
#include "framework-viewpointsource.hpp"
#include "framework-sparsefeature.hpp"
#include "framework-stillcompute.hpp"

#define GETSET(type, var) \
	private: \
		type _##var; \
	public: \
		type get##var() \
		{\
		  return _##var; \
		}\
		void set##var(type val) \
		{\
		  _##var = val; \
		}

void algorithmComputeModels(Database & database);