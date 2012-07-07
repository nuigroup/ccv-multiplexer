#include "ofxCameraBase.h"

void ofxCameraBase::getCameraSize(unsigned int* cameraWidth,unsigned int* cameraHeight,unsigned char* cameraDepth,unsigned char* pixelMode)
{
	if (!isInitialized)
	{
		*cameraWidth = *cameraHeight = *cameraDepth = *pixelMode = 0;
		return;
	}
	*cameraWidth = width;
	*cameraHeight = height;
	*cameraDepth = depth;
	*pixelMode = cameraPixelMode;
}

DWORD WINAPI ofxCameraBase::CaptureThread(LPVOID instance)
{
	ofxCameraBase *pThis = (ofxCameraBase*)instance;
	pThis->Capture();
	return 0;
}

void ofxCameraBase::StartThreadingCapture()
{
	InitializeCriticalSection(&criticalSection);
	isCaptureThreadRunning = true;
	captureThread = CreateThread(NULL, 0, &ofxCameraBase::CaptureThread, this, 0, 0);
}

void ofxCameraBase::StopThreadingCapture()
{
	isCaptureThreadRunning = false;
	Sleep(300);
}


void ofxCameraBase::Capture()
{
	while (isCaptureThreadRunning)
	{
		if (isPaused)
		{
			isNewFrame = true;
			Sleep(30);
		}
		else
		{
			if ((!isNewFrame) || (newFrameCurrentLifetime>=_MAX_FRAME_LIFETIME_))
			{
				isNewFrame = true;
				newFrameCurrentLifetime = 0;
				if (!isInitialized)
					return;
				EnterCriticalSection(&criticalSection); 
				updateCurrentFrame();
				LeaveCriticalSection(&criticalSection);
			}
			else
				newFrameCurrentLifetime++;
			Sleep(200);
		}
	}
}

void ofxCameraBase::getCameraFrame(unsigned char* newFrameData)	
{ 
	if (isNewFrame)
	{
		
		EnterCriticalSection(&criticalSection); 
		isNewFrame = false;
		memcpy((void*)newFrameData,cameraFrame,width*height*sizeof(unsigned char));
		LeaveCriticalSection(&criticalSection);
		//Sleep(200);
	}
}

void ofxCameraBase::initializeWithGUID(GUID cameraGuid)
{
	guid = cameraGuid;
	setCameraType();
	loadCameraSettings();
	cameraInitializationLogic();
	cameraFrame = (unsigned char*)malloc(width*height*sizeof(unsigned char));
	rawCameraFrame = (unsigned char*)malloc(depth*width*height*sizeof(unsigned char));
	isInitialized = true;
}

void ofxCameraBase::startCamera()
{
	StartThreadingCapture();
}

void ofxCameraBase::deinitializeCamera()
{
	if (isInitialized)
	{
		saveCameraSettings();
		StopThreadingCapture();
		EnterCriticalSection(&criticalSection);
		cameraDeinitializationLogic();
		isInitialized = false;
		LeaveCriticalSection(&criticalSection);
		Sleep(100);
		//quit();
		DeleteCriticalSection(&criticalSection);
		free(cameraFrame);
		free(rawCameraFrame);
		isInitialized = false;
	}
}

void ofxCameraBase::updateCurrentFrame()
{
	
	getNewFrame(depth>1 ? rawCameraFrame : cameraFrame);
	
	if (depth>1)
	{
		int size = width*height;
		if (isRaw)
		{
			//RGGB
			int x,y,index;
			float r = 0.0f;
			switch (isRaw)
			{
			case 1:		//RGGB
				for (y=0;y<height;y++)
				{
					for (x=0;x<width;x++)
					{
						index = x + y * width;
						if (x % 2)
						{
							if (y % 2)
								r = ((float)rawCameraFrame[(x-1+(y-1)*width)*depth] + rawCameraFrame[(x-1+(y+1)*width)*depth] + rawCameraFrame[(x+1+(y-1)*width)*depth] + rawCameraFrame[(x+1+(y+1)*width)*depth]) * 0.25f;
							else
								r = ((float)rawCameraFrame[(x-1+y*width)*depth] + rawCameraFrame[(x+1+y*width)*depth]) * 0.5f;
						}
						else
						{
							if (y % 2)
								r = ((float)rawCameraFrame[(x+(y-1)*width)*depth] + rawCameraFrame[(x+(y+1)*width)*depth]) * 0.5f;
							else
								r = rawCameraFrame[(x+y*width)*depth];
						}
						cameraFrame[index] = (unsigned char)r;
					}
				}
				break;
			case 2:		//GRBG
				for (y=0;y<height;y++)
				{
					for (x=0;x<width;x++)
					{
						index = x + y * width;
						if (x % 2)
						{
							if (y % 2)
								r = ((float)rawCameraFrame[(x+(y-1)*width)*depth] + rawCameraFrame[(x+(y+1)*width)*depth]) * 0.5f;
							else
								r = rawCameraFrame[(x+y*width)*depth];
						}
						else
						{
							if (y % 2)
							{
								if (x != 0)
									r = ((float)rawCameraFrame[(x-1+(y-1)*width)*depth] + rawCameraFrame[(x-1+(y+1)*width)*depth] + rawCameraFrame[(x+1+(y-1)*width)*depth] + rawCameraFrame[(x+1+(y+1)*width)*depth]) * 0.25f;
								else
									r = ((float)rawCameraFrame[(1+(y-1)*width)*depth] + rawCameraFrame[(1+(y+1)*width)*depth]) * 0.5f;
							}
							else
							{
								if (x != 0)
									r = ((float)rawCameraFrame[(x-1+y*width)*depth] + rawCameraFrame[(x+1+y*width)*depth]) * 0.5f;
								else
									r = rawCameraFrame[(1+y*width)*depth];
							}
						}
					}
					cameraFrame[index] = (unsigned char)r;
				}
				break;
			case 3: //GBRG
				for (y=0;y<height;y++)
				{
					for (x=0;x<width;x++)
					{
						index = x + y * width;
						if (x % 2)
						{
							if (y % 2)
								r = ((float)rawCameraFrame[(x-1+y*width)*depth] + rawCameraFrame[(x+1+y*width)*depth]) * 0.5f;
							else
								if (y!=0)
									r = ((float)rawCameraFrame[(x-1+(y-1)*width)*depth] + rawCameraFrame[(x-1+(y+1)*width)*depth] + rawCameraFrame[(x+1+(y-1)*width)*depth] + rawCameraFrame[(x+1+(y+1)*width)*depth]) * 0.25f;
								else
									r = ((float)rawCameraFrame[(x+1+width)*depth] + rawCameraFrame[(x-1+width)*depth]) * 0.5f;
						}
						else
						{
							if (y % 2)
								r = rawCameraFrame[(x+y*width)*depth];
							else
								if (y!=0)
									r = ((float)rawCameraFrame[(x+(y-1)*width)*depth] + rawCameraFrame[(x+(y+1)*width)*depth]) * 0.5f;
								else
									r = rawCameraFrame[(x+width)*depth];
								
						}
						cameraFrame[index] = (unsigned char)r;
					}
				}
				break;
			case 4:		//BGGR
				for (y=0;y<height;y++)
				{
					for (x=0;x<width;x++)
					{
						index = x + y * width;
						if (x % 2)
						{
							if (y % 2)
								r = rawCameraFrame[(x+y*width)*depth];
							else
								if (y!=0)
									r = ((float)rawCameraFrame[(x+(y-1)*width)*depth] + rawCameraFrame[(x+(y+1)*width)*depth]) * 0.5f;
								else
									r = rawCameraFrame[(x+width)*depth];
						}
						else
						{
							if (y % 2)
								if (x != 0)
									r = ((float)rawCameraFrame[(x-1+y*width)*depth] + rawCameraFrame[(x+1+y*width)*depth]) * 0.5f;
								else
									r = rawCameraFrame[(1+y*width)*depth];
							else
								if (x!=0)
									if (y!=0)
										r = ((float)rawCameraFrame[(x-1+(y-1)*width)*depth] + rawCameraFrame[(x-1+(y+1)*width)*depth] + rawCameraFrame[(x+1+(y-1)*width)*depth] + rawCameraFrame[(x+1+(y+1)*width)*depth]) * 0.25f;
									else
										r = ((float)rawCameraFrame[(x-1+width)*depth] + rawCameraFrame[(x+1+width)*depth]) * 0.5f;
								else
									if (y!=0)
										r = ((float)rawCameraFrame[(1+width*(y-1))*depth] + rawCameraFrame[(1+width*(y+1))*depth]) * 0.5f;
									else
										r = rawCameraFrame[(1+width)*depth];
						}
						cameraFrame[index] = (unsigned char)r;
					}
				}
				break;
			}	
		}
		else
		{
			for (int i=0;i<size;i++)
				cameraFrame[i] = (unsigned char)(0.3f*(float)rawCameraFrame[i*depth+2] + 0.59f*(float)rawCameraFrame[i*depth+1] +0.11f*(float)rawCameraFrame[i*depth]) ;
		}
	}
}


void ofxCameraBase::loadCameraSettings(ofxXmlSettings* xmlSettings)
{
	width = xmlSettings->getValue("SETTINGS:FRAME:WIDTH", -1);
	height = xmlSettings->getValue("SETTINGS:FRAME:HEIGHT", -1);
	isRaw = xmlSettings->getValue("SETTINGS:FRAME:RAW", 0);
	left = xmlSettings->getValue("SETTINGS:FRAME:LEFT", -1);
	top = xmlSettings->getValue("SETTINGS:FRAME:TOP", -1);
	cameraPixelMode = (unsigned char)(xmlSettings->getValue("SETTINGS:SENSOR:MODE", 0));
	depth = xmlSettings->getValue("SETTINGS:SENSOR:DEPTH", 1);
	bool isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:BRIGHTNESS:AUTO", 1) != 0;
	int settingValue = xmlSettings->getValue("SETTINGS:SENSOR:BRIGHTNESS:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_BRIGHTNESS);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:FRAMERATE", 30);
	if (settingValue>0)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_FRAMERATE);
		framerate = settingValue;
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:GAIN:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:GAIN:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_GAIN);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:EXPOSURE:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:EXPOSURE:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_EXPOSURE);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:SHARPNESS:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:SHARPNESS:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_SHARPNESS);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:WHITEBALANCE:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:WHITEBALANCE:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		int isFoundPropertyIndex = -1;
		for (int k=0;k<cameraBaseSettings->propertyType.size();k++)
		{
			if (cameraBaseSettings->propertyType[k] == BASE_WHITE_BALANCE)
			{
				isFoundPropertyIndex = k;
				break;
			}
		}
		if (isFoundPropertyIndex>0)
		{
			cameraBaseSettings->propertySecondValue[isFoundPropertyIndex] = settingValue;
		}
		else
		{
			cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
			cameraBaseSettings->isPropertyOn.push_back(true);
			cameraBaseSettings->propertyFirstValue.push_back(settingValue);
			cameraBaseSettings->propertySecondValue.push_back(0);
			cameraBaseSettings->propertyType.push_back(BASE_WHITE_BALANCE);
		}
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:BLACKBALANCE:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:BLACKBALANCE:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		int isFoundPropertyIndex = -1;
		for (int k=0;k<cameraBaseSettings->propertyType.size();k++)
		{
			if (cameraBaseSettings->propertyType[k] == BASE_WHITE_BALANCE)
			{
				isFoundPropertyIndex = k;
				break;
			}
		}
		if (isFoundPropertyIndex>0)
		{
			cameraBaseSettings->propertyFirstValue[isFoundPropertyIndex] = settingValue;
		}
		else
		{
			cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
			cameraBaseSettings->isPropertyOn.push_back(true);
			cameraBaseSettings->propertyFirstValue.push_back(0);
			cameraBaseSettings->propertySecondValue.push_back(settingValue);
			cameraBaseSettings->propertyType.push_back(BASE_WHITE_BALANCE);
		}		
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:HUE:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:HUE:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_HUE);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:SATURATION:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:SATURATION:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_SATURATION);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:GAMMA:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:GAMMA:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_GAMMA);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:SHUTTER:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:SHUTTER:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_SHUTTER);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:IRIS:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:IRIS:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_IRIS);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:FOCUS:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:FOCUS:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_FOCUS);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:ZOOM:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:ZOOM:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_ZOOM);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:PAN:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:PAN:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_PAN);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:MOTORPOSITION", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_MOTOR_POSITION);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:MOTORLED", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_MOTOR_LED);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:DEPTHNEARBOUND", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(DEPTH_NEAR_BOUND);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:DEPTHFARBOUND", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(DEPTH_FAR_BOUND);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:CONTRAST:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:CONTRAST:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_CONTRAST);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:TILT:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:TILT:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_TILT);
	}
	isAutoValue = xmlSettings->getValue("SETTINGS:SENSOR:ROLL:AUTO", 1) != 0;
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:ROLL:VALUE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(isAutoValue);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_ROLL);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:HFLIP", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_HFLIP);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:VFLIP", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_VFLIP);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:HKEYSTONE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_HKEYSTONE);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:VKEYSTONE", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_VKEYSTONE);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:XOFFSET", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_XOFFSET);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:YOFFSET", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_YOFFSET);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:LENSCORRECTION1", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_LENSCORRECTION1);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:LENSCORRECTION2", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_LENSCORRECTION2);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:LENSCORRECTION3", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_LENSCORRECTION3);
	}
	settingValue = xmlSettings->getValue("SETTINGS:SENSOR:LENSBRIGHTNESS", -0xFFFF);
	if (settingValue>-0xFFFF)
	{
		cameraBaseSettings->isPropertyAuto.push_back(false);
		cameraBaseSettings->isPropertyOn.push_back(true);
		cameraBaseSettings->propertyFirstValue.push_back(settingValue);
		cameraBaseSettings->propertySecondValue.push_back(0);
		cameraBaseSettings->propertyType.push_back(BASE_LENSBRIGHTNESS);
	}
}

void ofxCameraBase::loadCameraSettings()
{
	std::string fileName = "xml/camera_settings.xml";
	bool isLoadedFromXML = false;
	cameraBaseSettings = new ofxCameraBaseSettings();
 	cameraBaseSettings->camera = this;
	ofxXmlSettings* xmlSettings = new ofxXmlSettings();
	if (xmlSettings->loadFile(fileName))
	{
		xmlSettings->pushTag("CAMERAS", 0);
		int numCameraTags = xmlSettings->getNumTags("CAMERA");
		for (int i=0;i<numCameraTags;i++)
		{
			xmlSettings->pushTag("CAMERA", i);
			std::string xmlCameraType = xmlSettings->getValue("SETTINGS:TYPE","NONE");
			if (xmlCameraType == cameraTypeName)
			{
				GUID tempGUID = StringToGUID(xmlSettings->getValue("SETTINGS:GUID", "{00000000-0000-0000-0000-000000000000}"));
				if (tempGUID == guid)
				{
					isLoadedFromXML = true;
					loadCameraSettings(xmlSettings);
				}
			}
			xmlSettings->popTag();
		}
		xmlSettings->popTag();
	}
	delete xmlSettings;
	xmlSettings = NULL;
	if (!isLoadedFromXML)
		loadDefaultCameraSettings();
}

void ofxCameraBase::loadDefaultCameraSettings()
{
	std::string fileName = "xml/default_settings.xml";
	ofxXmlSettings* xmlSettings = new ofxXmlSettings();
	if (xmlSettings->loadFile(fileName))
	{
		xmlSettings->pushTag("CAMERAS", 0);
		int numCameraTags = xmlSettings->getNumTags("CAMERA");
		for (int i=0;i<numCameraTags;i++)
		{
			xmlSettings->pushTag("CAMERA", i);
			std::string xmlCameraType = xmlSettings->getValue("SETTINGS:TYPE","NONE");
			if (xmlCameraType == cameraTypeName)
				loadCameraSettings(xmlSettings);
			xmlSettings->popTag();
		}
		xmlSettings->popTag();
	}
	delete xmlSettings;
	xmlSettings = NULL;
}

void ofxCameraBase::receiveSettingsFromCamera()
{
	int featuresCount = 0;
	if (cameraBaseSettings == NULL)
	{
		cameraBaseSettings = new ofxCameraBaseSettings();
		cameraBaseSettings->camera = this;
	}
	CAMERA_BASE_FEATURE* supportedFeatures = getSupportedFeatures(&featuresCount);
	cameraBaseSettings->isPropertyAuto.clear();
	cameraBaseSettings->isPropertyOn.clear();
	cameraBaseSettings->propertyFirstValue.clear();
	cameraBaseSettings->propertySecondValue.clear();
	cameraBaseSettings->propertyType.clear();
	int firstValue,secondValue;
	bool isAuto,isEnabled;
	int maxValue,minValue;
	for (int i=0;i<featuresCount;i++)
	{
		getCameraFeature(supportedFeatures[i],&firstValue,&secondValue,&isAuto,&isEnabled,&minValue,&maxValue);
		bool isNewFeature = true;
		for (int j=0;j<cameraBaseSettings->propertyType.size();j++)
		{
			if ((cameraBaseSettings->propertyType[j] == supportedFeatures[i]) && (isEnabled && (firstValue >-0xFFFF) && (secondValue>-0xFFFF)))
			{
				isNewFeature = false;
				cameraBaseSettings->isPropertyAuto[j] = isAuto;
				cameraBaseSettings->isPropertyOn[j] = isEnabled;
				cameraBaseSettings->propertyFirstValue[j] = firstValue;
				cameraBaseSettings->propertySecondValue[j] = secondValue;
				break;
			}
		}
		if (isNewFeature)
		{
			if (isEnabled && (firstValue >-0xFFFF) && (secondValue>-0xFFFF))
			{
				cameraBaseSettings->propertyType.push_back(supportedFeatures[i]);
				cameraBaseSettings->isPropertyOn.push_back(isEnabled);
				cameraBaseSettings->isPropertyAuto.push_back(isAuto);
				cameraBaseSettings->propertyFirstValue.push_back(firstValue);
				cameraBaseSettings->propertySecondValue.push_back(secondValue);
			}
		}
	}
}

void ofxCameraBase::saveCameraSettings()
{
	receiveSettingsFromCamera();
	std::string fileName = "xml/camera_settings.xml";
	bool isLoadedFromXML = false;
	ofxXmlSettings* xmlSettings = new ofxXmlSettings();
	if (xmlSettings->loadFile(fileName))
	{
		xmlSettings->pushTag("CAMERAS", 0);
		int numCameraTags = xmlSettings->getNumTags("CAMERA");
		for (int i=0;i<numCameraTags;i++)
		{
			xmlSettings->pushTag("CAMERA", i);
			std::string xmlCameraType = xmlSettings->getValue("SETTINGS:TYPE","NONE");
			if (xmlCameraType == cameraTypeName)
			{
				GUID tempGUID = StringToGUID(xmlSettings->getValue("SETTINGS:GUID", "{00000000-0000-0000-0000-000000000000}"));
				if (tempGUID == guid)
				{
					isLoadedFromXML = true;
					xmlSettings->setValue("SETTINGS:GUID", GUIDToString(guid));
					xmlSettings->setValue("SETTINGS:TYPE", cameraTypeName);
					xmlSettings->setValue("SETTINGS:FRAME:RAW", isRaw);
					xmlSettings->setValue("SETTINGS:FRAME:WIDTH", (int)width);
					xmlSettings->setValue("SETTINGS:FRAME:HEIGHT", (int)height);
					xmlSettings->setValue("SETTINGS:FRAME:LEFT", (int)left);
					xmlSettings->setValue("SETTINGS:FRAME:TOP", (int)top);
					xmlSettings->setValue("SETTINGS:SENSOR:MODE", (int)cameraPixelMode);
					xmlSettings->setValue("SETTINGS:SENSOR:DEPTH", (int)depth);
					
					for (int j=0;j<cameraBaseSettings->propertyType.size();j++)
					{
						switch (cameraBaseSettings->propertyType[j])
						{
							case BASE_BRIGHTNESS:
								xmlSettings->setValue("SETTINGS:SENSOR:BRIGHTNESS:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:BRIGHTNESS:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_EXPOSURE:
								xmlSettings->setValue("SETTINGS:SENSOR:EXPOSURE:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:EXPOSURE:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_SHARPNESS:
								xmlSettings->setValue("SETTINGS:SENSOR:SHARPNESS:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:SHARPNESS:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_HUE:
								xmlSettings->setValue("SETTINGS:SENSOR:HUE:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:HUE:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_SATURATION:
								xmlSettings->setValue("SETTINGS:SENSOR:SATURATION:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:SATURATION:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_GAMMA:
								xmlSettings->setValue("SETTINGS:SENSOR:GAMMA:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:GAMMA:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_SHUTTER:
								xmlSettings->setValue("SETTINGS:SENSOR:SHUTTER:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:SHUTTER:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_GAIN:
								xmlSettings->setValue("SETTINGS:SENSOR:GAIN:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:GAIN:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_IRIS:
								xmlSettings->setValue("SETTINGS:SENSOR:IRIS:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:IRIS:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_FOCUS:
								xmlSettings->setValue("SETTINGS:SENSOR:FOCUS:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:FOCUS:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_ZOOM:
								xmlSettings->setValue("SETTINGS:SENSOR:ZOOM:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:ZOOM:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_PAN:
								xmlSettings->setValue("SETTINGS:SENSOR:PAN:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:PAN:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_FRAMERATE:
								xmlSettings->setValue("SETTINGS:SENSOR:FRAMERATE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_MOTOR_POSITION:
								xmlSettings->setValue("SETTINGS:SENSOR:MOTORPOSITION", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_MOTOR_LED:
								xmlSettings->setValue("SETTINGS:SENSOR:MOTORLED", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case DEPTH_NEAR_BOUND:
								xmlSettings->setValue("SETTINGS:SENSOR:DEPTHNEARBOUND", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case DEPTH_FAR_BOUND:
								xmlSettings->setValue("SETTINGS:SENSOR:DEPTHFARBOUND", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_CONTRAST:
								xmlSettings->setValue("SETTINGS:SENSOR:CONTRAST:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:CONTRAST:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_TILT:
								xmlSettings->setValue("SETTINGS:SENSOR:TILT:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:TILT:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_ROLL:
								xmlSettings->setValue("SETTINGS:SENSOR:ROLL:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:ROLL:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_WHITE_BALANCE:
								xmlSettings->setValue("SETTINGS:SENSOR:WHITEBALANCE:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:BLACKBALANCE:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
								xmlSettings->setValue("SETTINGS:SENSOR:WHITEBALANCE:VALUE", cameraBaseSettings->propertyFirstValue[j]);
								xmlSettings->setValue("SETTINGS:SENSOR:BLACKBALANCE:VALUE", cameraBaseSettings->propertySecondValue[j]);
								break;
							case BASE_HFLIP:
								xmlSettings->setValue("SETTINGS:SENSOR:HFLIP", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_VFLIP:
								xmlSettings->setValue("SETTINGS:SENSOR:VFLIP", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_HKEYSTONE:
								xmlSettings->setValue("SETTINGS:SENSOR:HKEYSTONE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_VKEYSTONE:
								xmlSettings->setValue("SETTINGS:SENSOR:VKEYSTONE", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_XOFFSET:
								xmlSettings->setValue("SETTINGS:SENSOR:XOFFSET", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_YOFFSET:
								xmlSettings->setValue("SETTINGS:SENSOR:YOFFSET", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_ROTATION:
								xmlSettings->setValue("SETTINGS:SENSOR:ROTATION", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_LENSCORRECTION1:
								xmlSettings->setValue("SETTINGS:SENSOR:LENSCORRECTION1", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_LENSCORRECTION2:
								xmlSettings->setValue("SETTINGS:SENSOR:LENSCORRECTION2", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_LENSCORRECTION3:
								xmlSettings->setValue("SETTINGS:SENSOR:LENSCORRECTION3", cameraBaseSettings->propertyFirstValue[j]);
								break;
							case BASE_LENSBRIGHTNESS:
								xmlSettings->setValue("SETTINGS:SENSOR:LENSBRIGHTNESS", cameraBaseSettings->propertyFirstValue[j]);
								break;
						}
					}
				}
			}
			xmlSettings->popTag();
		}
		xmlSettings->popTag();
	}
	if (!isLoadedFromXML)
	{
		xmlSettings->pushTag("CAMERAS", 0);
		int numCameraTags = xmlSettings->getNumTags("CAMERA");		
		xmlSettings->setValue("CAMERA","",numCameraTags);
		xmlSettings->pushTag("CAMERA", numCameraTags);
		xmlSettings->setValue("SETTINGS","",0);
		xmlSettings->pushTag("SETTINGS", 0);

		xmlSettings->setValue("GUID", GUIDToString(guid));
		xmlSettings->setValue("TYPE", cameraTypeName);
		xmlSettings->setValue("FRAME","",0);
		xmlSettings->pushTag("FRAME", 0);
		xmlSettings->setValue("RAW",isRaw);
		xmlSettings->setValue("WIDTH", (int)width);
		xmlSettings->setValue("HEIGHT", (int)height);
		xmlSettings->setValue("LEFT", (int)left);
		xmlSettings->setValue("TOP", (int)top);
		xmlSettings->popTag();
		xmlSettings->setValue("SENSOR","",0);
		xmlSettings->pushTag("SENSOR", 0);
		xmlSettings->setValue("MODE", (int)cameraPixelMode);
		xmlSettings->setValue("DEPTH", depth);
		for (int j=0;j<cameraBaseSettings->propertyType.size();j++)
		{
			switch (cameraBaseSettings->propertyType[j])
			{
				case BASE_BRIGHTNESS:
					xmlSettings->setValue("BRIGHTNESS:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("BRIGHTNESS:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_EXPOSURE:
					xmlSettings->setValue("EXPOSURE:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("EXPOSURE:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_SHARPNESS:
					xmlSettings->setValue("SHARPNESS:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("SHARPNESS:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_HUE:
					xmlSettings->setValue("HUE:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("HUE:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_SATURATION:
					xmlSettings->setValue("SATURATION:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("SATURATION:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_GAMMA:
					xmlSettings->setValue("GAMMA:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("GAMMA:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_SHUTTER:
					xmlSettings->setValue("SHUTTER:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("SHUTTER:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_GAIN:
					xmlSettings->setValue("GAIN:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("GAIN:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_IRIS:
					xmlSettings->setValue("IRIS:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("IRIS:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_FOCUS:
					xmlSettings->setValue("FOCUS:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("FOCUS:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_ZOOM:
					xmlSettings->setValue("ZOOM:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("ZOOM:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_PAN:
					xmlSettings->setValue("PAN:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("PAN:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_FRAMERATE:
					xmlSettings->setValue("FRAMERATE", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_MOTOR_POSITION:
					xmlSettings->setValue("MOTORPOSITION", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_MOTOR_LED:
					xmlSettings->setValue("MOTORLED", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case DEPTH_NEAR_BOUND:
					xmlSettings->setValue("DEPTHNEARBOUND", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case DEPTH_FAR_BOUND:
					xmlSettings->setValue("DEPTHFARBOUND", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_CONTRAST:
					xmlSettings->setValue("CONTRAST:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_TILT:
					xmlSettings->setValue("TILT:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("TILT:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_ROLL:
					xmlSettings->setValue("ROLL:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("ROLL:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_WHITE_BALANCE:
					xmlSettings->setValue("WHITEBALANCE:VALUE", cameraBaseSettings->propertyFirstValue[j]);
					xmlSettings->setValue("BLACKBALANCE:VALUE", cameraBaseSettings->propertySecondValue[j]);
					xmlSettings->setValue("WHITEBALANCE:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					xmlSettings->setValue("BLACKBALANCE:AUTO", cameraBaseSettings->isPropertyAuto[j] ? 1 :0);
					break;
				case BASE_HFLIP:
					xmlSettings->setValue("HFLIP", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_VFLIP:
					xmlSettings->setValue("VFLIP", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_HKEYSTONE:
					xmlSettings->setValue("HKEYSTONE", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_VKEYSTONE:
					xmlSettings->setValue("VKEYSTONE", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_XOFFSET:
					xmlSettings->setValue("XOFFSET", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_YOFFSET:
					xmlSettings->setValue("YOFFSET", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_ROTATION:
					xmlSettings->setValue("ROTATION", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_LENSCORRECTION1:
					xmlSettings->setValue("LENSCORRECTION1", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_LENSCORRECTION2:
					xmlSettings->setValue("LENSCORRECTION2", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_LENSCORRECTION3:
					xmlSettings->setValue("LENSCORRECTION3", cameraBaseSettings->propertyFirstValue[j]);
					break;
				case BASE_LENSBRIGHTNESS:
					xmlSettings->setValue("LENSBRIGHTNESS", cameraBaseSettings->propertyFirstValue[j]);
					break;
			}
		}
		xmlSettings->popTag();
		xmlSettings->popTag();
		xmlSettings->popTag();
		xmlSettings->popTag();
	}	
	xmlSettings->saveFile(fileName);
	delete xmlSettings;
	xmlSettings = NULL;
}




/*
void ofxCameraBase::serverSetup(string _fileString){
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
		//c->blobImage.allocate(320,240);
		//c->name = "noname";
		c->test=false;
		connections.push_back(c);
		
	}
	
}

void ofxCameraBase::loadServerSettings(string _fileString){
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





void ofxCameraBase::out(string _str) {
    print(_str);
}

void ofxCameraBase::print(string _str) {
    cout << "syncServer: " << _str << endl;
}

void ofxCameraBase::err(string _str) {
    cerr << "syncServer error: " << _str << endl;
}


void ofxCameraBase::startServer(){
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
	captureThread = CreateThread(NULL, 0, &ofxCameraBase::ServerThread, this, 0, 0);
}

DWORD WINAPI ofxCameraBase::ServerThread(LPVOID instance)
{
	ofxCameraBase *pThis = (ofxCameraBase*)instance;
	pThis->Server();
	return 0;
}

void ofxCameraBase::Server(){
	out("Running!");
        
	while (isServerThreadRunning) {
		//EnterCriticalSection(&criticalSection);
		//	cout<<"SYNCSERVER VALUE OF TEST"<<connections[0]->started<<endl;
		//	connections[0]->started=true;
		//	cvRectangle(connections[0]->blobImage.getCvImage(),cvPoint(0,0),cvPoint(320,240),cvScalar(0,0,0),-1);
	//	LeaveCriticalSection(&criticalSection);
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

void ofxCameraBase::shouldContinue(){
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

void ofxCameraBase::read(string response) {
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

void ofxCameraBase::read(string response,int i) {
	
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
			connections[clientID]->points.push_back(temp);
		//	cvCircle(connections[clientID]->blobImage.getCvImage(),cvPoint(320,240),30,cvScalar(255,255,255),-1);	
			
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

void ofxCameraBase::send(string _msg) {
    out("Sending: " + _msg);
   	if(bTCP)
	{
		if(allconnected)
		tcpServer.sendToAll(_msg);
	}
	else
	udpSender.Send(_msg.c_str(),_msg.length());
}

void ofxCameraBase::quit() {
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

void ofxCameraBase::restartServer(){
	out("Restarting TCP Server");
	tcpServer.close();
	isServerThreadRunning = false;
	//CloseHandle(serverThread);
	isServerThreadRunning = false;
	ofSleepMillis(3000);
	startServer();
	out("TCP Server Restart Complete!!!");
}
*/