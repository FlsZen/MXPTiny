#ifndef BlackMagicH
#define BlackMagicH
#include "../DeckLinkAPI_h.h"
#include <vector>
#include <string>
#include <map>
#include "mtkConstants.h"
//---------------------------------------------------------------------------
using std::string;
using std::vector;
using std::map;
using mtk::gEmptyString;

class BMDevice;

class BlackMagic :	public IBMDStreamingDeviceNotificationCallback, public IBMDStreamingH264InputCallback
{
    public:
				    								        BlackMagic(bool previewOnVLC = false, bool useTimeStamp = false);
		virtual	    								        ~BlackMagic();
        void										        init();
        void										        disconnect();
        void												setBitRate(int bitRate);

        bool												isDeviceReady();
        void							                    startCapture();
        void							                    stopCapture();

		void 												startRecordingToFile(const string& f = gEmptyString);
		void 												stopRecordingToFile();
        string												getOutputFileName();

        													//Need to override the following pure virtuals
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
    	bool												mPreviewOnVLC;
		bool										        mTimeStampOutputFile;
    	map<string, IBMDStreamingVideoEncodingMode*>        mEncodingModes;
        std::vector <BMDevice> 		  	                    mDevices;
        HANDLE                                              mFileHandle;
        HANDLE                                              mPipe;
        string                                              mFileName;
        string                                              mVLCExecutable;
        LARGE_INTEGER                                       mTscount;
        LARGE_INTEGER 	             	                    mLast_tscount;
        DWORD 							                    mBitRate;

        IDeckLink*						                    mStreamingDevice;
        IBMDStreamingDeviceInput*		                    mStreamingDeviceInput;
        bool							                    mPlaying;
        BMDStreamingDeviceMode			                    mDeviceMode;
        BMDDisplayMode					                    mInputMode;

        void							                    reportDeviceModeChange();
        void							                    reportCurrentDisplayMode();
        void							                    captureAvailableEncodingModes();
        void 							                    activateDevice(int i);

        													// We need to correctly implement QueryInterface, but not the AddRef/Release
        virtual HRESULT STDMETHODCALLTYPE			        QueryInterface (REFIID iid, LPVOID* ppv);
        virtual ULONG STDMETHODCALLTYPE				        AddRef ()	{return 1;}
        virtual ULONG STDMETHODCALLTYPE				        Release ()	{return 1;}
		virtual HRESULT STDMETHODCALLTYPE 		  	        MPEG2TSPacketArrived(IBMDStreamingMPEG2TSPacket* mpeg2TSPacket);
};

class BMDevice
{
    public:
        IDeckLink*						mDevice;
        string 							mName;
        IBMDStreamingDeviceInput*		mInput;
        BMDStreamingDeviceMode 			mStreamingDeviceMode;
};



#endif
