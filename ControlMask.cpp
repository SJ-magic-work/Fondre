/************************************************************
************************************************************/
#include "ControlMask.h"

/************************************************************
************************************************************/

/******************************
******************************/
CONTROL_MASK::CONTROL_MASK()
: BufferId(0)

, DURATION_OF_ONE_BUFFER_MS__FOR_DEBUG(3000)
, DURATION_OF_ONE_BUFFER_MS(700)

, t_From_ms(0)
, t_LastINT(0)
{
}

/******************************
******************************/
CONTROL_MASK::~CONTROL_MASK()
{
}

/******************************
******************************/
void CONTROL_MASK::setup()
{
	/********************
	********************/
	for(int i = 0; i < NUM_MASK_BUFFERS; i++){
		fbo_Mask_S[i].allocate(CAM_WIDTH, CAM_HEIGHT, GL_RGBA);
	}
	fbo_MaskToBeRepaired.allocate(CAM_WIDTH, CAM_HEIGHT, GL_RGBA);
	
	Reset();
	
	/********************
	********************/
	shader_Fadeout.load( "sj_shader/Fadeout.vert", "sj_shader/Fadeout.frag");
	shader_AddMask.load( "sj_shader/AddMask.vert", "sj_shader/AddMask.frag");
	shader_InvertAddMask.load( "sj_shader/Invert_AddMask.vert", "sj_shader/Invert_AddMask.frag");
	shader_MakeTotalMask.load( "sj_shader/makeTotalMask.vert", "sj_shader/makeTotalMask.frag");
}

/******************************
******************************/
void CONTROL_MASK::Reset()
{
	for(int i = 0; i < NUM_MASK_BUFFERS; i++) { Fbo_ClearWhite(fbo_Mask_S[i]); }
	
	Fbo_ClearWhite(fbo_MaskToBeRepaired);
}

/******************************
******************************/
void CONTROL_MASK::Fbo_ClearWhite(ofFbo& fbo)
{
	fbo.begin();
	ofSetColor(255, 255, 255, 255);
	ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
	
	fbo.end();
}

/******************************
******************************/
void CONTROL_MASK::update(int now_ms, bool b_debug)
{
	/********************
	********************/
	Fadeout_MaskTobeRepaired(float(now_ms - t_LastINT) / 1000.0);
	
	/********************
	********************/
	int Duration_Of_One_Buffer;
	if(b_debug)	Duration_Of_One_Buffer = DURATION_OF_ONE_BUFFER_MS__FOR_DEBUG;
	else		Duration_Of_One_Buffer = DURATION_OF_ONE_BUFFER_MS;
	
	if(Duration_Of_One_Buffer < now_ms - t_From_ms){
		ExchangeBuffer();
		t_From_ms = now_ms;
	}
	
	/********************
	********************/
	t_LastINT = now_ms;
}

/******************************
******************************/
void CONTROL_MASK::Fadeout_MaskTobeRepaired(float dt)
{
	ofDisableAlphaBlending();
	ofEnableSmoothing();
	
	float FadeVal = 1.0 / Gui_Global->FadeDuration * dt;
	
	fbo_MaskToBeRepaired.begin();
	shader_Fadeout.begin();
		
		shader_Fadeout.setUniform1f( "FadeVal", FadeVal );
		
		ofSetColor( 255, 255, 255 );
		fbo_MaskToBeRepaired.draw(0, 0, fbo_MaskToBeRepaired.getWidth(), fbo_MaskToBeRepaired.getHeight());
	
	shader_Fadeout.end();
	fbo_MaskToBeRepaired.end();
}

/******************************
******************************/
void CONTROL_MASK::ExchangeBuffer()
{
	/********************
	********************/
	ofDisableAlphaBlending();
	ofEnableSmoothing();
	
	fbo_MaskToBeRepaired.begin();
	shader_AddMask.begin();
		shader_AddMask.setUniformTexture( "MaskAll", fbo_MaskToBeRepaired.getTextureReference(), 1 );
		fbo_Mask_S[Next_Of_BufferId(BufferId)].draw(0, 0, fbo_MaskToBeRepaired.getWidth(), fbo_MaskToBeRepaired.getHeight());
		
	shader_AddMask.end();
	fbo_MaskToBeRepaired.end();
	
	
	/********************
	********************/
	Fbo_ClearWhite(fbo_Mask_S[Next_Of_BufferId(BufferId)]);
	
	BufferId = Next_Of_BufferId(BufferId);
}

/******************************
******************************/
int CONTROL_MASK::Next_Of_BufferId(int _id)
{
	int NextId = _id + 1;
	
	if(NUM_MASK_BUFFERS <= NextId) NextId = 0;
	
	return NextId;
}

/******************************
******************************/
void CONTROL_MASK::Add_Diff_to_Mask(ofImage& img_Bin_RGB)
{
	ofDisableAlphaBlending();
	
	fbo_Mask_S[BufferId].begin();
	shader_InvertAddMask.begin();
	
		// ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		
		shader_InvertAddMask.setUniformTexture( "MaskAll", fbo_Mask_S[BufferId].getTexture(), 1 );
		
		img_Bin_RGB.draw(0, 0, fbo_Mask_S[BufferId].getWidth(), fbo_Mask_S[BufferId].getHeight());
		
	shader_InvertAddMask.end();
	fbo_Mask_S[BufferId].end();
}

/******************************
******************************/
void CONTROL_MASK::make_TotalMask(ofFbo& fbo_Mask_Total_S)
{
	ofDisableAlphaBlending();
	
	fbo_Mask_Total_S.begin();
	shader_MakeTotalMask.begin();
		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		
		for(int i = 0; i < NUM_MASK_BUFFERS; i++){
			char buf[BUF_SIZE_S];
			sprintf(buf, "Mask_%d", i + 1);
			
			shader_MakeTotalMask.setUniformTexture( buf, fbo_Mask_S[i].getTexture(), i + 1 );
		}
		
		fbo_MaskToBeRepaired.draw(0, 0, fbo_Mask_Total_S.getWidth(), fbo_Mask_Total_S.getHeight());
		
	
	shader_MakeTotalMask.end();
	fbo_Mask_Total_S.end();
}

/******************************
description
	for debug.
******************************/
void CONTROL_MASK::draw()
{
	/********************
	********************/
	ofSetColor(255, 255, 255, 255);
	
	/********************
	********************/
	/* */
	int id = BufferId;
	fbo_Mask_S[id].draw(80, 45, DRAW_WIDTH, DRAW_HEIGHT);
	
	id = Next_Of_BufferId(id);
	fbo_Mask_S[id].draw(480, 45, DRAW_WIDTH, DRAW_HEIGHT);
	
	id = Next_Of_BufferId(id);
	fbo_Mask_S[id].draw(880, 45, DRAW_WIDTH, DRAW_HEIGHT);
	
	id = Next_Of_BufferId(id);
	fbo_Mask_S[id].draw(80, 270, DRAW_WIDTH, DRAW_HEIGHT);
	
	id = Next_Of_BufferId(id);
	fbo_Mask_S[id].draw(480, 270, DRAW_WIDTH, DRAW_HEIGHT);
	
	id = Next_Of_BufferId(id);
	fbo_Mask_S[id].draw(880, 270, DRAW_WIDTH, DRAW_HEIGHT);
	
	id = Next_Of_BufferId(id);
	fbo_Mask_S[id].draw(80, 495, DRAW_WIDTH, DRAW_HEIGHT);
	
	
	/* */
	fbo_MaskToBeRepaired.draw(480, 495, DRAW_WIDTH, DRAW_HEIGHT);
}


