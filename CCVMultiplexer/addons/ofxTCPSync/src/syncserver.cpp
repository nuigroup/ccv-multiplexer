#include "syncserver.h"

syncserver::~syncserver(){
	quit();

}

void syncserver::serverSetup(string _fileString){
	loadServerSettings(_fileString);
	setDefaults();
		
	for(int i = 0; i < numExpectedClients; i++){
		
		connection * c =new connection ;
		c->started = false;
		c->ready = false;
		c->height =240;
		c->width = 320;
		c->depth =1;
		c->serverIndex = i;
		c->blobImage.allocate(320,240);
		connections.push_back(c);
		
	}
	
}

void syncserver::loadServerSettings(string _fileString){
	ofxXmlSettings* xmlReader = new ofxXmlSettings();
	 if (!xmlReader->loadFile(_fileString)) 
        err("ERROR loading XML file!");
	else
		out("Xml Loaded successfully");

	 
    Fps=xmlReader->getValue("settings:fps", 5, 0);
	serverInPort = xmlReader->getValue("settings:serverinport", 11999, 0);
	serverOutPort = xmlReader->getValue("settings:serveroutport",11998,0);
	numExpectedClients = xmlReader->getValue("settings:numclients", 1, 0);
	broadcast = xmlReader->getValue("settings:broadcast", "192.168.1.255", 0);
	bTCP = xmlReader->getValue("settings:protocol","UDP") == "TCP";
	i=numExpectedClients;

    out("XML Settings: fps = " + ofToString(Fps) + ", Number of Client = " + ofToString(numExpectedClients));
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
	udpReceiver.Create();
	udpSender.Create();
	udpReceiver.SetReuseAddress(true);
	udpSender.SetReuseAddress(true);
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
	isServerThreadRunning = true;
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
	
		EnterCriticalSection(&criticalSection);
		check = 1;
		LeaveCriticalSection(&criticalSection);
		
		if(shouldTriggerFrame){
							
					float now = ofGetElapsedTimef();
					float elapsed = (now - lastFrameTriggeredTime);
					
					if(elapsed >= 1.0/Fps ){
						
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
						char udpMessage[100000];
						udpReceiver.Receive(udpMessage,100000);
						string response = udpMessage;
						if (response.length() > 0) {
							read(response);
						}	
					}
		}
		
	ofSleepMillis(5);
	}
}

void syncserver::shouldContinue(){
	i--;
	if(!allconnected){
		allconnected = true;
		for(int c = 0; c < connections.size(); c++){
			if(!connections[c]->started){
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
			if(!connections[c]->ready){
				allready = false;
				break;
			}
						
		}
		if(allready && (i==0)){
			shouldTriggerFrame = true;
		}
	}
	
				//	shouldContinue=false;
}

//For UDP
void syncserver::read(string response) {
	out("Receiving: " + response);
	char first = response.at(0);
	int clientID = ofToInt(response.substr(1,1));

	switch(first)
	{

	case 'S':
		{
					//this is where it starts!
			
			if(clientID < numExpectedClients){
					vector<string> info = ofSplitString(response, ",");
					connections[clientID]->serverIndex = clientID;
					connections[clientID]->started = true;
					cout << "Client ID " << clientID << " with response " << response << endl;
					shouldContinue();
				}
			else{
					err("Received Client ID " + ofToString(clientID)  + " out of range");
				}
		}
		break;

	case 'D':
		{
		
			vector<string> info = ofSplitString(response, ",");
			if(info.size() >= 3){
				
				int fc = ofToInt(info[1]);
				if(fc == currentFrame-1){

					connections[clientID]->ready = true;
					shouldContinue();
				}
				if(ofToInt(info[2])==0) //to draw black screen when no blobs are being received!!
				{
					EnterCriticalSection(&criticalSection);
					connections[clientID]->points.clear();
					memset(connections[clientID]->blobImage.getPixels(),0,320*240);
					LeaveCriticalSection(&criticalSection);
				}
			}
			
		}
		break;
	
	case 'F':
		{

			vector<string> strEntries = ofSplitString(response,"[/p]");
			connections[clientID]->points.clear();
			for(int i=1;i<strEntries.size();i++){
				vector<string> entry = ofSplitString(strEntries[i],"|");
				blob temp;
				temp.centroid.x= atof(entry[0].c_str())*320;
				temp.centroid.y = atof(entry[1].c_str())*240;
				temp.axes.width = atof(entry[2].c_str());
				temp.axes.height = atof(entry[3].c_str());
				temp.angle = atof(entry[4].c_str());
		
				connections[clientID]->points.push_back(temp);
			
			}
			EnterCriticalSection(&criticalSection);
			if(connections[clientID]->points.size()!=0)
			{
				memset(connections[clientID]->blobImage.getPixels(),0,320*240);
			//meset is faster than using cvRectangle!
			//cvRectangle(connections[clientID]->blobImage.getCvImage(),	cvPoint(0,0),cvPoint(320,240),cvScalar(0,0,0),-1);
				for(std::vector<blob>::iterator it = connections[clientID]->points.begin();it != connections[clientID]->points.end();it++)
				{	
					//cvCircle can be used instead of cvellipse, should be considerably faster!!
					cvEllipse(connections[clientID]->blobImage.getCvImage(),cvPoint((*it).centroid.x,(*it).centroid.y),cvSize((*it).axes.width,(*it).axes.height), (*it).angle,0,360,cvScalar(255,255,255),-1); 
				}
			}
			LeaveCriticalSection(&criticalSection);
		}
		break;
	
	case 'O':
		{
		vector<string> strEntries = ofSplitString(response,"[/p]");
		connections[clientID]->points.clear();
		for(int i=1;i<strEntries.size();i++){
			vector<string> entry = ofSplitString(strEntries[i],"|");
			blob temp;
			temp.centroid.x= atof(entry[0].c_str())*320;
			temp.centroid.y = atof(entry[1].c_str())*240;
			temp.axes.width = atof(entry[2].c_str());
			temp.axes.height = atof(entry[3].c_str());
			temp.angle = atof(entry[4].c_str());
			connections[clientID]->points.push_back(temp);
		}
		EnterCriticalSection(&criticalSection);
		if(connections[clientID]->points.size()!=0)
		{
			memset(connections[clientID]->blobImage.getPixels(),0,320*240);
			//cvRectangle(connections[clientID]->blobImage.getCvImage(),	cvPoint(0,0),cvPoint(320,240),cvScalar(0,0,0),-1);
			for(std::vector<blob>::iterator it = connections[clientID]->points.begin();it != connections[clientID]->points.end();it++)
			{	
				cvRectangle(connections[clientID]->blobImage.getCvImage(),cvPoint((*it).centroid.x,(*it).centroid.y),cvPoint((*it).centroid.x+(*it).axes.width,(*it).centroid.y+(*it).axes.height),cvScalar(255,255,255),-1);
			}
		}
		LeaveCriticalSection(&criticalSection);
		}
		break;

	case 'X':
		{
		connections[clientID]->started = false;
		connections[clientID]->ready = false;
		send("X" + ofToString(clientID));
		allconnected = false;
		currentFrame = 0;
		numConnectedClients--;
		}
		break;
	}
}

//for TCP
void syncserver::read(string response,int i) {
	
	out("Receiving: " + response);
	char first = response.at(0);
	int clientID = ofToInt(response.substr(1,1));

	switch(first)
	{
	case 'S':
		{
					//that's the start!
					if(clientID < numExpectedClients){
					vector<string> info = ofSplitString(response, ",");
					
					connections[clientID]->serverIndex = clientID;
					connections[clientID]->started = true;
					shouldContinue();
					
					numConnectedClients++;
					cout << "Client ID " << clientID << " with response " << response << endl;
					
				}
			else{
					err("Received Client ID " + ofToString(clientID)  + " out of range");
				}
		}
		break;

	case 'D':
		{
	
			vector<string> info = ofSplitString(response, ",");
			if(info.size() >= 3){
				
				int fc = ofToInt(info[1]);
				if(fc == currentFrame-1){
				   
					connections[clientID]->ready = true;
					shouldContinue();
					
				}
				if(ofToInt(info[2])==0)
				{
					EnterCriticalSection(&criticalSection);
					connections[clientID]->points.clear();
					memset(connections[clientID]->blobImage.getPixels(),0,320*240);
					LeaveCriticalSection(&criticalSection);
				}
						
			}
		}
		break;

	case 'F':
		{
		vector<string> strEntries = ofSplitString(response,"[/p]");
		int clientID = ofToInt(response.substr(1,1));
		connections[clientID]->points.clear();
		for(int i=1;i<strEntries.size();i++){
			vector<string> entry = ofSplitString(strEntries[i],"|");
			blob temp;
			temp.centroid.x= atof(entry[0].c_str())*320;
			temp.centroid.y = atof(entry[1].c_str())*240;
			temp.axes.width = atof(entry[2].c_str());
			temp.axes.height = atof(entry[3].c_str());
			temp.angle = atof(entry[4].c_str());
			connections[clientID]->points.push_back(temp);
		}

		EnterCriticalSection(&criticalSection);
			if(connections[clientID]->points.size()!=0)
			{
				memset(connections[clientID]->blobImage.getPixels(),0,320*240);
			//meset is faster than using cvRectangle!
			//cvRectangle(connections[clientID]->blobImage.getCvImage(),	cvPoint(0,0),cvPoint(320,240),cvScalar(0,0,0),-1);
				for(std::vector<blob>::iterator it = connections[clientID]->points.begin();it != connections[clientID]->points.end();it++)
				{	
					cvEllipse(connections[clientID]->blobImage.getCvImage(),cvPoint((*it).centroid.x,(*it).centroid.y),cvSize((*it).axes.width,(*it).axes.height), (*it).angle,0,360,cvScalar(255,255,255),-1); 
				}
			}
		LeaveCriticalSection(&criticalSection);
		
		}
		break;
	
	case 'O':
		{
		vector<string> strEntries = ofSplitString(response,"[/p]");
		connections[clientID]->points.clear();
		for(int i=1;i<strEntries.size();i++){
			vector<string> entry = ofSplitString(strEntries[i],"|");
			blob temp;
			temp.centroid.x= atof(entry[0].c_str())*320;
			temp.centroid.y = atof(entry[1].c_str())*240;
			temp.axes.width = atof(entry[2].c_str());
			temp.axes.height = atof(entry[3].c_str());
			temp.angle = atof(entry[4].c_str());
			connections[clientID]->points.push_back(temp);
		}
		EnterCriticalSection(&criticalSection);
		if(connections[clientID]->points.size()!=0)
		{
			memset(connections[clientID]->blobImage.getPixels(),0,320*240);
			//cvRectangle(connections[clientID]->blobImage.getCvImage(),cvPoint(0,0),cvPoint(320,240),cvScalar(0,0,0),-1);
			for(std::vector<blob>::iterator it = connections[clientID]->points.begin();it != connections[clientID]->points.end();it++)
			{	
				cvRectangle(connections[clientID]->blobImage.getCvImage(),cvPoint((*it).centroid.x,(*it).centroid.y),cvPoint((*it).centroid.x+(*it).axes.width,(*it).centroid.y+(*it).axes.height),cvScalar(255,255,255),-1);
			}
		}
		LeaveCriticalSection(&criticalSection);
		}
		break;

	case 'X':
		{
		
		send("X" + ofToString(clientID));
			connections[clientID]->started = false;
			connections[clientID]->ready = false;
			allconnected = false;
			currentFrame=0;
			numConnectedClients--;
		}
	break;
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
	EnterCriticalSection(&criticalSection);
	isServerThreadRunning = false;
	LeaveCriticalSection(&criticalSection);
	
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
	
    CloseHandle(serverThread);
}

//not used!! 
void syncserver::restartServer(){
	out("Restarting Server");
	isServerThreadRunning = false;
	if(bTCP){
	tcpServer.close();
	
	}
	else{
		udpSender.Close();
		udpReceiver.Close();
	}

	//EnterCriticalSection(&criticalSection);
	//isServerThreadRunning = false;
	//LeaveCriticalSection(&criticalSection);
	//CloseHandle(serverThread);
	
	out("Server Restart in 1 second!!!");
	ofSleepMillis(1000);
	startServer();
	out("Server Restart Complete!!!");
}


void syncserver::disconntinue(){
	EnterCriticalSection(&criticalSection);
	check = false;
	isServerThreadRunning = false;
	LeaveCriticalSection(&criticalSection);
}

