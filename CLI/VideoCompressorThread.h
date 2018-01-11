#ifndef atVideoCompressorThreadH
#define atVideoCompressorThreadH
#include "mtkThread.h"
#include <functional>
#include "Poco/Process.h"
#include "Poco/Timespan.h"
//---------------------------------------------------------------------------




class VideoCompressorThread : public mtk::Thread
{
	typedef boost::function<void(int, int)> Callback;
    public:
								VideoCompressorThread();
								~VideoCompressorThread();
		void					setFFMPEGLocation(const string& loc);
        void					setFFMPEGOutFileArguments(const string& args);
		void					setInputFile(const string& f);
		void					assignCallBacks(Callback one, Callback two, Callback three);
        void					deleteSourceFileAfterCompression(bool what);
        void					renameSourceFileAfterCompression(bool what);
        virtual void            run();
        string 					getInputFileName();


    protected:
    	string					mFFMPEGLocation;
        string				    mFFMPEGOutFileArguments;
        string					mInputFileName;
        bool					mDeleteSourceFile;
        bool					mRenameSourceFile;

	    Callback 				onEnter;
	    Callback 				onProgress;
	    Callback 				onExit;
	    Poco::Timespan 			mEstimatedDuration;
        int						parseFFMPEGOutput(const string& str);
};
#endif
