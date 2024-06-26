// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "AudioThread.h"
#include "SpectrumVisualProcessor.h"

class AudioFile
{

public:
    AudioFile();
    virtual ~AudioFile();

    virtual void setOutputFileName(std::string filename);
    virtual std::string getExtension() = 0;
    virtual std::string getOutputFileName();

    virtual bool writeToFile(AudioThreadInputPtr input) = 0;
    virtual bool closeFile() = 0;

protected:
    std::string filenameBase;

};

class SpectrumFile
{

public:
    SpectrumFile();
    virtual ~SpectrumFile();

    virtual void setOutputFileName(std::string filename);
    virtual std::string getExtension() = 0;
    virtual std::string getOutputFileName();

    virtual bool writeToFile(SpectrumVisualDataPtr input) = 0;
    virtual bool closeFile() = 0;

protected:
    std::string filenameBase;

};