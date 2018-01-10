#pragma hdrstop
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
    mtk::LogOutput::mShowLogLevel = true;
	Log(lInfo) << "Entering streaming app";
	BlackMagic bm;

	bm.init();

	char c;
	while (true)
    {
    	bool doExit(false);
    	cin >> c;
        switch(c)
        {
        	case 'q': doExit = true;  break;
		}

        if(doExit)
        	break;
    }



	return 0;
}


#pragma comment(lib, "mtkCommon.lib")
