#include "ofxIPImage.h"

ofxIPImage::ofxIPImage(){
	servercopy = NULL;
	cameraFrame = 0;
	settingsGUIThread = NULL;
	isSettedDefaultSettings = false;

}

ofxIPImage::~ofxIPImage()
{
	deinitializeCamera();
	
}

void ofxIPImage::callSettingsDialog()
{
	settingsGUIThread = NULL;
	isSettedDefaultSettings = false;
	if (settingsGUIThread == NULL)
		settingsGUIThread = CreateThread(NULL, 0, &ofxIPImage::SettingsThread, this, 0, 0);
}

DWORD WINAPI ofxIPImage::SettingsThread(LPVOID instance)
{
	ofxIPImage *pThis = (ofxIPImage*)instance;
	pThis->StartSettingsDialog();
	pThis->settingsGUIThread = NULL;
	pThis->isSettedDefaultSettings = false;
	return 0;
}

void ofxIPImage::StartSettingsDialog()
{
   HWND        hwnd;
   MSG         msg;
   WNDCLASS    wndclass;
   char        szAppName[64] = "IPImage settings: ";
   strcat(szAppName,GUIDToString(guid).c_str());
   wndclass.style         = 0;
   wndclass.lpfnWndProc   = ofxIPImage::WndProc;
   wndclass.cbClsExtra    = 0;
   wndclass.cbWndExtra    = 0;
   HMODULE hInstance;
   GetModuleHandleEx(0,NULL,&hInstance);
   wndclass.hInstance     = hInstance;
   wndclass.hIcon         = LoadIcon(hInstance, szAppName);
   wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
   wndclass.lpszMenuName  = szAppName;
   wndclass.lpszClassName = szAppName;
   RegisterClass(&wndclass);

   InitCommonControls();

   hwnd = CreateWindow(szAppName,
      szAppName,
      DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
	  0, 0, 415, 120,
      NULL, NULL, hInstance, 0);
   SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)(this));
   while (GetMessage(&msg, NULL, 0, 0)) 
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }
}

LRESULT CALLBACK ofxIPImage::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) 
{
	static CREATESTRUCT   *cs;
	static HWND	id,connected,running;
	
	ofxIPImage* camera = (ofxIPImage*) GetWindowLongPtr(hwnd,GWLP_USERDATA);
	if (camera!=NULL)
	{
		if (!camera->isSettedDefaultSettings)
		{
			camera->isSettedDefaultSettings = true;
			int firstValue, secondValue, minValue, maxValue;
			bool isEnabled,isAuto;
			camera->getCameraFeature(BASE_ID,&firstValue,&secondValue,&isAuto,&isEnabled,&minValue,&maxValue);
			SendMessage(GetDlgItem(hwnd, 0), TBM_SETPOS, TRUE, firstValue);
			string id1 = ofToString(firstValue);
			cout<<"valueof id:"<<id1<<endl;
			SetWindowTextA(id,id1.c_str());

			camera->getCameraFeature(BASE_CONNECTED,&firstValue,&secondValue,&isAuto,&isEnabled,&minValue,&maxValue);
			SendMessage(GetDlgItem(hwnd, 1), TBM_SETPOS, TRUE, firstValue);
			SetWindowTextA(connected,firstValue ? "Connected" : "Not Connected");

			camera->getCameraFeature(BASE_CONNECTED,&firstValue,&secondValue,&isAuto,&isEnabled,&minValue,&maxValue);
			SendMessage(GetDlgItem(hwnd, 2), TBM_SETPOS, TRUE, firstValue);
			SetWindowTextA(running,firstValue ? "Running" : "Not Running");

			
			
			
		}
	}

	switch (iMsg) 
    {
		case WM_CREATE:
			{
				CreateWindow("STATIC", "Client ID:", SS_LEFT | WS_CHILD | WS_VISIBLE, 
                                    10, 10, 140, 20, hwnd, NULL, NULL, NULL);
				id = CreateWindow("static", " ",SS_LEFT | WS_CHILD | WS_VISIBLE,
										70, 10, 145, 25,hwnd,(HMENU)0, NULL, NULL);
				CreateWindow("STATIC", "Connection Status:", SS_LEFT | WS_CHILD | WS_VISIBLE, 
                                    10, 30, 140, 20, hwnd, NULL, NULL, NULL);
				connected = CreateWindow("static", " ",SS_LEFT | WS_CHILD | WS_VISIBLE,
										135, 30, 145, 25,hwnd,(HMENU)1, NULL, NULL);
				CreateWindow("STATIC", "Running Status:", SS_LEFT | WS_CHILD | WS_VISIBLE, 
                                    10, 50, 140, 20, hwnd, NULL, NULL, NULL);
				running = CreateWindow("static", " ",SS_LEFT | WS_CHILD | WS_VISIBLE,
										120, 50, 145, 25,hwnd,(HMENU)2, NULL, NULL);
				
			}
			break;
		case WM_CLOSE :
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY :
			PostQuitMessage(0);
			break;
   }
   return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

CAMERA_BASE_FEATURE* ofxIPImage::getSupportedFeatures(int* featuresCount)
{
	*featuresCount = 2;
	CAMERA_BASE_FEATURE* features = (CAMERA_BASE_FEATURE*)malloc(*featuresCount * sizeof(CAMERA_BASE_FEATURE));
	features[0] = BASE_IP;
	features[1] = BASE_ID;
	features[2] = BASE_CONNECTED;
	features[3] = BASE_RUNNING;
	return features;
}

int ofxIPImage::getCameraBaseCount()
{
	ofxXmlSettings* xml = new ofxXmlSettings();
	 if (xml->loadFile("xml/TCPSyncServer.xml"))
        cout<<"ERROR loading XML file!";
	else
		cout<<"Xml Loaded successfully";
	int num = xml->getValue("settings:numclients", 1, 0);
	cout<<"clients"<<num;
	delete xml;
	xml = NULL;
	return num;
	
}

GUID* ofxIPImage::getBaseCameraGuids(int* camCount)
{
	*camCount = getCameraBaseCount();
	GUID* guids = (GUID*)malloc(*camCount * sizeof(GUID));
	for (int i=0;i<*camCount;i++)
	{
		GUID tempGuid;
		tempGuid.Data1 = i;
		tempGuid.Data2 = tempGuid.Data3 = 0;
		memset(tempGuid.Data4,0,8*sizeof(unsigned char));
		guids[i] = tempGuid;
	}
	return guids;
}

void ofxIPImage::setCameraType()
{
	cameraType = IPIMAGE;
	cameraTypeName = "IPIMAGE";
}

void ofxIPImage::setCameraFeature(CAMERA_BASE_FEATURE featureCode,int firstValue,int secondValue,bool isAuto,bool isEnabled)
{
	
}

void ofxIPImage::getCameraFeature(CAMERA_BASE_FEATURE featureCode,int* firstValue,int* secondValue, bool* isAuto, bool* isEnabled,int* minValue,int* maxValue)
{
	switch (featureCode)
	{
		case BASE_CONNECTED:
			*firstValue = servercopy->connections[guid.Data1]->started;
			*isEnabled = true;
			*secondValue = 0;
			*minValue = 0;
			*maxValue = 1;
			*isAuto = false;
			break;
		case BASE_ID:
			*firstValue = servercopy->connections[guid.Data1]->serverIndex;
			*isEnabled = true;
			*secondValue = 0;
			*minValue = 0;
			*maxValue = 100;
			*isAuto = true;
			break;
		case BASE_RUNNING:
			*firstValue = servercopy->connections[guid.Data1]->ready;
			*isEnabled = true;
			*secondValue = 0;
			*minValue = 0;
			*maxValue = 1;
			*isAuto = true;
			break;
	}
}

void ofxIPImage::getNewFrame(unsigned char* newFrame)
{	 
	
	//cout<<"thread run"<<endl;
	//cout<<"Number of Points"<<servercopy->connections.size()<<endl;
	

	
		
	//memcpy((void*)newFrame,(unsigned char*)image.getPixels(),320*240*3*sizeof(unsigned char));


	
	/*cvRectangle(connections[0].blobImage.getCvImage(),cvPoint(0,0),cvPoint(320,240),cvScalar(0,0,0),-1);
	memcpy((void*)newFrame,(unsigned char*)connections[0].blobImage.getPixels(),320*240*3*sizeof(unsigned char));
	if(connections[0].ready)
	memcpy((void*)newFrame,(unsigned char*)connections[0].blobImage.getPixels(),320*240*3*sizeof(unsigned char));*/
	//getPixels(guid.Data1,newFrame);
	
	//ofxCvColorImage test;
	//image.allocate(320,240);
	//cout<<"points value"<<connections[0]->points.size()<<endl;
	//cout<<"GUID number"<<guid.Data1<<endl;
	//EnterCriticalSection(&criticalSection); 
	//cvRectangle(image.getCvImage(),cvPoint(0,0),cvPoint(320,240),cvScalar(0,0,0),-1);
	//LeaveCriticalSection(&criticalSection); 
	//if(servercopy->connections[guid.Data1]->points.size()!=0)
	//if(z%2!=0)
	//{
		//cvRectangle(image.getCvImage(),cvPoint(0,0),cvPoint(320,240),cvScalar(0,0,0),-1);
		//for(std::vector<ofPoint>::iterator it = connections[guid.Data1]->points.begin();it != connections[guid.Data1]->points.end();it++)
		//{
			//cvCircle(image.getCvImage(),cvPoint((*it).x ,(*it).y),30,cvScalar(255,255,255),-1);
		//cvCircle(image.getCvImage(),cvPoint(70 ,40),30,cvScalar(255,255,255),-1);
		//}
		//cout<<"ENTER"<<endl;
		
	//}
	//else
	//{
		//cvRectangle(image.getCvImage(),	cvPoint(0,0),cvPoint(320,240),cvScalar(255,255,255),-1);
		//cvCircle(image.getCvImage(),cvPoint(70 ,40),30,cvScalar(0,0,0),-1);
		//cout<<"Outside"<<endl;
		
		/*cout<<"get new frae"<<endl;
		}
	if(servercopy->connections[guid.Data1]->points.size()!=0)
	{
		for(std::vector<ofPoint>::iterator it = servercopy->connections[guid.Data1]->points.begin();it != servercopy->connections[guid.Data1]->points.end();it++)
		{
			cout<<"Node:"<<guid.Data1<<"Points:"<<(*it).x<<","<<(*it).y<<endl;
		}
	}
	else
		cout<<"Node:"<<guid.Data1<<"No points recieved"<<endl;
			//cvCircle(image.getCvImage(),cvPoint((*it).x ,(*it).y),30,cvScalar(255,255,255),-1);
		//cvCircle(image.getCvImage(),cvPoint(70 ,40),30,cvScalar(255,255,255),-1);
		//}*/
	
	//if(z%2)
	//cout<<"check"<<servercopy->check;
	//if(servercopy->isServerThreadRunning)
	//{
	//grayImage = connections[
		//cvRectangle(grayImage.getCvImage(),	cvPoint(0,0),cvPoint(320,240),cvScalar(0,0,0),-1);
		//if(servercopy->connections[guid.Data1]->points.size()!=0)
		//{
		//	for(std::vector<ofPoint>::iterator it = servercopy->connections[guid.Data1]->points.begin();it != servercopy->connections[guid.Data1]->points.end();it++)
		//	{
		//		cvCircle(grayImage.getCvImage(),cvPoint((*it).x ,(*it).y),10,cvScalar(255,255,255),-1);
		//	}	
		//	servercopy->connections[guid.Data1]->points.clear();
		//}
	//}
	
	//image_fill(newFrame,0);
	//draw_circle (newFrame, 10,0,0, 0xff);
	
	//{
		//k=0;
		//image_fill(newFrame,0);
		//draw_circle (newFrame, 10,0,0, 0xff);
		//for(std::vector<ofPoint>::iterator it = servercopy->connections[guid.Data1]->points.begin();it != servercopy->connections[guid.Data1]->points.end();it++)
		//{	
		//	cout<<"for loop running"<<k<<"times";
	//		if((*it).x<160 && (*it).y<160)
	//		draw_circle (newFrame, 10,-(*it).x,-(*it).y, 0xff);
		//	else
		//	draw_circle (newFrame, 10,(*it).x-160,(*it).y-160, 0xff);
	//		k++;
	//	}
	//}
	
	//servercopy->connections[guid.Data1]->points.clear();
	

	
	memcpy((void*)newFrame,servercopy->connections[guid.Data1]->blobImage.getPixels(),320*240*1*sizeof(unsigned char));
	
}

void ofxIPImage::cameraInitializationLogic()
{
	//image.allocate(320,240);
	width = 320;
	height = 240;
	depth =1;
}

void ofxIPImage::cameraDeinitializationLogic()
{
	
}

