#include "ofxOpenCv.h"
//#include "Blob.h"


class blob{
	 public:
		 ofPoint             centroid;
		 CvSize				axes;
		 double  angle;
		 vector<CvPoint> pvect;
		 vector<vector<CvPoint> > contours;
		 
		 
		
};

class connection{
public:
	connection(){
		started =false;
		ready=false;
		height =240;
		width=240;
		depth =1;
		calibrate=false;
		//test.allocate(320,240,OF_IMAGE_COLOR);
		blobImage.allocate(320,240);	
	}
	bool started;
	bool ready;
	//string name;
	int serverIndex;
	bool calibrate;
	int	GRID_X;
	int	GRID_Y;
	int height;
	int width;
	int depth;
	//bool test;
	vector<blob> points;
	ofxCvGrayscaleImage blobImage;
	unsigned char * test;
	
	//std::vector<ofPoint> points;
	//int npnts;

} ;