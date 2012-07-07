#include "syncserver.h"


void syncserver::serverSetup(string _fileString){
	loadServerSettings(_fileString);
	setDefaults();
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
		
		connection * c =new connection ;
		c->started = false;
		c->ready = false;
		c->height =240;
		c->width = 320;
		c->depth =3;
		c->serverIndex = i;
		c->blobImage.allocate(320,240);
		//c->name = "noname";
		//c->test=false;
		connections.push_back(c);
		
	}
	
	y=1;
}

void syncserver::loadServerSettings(string _fileString){
	ofxXmlSettings* xmlReader = new ofxXmlSettings();
	 if (!xmlReader->loadFile(_fileString)) 
        err("ERROR loading XML file!");
	else
		out("Xml Loaded successfully");

	 
    fps=xmlReader->getValue("settings:fps", 5, 0);
	serverInPort = xmlReader->getValue("settings:serverinport", 11999, 0);
	serverOutPort = xmlReader->getValue("settings:serveroutport",11998,0);
	numExpectedClients = xmlReader->getValue("settings:numclients", 1, 0);
	broadcast = xmlReader->getValue("settings:broadcast", "192.168.1.255", 0);
	bTCP = xmlReader->getValue("settings:protocol","UDP") == "TCP";
	i=numExpectedClients;

    out("XML Settings: fps = " + ofToString(fps) + ", Number of Client = " + ofToString(numExpectedClients));
	delete xmlReader;
	xmlReader = NULL;
}





void syncserver::out(string _str) {
    print(_str);
}

void syncserver::print(string _str) {
    cout << "syncServer: " << _str << endl;
}

void syncserver::err(string _str) {
    cerr << "syncServer error: " << _str << endl;
}


void syncserver::startServer(){
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

	InitializeCriticalSection(&criticalSection);
	isServerThreadRunning =true;
	serverThread = CreateThread(NULL, 0, &syncserver::ServerThread, this, 0, 0);
}

DWORD WINAPI syncserver::ServerThread(LPVOID instance)
{
	syncserver *pThis = (syncserver*)instance;
	pThis->Server();
	return 0;
}

void syncserver::Server(){
	out("Running!");
        
	while (isServerThreadRunning) {
	//	EnterCriticalSection(&criticalSection);cout<<"y="<<y<<endl;
		//if(y%2==0){
			//cvRectangle(connections[0]->blobImage.getCvImage(),cvPoint(0,0),cvPoint(320,240),cvScalar(0,0,0),-1);
			//cvCircle(connections[0]->blobImage.getCvImage(),cvPoint(100,100),70,cvScalar(255,255,255),-1);
			//connections[0]->test.loadImage("car.jpg");
//			connections[1]->test.loadImage("car.jpg");
			
		

		//}
		//else{
			//cvRectangle(connections[0]->blobImage.getCvImage(),cvPoint(0,0),cvPoint(320,240),cvScalar(255,255,255),-1);
			//cvCircle(connections[0]->blobImage.getCvImage(),cvPoint(100,100),70,cvScalar(0,0,0),-1);
		//connections[0]->test.loadImage("black.jpg");
		
		
		//}

		//LeaveCriticalSection(&criticalSection);
		//y++;
		
		if(shouldTriggerFrame){
						
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
					
		}
		
		
		else {
						
			
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
		}
					
			
		
	ofSleepMillis(50);
	}
}

void syncserver::shouldContinue(){
	i--;
	if(!allconnected){
		allconnected = true;
		EnterCriticalSection(&criticalSection);
		for(int c = 0; c < connections.size(); c++){
			if(!connections[c]->started){
			allconnected = false;
			break;
			}
		}
		LeaveCriticalSection(&criticalSection);
		if(allconnected){
			shouldTriggerFrame = true;
		}
						
	}
					
					//All connected and going
	else {
		
		bool allready = true;
		EnterCriticalSection(&criticalSection);
		for(int c = 0; c < connections.size(); c++){
			if(!connections[c]->ready){
				allready = false;
				break;
			}
						
		}
		LeaveCriticalSection(&criticalSection);
		if(allready && (i==0)){
			shouldTriggerFrame = true;
		}
	}
				//	shouldContinue=false;
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
					connections[clientID]->started = true;
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
					connections[clientID]->ready = true;
					shouldContinue();
				}
				
			}
	}
	else if(first == 'X'){
		int clientID = ofToInt(response.substr(1,1));
		connections[clientID]->started = false;
		connections[clientID]->ready = false;
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
					EnterCriticalSection(&criticalSection);
					connections[clientID]->serverIndex = i;
					connections[clientID]->started = true;
					LeaveCriticalSection(&criticalSection);
//					connections[clientID].name = info[1];
					shouldContinue();
					
					numConnectedClients++;
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
				if(fc == currentFrame-1){
				   	//todo validate client id
					EnterCriticalSection(&criticalSection);
					connections[clientID]->ready = true;
					//cvCircle(connections[clientID]->blobImage.getCvImage(),cvPoint(320,240),70,cvScalar(255,255,255),-1);
					LeaveCriticalSection(&criticalSection);
					shouldContinue();
					
				}
				else
					out("Frame Mis-Match");
						
			}
	}
	else if(first == 'F'){
		vector<string> strEntries = ofSplitString(response,"[/p]");
		int clientID = ofToInt(response.substr(1,1));
		for(int i=1;i<strEntries.size();i++){
			vector<string> entry = ofSplitString(strEntries[i],"|");
			ofPoint temp;
			temp.x = atof(entry[0].c_str())*320;
			temp.y = atof(entry[1].c_str())*240;
			cout<<"X:"<<temp.x;
			cout<<"y"<<temp.y<<endl;
			EnterCriticalSection(&criticalSection);
			connections[clientID]->points.push_back(temp);
			//cvCircle(connections[clientID]->blobImage.getCvImage(),cvPoint(320,240),30,cvScalar(255,255,255),-1);	
			LeaveCriticalSection(&criticalSection);
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
			EnterCriticalSection(&criticalSection);
			connections[clientID]->started = false;
			connections[clientID]->ready = false;
			LeaveCriticalSection(&criticalSection);
		}
		//send("X");
		setDefaults();
		restartServer();
	}
	else{
		EnterCriticalSection(&criticalSection);
		connections[i]->started = false;
		connections[i]->ready =false;
		LeaveCriticalSection(&criticalSection);
	}
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
	EnterCriticalSection(&criticalSection);
	connections.clear();
	LeaveCriticalSection(&criticalSection);
	isServerThreadRunning = false;
  //  CloseHandle(serverThread);
}

void syncserver::restartServer(){
	out("Restarting TCP Server");
	tcpServer.close();
	isServerThreadRunning = false;
	//CloseHandle(serverThread);
	isServerThreadRunning = false;
	ofSleepMillis(3000);
	startServer();
	out("TCP Server Restart Complete!!!");
}










/*

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
		
		connection c ;
		c.started = false;
		c.ready = false;
		c.height =240;
		c.width = 320;
		c.depth =3;
		c.serverIndex = i;
		c.blobImage.allocate(320,240);
		//c->name = "noname";
		c.test=false;
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

//void syncserver::abc() {
void syncserver::threadedFunction(){
    out("Running!");
        
    while (isThreadRunning()) {
		//lock();
	cout<<"SYNCSERVER VALUE OF TEST"<<connections.size()<<endl;
		//connections[0].test=true;
		//unlock();

		
		
		if(shouldTriggerFrame){
						
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
					
		}
		
		
		else {
						
			
					if(bTCP){
						for(int i = 0; i < tcpServer.getLastID(); i++){
							if(tcpServer.isClientConnected(i)){
								string response = tcpServer.receive(i);
									if (response.length() > 0) {
										//lock();
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
		}
					
			
		
	ofSleepMillis(50);
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
					shouldContinue();
					
					numConnectedClients++;
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
				if(fc == currentFrame-1){
				   	//todo validate client id
					
					connections[clientID].ready = true;
					shouldContinue();
					
				}
				else
					out("Frame Mis-Match");
						
			}
	}
	else if(first == 'F'){
		vector<string> strEntries = ofSplitString(response,"[/p]");
		int clientID = ofToInt(response.substr(1,1));
		for(int i=1;i<strEntries.size();i++){
			vector<string> entry = ofSplitString(strEntries[i],"|");
			ofPoint temp;
			temp.x = atof(entry[0].c_str())*320;
			temp.y = atof(entry[1].c_str())*240;
			lock();
			//cvCircle(connections[clientID]->blobImage.getCvImage(),cvPoint(temp.x,temp.y),30,cvScalar(255,255,255),-1);	
			unlock();
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
			lock();
			connections[clientID].started = false;
			connections[clientID].ready = false;
			unlock();
		}
		//send("X");
		setDefaults();
		restartServer();
	}
	else{
		lock();
		connections[i].started = false;
		connections[i].ready =false;
		unlock();
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

void syncserver::copy(){
	//test.allocate(320,240);
	//if(shouldTriggerFrame)
	//test = connections[0].blobImage;
	
}

void syncserver::getPixels(int id, unsigned char * newFrame)
{
	//cout<<"**********<<"<<shouldTriggerFrame;
	//if(shouldTriggerFrame)
	//memcpy((void*)newFrame,(unsigned char*)test.getPixels(),320*240*3*sizeof(unsigned char));
	//int ID=id;
	
	//cout<<"\n*****************getpixels called*********************\n";
	//if(x>10)
	//te.setFromPixels((unsigned char*)fbopixels,320,240,OF_IMAGE_COLOR_ALPHA);
	//else
	

	//memcpy((unsigned char*)blackPixels,fbopixels,320 * 240 * 1 * sizeof(unsigned char));
	//for (int j = 0; j < 320*240; j++){
	//		connections[i].blackPixels[j] = (unsigned char)(255);
	//	}
	//ofImage test;
	//test.allocate(320,240);
	//memcpy((unsigned char*)newFrame,(unsigned char*)connections[i].blackPixels,320 * 240 * 1 * sizeof(unsigned char));
	//ofxFBOTexture* myfbo = new ofxFBOTexture;
	//myfbo->allocate(320,240);
	
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
	//}

