#include "ofxTCPSyncClient.h"

//--------------------------------------------------------------
ofxTCPSyncClient::ofxTCPSyncClient() {
    setDefaults(); 
}

//--------------------------------------------------------------
void ofxTCPSyncClient::setup(string _fileString, ofxTCPSyncClientListener* _parent, bool _autoMode) {
   parent   = _parent;
//void ofxTCPSyncClient::setup(string _fileString, bool _autoMode) {
    autoMode = _autoMode;
    
    loadIniFile(_fileString);
	
}

void ofxTCPSyncClient::create(){
	if(bTCP)
	{}
	else{
		udpReceiver.Create();
		udpSender.Create();
		udpReceiver.SetReuseAddress(true);
		udpSender.SetReuseAddress(true);
	}
}

void ofxTCPSyncClient::loadIniFile(string _fileString) {
	out("Loading settings from file " + _fileString);
    
	ofxXmlSettings xmlReader;
    if (!xmlReader.loadFile(_fileString)) 
        err("ERROR loading XML file!");
	else
		out("Xml Loaded successfully");

    // parse INI file
    hostName   = xmlReader.getValue("settings:server:ip", "127.0.0.1", 0);
    serverOutPort = xmlReader.getValue("settings:server:serveroutport", 11999, 0);
	id         = xmlReader.getValue("settings:client_id", 0, 0);
	serverInPort = xmlReader.getValue("settings:serveroutport", 11998, 0);
	bTCP = xmlReader.getValue("settings:protocol","UDP") == "TCP";
	
 //   out("Settings: Out server = " + hostName + ":" + ofToString(serverOutPort) + ",  id = " + ofToString(id));
}

void ofxTCPSyncClient::out(string _str) {
    print(_str);
}

void ofxTCPSyncClient::print(string _str) {
//    if (DEBUG)
        cout << "ofxTCPSyncClient: " << _str << endl;
}

void ofxTCPSyncClient::err(string _str) {
    cerr << "ofxTCPSyncClient: " << _str << endl;
}

void ofxTCPSyncClient::start() {
    if(bTCP)
	{
		if(!tcpClient.setup(hostName,serverOutPort, false)){
			err("TCP Client :: Setup failed");
			return;
		}
		else
			out("TCP Client sending on::" +ofToString(serverOutPort));

		startThread(true, false);
	}

	else{
		if (!udpSender.Connect(hostName.c_str(), 11999)) {
		    err("UDP Out Server :: Setup Failed");
			return;
		}
		else if (!udpReceiver.Bind(serverInPort)){
			err("UDP In Server :: Setup failed");
			return;
		}
		else{
			out("UDP Input Server setup on::" +ofToString(serverInPort));
			out("UDP Output Server setup on::" +ofToString(serverOutPort));
		}

		startThread(true, false);
	}
      // blocking, verbose
}

void ofxTCPSyncClient::threadedFunction() {
    out("Running!");
    
    // let the server know that this client is ready to start
	send("S" + ofToString(id));
	
    while (isThreadRunning()) {
        if (lock()) {
			if(bTCP){
				msg = tcpClient.receive();
				if (msg.length() > 0) {
					read(msg);
				}
			}
			else{
				char udpMessage[10];
				udpReceiver.Receive(udpMessage,10);
				msg = udpMessage;
				if (msg.length() > 0) {
					read(msg);
				}
			}
            
            
            unlock();
            ofSleepMillis(5);
		}
    }
}

void ofxTCPSyncClient::read(string _serverInput) {
    out("Receiving: " + _serverInput);
        
    char c = _serverInput.at(0);
	if (c == 'G' || c == 'B' || c == 'I') {
        if (!allConnected) {
//            if (DEBUG) out("all connected!");
            allConnected = true;
			out(ofToString(allConnected));
        }
       // out ("split into frame message and data message");
        vector<string> info = ofSplitString(_serverInput, ":");
        vector<string> frameMessage = ofSplitString(info[0], ",");
        int fc = ofToInt(frameMessage[1]);
        
        if (info.size() > 1) {
            // there is a message here with the frame event
            info.erase(info.begin());
            dataMessage.clear();
            dataMessage = info;
            bMessageAvailable = true;
        } else {
            bMessageAvailable = false;
        }
        
        // assume no arrays are available
        bIntsAvailable  = false;
        bBytesAvailable = false; 
        
        if (fc == frameCount) {
            rendering = true;
			//out("rendering true");
            frameCount++;
            
            // calculate new framerate
            float ms = ofGetElapsedTimeMillis() - lastMs;
            fps = 1000.f / ms;
            lastMs = ofGetElapsedTimeMillis();
            
            if (!autoMode) {
                parent->frameEvent();
            }
        }
    }

	if (c == 'X')
	{
		tcpClient.close();
		stopThread();
		ofSleepMillis(500);
		start();
	}
}

void ofxTCPSyncClient::send(string _msg) {
    out("Sending: " + _msg);
    
    //_msg += "\n";
	if(bTCP)
		tcpClient.send(_msg);
	else
    udpSender.Send(_msg.c_str(),_msg.length());
}

void ofxTCPSyncClient::broadcast(string _msg) {
    _msg = "T" + _msg;
    send(_msg);
}

//--------------------------------------------------------------
// Sends a "Done" command to the server. This must be called at 
// the end of the draw loop.
//--------------------------------------------------------------
void ofxTCPSyncClient::done() {
    //if (broadcastingData) {
    //    sayDoneAgain = true;
    //} else {
    
    rendering = false;
    string msg = "D," + ofToString(id) + "," + ofToString(frameCount);
    send(msg);
    //}
}

//--------------------------------------------------------------
// Stops the client thread.  You don't really need to do this ever.
//--------------------------------------------------------------
void ofxTCPSyncClient::quit() {
    out("Quitting.");
	if(bTCP){
		sendDisconnect();
		ofSleepMillis(250);
		tcpClient.close();
		setDefaults();
	}
	else{
		sendDisconnect();
		udpReceiver.Close();
		udpSender.Close();
	}
    stopThread();
}

void ofxTCPSyncClient::sendDisconnect() {

	send("X" + ofToString(id));
	//ofSleepMillis(10);
}