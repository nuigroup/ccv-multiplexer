#pragma once

#include "ofMain.h"
#include "ofxNetwork.h"
#include "ofxThread.h"

typedef struct
{
	bool started;
	bool ready;
	string name;
	int tcpServerIndex;
} Connection;

class ofxTCPSyncServer : public ofxThread
{
  public:
	ofxTCPSyncServer();
	~ofxTCPSyncServer();

	void setup(string setupFile);
	void setup(int framerate, int port, int numClients);
	void reset(); //sends a reset signal to all clients and reset frame count
	void close();
    void printClientStatus();

  protected:
	ofxTCPServer server;
	void update(ofEventArgs& args);
	void threadedFunction();

	float lastFrameTriggeredTime;
	bool allconnected;
	bool running;
	int framerate;
	int numExpectedClients;
	int numConnectedClients;
	int currentFrame;
	bool shouldTriggerFrame;
	vector<Connection> connections;
	vector<int> listeners;
	bool newMessage;
	string currentMessage;
	float timeOfNextHeartbeat;
	float heartBeatInterval;

};
