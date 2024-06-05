// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AudioSinkThread.h"

#define HEARTBEAT_CHECK_PERIOD_MICROS (50 * 1000) 

AudioSinkThread::AudioSinkThread() {
    inputQueuePtr = std::make_shared<AudioThreadInputQueue>();
    inputQueuePtr->set_max_num_items(1000);
    setInputQueue("input", inputQueuePtr);
}

AudioSinkThread::~AudioSinkThread() = default;

void AudioSinkThread::run() {
#ifdef __APPLE__
    pthread_t tID = pthread_self();	 // ID of this thread
    int priority = sched_get_priority_max(SCHED_RR) - 1;
    sched_param prio = { priority }; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_RR, &prio);
#endif

//**std::cout << "AudioSinkThread::run()xxxxxx stoping:" << stopping << std::endl;
    AudioThreadInputPtr inp;
    AudioThreadInput inputRef;

    while (!stopping) {
        if (!inputQueuePtr->pop(inp, HEARTBEAT_CHECK_PERIOD_MICROS)) {
            continue;
        }
        //** std::cout << "AudioSinkThread::run()->pop(inp, HEARTBEAT_CHECK_PERIOD_MICROS)xxxxxx" << stopping << std::endl;
        if (inputRef.channels != inp->channels || 
                inputRef.frequency != inp->frequency ||
                inputRef.inputRate != inp->inputRate ||
                inputRef.sampleRate != inp->sampleRate) {

            inputChanged(inputRef, inp);

            inputRef.channels = inp->channels;
            inputRef.frequency = inp->frequency;
            inputRef.inputRate = inp->inputRate;
            inputRef.sampleRate = inp->sampleRate;
        }

        sink(inp);
    }
}

void AudioSinkThread::terminate() {
    IOThread::terminate();
    inputQueuePtr->flush();
}

//**--------------------------------------------------------------------------------------------

SpectrumSinkThread::SpectrumSinkThread() {
    spectrumWrite.store(false);
    spectrumChange.store(false);
    inputQueuePtr = std::make_shared<SpectrumVisualDataQueue>();
    inputQueuePtr->set_max_num_items(1000);
    setInputQueue("input", inputQueuePtr);
}

SpectrumSinkThread::~SpectrumSinkThread() = default;

void SpectrumSinkThread::run() {
#ifdef __APPLE__
    pthread_t tID = pthread_self();	 // ID of this thread
    int priority = sched_get_priority_max(SCHED_RR) - 1;
    sched_param prio = { priority }; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_RR, &prio);
#endif

//**std::cout << "SpectrumSinkThread::run()xxxxxx stoping:" << stopping << std::endl;
    SpectrumVisualDataPtr inp;
    SpectrumVisualData inputRef;

    while (!stopping) {
        if (!inputQueuePtr->pop(inp, HEARTBEAT_CHECK_PERIOD_MICROS)) {
            continue;
        }
         //**std::cout << "AudioSinkThread::run()->pop(inp, HEARTBEAT_CHECK_PERIOD_MICROS)xxxxxx" << stopping << std::endl;
        if ( inputRef.centerFreq != inp->centerFreq ||
                inputRef.bandwidth != inp->bandwidth || 
                spectrumChange.load()) {

            inputChanged(inputRef, inp);

            inputRef.centerFreq = inp->centerFreq;
            inputRef.bandwidth = inp->bandwidth;
            spectrumChange.store(false);
        }

        if (spectrumWrite.load())
            sink(inp);
    }
}

void SpectrumSinkThread::terminate() {
    IOThread::terminate();
    inputQueuePtr->flush();
}

void SpectrumSinkThread::setSpectrumWriteStatus(bool status) {
    if (status != spectrumWrite.load())
    {
        spectrumChange.store(true);
    }
    spectrumWrite.store(status);
}

bool SpectrumSinkThread::getSpectrumWriteStatus() {
    return spectrumWrite.load();
}