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

#include <cassert>

#include "Model.h"
#include "Utilities.h"

namespace AIS {
	std::mutex MessageMutex::mtx;

	void ModelFrontend::buildModel(char CH1, char CH2, int sample_rate, bool timerOn, Device::Device* dev) {
		device = dev;

		ROT.setRotation((float)(PI * 25000.0 / 48000.0));

		Connection<RAW>& physical = timerOn ? (*device >> timer).out : device->out;

		if (mode == AIS::Mode::X) {

			if (sample_rate < 12000 || sample_rate > 192000)
				throw std::runtime_error("Model: sample rate must be between 12k and 192k (inclusive).");

			physical >> convert;

			const std::vector<uint32_t> definedRates = { 48000, 96000, 192000 };

			uint32_t bucket = 0xFFFF;
			bool interpolated = false;

			for (uint32_t r : definedRates)
				if (r >= sample_rate) {
					bucket = r;
					if (r != sample_rate) interpolated = true;
					break;
				}

			if (interpolated)
				std::cerr << "Warning: sample rate " << sample_rate / 1000 << "K upsampled to " << bucket / 1000 << "K." << std::endl;

			US.setParams(sample_rate, bucket);

			switch (bucket - (interpolated ? 1 : 0)) {
			case 192000:
				FDC.setTaps(-1.1f);
				if (!droop_compensation)
					convert >> DS2_2 >> DS2_1 >> FCIC5_a;
				else
					convert >> DS2_2 >> DS2_1 >> FDC >> FCIC5_a;
				break;
			case 192000 - 1:
				FDC.setTaps(-1.1f);
				if (!droop_compensation)
					convert >> US >> DS2_2 >> DS2_1 >> FCIC5_a;
				else
					convert >> US >> DS2_2 >> DS2_1 >> FDC >> FCIC5_a;
				break;
			case 96000:
				FDC.setTaps(-0.8f);
				if (!droop_compensation)
					convert >> DS2_1 >> FCIC5_a;
				else
					convert >> DS2_1 >> FDC >> FCIC5_a;
				break;
			case 96000 - 1:
				FDC.setTaps(-0.8f);
				if (!droop_compensation)
					convert >> US >> DS2_1 >> FCIC5_a;
				else
					convert >> US >> DS2_1 >> FDC >> FCIC5_a;
				break;
			case 48000:
				convert >> FCIC5_a;
				break;
			case 48000 - 1:
				convert >> US >> FCIC5_a;
				break;

			default:
				throw std::runtime_error("Model: internal error. Sample rate should be supported.");
			}

			C_a = &FCIC5_a.out;
			C_b = &FCIC5_b.out;

			return;
		}

		if (sample_rate < 96000 || sample_rate > 12288000)
			throw std::runtime_error("Model: sample rate must be between 96K and 12288K (inclusive).");


		if (SOXR_DS) {
			sox.setParams(sample_rate, 96000);
			physical >> convert >> sox >> ROT;
		}
		else if (SAMPLERATE_DS) {
			src.setParams(sample_rate, 96000);
			physical >> convert >> src >> ROT;
		}
		else if (MA_DS) {
			DS_MA.setRates(sample_rate, 96000);
			physical >> convert >> DS_MA >> ROT;
		}
		else {
			const std::vector<uint32_t> definedRatesNoDSK = { 96000, 192000, 288000, 384000, 768000, 1536000, 3072000, 6144000, 12288000 };
			const std::vector<uint32_t> definedRatesDSK = { 96000, 192000, 288000, 384000, 576000, 768000, 1152000, 1536000, 2304000, 3072000, 6144000, 12288000 };

			const std::vector<uint32_t>& definedRates = allowDSK ? definedRatesDSK : definedRatesNoDSK;

			uint32_t bucket = 0xFFFF;
			bool interpolated = false;

			for (uint32_t r : definedRates)
				if (r >= sample_rate) {
					bucket = r;
					if (r != sample_rate) interpolated = true;
					break;
				}

			if (interpolated)
				std::cerr << "Warning: sample rate " << sample_rate / 1000 << "K upsampled to " << bucket / 1000 << "K." << std::endl;

			US.setParams(sample_rate, bucket);
			DSK.setParams(Filters::BlackmanHarris_28_3, 3);

			physical >> convert;

			// FDC is a 3-tap filter to compensate for droop in the CIC5 downsampling filters
			// Filter coefficients currently set on empirical basis, and ignored for downsampling including decimation by 3

			switch (bucket - (interpolated ? 1 : 0)) {
				// 2^7
			case 12288000:
				FDC.setTaps(-2.0f);
				if (!droop_compensation)
					convert >> DS2_7 >> DS2_6 >> DS2_5 >> DS2_4 >> DS2_3 >> DS2_2 >> DS2_1 >> ROT;
				else
					convert >> DS2_7 >> DS2_6 >> DS2_5 >> DS2_4 >> DS2_3 >> DS2_2 >> DS2_1 >> FDC >> ROT;
				break;
			case 12288000 - 1:
				FDC.setTaps(-2.0f);
				if (!droop_compensation)
					convert >> DS2_7 >> DS2_6 >> DS2_5 >> DS2_4 >> DS2_3 >> US >> DS2_2 >> DS2_1 >> ROT;
				else
					convert >> DS2_7 >> DS2_6 >> DS2_5 >> DS2_4 >> DS2_3 >> US >> DS2_2 >> DS2_1 >> FDC >> ROT;
				break;

				// 2^6
			case 6144000:
				FDC.setTaps(-2.0f);
				if (!droop_compensation)
					convert >> DS2_6 >> DS2_5 >> DS2_4 >> DS2_3 >> DS2_2 >> DS2_1 >> ROT;
				else
					convert >> DS2_6 >> DS2_5 >> DS2_4 >> DS2_3 >> DS2_2 >> DS2_1 >> FDC >> ROT;
				break;
			case 6144000 - 1:
				FDC.setTaps(-2.0f);
				if (!droop_compensation)
					convert >> DS2_6 >> DS2_5 >> DS2_4 >> DS2_3 >> US >> DS2_2 >> DS2_1 >> ROT;
				else
					convert >> DS2_6 >> DS2_5 >> DS2_4 >> DS2_3 >> US >> DS2_2 >> DS2_1 >> FDC >> ROT;
				break;

				// 2^5
			case 3072000:
				FDC.setTaps(-1.5f);
				if (!droop_compensation)
					convert >> DS2_5 >> DS2_4 >> DS2_3 >> DS2_2 >> DS2_1 >> ROT;
				else
					convert >> DS2_5 >> DS2_4 >> DS2_3 >> DS2_2 >> DS2_1 >> FDC >> ROT;
				break;
			case 3072000 - 1:
				FDC.setTaps(-1.5f);
				if (!droop_compensation)
					convert >> DS2_5 >> DS2_4 >> DS2_3 >> US >> DS2_2 >> DS2_1 >> ROT;
				else
					convert >> DS2_5 >> DS2_4 >> DS2_3 >> US >> DS2_2 >> DS2_1 >> FDC >> ROT;
				break;

				// 2^3 * 3
			case 2304000:
				if (!droop_compensation)
					convert >> DS2_3 >> DS2_2 >> DS2_1 >> DSK >> ROT;
				else
					convert >> DS2_3 >> DS2_2 >> DS2_1 /* >> FDC */ >> DSK >> ROT;
				break;
			case 2304000 - 1:
				if (!droop_compensation)
					convert >> DS2_3 >> DS2_2 >> DS2_1 >> US >> DSK >> ROT;
				else
					convert >> DS2_3 >> DS2_2 >> DS2_1 >> US /* >> FDC */ >> DSK >> ROT;
				break;

				// 2^4
			case 1536000:
				FDC.setTaps(-1.2f);
				if (!fixedpointDS) {
					if (!droop_compensation)
						convert >> DS2_4 >> DS2_3 >> DS2_2 >> DS2_1 >> ROT;
					else
						convert >> DS2_4 >> DS2_3 >> DS2_2 >> DS2_1 >> FDC >> ROT;
				}
				else {
					if (!droop_compensation)
						convert.outCU8 >> DS16_CU8 >> ROT;
					else
						convert.outCU8 >> DS16_CU8 >> FDC >> ROT;
				}
				break;
			case 1536000 - 1:
				FDC.setTaps(-1.2f);
				if (!droop_compensation)
					convert >> DS2_4 >> DS2_3 >> US >> DS2_2 >> DS2_1 >> ROT;
				else
					convert >> DS2_4 >> DS2_3 >> US >> DS2_2 >> DS2_1 >> FDC >> ROT;
				break;

				// 2^2 * 3
			case 1152000:
				if (!droop_compensation)
					convert >> DS2_2 >> DS2_1 >> DSK >> ROT;
				else
					convert >> DS2_2 >> DS2_1 /* >> FDC */ >> DSK >> ROT;
				break;
			case 1152000 - 1:
				if (!droop_compensation)
					convert >> DS2_2 >> DS2_1 >> US >> DSK >> ROT;
				else
					convert >> DS2_2 >> DS2_1 /* >> FDC */ >> US >> DSK >> ROT;
				break;

				// 2^3
			case 768000:
				FDC.setTaps(-1.2f);
				if (!droop_compensation)
					convert >> DS2_3 >> DS2_2 >> DS2_1 >> ROT;
				else
					convert >> DS2_3 >> DS2_2 >> DS2_1 >> FDC >> ROT;
				break;
			case 768000 - 1:
				FDC.setTaps(-1.2f);
				if (!droop_compensation)
					convert >> DS2_3 >> US >> DS2_2 >> DS2_1 >> ROT;
				else
					convert >> DS2_3 >> US >> DS2_2 >> DS2_1 >> FDC >> ROT;
				break;

				// 2 * 3
			case 576000:
				if (!droop_compensation)
					convert >> DS2_1 >> DSK >> ROT;
				else
					convert >> DS2_1 /* >> FDC */ >> DSK >> ROT;
				break;
			case 576000 - 1:
				if (!droop_compensation)
					convert >> DS2_1 >> US >> DSK >> ROT;
				else
					convert >> DS2_1 /* >> FDC */ >> US >> DSK >> ROT;
				break;

				// 2^2
			case 384000:
				FDC.setTaps(-1.1f);
				if (!droop_compensation)
					convert >> DS2_2 >> DS2_1 >> ROT;
				else
					convert >> DS2_2 >> DS2_1 >> FDC >> ROT;
				break;
			case 384000 - 1:
				FDC.setTaps(-1.1f);
				if (!droop_compensation)
					convert >> US >> DS2_2 >> DS2_1 >> ROT;
				else
					convert >> US >> DS2_2 >> DS2_1 >> FDC >> ROT;
				break;

				// 3
			case 288000:
				convert >> DSK >> ROT;
				break;
			case 288000 - 1:
				convert >> US >> DSK >> ROT;
				break;

				// 2^1
			case 192000:
				FDC.setTaps(-0.8f);
				if (!droop_compensation)
					convert >> DS2_1 >> ROT;
				else
					convert >> DS2_1 >> FDC >> ROT;
				break;
			case 192000 - 1:
				FDC.setTaps(-0.8f);
				if (!droop_compensation)
					convert >> US >> DS2_1 >> ROT;
				else
					convert >> US >> DS2_1 >> FDC >> ROT;
				break;

				// 2^0
			case 96000:
				convert >> ROT;
				break;

			default:
				throw std::runtime_error("Model: internal error. Sample rate should be supported.");
			}
		}

		ROT.up >> DS2_a >> FCIC5_a;
		ROT.down >> DS2_b >> FCIC5_b;

		// pick up point for downstream decoders
		C_a = &FCIC5_a.out;
		C_b = &FCIC5_b.out;

		return;
	}

	Setting& ModelFrontend::Set(std::string option, std::string arg) {
		Util::Convert::toUpper(option);
		Util::Convert::toUpper(arg);

		if (option == "FP_DS") {
			fixedpointDS = Util::Parse::Switch(arg);
			MA_DS = false;
		}
		else if (option == "SOXR") {
			SOXR_DS = Util::Parse::Switch(arg);
			SAMPLERATE_DS = false;
			MA_DS = false;
		}
		else if (option == "SRC") {
			SAMPLERATE_DS = Util::Parse::Switch(arg);
			SOXR_DS = false;
			MA_DS = false;
		}
		else if (option == "MA") {
			MA_DS = Util::Parse::Switch(arg);
			SAMPLERATE_DS = false;
			SOXR_DS = false;
		}
		else if (option == "DSK") {
			allowDSK = Util::Parse::Switch(arg);
		}
		else if (option == "DROOP") {
			droop_compensation = Util::Parse::Switch(arg);
		}
		else if (option == "STATION_ID") {
			station = Util::Parse::Integer(arg);
		}
		else
			Model::Set(option, arg);

		return *this;
	}

	std::string ModelFrontend::Get() {

		std::string str;

		if (SOXR_DS)
			return "soxr ON " + Model::Get();
		else if (SAMPLERATE_DS)
			return "src ON " + Model::Get();
		else if (MA_DS)
			return "MA ON " + Model::Get();

		return "droop " + Util::Convert::toString(droop_compensation) + " fp_ds " + Util::Convert::toString(fixedpointDS) + " dsk " + Util::Convert::toString(allowDSK) + " " + Model::Get();
	}

	void ModelBase::buildModel(char CH1, char CH2, int sample_rate, bool timerOn, Device::Device* dev) {
		ModelFrontend::buildModel(CH1, CH2, sample_rate, timerOn, dev);
		setName("Base (non-coherent)");

		assert(C_a != NULL && C_b != NULL);

		FR_a.setTaps(Filters::Receiver);
		FR_b.setTaps(Filters::Receiver);

		DEC_a.setOrigin(CH1, station);
		DEC_b.setOrigin(CH2, station);

		*C_a >> FM_a >> FR_a >> sampler_a >> DEC_a >> output;
		*C_b >> FM_b >> FR_b >> sampler_b >> DEC_b >> output;

		DEC_a.DecoderMessage.Connect(sampler_a);
		DEC_b.DecoderMessage.Connect(sampler_b);

		return;
	}

	void ModelStandard::buildModel(char CH1, char CH2, int sample_rate, bool timerOn, Device::Device* dev) {
		ModelFrontend::buildModel(CH1, CH2, sample_rate, timerOn, dev);
		setName("Standard (non-coherent)");

		assert(C_a != NULL && C_b != NULL);

		FR_a.setTaps(Filters::Receiver);
		FR_b.setTaps(Filters::Receiver);

		S_a.setConnections(nSymbolsPerSample);
		S_b.setConnections(nSymbolsPerSample);

		DEC_a.resize(nSymbolsPerSample);
		DEC_b.resize(nSymbolsPerSample);

		*C_a >> FM_a >> FR_a >> S_a;
		*C_b >> FM_b >> FR_b >> S_b;

		for (int i = 0; i < nSymbolsPerSample; i++) {
			DEC_a[i].setOrigin(CH1, station);
			DEC_b[i].setOrigin(CH2, station);

			S_a.out[i] >> DEC_a[i] >> output;
			S_b.out[i] >> DEC_b[i] >> output;

			for (int j = 0; j < nSymbolsPerSample; j++) {
				if (i != j) {
					DEC_a[i].DecoderMessage.Connect(DEC_a[j]);
					DEC_b[i].DecoderMessage.Connect(DEC_b[j]);
				}
			}
		}

		return;
	}

	void ModelDefault::buildModel(char CH1, char CH2, int sample_rate, bool timerOn, Device::Device* dev) {
		ModelFrontend::buildModel(CH1, CH2, sample_rate, timerOn, dev);

		setName("AIS engine " VERSION);

		assert(C_a != NULL && C_b != NULL);

		FC_a.setTaps(Filters::Coherent);
		FC_b.setTaps(Filters::Coherent);

		S_a.setConnections(nSymbolsPerSample);
		S_b.setConnections(nSymbolsPerSample);

		DEC_a.resize(nSymbolsPerSample);
		DEC_b.resize(nSymbolsPerSample);

		if (!PS_EMA) {
			CD_a.resize(nSymbolsPerSample);
			CD_b.resize(nSymbolsPerSample);
		}
		else {
			CD_EMA_a.resize(nSymbolsPerSample);
			CD_EMA_b.resize(nSymbolsPerSample);
		}

		CGF_a.setParams(512, 187);
		CGF_b.setParams(512, 187);

		if (CGF_wide) {
			CGF_a.setWide(true);
			CGF_b.setWide(true);
		}

		*C_a >> CGF_a >> FC_a >> S_a;
		*C_b >> CGF_b >> FC_b >> S_b;

		for (int i = 0; i < nSymbolsPerSample; i++) {
			DEC_a[i].setOrigin(CH1, station);
			DEC_b[i].setOrigin(CH2, station);

			if (!PS_EMA) {
				CD_a[i].setParams(nHistory, nDelay);
				CD_b[i].setParams(nHistory, nDelay);

				S_a.out[i] >> CD_a[i] >> DEC_a[i] >> output;
				S_b.out[i] >> CD_b[i] >> DEC_b[i] >> output;
			}
			else {
				CD_EMA_a[i].setParams(nDelay);
				CD_EMA_b[i].setParams(nDelay);

				S_a.out[i] >> CD_EMA_a[i] >> DEC_a[i] >> output;
				S_b.out[i] >> CD_EMA_b[i] >> DEC_b[i] >> output;
			}
			for (int j = 0; j < nSymbolsPerSample; j++) {
				if (i != j) {
					DEC_a[i].DecoderMessage.Connect(DEC_a[j]);
					DEC_b[i].DecoderMessage.Connect(DEC_b[j]);
				}
			}
		}

		return;
	}

	Setting& ModelDefault::Set(std::string option, std::string arg) {
		Util::Convert::toUpper(option);
		Util::Convert::toUpper(arg);

		if (option == "PS_EMA") {
			PS_EMA = Util::Parse::Switch(arg);
		}
		else if (option == "AFC_WIDE") {
			CGF_wide = Util::Parse::Switch(arg);
		}
		else
			ModelFrontend::Set(option, arg);

		return *this;
	}

	std::string ModelDefault::Get() {
		return "ps_ema " + Util::Convert::toString(PS_EMA) + " afc_wide " + Util::Convert::toString(CGF_wide) + " " + ModelFrontend::Get();
	}

	void ModelChallenger::buildModel(char CH1, char CH2, int sample_rate, bool timerOn, Device::Device* dev) {
		ModelFrontend::buildModel(CH1, CH2, sample_rate, timerOn, dev);

		setName("Challenger " VERSION);

		assert(C_a != NULL && C_b != NULL);

		FC_a.setTaps(Filters::Coherent);
		FC_b.setTaps(Filters::Coherent);
		FR_af.setTaps(Filters::Receiver);
		FR_bf.setTaps(Filters::Receiver);

		S_a.setConnections(nSymbolsPerSample);
		S_b.setConnections(nSymbolsPerSample);
		S_af.setConnections(nSymbolsPerSample);
		S_bf.setConnections(nSymbolsPerSample);

		throttle_a.setConnections(1);
		throttle_b.setConnections(1);

		DEC_a.resize(nSymbolsPerSample);
		DEC_b.resize(nSymbolsPerSample);
		DEC_af.resize(nSymbolsPerSample);
		DEC_bf.resize(nSymbolsPerSample);

		CD_EMA_a.resize(nSymbolsPerSample);
		CD_EMA_b.resize(nSymbolsPerSample);

		CGF_a.setParams(512, 187);
		CGF_b.setParams(512, 187);

		if (CGF_wide) {
			CGF_a.setWide(true);
			CGF_b.setWide(true);
		}

		*C_a >> CGF_a >> throttle_a;
		*C_b >> CGF_b >> throttle_b;

		// throttling to make sure that the two channels are synchronized to avoid doubles
		throttle_a.out[0] >> FC_a >> S_a;
		throttle_b.out[0] >> FC_b >> S_b;

		// needs to be fixed for signal level
		throttle_a.out[0] >> FM_af >> FR_af >> S_af;
		throttle_b.out[0] >> FM_bf >> FR_bf >> S_bf;

		for (int i = 0; i < nSymbolsPerSample; i++) {
			DEC_a[i].setOrigin(CH1, station);
			DEC_af[i].setOrigin(CH1, station);

			DEC_b[i].setOrigin(CH2, station);
			DEC_bf[i].setOrigin(CH2, station);

			CD_EMA_a[i].setParams(nDelay);
			CD_EMA_b[i].setParams(nDelay);

			S_a.out[i] >> CD_EMA_a[i] >> DEC_a[i] >> output;
			S_b.out[i] >> CD_EMA_b[i] >> DEC_b[i] >> output;

			S_af.out[i] >> DEC_af[i] >> output;
			S_bf.out[i] >> DEC_bf[i] >> output;

			for (int j = 0; j < nSymbolsPerSample; j++) {
				DEC_af[i].DecoderMessage.Connect(DEC_a[j]);
				DEC_a[i].DecoderMessage.Connect(DEC_af[j]);

				DEC_bf[i].DecoderMessage.Connect(DEC_b[j]);
				DEC_b[i].DecoderMessage.Connect(DEC_bf[j]);

				if (i != j) {
					DEC_a[i].DecoderMessage.Connect(DEC_a[j]);
					DEC_af[i].DecoderMessage.Connect(DEC_af[j]);

					DEC_b[i].DecoderMessage.Connect(DEC_b[j]);
					DEC_bf[i].DecoderMessage.Connect(DEC_bf[j]);
				}
			}
		}

		return;
	}

	Setting& ModelChallenger::Set(std::string option, std::string arg) {
		Util::Convert::toUpper(option);
		Util::Convert::toUpper(arg);

		if (option == "PS_EMA") {
			PS_EMA = Util::Parse::Switch(arg);
		}
		else if (option == "AFC_WIDE") {
			CGF_wide = Util::Parse::Switch(arg);
		}
		else
			ModelFrontend::Set(option, arg);

		return *this;
	}

	std::string ModelChallenger::Get() {
		return "ps_ema " + Util::Convert::toString(PS_EMA) + " afc_wide " + Util::Convert::toString(CGF_wide) + " " + ModelFrontend::Get();
	}

	void ModelDiscriminator::buildModel(char CH1, char CH2, int sample_rate, bool timerOn, Device::Device* dev) {
		setName("FM discriminator output model");

		device = dev;
		const int nSymbolsPerSample = 48000 / 9600;

		FR_a.setTaps(Filters::Receiver);
		FR_b.setTaps(Filters::Receiver);

		S_a.setConnections(nSymbolsPerSample);
		S_b.setConnections(nSymbolsPerSample);

		DEC_a.resize(nSymbolsPerSample);
		DEC_b.resize(nSymbolsPerSample);

		Connection<RAW>& physical = timerOn ? (*device >> timer).out : device->out;

		if (sample_rate == 48000) {
			physical >> convert;
			convert >> RP >> FR_a;
			convert >> IP >> FR_b;
		}
		else if (sample_rate < 48000) {
			US.setParams(sample_rate, 48000);
			physical >> convert >> US;
			convert >> RP >> FR_a;
			convert >> IP >> FR_b;
		}
		else
			throw std::runtime_error("Internal error: sample rate not supported in FM discriminator model.");

		FR_a >> S_a;
		FR_b >> S_b;

		for (int i = 0; i < nSymbolsPerSample; i++) {
			S_a.out[i] >> DEC_a[i] >> output;
			S_b.out[i] >> DEC_b[i] >> output;

			DEC_a[i].setOrigin(CH1, station);
			DEC_b[i].setOrigin(CH2, station);

			for (int j = 0; j < nSymbolsPerSample; j++) {
				if (i != j) {
					DEC_a[i].DecoderMessage.Connect(DEC_a[j]);
					DEC_b[i].DecoderMessage.Connect(DEC_b[j]);
				}
			}
		}

		return;
	}

	void ModelNMEA::buildModel(char CH1, char CH2, int sample_rate, bool timerOn, Device::Device* dev) {
		setName("NMEA input");
		device = dev;
		*device >> nmea >> output;
		nmea.outGPS >> output_gps;

		nmea.setStation(station);
	}

	Setting& ModelNMEA::Set(std::string option, std::string arg) {
		Util::Convert::toUpper(option);
		// Util::Convert::toUpper(arg);

		if (option == "NMEA_REFRESH") {
			nmea.setRegenerate(Util::Parse::Switch(arg));
		}
		if (option == "STAMP") {
			nmea.setStamp(Util::Parse::Switch(arg));
		}
		else if (option == "CRC_CHECK") {
			nmea.setCRCcheck(Util::Parse::Switch(arg));
		}
		else if (option == "VDO") {
			nmea.setVDO(Util::Parse::Switch(arg));
		}
		else if (option == "UUID") {
			nmea.setUUID(arg);
		}
		else
			Model::Set(option, arg);

		return *this;
	}

	std::string ModelNMEA::Get() {
		return "nmea_refresh " + Util::Convert::toString(nmea.getRegenerate()) + " uuid " + nmea.getUUID() + " ID " + std::to_string(nmea.getStation()) + " stamp " + Util::Convert::toString(nmea.getStamp()) + " crc_check " + Util::Convert::toString(nmea.getCRCcheck()) + " VDO " + Util::Convert::toString(nmea.getVDO()) + Model::Get();
	}

	void ModelN2K::buildModel(char CH1, char CH2, int sample_rate, bool timerOn, Device::Device* dev) {
		setName("N2K input");
		device = dev;
		*device >> n2k >> output;
	}

	Setting& ModelN2K::Set(std::string option, std::string arg) {
		Util::Convert::toUpper(option);

		Model::Set(option, arg);
		return *this;
	}

	std::string ModelN2K::Get() {
		return Model::Get();
	}

}
