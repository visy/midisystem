// MIDISYS-ENGINE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef  __APPLE__
#include <malloc.h>
#endif
#include "midifile.h"
#include "midiutil.h"

#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "SOIL.h"

#include "bass.h"

#include "ggets.h"
#include "ggets.c"

// debug

int mouseX;
int mouseY;

// shaders

GLuint eye_shaderProg;
GLuint eye_post_shaderProg;
GLuint fsquad_shaderProg;
GLuint redcircle_shaderProg;

// fbo

GLuint fb;
GLuint fb_tex;
GLuint depth_rb = 0;

// textures

int grayeye_tex = -1;
int room_tex[3] = {-1};
int majictext1_tex = -1;

// texture switchers

int room_texnum = 0;

// assimp scene

const struct aiScene* scene = NULL;
GLuint scene_list = 0;
struct aiVector3D scene_min, scene_max, scene_center;

static float kujalla_angle = 0.f;

// assimp defines

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

// misc. gfx system

static GLfloat g_nearPlane = 1;
static GLfloat g_farPlane = 1000;
static int g_Width = 600;
static int g_Height = 600;

void InitFBO();

int frame = 0;

// effect pointers && logic && state

int dt = 0;
int current_scene = 0;

void dummy() {}

void EyeScene();
void RedCircleScene();
void KolmeDeeScene();

void KolmeDeeLogic();

typedef void (*SceneRenderCallback)();
SceneRenderCallback scene_render[] = {
										&KolmeDeeScene,
										&EyeScene, 
										&RedCircleScene
									 };

typedef void (*SceneLogicCallback)();
SceneLogicCallback scene_logic[] = {
										&KolmeDeeLogic,
										&dummy, 
										&dummy
									 };

// midi sync

MIDI_MSG timeline[64][100000];
char timeline_trackname[64][512];
int timeline_trackindex[64] = { 0 };
int timeline_tracklength[64] = { -1 };
int timeline_trackcount = 0;

// midi track number of mapping data
int mapping_tracknum[1000];
// midi to shader param map from mapping.txt
int mapping_paramnum[1000];
// track to map from
char mapping_trackname[1000][512];
// map type: 0 == trig (noteon / off), 1 == param (modwheel / cc value)
int mapping_type[1000];
// number of active mappings from midi to shader param
int mapping_count = 0;

// current shader param values 
int scene_shader_params[16] = {-1};
int scene_shader_param_type[16] = {-1};

// audio

float millis = 0;
float scene_start_millis = 0;

int music_started = -1;

DWORD music_channel = 0;

///////////////////////////////////////////////////// HELPER FUNCTIONS

void BassError(const char *text) 
{
	printf("BASS error(%d): %s\n",BASS_ErrorGetCode(),text);
	BASS_Free();
	exit(0);
}

void InitAudio(const char *pFilename)
{
	DWORD chan,act,time,level;
	QWORD pos;

	printf("--- MIDISYS-ENGINE: InitAudio()\n");

	if (!BASS_Init(-1,44100,0,0,NULL)) BassError("InitAudio() - can't initialize device");

	printf("\tInitAudio() - loading soundtrack from file \"%s\"\n", pFilename);

	int success = 0;

	if ((chan=BASS_StreamCreateFile(FALSE,pFilename,0,0,BASS_STREAM_PRESCAN))
		|| (chan=BASS_StreamCreateURL(pFilename,0,BASS_STREAM_PRESCAN,0,0))) {
		pos=BASS_ChannelGetLength(chan,BASS_POS_BYTE);
		if (BASS_StreamGetFilePosition(chan,BASS_FILEPOS_DOWNLOAD)!=-1) {
			// streaming from the internet
			if (pos!=-1)
			{
				printf("\tInitAudio() - streaming internet file [%I64u bytes]\n",pos);
				success = 1;
			}
			else
			{
				printf("\tstreaming internet file\n");
				success = 1;
			}
		} else
			{
				printf("\tstreaming file [%I64u bytes]\n",pos);
				success = 1;
			}
	}

	if (success == 1)
	{
		music_channel = chan;
	}
	else
	{
		printf("InitAudio() error! could not load file.");
		exit(1);
	}
}

void HexList(BYTE *pData, int iNumBytes)
{
	int i;

	for(i=0;i<iNumBytes;i++)
		printf("%.2x ", pData[i]);
}

char *File2String(const char *path)
{
	FILE *fd;
	long len,
		 r;
	char *str;
 
	if (!(fd = fopen(path, "r")))
	{
		fprintf(stderr, "\terror, can't open file '%s' for reading\n", path);
		return NULL;
	}
 
	fseek(fd, 0, SEEK_END);
	len = ftell(fd);
 
 	#ifdef SUPERVERBOSE
	printf("\tshader source file \"%s\" is %ld bytes long\n", path, len);
 	#endif

	fseek(fd, 0, SEEK_SET);
 
	if (!(str = malloc(len * sizeof(char))))
	{
		fprintf(stderr, "\terror, can't malloc space for file \"%s\"\n", path);
		return NULL;
	}
 
	r = fread(str, sizeof(char), len, fd);
 
	str[r - 1] = '\0'; /* Shader sources have to term with null */
 
	fclose(fd);
 
	return str;
}

void PrintShaderLog(GLuint obj)
{
	int infologLength = 0;
	int maxLength;
 
	if(glIsShader(obj))
		glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
	else
		glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
 
	char infoLog[maxLength];
 
	if (glIsShader(obj))
		glGetShaderInfoLog(obj, maxLength, &infologLength, infoLog);
	else
		glGetProgramInfoLog(obj, maxLength, &infologLength, infoLog);
 
	if (infologLength > 0)
	{
		printf("%s\n",infoLog);
	}
}

void DebugPrintEvent(int ev, MIDI_MSG msg)
{
	char str[128];
	unsigned int j;
	if (muGetMIDIMsgName(str, ev))	printf("%s\t", str);

	switch(ev)
		{
		case	msgNoteOff:
				muGetNameFromNote(str, msg.MsgData.NoteOff.iNote);
				printf("(%.2d) %s", msg.MsgData.NoteOff.iChannel, str);
				break;
		case	msgNoteOn:
				muGetNameFromNote(str, msg.MsgData.NoteOn.iNote);
				printf("\t(%.2d) %s %d", msg.MsgData.NoteOn.iChannel, str, msg.MsgData.NoteOn.iVolume);
				break;
		case	msgNoteKeyPressure:
				muGetNameFromNote(str, msg.MsgData.NoteKeyPressure.iNote);
				printf("(%.2d) %s %d", msg.MsgData.NoteKeyPressure.iChannel, 
						str,
						msg.MsgData.NoteKeyPressure.iPressure);
				break;
		case	msgSetParameter:
				muGetControlName(str, msg.MsgData.NoteParameter.iControl);
				printf("(%.2d) %s -> %d", msg.MsgData.NoteParameter.iChannel, 
						str, msg.MsgData.NoteParameter.iParam);
				break;
		case	msgSetProgram:
				muGetInstrumentName(str, msg.MsgData.ChangeProgram.iProgram);
				printf("(%.2d) %s", msg.MsgData.ChangeProgram.iChannel, str);
				break;
		case	msgChangePressure:
				muGetControlName(str, msg.MsgData.ChangePressure.iPressure);
				printf("(%.2d) %s", msg.MsgData.ChangePressure.iChannel, str);
				break;
		case	msgSetPitchWheel:
				printf("(%.2d) %d", msg.MsgData.PitchWheel.iChannel,  
						msg.MsgData.PitchWheel.iPitch);
				break;

		case	msgMetaEvent:
				printf("---- ");
				switch(msg.MsgData.MetaEvent.iType)
					{
					case	metaMIDIPort:
							printf("MIDI Port = %d", msg.MsgData.MetaEvent.Data.iMIDIPort);
							break;

					case	metaSequenceNumber:
							printf("Sequence Number = %d",msg.MsgData.MetaEvent.Data.iSequenceNumber);
							break;

					case	metaTextEvent:
							printf("Text = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
					case	metaCopyright:
							printf("Copyright = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
					case	metaTrackName:
							printf("Track name = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
					case	metaInstrument:
							printf("Instrument = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
					case	metaLyric:
							printf("Lyric = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
					case	metaMarker:
							printf("Marker = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
					case	metaCuePoint:
							printf("Cue point = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
					case	metaEndSequence:
							printf("End Sequence");
							break;
					case	metaSetTempo:
							printf("Tempo = %d",msg.MsgData.MetaEvent.Data.Tempo.iBPM);
							break;
					case	metaSMPTEOffset:
							printf("SMPTE offset = %d:%d:%d.%d %d",
									msg.MsgData.MetaEvent.Data.SMPTE.iHours,
									msg.MsgData.MetaEvent.Data.SMPTE.iMins,
									msg.MsgData.MetaEvent.Data.SMPTE.iSecs,
									msg.MsgData.MetaEvent.Data.SMPTE.iFrames,
									msg.MsgData.MetaEvent.Data.SMPTE.iFF
									);
							break;
					case	metaTimeSig:
							printf("Time sig = %d/%d",msg.MsgData.MetaEvent.Data.TimeSig.iNom,
										msg.MsgData.MetaEvent.Data.TimeSig.iDenom/MIDI_NOTE_CROCHET);
							break;
					case	metaKeySig:
							if (muGetKeySigName(str, msg.MsgData.MetaEvent.Data.KeySig.iKey))
								printf("Key sig = %s", str);
							break;

					case	metaSequencerSpecific:
							printf("Sequencer specific = ");
							HexList(msg.MsgData.MetaEvent.Data.Sequencer.pData, msg.MsgData.MetaEvent.Data.Sequencer.iSize);
							break;
					}
				break;

		case	msgSysEx1:
		case	msgSysEx2:
				printf("Sysex = ");
				HexList(msg.MsgData.SysEx.pData, msg.MsgData.SysEx.iSize);
				break;
		}

	if (ev == msgSysEx1 || ev == msgSysEx1 || (ev==msgMetaEvent && msg.MsgData.MetaEvent.iType==metaSequencerSpecific))
		{
		/* Already done a hex dump */
		}
	else
		{
		printf("\t[");
		if (msg.bImpliedMsg) printf("%.2x!", msg.iImpliedMsg);
		for(j=0;j<msg.iMsgSize;j++)
			printf("%.2x ", msg.data[j]);
		printf("]\n");
		}

}

void LoadMIDIEventList(const char *pFilename)
{
	printf("--- MIDISYS ENGINE: LoadMIDIEventList(\"%s\")\n", pFilename);
	MIDI_FILE *mf = midiFileOpen(pFilename);
	char str[128];
	int ev;

	int timeline_index = 0;
	int track_index = 0;
	MIDI_MSG msg;

	if (mf)
	{
		int i, iNum;
		unsigned int j;

		midiReadInitMessage(&msg);
		iNum = midiReadGetNumTracks(mf);

		for(i=0;i<iNum;i++)
		{
			#ifdef SUPERVERBOSE 
			printf("# Track %d\n", i);
			#endif
			while(midiReadGetNextMessage(mf, i, &msg))
			{
				#ifdef SUPERVERBOSE 
				printf(" %.6ld ", msg.dwAbsPos);
				#endif

				if (msg.bImpliedMsg) { ev = msg.iImpliedMsg; }
				else { ev = msg.iType; }

				memcpy(&timeline[track_index][timeline_index], &msg, sizeof(MIDI_MSG));

				if (ev == msgMetaEvent && msg.MsgData.MetaEvent.iType == metaTrackName) 
				{
					strncpy(timeline_trackname[track_index], msg.MsgData.MetaEvent.Data.Text.pData, 8);
					timeline_trackname[track_index][8] == '\0';
					printf("track #%d, name = \"%s\"\n", track_index, timeline_trackname[track_index]);
				}

				#ifdef SUPERVERBOSE 
				DebugPrintEvent(ev,msg); 
				#endif

				timeline_index++;

			}
			printf("track length: %d\n", timeline_index);
			timeline_tracklength[track_index] = timeline_index;
			track_index++;
			timeline_index = 0;
		}

		timeline_trackcount = track_index;
		midiReadFreeMessage(&msg);
		midiFileClose(mf);
	}

	//timeline_length = timeline_index+1;
	printf("--- MIDISYS ENGINE: LoadMIDIEventList() success\n");
}

void ParseMIDITimeline(const char* mappingFile)
{
	printf("--- MIDISYS ENGINE: ParseMIDITimeline(\"%s\")\n", mappingFile);
	int cnt;
	char *line;

	FILE *mapfile;

	if ((mapfile = fopen(mappingFile, "r"))) 
	{
		cnt = 0;
		while (0 == fggets(&line, mapfile)) {
			char* token = strtok(line, ",");
			int paramnum = atoi(token);
			mapping_paramnum[cnt] = paramnum;

			token = strtok(NULL, ",");
			sprintf(mapping_trackname[cnt],"%s",token);

			token = strtok(NULL, ",");
			if (strcmp("param",token) == 0) mapping_type[cnt] = 1;
			else mapping_type[cnt] = 0;

//			token = strtok(NULL, ",");
//			sprintf(englines[cnt],"%s",token);

			int i = 0;
			int found = -1;
			for (i = 0; i < timeline_trackcount; i++)
			{
				if (strcmp(timeline_trackname[i], mapping_trackname[cnt]) == 0) 
				{
					printf("MIDI track #%d (\"%s\") maps to shader param %d, type: %s\n", i, mapping_trackname[cnt], mapping_paramnum[cnt], mapping_type[cnt] == 0 ? "trig" : "param");
					mapping_tracknum[cnt] = i;
					cnt++;
					found = 1;
				}
				if (found == 1) break;
			}	
		}
		mapping_count = cnt;
	}
	else
	{
		printf("ParseMIDITimeline error: MIDI mapping file \"%s\" cannot be opened\n", mappingFile);
		exit(1);
	}
	printf("--- MIDISYS ENGINE: ParseMIDITimeline() success\n");
}

///////////////////////////////////////////////////////////////// EFFECTS
///////////////////////////////////////////////////////////////// EFFECTS
///////////////////////////////////////////////////////////////// EFFECTS
///////////////////////////////////////////////////////////////// EFFECTS
///////////////////////////////////////////////////////////////// EFFECTS

void KolmeDeeLogic()
{
	kujalla_angle += dt*0.01;
}

void KolmeDeeScene()
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0); // Uses default lighting parameters

	glEnable(GL_DEPTH_TEST);

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_NORMALIZE);

	// XXX docs say all polygons are emitted CCW, but tests show that some aren't.
	if(getenv("MODEL_IS_BROKEN"))
	glFrontFace(GL_CW);

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

	float tmp;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(0.f,0.f,3.f,0.f,0.f,-5.f,0.f,1.f,0.f);

	gluLookAt(0., 0., 3.f-((float)mouseX/(g_Width))*10.0f, 
    	      0, 0, -5.f,
    	      0, 1, 0); 

	// rotate it around the y axis
	glRotatef(kujalla_angle,0.f,1.f,0.f);

	// scale the whole asset to fit into our view frustum
	tmp = scene_max.x-scene_min.x;
	tmp = aisgl_max(scene_max.y - scene_min.y,tmp);
	tmp = aisgl_max(scene_max.z - scene_min.z,tmp);
	tmp = 1.f / tmp;
	glScalef(tmp, tmp, tmp);

    // center the model
	glTranslatef( -scene_center.x, -scene_center.y, -scene_center.z );

    // if the display list has not been made yet, create a new one and
    // fill it with scene contents
	if(scene_list == 0) {
		scene_list = glGenLists(1);
		glNewList(scene_list, GL_COMPILE);
		// now begin at the root node of the imported data and traverse
		// the scenegraph by multiplying subsequent local transforms
		// together on GL's matrix stack.
		recursive_render(scene, scene->mRootNode);
		glEndList();
	}

	glCallList(scene_list);
}


int beatmode = -1;

void EyeScene()
{
	int i, j;

	float mymillis = ((millis-scene_start_millis)*100);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb); // fbo

	glClearColor((float)(scene_shader_params[1]/127)*0.7,(float)(scene_shader_params[1]/127)*0.4,(float)(scene_shader_params[1]/127)*0.8,0.9-0.005*(float)(scene_shader_params[1]/127));

	if (scene_shader_params[0] == 65 && scene_shader_param_type[0] == 0)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		beatmode = -beatmode;
	}

	// render fbo copy to fbo
	glEnable(GL_BLEND);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_DST_COLOR);

	glUseProgram(eye_post_shaderProg);

//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLint widthLoc3 = glGetUniformLocation(eye_post_shaderProg, "width");
	GLint heightLoc3 = glGetUniformLocation(eye_post_shaderProg, "height");
	GLint timeLoc3 = glGetUniformLocation(eye_post_shaderProg, "time");

	glUniform1f(widthLoc3, g_Width);
	glUniform1f(heightLoc3, g_Height);
	glUniform1f(timeLoc3, mymillis/100);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fb_tex);

	GLint location3 = glGetUniformLocation(eye_post_shaderProg, "texture0");
	glUniform1i(location3, 0);

	glLoadIdentity();

	glTranslatef(-1.2, -1.0, -1.0+cos(mymillis*0.00001)*0.1);

	i=0;
	j=0;
	glBegin(GL_QUADS);
	glVertex2f(i, j);
	glVertex2f(i + 100, j);
	glVertex2f(i + 100, j + 100);
	glVertex2f(i, j + 100);
	glEnd();

	// render eye

	glEnable(GL_BLEND);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);

	glUseProgram(eye_shaderProg);

	GLfloat waveTime = 1+atan(mymillis*0.0001)*0.1,
			waveWidth = cos(mymillis*0.000001)*1000+atan(mymillis*0.00001)*2.0,
			waveHeight = sin(mymillis*0.0001)*100*atan(mymillis*0.00001)*2.0,
			waveFreq = 0.0001+cos(mymillis*0.000003)*1000.1;
	GLint waveTimeLoc = glGetUniformLocation(eye_shaderProg, "waveTime");
	GLint waveWidthLoc = glGetUniformLocation(eye_shaderProg, "waveWidth");
	GLint waveHeightLoc = glGetUniformLocation(eye_shaderProg, "waveHeight");

	GLint widthLoc = glGetUniformLocation(eye_shaderProg, "width");
	GLint heightLoc = glGetUniformLocation(eye_shaderProg, "height");

	GLint timeLoc = glGetUniformLocation(eye_shaderProg, "time");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grayeye_tex);

	GLuint location;
	location = glGetUniformLocation(eye_shaderProg, "texture0");
	glUniform1i(location, 0);

	glLoadIdentity();

	glTranslatef(0.0, 0.0, -150.0+sin(mymillis*0.0000004)*120);
	glRotatef(-75.0, 1.0, 0.0, 0.0);
	glRotatef(mymillis*0.01, 0.0, 0.0, 1.0);

	glUniform1f(waveTimeLoc, waveTime);
	glUniform1f(waveWidthLoc, waveWidth);
	glUniform1f(waveHeightLoc, waveHeight);

	glUniform1f(widthLoc, g_Width);
	glUniform1f(heightLoc, g_Height);

	glUniform1f(timeLoc, mymillis/100);


	int zoom = 0;

	for (zoom = 0; zoom < 8; zoom++)
	{

	glTranslatef(-0.01, -0.01, -0.1);
	glRotatef((90*zoom+mymillis*0.01)*0.1, 1.0, 0.0, 0.0);
	glRotatef((45*zoom+mymillis*0.01)*0.1, 0.0, 0.0, 1.0);
	glRotatef((25*zoom+mymillis*0.01)*0.1, 0.0, 1.0, 0.0);
	glBegin(GL_QUADS);

	for (i = -50; i < 50; i+=10)
		for (j = -50; j < 50; j+=10)
		{
			glVertex2f(i, j);
			glVertex2f(i + 1, j);
			glVertex2f(i + 1, j + 1);
			glVertex2f(i, j + 1);
		}
	glEnd();
	}

	// eye postprocess to screen

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); // default

	glUseProgram(eye_post_shaderProg);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_BLEND);

	GLint widthLoc2 = glGetUniformLocation(eye_post_shaderProg, "width");
	GLint heightLoc2 = glGetUniformLocation(eye_post_shaderProg, "height");
	GLint timeLoc2 = glGetUniformLocation(eye_post_shaderProg, "time");

	glUniform1f(widthLoc2, g_Width);
	glUniform1f(heightLoc2, g_Height);
	glUniform1f(timeLoc2, mymillis/100);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fb_tex);

	if (scene_shader_params[0] != -1 && scene_shader_param_type[0] == 0)
	{
		room_texnum = (int)(rand() % 3);
	}

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, room_tex[room_texnum]);

	GLint location2 = glGetUniformLocation(eye_post_shaderProg, "texture0");
	glUniform1i(location2, 0);

	GLint location4 = glGetUniformLocation(eye_post_shaderProg, "texture1");
	glUniform1i(location4, 1);

	glLoadIdentity();

	glTranslatef(-1.2, -1.0, -1.0);

	i=0;
	j=0;
	glBegin(GL_QUADS);
	glVertex2f(i, j);
	glVertex2f(i + 100, j);
	glVertex2f(i + 100, j + 100);
	glVertex2f(i, j + 100);
	glEnd();

	// text overlay

	if (beatmode == 1)
	{
		glUseProgram(fsquad_shaderProg);

		glEnable(GL_BLEND);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, beatmode == 1 ? GL_SUBTRACT : GL_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR);

		GLint widthLoc5 = glGetUniformLocation(fsquad_shaderProg, "width");
		GLint heightLoc5 = glGetUniformLocation(fsquad_shaderProg, "height");
		GLint timeLoc5 = glGetUniformLocation(fsquad_shaderProg, "time");
		GLint alphaLoc5 = glGetUniformLocation(fsquad_shaderProg, "alpha");

		glUniform1f(widthLoc5, g_Width);
		glUniform1f(heightLoc5, g_Height);
		glUniform1f(timeLoc5, mymillis/100);
		glUniform1f(alphaLoc5, cos(mymillis*0.1)*0.1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, majictext1_tex);

		GLint location5 = glGetUniformLocation(fsquad_shaderProg, "texture0");
		glUniform1i(location5, 0);

		glLoadIdentity();

		glTranslatef(-1.2, -1.0, -1.0);

		i=0;
		j=0;
		glBegin(GL_QUADS);
		glVertex2f(i, j);
		glVertex2f(i + 100, j);
		glVertex2f(i + 100, j + 100);
		glVertex2f(i, j + 100);
		glEnd();
	}

}

void RedCircleScene()
{
	glUseProgram(redcircle_shaderProg);
	float mymillis = (millis-scene_start_millis)*300;

	if (frame % 500 == 1) glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLint widthLoc2 = glGetUniformLocation(redcircle_shaderProg, "width");
	GLint heightLoc2 = glGetUniformLocation(redcircle_shaderProg, "height");

	glUniform1f(widthLoc2, g_Width);
	glUniform1f(heightLoc2, g_Height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grayeye_tex);

	GLint location2 = glGetUniformLocation(redcircle_shaderProg, "texture0");
	glUniform1i(location2, 0);

	glLoadIdentity();

	glTranslatef(0.0, 0.0, -90.0-cos(mymillis*0.0000004)*120);
	glRotatef(-75.0, 1.0, 0.0, 0.0);
	glRotatef(mymillis*0.01, 0.0, 0.0, 1.0);

	glBegin(GL_QUADS);

	int i,j;

	for (i = -50; i < 50; i+=10)
		for (j = -50; j < 50; j+=10)
		{

			glVertex2f(i, j);
			glVertex2f(i + 1, j);
			glVertex2f(i + 1, j + 1);
			glVertex2f(i, j + 1);
		}
	glEnd();

}

///////////////////////////////////////////////////////////////// END EFFECTS
///////////////////////////////////////////////////////////////// END EFFECTS
///////////////////////////////////////////////////////////////// END EFFECTS
///////////////////////////////////////////////////////////////// END EFFECTS
///////////////////////////////////////////////////////////////// END EFFECTS

// update sync from midi + mapping data

void UpdateShaderParams()
{
	int intmillis = (int)millis+(155520*1.212);

	int i,j = 0;

	for (i=0; i < mapping_count; i++)
	{
		int tracknum = mapping_tracknum[i];
		int trackidx = timeline_trackindex[tracknum];
		MIDI_MSG currentMsg = timeline[tracknum][trackidx];

		// reset trigs
		if (scene_shader_param_type[mapping_paramnum[i]] == 0) scene_shader_params[mapping_paramnum[i]] = -1;

		int dw = (int)currentMsg.dwAbsPos;
		int tarkistus = (int)(dw);

		//if (intmillis+155520 < tarkistus*1.212) break;
		if (intmillis < tarkistus*1.212) break;

		timeline_trackindex[tracknum]++;

		if (timeline_trackindex[tracknum] >= timeline_tracklength[tracknum]) { timeline_trackindex[tracknum] = 0; printf("track reset\n"); }

		int ev = 0;

		if (currentMsg.bImpliedMsg) { ev = currentMsg.iImpliedMsg; }
		else { ev = currentMsg.iType; }

		int trigVal = -1;
		int paramVal = -1;

		switch(mapping_type[i])
		{
			// trig
			case 0:
			{
				if (ev == msgNoteOn)
				{
					trigVal = currentMsg.MsgData.NoteOn.iNote;
					printf("noteon track #%d, len %d, pos %d: time: %d -> %d\n", tracknum, timeline_tracklength[tracknum], timeline_trackindex[tracknum], currentMsg.dwAbsPos, trigVal);
				}
				else if (ev == msgNoteOff)
				{
					trigVal = -1;
				}

				scene_shader_params[mapping_paramnum[i]] = trigVal;
				scene_shader_param_type[mapping_paramnum[i]] = 0;
				//printf("sync: trig #%d to: %d\n", mapping_paramnum[i], trigVal);
				break;
			}

			// param:
			case 1:
			{
				if (ev == msgSetParameter)
				{
					paramVal = currentMsg.MsgData.NoteParameter.iParam;
					printf("setparam track #%d, len %d, pos %d: time: %d -> %d\n", tracknum, timeline_tracklength[tracknum], timeline_trackindex[tracknum], currentMsg.dwAbsPos, paramVal);
				}

				scene_shader_params[mapping_paramnum[i]] = paramVal;
				scene_shader_param_type[mapping_paramnum[i]] = 1;

				//printf("sync: param #%d to: %d\n", mapping_paramnum[i], trigVal);
				break;
			}
		}
	}


}

///////////////////////////////////////////////////////////////// MAIN LOGIC

void logic()
{ 	
	QWORD bytepos = BASS_ChannelGetPosition(music_channel, BASS_POS_BYTE);
	double pos = BASS_ChannelBytes2Seconds(music_channel, bytepos);
	millis = (float)pos*1000;

	static GLint prev_time = 0;
	static GLint prev_fps_time = 0;
	static int frames = 0;

	scene_logic[current_scene]();

	int time = (int)millis;
	dt = time-prev_time;

	prev_time = time;

	frames += 1;
	if ((time - prev_fps_time) > 1000) // update every seconds
    {
	        int current_fps = frames * 1000 / (time - prev_fps_time);
	        printf("%d fps\n", current_fps);
	        frames = 0;
	        prev_fps_time = time;
    }

	if (music_started == -1) { BASS_ChannelPlay(music_channel,FALSE); music_started = 1; }

	//printf("millis:%f\n", millis);

	UpdateShaderParams();

	glutPostRedisplay();
}

///////////////////////////////////////////////////////////// RENDER FUNCTION

void display(void)
{
	scene_render[current_scene]();
	glutSwapBuffers();
	frame++;
	logic();
}

void reshape(GLint width, GLint height)
{
	g_Width = width;
	g_Height = height;

	printf("--- MIDISYS ENGINE: reshape event: %dx%d\n", (int)width, (int)height);

	glViewport(0, 0, g_Width, g_Height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(65.0, (float)g_Width / g_Height, g_nearPlane, g_farPlane);
	glMatrixMode(GL_MODELVIEW);

	glDeleteTextures(1, &fb_tex);
	glDeleteRenderbuffersEXT(1, &depth_rb);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glDeleteFramebuffersEXT(1, &fb);

	InitFBO();
}

void get_bounding_box_for_node (const struct aiNode* nd, struct aiVector3D* min, struct aiVector3D* max, struct aiMatrix4x4* trafo )
{
	struct aiMatrix4x4 prev;
	unsigned int n = 0, t;

	prev = *trafo;
	aiMultiplyMatrix4(trafo,&nd->mTransformation);

	for (; n < nd->mNumMeshes; ++n) {
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t) {
			struct aiVector3D tmp = mesh->mVertices[t];
			aiTransformVecByMatrix4(&tmp,trafo);

			min->x = aisgl_min(min->x,tmp.x);
			min->y = aisgl_min(min->y,tmp.y);
			min->z = aisgl_min(min->z,tmp.z);

			max->x = aisgl_max(max->x,tmp.x);
			max->y = aisgl_max(max->y,tmp.y);
			max->z = aisgl_max(max->z,tmp.z);
		}
	}

	for (n = 0; n < nd->mNumChildren; ++n) {
		get_bounding_box_for_node(nd->mChildren[n],min,max,trafo);
	}
	*trafo = prev;
}

// ----------------------------------------------------------------------------
void get_bounding_box (struct aiVector3D* min, struct aiVector3D* max)
{
	struct aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);

	min->x = min->y = min->z = 1e10f;
	max->x = max->y = max->z = -1e10f;
	get_bounding_box_for_node(scene->mRootNode,min,max,&trafo);
}

// ----------------------------------------------------------------------------
void color4_to_float4(const struct aiColor4D *c, float f[4])
{
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}

// ----------------------------------------------------------------------------
void set_float4(float f[4], float a, float b, float c, float d)
{
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}

// ----------------------------------------------------------------------------
void apply_material(const struct aiMaterial *mtl)
{
	float c[4];

	GLenum fill_mode;
	int ret1, ret2;
	struct aiColor4D diffuse;
	struct aiColor4D specular;
	struct aiColor4D ambient;
	struct aiColor4D emission;
	float shininess, strength;
	int two_sided;
	int wireframe;
	unsigned int max;

	set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
	if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
	color4_to_float4(&diffuse, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

	set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
	if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
	color4_to_float4(&specular, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

	set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
	if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
	color4_to_float4(&ambient, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

	set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
	if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
	color4_to_float4(&emission, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);

	max = 1;
	ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
	if(ret1 == AI_SUCCESS) {
	     max = 1;
	     ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
			if(ret2 == AI_SUCCESS)
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
	        else
	         glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
	    }
	else {
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
	set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
	}

	max = 1;
	if(AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, &max))
	fill_mode = wireframe ? GL_LINE : GL_FILL;
	else
	fill_mode = GL_FILL;
	glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

	max = 1;
	if((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
	glDisable(GL_CULL_FACE);
	else
	glEnable(GL_CULL_FACE);
}


// ----------------------------------------------------------------------------
void recursive_render (const struct aiScene *sc, const struct aiNode* nd)
{
	unsigned int i;
	unsigned int n = 0, t;
	struct aiMatrix4x4 m = nd->mTransformation;

	// update transform
	aiTransposeMatrix4(&m);
	glPushMatrix();
	glMultMatrixf((float*)&m);

	// draw all meshes assigned to this node
	for (; n < nd->mNumMeshes; ++n) {
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

		apply_material(sc->mMaterials[mesh->mMaterialIndex]);

		if(mesh->mNormals == NULL) {
			glDisable(GL_LIGHTING);
		} else {
			glEnable(GL_LIGHTING);
		}

		for (t = 0; t < mesh->mNumFaces; ++t) {
			const struct aiFace* face = &mesh->mFaces[t];
			GLenum face_mode;

			switch(face->mNumIndices) {
			case 1: face_mode = GL_POINTS; break;
			case 2: face_mode = GL_LINES; break;
			case 3: face_mode = GL_TRIANGLES; break;
			default: face_mode = GL_POLYGON; break;
		}

		glBegin(face_mode);

			for(i = 0; i < face->mNumIndices; i++) {
				int index = face->mIndices[i];
				if(mesh->mColors[0] != NULL)
				glColor4fv((GLfloat*)&mesh->mColors[0][index]);
				if(mesh->mNormals != NULL)
				glNormal3fv(&mesh->mNormals[index].x);
				glVertex3fv(&mesh->mVertices[index].x);
			}

		glEnd();
	}

	}

	// draw all children
	for (n = 0; n < nd->mNumChildren; ++n) {
		recursive_render(sc, nd->mChildren[n]);
	}

	glPopMatrix();
}

void quit()
{
	printf("--- MIDISYS ENGINE: time to quit()\n");
	glutLeaveMainLoop();
}

void keyPress(unsigned char key, int x, int y)
{
	if (key == VK_ESCAPE)
	{
		quit();
	}
}

void mouseMotion(int button, int state, int x, int y)
{
	mouseX = x;
	mouseY = y;
}

void InitFBO()
{
	glGenTextures(1, &fb_tex);
	glBindTexture(GL_TEXTURE_2D, fb_tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	printf("\tframebuffer size: %dx%d\n", g_Width, g_Height);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, g_Width, g_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glGenFramebuffersEXT(1, &fb);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fb_tex, 0);

	glGenRenderbuffersEXT(1, &depth_rb);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_rb);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, g_Width, g_Height);

	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth_rb);

	GLenum status;
	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	switch(status)
	{
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			printf("\tInitFBO() status: GL_FRAMEBUFFER_COMPLETE\n");
			break;
		default:
			printf("\tInitFBO() error: status != GL_FRAMEBUFFER_COMPLETE\n");
			exit(1);
			break;
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
}

void InitGraphics(int argc, char* argv[])
{
	fprintf(stdout, "--- MIDISYS ENGINE: InitGraphics()\n");
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInit(&argc, argv);
	glutCreateWindow("MIDISYS window");
	glutReshapeWindow(g_Width, g_Height);
	glutFullScreen();

	glutSetCursor(GLUT_CURSOR_NONE);

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
  		/* Problem: glewInit failed, something is seriously wrong. */
  		fprintf(stderr, "\tInitGraphics() error: %s\n", glewGetErrorString(err));
  		exit(1);
	}
	fprintf(stdout, "\tInitGraphics() status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	glEnable(GL_TEXTURE_2D);

	glShadeModel(GL_SMOOTH);

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(logic);
	glutKeyboardFunc(keyPress);
	glutMouseFunc(mouseMotion);

	fprintf(stdout, "--- MIDISYS ENGINE: InitGraphics() success\n");
}

GLuint LoadShader(const char* pFilename)
{
	fprintf(stdout,"--- MIDISYS ENGINE: LoadShader(\"%s\")", pFilename);

	#ifdef SUPERVERBOSE
	printf("\n");
	#endif

	char vsName[128] = "";
	strcpy(vsName, pFilename);
	strcat(vsName, ".vs");

	char fsName[128] = "";
	strcpy(fsName, pFilename);
	strcat(fsName, ".fs");

	#ifdef SUPERVERBOSE 
	fprintf(stdout,"\tLoadShader(\"%s\") vertex shader source file: \"%s\"\n", pFilename, vsName);
	#endif

	GLchar *vsSource = File2String(vsName);

	#ifdef SUPERVERBOSE 
	fprintf(stdout,"\tLoadShader(\"%s\") vertex shader source:\n----------------------------------------------------\n%s\n----------------------------------------------------\n", pFilename, vsSource);
	#endif

	#ifdef SUPERVERBOSE 
	fprintf(stdout,"\tLoadShader(\"%s\") fragment shader source file: \"%s\"\n", pFilename, fsName);
	#endif

	GLchar *fsSource = File2String(fsName);

	#ifdef SUPERVERBOSE 
	fprintf(stdout,"\tLoadShader(\"%s\") fragment shader source:\n----------------------------------------------------\n%s\n----------------------------------------------------\n", pFilename, fsSource);
	#endif

	GLuint vs, fs, sp;

	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): vs glCreateShader\n");
	#endif
	vs = glCreateShader(GL_VERTEX_SHADER);
	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): vs glShaderSource\n");
	#endif
	glShaderSource(vs, 1, (const GLchar**)&vsSource, NULL);
	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): vs glCompileShader\n");
	#endif
	glCompileShader(vs);
	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): vs compiled\n");
	#endif

	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): fs glCreateShader\n");
	#endif
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): fs glShaderSource\n");
	#endif
	glShaderSource(fs, 1, (const GLchar**)&fsSource, NULL);
	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): fs glCompileShader\n");
	#endif
	glCompileShader(fs);
	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): fs compiled\n");
	#endif

	free(vsSource);
	free(fsSource);

	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): glCreateProgram\n");
	#endif
	sp = glCreateProgram();
	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): glAttachShader vs\n");
	#endif
	glAttachShader(sp, vs);
	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): glAttachShader fs\n");
	#endif
	glAttachShader(sp, fs);
	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): glLinkProgram\n");
	#endif
	glLinkProgram(sp);
	PrintShaderLog(sp);

	#ifdef SUPERVERBOSE 
	printf("\tLoadShader(): glUseProgram\n");
	#endif
	glUseProgram(sp);
	PrintShaderLog(sp);

	#ifdef SUPERVERBOSE
	fprintf(stdout,"--- MIDISYS ENGINE: LoadShader(\"%s\") success\n", pFilename);
	#else
	printf(" success\n");
	#endif

	return sp;
}

GLuint LoadTexture(const char* pFilename)
{
	printf("--- MIDISYS ENGINE: LoadTexture(\"%s\")", pFilename);

	GLuint tex_2d = SOIL_load_OGL_texture
		(
			pFilename,
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
		);

	if(0 == tex_2d)
	{
		printf(" error loading texture from file \"%s\"\n", SOIL_last_result());
		exit(1);
	}

	printf(" success\n");

	return tex_2d;
}

int loadasset(const char* path)
{
	// we are taking one of the postprocessing presets to avoid
	// spelling out 20+ single postprocessing flags here.
	scene = aiImportFile(path,aiProcessPreset_TargetRealtime_MaxQuality);

	if (scene) {
		get_bounding_box(&scene_min,&scene_max);
		scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
		scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
		scene_center.z = (scene_min.z + scene_max.z) / 2.0f;
		return 0;
	}
	return 1;
}

void Load3DAsset(const char *assetFilename)
{
	printf("--- MIDISYS ENGINE: Load3DAsset(\"%s\") ", assetFilename);
	struct aiLogStream stream;
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);

	aiAttachLogStream(&stream);

	if(0 != loadasset(assetFilename)) 
	{
		printf("error loading 3d asset from file\n");
		exit(1);
	}

	printf(" success\n");
}


void StartMainLoop()
{
	printf("--- MIDISYS ENGINE: StartMainLoop()\n");
	glutMainLoop();
}

int main(int argc, char* argv[])
{
	printf("--- MIDISYS ENGINE: bilotrip foundation MIDISYS ENGINE 0.1 - dosing, please wait\n");
	
	// init graphics

	InitGraphics(argc, argv);

	// load shaders

	eye_shaderProg = LoadShader("eye");
	eye_post_shaderProg = LoadShader("eye_post");
	fsquad_shaderProg = LoadShader("fsquad");
	redcircle_shaderProg = LoadShader("redcircle");

	// load textures

	grayeye_tex = LoadTexture("grayeye.png");

	room_tex[0] = LoadTexture("room1.png");
	room_tex[1] = LoadTexture("room2.png");
	room_tex[2] = LoadTexture("room3.png");

	majictext1_tex = LoadTexture("majictext1.png");

	// load 3d assets

	Load3DAsset("templeton_peck.obj");

	// init MIDI sync and audio

	LoadMIDIEventList("music.mid");
	ParseMIDITimeline("mapping.txt");
	InitAudio("music.mp3");

	// start mainloop

	StartMainLoop();

	aiReleaseImport(scene);
	aiDetachAllLogStreams();

	return 0;
}
