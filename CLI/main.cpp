#pragma hdrstop
#pragma argsused
#include "BlackMagic.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include "mtkLogger.h"
#include "mtkStringUtils.h"

using namespace std;
using namespace mtk;


int main(int argc, char* argv[])
{
	mtk::LogOutput::mLogToConsole = true;
//    mtk::LogOutput::mShowLogLevel = true;

	Log(lInfo) << "Entering streaming app";
	BlackMagic bm;

	bm.init();

	char c;

    Sleep(6000);
	while (true)
    {
    	bool doExit(false);
    	cin >> c;
        switch(c)
        {
			case 'e':
//            	bm.StopPreview();
            break;
			case 's':
//            	bm.StartPreview();
            break;

        	case 'q':
	            doExit = true;
        	break;
		}

        if(doExit)
        	break;
    }



	return 0;
}


#pragma comment(lib, "mtkCommon.lib")
