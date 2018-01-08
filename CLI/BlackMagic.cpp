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


std::string ConvertWCSToMBS(const wchar_t* pstr, long wslen);
std::string stdstr(BSTR bstr)
{
    int wslen = ::SysStringLen(bstr);
    return ConvertWCSToMBS((wchar_t*)bstr, wslen);
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


BlackMagic::BlackMagic()
:
	m_streamingDiscovery 	(NULL),
	m_streamingDevice 		(NULL),
	m_streamingDeviceInput  (NULL),
	m_fh 					(NULL),
	m_pipe 					(NULL),
	m_playing 				(false),
	m_recording 			(false),
	m_deviceMode 			(bmdStreamingDeviceUnknown),
	m_autopreview 			(true),
	m_timestampSuffix 		(true),
	m_failCount 			(0),
	m_bitrate 				(50000),
	m_autorec 				(true),
    m_presetIndex			(0),
    m_filename			   	("D:\\Temp\\bDeckLing.ts"),
    m_vlcexe				("")
{
	Log(lInfo) << "Setting up a BlackMagic object instance..";
}

void BlackMagic::init()
{
	IBMDStreamingDiscovery*			m_streamingDiscovery = NULL;

	// Initialise Blackmagic Streaming API
	HRESULT						result = -1;

     //Initialize the OLE libraries
	CoInitialize(NULL);

	result = CoCreateInstance(CLSID_CBMDStreamingDiscovery, NULL, CLSCTX_ALL, IID_IBMDStreamingDiscovery, (void**)&m_streamingDiscovery);
	if (FAILED(result))
	{
		Log(lError) <<"This application requires the Blackmagic Streaming drivers installed.\nPlease install the Blackmagic Streaming drivers to use the features of this application.";
        throw("Bad init..");
	}

	// Note: at this point you may get device notification messages!
	result = m_streamingDiscovery->InstallDeviceNotifications(this);

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
	dev d;
	HRESULT			result;
	// These messages will happen on the main loop as a result
	// of the message pump.

	// See if it can do input:
	result = device->QueryInterface(IID_IBMDStreamingDeviceInput, (void**)&d.input);
	if (FAILED(result))
	{
		// This device doesn't support input. We can ignore this device.
		return S_OK;
	}

	// Ok, we're happy with this device, hold a reference to the device (we
	// also have a reference held from the QueryInterface, too).
	d.device = device;
	device->AddRef();

	if (FAILED(result))
	{
		d.device->Release();
		d.input->Release();
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
	d.name = dest;
	d.mode = bmdStreamingDeviceUnknown;
	m_devs.push_back(d);

	int newIndex = 0;//m_videoInputDeviceCombo.AddString(d.name);

	//	m_videoInputDeviceCombo.SetItemDataPtr(newIndex, &d);

	// Check we don't already have a device.
	if (m_streamingDevice != NULL)
	{
		return S_OK;
	}

	activate_device(newIndex);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE BlackMagic::StreamingDeviceRemoved(IDeckLink* device)
{
	int shutdownactive=0;
	// We only care about removal of the device we are using
	if (device == m_streamingDevice)
    {
		m_streamingDeviceInput = NULL;
		m_streamingDevice = NULL;
		StopPreview();
		shutdownactive=1;
	}

	for (std::vector <dev>::iterator d = m_devs.begin(); d != m_devs.end(); ++d )
	{
		if(d->device == device) {
			d->input->SetCallback(NULL);
			d->input->Release();
			d->device->Release();
			m_devs.erase(d);
			break;
		}
	}
//	int cursel=m_videoInputDeviceCombo.GetCurSel();
//	m_videoInputDeviceCombo.ResetContent();
//	for (std::vector <dev>::iterator d = m_devs.begin(); d != m_devs.end(); ++d )
//	{
//		int newIndex = m_videoInputDeviceCombo.AddString(d->name);
//		m_videoInputDeviceCombo.SetItemDataPtr(newIndex, &d);
//	}
//	m_videoInputDeviceCombo.SetCurSel(cursel);
//	if(shutdownactive) {
//		if(m_devs.size() == 0 ) {
//			UpdateUIForNoDevice();
//		} else {
//			activate_device(0);
//		}
//	}

	return S_OK;
}

void BlackMagic::activate_device(int i)
{
	if(m_streamingDevice != NULL)
    {
		m_streamingDeviceInput->SetCallback(NULL);
	}

	dev d = m_devs.at(i);
	m_streamingDevice = d.device;
	m_streamingDeviceInput = d.input;
	m_deviceMode = d.mode;
	m_deviceName = d.name;

	if (m_streamingDeviceInput->GetCurrentDetectedVideoInputMode(&m_inputMode) != S_OK)
    {
		Log(lError) << "Failed to get current detected input mode";
    }

	//m_videoInputDeviceCombo.SetCurSel(i);

	// Now install our callbacks. To do this, we must query our own delegate
	// to get it's IUnknown interface, and pass this to the device input interface.
	// It will then query our interface back to a IBMDStreamingH264InputCallback,
	// if that's what it wants.
	// Note, although you may be tempted to cast directly to an IUnknown, it's
	// not particular safe, and is invalid COM.
	IUnknown* ourCallbackDelegate;
	this->QueryInterface(IID_IUnknown, (void**)&ourCallbackDelegate);

	HRESULT result = d.input->SetCallback(ourCallbackDelegate);

	// Finally release ourCallbackDelegate, since we created a reference to it
	// during QueryInterface. The device will hold its own reference.
	ourCallbackDelegate->Release();
	UpdateUIForNewDevice();
	if (m_deviceMode != bmdStreamingDeviceUnknown)
    {
		UpdateUIForModeChanges();
	}
}

void BlackMagic::UpdateUIForNewDevice()
{
	// Add video input modes:
	IDeckLinkDisplayModeIterator* inputModeIterator;
	if (FAILED(m_streamingDeviceInput->GetVideoInputModeIterator(&inputModeIterator)))
	{
		MessageBox(NULL, _T("Failed to get input mode iterator"), _T("error"), 0);
		return;
	}

	BMDDisplayMode currentInputModeValue;
	if (FAILED(m_streamingDeviceInput->GetCurrentDetectedVideoInputMode(&currentInputModeValue)))
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

//			CString modeNameCString(modeName);
//			SysFreeString(modeName);
//			CString str;
//			str.Format(_T("Input Mode: % 26s"), modeNameCString);
//			m_encoding_static.SetWindowText(str);
			break;
		}
		inputMode->Release();
	}

	inputModeIterator->Release();
	UpdateEncodingPresetsUIForInputMode();
}

HRESULT STDMETHODCALLTYPE BlackMagic::StreamingDeviceModeChanged(IDeckLink* device, BMDStreamingDeviceMode mode)
{
	for(std::vector <dev>::iterator d = m_devs.begin(); d != m_devs.end(); ++d )
	{
		if(d->device == device)
        {
			d->mode = mode;
			break;
		}
	}

	if(device != m_streamingDevice)
    {
		return S_OK;
    }

	if(mode == m_deviceMode)
    {
    	return S_OK;
    }

	m_deviceMode = mode;

	UpdateUIForModeChanges();

	return S_OK;
}

void BlackMagic::StopPreview()
{
//	m_encoding_static.SetWindowText(_T(""));
	m_playing = false;
	m_recording = false;

	if (m_streamingDeviceInput)
    {
		m_streamingDeviceInput->StopCapture();
    }

	if(m_fh)
    {
//		m_record_button.SetWindowTextW(_T("Record"));
		CloseHandle(m_fh);
		m_fh = NULL;
	}
	if(m_pipe)
    {
		CloseHandle(m_pipe);
		m_pipe = NULL;
	}
}

void BlackMagic::UpdateUIForModeChanges()
{
//	CString status = _T(" (unknown)");
	string status;

	switch (m_deviceMode)
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
//	CString displayName = _T("Device: ") + m_deviceName + status;
//	m_configBoxStatic.SetWindowText(displayName);
//
//	bool enablePresets = !m_playing;/*m_deviceMode == bmdStreamingDeviceIdle && m_inputMode != bmdModeUnknown;*/
//	m_videoInputDeviceCombo.EnableWindow(enablePresets);
//	m_videoEncodingCombo.EnableWindow(enablePresets);
//	m_bitrate_slider.EnableWindow(enablePresets);
//	m_prevcfg_button.EnableWindow(enablePresets);
//	m_folder_button.EnableWindow(!m_recording);
//	m_record_button.EnableWindow(!enablePresets);
//	m_button_customize.EnableWindow(enablePresets);
//
//	bool enableStartStop = (m_deviceMode == bmdStreamingDeviceIdle || m_deviceMode == bmdStreamingDeviceEncoding);
//	m_startButton.EnableWindow(enableStartStop);
//
//	bool start = /*!m_playing;*/ m_deviceMode != bmdStreamingDeviceEncoding;
//	m_startButton.SetWindowText(start ? _T("Preview") : _T("Stop"));
	if (m_deviceMode == bmdStreamingDeviceEncoding)
	{
//		if (m_inputMode != bmdModeUnknown)
//			StartPreview();
	}
	else
		StopPreview();
}

HRESULT STDMETHODCALLTYPE BlackMagic::MPEG2TSPacketArrived(IBMDStreamingMPEG2TSPacket* mpeg2TSPacket)
{
	int len=mpeg2TSPacket->GetPayloadSize();
	int rec_error=0;
	void *buf;

	mpeg2TSPacket->GetBytes(&buf);
	DWORD dwBytesWritten;
	m_tscount.QuadPart+=len;
	if(m_playing)
    {
		if(!WriteFile(m_pipe, buf, len, &dwBytesWritten, NULL))
        {
			if(GetLastError() == ERROR_NO_DATA )
            {
				CloseHandle(m_pipe);
				m_pipe = CreateNamedPipe(_T("\\\\.\\pipe\\DeckLink.ts"), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_NOWAIT | PIPE_ACCEPT_REMOTE_CLIENTS, 100, 188*1000, 188*1000, 0, NULL);
			}
		}

		if(m_fh != NULL && !WriteFile(m_fh, buf, len, &dwBytesWritten, NULL))
        {
			rec_error = 1;
		}

		if((m_tscount.QuadPart-m_last_tscount.QuadPart)>(1024*10))
        {
			m_last_tscount.QuadPart = m_tscount.QuadPart;
			if(m_recording)
            {
				LARGE_INTEGER FileSize;
				GetFileSizeEx( m_fh, &FileSize);
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
	if (m_streamingDeviceInput->GetCurrentDetectedVideoInputMode(&m_inputMode) != S_OK)
    {
		MessageBox(NULL, _T("Failed to get current detected input mode"), _T("error"), 0);
    }
	else
	{
		UpdateEncodingPresetsUIForInputMode();
	}

	if (m_inputMode == bmdModeUnknown)
	{
	}

	UpdateUIForModeChanges();

	if((m_autorec || m_autopreview) && !m_recording)
    {
		StartPreview();
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

void BlackMagic::StartPreview()
{
	if (m_playing)
    {
		return;
    }
//	int i=m_videoEncodingCombo.GetCurSel();
	int iscustom=0;
	IBMDStreamingVideoEncodingMode* encodingMode = mEncodingModes["Native"];//(IBMDStreamingVideoEncodingMode*)m_videoEncodingCombo.GetItemDataPtr(i);
	int64_t rate = m_bitrate;
	IBMDStreamingMutableVideoEncodingMode *em;

//	CString str;
//	m_videoEncodingCombo.GetLBText(i, str);
//	if(!str.Compare(_T("Custom"))){
//		iscustom=1;
//		em=(IBMDStreamingMutableVideoEncodingMode*)m_videoEncodingCombo.GetItemDataPtr(i);
//	} else {
		encodingMode->CreateMutableVideoEncodingMode(&em);
//	}
	em->SetInt(bmdStreamingEncodingPropertyVideoBitRateKbps, rate);
	m_streamingDeviceInput->SetVideoEncodingMode(em);

	if(!iscustom)
    {
		em->Release();
	}

	m_streamingDeviceInput->GetVideoEncodingMode(&encodingMode);
	encodingMode->GetInt(bmdStreamingEncodingPropertyVideoBitRateKbps, &rate);
	encodingMode->Release();

	m_bitrate = (DWORD)rate;
	m_pipe = CreateNamedPipe(_T("\\\\.\\pipe\\DeckLink.ts"), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_NOWAIT | PIPE_ACCEPT_REMOTE_CLIENTS, 100, 188*1000, 188*1000, 0, NULL);

	m_playing = true;
	m_last_tscount.QuadPart = 0;
	m_tscount.QuadPart = 0;
	m_streamingDeviceInput->StartCapture();
	PROCESS_INFORMATION pi;
	STARTUPINFO si;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

	if(!m_autorec)
    {
		CreateProcessA( NULL, (char*) m_vlcexe.c_str(), NULL, NULL, false, 0, NULL, NULL,  &si, &pi);
	}
    else
    {
		OnBnClickedButtonRecord();
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

void BlackMagic::UpdateEncodingPresetsUIForInputMode()
{
	if (m_streamingDevice == NULL)
    {
		return;
    }
//
	BMDDisplayMode inputMode = BMDDisplayMode(m_inputMode);
//	EncodingPresetsRemoveItems();
	mEncodingModes.clear();
//
	IBMDStreamingVideoEncodingModePresetIterator* presetIterator;
//
	if (SUCCEEDED(m_streamingDeviceInput->GetVideoEncodingModePresetIterator(inputMode, &presetIterator)))
	{
		IBMDStreamingVideoEncodingMode* encodingMode = NULL;
		BSTR encodingModeName;

		while (presetIterator->Next(&encodingMode) == S_OK)
		{
			encodingMode->GetName(&encodingModeName);
            mEncodingModes[stdstr(encodingModeName)] = encodingMode;
//			CString encodingModeNameCString(encodingModeName);
//			SysFreeString(encodingModeName);
//
			// Add this item to the video input poup menu
//			int newIndex = m_videoEncodingCombo.AddString(encodingModeNameCString);
//			m_videoEncodingCombo.SetItemDataPtr(newIndex, encodingMode);
//
//			// We don't release the object here, as we hold the reference
//			// in the combo box.
		}
//
//		presetIterator->Release();
	}

//	m_videoEncodingCombo.SetCurSel(max(min(m_presetIndex, m_videoEncodingCombo.GetCount() - 1), 0));
}

void BlackMagic::OnBnClickedButtonRecord()
{
	if (m_streamingDevice == NULL)
    {
		return;
    }

	if(m_recording)
    {
		if(m_fh != NULL)
        {
			CloseHandle(m_fh);
			m_fh=NULL;
		}
		m_recording = false;
        Log(lInfo) << "Stopped recording";
	}
    else
    {

		if(m_filename.size())
        {
			if (m_timestampSuffix)
            {
            	string path = getFilePath(m_filename);
				string rootName = getFileNameNoExtension(m_filename);
				string fileName  = joinPath(path, rootName) + getFormattedDateTimeString("_%Y%m%d_%H%M%S") + ".ts";
				m_fh = CreateFileA(fileName.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			}
			else
			{
				m_fh = CreateFileA(m_filename.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			}

			if(m_fh != INVALID_HANDLE_VALUE)
            {
                Log(lInfo) << "Started recording";
				m_recording = true;
			}
		}
	}
}

