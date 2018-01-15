#pragma hdrstop
#include "bmArgs.h"
#include <sstream>
#include <iomanip>
//---------------------------------------------------------------------------

using namespace std;

Args::Args()
:
theVersion("0.5.0"),
streamToVLCExecutable(false),
recordToFile(false),
outputFileName(""),
autoStart(false),
appendTimeStampToOutputFile(false),
bitRate(20000)
{}

string Usage(const string& prg)
{
    Args args;
    stringstream usage;
    usage<< "bm.exe is a command line driven application that can be used to capture a video stream from a BlackMagic device\n\n";
    usage << "\nUSAGE for "<<prg<<"\n\n";
    usage<<left;
    usage<<setfill('.');
    usage<<setw(25)<<"-version"                     <<" Prints the current version.\n";
    usage<<setw(25)<<"-a"           				<<" Start recording immediately.\n";
    usage<<setw(25)<<"-b"           				<<" Set bitrate (kB/s).\n";
    usage<<setw(25)<<"-t"           				<<" Append a time stamp to outputfile name.\n";
    usage<<setw(25)<<"-f outputFilename"            <<" Write the stream to this file.\n";
    usage<<setw(25)<<"-usevlc"      		        <<" Pipe video stream to VLC.\n";
    usage<<setw(25)<<"-? "                          <<" Shows the help screen.\n\n";

    usage<<"Version: "<<args.theVersion<<"\n\n";
    usage<<"\nSmith Lab., Allen Institute for Brain Science, 2018\n";
    return usage.str();
}