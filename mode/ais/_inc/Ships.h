/*
	Copyright(c) 2021-2024 jvde.github@gmail.com

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <iostream>
#include <string.h>
#include <memory>
#include <ctime>

#include "Common.h"

const int CLASS_A_MASK = (1 << 1) | (1 << 2) | (1 << 3);
const int CLASS_B_MASK = (1 << 18) | (1 << 19);

const int CLASS_A_STATIC_MASK = 1 << 5;
const int CLASS_B_STATIC_MASK = 1 << 24;

const int BASESTATION_MASK = (1 << 4) | (1 << 16) | (1 << 17) | (1 << 20) | (1 << 22) | (1 << 23);
const int SAR_MASK = 1 << 9;
const int ATON_MASK = 1 << 21;

struct Ship {
	int prev, next;
	uint32_t mmsi;
	int count, msg_type, shipclass, mmsi_type, channels, shiptype, heading, status, virtual_aid, path_ptr;
	int to_port, to_bow, to_starboard, to_stern, IMO, angle, validated;
	char month, day, hour, minute;
	float lat, lon, ppm, level, speed, cog, draught, distance;
	std::time_t last_signal;
	bool approximate;
	char shipname[21], destination[21], callsign[8], country_code[3];
	std::string msg;
	uint64_t last_group, group_mask;

	void reset();
	int getMMSItype();
	int getShipTypeClassEri();
	int getShipTypeClass();
	void setType();
	void Serialize(std::vector<char>& v) const;
	bool getKML(std::string&) const;
	bool getGeoJSON(std::string&) const;
};
