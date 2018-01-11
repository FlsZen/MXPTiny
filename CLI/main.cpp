#pragma hdrstop
#include "BlackMagic.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include "mtkLogger.h"
#include "mtkStringUtils.h"
#include "bmArgs.h"
#include "mtkGetOptions.h"
#include "mtkStopWatch.h"


using namespace std;
using namespace mtk;

void ProcessCommandLineArguments(int argc, char* argv[], Args& args);
int main(int argc, char* argv[])
{
    mtk::LogOutput::mLogToConsole = true;
    mtk::LogOutput::mShowLogLevel = false;
    Log(lInfo) << "Entering streaming app";
    gLogger.setLogLevel(lInfo);
	bool doContinue = true;
	Args args;
   	try
    {
        if(argc < 2)
        {
            cout<<Usage(argv[0])<<endl;
            exit(0);
        }

	    Log(lInfo) << "Processing commandline..";
        ProcessCommandLineArguments(argc, argv, args);

        //Create a blackmagic object instance
        BlackMagic bm(args.streamToVLCExecutable, args.appendTimeStampToOutputFile);
        bm.init();

        //Waiting for device to get ready
        StopWatch sw;
        sw.start();
        while(bm.isDeviceReady() == false)
        {
        	Sleep(50);
            if(sw.getElapsedTime() > Poco::Timespan::SECONDS * 3.)
            {
            	throw(string("Device failed to get ready"));
            }
        };

        if(args.autoStart)
        {
        	bm.startRecordingToFile(args.outputFileName);
        }

        //We need to monitor standard in order to shut down streaming
        char c;
        while (true)
        {
            bool doExit(false);
            cin >> c;
            switch(c)
            {
                case 'q': doExit = true;  									break;
                case 'r': bm.startRecordingToFile(args.outputFileName);		break;
                case 's': bm.stopRecordingToFile();							break;
            }

            if(doExit)
                break;
        }

        //Disconnect device gracefully
        bm.disconnect();
    }
    catch(const string& e)
    {
        Log(lError)<<"There was a problem: "<<e<<endl;
    }

    gLogger.logToConsole(false);
	return 0;
}

void ProcessCommandLineArguments(int argc, char* argv[], Args& args)
{
    char c;

    while ((c = getOptions(argc, argv, (const char*) ("tav:f:u:"))) != -1)
    {
        switch (c)
        {
        	case 'a':
                    args.autoStart = true;
            break;

        	case 'u':
				if(string(mtk::optarg) == "sevlc")
                {
					Log(lInfo) << "Preview on VLC";
                    args.streamToVLCExecutable = true;
                }
            break;
            case ('f'):
                if(string(mtk::optarg).size())
		        {
                	args.outputFileName = string(mtk::optarg);
                	Log(lInfo) << "Recording to file: " << args.outputFileName;
                }
            break;
            case ('v'):
                if(string(mtk::optarg) == "ersion" )
                {
                    cout<<"Version: "<<args.theVersion<<endl;
                    exit(-1);
                }
            break;
            case ('t'):
            	args.appendTimeStampToOutputFile = true;
                Log(lInfo) << "Adding timestamp to output file name.";

            break;

            case ('?'):
            {
                    cout<<Usage(argv[0])<<endl;
                    exit(-1);
            }
            default:
            {
                string str = argv[mtk::optind-1];
                if(str != "-?")
                {
                    cout<<"*** Illegal option:\t"<<argv[mtk::optind-1]<<" ***\n"<<endl;
                }
                exit(-1);
            }
        }
    }
}

#pragma comment(lib, "mtkCommon.lib")
#pragma comment(lib, "poco_foundation-static.lib")
