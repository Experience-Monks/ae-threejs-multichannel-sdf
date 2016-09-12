/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007 Adobe Systems Incorporated                       */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Systems Incorporated and its suppliers, if    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Systems Incorporated and its    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Systems         */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

/*	MCSDF.cpp	

	This is a sample OpenGL plugin. The framework is done for you.
	Utilize it to create more funky effects.
	
	Revision history: 

	1.0 Win and Mac versions use the same base files.	anindyar	7/4/2007
	1.1 Add OpenGL context switching to play nicely with
		AE's own OpenGL usage (thanks Brendan Bolles!)	zal			8/13/2012

*/

#include "MCSDF.h"

#include "GL_base.h"
using namespace AESDK_OpenGL;

//toggle flag to use the GLSL shaders
#define USE_SHADERS 1

//fwd declaring a helper function - to create shader path string
string CreateShaderPath( string inPluginPath, string inShaderFileName );

/* AESDK_OpenGL effect specific variables */
static GLuint S_MCSDF_InputFrameTextureIDSu; //input texture
static AESDK_OpenGL::AESDK_OpenGL_EffectCommonData S_MCSDF_EffectCommonData; //effect state variables

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											STR(StrID_Name), 
											MAJOR_VERSION, 
											MINOR_VERSION, 
											STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION,
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags = 	PF_OutFlag_I_EXPAND_BUFFER				 |
							PF_OutFlag_I_HAVE_EXTERNAL_DEPENDENCIES;

	out_data->out_flags2 =  PF_OutFlag2_NONE;
	
	PF_Err err = PF_Err_NONE;
	try
	{
		AEGP_SuiteHandler suites(in_data->pica_basicP);

		AESDK_OpenGL_Err error_desc; 
		//Now comes the OpenGL part - OS specific loading to start with
		if( (error_desc = AESDK_OpenGL_Startup(S_MCSDF_EffectCommonData)) != AESDK_OpenGL_OK)
		{
			PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
			CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
		}
		
		
		SetPluginContext(S_MCSDF_EffectCommonData);

		//loading OpenGL resources
		if( (error_desc = AESDK_OpenGL_InitResources(S_MCSDF_EffectCommonData)) != AESDK_OpenGL_OK)
		{
			PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
			CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
		}

		//MCSDF effect specific OpenGL resource loading
		//create an empty texture for the input surface
		S_MCSDF_InputFrameTextureIDSu = -1;

		PF_Handle	dataH	=	suites.HandleSuite1()->host_new_handle(((S_MCSDF_EffectCommonData.mRenderBufferWidthSu * S_MCSDF_EffectCommonData.mRenderBufferHeightSu)* sizeof(GL_RGBA)));
		if (dataH)
		{
			unsigned int *dataP = reinterpret_cast<unsigned int*>(suites.HandleSuite1()->host_lock_handle(dataH));
			
			//create empty input frame texture
			glGenTextures( 1, &S_MCSDF_InputFrameTextureIDSu );
			glBindTexture(GL_TEXTURE_2D, S_MCSDF_InputFrameTextureIDSu);

			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

			glTexImage2D( GL_TEXTURE_2D, 0, 4, S_MCSDF_EffectCommonData.mRenderBufferWidthSu, S_MCSDF_EffectCommonData.mRenderBufferHeightSu, 0, GL_RGBA, GL_UNSIGNED_BYTE, dataP);

			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

			glTexImage2D( GL_TEXTURE_2D, 0, 4, S_MCSDF_EffectCommonData.mRenderBufferWidthSu, S_MCSDF_EffectCommonData.mRenderBufferHeightSu, 0, GL_RGBA, GL_UNSIGNED_BYTE, dataP);

			suites.HandleSuite1()->host_unlock_handle(dataH);
			suites.HandleSuite1()->host_dispose_handle(dataH);
		}
		else
		{
			CHECK(PF_Err_OUT_OF_MEMORY);
		}
#if USE_SHADERS	
		//initialize and compile the shader objects
		A_char pluginFolderPath[AEFX_MAX_PATH];
		PF_GET_PLATFORM_DATA(PF_PlatData_EXE_FILE_PATH_W, &pluginFolderPath);
		string pluginPath = string("/Applications/Adobe After Effects CC 2015.3/Plug-ins/Effects/blahblahblah");

		if( (error_desc = AESDK_OpenGL_InitShader(	S_MCSDF_EffectCommonData, 
													CreateShaderPath(pluginPath, string("MCSDF.vert")),
													CreateShaderPath(pluginPath, string("MCSDF.frag")) )) != AESDK_OpenGL_OK)
		{
			PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
			CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
		}
#endif //USE_SHADERS

		SetHostContext(S_MCSDF_EffectCommonData);
	}
	catch(PF_Err& thrown_err)
	{
		err = thrown_err;
		
		SetHostContext(S_MCSDF_EffectCommonData);
	}
	
	return err;
}

static PF_Err 
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	

	AEFX_CLR_STRUCT(def);
    
    PF_ADD_SLIDER(	STR(StrID_SliderName),
                  MCSDF_SLIDER_MIN,
                  MCSDF_SLIDER_MAX,
                  MCSDF_SLIDER_MIN,
                  MCSDF_SLIDER_MAX,
                  MCSDF_SLIDER_DFLT,
                  SLIDER_DISK_ID);
    
    PF_ADD_SLIDER(	STR(StrID_SliderName2),
                  MCSDF_SLIDER_MIN,
                  MCSDF_SLIDER_MAX,
                  MCSDF_SLIDER_MIN,
                  MCSDF_SLIDER_MAX,
                  MCSDF_SLIDER_DFLT,
                  SLIDER_DISK_ID2);
    
    PF_ADD_SLIDER(	STR(StrID_SliderName3),
                  MCSDF_SLIDER_MIN,
                  MCSDF_SLIDER_MAX,
                  MCSDF_SLIDER_MIN,
                  MCSDF_SLIDER_MAX,
                  MCSDF_SLIDER_DFLT,
                  SLIDER_DISK_ID3);
    
    out_data->num_params = MCSDF_NUM_PARAMS;

	return err;
}


static PF_Err 
GlobalSetdown (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err			err			=	PF_Err_NONE;

	try
	{
		SetPluginContext(S_MCSDF_EffectCommonData);
		
		//local OpenGL resource un-loading
		glDeleteTextures( 1, &S_MCSDF_InputFrameTextureIDSu);

		//common OpenGL resource unloading
		AESDK_OpenGL_Err error_desc;
		if( (error_desc = AESDK_OpenGL_ReleaseResources(S_MCSDF_EffectCommonData)) != AESDK_OpenGL_OK)
		{
			PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
			CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
		}

		SetHostContext(S_MCSDF_EffectCommonData);
		
		//OS specific unloading
		if( (error_desc = AESDK_OpenGL_Shutdown(S_MCSDF_EffectCommonData)) != AESDK_OpenGL_OK)
		{
			PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
			CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
		}

		if (in_data->sequence_data) {
			PF_DISPOSE_HANDLE(in_data->sequence_data);
			out_data->sequence_data = NULL;
		}
	}
	catch(PF_Err& thrown_err)
	{
		err = thrown_err;
		
		SetHostContext(S_MCSDF_EffectCommonData);
	}
	
	return err;
}


static PF_Err 
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err				err		= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	PF_EffectWorld	openGL_world;
	A_long				widthL	=	output->width,
						heightL	=	output->height;
    
    PF_FpLong sliderVal = params[MCSDF_SLIDER]->u.fd.value / 100.0f;
    PF_FpLong sliderVal2 = params[MCSDF_SLIDER2]->u.fd.value / 100.0f;
    PF_FpLong sliderVal3 = params[MCSDF_SLIDER3]->u.fd.value / 100.0f;
	
	try
	{
		SetPluginContext(S_MCSDF_EffectCommonData);
		
		//setup openGL_world
		AEFX_CLR_STRUCT(openGL_world);	
		ERR(suites.WorldSuite1()->new_world(	in_data->effect_ref,
												widthL,
												heightL,
												PF_NewWorldFlag_CLEAR_PIXELS,
												&openGL_world));

		//update the texture - only a portion of the texture that is
		PF_EffectWorld	*inputP		=	&params[MCSDF_INPUT]->u.ld;
		glEnable( GL_TEXTURE_2D );
			
		PF_Handle bufferH	= NULL;
		bufferH = suites.HandleSuite1()->host_new_handle(((S_MCSDF_EffectCommonData.mRenderBufferWidthSu * S_MCSDF_EffectCommonData.mRenderBufferHeightSu)* sizeof(GL_RGBA)));
		if (bufferH)
		{
			unsigned int *bufferP = reinterpret_cast<unsigned int*>(suites.HandleSuite1()->host_lock_handle(bufferH));

			//copy inputframe to openGL_world
			for (int ix=0; ix < inputP->height; ++ix)
			{
				PF_Pixel8 *pixelDataStart = NULL;
				PF_GET_PIXEL_DATA8( inputP , NULL, &pixelDataStart);
				::memcpy(	bufferP + (ix * S_MCSDF_EffectCommonData.mRenderBufferWidthSu ),
						pixelDataStart + (ix * (inputP->rowbytes)/sizeof(GL_RGBA)),
						inputP->width * sizeof(GL_RGBA));
			}
			
			//upload to texture memory
			glBindTexture(GL_TEXTURE_2D, S_MCSDF_InputFrameTextureIDSu);
            GLuint uniformWeightLocation = glGetUniformLocationARB(S_MCSDF_EffectCommonData.mProgramObjSu, "weight");
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, S_MCSDF_EffectCommonData.mRenderBufferWidthSu, S_MCSDF_EffectCommonData.mRenderBufferHeightSu, GL_RGBA, GL_UNSIGNED_BYTE, bufferP);
			
			AESDK_OpenGL_Err error_desc;
			if( (error_desc = AESDK_OpenGL_MakeReadyToRender(S_MCSDF_EffectCommonData)) != AESDK_OpenGL_OK)
			{
				PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
				CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
			}

			//unbind all textures
			glBindTexture(GL_TEXTURE_2D, 0);

			//set the matrix modes
			glMatrixMode( GL_PROJECTION );
			glLoadIdentity();
            gluOrtho2D(-0.6, -0.4, -0.6, -0.4);
			
			// Set up the frame-buffer object just like a window.
			glViewport( 0, 0, in_data->width, in_data->height );
			glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			
			//spin using slider [TODO] as slider control
			glMatrixMode( GL_MODELVIEW );
			glLoadIdentity();
			glTranslatef( 0.5f - (fpshort)sliderVal, 0.5f - (fpshort)sliderVal2, 0.0f);
			
			glBindTexture( GL_TEXTURE_2D, S_MCSDF_InputFrameTextureIDSu );
			if( S_MCSDF_EffectCommonData.mUsingShaderB )
			{
				AESDK_OpenGL_StartRenderToShader(S_MCSDF_EffectCommonData);

				// Identify the texture to use and bind it to texture unit 0
				if( (error_desc = AESDK_OpenGL_BindTextureToTarget(S_MCSDF_EffectCommonData, S_MCSDF_InputFrameTextureIDSu, string("videoTexture"))) != AESDK_OpenGL_OK)
				{
					PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
					CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
				}
			}
			else
			{
				glBindTexture( GL_TEXTURE_2D, S_MCSDF_InputFrameTextureIDSu );
			}
            
            glUniform1fARB(uniformWeightLocation, (fpshort)sliderVal3);
			
			//Render the geometry to the frame-buffer object
//			fpshort aspectF = (static_cast<fpshort>(widthL))/heightL;
			glBegin(GL_QUADS); //input frame
				glTexCoord2f(0.0f,0.0f);	glVertex3f(-1.0f,-1.0f,0.0f);
				glTexCoord2f(1.0f,0.0f);	glVertex3f(1.0f,-1.0f,0.0f);
				glTexCoord2f(1.0f,1.0f);	glVertex3f(1.0f,1.0f,0.0f);
				glTexCoord2f(0.0f,1.0f);	glVertex3f(-1.0f,1.0f,0.0f);
			glEnd();
			
			if( S_MCSDF_EffectCommonData.mUsingShaderB )
			{
				AESDK_OpenGL_StopRenderToShader();
			}

			glFlush();	
			
			// Check for errors...
			string error_msg;
			if( (error_msg = CheckFramebufferStatus()) != string("OK"))
			{
				PF_SPRINTF(out_data->return_msg, error_msg.c_str());
				CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
			}
			glFlush();
			
			//download from texture memory onto the same surface
			glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
			glReadPixels(0, 0, S_MCSDF_EffectCommonData.mRenderBufferWidthSu, S_MCSDF_EffectCommonData.mRenderBufferHeightSu, GL_RGBA, GL_UNSIGNED_BYTE, bufferP);
			
			//copy to openGL_world
			for (int ix=0; ix < openGL_world.height; ++ix)
			{
				PF_Pixel8 *pixelDataStart = NULL;
				PF_GET_PIXEL_DATA8( &openGL_world , NULL, &pixelDataStart);
				::memcpy(	pixelDataStart + (ix * openGL_world.rowbytes/sizeof(GL_RGBA)),
						bufferP + (ix * S_MCSDF_EffectCommonData.mRenderBufferWidthSu ),
						openGL_world.width * sizeof(GL_RGBA));
			}
			
			//clean the data after being copied
			suites.HandleSuite1()->host_unlock_handle(bufferH);
			suites.HandleSuite1()->host_dispose_handle(bufferH);
		}
		else
		{
			CHECK(PF_Err_OUT_OF_MEMORY);
		}
		
		if (PF_Quality_HI == in_data->quality) {
			ERR(suites.WorldTransformSuite1()->copy_hq(	in_data->effect_ref,
														&openGL_world,
														output,
														NULL,
														NULL));
		}
		else
		{
			ERR(suites.WorldTransformSuite1()->copy(	in_data->effect_ref,
														&openGL_world,
														output,
														NULL,
														NULL));

		}
		
		// This must be called before PF_ABORT because After Effects may
		// use the opportunity to draw something and it will expect its context
		// to be there.  If you have PF_ABORT or PF_PROG higher up, you must set
		// the AE context back before calling them, and then take it back again
		// if you want to call some more OpenGL.
		SetHostContext(S_MCSDF_EffectCommonData);
		
		ERR( suites.WorldSuite1()->dispose_world( in_data->effect_ref, &openGL_world));
		ERR(PF_ABORT(in_data));
	}
	catch(PF_Err& thrown_err)
	{
		err = thrown_err;
		
		SetHostContext(S_MCSDF_EffectCommonData);
	}
	catch(...)
	{
		SetHostContext(S_MCSDF_EffectCommonData);
	}
	
	return err;
}

DllExport	
PF_Err 
EntryPointFunc (
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:
				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:
				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:
				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:
				err = Render(		in_data,
								out_data,
								params,
								output);
				break;

			case PF_Cmd_GLOBAL_SETDOWN:
				err = GlobalSetdown(	in_data,
										out_data,
										params,
										output);
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

//helper function
string CreateShaderPath( string inPluginPath, string inShaderFileName )
{
	string::size_type pos;
	string shaderPath;
#ifdef AE_OS_WIN
	//delete the plugin name
	pos = inPluginPath.rfind("\\",inPluginPath.length());
	shaderPath = inPluginPath.substr( 0, pos);
	shaderPath = shaderPath + string("\\") + inShaderFileName;
#elif defined(AE_OS_MAC)
#if __LP64__
	const char *delim = "/";
#else
	const char *delim = ":";
#endif

	//delete the plugin name
	pos = inPluginPath.rfind(delim,inPluginPath.length());
	shaderPath = inPluginPath.substr( 0, pos);
	//delete the parent volume
	pos = shaderPath.find_first_of( string(delim), 0);
	shaderPath.erase( 0, pos);
	//next replace all the colons with slashes
	while( string::npos != (pos = inPluginPath.find( string(":"), 0)) )
	{
		shaderPath.replace(pos, 1, string("/"));
	}
	shaderPath = shaderPath + string(delim) + inShaderFileName;
#endif
	return shaderPath;
}