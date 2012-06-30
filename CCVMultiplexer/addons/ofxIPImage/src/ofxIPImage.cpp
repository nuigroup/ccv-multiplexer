#include "ofxIPImage.h"

ofxIPImage::ofxIPImage(){
	cout<<"ofxIPImage constructor";
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
{
	ofImage test;
	//syncserver::getPixels(guid.Data1, newFrame);
	test.allocate(320,240,OF_IMAGE_COLOR_ALPHA);
	cout<<"new Frame called";
	fbo.clear();
	fbo.begin();
		//test.loadImage("car.jpg");
		ofCircle(10,10,50);
	fbo.end();
	
	

	test.setFromPixels((unsigned char*)fbo.getPixels(),320,240,OF_IMAGE_COLOR_ALPHA);
	test.saveImage("output.jpg");

	
	memcpy((void*)newFrame,test.getPixels(),320 * 240 * depth * sizeof(unsigned char));
	//for (int j = 0; j < 320*240; j++){
			//newFrame[j]=(unsigned char) (210);
		//	cout<<newFrame[j];
		//}
}

void ofxIPImage::cameraInitializationLogic()
{
	width =320;
	height =240;
	depth = 3;
	fbo.allocate(320,240,GL_RGB);
}

void ofxIPImage::cameraDeinitializationLogic()
{
	
}
