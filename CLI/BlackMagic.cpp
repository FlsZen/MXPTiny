#pragma hdrstop
#include "BlackMagic.h"
#include <iostream>
#include <algorithm>
#include "mtkLogger.h"
#include "mtkStringUtils.h"
#include "mtkFileUtils.h"
//---------------------------------------------------------------------------
using namespace std;
using namespace mtk;
std::string stdstr(BSTR wstr);

BlackMagic::BlackMagic(bool vlcPreview, bool useTimeStamp)
:
	mPreviewOnVLC           (vlcPreview),
	mStreamingDiscovery 	(NULL),
	mStreamingDevice 		(NULL),
	mStreamingDeviceInput   (NULL),
	mDeckLinkIterator		(NULL),
	mFileHandle 			(NULL),
	mPipe 					(NULL),
	mPlaying 				(false),
	mDeviceMode 			(bmdStreamingDeviceUnknown),
	mTimeStampOutputFile 	(useTimeStamp),
	mBitRate 				(25000),
    mFileName			   	("bDeckLing.ts"),
    mVLCExecutable	 		("C:\\Program Files (x86)\\VideoLAN\\VLC\\vlc.exe stream://\\\\\\.\\pipe\\DeckLink.ts")
{
	Log(lInfo) << "Setting up a BlackMagic object instance.";
}

BlackMagic::~BlackMagic()
{
	Log(lInfo) << "In BlackMagic destructor.";
}

string BlackMagic::getOutputFileName()
{
	return mFileName;
}

void BlackMagic::init()
{
	IBMDStreamingDiscovery*	mStreamingDiscovery = NULL;

	// Initialise Blackmagic Streaming API
     //Initialize the OLE libraries
	CoInitialize(NULL);
    HRESULT result(-1);

	result = CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&mDeckLinkIterator);
	if (FAILED(result))
	{
		Log(lError) <<"This application requires the Blackmagic Streaming drivers installed.\nPlease install the Blackmagic Streaming drivers to use the features of this application.";
        throw("Bad init..");
	}

	result = CoCreateInstance(CLSID_CBMDStreamingDiscovery, NULL, CLSCTX_ALL, IID_IBMDStreamingDiscovery, (void**)&mStreamingDiscovery);
	if (FAILED(result))
	{
		Log(lError) <<"This application requires the Blackmagic Streaming drivers installed.\nPlease install the Blackmagic Streaming drivers to use the features of this application.";
        throw("Bad init..");
	}

	// Note: at this point you may get device notification messages!
	result = mStreamingDiscovery->InstallDeviceNotifications(this);

	if (FAILED(result))
	{
		Log(lError) << "Failed to install device notifications for the Blackmagic Streaming devices";
        throw("Bad init..");
	}
}

void BlackMagic::disconnect()
{
	if(mFileHandle)
    {
		stopRecordingToFile();
    }

	stopCapture();
}

bool BlackMagic::isDeviceReady()
{
	return mStreamingDevice ? true : false;
}


void BlackMagic::startCapture()
{
	if(mPlaying)
    {
    	Log(lWarning) <<"Tried to start already playing device";
		return;
    }

    if(!mStreamingDevice)
    {
    	Log(lError) << "Streaming device is not ready";
        return;
    }

	IBMDStreamingVideoEncodingMode* encodingMode = mEncodingModes["Native"];
    if(!encodingMode)
    {
    	throw(string("Encoding mode is NULL"));
    }

	int64_t rate = mBitRate;
	IBMDStreamingMutableVideoEncodingMode *em;
	encodingMode->CreateMutableVideoEncodingMode(&em);

	em->SetInt(bmdStreamingEncodingPropertyVideoBitRateKbps, rate);
	mStreamingDeviceInput->SetVideoEncodingMode(em);
	em->Release();

	mStreamingDeviceInput->GetVideoEncodingMode(&encodingMode);
	encodingMode->GetInt(bmdStreamingEncodingPropertyVideoBitRateKbps, &rate);
	encodingMode->Release();

	mBitRate = (DWORD)rate;
	mPipe = CreateNamedPipe(_T("\\\\.\\pipe\\DeckLink.ts"), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_NOWAIT | PIPE_ACCEPT_REMOTE_CLIENTS, 100, 188*1000, 188*1000, 0, NULL);

	mLast_tscount.QuadPart = 0;
	mTscount.QuadPart = 0;

    //================ Start capture stream ==================
    //Should check return value on this one..
	mStreamingDeviceInput->StartCapture();

	mPlaying = true;

	if(mPreviewOnVLC)
	{
        PROCESS_INFORMATION pi;
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

		CreateProcessA(NULL, (char*) mVLCExecutable.c_str(), NULL, NULL, false, 0, NULL, NULL,  &si, &pi);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

void BlackMagic::stopCapture()
{
	mPlaying = false;
	if(mStreamingDeviceInput)
    {
		mStreamingDeviceInput->StopCapture();
    }

	if(mPipe)
    {
		CloseHandle(mPipe);
		mPipe = NULL;
	}
}

void BlackMagic::captureAvailableEncodingModes()
{
	if(mStreamingDevice == NULL)
    {
		return;
    }

	BMDDisplayMode inputMode = BMDDisplayMode(mInputMode);
	mEncodingModes.clear();

	IBMDStreamingVideoEncodingModePresetIterator* presetIterator;

	if(SUCCEEDED(mStreamingDeviceInput->GetVideoEncodingModePresetIterator(inputMode, &presetIterator)))
	{
		IBMDStreamingVideoEncodingMode* encodingMode = NULL;
		BSTR encodingModeName;

		while (presetIterator->Next(&encodingMode) == S_OK)
		{
			encodingMode->GetName(&encodingModeName);
            Log(lInfo) << "Adding \""<< stdstr(encodingModeName) <<"\" mode to available encoding modes";
            mEncodingModes[stdstr(encodingModeName)] = encodingMode;
		}
	}
}

void BlackMagic::startRecordingToFile(const string& fName)
{
	if(!mPlaying)
    {
		startCapture();
    }

    if(mFileHandle)
    {
        CloseHandle(mFileHandle);
        mFileHandle = NULL;
    }

    if(fName.size())
    {
        mFileName = fName;
    }

    if(mFileName.size())
    {
        if (mTimeStampOutputFile)
        {
            string path 		= getFilePath(mFileName);
            string rootName 	= getFileNameNoExtension(mFileName);
            mFileName  			= joinPath(path, rootName) + getFormattedDateTimeString("_%Y%m%d_%H%M%S") + ".ts";
        }

        mFileHandle = CreateFileA(mFileName.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

        if(mFileHandle != INVALID_HANDLE_VALUE)
        {
            Log(lInfo) << "Started recording to file: " << mFileName;
        }
    }
}

void BlackMagic::stopRecordingToFile()
{
	if(mFileHandle)
    {
		CloseHandle(mFileHandle);
		mFileHandle = NULL;
	}

    Log(lInfo) << "Stopped recording to file";
}

void BlackMagic::activateDevice(int i)
{
	if(mStreamingDevice != NULL)
    {
		mStreamingDeviceInput->SetCallback(NULL);
	}

	BMDevice d 				= mDevices.at(i);
	mStreamingDevice 		= d.mDevice;
	mStreamingDeviceInput 	= d.mInput;
	mDeviceMode 			= d.mStreamingDeviceMode;

	if (mStreamingDeviceInput->GetCurrentDetectedVideoInputMode(&mInputMode) != S_OK)
    {
		Log(lError) << "Failed to get current input mode";
    }

	// Now install our callbacks. To do this, we must query our own delegate
	// to get it's IUnknown interface, and pass this to the device input interface.
	// It will then query our interface back to a IBMDStreamingH264InputCallback,
	// if that's what it wants.
	// Note, although you may be tempted to cast directly to an IUnknown, it's
	// not particular safe, and is invalid COM.
	IUnknown* ourCallbackDelegate;
	this->QueryInterface(IID_IUnknown, (void**)&ourCallbackDelegate);

	HRESULT result = d.mInput->SetCallback(ourCallbackDelegate);

	// Finally release ourCallbackDelegate, since we created a reference to it
	// during QueryInterface. The device will hold its own reference.
	ourCallbackDelegate->Release();

	reportCurrentDisplayMode();
	if (mDeviceMode != bmdStreamingDeviceUnknown)
    {
		reportDeviceModeChange();
	}
}

//This is not really doing anything..
void BlackMagic::reportCurrentDisplayMode()
{
	// Add video input modes:
	IDeckLinkDisplayModeIterator* inputModeIterator;
	if (FAILED(mStreamingDeviceInput->GetVideoInputModeIterator(&inputModeIterator)))
	{
		Log(lError) << "Failed to get input mode iterator";
		return;
	}

	BMDDisplayMode currentInputModeValue;
	if (FAILED(mStreamingDeviceInput->GetCurrentDetectedVideoInputMode(&currentInputModeValue)))
	{
		Log(lError) << "Failed to get current detected input mode";
		return;
	}

	IDeckLinkDisplayMode* inputMode;
	while (inputModeIterator->Next(&inputMode) == S_OK)
	{
		if (inputMode->GetDisplayMode() == currentInputModeValue)
        {
			BSTR modeName;
			if (inputMode->GetName(&modeName) != S_OK)
			{
				inputMode->Release();
				inputModeIterator->Release();
				return;
			}
            else
            {
            	Log(lInfo) << "Current display mode is: " << stdstr(modeName);
            }
			break;
		}
		inputMode->Release();
	}

	inputModeIterator->Release();
}

HRESULT STDMETHODCALLTYPE BlackMagic::QueryInterface(REFIID iid, LPVOID* ppv)
{
	HRESULT result = E_NOINTERFACE;
	if (ppv == NULL)
    {
		return E_POINTER;
    }

	*ppv = NULL;

	if (iid == IID_IUnknown)
	{
		*ppv = static_cast<IUnknown*>(static_cast<IBMDStreamingDeviceNotificationCallback*>(this));
		AddRef();
		result = S_OK;
	}
	else if (iid == IID_IBMDStreamingDeviceNotificationCallback)
	{
		*ppv = static_cast<IBMDStreamingDeviceNotificationCallback*>(this);
		AddRef();
		result = S_OK;
	}
	else if (iid == IID_IBMDStreamingH264InputCallback)
	{
		*ppv = static_cast<IBMDStreamingH264InputCallback*>(this);
		AddRef();
		result = S_OK;
	}

	return result;
}

HRESULT STDMETHODCALLTYPE BlackMagic::StreamingDeviceArrived(IDeckLink* device)
{
	Log(lInfo) << "Streaming device arrived";
	BMDevice d;
	HRESULT	 result;

	// These messages will happen on the main loop as a result
	// of the message pump.

	// See if it can do input:
	result = device->QueryInterface(IID_IBMDStreamingDeviceInput, (void**)&d.mInput);
	if(FAILED(result))
	{
		// This device doesn't support input. We can ignore this device.
		return S_OK;
	}

	// Ok, we're happy with this device, hold a reference to the device (we
	// also have a reference held from the QueryInterface, too).
	d.mDevice = device;
	device->AddRef();

	if(FAILED(result))
	{
		d.mDevice->Release();
		d.mInput->Release();
		return S_OK;
	}

	BSTR modelName;
	if(device->GetModelName(&modelName) != S_OK)
    {
		return S_OK;
    }

	string dest;
    dest = stdstr(modelName);

    Log(lDebug) << "Model Name is: " << dest;
	d.mName = dest;
	d.mStreamingDeviceMode = bmdStreamingDeviceUnknown;
	mDevices.push_back(d);

	// Check we don't already have a device.
	if(mStreamingDevice != NULL)
	{
		return S_OK;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE BlackMagic::StreamingDeviceRemoved(IDeckLink* device)
{
	Log(lInfo) << "Streaming device is being removed";

	// We only care about removal of the device that we are using
	if (device == mStreamingDevice)
    {
		mStreamingDeviceInput 	= NULL;
		mStreamingDevice 		= NULL;
		stopCapture();
	}

	for (std::vector <BMDevice>::iterator d = mDevices.begin(); d != mDevices.end(); ++d )
	{
		if(d->mDevice == device)
        {
			d->mInput->SetCallback(NULL);
			d->mInput->Release();
			d->mDevice->Release();
			mDevices.erase(d);
			break;
		}
	}

    if(mDevices.size() == 0)
    {
        Log(lError) << "No devices present";
    }

	return S_OK;
}

HRESULT STDMETHODCALLTYPE BlackMagic::StreamingDeviceModeChanged(IDeckLink* device, BMDStreamingDeviceMode mode)
{
	Log(lInfo) << "Streaming device mode changed";
	for(std::vector <BMDevice>::iterator d = mDevices.begin(); d != mDevices.end(); ++d )
	{
		if(d->mDevice == device)
        {
			d->mStreamingDeviceMode = mode;
			break;
		}
	}

	if(device != mStreamingDevice)
    {
    	activateDevice(0);
		return S_OK;
    }

	mDeviceMode = mode;
	reportDeviceModeChange();
	return S_OK;
}

//This is a callback whose return value we are not handling..
HRESULT STDMETHODCALLTYPE BlackMagic::MPEG2TSPacketArrived(IBMDStreamingMPEG2TSPacket* mpeg2TSPacket)
{
	void *buf;
	int len = mpeg2TSPacket->GetPayloadSize();

    //Get the data
	mpeg2TSPacket->GetBytes(&buf);
	mTscount.QuadPart += len;

	DWORD dwBytesWritten;
	if(!mPlaying)
    {
    	//Log(lError) << "MPEG package arrived, but we are not playing ??!";
    	return S_OK;
    }
    //Write to pipe..
    if(!WriteFile(mPipe, buf, len, &dwBytesWritten, NULL))
    {
        if(GetLastError() == ERROR_NO_DATA)
        {
            CloseHandle(mPipe);
            mPipe = CreateNamedPipe(_T("\\\\.\\pipe\\DeckLink.ts"), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_NOWAIT | PIPE_ACCEPT_REMOTE_CLIENTS, 100, 188*1000, 188*1000, 0, NULL);
        }
    }

    //Write to file
    if(mFileHandle != NULL && !WriteFile(mFileHandle, buf, len, &dwBytesWritten, NULL))
    {
        Log(lError) << "There was a problem writing to file";
    	return S_OK;
    }

    if((mTscount.QuadPart - mLast_tscount.QuadPart) > (1024*1000))
    {
        mLast_tscount.QuadPart = mTscount.QuadPart;
        if(mFileHandle)
        {
            LARGE_INTEGER FileSize;
            GetFileSizeEx(mFileHandle, &FileSize);

            Log(lInfo) << "Recording (kB * 100): "<< (FileSize.QuadPart>>10);

        }
    }

	return S_OK;
}

HRESULT STDMETHODCALLTYPE BlackMagic::H264VideoInputModeChanged(void)
{
	if (mStreamingDeviceInput->GetCurrentDetectedVideoInputMode(&mInputMode) != S_OK)
    {
		Log(lError) << "Failed to get current detected input mode";
    }
	else
	{
		captureAvailableEncodingModes();
	}

	reportDeviceModeChange();
	return S_OK;
}

void BlackMagic::reportDeviceModeChange()
{
	string status;
	switch (mDeviceMode)
	{
		case bmdStreamingDeviceIdle:	 status = "(idle)";     break;
		case bmdStreamingDeviceEncoding: status = "(encoding)";	break;
		case bmdStreamingDeviceStopping: status = "(stopping)";	break;
	}

    Log(lInfo) <<"Streaming status: "<<status;
}


//========================================================================================================================================================
HRESULT STDMETHODCALLTYPE BlackMagic::StreamingDeviceFirmwareUpdateProgress(IDeckLink* device, unsigned char percent)	{return E_NOTIMPL;}
HRESULT STDMETHODCALLTYPE BlackMagic::H264NALPacketArrived(IBMDStreamingH264NALPacket* nalPacket)						{return E_NOTIMPL;}
HRESULT STDMETHODCALLTYPE BlackMagic::H264AudioPacketArrived(IBMDStreamingAudioPacket* audioPacket)						{return E_NOTIMPL;}
HRESULT STDMETHODCALLTYPE BlackMagic::H264VideoInputConnectorScanningChanged(void)										{return E_NOTIMPL;}
HRESULT STDMETHODCALLTYPE BlackMagic::H264VideoInputConnectorChanged(void)												{return E_NOTIMPL;}

std::string ConvertWCSToMBS(const wchar_t* pstr, long wslen);
std::string stdstr(BSTR bstr)
{
    int wslen = ::SysStringLen(bstr);
    return ConvertWCSToMBS(bstr, wslen);
}

std::string ConvertWCSToMBS(const wchar_t* pstr, long wslen)
{
    int len = ::WideCharToMultiByte(CP_ACP, 0, pstr, wslen, NULL, 0, NULL, NULL);

    std::string dblstr(len, '\0');
    len = ::WideCharToMultiByte(CP_ACP, 0 /* no flags */,
                                pstr, wslen /* not necessary NULL-terminated */,
                                &dblstr[0], len,
                                NULL, NULL /* no default char */);
    return dblstr;
}

