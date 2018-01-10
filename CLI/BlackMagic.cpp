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

BlackMagic::BlackMagic()
:
	mStreamingDiscovery 	(NULL),
	mStreamingDevice 		(NULL),
	mStreamingDeviceInput  (NULL),
	mFh 					(NULL),
	mPipe 					(NULL),
	mPlaying 				(false),
	mRecording 			(false),
	mDeviceMode 			(bmdStreamingDeviceUnknown),
	mAutopreview 			(true),
	mTimestampSuffix 		(true),
	mFailCount 			(0),
	mBitrate 				(50000),
	mAutorec 				(true),
    mPresetIndex			(0),
    mFilename			   	("D:\\Temp\\bDeckLing.ts"),
    mVlcexe				("")
{
	Log(lInfo) << "Setting up a BlackMagic object instance..";
}

void BlackMagic::init()
{
	IBMDStreamingDiscovery*			mStreamingDiscovery = NULL;

	// Initialise Blackmagic Streaming API
	HRESULT						result = -1;

     //Initialize the OLE libraries
	CoInitialize(NULL);

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
	BMDevice d;
	HRESULT			result;
	// These messages will happen on the main loop as a result
	// of the message pump.

	// See if it can do input:
	result = device->QueryInterface(IID_IBMDStreamingDeviceInput, (void**)&d.mInput);
	if (FAILED(result))
	{
		// This device doesn't support input. We can ignore this device.
		return S_OK;
	}

	// Ok, we're happy with this device, hold a reference to the device (we
	// also have a reference held from the QueryInterface, too).
	d.mDevice = device;
	device->AddRef();

	if (FAILED(result))
	{
		d.mDevice->Release();
		d.mInput->Release();
		return S_OK;
	}

	BSTR modelName;
	if (device->GetModelName(&modelName) != S_OK)
    {
		return S_OK;
    }

	string dest;
    dest = stdstr(modelName);

    Log(lDebug) << "Model Name is" << dest;
	d.mName = dest;
	d.mStreamingDeviceMode = bmdStreamingDeviceUnknown;
	mDevs.push_back(d);

	int newIndex = 0;//mVideoInputDeviceCombo.AddString(d.name);

	//	mVideoInputDeviceCombo.SetItemDataPtr(newIndex, &d);

	// Check we don't already have a device.
	if (mStreamingDevice != NULL)
	{
		return S_OK;
	}

	activateDevice(newIndex);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE BlackMagic::StreamingDeviceRemoved(IDeckLink* device)
{
	int shutdownactive = 0;
	// We only care about removal of the device we are using
	if (device == mStreamingDevice)
    {
		mStreamingDeviceInput = NULL;
		mStreamingDevice = NULL;
		stopPreview();
		shutdownactive = 1;
	}

	for (std::vector <BMDevice>::iterator d = mDevs.begin(); d != mDevs.end(); ++d )
	{
		if(d->mDevice == device)
        {
			d->mInput->SetCallback(NULL);
			d->mInput->Release();
			d->mDevice->Release();
			mDevs.erase(d);
			break;
		}
	}

//	int cursel=mVideoInputDeviceCombo.GetCurSel();
//	mVideoInputDeviceCombo.ResetContent();
//	for (std::vector <dev>::iterator d = mDevs.begin(); d != mDevs.end(); ++d )
//	{
//		int newIndex = mVideoInputDeviceCombo.AddString(d->name);
//		mVideoInputDeviceCombo.SetItemDataPtr(newIndex, &d);
//	}
//	mVideoInputDeviceCombo.SetCurSel(cursel);
//	if(shutdownactive) {
//		if(mDevs.size() == 0 ) {
//			UpdateUIForNoDevice();
//		} else {
//			activate_device(0);
//		}
//	}

	return S_OK;
}

void BlackMagic::activateDevice(int i)
{
	if(mStreamingDevice != NULL)
    {
		mStreamingDeviceInput->SetCallback(NULL);
	}

	BMDevice d = mDevs.at(i);
	mStreamingDevice = d.mDevice;
	mStreamingDeviceInput = d.mInput;
	mDeviceMode = d.mStreamingDeviceMode;
	mDeviceName = d.mName;

	if (mStreamingDeviceInput->GetCurrentDetectedVideoInputMode(&mInputMode) != S_OK)
    {
		Log(lError) << "Failed to get current detected input mode";
    }

	//mVideoInputDeviceCombo.SetCurSel(i);

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
	updateUIForNewDevice();
	if (mDeviceMode != bmdStreamingDeviceUnknown)
    {
		updateUIForModeChanges();
	}
}

void BlackMagic::updateUIForNewDevice()
{
	// Add video input modes:
	IDeckLinkDisplayModeIterator* inputModeIterator;
	if (FAILED(mStreamingDeviceInput->GetVideoInputModeIterator(&inputModeIterator)))
	{
		MessageBox(NULL, _T("Failed to get input mode iterator"), _T("error"), 0);
		return;
	}

	BMDDisplayMode currentInputModeValue;
	if (FAILED(mStreamingDeviceInput->GetCurrentDetectedVideoInputMode(&currentInputModeValue)))
	{
		MessageBox(NULL, _T("Failed to get current detected input mode"), _T("error"), 0);
		return;
	}

	IDeckLinkDisplayMode* inputMode;
	while (inputModeIterator->Next(&inputMode) == S_OK)
	{
		if (inputMode->GetDisplayMode() == currentInputModeValue) {
			BSTR modeName;
			if (inputMode->GetName(&modeName) != S_OK)
			{
				inputMode->Release();
				inputModeIterator->Release();
				return;
			}
			break;
		}
		inputMode->Release();
	}

	inputModeIterator->Release();
	updateEncodingPresetsUIForInputMode();
}

HRESULT STDMETHODCALLTYPE BlackMagic::StreamingDeviceModeChanged(IDeckLink* device, BMDStreamingDeviceMode mode)
{
	for(std::vector <BMDevice>::iterator d = mDevs.begin(); d != mDevs.end(); ++d )
	{
		if(d->mDevice == device)
        {
			d->mStreamingDeviceMode = mode;
			break;
		}
	}

	if(device != mStreamingDevice)
    {
		return S_OK;
    }

	if(mode == mDeviceMode)
    {
    	return S_OK;
    }

	mDeviceMode = mode;

	updateUIForModeChanges();
	return S_OK;
}

void BlackMagic::stopPreview()
{
//	mEncoding_static.SetWindowText(_T(""));
	mPlaying = false;
	mRecording = false;

	if (mStreamingDeviceInput)
    {
		mStreamingDeviceInput->StopCapture();
    }

	if(mFh)
    {
//		mRecord_button.SetWindowTextW(_T("Record"));
		CloseHandle(mFh);
		mFh = NULL;
	}
	if(mPipe)
    {
		CloseHandle(mPipe);
		mPipe = NULL;
	}
}

void BlackMagic::updateUIForModeChanges()
{
//	CString status = _T(" (unknown)");
	string status;

	switch (mDeviceMode)
	{
		case bmdStreamingDeviceIdle:
			status = _T(" (idle)");
        break;
		case bmdStreamingDeviceEncoding:
			status = _T(" (encoding)");
		break;
		case bmdStreamingDeviceStopping:
			status = _T(" (stopping)");
		break;
	}

    Log(lInfo) <<"Streaming status: "<<status;
//	CString displayName = _T("Device: ") + mDeviceName + status;
//	mConfigBoxStatic.SetWindowText(displayName);
//
//	bool enablePresets = !mPlaying;/*mDeviceMode == bmdStreamingDeviceIdle && mInputMode != bmdModeUnknown;*/
//	mVideoInputDeviceCombo.EnableWindow(enablePresets);
//	mVideoEncodingCombo.EnableWindow(enablePresets);
//	mBitrate_slider.EnableWindow(enablePresets);
//	mPrevcfg_button.EnableWindow(enablePresets);
//	mFolder_button.EnableWindow(!mRecording);
//	mRecord_button.EnableWindow(!enablePresets);
//	mButton_customize.EnableWindow(enablePresets);
//
//	bool enableStartStop = (mDeviceMode == bmdStreamingDeviceIdle || mDeviceMode == bmdStreamingDeviceEncoding);
//	mStartButton.EnableWindow(enableStartStop);
//
//	bool start = /*!mPlaying;*/ mDeviceMode != bmdStreamingDeviceEncoding;
//	mStartButton.SetWindowText(start ? _T("Preview") : _T("Stop"));
	if (mDeviceMode == bmdStreamingDeviceEncoding)
	{
//		if (mInputMode != bmdModeUnknown)
//			StartPreview();
	}
	else
    {
		stopPreview();
    }
}

HRESULT STDMETHODCALLTYPE BlackMagic::MPEG2TSPacketArrived(IBMDStreamingMPEG2TSPacket* mpeg2TSPacket)
{
	int len=mpeg2TSPacket->GetPayloadSize();
	int rec_error=0;
	void *buf;

	mpeg2TSPacket->GetBytes(&buf);
	DWORD dwBytesWritten;
	mTscount.QuadPart+=len;
	if(mPlaying)
    {
		if(!WriteFile(mPipe, buf, len, &dwBytesWritten, NULL))
        {
			if(GetLastError() == ERROR_NO_DATA )
            {
				CloseHandle(mPipe);
				mPipe = CreateNamedPipe(_T("\\\\.\\pipe\\DeckLink.ts"), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_NOWAIT | PIPE_ACCEPT_REMOTE_CLIENTS, 100, 188*1000, 188*1000, 0, NULL);
			}
		}

		if(mFh != NULL && !WriteFile(mFh, buf, len, &dwBytesWritten, NULL))
        {
			rec_error = 1;
		}

		if((mTscount.QuadPart-mLast_tscount.QuadPart)>(1024*10))
        {
			mLast_tscount.QuadPart = mTscount.QuadPart;
			if(mRecording)
            {
				LARGE_INTEGER FileSize;
				GetFileSizeEx( mFh, &FileSize);
                Log(lInfo) << "Recording (kB): "<< (FileSize.QuadPart>>10);
			}
		}
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE BlackMagic::H264VideoInputConnectorScanningChanged(void)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE BlackMagic::H264VideoInputConnectorChanged(void)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE BlackMagic::H264VideoInputModeChanged(void)
{
	if (mStreamingDeviceInput->GetCurrentDetectedVideoInputMode(&mInputMode) != S_OK)
    {
		MessageBox(NULL, _T("Failed to get current detected input mode"), _T("error"), 0);
    }
	else
	{
		updateEncodingPresetsUIForInputMode();
	}

	if (mInputMode == bmdModeUnknown)
	{
	}

	updateUIForModeChanges();

	if((mAutorec || mAutopreview) && !mRecording)
    {
		startPreview();
	}

	return S_OK;
}


HRESULT STDMETHODCALLTYPE BlackMagic::StreamingDeviceFirmwareUpdateProgress(IDeckLink* device, unsigned char percent)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE BlackMagic::H264NALPacketArrived(IBMDStreamingH264NALPacket* nalPacket)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE BlackMagic::H264AudioPacketArrived(IBMDStreamingAudioPacket* audioPacket)
{
	return S_OK;
}

void BlackMagic::startPreview()
{
	if (mPlaying)
    {
		return;
    }

	int iscustom = 0;
	IBMDStreamingVideoEncodingMode* encodingMode = mEncodingModes["Native"];

	int64_t rate = mBitrate;
	IBMDStreamingMutableVideoEncodingMode *em;
	encodingMode->CreateMutableVideoEncodingMode(&em);

	em->SetInt(bmdStreamingEncodingPropertyVideoBitRateKbps, rate);
	mStreamingDeviceInput->SetVideoEncodingMode(em);

	if(!iscustom)
    {
		em->Release();
	}

	mStreamingDeviceInput->GetVideoEncodingMode(&encodingMode);
	encodingMode->GetInt(bmdStreamingEncodingPropertyVideoBitRateKbps, &rate);
	encodingMode->Release();

	mBitrate = (DWORD)rate;
	mPipe = CreateNamedPipe(_T("\\\\.\\pipe\\DeckLink.ts"), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_NOWAIT | PIPE_ACCEPT_REMOTE_CLIENTS, 100, 188*1000, 188*1000, 0, NULL);

	mPlaying = true;
	mLast_tscount.QuadPart = 0;
	mTscount.QuadPart = 0;
	mStreamingDeviceInput->StartCapture();
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

	if(!mAutorec)
    {
		CreateProcessA( NULL, (char*) mVlcexe.c_str(), NULL, NULL, false, 0, NULL, NULL,  &si, &pi);
	}
    else
    {
		onBnClickedButtonRecord();
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

void BlackMagic::updateEncodingPresetsUIForInputMode()
{
	if (mStreamingDevice == NULL)
    {
		return;
    }

	BMDDisplayMode inputMode = BMDDisplayMode(mInputMode);
	mEncodingModes.clear();

	IBMDStreamingVideoEncodingModePresetIterator* presetIterator;

	if (SUCCEEDED(mStreamingDeviceInput->GetVideoEncodingModePresetIterator(inputMode, &presetIterator)))
	{
		IBMDStreamingVideoEncodingMode* encodingMode = NULL;
		BSTR encodingModeName;

		while (presetIterator->Next(&encodingMode) == S_OK)
		{
			encodingMode->GetName(&encodingModeName);
            mEncodingModes[stdstr(encodingModeName)] = encodingMode;
		}
	}
}

void BlackMagic::onBnClickedButtonRecord()
{
	if (mStreamingDevice == NULL)
    {
		return;
    }

	if(mRecording)
    {
		if(mFh != NULL)
        {
			CloseHandle(mFh);
			mFh=NULL;
		}
		mRecording = false;
        Log(lInfo) << "Stopped recording";
	}
    else
    {

		if(mFilename.size())
        {
			if (mTimestampSuffix)
            {
            	string path = getFilePath(mFilename);
				string rootName = getFileNameNoExtension(mFilename);
				string fileName  = joinPath(path, rootName) + getFormattedDateTimeString("_%Y%m%d_%H%M%S") + ".ts";
				mFh = CreateFileA(fileName.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			}
			else
			{
				mFh = CreateFileA(mFilename.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			}

			if(mFh != INVALID_HANDLE_VALUE)
            {
                Log(lInfo) << "Started recording";
				mRecording = true;
			}
		}
	}
}


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

