#pragma once

#include "ofMain.h"
#include "../Tracking/ContourFinder.h"
#include "ofxFiducial.h"
#include "ofxNetwork.h"
#include "ofxThread.h"
#include "ofxXmlSettings.h"
#include "ofxMultiplexerManager.h"

//--------------------------------------------------------------
class ofxTCPSyncClientListener {
    public:
        virtual void frameEvent() = 0;
};

//--------------------------------------------------------------
class ofxTCPSyncClient : public ofxThread, public ofxMultiplexerManager {

	public:
        ofxTCPSyncClient();
		~ofxTCPSyncClient();
        void  setup(string _fileString, ofxTCPSyncClientListener* _parent, bool _autoMode = true);
        void  start();
        void  stop();
		void create();
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
                             
            rendering = false;
            autoMode  = false;
            frameCount = 0;
            fps        = 0.f;
            lastMs     = 0;
            cont =false;
            allConnected = false;
            
        }
    
        void threadedFunction();
		void startT();
        void loadIniFile(string _fileString);
        
        void  out(string _msg);
        void  print(string _msg);
        void  err(string _msg);

        void run();
        void read(string _serverInput);
        void send(string _msg);
		void sendDisconnect();
		bool shouldContinue(){return cont;}
		bool cont;
    
        ofxTCPSyncClientListener* parent;
    
        string       hostName;
        int          serverInPort;
		int			serverOutPort;
        ofxUDPManager udpSender;
		ofxUDPManager udpReceiver;
		ofxTCPClient tcpClient;
		bool bTCP;
		string msg;
        
        /** The id is used for communication with the server, to let it know which 
         *  client is speaking and how to order the screens. */
        int id;
        /** The total number of screens. */
               
        bool rendering;
        bool autoMode;
        
        int   frameCount;
		//int   lastFrame;
        float fps;
        long  lastMs;
        
        /** True if all the other clients are connected. */
        bool allConnected;
        
        bool bMessageAvailable;
        bool bIntsAvailable;
        bool bBytesAvailable;
        vector<string> dataMessage;
        vector<int>    ints;
        vector<char>   bytes;

		bool bfinger;
		bool bobject;
		bool bfiducial;
		void setMode(bool fingers, bool objects, bool fiducials);
		void sendCoordinates(std::map<int, Blob> * fingerBlobs, std::map<int, Blob> * objectBlobs ,std::list <ofxFiducial> * fiducialsList);

		    
};