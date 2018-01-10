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
	    class BMDevice
        {
            public:
                IDeckLink*						mDevice;
                string 							mName;
                IBMDStreamingDeviceInput*		mInput;
                BMDStreamingDeviceMode 			mStreamingDeviceMode;
		};

    	map<string, IBMDStreamingVideoEncodingMode*>        mEncodingModes;
		DWORD										        mPresetIndex;
        std::vector <BMDevice> 		  	                    mDevs;
        HANDLE                                              mFh;
        HANDLE                                              mPipe;
        string                                              mFilename;
        string                                              mVlcexe;
        LARGE_INTEGER                                       mTscount;
        LARGE_INTEGER 	             	                    mLast_tscount;
        DWORD 							                    mBitrate;
        int 							                    mFailCount;
		BOOL						                        mAutorec;
		BOOL							                    mAutopreview;
		BOOL										        mTimestampSuffix;
        string							                    mDeviceName;

		IBMDStreamingDiscovery*			                    mStreamingDiscovery;
        IDeckLink*						                    mStreamingDevice;
        IBMDStreamingDeviceInput*		                    mStreamingDeviceInput;
        bool							                    mPlaying;
        bool							                    mRecording;
        BMDStreamingDeviceMode			                    mDeviceMode;
        BMDVideoConnection				                    mInputConnector;
        BMDDisplayMode					                    mInputMode;

		void 												onBnClickedButtonRecord();
        void							                    startPreview();
        void							                    stopPreview();
        void							                    updateUIForNewDevice();
        void							                    updateUIForNoDevice();
        void							                    updateUIForModeChanges();
        void							                    updateEncodingPresetsUIForInputMode();
        void 							                    activateDevice(int i);

        													// We need to correctly implement QueryInterface, but not the AddRef/Release
        virtual HRESULT STDMETHODCALLTYPE			        QueryInterface (REFIID iid, LPVOID* ppv);
        virtual ULONG STDMETHODCALLTYPE				        AddRef ()	{return 1;}
        virtual ULONG STDMETHODCALLTYPE				        Release ()	{return 1;}
		virtual HRESULT STDMETHODCALLTYPE 		  	        MPEG2TSPacketArrived(IBMDStreamingMPEG2TSPacket* mpeg2TSPacket);
};

#endif
