// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "AudioThread.h"
#include "../process/SpectrumVisualProcessor.h"

class AudioSinkThread : public IOThread {

public:

    AudioSinkThread();
    ~AudioSinkThread() override;

	void run() override;
	void terminate() override;

    virtual void sink(AudioThreadInputPtr input) = 0;
    virtual void inputChanged(AudioThreadInput oldProps, AudioThreadInputPtr newProps) = 0;

protected:
    std::recursive_mutex m_mutex;
    AudioThreadInputQueuePtr inputQueuePtr;

};


class SpectrumSinkThread : public IOThread {

public:

    SpectrumSinkThread();
    ~SpectrumSinkThread() override;

	void run() override;
	void terminate() override;

    virtual void sink(SpectrumVisualDataPtr input) = 0;
    virtual void inputChanged(SpectrumVisualData oldProps, SpectrumVisualDataPtr newProps) = 0;

    void setSpectrumWriteStatus(bool status);
    bool getSpectrumWriteStatus();

protected:
    std::recursive_mutex m_mutex;
    SpectrumVisualDataQueuePtr inputQueuePtr;
    std::atomic_bool spectrumWrite;
    std::atomic_bool spectrumChange;
};


