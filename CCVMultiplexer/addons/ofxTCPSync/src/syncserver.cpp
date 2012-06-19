#include "syncserver.h"

syncserver::syncserver() {
    setDefaults(); 
}

void syncserver::setup(string _fileString) {
    loadIniFile(_fileString);
	if(bTCP)
	{}
	else
	{
		udpReceiver.Create();
		udpSender.Create();
		udpReceiver.SetReuseAddress(true);
		udpSender.SetReuseAddress(true);
	}
	for(int i = 0; i < numExpectedClients; i++){
		Connection c;
		c.started = false;
		c.ready = false;
		//c.name = "noname";
		connections.push_back(c);
	}
  }

void syncserver::loadIniFile(string _fileString) {
	out("Loading settings from file " + _fileString);
    
	ofxXmlSettings xmlReader;
    if (!xmlReader.loadFile(_fileString)) 
        err("ERROR loading XML file!");
	else
		out("Xml Loaded successfully");

    // parse INI file
   // fps   = xmlReader.getValue("settings:server:framerate", 5, 0);
    fps=xmlReader.getValue("settings:fps", 5, 0);
	serverInPort = xmlReader.getValue("settings:serverinport", 11999, 0);
	serverOutPort = xmlReader.getValue("settings:serveroutport",11998,0);
	numExpectedClients        = xmlReader.getValue("settings:numclients", 1, 0);
	broadcast = xmlReader.getValue("settings:broadcast", "192.168.1.255", 0);
	bTCP = xmlReader.getValue("settings:protocol","UDP") == "TCP";

	//if (xmlReader.getValue("settings:debug", 0, 0) == 1) 
//        DEBUG = true;

    out("XML Settings: fps = " + ofToString(fps) + ", Number of Client = " + ofToString(numExpectedClients));
	
}

void syncserver::out(string _str) {
    print(_str);
}

void syncserver::print(string _str) {
//    if (DEBUG)
        cout << "syncServer: " << _str << endl;
}

void syncserver::err(string _str) {
    cerr << "ofxTCPSyncClient: " << _str << endl;
}

void syncserver::start() {
//    tcpClient.setVerbose(DEBUG);

	if(bTCP)
	{
		if(!tcpServer.setup(serverInPort, false)){
			err("TCP Serever :: Setup failed");
			return;
		}
		else
			out("TCP Server setup on::" +ofToString(serverInPort));
	}
	else
	{
	udpSender.SetEnableBroadcast(true);
	udpSender.SetReuseAddress(true);
	udpReceiver.SetReuseAddress(true);
		if(!udpReceiver.Bind(serverInPort)){
			err("UDP In Server :: Setup failed");
			return;
		}
		else if(!udpSender.Connect("192.168.1.255",serverOutPort)){
			err("UDP Out Server :: Setup Failed");
			return;
		}
		else{
			out("UDP Input Server setup on::" +ofToString(serverInPort));
			out("UDP Output Server setup on::" +ofToString(serverOutPort));
		}
	}
  startThread(true,false);  // blocking, verbose
}

void syncserver::threadedFunction() {
    out("Running!");
        
    while (isThreadRunning()) {
		if (lock()) {
			//shouldTriggerFrame=true;
			if(shouldTriggerFrame){
				float now = ofGetElapsedTimef();
				float elapsed = (now - lastFrameTriggeredTime);

				if(elapsed >= 1.0/fps){
					
					string message = "G,"+ofToString(currentFrame);
					if (newMessage){
						message += currentMessage;
						newMessage = false;
						currentMessage = "";
					}

					send(message);
					shouldTriggerFrame = false;
					lastFrameTriggeredTime = now;
					currentFrame++;
				}
			}

			else {
				//lock();

			/*	bool lostConnection = false;
				for(int c = 0; c < numExpectedClients; c++){
					if(connections[c].started && !server.isClientConnected(connections[c].tcpServerIndex)){
						connections[c].started = false;
						lostConnection = true;
					}
				}*/


				if(bTCP){
					for(int i = 0; i < tcpServer.getLastID(); i++){
						if(tcpServer.isClientConnected(i)){
						string response = tcpServer.receive(i);
						if (response.length() > 0) {
							read(response,i);
						}
						}	
					}
				}
				
				else{
					char udpMessage[10];
					udpReceiver.Receive(udpMessage,10);
					string response = udpMessage;
					//out(response);
					if (response.length() > 0) {
						read(response);
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
					
			
				//unlock();
				//ofSleepMillis(5);
			}
			
		
			unlock();
			ofSleepMillis(5);
		}
	}
}

void syncserver::read(string response) {
	out("Receiving: " + response);

	char first = response.at(0);
	if(first == 'S'){
					//this is where it starts!
			int clientID = ofToInt(response.substr(1,1));
			if(clientID < numExpectedClients){
					vector<string> info = ofSplitString(response, ",");
					//connections[clientID].tcpServerIndex = i;
					connections[clientID].started = true;
//					connections[clientID].name = info[1];
					cout << "Client ID " << clientID << " with response " << response << endl;
					//currentFrame=0;
				}
			else{
					err("Received Client ID " + ofToString(clientID)  + " out of range");
				}
	}
	else if(first == 'D'){
			vector<string> info = ofSplitString(response, ",");
			if(info.size() >= 3){
				int clientID = ofToInt(info[1]);
				int fc = ofToInt(info[2]);
				if(fc == currentFrame){
					//todo validate client id
					connections[clientID].ready = true;
					//cout << " client " << clientID << " is ready " << endl
				}
				
			}
	}
	else if(first == 'X'){
		int clientID = ofToInt(response.substr(1,1));
		connections[clientID].started = false;
		connections[clientID].ready = false;
		setDefaults();
		
		//currentFrame=0;
		//shouldTriggerFrame=false;
		//ofSleepMillis(5);
		
	}
	
	
}

void syncserver::read(string response,int i) {
	out("Receiving: " + response);

	char first = response.at(0);
	if(first == 'S'){
					//that's the start!
			int clientID = ofToInt(response.substr(1,1));
			if(clientID < numExpectedClients){
					vector<string> info = ofSplitString(response, ",");
					connections[clientID].tcpServerIndex = i;
					connections[clientID].started = true;
//					connections[clientID].name = info[1];
					cout << "Client ID " << clientID << " with response " << response << endl;
				}
			else{
					err("Received Client ID " + ofToString(clientID)  + " out of range");
				}
	}
	else if(first == 'D'){
			vector<string> info = ofSplitString(response, ",");
			if(info.size() >= 3){
				int clientID = ofToInt(info[1]);
				int fc = ofToInt(info[2]);
				if(fc == currentFrame){
					//todo validate client id
					connections[clientID].ready = true;
					//cout << " client " << clientID << " is ready " << endl;
				}
				
			}
	}
	else if(first == 'X'){
		
		ofSleepMillis(150);
		send("X");
		ofSleepMillis(50);
		int clientID = ofToInt(response.substr(1,1));
		out("Diconnect client"+ofToString(i));
		tcpServer.disconnectClient(clientID);
		connections[clientID].started = false;
		connections[clientID].ready = false;
		setDefaults();
		restartServer();
	}
}

void syncserver::send(string _msg) {
    out("Sending: " + _msg);
   // _msg += "\n";

	if(bTCP)
	{
		if(allconnected)
		tcpServer.sendToAll(_msg);
	}
	else
	udpSender.Send(_msg.c_str(),_msg.length());
}

void syncserver::quit() {
    out("Quitting.");
	if(bTCP){
		tcpServer.close();
	}
	else{
		udpSender.Close();
		udpReceiver.Close();
	}
	connections.clear();
    stopThread();
}

void syncserver::restartServer(){
	out("Restarting TCP Server");
	tcpServer.close();
	stopThread();
	ofSleepMillis(500);
	start();
	out("TCP Server Restart Complete!!!");
}







