#pragma once

#include "ofMain.h"

#include "ofxNetwork.h"
#include "ofxThread.h"
#include "ofxXmlSettings.h"

//--------------------------------------------------------------
class ofxTCPSyncClientListener {
    public:
        virtual void frameEvent() = 0;
};

//--------------------------------------------------------------
class ofxTCPSyncClient : public ofxThread {

	public:
        ofxTCPSyncClient();
        void  setup(string _fileString, ofxTCPSyncClientListener* _parent, bool _autoMode = true);
    
        void  start();
        void  stop();
    
        int   getID()   { return id; }
    
        int   getFrameCount() { return frameCount; }
        float getFPS()        { return fps; }
        bool  isRendering()   { return rendering; }
    
        
        void  broadcast(string _msg);
    
        bool  messageAvailable() { return bMessageAvailable; }
        vector<string> getDataMessage() { return dataMessage; }
        bool  intsAvailable() { return bIntsAvailable; }
        vector<int> getInts() { return ints; }
        bool  bytesAvailable() { return bBytesAvailable; }
        vector<char> getBytes() { return bytes; }
    
        void  done();
        void  quit();
    
     
        
   
        void setDefaults() {
         
            
            id = 0;
                      
            rendering = false;
            autoMode  = false;
            
            frameCount = 0;
            fps        = 0.f;
            lastMs     = 0;
            
            allConnected = false;
            
        }
    
       
        void threadedFunction();
        
        void loadIniFile(string _fileString);
        
        void  out(string _msg);
        void  print(string _msg);
        void  err(string _msg);

        void run();
        void read(string _serverInput);
        void send(string _msg);
    
        ofxTCPSyncClientListener* parent;
    
        string       hostName;
        int          serverInPort;
		int			serverOutPort;
        ofxUDPManager udpSender;
		ofxUDPManager udpReceiver;
        
        
        int id;
       
               
        bool rendering;
        bool autoMode;
        
        int   frameCount;
        float fps;
        long  lastMs;
        
       
        bool allConnected;
        
        bool bMessageAvailable;
        bool bIntsAvailable;
        bool bBytesAvailable;
        vector<string> dataMessage;
        vector<int>    ints;
        vector<char>   bytes;
       
    
};