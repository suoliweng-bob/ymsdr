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

#include <chrono>
#include <complex>

#ifdef _WIN32
#include <windows.h>
#define SleepSystem(x) Sleep(x)
#else
#include <unistd.h>
#include <signal.h>
#define SleepSystem(x) usleep(x * 1000)
#endif

class tN2kMsg;

void StopRequest();

#define GROUPS_ALL			0xFFFFFFFFFFFFFFFF
#define GROUP_OUT_UNDEFINED (1ULL << 63)

typedef float FLOAT32;
typedef double FLOAT64;
typedef std::complex<FLOAT32> CFLOAT32;
typedef std::complex<FLOAT64> CFLOAT64;
typedef int16_t S16;
typedef std::complex<int32_t> CS32;
typedef std::complex<int16_t> CS16;
typedef std::complex<uint8_t> CU8;
typedef std::complex<int8_t> CS8;
typedef char BIT;

enum class Format {
	CU8,
	CF32,
	CS16,
	CS8,
	TXT,
	N2K,
	UNKNOWN
};

enum class Type {
	NONE,
	RTLSDR,
	AIRSPYHF,
	SERIALPORT,
	AIRSPY,
	SDRPLAY,
	WAVFILE,
	RAWFILE,
	RTLTCP,
	UDP,
	HACKRF,
	SOAPYSDR,
	ZMQ,
	SPYSERVER,
	N2K
};

enum class OutputLevel {
	NONE,
	NMEA,
	NMEA_TAG,
	FULL,
	JSON_NMEA,
	JSON_SPARSE,
	JSON_FULL
};

enum ShippingClass {
	CLASS_OTHER = 0,
	CLASS_UNKNOWN = 1,
	CLASS_CARGO = 2,
	CLASS_B = 3,
	CLASS_PASSENGER = 4,
	CLASS_SPECIAL = 5,
	CLASS_TANKER = 6,
	CLASS_HIGHSPEED = 7,
	CLASS_FISHING = 8,
	CLASS_PLANE = 9,
	CLASS_HELICOPTER = 10,
	CLASS_STATION = 11,
	CLASS_ATON = 12,
	CLASS_SARTEPIRB = 13
};

enum MMSI_Class {
	MMSI_OTHER = 0,
	MMSI_CLASS_A = 1,
	MMSI_CLASS_B = 2,
	MMSI_BASESTATION = 3,
	MMSI_SAR = 4,
	MMSI_SARTEPIRB = 5,
	MMSI_ATON = 6
};

const float DISTANCE_UNDEFINED = -1;
const float LAT_UNDEFINED = 91;
const float LON_UNDEFINED = 181;
const float COG_UNDEFINED = 360;
const float SPEED_UNDEFINED = -1;
const float DRAUGHT_UNDEFINED = -1;

const int HEADING_UNDEFINED = 511;
const int STATUS_UNDEFINED = 15;
const int DIMENSION_UNDEFINED = -1;
const int ALT_UNDEFINED = 4095;
const int ETA_DAY_UNDEFINED = 0;
const int ETA_MONTH_UNDEFINED = 0;
const int ETA_HOUR_UNDEFINED = 24;
const int ETA_MINUTE_UNDEFINED = 60;
const int IMO_UNDEFINED = 0;
const int ANGLE_UNDEFINED = -1;

const float LEVEL_UNDEFINED = 1024;
const float PPM_UNDEFINED = 1024;

struct TAG {
	unsigned mode = 0;
	float sample_lvl = 0;
	float level = 0;
	float ppm = 0;
	uint64_t group = 0;

	// some data flowing from DB downstream
	int angle = -1;
	float distance = -1;
	float lat = 0, lon = 0;
	bool validated = false;
	std::time_t previous_signal = (std::time_t)0;
	int shipclass = CLASS_UNKNOWN;
	float speed = SPEED_UNDEFINED;

	void clear() {
		group = GROUP_OUT_UNDEFINED;
		sample_lvl = LEVEL_UNDEFINED;
		level = LEVEL_UNDEFINED;
		ppm = PPM_UNDEFINED;
		lat = LAT_UNDEFINED;
		lon = LON_UNDEFINED;
		distance = DISTANCE_UNDEFINED;
		speed = SPEED_UNDEFINED;
		angle = ANGLE_UNDEFINED;
		validated = false;
		previous_signal = (std::time_t)0;
		shipclass = CLASS_UNKNOWN;
	}
};

struct RAW {
	Format format;
	void* data;
	int size;
};

struct Setting {
	virtual ~Setting() {}
	virtual Setting& Set(std::string option, std::string arg) { return *this; }
	virtual std::string Get() { return ""; }
};

template <typename T>
class Callback {
public:
	virtual ~Callback() {}
	virtual void onMsg(const T&) {}
};

using namespace std::chrono;

const FLOAT32 PI = 3.14159265358979323846f;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
