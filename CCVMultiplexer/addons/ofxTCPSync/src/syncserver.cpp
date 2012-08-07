#include "syncserver.h"

syncserver::~syncserver(){
	quit();

}

void syncserver::serverSetup(string _fileString){
	loadServerSettings(_fileString);
	setDefaults();
	//if(bTCP)
	//{}
	//else
	//{
		//udpReceiver.Create();
		//udpSender.Create();
		//udpReceiver.SetReuseAddress(true);
		//udpSender.SetReuseAddress(true);
	//}
	
	for(int i = 0; i < numExpectedClients; i++){
		
		connection * c =new connection ;
		c->started = false;
		c->ready = false;
		c->height =240;
		c->width = 320;
		c->depth =3;
		c->serverIndex = i;
		c->blobImage.allocate(320,240);
		c->test = (unsigned char*)malloc(320*240*sizeof(unsigned char));
		//c->name = "noname";
		//c->test=false;
		connections.push_back(c);
		
	}
	
	y=0;
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
		//cout<<"y="<<y<<endl;
		check = 1;
		


		LeaveCriticalSection(&criticalSection);
		
		//cout<<"ok"<<check;
		if(shouldTriggerFrame){
							
					float now = ofGetElapsedTimef();
					float elapsed = (now - lastFrameTriggeredTime);
					
					if(elapsed >= 1.0/Fps ){
						y=1;
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
						//out(response);
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
	y=0;
	cout<<"Should continue loop"<<endl;
	if(!allconnected){
		cout<<"abc asdasd**********"<<endl;
		allconnected = true;
		//EnterCriticalSection(&criticalSection);
		for(int c = 0; c < connections.size(); c++){
			if(!connections[c]->started){
			allconnected = false;
			break;
			}
		}
		
		//LeaveCriticalSection(&criticalSection);
		if(allconnected){
			shouldTriggerFrame = true;
		}
						
	}
			
					//All connected and going
	else {
		
		bool allready = true;
		//EnterCriticalSection(&criticalSection);
		for(int c = 0; c < connections.size(); c++){
			if(!connections[c]->ready){
				allready = false;
				break;
			}
						
		}
		//LeaveCriticalSection(&criticalSection);
		if(allready && (i==0)){
			shouldTriggerFrame = true;
		}
	}
	
				//	shouldContinue=false;
}

void syncserver::read(string response) {
	out("Receiving: " + response);
	char first = response.at(0);
	int clientID = ofToInt(response.substr(1,1));

	switch(first)
	{

	case 'S':
		{
					//this is where it starts!
			//int clientID = ofToInt(response.substr(1,1));
			if(clientID < numExpectedClients){
					vector<string> info = ofSplitString(response, ",");
					connections[clientID]->serverIndex = clientID;
					connections[clientID]->started = true;
//					connections[clientID].name = info[1];
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
				
				//int clientID = ofToInt(info[1]);
				int fc = ofToInt(info[1]);
				if(fc == currentFrame-1){

					cout<<"frame count"<<fc<<endl;
					connections[clientID]->ready = true;
					//EnterCriticalSection(&criticalSection);
					
					//cvCircle(connections[clientID]->blobImage.getCvImage(),cvPoint(320,240),70,cvScalar(255,255,255),-1);
					//image_fill(connections[clientID]->test,0);
					//LeaveCriticalSection(&criticalSection);
					shouldContinue();
				}
				if(ofToInt(info[2])==0)
				{
					cout<<"in black loop"<<endl;
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
			//int clientID = ofToInt(response.substr(1,1));
			//EnterCriticalSection(&criticalSection);
			connections[clientID]->points.clear();
			//LeaveCriticalSection(&criticalSection);
			for(int i=1;i<strEntries.size();i++){
				vector<string> entry = ofSplitString(strEntries[i],"|");
				blob temp;
			//for(int j=1;j<entry.size();j=j+2)
		//	{
			//	CvPoint tempp;
				
			//	tempp.x=atof(entry[j].c_str());
			//	tempp.y=atof(entry[j+1].c_str());

				//temp.pvect.push_back(tempp);
			//}

			//temp.contours.push_back(temp.pvect);

				temp.centroid.x= atof(entry[0].c_str())*320;
				temp.centroid.y = atof(entry[1].c_str())*240;
				temp.axes.width = atof(entry[2].c_str());
				temp.axes.height = atof(entry[3].c_str());
				temp.angle = atof(entry[4].c_str());
		
			//EnterCriticalSection(&criticalSection);
				connections[clientID]->points.push_back(temp);
			//cvCircle(connections[clientID]->blobImage.getCvImage(),cvPoint(320,240),30,cvScalar(255,255,255),-1);
			//image_fill(connections[clientID]->test,0);
			
			//LeaveCriticalSection(&criticalSection);
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
				//cvCircle(connections[clientID]->blobImage.getCvImage(),cvPoint((*it).centroid.x ,(*it).centroid.y),10,cvScalar(255,255,255),-1);
			//	draw_circle (connections[clientID]->test, 10,(*it).x,(*it).y, 0xff);
			//	cvDrawContours(connections[clientID]->blobImage.getCvImage(),(*it).contours,cvScalar(255,255,255),cvScalar(255,255,255),0,-1);
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
		//int clientID = ofToInt(response.substr(1,1));
		connections[clientID]->started = false;
		connections[clientID]->ready = false;
		send("X" + ofToString(clientID));
		allconnected = false;
			//numConnectedClients = 0;
			currentFrame = 0;
			numConnectedClients--;
			//shouldTriggerFrame = false;
		//setDefaults();
		//restartServer();

		}
		break;
	}
}

void syncserver::read(string response,int i) {
	
	out("Receiving: " + response);
	char first = response.at(0);
	int clientID = ofToInt(response.substr(1,1));

	switch(first)
	{
	case 'S':
		{
					//that's the start!
			//int clientID = ofToInt(response.substr(1,1));
			if(clientID < numExpectedClients){
					vector<string> info = ofSplitString(response, ",");
					//EnterCriticalSection(&criticalSection);
					connections[clientID]->serverIndex = clientID;
					connections[clientID]->started = true;
					//LeaveCriticalSection(&criticalSection);
//					connections[clientID].name = info[1];
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
				//int clientID = ofToInt(info[1]);
				int fc = ofToInt(info[1]);
				if(fc == currentFrame-1){
				   	//todo validate client id
					connections[clientID]->ready = true;
					EnterCriticalSection(&criticalSection);
					
					//cvCircle(connections[clientID]->blobImage.getCvImage(),cvPoint(320,240),70,cvScalar(255,255,255),-1);
					//image_fill(connections[clientID]->test,0);
					LeaveCriticalSection(&criticalSection);
					shouldContinue();
					
				}
				if(ofToInt(info[2])==0)
				{
					cout<<"in black loop"<<endl;
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
		
			//cout<<"X:"<<temp.x;
			//cout<<"y"<<temp.y<<endl;
			EnterCriticalSection(&criticalSection);
			connections[clientID]->points.push_back(temp);
			//cvCircle(connections[clientID]->blobImage.getCvImage(),cvPoint(320,240),30,cvScalar(255,255,255),-1);
			//image_fill(connections[clientID]->test,0);
			
			LeaveCriticalSection(&criticalSection);
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
				//cvCircle(connections[clientID]->blobImage.getCvImage(),cvPoint((*it).centroid.x ,(*it).centroid.y),10,cvScalar(255,255,255),-1);
			//	draw_circle (connections[clientID]->test, 10,(*it).x,(*it).y, 0xff);
			//	cvDrawContours(connections[clientID]->blobImage.getCvImage(),(*it).contours,cvScalar(255,255,255),cvScalar(255,255,255),0,-1);
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
		//ofSleepMillis(150);
		//ofSleepMillis(550);
		//send("X");
		//ofSleepMillis(500);
		
		//int clientID = ofToInt(response.substr(1,1));
		send("X" + ofToString(clientID));
		//out("Diconnect all clients"+ofToString(i));
	//	for(int i = 0; i < numExpectedClients; i++){
			//tcpServer.disconnectClient(clientID);
			//EnterCriticalSection(&criticalSection);
			//connections[clientID]->started = false;
			connections[clientID]->started = false;
			connections[clientID]->ready = false;
			allconnected = false;
			currentFrame=0;
			numConnectedClients--;
			//LeaveCriticalSection(&criticalSection);
		//}
		
		//setDefaults();
		//restartServer();
	}
	break;
}
	//else{
		//EnterCriticalSection(&criticalSection);
		//connections[i]->started = false;
		//connections[i]->ready =false;
		//LeaveCriticalSection(&criticalSection);
	//}
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
	
    CloseHandle(serverThread);
}

void syncserver::restartServer(){
	out("Restarting Server");
	isServerThreadRunning = false;
	if(bTCP){
	tcpServer.close();
	//Sleep(200);
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

void syncserver::image_set_pixel (unsigned char *data, size_t x, size_t y, unsigned char  value)
{
  size_t tx, ty;
  
 //cout<<"value of tc"<<tx;
	//tx=0;
  tx = 0+x;
  ty = 0+y;
  if(tx<0)
  {tx=0;
  ty=0;}
 *(data + (ty * 320) + tx)=value;
 // p = data + (ty * 320) + tx;
  //*p = value;
}

void syncserver::draw_circle (unsigned char *data,int radius,int p1,int p2, unsigned char  value)
{
  int x, y;

  for (y = -radius; y <= radius; y++)
    for (x = -radius; x <= radius; x++)
      if ((x * x) + (y * y) <= (radius * radius))
        image_set_pixel (data, x+p1, y+p2, value);
}

//void syncserver::image_fill(unsigned char *data, unsigned char  value)
//{
	// memset (data, value, 320*240);
//}