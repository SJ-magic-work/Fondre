/************************************************************
************************************************************/
#include "ofApp.h"

#include <time.h>

/* for dir search */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h> 
#include <dirent.h>
#include <string>

using namespace std;


/************************************************************
************************************************************/
using namespace ofxCv;
using namespace cv;


/************************************************************
************************************************************/

/******************************
******************************/
ofApp::ofApp(int _Cam_id, bool _b_flipCamera)
: Cam_id(_Cam_id)
, b_flipCamera(_b_flipCamera)
, Mov_id(0)

#ifdef SJ_DEBUG
, b_DebugDisp(true)
#else
, b_DebugDisp(false)
#endif

{
	/********************
	********************/
	srand((unsigned) time(NULL));
	
	/********************
	********************/
	font[FONT_S].load("font/RictyDiminished-Regular.ttf", 10, true, true, true);
	font[FONT_M].load("font/RictyDiminished-Regular.ttf", 12, true, true, true);
	font[FONT_L].load("font/RictyDiminished-Regular.ttf", 15, true, true, true);
	font[FONT_LL].load("font/RictyDiminished-Regular.ttf", 30, true, true, true);
}

/******************************
******************************/
ofApp::~ofApp()
{
}

/******************************
******************************/
void ofApp::setup(){

	/********************
	********************/
	ofSetWindowTitle("RESTORATION");
	
	ofSetWindowShape( WINDOW_WIDTH, WINDOW_HEIGHT );
	/*
	ofSetVerticalSync(false);
	ofSetFrameRate(0);
	/*/
	ofSetVerticalSync(true);
	ofSetFrameRate(30);
	//*/
	
	ofSetEscapeQuitsApp(false);
	
	/********************
	********************/
	setup_Gui();
	
	setup_Camera();
	
	/********************
	********************/
	shader_Mask.load( "sj_shader/mask.vert", "sj_shader/mask.frag");
	
	/********************
	********************/
	/* */
	img_Frame.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_COLOR);
	img_Frame_Gray.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_GRAYSCALE);
	img_LastFrame_Gray.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_GRAYSCALE);
	img_AbsDiff_BinGray.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_GRAYSCALE);
	img_BinGray_Cleaned.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_GRAYSCALE);
	img_Bin_RGB.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_COLOR);
	
	/* */
    fbo_CamFrame.allocate(OUT_WIDTH, OUT_HEIGHT, GL_RGBA);
	clear_fbo(fbo_CamFrame);
	
    fbo_Movie.allocate(OUT_WIDTH, OUT_HEIGHT, GL_RGBA);
	clear_fbo(fbo_Movie);
	
    fbo_Mask_S.allocate(CAM_WIDTH, CAM_HEIGHT, GL_RGBA);
	Reset_FboMask();
	
    fbo_PreOut.allocate(OUT_WIDTH, OUT_HEIGHT, GL_RGBA);
	clear_fbo(fbo_PreOut);
	
    fbo_Out.allocate(OUT_WIDTH, OUT_HEIGHT, GL_RGBA);
	clear_fbo(fbo_Out);
	
	/* */
	pix_Mask_S.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_COLOR);
	img_Mask_S.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_COLOR);
	img_Mask_S_Gray.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_GRAYSCALE);
	pix_Mask_L.allocate(OUT_WIDTH, OUT_HEIGHT, OF_IMAGE_GRAYSCALE);
	img_Mask_L.allocate(OUT_WIDTH, OUT_HEIGHT, OF_IMAGE_COLOR);
	img_Mask_L_Gray.allocate(OUT_WIDTH, OUT_HEIGHT, OF_IMAGE_GRAYSCALE);
	
	/********************
	********************/
	myGlitch.setup(&fbo_Out);
	Clear_AllGlitch();
	
	ControlMask.setup();
	
	/********************
	********************/
	makeup_mov_list("../../../data/mov", Movies);
	Order_Movies.resize(Movies.size());
	SJ_UTIL::FisherYates(Order_Movies);
	Start_Mov(get_Active_Mov());
	
	/********************
	********************/
	SyphonServer.setName("RESTORATION");
}

/******************************
******************************/
void ofApp::clear_fbo(ofFbo& fbo)
{
	fbo.begin();
	ofClear(0, 0, 0, 0);
	fbo.end();
}

/******************************
******************************/
void ofApp::inc_Movies_id()
{
	Mov_id = getNextId_of_Movies();
}

/******************************
******************************/
int ofApp::getNextId_of_Movies()
{
	int ret = Mov_id + 1;
	if(Movies.size() <= ret) ret = 0;
	
	return ret;
}

/******************************
******************************/
ofVideoPlayer& ofApp::get_Active_Mov()
{
	return Movies[Order_Movies[Mov_id]];
}

/******************************
******************************/
void ofApp::Reset_FboMask()
{
	fbo_Mask_S.begin();
	ofClear(0, 0, 0, 0);
	ofSetColor(255, 255, 255, 255);
	ofDrawRectangle(0, 0, fbo_Mask_S.getWidth(), fbo_Mask_S.getHeight());
	
	fbo_Mask_S.end();
}

/******************************
******************************/
void ofApp::Refresh_Fbo_Movie(ofVideoPlayer& video)
{
	fbo_Movie.begin();
	ofClear(0, 0, 0, 0);
	ofSetColor(255, 255, 255, 255);
	
	if(video.isLoaded() && video.isPlaying()){
		video.draw(0, 0, fbo_Movie.getWidth(), fbo_Movie.getHeight());
	}else{
		ofSetColor(0, 0, 0, 255);
		ofDrawRectangle(0, 0, fbo_Movie.getWidth(), fbo_Movie.getHeight());
	}
	
	fbo_Movie.end();
}

/******************************
******************************/
void ofApp::Start_Mov(ofVideoPlayer& video)
{
	if(video.isLoaded()){
		video.setLoopState(OF_LOOP_NONE);
		video.setSpeed(1);
		video.setVolume(1.0);
		video.play();
	}
}

/******************************
******************************/
void ofApp::makeup_mov_list(const string dirname, vector<ofVideoPlayer>& _Movies)
{
	/********************
	********************/
	DIR *pDir;
	struct dirent *pEnt;
	struct stat wStat;
	string wPathName;

	pDir = opendir( dirname.c_str() );
	if ( NULL == pDir ) { ERROR_MSG(); std::exit(1); }

	pEnt = readdir( pDir );
	while ( pEnt ) {
		// .と..は処理しない
		if ( strcmp( pEnt->d_name, "." ) && strcmp( pEnt->d_name, ".." ) ) {
		
			wPathName = dirname + "/" + pEnt->d_name;
			
			// ファイルの情報を取得
			if ( stat( wPathName.c_str(), &wStat ) ) {
				printf( "Failed to get stat %s \n", wPathName.c_str() );
				break;
			}
			
			if ( S_ISDIR( wStat.st_mode ) ) {
				// nothing.
			} else {
			
				vector<string> str = ofSplitString(pEnt->d_name, ".");
				if( (str[str.size()-1] == "mp4") || (str[str.size()-1] == "mov") ){
					ofVideoPlayer _mov;
					
					_mov.load(wPathName);
					_Movies.push_back(_mov);
					
					if(MAX_MOV_LOAD <= _Movies.size()) break;
				}
			}
		}
		
		pEnt = readdir( pDir ); // 次のファイルを検索する
	}

	closedir( pDir );
	
	/********************
	********************/
	if(_Movies.size() < 1)	{ ERROR_MSG();std::exit(1);}
	else					{ printf("> %d movies loaded\n", int(_Movies.size())); fflush(stdout); }
}

/******************************
******************************/
void ofApp::setup_Camera()
{
	/********************
	********************/
	printf("> setup camera\n");
	fflush(stdout);
	
	ofSetLogLevel(OF_LOG_VERBOSE);
    cam.setVerbose(true);
	
	vector< ofVideoDevice > Devices = cam.listDevices();// 上 2行がないと、List表示されない.
	
	if(Cam_id == -1){
		std::exit(1);
	}else{
		if(Devices.size() <= Cam_id) { ERROR_MSG(); std::exit(1); }
		
		cam.setDeviceID(Cam_id);
		cam.initGrabber(CAM_WIDTH, CAM_HEIGHT);
		
		printf("> Cam size asked = (%d, %d), actual = (%d, %d)\n", int(CAM_WIDTH), int(CAM_HEIGHT), int(cam.getWidth()), int(cam.getHeight()));
		fflush(stdout);
	}
	
	return;
}

/******************************
description
	memoryを確保は、app start後にしないと、
	segmentation faultになってしまった。
******************************/
void ofApp::setup_Gui()
{
	/********************
	********************/
	Gui_Global = new GUI_GLOBAL;
	Gui_Global->setup("RESTORATION", "gui.xml", 1000, 10);
}

/******************************
******************************/
void ofApp::update(){
	/********************
	********************/
	int now = ofGetElapsedTimeMillis();
	
	/********************
	********************/
	ofSoundUpdate();
	
	/********************
	********************/
    cam.update();
    if(cam.isFrameNew())	{ update_img_OnCam(); }
	
	update_mov(get_Active_Mov());
	
	ControlMask.update(now, b_DebugDisp);
	
	update_img();
	
	/********************
	********************/
	StateChart_Top(now);

	/********************
	********************/
	printf("%5d, %5.0f\r", StateTop.State, ofGetFrameRate());
	fflush(stdout);
}

/******************************
******************************/
void ofApp::update_mov(ofVideoPlayer& video){
	if(video.isLoaded()){
		video.update();
		if(video.isFrameNew()){
			Refresh_Fbo_Movie(video);
		}
	}
}

/******************************
******************************/
void ofApp::update_img_OnCam(){
	ofxCv::copy(cam, img_Frame);
	if(b_flipCamera) img_Frame.mirror(false, true);
	img_Frame.update();
	
	Copy_CamFrame_to_fbo();
	
	img_LastFrame_Gray = img_Frame_Gray;
	// img_LastFrame_Gray.update(); // drawしないので不要.
	
	convertColor(img_Frame, img_Frame_Gray, CV_RGB2GRAY);
	ofxCv::blur(img_Frame_Gray, ForceOdd((int)Gui_Global->BlurRadius));
	img_Frame_Gray.update();
}

/******************************
******************************/
void ofApp::Copy_CamFrame_to_fbo(){
	ofDisableAlphaBlending();
	
	fbo_CamFrame.begin();
		ofClear(0, 0, 0, 0);
		ofSetColor(255);
		
		img_Frame.draw(0, 0, fbo_CamFrame.getWidth(), fbo_CamFrame.getHeight());
	fbo_CamFrame.end();
}

/******************************
******************************/
void ofApp::update_img(){
	/* */
	absdiff(img_Frame_Gray, img_LastFrame_Gray, img_AbsDiff_BinGray);
	threshold(img_AbsDiff_BinGray, Gui_Global->thresh_Diff_to_Bin);
	img_AbsDiff_BinGray.update();
	
	ofxCv::copy(img_AbsDiff_BinGray, img_BinGray_Cleaned);
	ofxCv::medianBlur(img_BinGray_Cleaned, ForceOdd((int)Gui_Global->MedianRadius));
	threshold(img_BinGray_Cleaned, Gui_Global->thresh_Medianed_to_Bin);
	img_BinGray_Cleaned.update();
	
	// ofxCv::copy(img_BinGray_Cleaned, img_Bin_RGB);
	convertColor(img_BinGray_Cleaned, img_Bin_RGB, CV_GRAY2RGB);
	img_Bin_RGB.update();
	
	/* */
	if(StateTop.State == STATE_TOP::STATE__RUN) ControlMask.Add_Diff_to_Mask(img_Bin_RGB);
	ControlMask.make_TotalMask(fbo_Mask_S);
	
	fbo_Mask_S.readToPixels(pix_Mask_S);
	img_Mask_S.setFromPixels(pix_Mask_S);
	convertColor(img_Mask_S, img_Mask_S_Gray, CV_RGB2GRAY);
	
	/* */
	MaskS_to_MaskL();
	
	Mask_x_Movie();
	
	drawFbo_Preout_to_Out();
}

/******************************
******************************/
void ofApp::StateChart_Top(int now){
	switch(StateTop.State){
		case STATE_TOP::STATE__SLEEP:
			if( get_Active_Mov().getIsMovieDone() || !get_Active_Mov().isPlaying() ){
				if(get_Active_Mov().isPlaying())	get_Active_Mov().stop();
				
				inc_Movies_id();
				Start_Mov(get_Active_Mov());

				Refresh_Fbo_Movie(get_Active_Mov());
			}
			break;
			
		case STATE_TOP::STATE__RUN:
			if( StateTop.IsTimeout(get_Active_Mov()) ){
				ControlMask.Reset();
				Reset_FboMask();
				Random_Enable_myGlitch();
				StateTop.Transition(STATE_TOP::STATE__CHANGING_CONTENTS, now);
			}
			break;
			
		case STATE_TOP::STATE__CHANGING_CONTENTS:
			if( get_Active_Mov().getIsMovieDone() || !get_Active_Mov().isPlaying() || StateTop.IsTimeout(get_Active_Mov()) ){
				if(get_Active_Mov().isPlaying())	get_Active_Mov().stop();
				
				inc_Movies_id();
				Start_Mov(get_Active_Mov());
				Refresh_Fbo_Movie(get_Active_Mov());
				
				StateTop.Transition(STATE_TOP::STATE__WAIT_STABLE, now);
			}
			
			break;
			
		case STATE_TOP::STATE__WAIT_STABLE:
			if(StateTop.Is_SomeThingWrong(now)){
				if(get_Active_Mov().isPlaying())	get_Active_Mov().stop();
				Start_Mov(get_Active_Mov());
				
			}else if( StateTop.IsTimeout(get_Active_Mov()) ){
				StateTop.Transition(STATE_TOP::STATE__RUN, now);
				Clear_AllGlitch();
			}
			break;
	}
}

/******************************
******************************/
ofVec2f ofApp::SizeConvert_StoL(ofVec2f from){
	return from * (float(OUT_WIDTH) / CAM_WIDTH);
}

/******************************
******************************/
void ofApp::drawFbo_Preout_to_Out()
{
	/********************
	********************/
	ofDisableAlphaBlending();
	
	fbo_Out.begin();
		ofClear(0, 0, 0, 0);
	
		ofSetColor(255, 255, 255, 255);
		
		fbo_PreOut.draw(0, 0, fbo_Out.getWidth(), fbo_Out.getHeight());
	fbo_Out.end();
}

/******************************
opencv:resize
	https://cvtech.cc/resize/
	
OpenCVの膨張縮小って4近傍？8近傍?	
	http://micchysdiary.blogspot.com/2012/10/opencv48.html
******************************/
void ofApp::MaskS_to_MaskL()
{
	cv::Mat srcMat = toCv(img_Mask_S_Gray);
	cv::Mat src_Cleaned_Mat;
	cv::Mat src_Eroded_Mat;
	cv::dilate(srcMat, src_Cleaned_Mat, Mat(), cv::Point(-1, -1), 2); // 白が拡大 : ゴミ取り
	cv::erode(src_Cleaned_Mat, src_Eroded_Mat, Mat(), cv::Point(-1, -1), (int)Gui_Global->ErodeSize); // 白に黒が侵食 : 拡大
	
	cv::Mat dstMat;// = toCv(img_Mask_L);
	// resize(src_Eroded_Mat, dstMat, dstMat.size(), 0, 0, INTER_LINEAR);
	resize(src_Eroded_Mat, dstMat, cv::Size(), img_Mask_L.getWidth()/src_Eroded_Mat.cols, img_Mask_L.getHeight()/src_Eroded_Mat.rows);
	
	toOf(dstMat, pix_Mask_L);
	img_Mask_L_Gray.setFromPixels(pix_Mask_L);
	
	ofxCv::blur(img_Mask_L_Gray, ForceOdd((int)Gui_Global->BlurRadius_MaskL));
	img_Mask_L_Gray.update();
	
	convertColor(img_Mask_L_Gray, img_Mask_L, CV_GRAY2RGB);
	img_Mask_L.update();
}

/******************************
******************************/
void ofApp::Mask_x_Movie()
{
	ofDisableAlphaBlending();
	
	fbo_PreOut.begin();
	shader_Mask.begin();
	
		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		
		shader_Mask.setUniform1f( "val_min", Gui_Global->MixImage_minVal );
		
		shader_Mask.setUniformTexture( "Back", fbo_CamFrame.getTexture(), 1 );
		shader_Mask.setUniformTexture( "mask", img_Mask_L.getTexture(), 2 );
		
		fbo_Movie.draw(0, 0, fbo_PreOut.getWidth(), fbo_PreOut.getHeight());
		
	shader_Mask.end();
	fbo_PreOut.end();
}

/******************************
******************************/
int ofApp::ForceOdd(int val){
	if(val <= 0)			return val;
	else if(val % 2 == 0)	return val - 1;
	else					return val;
}

/******************************
******************************/
void ofApp::draw(){
	/********************
	********************/
	/*
	ofEnableAlphaBlending();
	// ofEnableBlendMode(OF_BLENDMODE_ADD);
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	*/
	ofDisableAlphaBlending();
	
	/********************
	********************/
	// ofClear(0, 0, 0, 0);
	ofBackground(50);
	ofSetColor(255, 255, 255, 255);
	
	/********************
	********************/
	myGlitch.generateFx(); // Apply effects
	
	/********************
	********************/
	draw_ProcessLine();
	
	if(b_DebugDisp){
		fbo_Mask_S.draw(880, 495, DRAW_WIDTH, DRAW_HEIGHT);
		ControlMask.draw();
	}else{
		draw_ProcessedImages();
	}
	
	/********************
	********************/
	ofTexture tex = fbo_Out.getTextureReference();
	SyphonServer.publishTexture(&tex);
	
	/********************
	********************/
	Gui_Global->gui.draw();
}

/******************************
******************************/
void ofApp::draw_ProcessLine(){
	/********************
	********************/
	ofSetColor(250);
	glPointSize(1.0);
	// glLineWidth(1);
	ofSetLineWidth(1);
	
	
	ofDrawLine(240, 135, 1040, 135);
	ofDrawLine(240, 360, 1040, 360);
	ofDrawLine(240, 585, 1040, 585);
	
	ofDrawLine(1040, 135, 1040, 360);
	ofDrawLine(240, 360, 240, 585);
}

/******************************
******************************/
void ofApp::draw_ProcessedImages(){
	/********************
	********************/
	ofSetColor(255, 255, 255, 255);
	
	/********************
	********************/
	/* */
	img_Frame.draw(80, 45, DRAW_WIDTH, DRAW_HEIGHT);
	img_Frame_Gray.draw(480, 45, DRAW_WIDTH, DRAW_HEIGHT);
	img_AbsDiff_BinGray.draw(880, 45, DRAW_WIDTH, DRAW_HEIGHT);
	
	/* */
	img_BinGray_Cleaned.draw(880, 270, DRAW_WIDTH, DRAW_HEIGHT);
	fbo_Mask_S.draw(480, 270, DRAW_WIDTH, DRAW_HEIGHT);
	img_Mask_L.draw(80, 270, DRAW_WIDTH, DRAW_HEIGHT);
	
	/* */
	fbo_Movie.draw(80, 495, DRAW_WIDTH, DRAW_HEIGHT);
	fbo_PreOut.draw(480, 495, DRAW_WIDTH, DRAW_HEIGHT);
	fbo_Out.draw(880, 495, DRAW_WIDTH, DRAW_HEIGHT);
}

/******************************
******************************/
void ofApp::Random_Enable_myGlitch(){
	SJ_UTIL::FisherYates(Glitch_Ids, NUM_GLITCH_TYPES);
	NumGlitch_Enable = int(ofRandom(1, MAX_NUM_GLITCHS_ONE_TIME));
	for(int i = 0; i < NumGlitch_Enable; i++) { Set_myGlitch(Glitch_Ids[i], true); }
}

/******************************
******************************/
void ofApp::Set_myGlitch(int key, bool b_switch)
{
	if (key == 0)	myGlitch.setFx(OFXPOSTGLITCH_INVERT			, b_switch);
	if (key == 1)	myGlitch.setFx(OFXPOSTGLITCH_CONVERGENCE	, b_switch);
	if (key == 2)	myGlitch.setFx(OFXPOSTGLITCH_GLOW			, b_switch);
	if (key == 3)	myGlitch.setFx(OFXPOSTGLITCH_SHAKER			, b_switch);
	if (key == 4)	myGlitch.setFx(OFXPOSTGLITCH_CUTSLIDER		, b_switch);
	if (key == 5)	myGlitch.setFx(OFXPOSTGLITCH_TWIST			, b_switch);
	if (key == 6)	myGlitch.setFx(OFXPOSTGLITCH_OUTLINE		, b_switch);
	if (key == 7)	myGlitch.setFx(OFXPOSTGLITCH_NOISE			, b_switch);
	if (key == 8)	myGlitch.setFx(OFXPOSTGLITCH_SLITSCAN		, b_switch);
	if (key == 9)	myGlitch.setFx(OFXPOSTGLITCH_SWELL			, b_switch);
	if (key == 10)	myGlitch.setFx(OFXPOSTGLITCH_CR_HIGHCONTRAST, b_switch);
	if (key == 11)	myGlitch.setFx(OFXPOSTGLITCH_CR_BLUERAISE	, b_switch);
	if (key == 12)	myGlitch.setFx(OFXPOSTGLITCH_CR_REDRAISE	, b_switch);
	if (key == 13)	myGlitch.setFx(OFXPOSTGLITCH_CR_GREENRAISE	, b_switch);
	if (key == 14)	myGlitch.setFx(OFXPOSTGLITCH_CR_BLUEINVERT	, b_switch);
	if (key == 15)	myGlitch.setFx(OFXPOSTGLITCH_CR_REDINVERT	, b_switch);
	if (key == 16)	myGlitch.setFx(OFXPOSTGLITCH_CR_GREENINVERT	, b_switch);
}

/******************************
******************************/
void ofApp::Clear_AllGlitch()
{
	for(int i = 0; i < NUM_GLITCH_TYPES; i++){
		Set_myGlitch(i, false);
	}
}

/******************************
******************************/
void ofApp::keyPressed(int key){
	switch(key){
		case 'b':
			b_DebugDisp = !b_DebugDisp;
			break;
			
		case 'r':
			if(StateTop.State == STATE_TOP::STATE__SLEEP){
				StateTop.Transition(STATE_TOP::STATE__RUN, ofGetElapsedTimeMillis());
				Clear_AllGlitch();
			}
			break;
			
		case 's':
			if(StateTop.State == STATE_TOP::STATE__RUN){
				int now = ofGetElapsedTimeMillis();
				
				StateTop.Transition(STATE_TOP::STATE__SLEEP, ofGetElapsedTimeMillis());
				Reset_FboMask();
				
				Clear_AllGlitch();
			}
			break;
			
			
		case ' ':
			{
				char buf[512];
				
				sprintf(buf, "image_%d.png", png_id);
				ofSaveScreen(buf);
				// ofSaveFrame();
				printf("> %s saved\n", buf);
				
				png_id++;
			}
			
			break;
	}

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
