#pragma hdrstop
#pragma argsused


#include <stdio.h>
#include <iostream>
#include "../DeckLinkAPI_h.h"
#include <vector>
using namespace std;
//using namespace mtk;

class BMCallback :	public IBMDStreamingDeviceNotificationCallback, public IBMDStreamingH264InputCallback
{
    public:
	    class dev {
		public:
			IDeckLink *device;
			string name;
			IBMDStreamingDeviceInput *input;
			BMDStreamingDeviceMode mode;
		};

        std::vector <dev> m_devs;
        HANDLE m_fh;
        HANDLE m_pipe;
        string m_filename;
        string m_vlcexe;
        LARGE_INTEGER m_tscount;
        LARGE_INTEGER m_last_tscount;
        DWORD m_bitrate;
        int m_failCount;
		BOOL						    m_autorec;
		BOOL							m_autopreview;

        IDeckLink*						m_streamingDevice;
        IBMDStreamingDeviceInput*		m_streamingDeviceInput;
        bool							m_playing;
        bool							m_recording;
        BMDStreamingDeviceMode			m_deviceMode;
        BMDVideoConnection				m_inputConnector;
        BMDDisplayMode					m_inputMode;

        void							StartPreview();
        void							StopPreview();
        void							UpdateUIForNewDevice();
        void							UpdateUIForNoDevice();
        void							UpdateUIForModeChanges();
        void							UpdateEncodingPresetsUIForInputMode();
        void							EncodingPresetsRemoveItems();
        void updBitrate();
        void activate_device(int i);



            // IUknown
        // We need to correctly implement QueryInterface, but not the AddRef/Release
        virtual HRESULT STDMETHODCALLTYPE	QueryInterface (REFIID iid, LPVOID* ppv);

		//HRESULT BMCallback::QueryInterface(REFIID iid, LPVOID* ppv)

        virtual ULONG STDMETHODCALLTYPE		AddRef ()	{return 1;}
        virtual ULONG STDMETHODCALLTYPE		Release ()	{return 1;}



		virtual HRESULT STDMETHODCALLTYPE MPEG2TSPacketArrived(IBMDStreamingMPEG2TSPacket* mpeg2TSPacket);
        // IBMDStreamingDeviceNotificationCallback
        virtual HRESULT STDMETHODCALLTYPE StreamingDeviceArrived(IDeckLink* device);
        virtual HRESULT STDMETHODCALLTYPE StreamingDeviceRemoved(IDeckLink* device);
        virtual HRESULT STDMETHODCALLTYPE StreamingDeviceModeChanged(IDeckLink* device, BMDStreamingDeviceMode mode);
        virtual HRESULT STDMETHODCALLTYPE StreamingDeviceFirmwareUpdateProgress(IDeckLink* device, unsigned char percent);


        virtual HRESULT STDMETHODCALLTYPE H264NALPacketArrived(IBMDStreamingH264NALPacket* nalPacket);
        virtual HRESULT STDMETHODCALLTYPE H264AudioPacketArrived(IBMDStreamingAudioPacket* audioPacket);
        virtual HRESULT STDMETHODCALLTYPE H264VideoInputConnectorScanningChanged(void);
        virtual HRESULT STDMETHODCALLTYPE H264VideoInputConnectorChanged(void);
        virtual HRESULT STDMETHODCALLTYPE H264VideoInputModeChanged(void);
};


int main(int argc, char* argv[])
{
	BMCallback bc;

	IBMDStreamingDiscovery*			m_streamingDiscovery = NULL;
	// Initialise Blackmagic Streaming API
	HRESULT						result = -1;

     //Initialize the OLE libraries
	CoInitialize(NULL);

	result = CoCreateInstance(CLSID_CBMDStreamingDiscovery, NULL, CLSCTX_ALL, IID_IBMDStreamingDiscovery, (void**)&m_streamingDiscovery);
	if (FAILED(result))
	{
		MessageBox(NULL, _T("This application requires the Blackmagic Streaming drivers installed.\nPlease install the Blackmagic Streaming drivers to use the features of this application."), _T("Error"), 0);
		goto bail;
	}

	// Note: at this point you may get device notification messages!
	result = m_streamingDiscovery->InstallDeviceNotifications(&bc);
	if (FAILED(result))
	{
		MessageBox(NULL, _T("Failed to install device notifications for the Blackmagic Streaming devices"), _T("Error"), 0);
		goto bail;
	}

	// Create background thread to service the automatic recording while a host is alive option
	//AfxBeginThread(MonitorHostThreadProc, this);

	char c;
	while (true)
    {
    	cin >> c;
        if(c == 'q')
        	break;
    }


bail:
	cout << "Crap..";

	return 0;
}

HRESULT STDMETHODCALLTYPE BMCallback::QueryInterface(REFIID iid, LPVOID* ppv)
{
	HRESULT result = E_NOINTERFACE;

	if (ppv == NULL)
		return E_POINTER;
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

HRESULT STDMETHODCALLTYPE BMCallback::StreamingDeviceArrived(IDeckLink* device)
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
		return S_OK;

//	string modelNameCString(stdstr(modelName));
//	SysFreeString(modelName);
//
//	d.name = modelNameCString;
//	d.mode = bmdStreamingDeviceUnknown;
//
//	m_devs.push_back(d);
//
//	int newIndex = m_videoInputDeviceCombo.AddString(d.name);
//	m_videoInputDeviceCombo.SetItemDataPtr(newIndex, &d);
//
//	// Check we don't already have a device.
//	if (m_streamingDevice != NULL)
//	{
//		return S_OK;
//	}

//	activate_device(newIndex);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE BMCallback::StreamingDeviceRemoved(IDeckLink* device)
{
	int shutdownactive=0;
	// We only care about removal of the device we are using
	if (device == m_streamingDevice) {
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

HRESULT STDMETHODCALLTYPE BMCallback::StreamingDeviceModeChanged(IDeckLink* device, BMDStreamingDeviceMode mode)
{
	for (std::vector <dev>::iterator d = m_devs.begin(); d != m_devs.end(); ++d )
	{
		if(d->device == device) {
			d->mode=mode;
			break;
		}
	}
	if(device != m_streamingDevice)
		return S_OK;

	if (mode == m_deviceMode)
		return S_OK;

	m_deviceMode = mode;

	UpdateUIForModeChanges();

	return S_OK;
}

void BMCallback::StopPreview()
{
//	m_encoding_static.SetWindowText(_T(""));
	m_playing = false;
	m_recording = false;

	if (m_streamingDeviceInput)
		m_streamingDeviceInput->StopCapture();
	if(m_fh) {
//		m_record_button.SetWindowTextW(_T("Record"));
		CloseHandle(m_fh);
		m_fh=NULL;
	}
	if(m_pipe) {
		CloseHandle(m_pipe);
		m_pipe=NULL;
	}
}

void BMCallback::UpdateUIForModeChanges()
{
//	CString status = _T(" (unknown)");
//
//	switch (m_deviceMode)
//	{
//		case bmdStreamingDeviceIdle:
//			status = _T(" (idle)");
//			break;
//		case bmdStreamingDeviceEncoding:
//			status = _T(" (encoding)");
//			break;
//		case bmdStreamingDeviceStopping:
//			status = _T(" (stopping)");
//			break;
//	}
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
//	if (m_deviceMode == bmdStreamingDeviceEncoding)
//	{
////		if (m_inputMode != bmdModeUnknown)
////			StartPreview();
//	}
//	else
//		StopPreview();
}

HRESULT STDMETHODCALLTYPE BMCallback::MPEG2TSPacketArrived(IBMDStreamingMPEG2TSPacket* mpeg2TSPacket)
{
	int len=mpeg2TSPacket->GetPayloadSize();
	int rec_error=0;
	void *buf;

	mpeg2TSPacket->GetBytes(&buf);
	DWORD dwBytesWritten;
	m_tscount.QuadPart+=len;
	if(m_playing) {
		if(!WriteFile(m_pipe, buf, len, &dwBytesWritten, NULL)) {
			if(GetLastError() == ERROR_NO_DATA ) {
				CloseHandle(m_pipe);
				m_pipe=CreateNamedPipe(_T("\\\\.\\pipe\\DeckLink.ts"), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_NOWAIT | PIPE_ACCEPT_REMOTE_CLIENTS, 100, 188*1000, 188*1000, 0, NULL);
			}
		}
		if(m_fh != NULL && !WriteFile(m_fh, buf, len, &dwBytesWritten, NULL)) {
			rec_error=1;
		}
		if((m_tscount.QuadPart-m_last_tscount.QuadPart)>(1024*10)) {
			//CString str;
			m_last_tscount.QuadPart=m_tscount.QuadPart;
			//str.Format(_T("Receiving (kB): % 26llu"), m_tscount.QuadPart>>10);
			if(m_recording) {
				LARGE_INTEGER FileSize;
				GetFileSizeEx( m_fh, &FileSize);
//				str.Format(_T("%s    -    Recording (kB): % 6llu %s"), str, FileSize.QuadPart>>10, rec_error
//					 ?
//					_T("- ERROR WRITING !!!")
//					:
//					_T(""));
			}
//			m_encoding_static.SetWindowText(str);
		}
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE BMCallback::H264VideoInputConnectorScanningChanged(void)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE BMCallback::H264VideoInputConnectorChanged(void)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE BMCallback::H264VideoInputModeChanged(void)
{
	if (m_streamingDeviceInput->GetCurrentDetectedVideoInputMode(&m_inputMode) != S_OK)
		MessageBox(NULL, _T("Failed to get current detected input mode"), _T("error"), 0);
	else
	{
		UpdateEncodingPresetsUIForInputMode();
	}

	if (m_inputMode == bmdModeUnknown)
	{
	}

	UpdateUIForModeChanges();

	if((m_autorec || m_autopreview) && !m_recording) {
		StartPreview();
	}

	return S_OK;
}


HRESULT STDMETHODCALLTYPE BMCallback::StreamingDeviceFirmwareUpdateProgress(IDeckLink* device, unsigned char percent)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE BMCallback::H264NALPacketArrived(IBMDStreamingH264NALPacket* nalPacket)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE BMCallback::H264AudioPacketArrived(IBMDStreamingAudioPacket* audioPacket)
{
	return S_OK;
}

void BMCallback::StartPreview()
{
	if (m_playing)
		return;
//	int i=m_videoEncodingCombo.GetCurSel();
	int iscustom=0;
//	IBMDStreamingVideoEncodingMode* encodingMode = (IBMDStreamingVideoEncodingMode*)m_videoEncodingCombo.GetItemDataPtr(i);
//	int64_t rate=m_bitrate_slider.GetPos();
	IBMDStreamingMutableVideoEncodingMode *em;

//	CString str;
//	m_videoEncodingCombo.GetLBText(i, str);
//	if(!str.Compare(_T("Custom"))){
//		iscustom=1;
//		em=(IBMDStreamingMutableVideoEncodingMode*)m_videoEncodingCombo.GetItemDataPtr(i);
//	} else {
//		encodingMode->CreateMutableVideoEncodingMode(&em);
//	}
//	em->SetInt(bmdStreamingEncodingPropertyVideoBitRateKbps, rate);
//
//	m_streamingDeviceInput->SetVideoEncodingMode(em);
//	if(!iscustom) {
//		em->Release();
//	}
//
//	m_streamingDeviceInput->GetVideoEncodingMode(&encodingMode);
//	encodingMode->GetInt(bmdStreamingEncodingPropertyVideoBitRateKbps, &rate);
//	encodingMode->Release();
//
//	m_bitrate_slider.SetPos((int)rate);
//	updBitrate();
//	m_bitrate=(DWORD)rate;
//	SetKeyData(HKEY_CURRENT_USER, _T("Software\\BayCom\\MXPTiny\\Settings"), REG_DWORD, _T("bitrate"), (BYTE *)&m_bitrate, sizeof(m_bitrate));
//	m_filename.ReleaseBuffer();
//
//	m_pipe=CreateNamedPipe(_T("\\\\.\\pipe\\DeckLink.ts"), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_NOWAIT | PIPE_ACCEPT_REMOTE_CLIENTS, 100, 188*1000, 188*1000, 0, NULL);
//
//	m_playing = true;
//	m_last_tscount.QuadPart = 0;
//	m_tscount.QuadPart = 0;
//	m_streamingDeviceInput->StartCapture();
//	PROCESS_INFORMATION pi;
//	STARTUPINFO si;
//
//    ZeroMemory( &si, sizeof(si) );
//    si.cb = sizeof(si);
//    ZeroMemory( &pi, sizeof(pi) );
//
//	if(!m_autorec) {
//		CreateProcess( NULL, m_vlcexe.GetBuffer(MAX_PATH), NULL, NULL, false, 0, NULL, NULL,  &si, &pi);
//		m_vlcexe.ReleaseBuffer();
//	} else {
//		if(m_recording)
//			OnBnClickedButtonRecord();
//		OnBnClickedButtonRecord();
//	}
//
//	CloseHandle(pi.hProcess);
//	CloseHandle(pi.hThread);

}

void BMCallback::UpdateEncodingPresetsUIForInputMode()
{
//	if (m_streamingDevice == NULL)
//		return;
//
//	BMDDisplayMode inputMode = BMDDisplayMode(m_inputMode);
//	EncodingPresetsRemoveItems();
//
//	IBMDStreamingVideoEncodingModePresetIterator* presetIterator;
//
//	if (SUCCEEDED(m_streamingDeviceInput->GetVideoEncodingModePresetIterator(inputMode, &presetIterator)))
//	{
//		IBMDStreamingVideoEncodingMode* encodingMode = NULL;
//		BSTR encodingModeName;
//
//		while (presetIterator->Next(&encodingMode) == S_OK)
//		{
//			encodingMode->GetName(&encodingModeName);
//			CString encodingModeNameCString(encodingModeName);
//			SysFreeString(encodingModeName);
//
//			// Add this item to the video input poup menu
//			int newIndex = m_videoEncodingCombo.AddString(encodingModeNameCString);
//			m_videoEncodingCombo.SetItemDataPtr(newIndex, encodingMode);
//
//			// We don't release the object here, as we hold the reference
//			// in the combo box.
//		}
//
//		presetIterator->Release();
//	}
//
//	m_videoEncodingCombo.SetCurSel(max(min(m_presetIndex, m_videoEncodingCombo.GetCount() - 1), 0));
}

