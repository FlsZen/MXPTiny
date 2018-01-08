#ifndef BlackMagicH
#define BlackMagicH
#include "../DeckLinkAPI_h.h"
#include <vector>
#include <string>
#include <map>
//---------------------------------------------------------------------------
using std::string;
using std::vector;
using std::map;

class BlackMagic :	public IBMDStreamingDeviceNotificationCallback, public IBMDStreamingH264InputCallback
{
    public:
				    								        BlackMagic();
        void										        init();
        // IUknown
        // We need to correctly implement QueryInterface, but not the AddRef/Release
        virtual HRESULT STDMETHODCALLTYPE			        QueryInterface (REFIID iid, LPVOID* ppv);
        virtual ULONG STDMETHODCALLTYPE				        AddRef ()	{return 1;}
        virtual ULONG STDMETHODCALLTYPE				        Release ()	{return 1;}
		virtual HRESULT STDMETHODCALLTYPE 		  	        MPEG2TSPacketArrived(IBMDStreamingMPEG2TSPacket* mpeg2TSPacket);

        // IBMDStreamingDeviceNotificationCallback
        virtual HRESULT STDMETHODCALLTYPE         	        StreamingDeviceArrived(IDeckLink* device);
        virtual HRESULT STDMETHODCALLTYPE         	        StreamingDeviceRemoved(IDeckLink* device);
        virtual HRESULT STDMETHODCALLTYPE         	        StreamingDeviceModeChanged(IDeckLink* device, BMDStreamingDeviceMode mode);
        virtual HRESULT STDMETHODCALLTYPE         	        StreamingDeviceFirmwareUpdateProgress(IDeckLink* device, unsigned char percent);
        virtual HRESULT STDMETHODCALLTYPE         	        H264NALPacketArrived(IBMDStreamingH264NALPacket* nalPacket);
        virtual HRESULT STDMETHODCALLTYPE         	        H264AudioPacketArrived(IBMDStreamingAudioPacket* audioPacket);
        virtual HRESULT STDMETHODCALLTYPE         	        H264VideoInputConnectorScanningChanged(void);
        virtual HRESULT STDMETHODCALLTYPE         	        H264VideoInputConnectorChanged(void);
        virtual HRESULT STDMETHODCALLTYPE         	        H264VideoInputModeChanged(void);

	protected:
	    class dev
        {
            public:
                IDeckLink						*device;
                string 							name;
                IBMDStreamingDeviceInput		*input;
                BMDStreamingDeviceMode 			mode;
		};

    	map<string, IBMDStreamingVideoEncodingMode*>        mEncodingModes;
		DWORD										        m_presetIndex;
        std::vector <dev> 				                    m_devs;
        HANDLE                                              m_fh;
        HANDLE                                              m_pipe;
        string                                              m_filename;
        string                                              m_vlcexe;
        LARGE_INTEGER                                       m_tscount;
        LARGE_INTEGER 	             	                    m_last_tscount;
        DWORD 							                    m_bitrate;
        int 							                    m_failCount;
		BOOL						                        m_autorec;
		BOOL							                    m_autopreview;
		BOOL										        m_timestampSuffix;
        string							                    m_deviceName;

		IBMDStreamingDiscovery*			                    m_streamingDiscovery;
        IDeckLink*						                    m_streamingDevice;
        IBMDStreamingDeviceInput*		                    m_streamingDeviceInput;
        bool							                    m_playing;
        bool							                    m_recording;
        BMDStreamingDeviceMode			                    m_deviceMode;
        BMDVideoConnection				                    m_inputConnector;
        BMDDisplayMode					                    m_inputMode;

		void 												OnBnClickedButtonRecord();
        void							                    StartPreview();
        void							                    StopPreview();
        void							                    UpdateUIForNewDevice();
        void							                    UpdateUIForNoDevice();
        void							                    UpdateUIForModeChanges();
        void							                    UpdateEncodingPresetsUIForInputMode();
        void							                    EncodingPresetsRemoveItems();
        void 							                    updBitrate();
        void 							                    activate_device(int i);

};

#endif
