/************************************************************
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "ofMain.h"
#include "sj_common.h"


/************************************************************
************************************************************/

/**************************************************
**************************************************/
class CONTROL_MASK : Noncopyable{
private:
	/****************************************
	****************************************/
	enum{
		NUM_MASK_BUFFERS = 7, // caution : shader = makeTotalMask は、NUM固定なので、ここを変えたら一緒に変更すること.
	};
	
	const int DURATION_OF_ONE_BUFFER_MS;
	const int DURATION_OF_ONE_BUFFER_MS__FOR_DEBUG;
	
	int t_From_ms;
	int t_LastINT;
	
	int BufferId;
	
	ofFbo fbo_Mask_S[NUM_MASK_BUFFERS];
	ofFbo fbo_MaskToBeRepaired;
	
	
	ofShader shader_Fadeout;
	ofShader shader_AddMask;
	ofShader shader_InvertAddMask;
	ofShader shader_MakeTotalMask;
	
	/****************************************
	****************************************/
	void Fadeout_MaskTobeRepaired(float dt);
	void ExchangeBuffer();
	int Next_Of_BufferId(int _id);
	void Fbo_ClearWhite(ofFbo& fbo);
	
public:
	/****************************************
	****************************************/
	CONTROL_MASK();
	~CONTROL_MASK();
	
	void setup();
	void update(int now_ms, bool b_debug);
	void Add_Diff_to_Mask(ofImage& img_Bin_RGB);
	void make_TotalMask(ofFbo& fbo_Mask_Total_S);
	void Reset();
	
	void draw();
};

