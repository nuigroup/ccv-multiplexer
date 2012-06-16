#include "ofxTCPSyncClient.h"

//--------------------------------------------------------------
ofxTCPSyncClient::ofxTCPSyncClient() {
    setDefaults(); 
}

//--------------------------------------------------------------
void ofxTCPSyncClient::setup(string _fileString, ofxTCPSyncClientListener* _parent, bool _autoMode) {
   parent   = _parent;

    autoMode = _autoMode;
    
    loadIniFile(_fileString);
    udpReceiver.Create();
	udpSender.Create();
	udpReceiver.SetReuseAddress(true);
	udpSender.SetReuseAddress(true);
    
}

void ofxTCPSyncClient::loadIniFile(string _fileString) {
	out("Loading settings from file " + _fileString);
    
	ofxXmlSettings xmlReader;
    if (!xmlReader.loadFile(_fileString)) 
        err("ERROR loading XML file!");

    
    hostName   = xmlReader.getValue("settings:server:ip", "127.0.0.1", 0);
    serverOutPort = xmlReader.getValue("settings:server:port", 11999, 0);
	id         = xmlReader.getValue("settings:client_id", 0, 0);
	serverInPort = 11998;
	//if (xmlReader.getValue("settings:debug", 0, 0) == 1) 
//        DEBUG = true;

    out("Settings: Out server = " + hostName + ":" + ofToString(serverOutPort) + ",  id = " + ofToString(id));
}

void ofxTCPSyncClient::out(string _str) {
    print(_str);
}

void ofxTCPSyncClient::print(string _str) {

        cout << "ofxTCPSyncClient: " << _str << endl;
}

void ofxTCPSyncClient::err(string _str) {
    cerr << "ofxTCPSyncClient: " << _str << endl;
}

void ofxTCPSyncClient::start() {
//    tcpClient.setVerbose(DEBUG);
    if (!udpSender.Connect("127.0.0.1", 11999)) {
        err("UDP failed to connect to port " + ofToString(serverOutPort));
        return;
    }
    udpReceiver.Bind(serverInPort);
    out("UDP out-bound connectionon port " + ofToString(serverOutPort));
	out("UDP In-bound connectionon port " + ofToString(serverInPort));
    startThread(true, false);  // blocking, verbose
}

void ofxTCPSyncClient::threadedFunction() {
    out("Running!");
    
    // let the server know that this client is ready to start
    send("S" + ofToString(id));
    
    while (isThreadRunning()) {
        if (lock()) {
			char udpMessage[10];
			udpReceiver.Receive(udpMessage,10);
            string msg = udpMessage;
            if (msg.length() > 0) {
                read(msg);
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

            allConnected = true;
			out(ofToString(allConnected));
        }
        out ("split into frame message and data message");
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
        
       
        bIntsAvailable  = false;
        bBytesAvailable = false; 
        
        if (fc == frameCount) {
            rendering = true;
			out("rendering true");
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
}

void ofxTCPSyncClient::send(string _msg) {
    out("Sending: " + _msg);
    
    //_msg += "\n"; --->using as delimiter
    udpSender.Send(_msg.c_str(),_msg.length());
}

void ofxTCPSyncClient::broadcast(string _msg) {
    _msg = "T" + _msg;
    send(_msg);
}


void ofxTCPSyncClient::done() {
   
    
    rendering = false;
    string msg = "D," + ofToString(id) + "," + ofToString(frameCount);
    send(msg);
    //}
}

void ofxTCPSyncClient::quit() {
    out("Quitting.");
	udpReceiver.Close();
	udpSender.Close();
    stopThread();
}

