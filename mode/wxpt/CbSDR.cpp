// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#define OPENGL

#include "CbSDRDefs.h"
//#include "wx/wxprec.h"

//#ifndef WX_PRECOMP
//#include "wx/wx.h"
//#endif

// #if !wxUSE_GLCANVAS
// #error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
// #endif

#include "CbSDR.h"
#include <iomanip>

#ifdef _OSX_APP_
#include "CoreFoundation/CoreFoundation.h"
#endif

#ifdef USE_HAMLIB
#include "RigThread.h"
#endif

//IMPLEMENT_APP(CbSDR)

#include <fstream>
#include <clocale>

//#include "ActionDialog.h"

#include <memory>

#include "./audio/AudioFileWAV.h"
#include "./audio/AudioSinkFileThread.h"

//#ifdef ENABLE_DIGITAL_LAB
//// console output buffer for windows
//#ifdef _WINDOWS
//class outbuf : public std::streambuf {
//	public:
//	outbuf() {
//		setp(0, 0);
//	}
//	virtual int_type overflow(int_type c = traits_type::eof()) {
//		return fputc(c, stdout) == EOF ? traits_type::eof() : c;
//	}
//};
//#endif
//#endif

#ifdef MINGW_PATCH
	FILE _iob[] = { *stdin, *stdout, *stderr };

	extern "C" FILE * __cdecl __iob_func(void)
	{
		return _iob;
	}

	extern "C" int __cdecl __isnan(double x)
	{
		return _finite(x)?0:1;
	}

	extern "C" int __cdecl __isnanf(float x)
	{
		return _finitef(x)?0:1;
	}
#endif


std::string& filterChars(std::string& s, const std::string& allowed) {
    s.erase(remove_if(s.begin(), s.end(), [&allowed](const char& c) {
        return allowed.find(c) == std::string::npos;
    }), s.end());
    return s;
}

std::string frequencyToStr(long long freq) {
    long double freqTemp;
    
    freqTemp = freq;
    std::string suffix;
    std::stringstream freqStr;
    
    if (freqTemp >= 1.0e9) {
        freqTemp /= 1.0e9;
        freqStr << std::setprecision(10);
        suffix = std::string("GHz");
    } else if (freqTemp >= 1.0e6) {
        freqTemp /= 1.0e6;
        freqStr << std::setprecision(7);
        suffix = std::string("MHz");
    } else if (freqTemp >= 1.0e3) {
        freqTemp /= 1.0e3;
        freqStr << std::setprecision(4);
        suffix = std::string("KHz");
    }
    
    freqStr << freqTemp;
    freqStr << suffix;
    
    return freqStr.str();
}

long long strToFrequency(std::string freqStr) {
    std::string filterStr = filterChars(freqStr, std::string("0123456789.MKGHmkgh"));
    
    size_t numLen = filterStr.find_first_not_of("0123456789.");
    
    if (numLen == std::string::npos) {
        numLen = freqStr.length();
    }
    
    std::string numPartStr = freqStr.substr(0, numLen);
    std::string suffixStr = freqStr.substr(numLen);
    
    std::stringstream numPartStream;
    numPartStream.str(numPartStr);
    
    long double freqTemp = 0;
    
    numPartStream >> freqTemp;
    
    if (suffixStr.length()) {
        if (suffixStr.find_first_of("Gg") != std::string::npos) {
            freqTemp *= 1.0e9;
        } else if (suffixStr.find_first_of("Mm") != std::string::npos) {
            freqTemp *= 1.0e6;
        } else if (suffixStr.find_first_of("Kk") != std::string::npos) {
            freqTemp *= 1.0e3;
        } else if (suffixStr.find_first_of("Hh") != std::string::npos) {
            // ...
        }
    } else if (numPartStr.find_first_of('.') != std::string::npos || freqTemp <= 3000) {
        freqTemp *= 1.0e6;
    }
    
    return (long long) freqTemp;
}



/* class ActionDialogBookmarkCatastophe : public ActionDialog {
public:
    ActionDialogBookmarkCatastophe() : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Bookmark Last-Loaded Backup Failure :( :( :(")) {
        m_questionText->SetLabelText(wxT("All attempts to recover bookmarks have failed. \nWould you like to exit without touching any more save files?\nClick OK to exit without saving; or Cancel to continue anyways."));
    }
    
    void doClickOK() override {
        wxGetApp().getAppFrame()->disableSave(true);
        wxGetApp().getAppFrame()->Close(false);
    }
};*/



/*class ActionDialogBookmarkBackupLoadFailed : public ActionDialog {
public:
    ActionDialogBookmarkBackupLoadFailed() : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Bookmark Backup Load Failure :( :(")) {
        m_questionText->SetLabelText(wxT("Sorry; unable to load your bookmarks backup file. \nWould you like to attempt to load the last succssfully loaded bookmarks file?"));
    }
    
    void doClickOK() override {
        if (wxGetApp().getBookmarkMgr().hasLastLoad("bookmarks.xml")) {
            if (wxGetApp().getBookmarkMgr().loadFromFile("bookmarks.xml.lastloaded",false)) {
                wxGetApp().getBookmarkMgr().updateBookmarks();
                wxGetApp().getBookmarkMgr().updateActiveList();
            } else {
                ActionDialog::showDialog(new ActionDialogBookmarkCatastophe());
            }
        }
    }
};*/


/*class ActionDialogBookmarkLoadFailed : public ActionDialog {
public:
    ActionDialogBookmarkLoadFailed() : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Bookmark Load Failure :(")) {
        m_questionText->SetLabelText(wxT("Sorry; unable to load your bookmarks file. \nWould you like to attempt to load the backup file?"));
    }
    
    void doClickOK() override {
        bool loadOk = false;
        if (wxGetApp().getBookmarkMgr().hasBackup("bookmarks.xml")) {
            loadOk = wxGetApp().getBookmarkMgr().loadFromFile("bookmarks.xml.backup",false);
        }
        if (loadOk) {
            wxGetApp().getBookmarkMgr().updateBookmarks();
            wxGetApp().getBookmarkMgr().updateActiveList();
        } else if (wxGetApp().getBookmarkMgr().hasLastLoad("bookmarks.xml")) {
            ActionDialog::showDialog(new ActionDialogBookmarkBackupLoadFailed());
        } else {
            ActionDialog::showDialog(new ActionDialogBookmarkCatastophe());
        }
    }
};*/



/*class ActionDialogRigError : public ActionDialog {
public:
    explicit ActionDialogRigError(const std::string& message) : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Rig Control Error")) {
        m_questionText->SetLabelText(message);
    }

    void doClickOK() override {
    }
};*/
//静态成员赋值
CbSDR *CbSDR::m_instance = NULL;

CbSDR::CbSDR() : frequency(0), offset(0), ppm(0), snap(1), sampleRate(DEFAULT_SAMPLE_RATE), agcMode(false)
{
        config.load();

        sampleRateInitialized.store(false);
        agcMode.store(true);
        soloMode.store(false);
        shuttingDown.store(false);

        freqChange.store(false);
        typeChange.store(false);
        collectionStatus.store(true);
        spectrumWrite.store(false);

        audioType = "FM";
        /* fdlgTarget = FrequencyDialog::FDIALOG_TARGET_DEFAULT; */
        stoppedDev = nullptr;

        dev = nullptr;

        //set OpenGL configuration:
       /*  m_glContextAttributes = new wxGLContextAttrs();
        
        wxGLContextAttrs glSettings;
        glSettings.CompatibilityProfile().EndList();

        *m_glContextAttributes = glSettings; */
}

CbSDR::~CbSDR()
{
    OnExit();
}

void CbSDR::initAudioDevices() const {
    std::vector<RtAudio::DeviceInfo> devices;
    std::map<int, RtAudio::DeviceInfo> inputDevices, outputDevices;

    AudioThread::enumerateDevices(devices);

    int i = 0;

    for (auto & device : devices) {
        if (device.inputChannels) {
            inputDevices[i] = device;
        }
        if (device.outputChannels) {
            outputDevices[i] = device;
        }
        i++;
    }

    /* wxGetApp().getDemodMgr().setOutputDevices(outputDevices); */
    CbSDR::GetInstance()->getDemodMgr().setOutputDevices(outputDevices); 
}

bool CbSDR::OnInit() {

    //use the current locale most appropriate to this system,
    //so that character-related functions are likely to handle Unicode
    //better (by default, was "C" locale).
    std::setlocale(LC_ALL, "");

//#ifdef _OSX_APP_
//    CFBundleRef mainBundle = CFBundleGetMainBundle();
//    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
//    char path[PATH_MAX];
//    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
//    {
//        // error!
//    }
//    CFRelease(resourcesURL);
//    chdir(path);
//#endif

    // if (!wxApp::OnInit()) {
    //     return false;
    // }

    //Deactivated code to allocate an explicit Console on Windows.
    //This tends to hang the application on heavy demod (re)creation.
    //To continue to debug with std::cout traces, simply run CbSDR in a MINSYS2 compatble shell on Windows:
    //ex: Cygwin shell, Git For Windows Bash shell....
#if (0)
    	if (AllocConsole()) {
    		freopen("CONOUT$", "w", stdout);
    		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
            SetConsoleTitle(L"CbSDR: stdout");
          
    	}

        //refresh
        ofstream ob;
        std::streambuf *sb = std::cout.rdbuf();
        std::cout.rdbuf(sb);
#endif
        
    
   // wxApp::SetAppName(CUBICSDR_INSTALL_NAME);

#ifdef USE_HAMLIB
    t_Rig = nullptr;
    rigThread = nullptr;
    
    RigThread::enumerate();
#endif

    Modem::addModemFactory(ModemFM::factory, "FM", 200000);
    Modem::addModemFactory(ModemNBFM::factory, "NBFM", 12500);
    Modem::addModemFactory(ModemFMStereo::factory, "FMS", 200000);
    Modem::addModemFactory(ModemAM::factory, "AM", 6000);
    Modem::addModemFactory(ModemCW::factory, "CW", 500);
    Modem::addModemFactory(ModemLSB::factory, "LSB", 5400);
    Modem::addModemFactory(ModemUSB::factory, "USB", 5400);
    Modem::addModemFactory(ModemDSB::factory, "DSB", 5400);
    Modem::addModemFactory(ModemIQ::factory, "I/Q", 48000);

#ifdef ENABLE_DIGITAL_LAB
    Modem::addModemFactory(ModemAPSK::factory, "APSK", 200000);
    Modem::addModemFactory(ModemASK::factory, "ASK", 200000);
    Modem::addModemFactory(ModemBPSK::factory, "BPSK", 200000);
    Modem::addModemFactory(ModemDPSK::factory, "DPSK", 200000);
    Modem::addModemFactory(ModemFSK::factory, "FSK", 19200);
    Modem::addModemFactory(ModemGMSK::factory, "GMSK", 19200);
    Modem::addModemFactory(ModemOOK::factory, "OOK", 200000);
    Modem::addModemFactory(ModemPSK::factory, "PSK", 200000);
    Modem::addModemFactory(ModemQAM::factory, "QAM", 200000);
    Modem::addModemFactory(ModemQPSK::factory, "QPSK", 200000);
    Modem::addModemFactory(ModemSQAM::factory, "SQAM", 200000);
    Modem::addModemFactory(ModemST::factory, "ST", 200000);
#endif
    
    frequency = getConfig()->getCenterFreq();
    offset = 0;
    ppm = 0;
    devicesReady.store(false);
    devicesFailed.store(false);
    deviceSelectorOpen.store(false);

    initAudioDevices();

    // Visual Data
    spectrumVisualThread = new SpectrumVisualDataThread();
    
    pipeIQVisualData = std::make_shared<DemodulatorThreadInputQueue>();
    pipeIQVisualData->set_max_num_items(1);
    
    pipeWaterfallIQVisualData = std::make_shared<DemodulatorThreadInputQueue>();
    pipeWaterfallIQVisualData->set_max_num_items(128);
    
    getSpectrumProcessor()->setInput(pipeIQVisualData);
    getSpectrumProcessor()->setup(DEFAULT_FFT_SIZE);
    getSpectrumProcessor()->setHideDC(true);
    
    // I/Q Data
    pipeSDRIQData = std::make_shared<SDRThreadIQDataQueue>();
    pipeSDRIQData->set_max_num_items(1000);
    
    sdrThread = new SDRThread();
    sdrThread->setOutputQueue("IQDataOutput",pipeSDRIQData);

    sdrPostThread = new SDRPostThread();
    sdrPostThread->setInputQueue("IQDataInput", pipeSDRIQData);

    sdrPostThread->setOutputQueue("IQVisualDataOutput", pipeIQVisualData);
    sdrPostThread->setOutputQueue("IQDataOutput", pipeWaterfallIQVisualData);
     
#if CUBICSDR_ENABLE_VIEW_SCOPE
    pipeAudioVisualData = std::make_shared<DemodulatorThreadOutputQueue>();
    pipeAudioVisualData->set_max_num_items(1);
    
    getScopeProcessor()->setup(DEFAULT_SCOPE_FFT_SIZE);//**
    scopeProcessor.setInput(pipeAudioVisualData);
#else
    pipeAudioVisualData = nullptr;
#endif
    
#if CUBICSDR_ENABLE_VIEW_DEMOD
    demodVisualThread = new SpectrumVisualDataThread();
    pipeDemodIQVisualData = std::make_shared<DemodulatorThreadInputQueue>();
    pipeDemodIQVisualData->set_max_num_items(1);
    
    if (getDemodSpectrumProcessor()) {
        getDemodSpectrumProcessor()->setup(DEFAULT_DMOD_FFT_SIZE);//**
        getDemodSpectrumProcessor()->setInput(pipeDemodIQVisualData);
    }
    sdrPostThread->setOutputQueue("IQActiveDemodVisualDataOutput", pipeDemodIQVisualData);
#else
    demodVisualThread = nullptr;
    pipeDemodIQVisualData = nullptr;
    t_DemodVisual = nullptr;
#endif
    //pipeSDRIQData:0x55e2c13f5e70
    //pipeAudioVisualData:0x55e2c13f7340
    std::cout << "pipeAudioVisualData:" << pipeAudioVisualData << std::endl;
    std::cout  << "  pipeIQVisualData:" << pipeIQVisualData << std::endl;
    std::cout  <<  "  pipeWaterfallIQVisualData:"  << pipeWaterfallIQVisualData << std::endl;
    std::cout  << "  pipeSDRIQData:" << pipeSDRIQData << std::endl;
    std::cout  << "  pipeDemodIQVisualData:" << pipeDemodIQVisualData << std::endl;
  
    // Now that input/output queue plumbing is completely done, we can
    //safely starts all the threads:
    t_SpectrumVisual = new std::thread(&SpectrumVisualDataThread::threadMain, spectrumVisualThread);

    if (demodVisualThread != nullptr) {
        t_DemodVisual = new std::thread(&SpectrumVisualDataThread::threadMain, demodVisualThread);
    }

    //Start SDRPostThread last.
    t_PostSDR = new std::thread(&SDRPostThread::threadMain, sdrPostThread);
    

    sdrEnum = new SDREnumerator();
    
    SDREnumerator::setManuals(config.getManualDevices());

    initAppConfig();
    //appframe = new AppFrame();
	t_SDREnum = new std::thread(&SDREnumerator::threadMain, sdrEnum);

//#ifdef __APPLE__
//    int main_policy;
//    struct sched_param main_param;
//
//    main_policy = SCHED_RR;
//    main_param.sched_priority = sched_get_priority_min(SCHED_RR)+2;
//
//    pthread_setschedparam(pthread_self(), main_policy, &main_param);
//#endif

   /*  if (!wxGetApp().getBookmarkMgr().loadFromFile("bookmarks.xml")) {
        if (wxGetApp().getBookmarkMgr().hasBackup("bookmarks.xml")) {
            ActionDialog::showDialog(new ActionDialogBookmarkLoadFailed());
        } else if (wxGetApp().getBookmarkMgr().hasLastLoad("bookmarks.xml")) {
            ActionDialog::showDialog(new ActionDialogBookmarkBackupLoadFailed());
        } else {
            ActionDialog::showDialog(new ActionDialogBookmarkCatastophe());
        }
    } else {
        getBookmarkMgr().updateActiveList();
        getBookmarkMgr().updateBookmarks();
    }
     */

      if (!getBookmarkMgr().loadFromFile("bookmarks.xml")) {
        if (getBookmarkMgr().hasBackup("bookmarks.xml")) {
            //ActionDialog::showDialog(new ActionDialogBookmarkLoadFailed());
        } else if (getBookmarkMgr().hasLastLoad("bookmarks.xml")) {
           //ActionDialog::showDialog(new ActionDialogBookmarkBackupLoadFailed());
        } else {
           // ActionDialog::showDialog(new ActionDialogBookmarkCatastophe());
        }
    } else {
        getBookmarkMgr().updateActiveList();
        getBookmarkMgr().updateBookmarks();
    }

    Start();
    cubServerThread = new  CUBServer();
    cubServerThread->setOutputQueue("cliDataOutput",   pipeCliQueueData);
    pipeCliQueueData->set_max_num_items(1);
    t_CliServer = new std::thread(&CUBServer::threadMain, cubServerThread);


    while(true)
    {
         CliDataPtr data_in;
        
        if (!pipeCliQueueData->pop(data_in, 50*1000)) {
            continue;
        }

        if(data_in && !data_in->msg.empty()) 
        {
            std::string msg = data_in->msg ;
             if(msg == "q") 
             {
                 std::cout << "quit" << std::endl;
                 break;
             }else if(msg == "r")
            {
                startRecording();
            }else if(msg == "f")
            {
                stopRecording();
            }else if(msg == "s")
            {
                std::cout << "msg:"  << msg << std::endl;
                setSdrCollectionStatus(true);
            
            }else if(msg == "x")
            {
                std::cout << "msg:"  << msg << std::endl;
                setSdrCollectionStatus(false);
            }else if(msg == "a")
            {
                setSpectrumWriteStatus(true);
            }
            else if(msg == "z"){
                setSpectrumWriteStatus(false);
            }
            else{
                std::cout << "msg:"  << msg << std::endl;
                setAudioType(msg);
                startRecording();
            }
    
        }
    }

    /* 
    std::cout << " frequency:" << frequency << "  offset:"   <<  offset  << "  ppm:" << ppm << " " << "  snap:"  << snap << "  sampleRate:" << sampleRate<< "  agcMode:" << agcMode  << "  antennaName:" << antennaName << std::endl <<std::fflush;
     */
    return true;
}

int CbSDR::OnExit() {
    saveConfig();
    shuttingDown.store(true);

#if USE_HAMLIB
    if (rigIsActive()) {
        std::cout << "Terminating Rig thread.."  << std::endl << std::flush;
        stopRig();
    }
#endif

    bool terminationSequenceOK = true;

    spectrumSinkThread->terminate(); 

    std::cout << "Terminating Server thread.." << std::endl << std::flush ;
    cubServerThread->terminate();
    //The thread feeding them all should be terminated first, so: 
    std::cout << "Terminating SDR thread.." << std::endl << std::flush ;
    sdrThread->terminate();
    terminationSequenceOK = terminationSequenceOK && sdrThread->isTerminated(3000);

    //in case termination sequence goes wrong, kill App brutally now because it can get stuck. 
    if (!terminationSequenceOK) {
        //no trace here because it could occur if the device is not started.  
        ::exit(11);
    }

    std::cout << "Terminating SDR post-processing thread.." << std::endl << std::flush;
    sdrPostThread->terminate();

    //Wait for termination for sdrPostThread second:: since it is doing
    //mostly blocking push() to the other threads, they must stay alive
    //so that sdrPostThread can complete a processing loop and die.
    terminationSequenceOK = terminationSequenceOK && sdrPostThread->isTerminated(3000);

    //in case termination sequence goes wrong, kill App brutally now because it can get stuck. 
    if (!terminationSequenceOK) {
        std::cout << "Cannot terminate application properly, calling exit() now." << std::endl << std::flush;
        ::exit(12);
    }

    std::cout << "Terminating All Demodulators.." << std::endl << std::flush;
    demodMgr.terminateAll();

    std::cout << "Terminating Visual Processor threads.." << std::endl << std::flush;
    spectrumVisualThread->terminate();
    if (demodVisualThread) {
        demodVisualThread->terminate();
    }
    
    //Wait nicely
    terminationSequenceOK = terminationSequenceOK &&  spectrumVisualThread->isTerminated(1000);

    if (demodVisualThread) {
        terminationSequenceOK = terminationSequenceOK && demodVisualThread->isTerminated(1000);
    }

    //in case termination sequence goes wrong, kill App brutally because it can get stuck. 
    if (!terminationSequenceOK) {
        std::cout << "Cannot terminate application properly, calling exit() now." << std::endl << std::flush;
        ::exit(13);
    }

    if(t_SpectrumSink)
    {
        t_SpectrumSink->join();
    }

    //Then join the thread themselves:
    if (t_SDR) {
        t_SDR->join();
    }

    t_PostSDR->join();
    
    if (t_DemodVisual) {
        t_DemodVisual->join();
    }

    if(t_CliServer){
        t_CliServer->join();
    }

     /**********/
    if (waterfallDataThread)
    {
        waterfallDataThread->terminate();
    }

    if (t_FFTData)
    {
        t_FFTData->join();
        delete t_FFTData;
        t_FFTData = nullptr;
    }

    t_SpectrumVisual->join();

    //Now only we can delete:
    delete spectrumSinkThread;
    spectrumSinkThread = nullptr;

    delete t_SpectrumSink;
    t_SpectrumSink = nullptr;
    
    delete t_SDR;
    t_SDR = nullptr;

    delete sdrThread;
    sdrThread = nullptr;

    delete sdrPostThread;
    sdrPostThread = nullptr;

    delete t_PostSDR;
    t_PostSDR = nullptr;

    delete cubServerThread;
    cubServerThread = nullptr;
    
    delete t_CliServer;
    t_CliServer = nullptr;

    delete t_SpectrumVisual;
    t_SpectrumVisual = nullptr;

    delete spectrumVisualThread;
    spectrumVisualThread = nullptr;

    delete t_DemodVisual;
    t_DemodVisual = nullptr;

    delete demodVisualThread;
    demodVisualThread = nullptr;

   /*  delete m_glContext;
    m_glContext = nullptr; */

    //
    AudioThread::deviceCleanup();

    std::cout << "Application termination complete." << std::endl << std::flush;

    return 0;
}

/* PrimaryGLContext& CbSDR::GetContext(wxGLCanvas *canvas) {
    PrimaryGLContext *glContext;
    if (!m_glContext) {
        m_glContext = new PrimaryGLContext(canvas, nullptr, GetContextAttributes());
    }
    glContext = m_glContext;

    return *glContext;
} */

/* wxGLContextAttrs* CbSDR::GetContextAttributes() {
   
    return m_glContextAttributes;
} */

/* void CbSDR::OnInitCmdLine(wxCmdLineParser& parser) {
    parser.SetDesc (commandLineInfo);
    parser.SetSwitchChars (wxT("-"));
}
 */
/* bool CbSDR::OnCmdLineParsed(wxCmdLineParser& parser) {
    auto *confName = new wxString;
    if (parser.Found("c",confName)) {
        if (!confName->empty()) {
            config.setConfigName(confName->ToStdString());
        }
    } 

#ifdef BUNDLE_SOAPY_MODS
    if (parser.Found("b")) {
        useLocalMod.store(false);
    } else {
        useLocalMod.store(true);
    }
#else
    useLocalMod.store(true);
#endif

    auto *modPath = new wxString;

    if (parser.Found("m",modPath)) {
        if (!modPath->empty()) {
            modulePath = modPath->ToStdString();
        } else {
            modulePath = "";
        }
    }
    
    return true;
}*/

/* void CbSDR::closeDeviceSelector() {
    if (deviceSelectorOpen) {
        deviceSelectorDialog->Close();
    }
} */

/* void CbSDR::deviceSelector() {
    if (deviceSelectorOpen) {
        deviceSelectorDialog->Raise();
        deviceSelectorDialog->SetFocus();
        return;
    } 
    deviceSelectorOpen.store(true);
    wxRect *winRect = getConfig()->getWindow();
    wxPoint pos(wxDefaultPosition);
    if (winRect != nullptr) {
        pos = wxPoint(winRect->x, winRect->y);
    }
    deviceSelectorDialog = new SDRDevicesDialog(appframe, pos);
    deviceSelectorDialog->Show();
}*/

void CbSDR::addRemote(const std::string& remoteAddr) {
    SDREnumerator::addRemote(remoteAddr);
    devicesReady.store(false);
    t_SDREnum = new std::thread(&SDREnumerator::threadMain, sdrEnum);
}

void CbSDR::removeRemote(const std::string& remoteAddr) {
    SDREnumerator::removeRemote(remoteAddr);
}

void CbSDR::sdrThreadNotify(SDRThread::SDRThreadState state, const std::string& message) {

    std::lock_guard < std::mutex > lock(notify_busy);

   
    if (state == SDRThread::SDR_THREAD_INITIALIZED) {
        //appframe->initDeviceParams(getDevice());
    }
    if (state == SDRThread::SDR_THREAD_MESSAGE) {
        notifyMessage = message;
    }
    if (state == SDRThread::SDR_THREAD_FAILED) {
        notifyMessage = message;
//        wxMessageDialog *info;
//        info = new wxMessageDialog(NULL, message, wxT("Error initializing device"), wxOK | wxICON_ERROR);
//        info->ShowModal();
    }
    //if (appframe) { appframe->SetStatusText(message); }
  
}


void CbSDR::sdrEnumThreadNotify(SDREnumerator::SDREnumState state, std::string message) {
    std::lock_guard < std::mutex > lock(notify_busy);

    if (state == SDREnumerator::SDR_ENUM_MESSAGE) {
        notifyMessage = message;
    }
    if (state == SDREnumerator::SDR_ENUM_DEVICES_READY) {
        devs = SDREnumerator::enumerate_devices("", true);
        devicesReady.store(true);
    }
    if (state == SDREnumerator::SDR_ENUM_FAILED) {
        devicesFailed.store(true);
    }
    //if (appframe) { appframe->SetStatusText(message); }
   

}


void CbSDR::setFrequency(long long freq) {
    if(frequency != freq) freqChange.store(true);
    if (freq < sampleRate / 2) {
        freq = sampleRate / 2;
    }
    frequency = freq;
    sdrThread->setFrequency(freq);
    getSpectrumProcessor()->setPeakHold(getSpectrumProcessor()->getPeakHold());

    //make the peak hold act on the current dmod also, like a zoomed-in version.
    if (getDemodSpectrumProcessor()) {
        getDemodSpectrumProcessor()->setPeakHold(getSpectrumProcessor()->getPeakHold());
    }
}

long long CbSDR::getOffset() {
    return offset;
}

void CbSDR::setOffset(long long ofs) {
    offset = ofs;
    
    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->setOffset(offset);
    }
}

void CbSDR::setAntennaName(const std::string& name) {
    antennaName = name;
     
    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->setAntenna(antennaName);
    }
}

const std::string& CbSDR::getAntennaName() {
    return antennaName;
}

void CbSDR::setChannelizerType(SDRPostThreadChannelizerType chType) {
    if (sdrPostThread && !sdrPostThread->isTerminated()) {
        sdrPostThread->setChannelizerType(chType);
    }
}

SDRPostThreadChannelizerType CbSDR::getChannelizerType() {

    if (sdrPostThread && !sdrPostThread->isTerminated()) {
        return sdrPostThread->getChannelizerType();
    }

    return SDRPostThreadChannelizerType::SDRPostPFBCH;
}

long long CbSDR::getFrequency() {
    return frequency;
}

void CbSDR::setAudioType(const std::string type)
{
    if(audioType  != type)
        typeChange.store(true);
    audioType = type;
}

std::string CbSDR::getAudioType()
{
    return audioType;
}

void CbSDR::lockFrequency(long long freq) {
    frequency_locked.store(true);
    lock_freq.store(freq);
    
    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->lockFrequency(freq);
    }
}

bool CbSDR::isFrequencyLocked() {
    return frequency_locked.load();
}

void CbSDR::unlockFrequency() {
    frequency_locked.store(false);
    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->unlockFrequency();
    }
}

void CbSDR::setSampleRate(long long rate_in) {
    sampleRate = rate_in;
    
    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->setSampleRate(sampleRate);
    }

    setFrequency(frequency);

    if (rate_in <= CHANNELIZER_RATE_MAX / 8) {
        /* appframe->setMainWaterfallFFTSize(DEFAULT_FFT_SIZE / 4);
        appframe->getWaterfallDataThread()->getProcessor()->setHideDC(false); */
        CbSDR::GetInstance()->getSpectrumProcessor()->setFFTSize(DEFAULT_FFT_SIZE / 4);
        spectrumVisualThread->getProcessor()->setHideDC(false);
    } else if (rate_in <= CHANNELIZER_RATE_MAX) {
        /* appframe->setMainWaterfallFFTSize(DEFAULT_FFT_SIZE / 2);
        appframe->getWaterfallDataThread()->getProcessor()->setHideDC(false); */
        CbSDR::GetInstance()->getSpectrumProcessor()->setFFTSize(DEFAULT_FFT_SIZE / 2);
        spectrumVisualThread->getProcessor()->setHideDC(false);
    } else if (rate_in > CHANNELIZER_RATE_MAX) {
        /* appframe->setMainWaterfallFFTSize(DEFAULT_FFT_SIZE);
        appframe->getWaterfallDataThread()->getProcessor()->setHideDC(true); */
        CbSDR::GetInstance()->getSpectrumProcessor()->setFFTSize(DEFAULT_FFT_SIZE);
        spectrumVisualThread->getProcessor()->setHideDC(true);
    }
}

void CbSDR::stopDevice(bool store, int waitMsForTermination) {
    
    //First we must stop the threads
    sdrThread->terminate();
    sdrThread->isTerminated(waitMsForTermination);

    if (t_SDR) {
        t_SDR->join();
        delete t_SDR;
        t_SDR = nullptr;
    }
    
    //Only now we can nullify devices
    if (store) {
        stoppedDev = sdrThread->getDevice();
    }
    else {
        stoppedDev = nullptr;
    }

    sdrThread->setDevice(nullptr);
}

void CbSDR::reEnumerateDevices() {
    devicesReady.store(false);
    devs = nullptr;
    SDREnumerator::reset();
    t_SDREnum = new std::thread(&SDREnumerator::threadMain, sdrEnum);
}

void CbSDR::Start( ) {
    static int count = 0;
    while (devs == nullptr && count <= 5)
    {
        count++;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (devs == nullptr && count > 5)
        {
            std::cout << " CbSDR::Start( )--no device found!" << std::endl;
            exit(-1);
        }
    }

    dev = devs->front();
     if (dev != nullptr) {
        
        SoapySDR::ArgInfoList args = dev->getSoapyDevice()->getSettingInfo();
        
        SoapySDR::Kwargs settingArgs;
        SoapySDR::Kwargs streamArgs;   

        /* for (const SoapySDR::ArgInfo &arg : args)
        {
            settingArgs[arg.key] = arg.value;
        }

        if (dev)
        {
            args = dev->getSoapyDevice()->getStreamArgsInfo(SOAPY_SDR_RX, 0);

            if (!args.empty())
            {
                for (const auto &args_i : args)
                {
                    settingArgs[args_i.key] = args_i.value;
                }
            }
        } */

        AppConfig *cfg = CbSDR:: GetInstance()->getConfig();
        DeviceConfig *devConfig = cfg->getDevice(dev->getDeviceId());
        devConfig->setSettings(settingArgs);
        devConfig->setStreamOpts(streamArgs);
        CbSDR:: GetInstance()->setDeviceArgs(settingArgs);
        CbSDR:: GetInstance()->setStreamArgs(streamArgs);
        CbSDR:: GetInstance()->setDevice(dev,0);
    
    }

}

void CbSDR::initAppConfig()
{
    auto *newSinkThread = new SpectrumSinkFileThread();
    auto *afHandler = new SpectrumFileCSV();

    std::stringstream fileName;
    fileName.precision(3);
    fileName << std::fixed << ((long double)CbSDR::GetInstance()->getFrequency() / 1000000.0);

	newSinkThread->setSectrumFileNameBase(fileName.str());
	//attach options:
	newSinkThread->setFileTimeLimit(CbSDR::GetInstance()->getConfig()->getRecordingFileTimeLimit());
    newSinkThread->setSectrumFileHandler(afHandler);

    spectrumSinkThread = newSinkThread;
    t_SpectrumSink = new std::thread(&SpectrumSinkThread::threadMain, newSinkThread);

    // Create and connect the FFT visual data thread
    waterfallDataThread = new FFTVisualDataThread();
    waterfallDataThread->setInputQueue("IQDataInput", getWaterfallVisualQueue());
   // waterfallDataThread->setOutputQueue("FFTDataOutput", visualDataQueue);
   waterfallDataThread->setOutputQueue("FFTDataOutput", spectrumSinkThread->getInputQueue("input"));
    waterfallDataThread->getProcessor()->setHideDC(true);

    t_FFTData = new std::thread(&FFTVisualDataThread::threadMain, waterfallDataThread);

}

void CbSDR::setDevice(SDRDeviceInfo *dev, int waitMsForTermination) {

    sdrThread->terminate();
    sdrThread->isTerminated(waitMsForTermination);
    
    if (t_SDR) {
       t_SDR->join();
       delete t_SDR;
       t_SDR = nullptr;
    }
    
    for (SoapySDR::Kwargs::const_iterator i = settingArgs.begin(); i != settingArgs.end(); i++) {
        sdrThread->writeSetting(i->first, i->second);
    }
    sdrThread->setStreamArgs(streamArgs);
    sdrThread->setDevice(dev);
    
    DeviceConfig *devConfig = config.getDevice(dev->getDeviceId());
    
    SoapySDR::Device *soapyDev = dev->getSoapyDevice();
    
    if (soapyDev) {
        if (long devSampleRate = devConfig->getSampleRate()) {
            sampleRate = dev->getSampleRateNear(SOAPY_SDR_RX, 0, devSampleRate);
            sampleRateInitialized.store(true);
        }
        
        if (!sampleRateInitialized.load()) {
            sampleRate = dev->getSampleRateNear(SOAPY_SDR_RX, 0, DEFAULT_SAMPLE_RATE);
            sampleRateInitialized.store(true);
        } else {
            sampleRate = dev->getSampleRateNear(SOAPY_SDR_RX, 0, sampleRate);
        }

        if (frequency < sampleRate/2) {
            frequency = sampleRate/2;
        }

        setFrequency(frequency);
        setSampleRate(sampleRate);

        setPPM(devConfig->getPPM());
        setOffset(devConfig->getOffset());
        setAGCMode(devConfig->getAGCMode());
        setAntennaName(devConfig->getAntennaName());

        t_SDR = new std::thread(&SDRThread::threadMain, sdrThread);
}
    
    stoppedDev = nullptr;
}

SDRDeviceInfo *CbSDR::getDevice() {
    if (!sdrThread->getDevice() && stoppedDev) {
        return stoppedDev;
    }

    return sdrThread->getDevice();
}

ScopeVisualProcessor *CbSDR::getScopeProcessor() {
    return &scopeProcessor;
}

SpectrumVisualProcessor *CbSDR::getSpectrumProcessor() {
    return spectrumVisualThread->getProcessor();
}

SpectrumVisualProcessor *CbSDR::getDemodSpectrumProcessor() {
    if (demodVisualThread) {
        return demodVisualThread->getProcessor();
    } else {
        return nullptr;
    }
}

DemodulatorThreadOutputQueuePtr CbSDR::getAudioVisualQueue() {
    return pipeAudioVisualData;
}

DemodulatorThreadInputQueuePtr CbSDR::getIQVisualQueue() {
    return pipeIQVisualData;
}

DemodulatorThreadInputQueuePtr CbSDR::getWaterfallVisualQueue() {
    return pipeWaterfallIQVisualData;
}

 BookmarkMgr &CbSDR::getBookmarkMgr() {
    return bookmarkMgr;
}

DemodulatorMgr &CbSDR::getDemodMgr() {
    return demodMgr;
}

/*SessionMgr &CbSDR::getSessionMgr() {
    return sessionMgr;
} */

SDRPostThread *CbSDR::getSDRPostThread() {
    return sdrPostThread;
}

SDRThread *CbSDR::getSDRThread() {
    return sdrThread;
}


void CbSDR::notifyDemodulatorsChanged() {
    
    sdrPostThread->notifyDemodulatorsChanged();
}

long long CbSDR::getSampleRate() {
    return sampleRate;
}

void CbSDR::removeDemodulator(const DemodulatorInstancePtr& demod) {
    if (!demod) {
        return;
    }
    demod->setActive(false);
    sdrPostThread->notifyDemodulatorsChanged();
    /* wxGetApp().getAppFrame()->notifyUpdateModemProperties(); */
}

std::vector<SDRDeviceInfo*>* CbSDR::getDevices() {
    return devs;
}


AppConfig *CbSDR::getConfig() {
    return &config;
}

void CbSDR::saveConfig() {
    std::cout << "saveConfig" << std::endl;
    config.save();
    bookmarkMgr.saveToFile("bookmarks.xml");
}

void CbSDR::setPPM(int ppm_in) {
    ppm = ppm_in;
    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->setPPM(ppm);
    }
}

int CbSDR::getPPM() {
    SDRDeviceInfo *dev = sdrThread->getDevice();
    if (dev) {
        ppm = config.getDevice(dev->getDeviceId())->getPPM();
    }
    return ppm;
}

/* void CbSDR::showFrequencyInput(FrequencyDialog::FrequencyDialogTarget targetMode, const wxString& initString) {
    const wxString demodTitle("Set Demodulator Frequency");
    const wxString freqTitle("Set Center Frequency");
    const wxString bwTitle("Modem Bandwidth (150Hz - 500KHz)");
    const wxString lpsTitle("Lines-Per-Second (1-1024)");
    const wxString avgTitle("Average Rate (0.1 - 0.99)");
    const wxString gainTitle("Gain Entry: "+wxGetApp().getActiveGainEntry());

    wxString title;
    auto activeModem = demodMgr.getActiveContextModem();

    switch (targetMode) {
        case FrequencyDialog::FDIALOG_TARGET_DEFAULT:
        case FrequencyDialog::FDIALOG_TARGET_FREQ:
            title = activeModem ?demodTitle:freqTitle;
            break;
        case FrequencyDialog::FDIALOG_TARGET_BANDWIDTH:
            title = bwTitle;
            break;
        case FrequencyDialog::FDIALOG_TARGET_WATERFALL_LPS:
            title = lpsTitle;
            break;
        case FrequencyDialog::FDIALOG_TARGET_SPECTRUM_AVG:
            title = avgTitle;
            break;
        case FrequencyDialog::FDIALOG_TARGET_GAIN:
            title = gainTitle;
            if (wxGetApp().getActiveGainEntry().empty()) {
                return;
            }
            break;
        default:
            break;
    }

    FrequencyDialog fdialog(appframe, -1, title, activeModem, wxPoint(-100,-100), wxSize(350, 75), wxDEFAULT_DIALOG_STYLE, targetMode, initString);
    fdialog.ShowModal();
}

void CbSDR::showLabelInput() {

    DemodulatorInstancePtr activeDemod = wxGetApp().getDemodMgr().getActiveContextModem();

    if (activeDemod != nullptr) {

        const wxString demodTitle("Edit Demodulator label");

        DemodLabelDialog labelDialog(appframe, -1, demodTitle, activeDemod, wxPoint(-100, -100), wxSize(500, 75), wxDEFAULT_DIALOG_STYLE);
        labelDialog.ShowModal();
    }
}

AppFrame *CbSDR::getAppFrame() {
    return appframe;
} */

void CbSDR::setFrequencySnap(int snap_in) {
    if (snap_in > 1000000) {
        snap_in = 1000000;
    }
    this->snap = snap_in;
}

int CbSDR::getFrequencySnap() {
    return snap;
}

bool CbSDR::areDevicesReady() {
    return devicesReady.load();
}

/* void CbSDR::notifyMainUIOfDeviceChange(bool forceRefreshOfGains) {
    appframe->notifyDeviceChanged();

	if (forceRefreshOfGains) {
		appframe->refreshGainUI();
	}
} */

bool CbSDR::areDevicesEnumerating() {
    return !sdrEnum->isTerminated();
}

bool CbSDR::areModulesMissing() {
    return devicesFailed.load();
}

std::string CbSDR::getNotification() {
    std::string msg;
    std::lock_guard < std::mutex > lock(notify_busy);
    msg = notifyMessage;
   
    return msg;
}

void CbSDR::setDeviceSelectorClosed() {
    deviceSelectorOpen.store(false);
}

bool CbSDR::isDeviceSelectorOpen() {
	return deviceSelectorOpen.load();
}

void CbSDR::setAGCMode(bool mode) {
    agcMode.store(mode);

    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->setAGCMode(mode);
    }
}

bool CbSDR::getAGCMode() {
    return agcMode.load();
}

void CbSDR::setSdrCollectionStatus(bool status)
{
    collectionStatus.store(status);
    if(sdrThread)
        sdrThread->setCollectionStatus(status);
}

bool CbSDR::getSdrCollectionStatus()
{
    return collectionStatus.load();
}

bool CbSDR::getSpectrumWriteStatus()
{
    return spectrumWrite.load();
}

void CbSDR::setSpectrumWriteStatus(bool status)
{
    spectrumWrite.store(status);
     if(spectrumSinkThread)
        spectrumSinkThread->setSpectrumWriteStatus(status); 
}

void CbSDR::setGain(const std::string& name, float gain_in) {
    sdrThread->setGain(name,gain_in);
}

float CbSDR::getGain(const std::string& name) {
    return sdrThread->getGain(name);
}

void CbSDR::setStreamArgs(SoapySDR::Kwargs streamArgs_in) {
    streamArgs = streamArgs_in;
}

void CbSDR::setDeviceArgs(SoapySDR::Kwargs settingArgs_in) {
    settingArgs = settingArgs_in;
}

bool CbSDR::getUseLocalMod() {
    return useLocalMod.load();
}

std::string CbSDR::getModulePath() {
    return modulePath;
}

void CbSDR::setActiveGainEntry(std::string gainName) {
    activeGain = gainName;
}

std::string CbSDR::getActiveGainEntry() {
    return activeGain;
}

void CbSDR::setSoloMode(bool solo) {
    soloMode.store(solo);
}

bool CbSDR::getSoloMode() {
    return soloMode.load();
}

bool CbSDR::isShuttingDown()
{
    return shuttingDown.load();
}

void CbSDR::setActiveRecording()
{
    DemodulatorMgr *mgr = &CbSDR::GetInstance()->getDemodMgr();
    DemodulatorInstancePtr demod = nullptr;

    bool isNew = mgr->getDemodulators().empty();


    if (isNew || freqChange.load() || typeChange.load())
    {
        // read the user label associated with the demodulator
        ModemSettings mSettings;

        mSettings["biastee"] = "false";
        mSettings["digital_agc"] = "false";
        mSettings["direct_samp"] = "0";
        mSettings["iq_swap"] = "false";
        mSettings["offset_tune"] = "false";
        mSettings["testmode"] = "false";

        std::string type = getAudioType();

        long bandwidth = (long)200000;
        long long freq = (long long)getFrequency();
        float squelch_level = -100;
        int squelch_enabled = 1;
        int muted = 0;
        int delta_locked = 0;
        int delta_ofs = 0;
        std::string output_device = "";
        float gain = 1.0f;
        std::wstring user_label = L"";

        std::cout << "type:" << type << std::endl;
        std::cout << "bandwidth:" << bandwidth << std::endl;
        std::cout << "freq:" << freq << std::endl;
        std::cout << "gain:" << gain << std::endl;
        std::cout << "squelch_level:" << squelch_level << std::endl;
        std::cout << "squelch_enabled:" << squelch_enabled << std::endl;
        std::cout << "delta_ofs:" << delta_ofs << std::endl;
        std::cout << "type:" << type << std::endl;

         if (isNew)
        {
            std::cout << "New demodulator at frequency: " << freq <<  "  type:" <<  type << std::endl;
        }
        else
        {
            std::cout << "Moved demodulator to frequency: " << freq << "  type:" <<  type << std::endl;
        }
        freqChange.store(false);
        typeChange.store(false);

        if (!isNew)
        {
            demod = mgr->getActiveContextModem();
            demod->terminate();
        }
        
        demod = CbSDR::GetInstance()->getDemodMgr().newThread();

        demod->setDemodulatorType(type);
        demod->setDemodulatorUserLabel(user_label);
        demod->writeModemSettings(mSettings);
        demod->setBandwidth(bandwidth);
        demod->setFrequency(freq);
        demod->setGain(gain);
        demod->updateLabel(freq);
        demod->setMuted(muted != 0);
        if (delta_locked)
        {
            demod->setDeltaLock(true);
            demod->setDeltaLockOfs(delta_ofs);
        }
        if (squelch_enabled)
        {
            demod->setSquelchEnabled(true);
            demod->setSquelchLevel(squelch_level);
        }

        // Attach to sound output:
        /*   std::map<int, RtAudio::DeviceInfo>::iterator i;

          bool matching_device_found = false;

          for (i =  CbSDR::GetInstance()->getDemodMgr().outputDevices.begin(); i != outputDevices.end(); i++) {
              if (i->second.name == output_device) {
                  std::cout << "setOutputDevice:" << i->first << std::endl;
                  demod->setOutputDevice(i->first);
                  matching_device_found = true;
                  break;
              }
          }
          //if no device is found, choose the first of the list anyway.
          if (!matching_device_found) {
              std::cout << "outputDevices.begin()->first:" << outputDevices.begin()->first << std::endl;
              demod->setOutputDevice(outputDevices.begin()->first);
          } */

        demod->setOutputDevice(0);
        demod->run();
        CbSDR::GetInstance()->notifyDemodulatorsChanged();

        /*  DemodulatorInstancePtr matchingDemod = wxGetApp().getDemodMgr().getLastDemodulatorWith(
                                                                             bmEnt->type,
                                                                             bmEnt->label,
                                                                             bmEnt->frequency,
                                                                             bmEnt->bandwidth);
         //not found, create a new demod instance:
         if (matchingDemod == nullptr) {

             matchingDemod = wxGetApp().getDemodMgr().loadInstance(bmEnt->node);
             matchingDemod->run();
             wxGetApp().notifyDemodulatorsChanged();
         } */

        demod->setActive(true);

        long long freq1 = demod->getFrequency();
        long long currentFreq = CbSDR::GetInstance()->getFrequency();
        long long currentRate = CbSDR::GetInstance()->getSampleRate();

        if ((abs(freq1 - currentFreq) > currentRate / 2) || (abs(currentFreq - freq1) > currentRate / 2)) {
            CbSDR::GetInstance()->setFrequency(freq1);
        }

        mgr->setActiveDemodulator(demod, false);
        mgr->updateLastState();
    }
}

void CbSDR::startRecording()
{
    setActiveRecording();
    DemodulatorInstancePtr activeDemod = demodMgr.getActiveContextModem();
    if (activeDemod && !activeDemod->isRecording() && getConfig()->verifyRecordingPath())
    {
        activeDemod->setRecording(true);
    }
}

void CbSDR::stopRecording()
{
    DemodulatorInstancePtr activeDemod = demodMgr.getActiveContextModem();

    if (activeDemod && activeDemod->isRecording())
    {
        std::cout << " stop xxxxxx00000" << std::endl;
        activeDemod->setRecording(false);
    }
}

void CbSDR::toggleActiveDemodRecording() {
    if (!CbSDR::GetInstance()->getConfig()->verifyRecordingPath()) {
        return;
    }

    //notifyDemodulatorsChanged();
    DemodulatorInstancePtr activeDemod = demodMgr.getActiveContextModem();

    if (activeDemod) {
        activeDemod->setRecording(!activeDemod->isRecording());
        /* wxGetApp().getBookmarkMgr().updateActiveList(); */
    }else
    {
        std::cout << "activeDemod is nullptr" << std::endl;
    }
}

/* int CbSDR::FilterEvent(wxEvent& event) {
    if (!appframe) {
        return -1;
    }

    if (event.GetEventType() == wxEVT_KEY_DOWN || event.GetEventType() == wxEVT_CHAR_HOOK) {
		return appframe->OnGlobalKeyDown((wxKeyEvent&)event);
    }
    
    if (event.GetEventType() == wxEVT_KEY_UP || event.GetEventType() == wxEVT_CHAR_HOOK) {
        return appframe->OnGlobalKeyUp((wxKeyEvent&)event);
    }
    
    return -1;  // process normally
} */

#ifdef USE_HAMLIB
RigThread *CbSDR::getRigThread() {
    return rigThread;
}

void CbSDR::initRig(int rigModel, std::string rigPort, int rigSerialRate) {
    if (rigThread) {

        rigThread->terminate();
        rigThread->isTerminated(1000);
    }

    if (t_Rig && t_Rig->joinable()) {
        t_Rig->join();
    }

    //now we can delete
    if (rigThread) {

        delete rigThread;
        rigThread = nullptr;
    }
    if (t_Rig) {
      
        delete t_Rig;
        t_Rig = nullptr;
    }

    rigThread = new RigThread();
    rigThread->initRig(rigModel, rigPort, rigSerialRate);
    rigThread->setControlMode(wxGetApp().getConfig()->getRigControlMode());
    rigThread->setFollowMode(wxGetApp().getConfig()->getRigFollowMode());
    rigThread->setCenterLock(wxGetApp().getConfig()->getRigCenterLock());
    rigThread->setFollowModem(wxGetApp().getConfig()->getRigFollowModem());

    t_Rig = new std::thread(&RigThread::threadMain, rigThread);
}

void CbSDR::stopRig() {
    if (!rigThread) {
        return;
    }
    
    if (rigThread) {
        rigThread->terminate();
        rigThread->isTerminated(1000);

        if (rigThread->getErrorState()) {
            ActionDialog::showDialog(new ActionDialogRigError(rigThread->getErrorMessage()));
        }
    }

    if (t_Rig && t_Rig->joinable()) {
        t_Rig->join();   
    }

    //now we can delete
    if (rigThread) {

        delete rigThread;
        rigThread = nullptr;
    }

    if (t_Rig) {
       
        delete t_Rig;
        t_Rig = nullptr;
    }
}

bool CbSDR::rigIsActive() {
    return (rigThread && !rigThread->isTerminated());
}

#endif
