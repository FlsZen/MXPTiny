#pragma hdrstop
#include "atVideoCompressorThread.h"
#include "mtkLogger.h"
#include "Poco/Process.h"
#include "Poco/PipeStream.h"
#include "Poco/StreamCopier.h"
#include <fstream>
#include "mtkFileUtils.h"
#include "mtkMathUtils.h"
#include "Poco/Path.h"
#include "Poco/File.h"
using Poco::Path;

using namespace Poco;
using namespace mtk;

VideoCompressorThread::VideoCompressorThread()
:
mFFMPEGLocation("ffmpeg.exe"),
mDeleteSourceFile(false)
{
}

VideoCompressorThread::~VideoCompressorThread()
{}

void VideoCompressorThread::deleteSourceFileAfterCompression(bool what)
{
	mDeleteSourceFile = what;
}

void VideoCompressorThread::renameSourceFileAfterCompression(bool what)
{
	mRenameSourceFile = what;
}

void VideoCompressorThread::setFFMPEGLocation(const string& loc)
{
	mFFMPEGLocation = loc;
}

string VideoCompressorThread::getInputFileName()
{
	return mInputFileName;
}

void VideoCompressorThread::assignCallBacks(Callback one, Callback two, Callback three)
{
	onEnter 	= one;
    onProgress 	= two;
    onExit 		= three;
}

void VideoCompressorThread::setFFMPEGOutFileArguments(const string& args)
{
	mFFMPEGOutFileArguments = args;
}

void VideoCompressorThread::setInputFile(const string& f)
{
	mInputFileName = f;
}


void VideoCompressorThread::run()
{
	mIsStarted = true;
    if(onEnter)
    {
    	onEnter(0,0);
    }

	mIsRunning = true;

	try
    {
        string outputFolder(getFilePath(mInputFileName));
        string outFile(joinPath(outputFolder, getFileNameNoExtension(mInputFileName)));
        outFile += ".mp4";

        if(fileExists(outFile))
        {
            removeFile(outFile);
            Log(lWarning) << "Removed file: "<<outFile;
        }

        //Setup arguments
        StringList args;
        args.append("-i " + mInputFileName);
        args.append(mFFMPEGOutFileArguments);
        args.append(outFile);
        Log(lDebug5) << "OutputFile compression arguments: " << mFFMPEGOutFileArguments;

        //Capture output
        Pipe outPipe;

        //ffmpeg need a 'q' in order to quit
        Pipe inPipe;
        mEstimatedDuration = 0;

		//Capture stdout and stderr to outPipe
        ProcessHandle ph = Process::launch(mFFMPEGLocation, args, &inPipe, &outPipe, &outPipe);

        //Use stream objects to read and write to the pipes
        PipeInputStream istr(outPipe);
		PipeOutputStream ostr(inPipe);

        string s;
		int progress(0);
        int c = istr.get();
        bool exitRequested(false);
        while (c != -1)
        {
            s += (char) c;
            if(c == '\n' || c == '\r')
            {
            	progress = parseFFMPEGOutput(s);
                Log(lDebug5) << s;
                s.clear();
                if(onProgress && progress > 0)
                {
	                onProgress(progress, 0);
                }
            }

            c = istr.get();
            if(mIsTimeToDie && !exitRequested)
            {
                //Tell ffmpeg to quit
                ostr << std::string(1, 'q');
                ostr.close();
                exitRequested = true;
            }
        }

        int rc = ph.wait();
        Log(lInfo) <<"RC: "<<rc;

        //Check if we are to delete the source file
        if(exitRequested != true)
        {
        	if(mDeleteSourceFile)
            {
            	Log(lInfo) << "Deleting source video file: "<<mInputFileName;
                removeFile(mInputFileName);
            }
            else if(mRenameSourceFile)
            {
            	 Poco::File p(mInputFileName);
                 string path(getFilePath(mInputFileName));
                 string fn(getFileNameNoExtension(mInputFileName));
                 string fne(getFileExtension(mInputFileName));

                 string f(joinPath(path, fn + "_compressed" + "." + fne));
                 p.renameTo(f);
            }
        }
    }
    catch(...)
    {
		Log(lError) << "Unhandled exception..";
    }

	Log(lInfo) << "Exiting thread..";

    if(onExit)
    {
    	onExit(0,0);
    }

	mIsRunning = false;
	mIsFinished = true;
}

//Parsing text can be ugly..
int VideoCompressorThread::parseFFMPEGOutput(const string& s)
{
	//    Log(lDebug5) << "Parsing string: "<<s;
    //Check and parse string
    if(mEstimatedDuration <= 0 && contains("Duration", s))
    {
        StringList l(s, ',');
        if(l.size() > 2)
        {
            StringList dur(trim(l[0]), ' ');
            if(dur.size() == 2)
            {
                //Extract time here
                string time(dur[1]);
                Log(lInfo) << "Estimated compress duration is: "<<time;
                StringList timeDur(time, ':');
                if(timeDur.size() == 3)
                {
                    mEstimatedDuration.assign(0, toInt(timeDur[0]), toInt(timeDur[1]), toInt(timeDur[2]), 0);
                    //We just parsed part of the header.. no progress really
                    return -1;
                }
            }
        }
    }

    //Now we are looking for progress...
	if(contains("time", s))
    {
    	//We have some progress
        StringList l(s, ' ');

      	//get string starting with 'time'
        string time = l.getLineContaining("time");
        StringList timeVal(time, '=');
        if(timeVal.size() == 2)
        {
        	StringList timeDur(timeVal[1], ':');
	        //Create poco time stamp object
			Poco::Timespan ts;
            if(timeDur.size() == 3)
            {
                ts.assign(0, toInt(timeDur[0]), toInt(timeDur[1]), toInt(timeDur[2]), 0);
            }

            int nowSeconds = ts.totalSeconds();
            int estSeconds = mEstimatedDuration.totalSeconds();
            double progress =  (nowSeconds / (double) estSeconds) * 100;
            return round(progress);
        }
    }
    return -1;
}

