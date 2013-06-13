// MIDISYS-ENGINE for windows

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef  __APPLE__
#include <malloc.h>
#endif
#include "midifile.h"
#include "midiutil.h"

#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#include "SOIL.h"

#include "bass.h"

GLuint test_shaderProg;

int grayeye_tex = -1;

MIDI_MSG timeline[1000000];
int timeline_length = 0;
int frame = 0;

static GLfloat g_nearPlane = 1;
static GLfloat g_farPlane = 1000;
static int g_Width = 600;
static int g_Height = 600;

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

	printf("InitAudio() - loading soundtrack from file \"%s\"\n", pFilename);

	int success = 0;

	if ((chan=BASS_StreamCreateFile(FALSE,pFilename,0,0,BASS_SAMPLE_LOOP))
		|| (chan=BASS_StreamCreateURL(pFilename,0,BASS_SAMPLE_LOOP,0,0))) {
		pos=BASS_ChannelGetLength(chan,BASS_POS_BYTE);
		if (BASS_StreamGetFilePosition(chan,BASS_FILEPOS_DOWNLOAD)!=-1) {
			// streaming from the internet
			if (pos!=-1)
			{
				printf("streaming internet file [%I64u bytes]",pos);
				success = 1;
			}
			else
			{
				printf("streaming internet file",pos);
				success = 1;
			}
		} else
			{
				printf("streaming file [%I64u bytes]",pos);
				success = 1;
			}
	}

	if (success == 1)
	{
		BASS_ChannelPlay(chan,FALSE);
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
		fprintf(stderr, "Can't open file '%s' for reading\n", path);
		return NULL;
	}
 
	fseek(fd, 0, SEEK_END);
	len = ftell(fd);
 
	printf("File '%s' is %ld bytes long\n", path, len);
 
	fseek(fd, 0, SEEK_SET);
 
	if (!(str = malloc(len * sizeof(char))))
	{
		fprintf(stderr, "Can't malloc space for '%s'\n", path);
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
		exit(1);
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
	printf("--- MIDISYS ENGINE: loading MIDI data from file \"%s\"\n", pFilename);
	MIDI_FILE *mf = midiFileOpen(pFilename);
	char str[128];
	int ev;

	int timeline_index = 0;

	if (mf)
	{
		MIDI_MSG msg;
		int i, iNum;
		unsigned int j;

		midiReadInitMessage(&msg);
		iNum = midiReadGetNumTracks(mf);

		for(i=0;i<iNum;i++)
		{
			printf("# Track %d\n", i);
			while(midiReadGetNextMessage(mf, i, &msg))
			{
				printf(" %.6ld ", msg.dwAbsPos);
				if (msg.bImpliedMsg) { ev = msg.iImpliedMsg; }
				else { ev = msg.iType; }

				timeline[timeline_index] = msg;
				timeline_index++;
				#ifdef SUPERVERBOSE 
				DebugPrintEvent(ev,msg); 
				#endif
			}
		}

		midiReadFreeMessage(&msg);
		midiFileClose(mf);
	}

	timeline_length = timeline_index+1;
	printf("\n--- MIDISYS ENGINE: midi data parsed\n");
	printf("--- MIDISYS ENGINE: demo timeline length: %d\n", timeline_length);
}

/*
void ParseEventList(const char *pFilename)
{
MIDI_FILE *mf = midiFileOpen(pFilename);

	if (mf)
		{
		MIDI_MSG msg;
		int i, iNum;
		unsigned int j;

		midiReadInitMessage(&msg);
		iNum = midiReadGetNumTracks(mf);
		for(i=0;i<iNum;i++)
			{
			printf("# Track %d\n", i);
			while(midiReadGetNextMessage(mf, i, &msg))
				{
				printf("\t");
				for(j=0;j<msg.iMsgSize;j++)
					printf("%.2x ", msg.data[j]);
				printf("\n");
				}
			}

		midiReadFreeMessage(&msg);
		midiFileClose(mf);
		}
}
*/

void TestEffu()
{
	int millis = glutGet(GLUT_ELAPSED_TIME)*100; 

	int milliframes = (int)(millis*0.000008);
	if ((frame % milliframes) == 1) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(test_shaderProg);

	GLfloat waveTime = 1+atan(millis*0.0001)*0.1,
			waveWidth = cos(millis*0.000001)*1000+atan(millis*0.00001)*2.0,
			waveHeight = sin(millis*0.0001)*100*atan(millis*0.00001)*2.0,
			waveFreq = 0.0001+cos(millis*0.000003)*1000.1;
	GLint waveTimeLoc = glGetUniformLocation(test_shaderProg, "waveTime");
	GLint waveWidthLoc = glGetUniformLocation(test_shaderProg, "waveWidth");
	GLint waveHeightLoc = glGetUniformLocation(test_shaderProg, "waveHeight");

	GLint widthLoc = glGetUniformLocation(test_shaderProg, "width");
	GLint heightLoc = glGetUniformLocation(test_shaderProg, "height");

	GLint timeLoc = glGetUniformLocation(test_shaderProg, "time");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grayeye_tex);

	GLuint location;
	location = glGetUniformLocation(test_shaderProg, "texture0");

	glUniform1i(location, 0);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glLoadIdentity();

	glTranslatef(0.0, 0.0, -150.0+sin(millis*0.0000004)*120);
	glRotatef(-75.0, 1.0, 0.0, 0.0);
	glRotatef(millis*0.01, 0.0, 0.0, 1.0);

	glUniform1f(waveTimeLoc, waveTime);
	glUniform1f(waveWidthLoc, waveWidth);
	glUniform1f(waveHeightLoc, waveHeight);

	glUniform1f(widthLoc, g_Width);
	glUniform1f(heightLoc, g_Height);

	glUniform1f(timeLoc, millis/100);

	int i, j;

	int zoom = 0;

	for (zoom = 0; zoom < 8; zoom++)
	{

	glTranslatef(-0.01, -0.01, -0.1);
	glRotatef((90*zoom+millis*0.01)*0.1, 1.0, 0.0, 0.0);
	glRotatef((45*zoom+millis*0.01)*0.1, 0.0, 0.0, 1.0);
	glRotatef((25*zoom+millis*0.01)*0.1, 0.0, 1.0, 0.0);
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
}

///////////////////////////////////////////////////////////// RENDER FUNCTION

void display(void)
{

	glEnable(GL_BLEND);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	TestEffu();

	glutSwapBuffers();
	frame++;
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
}

///////////////////////////////////////////////////////////////// MAIN LOGIC

void logic()
{
	glutPostRedisplay();
}

void quit()
{
	printf("--- MIDISYS ENGINE: time to quit()\n");
	glutLeaveMainLoop();
	exit(1);
}

void keyPress(unsigned char key, int x, int y)
{
	if (key == VK_ESCAPE)
	{
		quit();
	}
}

void InitGraphics(int argc, char* argv[])
{
	fprintf(stdout, "--- MIDISYS ENGINE: InitGraphics()\n");
	glutInit(&argc, argv);
	glutCreateWindow("MIDISYS window");
	glutReshapeWindow(g_Width, g_Height);
	glutFullScreen();

	glutSetCursor(GLUT_CURSOR_NONE);

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
  		/* Problem: glewInit failed, something is seriously wrong. */
  		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
  		exit(1);
	}
	fprintf(stdout, "InitGraphics() -> Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	glEnable(GL_TEXTURE_2D);

	glShadeModel(GL_SMOOTH);

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(logic);
	glutKeyboardFunc(keyPress);

	fprintf(stdout, "--- MIDISYS ENGINE: InitGraphics() success\n");
}

GLuint LoadShader(const char* pFilename)
{
	fprintf(stdout,"--- MIDISYS ENGINE: LoadShader(\"%s\")\n", pFilename);

	char vsName[128] = "";
	strcpy(vsName, pFilename);
	strcat(vsName, ".vs");

	char fsName[128] = "";
	strcpy(fsName, pFilename);
	strcat(fsName, ".fs");

	#ifdef SUPERVERBOSE 
	fprintf(stdout,"LoadShader(\"%s\") vertex shader source file: \"%s\"\n", pFilename, vsName);
	#endif

	char *vsSource = File2String(vsName);

	#ifdef SUPERVERBOSE 
	fprintf(stdout,"LoadShader(\"%s\") vertex shader source:\n----------------------------------------------------\n%s\n----------------------------------------------------\n", pFilename, vsSource);
	#endif

	#ifdef SUPERVERBOSE 
	fprintf(stdout,"LoadShader(\"%s\") fragment shader source file: \"%s\"\n", pFilename, fsName);
	#endif

	char *fsSource = File2String(fsName);

	#ifdef SUPERVERBOSE 
	fprintf(stdout,"LoadShader(\"%s\") fragment shader source:\n----------------------------------------------------\n%s\n----------------------------------------------------\n", pFilename, fsSource);
	#endif

	GLuint vs, fs, sp;

	printf("LoadShader(): vs glCreateShader\n");
	vs = glCreateShader(GL_VERTEX_SHADER);
	printf("LoadShader(): vs glShaderSource\n");
	glShaderSource(vs, 1, &vsSource, NULL);
	printf("LoadShader(): vs glCompileShader\n");
	glCompileShader(vs);
	printf("LoadShader(): vs compiled\n");

	printf("LoadShader(): fs glCreateShader\n");
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	printf("LoadShader(): fs glShaderSource\n");
	glShaderSource(fs, 1, &fsSource, NULL);
	printf("LoadShader(): fs glCompileShader\n");
	glCompileShader(fs);
	printf("LoadShader(): fs compiled\n");

	free(vsSource);
	free(fsSource);

	printf("LoadShader(): glCreateProgram\n");
	sp = glCreateProgram();
	printf("LoadShader(): glAttachShader vs\n");
	glAttachShader(sp, vs);
	printf("LoadShader(): glAttachShader fs\n");
	glAttachShader(sp, fs);
	printf("LoadShader(): glLinkProgram\n");
	glLinkProgram(sp);
	PrintShaderLog(sp);

	printf("LoadShader(): glUseProgram\n");
	glUseProgram(test_shaderProg);
	PrintShaderLog(sp);

	fprintf(stdout,"--- MIDISYS ENGINE: LoadShader(\"%s\") success\n", pFilename);

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
		printf(", texture loading error: '%s'\n", SOIL_last_result());
		exit(1);
	}

	printf(", success.\n");

	return tex_2d;
}


void StartMainLoop()
{
	printf("--- MIDISYS ENGINE: StartMainLoop()\n");
	glutMainLoop();
}

int main(int argc, char* argv[])
{
	printf("--- MIDISYS ENGINE: bilotrip foundation MIDISYS ENGINE 0.1 dosing\n");
	
	// init graphics

	InitGraphics(argc, argv);

	// graphic data files and shaders

	test_shaderProg = LoadShader("test");
	grayeye_tex = LoadTexture("grayeye.png");

	// init MIDI sync and audio

	LoadMIDIEventList("music.mid");
	InitAudio("music.mp3");

	// mainloop

	StartMainLoop();
	return 0;
}
