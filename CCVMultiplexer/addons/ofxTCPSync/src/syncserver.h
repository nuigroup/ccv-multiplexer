#pragma once

#include "ofMain.h"

#include "ofxNetwork.h"
#include "ofxThread.h"
#include "ofxXmlSettings.h"

typedef struct
{
	bool started;
	bool ready;
	//string name;
	int tcpServerIndex;
	bool data;
} Connection;


class syncserver : public ofxThread {

	public:

        syncserver();
		void  setup(string _fileString);
        void  start();
        void  quit();
		void shouldContinue();
		void setDefaults(){
            allconnected = false;
			//numConnectedClients = 0;
			currentFrame = 0;
			shouldTriggerFrame = false;
			running = false;
			newMessage = false;
			lastFrameTriggeredTime = 0;
			timeOfNextHeartbeat = ofGetElapsedTimef();
			heartBeatInterval = 2.0;
        }

		void threadedFunction();
        void loadIniFile(string _fileString);
		void restartServer();
        
        void  out(string _msg);
        void  print(string _msg);
        void  err(string _msg);

        void read(string response);
		void read(string response, int i);
        void send(string _msg);
		
	    int i;
		int serverInPort;
		int serverOutPort;
	    string broadcast;
		ofxTCPServer tcpServer;
	   	ofxUDPManager udpSender;
		ofxUDPManager udpReceiver;
		bool bTCP;
		float lastFrameTriggeredTime;
		bool allconnected;
		bool running;
		int fps;
		int numExpectedClients;
		int numConnectedClients;
		int currentFrame;
		bool shouldTriggerFrame;
		vector<Connection> connections;
		bool newMessage;
		string currentMessage;
		float timeOfNextHeartbeat;
		float heartBeatInterval;


};
       