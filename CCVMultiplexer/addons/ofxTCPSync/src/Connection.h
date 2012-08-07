#include "ofxOpenCv.h"

class blob{
	 public:
		 ofPoint  centroid;
		 CvSize	  axes;
		 double  angle;
				
};

class connection{
public:
	connection(){
		started =false;
		ready=false;
		height =240;
		width=240;
		depth =1;
		blobImage.allocate(320,240);	
	}

	bool started;
	bool ready;
	int serverIndex;
	int height;
	int width;
	int depth;
	
	vector<blob> points;
	ofxCvGrayscaleImage blobImage;

} ;