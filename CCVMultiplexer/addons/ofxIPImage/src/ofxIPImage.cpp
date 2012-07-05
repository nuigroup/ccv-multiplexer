#include "ofxIPImage.h"

ofxIPImage::ofxIPImage(){
//	x=1;
	
	//fbo.allocate(320,240,GL_RGB);
	//cout<<"FBO Allocate"<<endl;
	/*height = 240;
	width=320;
	blobImage.allocate(width, height); //main Image that'll be processed.
	blackImage.allocate(width, height);
	blobImageBw.allocate(width, height);
	testImage.allocate(width, .height,OF_IMAGE_GRAYSCALE);
	blackPixels			= new unsigned char [width* height]; //empty the image*/
}

ofxIPImage::~ofxIPImage()
{
	deinitializeCamera();
//	stopThread();
}

void ofxIPImage::callSettingsDialog()
{
}

CAMERA_BASE_FEATURE* ofxIPImage::getSupportedFeatures(int* featuresCount)
{
	*featuresCount = 0;
	CAMERA_BASE_FEATURE* features = (CAMERA_BASE_FEATURE*)malloc(*featuresCount * sizeof(CAMERA_BASE_FEATURE));
	return features;
}

int ofxIPImage::getCameraBaseCount()
{
	int x=1;
	return x;
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
}

void ofxIPImage::getNewFrame(unsigned char* newFrame)
{	 //copy =getValue();
	//cout<<copy[0].size<<endl;
	//if(x==1){
	/*for(int i = 0; i < 1; i++){
		
		Connection c;
		c.started = false;
		c.ready = false;
		c.height =240;
		c.width = 320;
		c.depth =3;
		c.serverIndex = i;
		c.test=22;
		//c.blobImage.allocate(320,240);
		//c.name = "noname";
		connections.push_back(c);
	}
	}
	x++;*/



	//ofSleepMillis(120);
	/*cvRectangle(connections[0].blobImage.getCvImage(),cvPoint(0,0),cvPoint(320,240),cvScalar(0,0,0),-1);
	memcpy((void*)newFrame,(unsigned char*)connections[0].blobImage.getPixels(),320*240*3*sizeof(unsigned char));
	if(connections[0].ready)
	memcpy((void*)newFrame,(unsigned char*)connections[0].blobImage.getPixels(),320*240*3*sizeof(unsigned char));*/
	//getPixels(guid.Data1,newFrame);
	
	cout<<"IPIMAGE VALUE OF TEST"<<connections[0]->started<<endl;
	//connections[0].test = false;
}

void ofxIPImage::cameraInitializationLogic()
{
	
	
}

void ofxIPImage::cameraDeinitializationLogic()
{
}


