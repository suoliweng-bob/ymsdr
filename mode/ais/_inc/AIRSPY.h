/*
	Copyright(c) 2021-2022 gtlittlewing
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

#include "Device.h"
#include "Utilities.h"

#ifdef HASAIRSPY
#include <airspy.h>
#endif

namespace Device {


	class AIRSPY : public Device {
#ifdef HASAIRSPY

		struct airspy_device* dev = nullptr;
		bool lost = false;
		std::vector<uint32_t> rates;
		uint64_t serial;

		enum class AIRSPYGainMode {
			Free,
			Sensitivity,
			Linearity
		} mode = AIRSPYGainMode::Linearity;
		int gain = 17;

		bool mixer_AGC = true;
		bool LNA_AGC = true;

		int mixer_Gain = 10;
		int LNA_Gain = 10;
		int VGA_Gain = 10;

		bool bias_tee = false;

		static int callback_static(airspy_transfer_t* tf);
		void callback(CFLOAT32*, int);

		void setBiasTee(bool);
		void setLNA_AGC(int);
		void setMixer_AGC(int);

		void setLNA_Gain(int);
		void setVGA_Gain(int);
		void setMixer_Gain(int);

		void setSensitivity_Gain(int);
		void setLinearity_Gain(int);

		void applySettings();

		void setDefaultRate();

	public:
		AIRSPY() : Device(Format::CF32, 0) {}

		// Control
		void Open(uint64_t h);

#ifdef HASAIRSPY_ANDROID
		void OpenWithFileDescriptor(int);
#endif

		void Play();
		void Stop();
		void Close();

		bool isStreaming();
		bool isCallback() { return true; }

		void getDeviceList(std::vector<Description>& DeviceList);

		// Settings
		Setting& Set(std::string option, std::string arg);
		std::string Get();

		std::string getProduct() { return "AIRSPY"; }
		std::string getVendor() { return "AIRSPY"; }
		std::string getSerial() { return Util::Convert::toHexString(serial); }

		void setFormat(Format f) {}
#endif
	};
}
