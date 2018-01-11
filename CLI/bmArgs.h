#ifndef bmArgsH
#define bmArgsH
#include <string>
//---------------------------------------------------------------------------
using std::string;

string Usage(const string& prg);

class Args
{
    public:
                                        Args();
        virtual                        ~Args(){}

										//!Record the stream to a file with name outPutFileName
        bool                            recordToFile;

										//!Record the stream to a file with name outPutFileName
        bool                            convertToMP4;

										//!append timestamp to output file name
        bool                            appendTimeStampToOutputFile;

        								//!Launch VLC to sniff the Pipe, i.e. preview whats is streaming
        bool                            streamToVLCExecutable;

        								//!Start recording immediately
        bool							autoStart;


										//!Record the stream to a file with name outPutFileName
        string                          outputFileName;

        string                          theVersion;
};

#endif


