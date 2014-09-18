#include "ofxVideoGrabberPvAPI.h"


//--------------------------------------------------------------------
ofxVideoGrabberPvAPI::ofxVideoGrabberPvAPI(){
	bIsFrameNew				= false;
	bVerbose 				= false;
	bGrabberInited 			= false;
	bUseTexture				= true;
	bChooseDevice			= false;
	deviceID				= 0;
	width 					= 320;	// default setting
	height 					= 240;	// default setting
	pixels					= NULL;
    
    maxConcurrentCams       = 20;
    bPvApiInitiated         = false;
    bWaitingForFrame        = false;
}

//--------------------------------------------------------------------
ofxVideoGrabberPvAPI::~ofxVideoGrabberPvAPI(){
	close();
}


//--------------------------------------------------------------------
void ofxVideoGrabberPvAPI::listDevices(){
    
    
    // lazy initialization of the Prosilica API
    if( !bPvApiInitiated ){
        int ret = PvInitialize();
        if( ret == ePvErrSuccess ) {
            ofLog(OF_LOG_VERBOSE, "PvAPI initialized");
        } else {
            ofLog(OF_LOG_ERROR, "unable to initialize PvAPI");
            return;
        }
        bPvApiInitiated = true;
    }

    ofLog(OF_LOG_NOTICE, "-------------------------------------");
    
    // get a camera from the list
    tPvUint32 count, connected;
    tPvCameraInfo list[maxConcurrentCams];

    count = PvCameraList( list, maxConcurrentCams, &connected );
    if( connected > maxConcurrentCams ) {
        ofLog(OF_LOG_NOTICE, "more cameras connected than will be listed");
    }
    
    if(count >= 1) {
        ofLog(OF_LOG_NOTICE, "listing available capture devices");
        for( int i=0; i<count; ++i) {
            ofLog(OF_LOG_NOTICE, "got camera %s - %s - id: %i\n", list[i].DisplayName, list[i].SerialString, list[i].UniqueId);
        }
    } else {
        ofLog(OF_LOG_NOTICE, "no cameras found");
    }
    
    ofLog(OF_LOG_NOTICE, "-------------------------------------");
}

//--------------------------------------------------------------------
void ofxVideoGrabberPvAPI::setVerbose(bool bTalkToMe){
	bVerbose = bTalkToMe;

}


//--------------------------------------------------------------------
void ofxVideoGrabberPvAPI::setDeviceID(int _deviceID){
	deviceID		= _deviceID;
	bChooseDevice	= true;
}

//---------------------------------------------------------------------------
unsigned char * ofxVideoGrabberPvAPI::getPixels(){
	return pixels;
}

//------------------------------------
//for getting a reference to the texture
ofTexture & ofxVideoGrabberPvAPI::getTextureReference(){
	if(!tex.bAllocated() ){
		ofLog(OF_LOG_WARNING, "ofVideoGrabber - getTextureReference - texture is not allocated");
	}
	return tex;
}

//---------------------------------------------------------------------------
bool  ofxVideoGrabberPvAPI::isFrameNew(){
	return bIsFrameNew;
}

//--------------------------------------------------------------------
void ofxVideoGrabberPvAPI::update(){
	grabFrame();
}

//--------------------------------------------------------------------
void ofxVideoGrabberPvAPI::grabFrame(){
    int ret;
    
    if( bGrabberInited ){
        if( !bWaitingForFrame ) {
            bIsFrameNew = false;
            queueFrame();
        } else {
            // check if queued capture frame is ready
            ret = PvCaptureWaitForFrameDone(cameraHandle, &cameraFrame, 5);
            if( ret == ePvErrTimeout ) {
                bIsFrameNew = false;
            } else if( ret == ePvErrSuccess ){
                bIsFrameNew = true;
                if (bUseTexture){
                    tex.loadData((unsigned char*)cameraFrame.ImageBuffer, width, height, GL_LUMINANCE ); //GL_LUMINANCE8
                }
                memcpy(pixels, cameraFrame.ImageBuffer, width*height);
                queueFrame();   
            }
        }
    }
}

//--------------------------------------------------------------------
void ofxVideoGrabberPvAPI::queueFrame(){
    if( PvCaptureQueueFrame( cameraHandle, &cameraFrame, NULL) != ePvErrSuccess ){
        ofLog(OF_LOG_ERROR, "failed to queue frame buffer");
    } else {
        bWaitingForFrame = true;
    }  
}


//--------------------------------------------------------------------
void ofxVideoGrabberPvAPI::close(){

    if( bGrabberInited ) {
        // stop the streaming
        PvCommandRun(cameraHandle,"AcquisitionStop");
        PvCaptureEnd(cameraHandle);  
        
        // unsetup the camera
        PvCameraClose(cameraHandle);
        // and free the image buffer of the frame
        delete [] (char*)cameraFrame.ImageBuffer;  
    }

    if( bPvApiInitiated ) {
        // uninitialise the API
        PvUnInitialize();
    }

	if (pixels != NULL){
		delete[] pixels;
		pixels = NULL;
	}

	tex.clear();

}

//--------------------------------------------------------------------
void ofxVideoGrabberPvAPI::videoSettings(void){
    ofLog(OF_LOG_NOTICE, "videoSettings not implemented");
}



//--------------------------------------------------------------------
bool ofxVideoGrabberPvAPI::initGrabber(int w, int h, bool setUseTexture){

    width = w;
    height = h;
    tPvErr ret;   //PvAPI return codes
	bUseTexture = setUseTexture;
    memset( &cameraUID, 0, sizeof(cameraUID) );
    memset( &cameraHandle, 0, sizeof(cameraHandle) );
    memset( &cameraFrame, 0, sizeof(cameraFrame) );
    
    
    //---------------------------------- 1 - open the sequence grabber
    // lazy initialization of the Prosilica API
    if( !bPvApiInitiated ){
        ret = PvInitialize();
        if( ret == ePvErrSuccess ) {
            ofLog(OF_LOG_VERBOSE, "PvAPI initialized");
        } else {
            ofLog(OF_LOG_ERROR, "unable to initialize PvAPI");
            return false;
        }
        
        bPvApiInitiated = true;
    }
            
    //---------------------------------- 3 - buffer allocation
    // Create a buffer big enough to hold the video data,
    // make sure the pointer is 32-byte aligned.
    // also the rgb image that people will grab
    
    pixels = new unsigned char[width*height];
    
    // check for any cameras plugged in
    int waitIterations = 0;
    while( PvCameraCount() < 1 ) {
        ofSleepMillis(250);
        waitIterations++;
        
        if( waitIterations > 8 ) {
            ofLog(OF_LOG_ERROR, "error: no camera found");
            return false;        
        }
    }


    //---------------------------------- 4 - device selection
    
    tPvUint32 count, connected;
    tPvCameraInfo list[maxConcurrentCams];

    count = PvCameraList( list, maxConcurrentCams, &connected );
    if(count >= 1) {
        bool bSelectedDevicePresent = false;
        if(bChooseDevice) {
            //check if selected device is available
            for( int i=0; i<count; ++i) {
                if( deviceID == list[i].UniqueId ) {
                    bSelectedDevicePresent = true;
                    cameraUID = list[i].UniqueId;
                }
            }
        }
        
        if( !bSelectedDevicePresent ){
            cameraUID = list[0].UniqueId;
            ofLog(OF_LOG_NOTICE, "cannot find selected camera -> defaulting to first available");
            ofLog(OF_LOG_VERBOSE, "there is currently an arbitrary hard limit of %i concurrent cams", maxConcurrentCams);
        }        
    } else {
        ofLog(OF_LOG_ERROR, "no cameras available");
        return false;     
    }
        
    
    //---------------------------------- 5 - final initialization steps
    
    ret = PvCameraOpen( cameraUID, ePvAccessMaster, &cameraHandle );
    if( ret == ePvErrSuccess ){
        ofLog(OF_LOG_VERBOSE, "camera opened");
    } else {
        if( ret == ePvErrAccessDenied ) {
            ofLog(OF_LOG_ERROR, "camera access denied, probably already in use");
        }
        ofLog(OF_LOG_ERROR, "failed to open camera");
        return false;     
    }


    unsigned long FrameSize = 0;
    ret = PvAttrUint32Get( cameraHandle, "TotalBytesPerFrame", &FrameSize );
    if( ret == ePvErrSuccess ){
        // allocate the buffer for the single frame we need
        cameraFrame.ImageBuffer = new char[FrameSize];
        cameraFrame.ImageBufferSize = FrameSize;    
        ofLog(OF_LOG_VERBOSE, "camera asked for TotalBytesPerFrame");
    } else { 
        ofLog(OF_LOG_ERROR, "failed to allocate capture buffer");
        return false;    
    }    
    
    
    ret = PvCaptureStart(cameraHandle);
    if( ret == ePvErrSuccess ){
        ofLog(OF_LOG_VERBOSE, "camera set to capture mode");
    } else { 
        if( ret == ePvErrUnplugged ){
            ofLog(OF_LOG_ERROR, "cannot start capture, camera was unplugged");
        }
        ofLog(OF_LOG_ERROR, "cannot set to capture mode");
        return false;    
    }
    
    
    ret = PvAttrEnumSet(cameraHandle,"FrameStartTriggerMode","Freerun");
    if( ret == ePvErrSuccess ){
        ofLog(OF_LOG_VERBOSE, "camera set to continuous mode");
    } else {
        ofLog(OF_LOG_ERROR, "cannot set to continous mode");
        return false;    
    }    
    
    
    ret = PvCommandRun(cameraHandle,"AcquisitionStart");
    if( ret == ePvErrSuccess ){
        ofLog(OF_LOG_VERBOSE, "camera continuous acquisition started");
    } else {
        // if that fail, we reset the camera to non capture mode
        PvCaptureEnd(cameraHandle) ;
        ofLog(OF_LOG_ERROR, "cannot start continuous acquisition");
        return false;    
    }
    
    bGrabberInited = true;
    //loadSettings();    
    ofLog(OF_LOG_NOTICE,"camera is ready now");  
                
                
    //---------------------------------- 6 - setup texture if needed

    if (bUseTexture){
        // create the texture, set the pixels to black and
        // upload them to the texture (so at least we see nothing black the callback)
        tex.allocate(width,height,GL_LUMINANCE);
        memset(pixels, 0, width*height);
        tex.loadData(pixels, width, height, GL_LUMINANCE);
    }

    // we are done
    return true;
}

//------------------------------------
void ofxVideoGrabberPvAPI::setUseTexture(bool bUse){
	bUseTexture = bUse;
}

//we could cap these values - but it might be more useful
//to be able to set anchor points outside the image

//----------------------------------------------------------
void ofxVideoGrabberPvAPI::setAnchorPercent(float xPct, float yPct){
    if (bUseTexture)tex.setAnchorPercent(xPct, yPct);
}

//----------------------------------------------------------
void ofxVideoGrabberPvAPI::setAnchorPoint(int x, int y){
    if (bUseTexture)tex.setAnchorPoint(x, y);
}

//----------------------------------------------------------
void ofxVideoGrabberPvAPI::resetAnchor(){
   	if (bUseTexture)tex.resetAnchor();
}

//------------------------------------
void ofxVideoGrabberPvAPI::draw(float _x, float _y, float _w, float _h){
	if(bUseTexture) {
		tex.draw(_x, _y, _w, _h);
	}
}

//------------------------------------
void ofxVideoGrabberPvAPI::draw(float _x, float _y){
	draw(_x, _y, (float)width, (float)height);
}


//----------------------------------------------------------
float ofxVideoGrabberPvAPI::getHeight(){
	return (float)height;
}

//----------------------------------------------------------
float ofxVideoGrabberPvAPI::getWidth(){
	return (float)width;
}
