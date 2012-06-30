#ifndef SYNCSERVER_H_INCLUDED
#define SYNCSERVER_H_INCLUDED

//#include "ofMain.h"

#include "ofxNetwork.h"
#include "ofxThread.h"
#include "ofxXmlSettings.h"
#include "ofxFBOTexture.h"
#include "ofxThread.h"



typedef struct
{
	bool started;
	bool ready;
	//string name;
	int serverIndex;
	bool calibrate;
	int	GRID_X;
	int	GRID_Y;
	
//	ofImage				testImage;
//	ofxCvColorImage		blobImage;
//	ofxCvGrayscaleImage blackImage;
//	ofxCvGrayscaleImage blobImageBw;
	int height;
	int width;
	int depth;
	std::vector<ofPoint> points;

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
			numConnectedClients=0;
			numExpectedClients = 1;
			
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
		int getDeviceCount(){return numConnectedClients;}

		void getCalibData();
		
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


		void getPixels(int id,unsigned char * pixels);

	//	ofxTuioClient tuioclient;
		//TUIOListener Interface
		//void addTuioObject(TuioObject * tobj);
		//void updateTuioObject(TuioObject * tobj);
		//void removeTuioObject(TuioObject * tobj);
	
		//void addTuioCursor(TuioCursor * tcur);
		//void updateTuioCursor(TuioCursor * tcur);
		//void removeTuioCursor(TuioCursor * tcur);

		//std::list< struct connection*> tuioSources;
		unsigned char * blackPixels;
		ofxFBOTexture fbo;
		
//private:
	//TuioTime currentTime; //namespace tuio


};
  
#endif