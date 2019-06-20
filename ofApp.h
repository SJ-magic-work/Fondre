/************************************************************
■ofxCv
	https://github.com/kylemcdonald/ofxCv
	
	note
		Your linker will also need to know where the OpenCv headers are. In XCode this means modifying one line in Project.xconfig:
			HEADER_SEARCH_PATHS = $(OF_CORE_HEADERS) "../../../addons/ofxOpenCv/libs/opencv/include/" "../../../addons/ofxCv/libs/ofxCv/include/"
			
		
■openFrameworksのAddon、ofxCvの導入メモ
	https://qiita.com/nenjiru/items/50325fabe4c3032da270
	
	contents
		導入時に、一手間 必要.
		
■Kill camera process
	> sudo killall VDCAssistant
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "ofMain.h"
#include "ofxCv.h"
#include "ofxPostGlitch.h"
#include "ofxSyphon.h"

#include "sj_common.h"
#include "util.h"
#include "ControlMask.h"

/************************************************************
************************************************************/

/**************************************************
**************************************************/
struct STATE_TOP{
	enum STATE{
		STATE__SLEEP,
		STATE__RUN,
		STATE__CHANGING_CONTENTS,
		STATE__WAIT_STABLE,
		
		NUM_STATE,
	};
	
	ofSoundPlayer sound_Noise;
	float MaxVol_SoundNoise;
	int t_From_ms;
	
	STATE State;
	
	STATE_TOP()
	: State(STATE__SLEEP)
	, MaxVol_SoundNoise(1.0)
	, t_From_ms(0)
	{
		SJ_UTIL::setup_sound(sound_Noise, "sound/top/radio-tuning-noise-short-waves_zydE-HEd.mp3", true, 0.0);
		sound_Noise.play();
	}
	
	void Transition(int NextState, int now)
	{
		if(NextState < NUM_STATE){
			State = STATE(NextState);
			
			switch(State){
				case STATE__SLEEP:
					sound_Noise.setVolume(0);
					break;
					
				case STATE__RUN:
					break;
					
				case STATE__CHANGING_CONTENTS:
					sound_Noise.setVolume(MaxVol_SoundNoise);
					break;
					
				case STATE__WAIT_STABLE:
					sound_Noise.setVolume(0);
					break;
			}
			
			t_From_ms = now;
		}
	}
	
	bool IsTimeout(ofVideoPlayer& video){
		/********************
		********************/
		if(!video.isLoaded() || !video.isPlaying())	return false;
		
		if(State == STATE__SLEEP) return false;
		
		/********************
		********************/
		int TotalNumFrames = video.getTotalNumFrames();
		int CurrentFrame = video.getCurrentFrame();
		
		switch(State){
			case STATE__RUN:
				if(TotalNumFrames - 20 < CurrentFrame)	return true;
				else									return false;
				break;
				
			case STATE__CHANGING_CONTENTS:
				if(TotalNumFrames - 5 < CurrentFrame)	return true;
				else									return false;
				break;
				
			case STATE__WAIT_STABLE:
				if(5 < CurrentFrame)	return true;
				else					return false;
				break;
		}
		
		return false;
	}
	
	bool Is_SomeThingWrong(int now_ms){
		switch(State){
			case STATE__SLEEP:
			case STATE__RUN:
			case STATE__CHANGING_CONTENTS:
				return false;
				
			case STATE__WAIT_STABLE:
				if(2000 < now_ms - t_From_ms)	return true;
				else							return false;
		}
		
		return false;
	}
};

/**************************************************
**************************************************/
class ofApp : public ofBaseApp{
private:
	/****************************************
	****************************************/
	enum{
		FONT_S,
		FONT_M,
		FONT_L,
		FONT_LL,
		
		NUM_FONTSIZE,
	};
	
	enum{
		NUM_GLITCH_TYPES = 17,
		MAX_NUM_GLITCHS_ONE_TIME = 5,
	};
	
	enum{
		MAX_MOV_LOAD = 5,
	};
	
	/****************************************
	****************************************/
	/********************
	********************/
	bool b_DebugDisp;
	
	/********************
	********************/
	ofTrueTypeFont font[NUM_FONTSIZE];
	int png_id;
	
	ofVideoGrabber cam;
	int Cam_id;
	
	STATE_TOP StateTop;
	
    ofImage img_Frame;
    ofImage img_Frame_Gray;
    ofImage img_LastFrame_Gray;
    ofImage img_AbsDiff_BinGray;
    ofImage img_BinGray_Cleaned;
    ofImage img_Bin_RGB;
    ofFbo fbo_CamFrame;
    ofFbo fbo_Movie;
	ofFbo fbo_Mask_S;
	ofFbo fbo_PreOut;
	ofFbo fbo_Out;
	
	ofPixels pix_Mask_S;
	ofImage img_Mask_S;
	ofImage img_Mask_S_Gray;
	ofPixels pix_Mask_L;
	ofImage img_Mask_L;
	ofImage img_Mask_L_Gray;
	
	/********************
	********************/
	ofShader shader_Mask;
	
	/********************
	********************/
	int Mov_id;
	vector<ofVideoPlayer> Movies;
	vector<int> Order_Movies;
	
	/********************
	********************/
	ofxPostGlitch	myGlitch;
	int Glitch_Ids[NUM_GLITCH_TYPES];
	int NumGlitch_Enable;
	
	/********************
	********************/
	ofxSyphonServer SyphonServer;
	
	bool b_flipCamera;
	
	/********************
	********************/
	CONTROL_MASK ControlMask;
	
	/****************************************
	****************************************/
	void clear_fbo(ofFbo& fbo);
	ofVideoPlayer& get_Active_Mov();
	void Copy_CamFrame_to_fbo();
	void setup_Gui();
	void setup_Camera();
	void update_img();
	void update_img_OnCam();
	int ForceOdd(int val);
	void draw_ProcessedImages();
	void StateChart_Top(int now);
	void makeup_mov_list(const string dirname, vector<ofVideoPlayer>& _Movies);
	void inc_Movies_id();
	int getNextId_of_Movies();
	void Refresh_Fbo_Movie(ofVideoPlayer& video);
	void Start_Mov(ofVideoPlayer& video);
	void Reset_FboMask();
	void update_mov(ofVideoPlayer& video);
	void Random_Enable_myGlitch();
	void Set_myGlitch(int key, bool b_switch);
	void Clear_AllGlitch();
	
	void MaskS_to_MaskL();
	void Mask_x_Movie();
	void drawFbo_Preout_to_Out();
	
	ofVec2f SizeConvert_StoL(ofVec2f from);
	
	void draw_ProcessLine();
	
public:
	/****************************************
	****************************************/
	ofApp(int _Cam_id, bool _b_flipCamera);
	~ofApp();

	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	
};
