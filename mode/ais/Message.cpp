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

#include "Message.h"

namespace AIS {

	int Message::ID = 0;

	static int NMEAchecksum(const std::string& s) {
		int check = 0;
		for (int i = 1; i < s.length(); i++) check ^= s[i];
		return check;
	}

	const std::string GPS::formatLatLon(float value, bool isLatitude) const {
		std::stringstream ss;

		int degrees = static_cast<int>(value);
		float minutes = (value - degrees) * 60;

		ss << std::setfill('0') << std::setw(isLatitude ? 2 : 3) << std::abs(degrees);
		ss << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2) << minutes;

		return ss.str();
	}

	const std::string GPS::getNMEA() const {
		if (nmea.empty()) {
			std::string flat = formatLatLon(lat, true);
			std::string flon = formatLatLon(lon, false);

			char latDir = lat >= 0 ? 'N' : 'S';
			char lonDir = lon >= 0 ? 'E' : 'W';

			std::string line = "$GPGLL," + flat + "," + latDir + "," + flon + "," + lonDir + ",,,";

			int c = NMEAchecksum(line);

			line += '*';
			line += (c >> 4) < 10 ? (c >> 4) + '0' : (c >> 4) + 'A' - 10;
			line += (c & 0xF) < 10 ? (c & 0xF) + '0' : (c & 0xF) + 'A' - 10;

			return line;
		}
		else
			return nmea;
	}

	const std::string GPS::getJSON() const {
		if (json.empty())
			return "{\"class\":\"TPV\",\"lat\":" + std::to_string(lat) + ",\"lon\":" + std::to_string(lon) + "}";
		else
			return json;
	}

	std::string Message::getNMEAJSON(unsigned mode, float level, float ppm, const std::string& uuid) const {
		std::stringstream ss;

		ss << "{\"class\":\"AIS\",\"device\":\"AIS-catcher\",\"channel\":\"" << getChannel() << "\"";

		if (mode & 2) {
			ss << ",\"rxuxtime\":" << getRxTimeUnix();
			ss << ",\"rxtime\":\"" << getRxTime() << "\"";
		}
		if (!uuid.empty()) ss << ",\"uuid\":\"" << uuid << "\"";
		if (mode & 1) {
			ss << ",\"signalpower\":";
			if (level == LEVEL_UNDEFINED)
				ss << "null";
			else
				ss << level;
			ss << ",\"ppm\":";
			if (ppm == PPM_UNDEFINED)
				ss << "null";
			else
				ss << ppm;
		}

		if (getStation()) ss << ",\"station_id\":" << getStation();

		ss << ",\"mmsi\":" << mmsi() << ",\"type\":" << type() << ",\"nmea\":[\"" << NMEA[0] << "\"";

		for (int j = 1; j < NMEA.size(); j++)
			ss << ",\"" << NMEA[j] << "\"";

		ss << "]}";

		return ss.str();
	}

	bool Message::validate() {
		const int ml[27] = { 149, 149, 149, 168, 418, 88, 72, 56, 168, 70, 168, 72, 40, 40, 88, 92, 80, 168, 312, 70, 271, 145, 154, 160, 72, 60, 96 };

		if (type() < 1 || type() > 27) return false;
		if (getLength() < ml[type() - 1]) return false;

		return true;
	}

	unsigned Message::getUint(int start, int len) const {
		// we start 2nd part of first byte and stop first part of last byte
		int x = start >> 3, y = start & 7;
		unsigned u = data[x] & (0xFF >> y);
		int remaining = len - 8 + y;

		// first byte is last byte
		if (remaining <= 0) {
			return u >> (-remaining);
		}
		// add full bytes
		while (remaining >= 8) {
			u <<= 8;
			u |= data[++x];
			remaining -= 8;
		}
		// make room for last bits if needed
		if (remaining > 0) {
			u <<= remaining;
			u |= data[++x] >> (8 - remaining);
		}

		return u;
	}

	bool Message::setUint(int start, int len, unsigned val) {
		if (length >> 3 >= MAX_AIS_LENGTH) return false;

		int x = start >> 3, y = start & 7;
		int remaining = len;

		if (8 - y >= remaining) {
			uint8_t bitmask = (0xFF >> (8 - remaining)) << (8 - y - remaining);
			data[x] = (data[x] & ~bitmask) | ((val << (8 - y - remaining)) & bitmask);
			return true;
		}

		uint8_t bitmask = 0xFF >> y;
		data[x] = (data[x] & ~bitmask) | ((val >> (remaining - 8 + y)) & bitmask);
		remaining -= 8 - y;
		x++;

		while (remaining >= 8) {
			data[x++] = (val >> (remaining - 8)) & 0xFF;
			remaining -= 8;
		}

		if (remaining > 0) {
			uint8_t bitmask = 0xFF << (8 - remaining);
			data[x] = (data[x] & ~bitmask) | ((val << (8 - remaining)) & bitmask);
		}
		length = MAX(start + len, length);
		return true;
	}

	int Message::getInt(int start, int len) const {
		const unsigned ones = ~0;
		unsigned u = getUint(start, len);

		// extend sign bit for the full bit
		if (u & (1 << (len - 1))) u |= ones << len;
		return (int)u;
	}

	bool Message::setInt(int start, int len, int val) {
		return setUint(start, len, (unsigned)val);
	}

	void Message::getText(int start, int len, std::string& str) const {

		int end = start + len;
		str.clear();

		while (start < end) {
			int c = getUint(start, 6);

			// 0       ->   @ and ends the string
			// 1 - 31  ->   65+ ( i.e. setting bit 6 )
			// 32 - 63 ->   32+ ( i.e. doing nothing )

			if (!c) break;
			if (!(c & 32)) c |= 64;

			str += (char)c;
			start += 6;
		}
		return;
	}

	void Message::setText(int start, int len, const char* str) {

		int end = start + len;
		if (end >> 3 >= MAX_AIS_LENGTH) return;

		int idx = 0;
		bool reachedEnd = false;

		while (start < end) {
			unsigned c = 0;

			if (!reachedEnd) {
				c = str[idx++];

				if (!(c >= 32 && c <= 63) && !(c >= 65 && c <= 95)) {
					reachedEnd = true;
					c = 0;
				}
				else if (c & 64)
					c &= 0b00111111;
			}

			setUint(start, 6, c);

			start += 6;
		}

		length = MAX(end, length);

		return;
	}

	void Message::buildNMEA(TAG& tag, int id) {
		const char comma = ',';

		const int IDX_COUNT = 7;
		const int IDX_NUMBER = 9;

		if (id >= 0 && id < 10) ID = id;

		int nAISletters = (length + 6 - 1) / 6;
		int nSentences = (nAISletters + MAX_NMEA_CHARS - 1) / MAX_NMEA_CHARS;

		line.resize(11);

		line[IDX_COUNT] = (char)(nSentences + '0');
		line[IDX_NUMBER] = '0';

		if (nSentences > 1) {
			line += (char)(ID + '0');
			ID = (ID + 1) % 10;
		}

		line += comma;
		if (channel != '?') line += channel;
		line += comma;

		int header = line.length();
		NMEA.clear();

		for (int s = 0, l = 0; s < nSentences; s++) {

			line.resize(header);
			line[IDX_NUMBER]++;

			for (int i = 0; l < nAISletters && i < MAX_NMEA_CHARS; i++, l++)
				line += getLetter(l);

			line += comma;
			line += (char)(((s == nSentences - 1) ? nAISletters * 6 - length : 0) + '0');

			int c = NMEAchecksum(line);
			line += '*';
			line += (c >> 4) < 10 ? (c >> 4) + '0' : (c >> 4) + 'A' - 10;
			line += (c & 0xF) < 10 ? (c & 0xF) + '0' : (c & 0xF) + 'A' - 10;
			NMEA.push_back(line);
		}
	}

	char Message::getLetter(int pos) const {
		int x = (pos * 6) >> 3, y = (pos * 6) & 7;
		uint16_t w = (data[x] << 8) | data[x + 1];

		const int mask = (1 << 6) - 1;
		int l = (w >> (16 - 6 - y)) & mask;

		// zero for bits not formally set
		int overrun = pos * 6 + 6 - length;
		if (overrun > 0) l &= 0xFF << overrun;
		return l < 40 ? (char)(l + 48) : (char)(l + 56);
	}

	void Message::setLetter(int pos, char c) {
		int x = (pos * 6) >> 3, y = (pos * 6) & 7;

		if (length < (pos + 1) * 6) length = (pos + 1) * 6;
		if (length >= MAX_AIS_LENGTH) return;

		c = (c >= 96 ? c - 56 : c - 48) & 0b00111111;

		switch (y) {
		case 0:
			data[x] = (data[x] & 0b00000011) | (c << 2);
			break;
		case 2:
			data[x] = (data[x] & 0b11000000) | c;
			break;
		case 4:
			data[x] = (data[x] & 0b11110000) | (c >> 2);
			data[x + 1] = (data[x + 1] & 0b00111111) | ((c & 3) << 6);
			break;
		case 6:
			data[x] = (data[x] & 0b11111100) | (c >> 4);
			data[x + 1] = (data[x + 1] & 0b00001111) | ((c & 15) << 4);
			break;
		default:
			break;
		}
	}

	bool Filter::SetOption(std::string option, std::string arg) {
		Util::Convert::toUpper(option);

		if (option == "ALLOW_TYPE") {

			std::stringstream ss(arg);
			std::string type_str;
			allow = 0;
			while (ss.good()) {
				getline(ss, type_str, ',');
				unsigned type = Util::Parse::Integer(type_str, 1, 27);
				allow |= 1U << type;
			}
			return true;
		}
		else if (option == "DESC" || option == "DESCRIPTION")
			return true;
		else if (option == "BLOCK_TYPE") {

			std::stringstream ss(arg);
			std::string type_str;
			unsigned block = 0;
			while (ss.good()) {
				getline(ss, type_str, ',');
				unsigned type = Util::Parse::Integer(type_str, 1, 27);
				block |= 1U << type;
			}
			allow = ~block & all_msg;
			return true;
		}
		else if (option == "FILTER") {
			Util::Convert::toUpper(arg);
			on = Util::Parse::Switch(arg);
			return true;
		}
		else if (option == "GPS") {
			Util::Convert::toUpper(arg);
			GPS = Util::Parse::Switch(arg);
			return true;
		}
		else if (option == "AIS") {
			Util::Convert::toUpper(arg);
			AIS = Util::Parse::Switch(arg);
			return true;
		}
		return false;
	}

	std::string Filter::getAllowed() {

		if(allow == all_msg)
			return std::string("ALL");

		std::string ret;
		for (unsigned i = 1; i <= 27; i++) {
			if ((allow & (1U << i)) > 0)
				ret += (!ret.empty() ? std::string(",") : std::string("")) + std::to_string(i);
		}
		return ret;
	}

	bool Filter::include(const Message& msg) {
		if (!on) return true;
		if (!AIS) return false;
		unsigned type = msg.type() & 31;
		return ((1U << type) & allow) != 0;
	}
}
