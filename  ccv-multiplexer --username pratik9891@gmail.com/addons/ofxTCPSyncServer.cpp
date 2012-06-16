#include "ofxTCPSyncServer.h"
#include "ofxXmlSettings.h"

ofxTCPSyncServer::ofxTCPSyncServer()
{
	allconnected = false;
	framerate = 30;
	numExpectedClients = 0;
	numConnectedClients = 0;
	currentFrame = 0;
	shouldTriggerFrame = false;
	running = false;
	newMessage = false;

	timeOfNextHeartbeat = ofGetElapsedTimef();
	heartBeatInterval = 2.0;

}

ofxTCPSyncServer::~ofxTCPSyncServer()
{
	close();
}

void ofxTCPSyncServer::setup(string settingsFile)
{

	ofxXmlSettings settings;
	if(!settings.loadFile(settingsFile)){
		ofLog(OF_LOG_ERROR, "MPE Server -- Couldn't load settings file: " + settingsFile);
		return;
	}

	setup(settings.getValue("settings:framerate", 30, 0),
		  settings.getValue("settings:port", 9001, 0),
		  settings.getValue("settings:numclients", 2, 0));
}

void ofxTCPSyncServer::setup(int fps, int port, int numClients)
{

	close(); // in case of double set up

	//make sure vsync is OFF
	//make sure framerate is fast as it can go

	if(!server.setup(port, false)){
		ofLog(OF_LOG_ERROR, "MPE Serever :: Setup failed");
	}
	numExpectedClients = numClients;
	framerate = fps;

	for(int i = 0; i < numExpectedClients; i++){
		Connection c;
		c.started = false;
		c.ready = false;
		c.name = "noname";
		connections.push_back(c);
	}

	startThread(true, false);

	shouldTriggerFrame = false;
	allconnected = false;
	lastFrameTriggeredTime = 0;
	currentFrame = 0;

	//ofAddListener(ofEvents.update, this, &mpeServerTCP::update);

	cout << "Setting up server with FPS " << fps << " on port " << port << " with clients " << numClients << endl;
}


void ofxTCPSyncServer::update(ofEventArgs& args)
{
//	if(!server.isConnected()){
//		ofLog(OF_LOG_ERROR, "MPE Server :: Server Disconnected");
//	}
//    if(lock()){

//        if(shouldTriggerFrame){
//            float now = ofGetElapsedTimef();
//            float elapsed = (now - lastFrameTriggeredTime);
//    //		cout << "should trigger frame!" << endl;
//
//            if(elapsed >= 1.0/framerate){
//
//    //			cout << "triggered frame with framerate error of " << fabs( elapsed - 1.0/framerate)  << endl;
//
//                string message = "G,"+ofToString(currentFrame);
//                if (newMessage){
//                    message += currentMessage;
//                    newMessage = false;
//                    currentMessage = "";
//                }
//
//                //TODO append message
//                server.sendToAll(message);
//
//                for(int i = 0; i < connections.size(); i++){
//                    connections[i].ready = false;
//                }
//
//                shouldTriggerFrame = false;
//                lastFrameTriggeredTime = now;
//                currentFrame++;
//            }
//        }

//        unlock();
//    }
}


void ofxTCPSyncServer::reset()
{
	currentFrame = 0;
	shouldTriggerFrame = false;
	server.sendToAll("R");

}

void ofxTCPSyncServer::threadedFunction()
{
	while(isThreadRunning()){

		if(shouldTriggerFrame){
			float now = ofGetElapsedTimef();
			float elapsed = (now - lastFrameTriggeredTime);

			//cout << "should trigger frame!" << endl;

			if(elapsed >= 1.0/framerate){

				//cout << "triggered frame with framerate error of " << fabs( elapsed - 1.0/framerate)  << endl;

				string message = "G,"+ofToString(currentFrame);
				if (newMessage){
					message += currentMessage;
					newMessage = false;
					currentMessage = "";
				}

				server.sendToAll(message);

				for(int i = 0; i < connections.size(); i++){
					connections[i].ready = false;
				}

				shouldTriggerFrame = false;
				lastFrameTriggeredTime = now;
				currentFrame++;
			}
		}
		else {

			//check for dead clients
			bool lostConnection = false;
			for(int c = 0; c < numExpectedClients; c++){
				if(connections[c].started && !server.isClientConnected(connections[c].tcpServerIndex)){
					connections[c].started = false;
					lostConnection = true;
				}
			}

			if(allconnected && lostConnection){

				ofLog(OF_LOG_NOTICE, "MPE :: Client Disconnected -- RESET");

				//oops someone left
				printClientStatus();
				currentFrame = 0;
				shouldTriggerFrame = false;
				allconnected = false;

				server.sendToAll("R");
			}

			//cout << "All clients are connected! " << endl;

			for(int i = 0; i < server.getLastID(); i++){
				
				if(!server.isClientConnected(i)){
					continue;
				}
				
				string response = server.receive(i);

				if(response == ""){
					continue;
				}

	//			cout << "received a response " << response << endl;

				char first = response.at(0);
				if(first == 'L'){
					//Listener connected
					listeners.push_back(i);
				}
				else if(first == 'S'){
					//that's the start!
					int clientID = ofToInt(response.substr(1,1));
					if(clientID < numExpectedClients){
						vector<string> info = ofSplitString(response, ",");
						if(connections[clientID].started && currentFrame != 0){
                            //client already started, must have reset...
                            allconnected = false;
                            currentFrame = 0;
                            shouldTriggerFrame = false;
                            server.sendToAll("R");
						}

						connections[clientID].tcpServerIndex = i;
						connections[clientID].started = true;
						connections[clientID].name = info[1];
						cout << "Client ID " << clientID << " with response " << response << endl;
						//TODO: parse name
						printClientStatus();
					}
					else{
						ofLog(OF_LOG_ERROR, "Received Client ID " + ofToString(clientID)  + " out of range");
					}
				}
				else if(first == 'D'){

					if(!allconnected){
						continue;
					}

					vector<string> info = ofSplitString(response, ",");
					if(info.size() >= 3){
						int clientID = ofToInt(info[1]);
						int fc = ofToInt(info[2]);
						if(fc == currentFrame){
							//todo validate client id
							connections[clientID].ready = true;
							//cout << " client " << clientID << " is ready " << endl;
						}
						if(info.size() > 3){
							newMessage = true;
							for(int i = 3; i < info.size(); i++){
								currentMessage += ":" + info[i];
							}
							//cout << "NEW FORMAT :: MESSSAGE IS " << currentMessage << endl;
						}
					}
					else {
						ofLog(OF_LOG_ERROR, "MPE Server :: Response String " + response + " Invalid size");
					}
				}
			}

			if(!allconnected){
				allconnected = true;
				for(int c = 0; c < connections.size(); c++){
					if(!connections[c].started){
						allconnected = false;
						break;
					}
				}
				if(allconnected){
					shouldTriggerFrame = true;
					cout << "All clients connected!" << endl;
				}
			}
			//All connected and going
			else {
				bool allready = true;
				for(int c = 0; c < connections.size(); c++){
					if(!connections[c].ready){
						allready = false;
						break;
					}
				}
				if(allready){
					shouldTriggerFrame = true;
				}
			}
		}
        ofSleepMillis(5);
	}//end while
}

void ofxTCPSyncServer::printClientStatus()
{
    ofLog(OF_LOG_NOTICE, "MPE Client Status:");
    ofLog(OF_LOG_NOTICE, "  Expecting " + ofToString(numExpectedClients) + " Clients");
    for(int i = 0; i < connections.size(); i++){
        ofLog(OF_LOG_NOTICE, "  Client (" + ofToString(i) + ") " + connections[i].name + " connected? " + (connections[i].started ? "yes" : "no") );
    }
}

void ofxTCPSyncServer::close()
{
	if(!running) return;

	ofRemoveListener(ofEvents.update, this, &ofxTCPSyncServer::update);

	cout << " closing MPE Server " << endl;

	if(server.isConnected()){
		server.close();
	}

	connections.clear();

	stopThread();

	running = false;
}
