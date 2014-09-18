#pragma once

#include "ofConstants.h"
#include "ofTexture.h"
#include "ofGraphics.h"
#include "ofTypes.h"
#include "ofUtils.h"
#include "ofAppRunner.h"

#include <unistd.h>
#include <time.h>

#define _x86
#define _OSX

#include <PvApi.h>
#include <ImageLib.h>

// Note: this is optimized for monochrome cameras

class ofxVideoGrabberPvAPI {

	public:

		ofxVideoGrabberPvAPI();
		virtual ~ofxVideoGrabberPvAPI();

		void 			listDevices();
		bool 			isFrameNew();
		void			grabFrame();
		void			close();
		bool			initGrabber(int w, int h, bool bTexture = true);
		void			videoSettings();
		unsigned char 	* getPixels();
		ofTexture &		getTextureReference();
		void 			setVerbose(bool bTalkToMe);
		void			setDeviceID(int _deviceID);
		void 			setUseTexture(bool bUse);
		void 			draw(float x, float y, float w, float h);
		void 			draw(float x, float y);
		void			update();


		//the anchor is the point the image is drawn around. 
		//this can be useful if you want to rotate an image around a particular point. 
        void			setAnchorPercent(float xPct, float yPct);	//set the anchor as a percentage of the image width/height ( 0.0-1.0 range )
        void			setAnchorPoint(int x, int y);				//set the anchor point in pixels
        void			resetAnchor();								//resets the anchor to (0, 0)

		float 			getHeight();
		float 			getWidth();

		int			    height;
		int			    width;


	protected:
        
		bool					bChooseDevice;
		int						deviceID;
		bool					bUseTexture;
		ofTexture 				tex;
		bool 					bVerbose;
		bool 					bGrabberInited;
	    unsigned char * 		pixels;
		bool 					bIsFrameNew;
        
        // camera fields
        unsigned long   cameraUID;
        tPvHandle       cameraHandle;
        tPvFrame        cameraFrame;
        
        int             maxConcurrentCams;
        bool            bPvApiInitiated;
        bool            bWaitingForFrame;

        void  queueFrame();

};

