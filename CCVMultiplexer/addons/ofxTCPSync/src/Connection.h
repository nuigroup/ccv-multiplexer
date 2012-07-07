#include "ofxOpenCv.h"

class connection{
public:
	connection(){
		started =false;
		ready=false;
		height =240;
		width=240;
		depth =3;
		calibrate=false;
		test.allocate(320,240,OF_IMAGE_COLOR);
		//blobImage.allocate(320,240);	
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
	vector<ofPoint> points;
	ofxCvColorImage blobImage;
	ofImage test;
	//std::vector<ofPoint> points;
	//int npnts;

} ;