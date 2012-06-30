#include "syncserver.h"

syncserver::syncserver() {
//	fbo.allocate(320,240);
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
		c.height =240;
		c.width = 320;
		c.depth =1;
		c.serverIndex = i;
		//c.name = "noname";
		connections.push_back(c);
	}

	blackPixels			= new unsigned char [320* 240];
	for (int j = 0; j < 320*240; j++){
			blackPixels[j] = (unsigned char)(255);
		}
}

void syncserver::loadIniFile(string _fileString) {
	out("Loading settings from file " + _fileString);
    
	ofxXmlSettings xmlReader;
    if (!xmlReader.loadFile(_fileString)) 
        err("ERROR loading XML file!");
	else
		out("Xml Loaded successfully");

    fps=xmlReader.getValue("settings:fps", 5, 0);
	serverInPort = xmlReader.getValue("settings:serverinport", 11999, 0);
	serverOutPort = xmlReader.getValue("settings:serveroutport",11998,0);
	numExpectedClients        = xmlReader.getValue("settings:numclients", 1, 0);
	broadcast = xmlReader.getValue("settings:broadcast", "192.168.1.255", 0);
	bTCP = xmlReader.getValue("settings:protocol","UDP") == "TCP";
	i=numExpectedClients;

    out("XML Settings: fps = " + ofToString(fps) + ", Number of Client = " + ofToString(numExpectedClients));
	
}

void syncserver::out(string _str) {
    print(_str);
}

void syncserver::print(string _str) {
    cout << "syncServer: " << _str << endl;
}

void syncserver::err(string _str) {
    cerr << "ofxTCPSyncClient: " << _str << endl;
}

void syncserver::start() {
	//tuioclient.connect(3333);
	//this->addTuioListener(this);
	
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
				if(shouldTriggerFrame){
					lock();
					float now = ofGetElapsedTimef();
					float elapsed = (now - lastFrameTriggeredTime);

					if(elapsed >= 1.0/fps){
							string message = "G,"+ofToString(currentFrame);
							if (newMessage){
								message += ","+currentMessage;
								newMessage = false;
								currentMessage = "";
							}

							send(message);
							shouldTriggerFrame = false;
							lastFrameTriggeredTime = now;
						currentFrame++;
						i=numExpectedClients;
					}
					unlock();
				}

				else {
					lock();
					if(bTCP){
						for(int i = 0; i < tcpServer.getLastID(); i++){
							if(tcpServer.isClientConnected(i)){
								string response = tcpServer.receive(i);
									if (response.length() > 0) {
										read(response,i);
									}
							}
							else
								tcpServer.disconnectClient(i);

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
					unlock();
					ofSleepMillis(5);
				}
			
	}
}


void syncserver::shouldContinue(){
	i--;
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
		if(allready && (i==0)){
			shouldTriggerFrame = true;
		}
	}
				//	shouldContinue=false;
} //should continue loops end


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
					shouldContinue();
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
				if(fc == currentFrame-1){

					//todo validate client id
					connections[clientID].ready = true;
					shouldContinue();
				}
				
			}
	}
	else if(first == 'X'){
		int clientID = ofToInt(response.substr(1,1));
		connections[clientID].started = false;
		connections[clientID].ready = false;
		allconnected = false;
			//numConnectedClients = 0;
			currentFrame = 0;
			shouldTriggerFrame = false;
		//setDefaults();
				
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
					connections[clientID].serverIndex = i;
					connections[clientID].started = true;
//					connections[clientID].name = info[1];
					numConnectedClients++;
					cout << "Client ID " << clientID << " with response " << response << endl;
					shouldContinue();
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
				if(fc == currentFrame-1){
				   	//todo validate client id
					connections[clientID].ready = true;
					shouldContinue();
				}
				else
					out("here is the error");
						
			}
	}
	else if(first == 'F'){
		vector<string> strEntries = ofSplitString(response,"[/p]");
		int clientID = ofToInt(response.substr(1,1));
		for(int i=1;i<strEntries.size();i++){
			vector<string> entry = ofSplitString(strEntries[i],"|");
			ofPoint temp;
			temp.x = atof(entry[0].c_str());
			temp.y = atof(entry[1].c_str());
			connections[clientID].points.push_back(temp);
		}
	}
		

	else if(first == 'X'){

		
		//ofSleepMillis(150);
		ofSleepMillis(550);
		send("X");
		ofSleepMillis(500);
		int clientID = ofToInt(response.substr(1,1));
		out("Diconnect all clients"+ofToString(i));
		for(int i = 0; i < numExpectedClients; i++){
			tcpServer.disconnectClient(i);
			connections[clientID].started = false;
			connections[clientID].ready = false;
		}
		//send("X");
		setDefaults();
		restartServer();
	}
	else{
		connections[i].started = false;
		connections[i].ready =false;
	}
}

void getCalibData()
{
	string mesg;
	mesg = "C";
	//send(message);
	//send(mesg);	
}

void syncserver::send(string _msg) {
    out("Sending: " + _msg);
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
//	tuioclient.disconnect();
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
	ofSleepMillis(3000);
	start();
	out("TCP Server Restart Complete!!!");
}


/*void syncserver::initimage(){
	
	for(int i = 0; i < numExpectedClients; i++){
	//	connections[i].blobImage.allocate(width, height); //main Image that'll be processed.
	//	connections[i].blackImage.allocate(width, height);
	//	connections[i].blobImageBw.allocate(width, height);
	//	connections[i].testImage.allocate(width, height,OF_IMAGE_GRAYSCALE);
		connections[i].blackPixels			= new unsigned char [width* height];

		// black pixels
		for (int j = 0; j < width*height; j++){
			connections[i].blackPixels[j] = (unsigned char)(0);
		}
	
		connections[i].blackImage.setFromPixels(blackPixels,width, height);
	}
}*/


void syncserver::getPixels(int id, unsigned char *newFrame)
{
	int ID=id;
	cout<<"\n*****************getpixels called*********************\n";
	//for (int j = 0; j < 320*240; j++){
	//		connections[i].blackPixels[j] = (unsigned char)(255);
	//	}
	//ofImage test;
	//test.allocate(320,240);
	//memcpy((unsigned char*)newFrame,(unsigned char*)connections[i].blackPixels,320 * 240 * 1 * sizeof(unsigned char));
	//ofxFBOTexture* myfbo = new ofxFBOTexture;
	//myfbo->allocate(320,240);
	//myfbo->begin();
	//myfbo->clear();
	//ofSetColor(0);
	//myFBO.begin();
	//ofSetColor(100, 0,0);
	//ofRect(0,0,320,240);
	//ofSetColor(255,0,0);
	//ofCircle(50,50,20);
	//ofLine(10,10,50,50);
	//ofRect(0,0,320,240);
	//ofSetColor(1);
	//for(std::vector<ofPoint>::iterator it = connections[ID].points.begin();it != connections[ID].points.end();it++)
	//{
		//ofCircle( (*it).x * connections[ID].width, (*it).y*connections[ID].height, 10);

	//}
	//myFBO.end();
	//myFBO.draw(5,4,320,240);
	//unsigned char * depth = (unsigned char*)myFBO.getPixels();
	/*for (int k = 0; k < 320; k++){
		for (int j = 0; j < 240; j++){
			int value = depth[j * 320 +k];
			cout<<value<<" ";
		}
	}
	*/

	//memcpy((unsigned char*)pixels,(unsigned char*)myfbo->getPixels(),320 * 240 * 1 * sizeof(unsigned char));
	
	//myfbo->end();
	//connections[ID].points.clear();
	//delete myfbo;

	//fbo.allocate(320,240);
	//fbo.begin();
	//	ofCircle(10,10,50);
	//fbo.end();

}

