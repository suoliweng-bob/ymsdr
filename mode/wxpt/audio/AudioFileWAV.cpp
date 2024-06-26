// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AudioFileWAV.h"
#include "CbSDR.h"
#include <iomanip>

//limit file size to 2GB (- margin) for maximum compatibility.
//#define MAX_WAV_FILE_SIZE (0x7FFFFFFF - 1024)
//#define MAX_BIN_FILE_SIZE (0x7FFFFFFF - 1024)
#define MAX_WAV_FILE_SIZE (0x1400000 - 1024)
#define MAX_BIN_FILE_SIZE (0x1400000 - 1024)

// Simple endian io read/write handling from 
// http://www.cplusplus.com/forum/beginner/31584/#msg171056
namespace little_endian_io
{
    template <typename Word>
    std::ostream& write_word(std::ostream& outs, Word value, unsigned size = sizeof(Word)) {
        for (; size; --size, value >>= 8) {
            outs.put(static_cast <char> (value & 0xFF));
        }
        return outs;
    }

    template <typename Word>
    std::istream& read_word(std::istream& ins, Word& value, unsigned size = sizeof(Word)) {
        value = 0;
        for (unsigned n = 0; n < size; ++n) {
            value |= ins.get() << (8 * n);
        }
        return ins;
    }
}

namespace big_endian_io
{
    template <typename Word>
    std::ostream& write_word(std::ostream& outs, Word value, unsigned size = sizeof(Word)) {
        while (size) {
            outs.put(static_cast <char> ((value >> (8 * --size)) & 0xFF));
        }
        return outs;
    }

    template <typename Word>
    std::istream& read_word(std::istream& ins, Word& value, unsigned size = sizeof(Word)) {
        for (value = 0; size; --size) {
            value = (value << 8) | ins.get();
        }
        return ins;
    }
}

using namespace little_endian_io;

AudioFileWAV::AudioFileWAV() : AudioFile(), dataChunkPos(0) {
}

AudioFileWAV::~AudioFileWAV() = default;


std::string AudioFileWAV::getExtension()
{
    return "wav";
}

bool AudioFileWAV::writeToFile(AudioThreadInputPtr input)
{
    if (!outputFileStream.is_open()) {

        std::string ofName = getOutputFileName();
               
        outputFileStream.open(ofName.c_str(), std::ios::binary);
		currentFileSize = 0;
		writeHeaderToFileStream(input);
    }

	size_t maxRoomInCurrentFileInSamples = getMaxWritableNumberOfSamples(input);
	//**std::cout << "maxRoomInCurrentFileInSamples: " << maxRoomInCurrentFileInSamples << " input->data.size():"<< input->data.size() << std::endl;
	if (maxRoomInCurrentFileInSamples >= input->data.size()) {
		writePayloadToFileStream(input, 0, input->data.size());
	}
	else {
		//we complete the current file and open another:
		writePayloadToFileStream(input, 0, maxRoomInCurrentFileInSamples);

		closeFile();

		// Open a new file with the next sequence number, and dump the rest of samples in it.
		currentSequenceNumber++;
		currentFileSize = 0;

		std::string ofName = getOutputFileName();
		
		outputFileStream.open(ofName.c_str(), std::ios::binary);
		
		writeHeaderToFileStream(input);
		writePayloadToFileStream(input, maxRoomInCurrentFileInSamples, input->data.size());
	}

    return true;
}

bool AudioFileWAV::closeFile()
{
    if (outputFileStream.is_open()) {
        size_t file_length = outputFileStream.tellp();

        // Fix the data chunk header to contain the data size
        outputFileStream.seekp(dataChunkPos + 4);
        write_word(outputFileStream, file_length - (dataChunkPos + 8), 4);

        // Fix the file header to contain the proper RIFF chunk size, which is (file size - 8) bytes
        outputFileStream.seekp(0 + 4);
        write_word(outputFileStream, file_length - 8, 4);

        outputFileStream.close();
		currentFileSize = 0;
    }

    return true;
}

void AudioFileWAV::writeHeaderToFileStream(const AudioThreadInputPtr& input) {

	// Based on simple wav file output code from
	// http://www.cplusplus.com/forum/beginner/166954/

	// Write the wav file headers
	outputFileStream << "RIFF----WAVEfmt "; // (chunk size to be filled in later)
	write_word(outputFileStream, 16, 4); // no extension data
	write_word(outputFileStream, 1, 2); // PCM - integer samples
	write_word(outputFileStream, input->channels, 2); // channels
	write_word(outputFileStream, input->sampleRate, 4); // samples per second (Hz)
	write_word(outputFileStream, (input->sampleRate * 16 * input->channels) / 8, 4); // (Sample Rate * BitsPerSample * Channels) / 8
	write_word(outputFileStream, input->channels * 2, 2); // data block size (size of integer samples, one for each channel, in bytes)
	write_word(outputFileStream, 16, 2); // number of bits per sample (use a multiple of 8)

										 // Write the data chunk header
	dataChunkPos = outputFileStream.tellp();
	currentFileSize = dataChunkPos;
	outputFileStream << "data----";  // (chunk size to be filled in later)
}

void AudioFileWAV::writePayloadToFileStream(const AudioThreadInputPtr& input, size_t startInputPosition, size_t endInputPosition) {

	// Prevent clipping
	float intScale = (input->peak < 1.0) ? 32767.0f : (32767.0f / input->peak);

	if (input->channels == 1) {
		for (size_t i = startInputPosition, iMax = endInputPosition; i < iMax; i++) {

			write_word(outputFileStream, int(input->data[i] * intScale), 2);
			
			currentFileSize += 2;
		}
	}
	else if (input->channels == 2) {
		for (size_t i = startInputPosition, iMax = endInputPosition / 2; i < iMax; i++) {

			write_word(outputFileStream, int(input->data[i * 2] * intScale), 2);
			write_word(outputFileStream, int(input->data[i * 2 + 1] * intScale), 2);

			currentFileSize += 4;
		}
	}
}

size_t AudioFileWAV::getMaxWritableNumberOfSamples(const AudioThreadInputPtr& input) const {

	long long remainingBytesInFile = (long long)(MAX_WAV_FILE_SIZE) - currentFileSize;

    return (size_t)(remainingBytesInFile / (input->channels * 2));
	
}

void AudioFileWAV::setOutputFileName(std::string filename) {

	if (filename != filenameBase) {

		currentSequenceNumber = 0;
	}

	AudioFile::setOutputFileName(filename);
}

std::string AudioFileWAV::getOutputFileName() {

	std::string recPath = CbSDR::GetInstance()->getConfig()->getRecordingPath();

	// Strip any invalid characters from the name
	std::string stripChars("<>:\"/\\|?*");
	std::string filenameBaseSafe = filenameBase;

	for (size_t i = 0, iMax = filenameBaseSafe.length(); i < iMax; i++) {
		if (stripChars.find(filenameBaseSafe[i]) != std::string::npos) {
			filenameBaseSafe.replace(i, 1, "_");
		}
	}

	// Create output file name
	std::stringstream outputFileName;
	outputFileName << recPath << filePathSeparator << filenameBaseSafe;

	//customized part: append a sequence number.
	if (currentSequenceNumber > 0) {
		outputFileName << "_" << std::setfill('0') << std::setw(3) << currentSequenceNumber;
	}

	int idx = 0;

	// If the file exists; then find the next non-existing file in sequence.
	std::string fileNameCandidate = outputFileName.str();

	while (FILE *file = fopen((fileNameCandidate + "." + getExtension()).c_str(), "r")) {
		fclose(file);
		fileNameCandidate = outputFileName.str() + "-" + std::to_string(++idx);
	}

	return fileNameCandidate + "." + getExtension();
}


//**-------------------------------------------------------------------------

SpectrumFileCSV::SpectrumFileCSV() : SpectrumFile(), dataChunkPos(0) {
}

SpectrumFileCSV::~SpectrumFileCSV() = default;


std::string SpectrumFileCSV::getExtension()
{
    return "bin";
}

bool SpectrumFileCSV::writeToFile(SpectrumVisualDataPtr input)
{
    if (!outputFileStream.is_open()) {

        std::string ofName = getOutputFileName();
    
        outputFileStream.open(ofName.c_str(), std::ios::binary);
		currentFileSize = 0;
		writeHeaderToFileStream(input);
    }

	size_t maxRoomInCurrentFileInSamples = getMaxWritableNumberOfSamples(input);
	size_t spectrumSize = input->spectrum_points.size() * sizeof(float) + input->spectrum_hold_points.size() * sizeof(float) + 2 * sizeof(double) + sizeof(long long) + sizeof(int) + 2 * sizeof(size_t);
	//**std::cout << "maxRoomInCurrentFileInSamples: " << maxRoomInCurrentFileInSamples << " spectrumSize:"<< spectrumSize << std::endl;
	if (maxRoomInCurrentFileInSamples >= spectrumSize) {
		writePayloadToFileStream(input, spectrumSize);
	}
	else {
		//we complete the current file and open another:
		//writePayloadToFileStream(input);

		closeFile();

		// Open a new file with the next sequence number, and dump the rest of samples in it.
		currentSequenceNumber++;
		currentFileSize = 0;

		std::string ofName = getOutputFileName();
		
		outputFileStream.open(ofName.c_str(), std::ios::binary);
		
		writeHeaderToFileStream(input);
		writePayloadToFileStream(input, spectrumSize); 
	}

    return true;
}

bool SpectrumFileCSV::closeFile()
{
    if (outputFileStream.is_open()) {
        size_t file_length = outputFileStream.tellp();
        outputFileStream.seekp(0);

        outputFileStream.write(reinterpret_cast<const char*>(&file_length), sizeof(file_length));
        outputFileStream.close();
		currentFileSize = 0;
    }

    return true;
}

void SpectrumFileCSV::writeHeaderToFileStream(const SpectrumVisualDataPtr& input) {
	size_t placeholder = 0;
	outputFileStream.write(reinterpret_cast<const char*>(&placeholder), sizeof(placeholder));
	dataChunkPos = outputFileStream.tellp();
	currentFileSize += dataChunkPos;
}

void SpectrumFileCSV::writePayloadToFileStream(const SpectrumVisualDataPtr& input, size_t inputSize) {

	size_t spectrum_points_size = input->spectrum_points.size();
	size_t spectrum_hold_points_size = input->spectrum_hold_points.size();
	outputFileStream.write(reinterpret_cast<const char *>(&spectrum_points_size), sizeof(spectrum_points_size));
	outputFileStream.write(reinterpret_cast<const char *>(&spectrum_hold_points_size), sizeof(spectrum_hold_points_size));

	// 写入向量数据
	outputFileStream.write(reinterpret_cast<const char *>(input->spectrum_points.data()), input->spectrum_points.size() * sizeof(float));
	outputFileStream.write(reinterpret_cast<const char *>(input->spectrum_hold_points.data()), input->spectrum_hold_points.size() * sizeof(float));

	// 写入其他成员变量
	outputFileStream.write(reinterpret_cast<const char *>(&input->fft_ceiling), sizeof(input->fft_ceiling));
	outputFileStream.write(reinterpret_cast<const char *>(&input->fft_floor), sizeof(input->fft_floor));
	outputFileStream.write(reinterpret_cast<const char *>(&input->centerFreq), sizeof(input->centerFreq));
	outputFileStream.write(reinterpret_cast<const char *>(&input->bandwidth), sizeof(input->bandwidth));

	currentFileSize += inputSize;
}

size_t SpectrumFileCSV::getMaxWritableNumberOfSamples(const SpectrumVisualDataPtr& input) const {

	long long remainingBytesInFile = (long long)(MAX_BIN_FILE_SIZE) - currentFileSize;

    return (size_t)remainingBytesInFile;
	
}

void SpectrumFileCSV::setOutputFileName(std::string filename) {

	if (filename != filenameBase) {

		currentSequenceNumber = 0;
	}

	SpectrumFile::setOutputFileName(filename);
}

std::string SpectrumFileCSV::getOutputFileName() {

	std::string recPath = CbSDR::GetInstance()->getConfig()->getRecordingPath();

	// Strip any invalid characters from the name
	std::string stripChars("<>:\"/\\|?*");
	std::string filenameBaseSafe = filenameBase;

	for (size_t i = 0, iMax = filenameBaseSafe.length(); i < iMax; i++) {
		if (stripChars.find(filenameBaseSafe[i]) != std::string::npos) {
			filenameBaseSafe.replace(i, 1, "_");
		}
	}

	// Create output file name
	std::stringstream outputFileName;
	outputFileName << recPath << filePathSeparator << filenameBaseSafe;

	//customized part: append a sequence number.
	if (currentSequenceNumber > 0) {
		outputFileName << "_" << std::setfill('0') << std::setw(3) << currentSequenceNumber;
	}

	int idx = 0;

	// If the file exists; then find the next non-existing file in sequence.
	std::string fileNameCandidate = outputFileName.str();

	while (FILE *file = fopen((fileNameCandidate + "." + getExtension()).c_str(), "r")) {
		fclose(file);
		fileNameCandidate = outputFileName.str() + "-" + std::to_string(++idx);
	}

	return fileNameCandidate + "." + getExtension();
}