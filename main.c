// MIDISYS-ENGINE for windows and osx

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fstream>
#include <map>

#ifndef  __APPLE__
#include <malloc.h>
#endif

#include "midifile.h"
#include "midiutil.h"

#ifdef __APPLE__
	#include "glew.h"
	#include <OpenGL/OpenGL.h>
	#include <OpenGL/glu.h>
	#include "freeglut.h"
#else
	#include <GL/glew.h>
	#include <GL/glu.h>
	#include <GL/freeglut.h>
#endif

#include "assimp/Importer.hpp"
#include "assimp/PostProcess.h"
#include "assimp/Scene.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/LogStream.hpp"

#include "SOIL.h"

#include "bass.h"

#include "ggets.h"
#include "ggets.c"

// debug

int mouseX;
int mouseY;

// shaders

GLuint projector_shaderProg;

GLuint eye_shaderProg;
GLuint eye_post_shaderProg;
GLuint fsquad_shaderProg;
GLuint redcircle_shaderProg;

// fbo

GLuint fb;
GLuint fb_tex;
GLuint fb2;
GLuint fb_tex2;

GLuint depth_rb = 0;
GLuint depth_rb2 = 0;

// textures

int grayeye_tex = -1;
int room_tex[3] = {-1};
int majictext1_tex = -1;

int copkiller1_tex = -1;

int scene_tex = -1;

// texture switchers

int room_texnum = 0;

// assimp scene

const aiScene* scene = NULL;
GLuint scene_list = 0;
aiVector3D scene_min, scene_max, scene_center;

Assimp::Importer importer;

std::map<std::string, GLuint*> textureIdMap; // map image filenames to textureIds
GLuint*	textureIds;	// pointer to texture array

static float kujalla_angle = 0.f;
int beatmode = -1;

// assimp defines

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

// misc. gfx system

static GLfloat g_nearPlane = 0.001;
static GLfloat g_farPlane = 1000;
static int g_Width = 600;
static int g_Height = 600;

void InitFBO();

int frame = 0;

// effect pointers && logic && state

int current_scene;

void dummy(float dt) {}

void LeadMaskScene();
void CopScene();
void EyeScene();
void RedCircleScene();
void KolmeDeeScene();

void KolmeDeeLogic(float dt);

typedef void (*SceneRenderCallback)();
SceneRenderCallback scene_render[] = {
										&LeadMaskScene,
										&CopScene,
										&EyeScene, 
										&KolmeDeeScene,
										&RedCircleScene
									 };

typedef void (*SceneLogicCallback)(float);
SceneLogicCallback scene_logic[] = {
										&dummy,
										&dummy,
										&KolmeDeeLogic,
										&KolmeDeeLogic,
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


/////////////////////////////////////////////////////////
//////////////////////// PLAYLIST ////////////////////////
///////////////////////////////////////////////////////////

int demo_playlist()
{
	int sc = current_scene;
	if (millis >= 0 && millis < (55*1000)+922)
	{
		current_scene = 0; // lead masks
	}
	else if (millis >= (55*1000)+922 && (60*1000)+(51*1000)+844)
	{
		current_scene = 1;
	}
	else if (millis >= ((60*1000)+(51*1000)+844) && millis < ((120*1000)+(29*1000)+126))
	{
		current_scene = 2;
	}

	if (sc != current_scene)
	{
		scene_start_millis = millis;
	}
}

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

GLuint LoadTexture(const char* pFilename, int invert)
{
	printf("--- MIDISYS ENGINE: LoadTexture(\"%s\")", pFilename);
	GLuint tex_2d;

	if (strcmp(pFilename,"") == 0) return 99999;

	if (invert == 1) 
	{
		tex_2d = SOIL_load_OGL_texture
		(
			pFilename,
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
		);
	}
	else
	{
		tex_2d = SOIL_load_OGL_texture
		(
			pFilename,
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
		);

	}

	if(0 == tex_2d)
	{
		printf(" error loading texture from file \"%s\"\n", SOIL_last_result());
		exit(1);
	}

	printf(" success\n");

	return tex_2d;
}

GLuint LoadTexture(const char* pFilename)
{
	return LoadTexture(pFilename, 1);
}

int LoadGLTextures(const aiScene* scene) {

	if (scene->HasTextures())
	{
		printf("ERROR: support for meshes with embedded textures is not implemented\n");
		exit(1);
	}

	/* scan scene's materials for textures */
	for (unsigned int m=0; m<scene->mNumMaterials; m++)
	{
		int texIndex = 0;
		aiReturn texFound = AI_SUCCESS;

		aiString path;	// filename

		while (texFound == AI_SUCCESS)
		{
			texFound = scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
			GLuint texnum = LoadTexture(path.data, 0);
			textureIdMap[path.data] = texnum;
			texIndex++;
		}
	}
	//return success;
	return true;
}


///////////////////////////////////////////////////////////////// EFFECTS
///////////////////////////////////////////////////////////////// EFFECTS
///////////////////////////////////////////////////////////////// EFFECTS
///////////////////////////////////////////////////////////////// EFFECTS
///////////////////////////////////////////////////////////////// EFFECTS


// Can't send color down as a pointer to aiColor4D because AI colors are ABGR.
void Color4f(const aiColor4D *color)
{
glColor4f(color->r, color->g, color->b, color->a);
}

void set_float4(float f[4], float a, float b, float c, float d)
{
f[0] = a;
f[1] = b;
f[2] = c;
f[3] = d;
}

void color4_to_float4(const aiColor4D *c, float f[4])
{
f[0] = c->r;
f[1] = c->g;
f[2] = c->b;
f[3] = c->a;
}

void apply_material(const aiMaterial *mtl)
{
	float c[4];

	GLenum fill_mode;
	int ret1, ret2;
	aiColor4D diffuse;
	aiColor4D specular;
	aiColor4D ambient;
	aiColor4D emission;
	float shininess, strength;
	int two_sided;
	int wireframe;
	unsigned int max;	// changed: to unsigned

	int texIndex = 0;
	aiString texPath;	//contains filename of texture

	if(AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, texIndex, &texPath))
	{
	//bind texture
	GLuint texId = textureIdMap.at(texPath.data);
	if (texId != 99999) glBindTexture(GL_TEXTURE_2D, texId);
	}

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
	max = 1;
	ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
	if((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
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
	glEnable(GL_CULL_FACE);
	else
	glDisable(GL_CULL_FACE);
}


void recursive_render (const struct aiScene *sc, const struct aiNode* nd, float scale)
{
	unsigned int i;
	unsigned int n=0, t;
	aiMatrix4x4 m = nd->mTransformation;

	aiMatrix4x4 m2;
	aiMatrix4x4::Scaling(aiVector3D(scale, scale, scale), m2);
	m = m * m2;

	// update transform
	m.Transpose();
	glPushMatrix();
	glMultMatrixf((float*)&m);

	// draw all meshes assigned to this node
	for (; n < nd->mNumMeshes; ++n)
	{
	const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

	apply_material(sc->mMaterials[mesh->mMaterialIndex]);


	if(mesh->mNormals == NULL)
	{
	glDisable(GL_LIGHTING);
	}
	else
	{
	glEnable(GL_LIGHTING);
	}

	if(mesh->mColors[0] != NULL)
	{
	glEnable(GL_COLOR_MATERIAL);
	}
	else
	{
	glDisable(GL_COLOR_MATERIAL);
	}



	for (t = 0; t < mesh->mNumFaces; ++t) {
	const struct aiFace* face = &mesh->mFaces[t];
	GLenum face_mode;

	switch(face->mNumIndices)
	{
	case 1: face_mode = GL_POINTS; break;
	case 2: face_mode = GL_LINES; break;
	case 3: face_mode = GL_TRIANGLES; break;
	default: face_mode = GL_POLYGON; break;
	}

	glBegin(face_mode);

	for(i = 0; i < face->mNumIndices; i++)	// go through all vertices in face
	{
	int vertexIndex = face->mIndices[i];	// get group index for current index
	if(mesh->mColors[0] != NULL)
	Color4f(&mesh->mColors[0][vertexIndex]);
	if(mesh->mNormals != NULL)

	if(mesh->HasTextureCoords(0))	//HasTextureCoords(texture_coordinates_set)
	{
	glTexCoord2f(mesh->mTextureCoords[0][vertexIndex].x, 1 - mesh->mTextureCoords[0][vertexIndex].y); //mTextureCoords[channel][vertex]
	}

	glNormal3fv(&mesh->mNormals[vertexIndex].x);
	glVertex3fv(&mesh->mVertices[vertexIndex].x);
	}

	glEnd();

	}

	}


	// draw all children
	for (n = 0; n < nd->mNumChildren; ++n)
	{
	recursive_render(sc, nd->mChildren[n], scale);
	}

	glPopMatrix();
}


void KolmeDeeLogic(float dt)
{
	kujalla_angle += dt*0.01;
}

void KolmeDeeScene()
{
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);	// Enables Smooth Shading
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClearDepth(1.0f);	// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);	// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);	// The Type Of Depth Test To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculation

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0); // Uses default lighting parameters
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_NORMALIZE);

	GLfloat LightAmbient[]= { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat LightDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat LightPosition[]= { 0.0f, 0.0f, 15.0f, 1.0f };

	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
	glEnable(GL_LIGHT1);

	float tmp;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();

	glTranslatef(0.0f, -10.0f, -50.0f);	// Move 40 Units And Into The Screen

	glRotatef(kujalla_angle, 1.0f, 0.0f, 0.0f);
	glRotatef(kujalla_angle, 0.0f, 1.0f, 0.0f);
	glRotatef(kujalla_angle, 0.0f, 0.0f, 1.0f);

	recursive_render(scene, scene->mRootNode, 0.5);
}


void LeadMaskScene()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb); // fbo

	float mymillis = millis;
	glUseProgram(projector_shaderProg);

	glEnable(GL_BLEND);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);

	GLint widthLoc5 = glGetUniformLocation(projector_shaderProg, "width");
	GLint heightLoc5 = glGetUniformLocation(projector_shaderProg, "height");
	GLint timeLoc5 = glGetUniformLocation(projector_shaderProg, "time");
	GLint alphaLoc5 = glGetUniformLocation(projector_shaderProg, "alpha");

	glUniform1f(widthLoc5, g_Width);
	glUniform1f(heightLoc5, g_Height);
	glUniform1f(timeLoc5, mymillis);
	glUniform1f(alphaLoc5, mymillis*0.0001+0.2-cos(mymillis*0.0005)*0.15);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scene_tex);

	GLint location5 = glGetUniformLocation(projector_shaderProg, "texture0");
	glUniform1i(location5, 0);

	glLoadIdentity();
	glTranslatef(-1.2, -1.0, -1.0);

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

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb2); // default

	glUseProgram(fsquad_shaderProg);

	glEnable(GL_BLEND);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_ONE_MINUS_DST_COLOR);

	GLint widthLoc6 = glGetUniformLocation(fsquad_shaderProg, "width");
	GLint heightLoc6 = glGetUniformLocation(fsquad_shaderProg, "height");
	GLint timeLoc6 = glGetUniformLocation(fsquad_shaderProg, "time");
	GLint alphaLoc6 = glGetUniformLocation(fsquad_shaderProg, "alpha");
	GLint gammaLoc = glGetUniformLocation(fsquad_shaderProg, "gamma");
	GLint gridLoc6 = glGetUniformLocation(fsquad_shaderProg, "grid");
	glUniform1f(gridLoc6, 0.001+cos(mymillis)*0.0005);


	glUniform1f(widthLoc6, g_Width);
	glUniform1f(heightLoc6, g_Height);
	glUniform1f(timeLoc6, mymillis/100);
	glUniform1f(alphaLoc6, 0.1+abs(cos(mymillis*0.08)*0.05));
	glUniform1f(gammaLoc, 0.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fb_tex);

	GLint location6 = glGetUniformLocation(fsquad_shaderProg, "texture0");
	glUniform1i(location6, 0);

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

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); // default

	glUseProgram(fsquad_shaderProg);

	glDisable(GL_BLEND);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLint widthLoc7 = glGetUniformLocation(fsquad_shaderProg, "width");
	GLint heightLoc7 = glGetUniformLocation(fsquad_shaderProg, "height");
	GLint timeLoc7 = glGetUniformLocation(fsquad_shaderProg, "time");
	GLint alphaLoc7 = glGetUniformLocation(fsquad_shaderProg, "alpha");
	GLint gammaLoc2 = glGetUniformLocation(fsquad_shaderProg, "gamma");
	GLint gridLoc = glGetUniformLocation(fsquad_shaderProg, "grid");

	glUniform1f(widthLoc7, g_Width);
	glUniform1f(heightLoc7, g_Height);
	glUniform1f(timeLoc7, mymillis/100);
	glUniform1f(alphaLoc7, 1.0);
	glUniform1f(gammaLoc2, 4.0f);
	glUniform1f(gridLoc, 1.0f+tan(mymillis*10)*0.3);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fb_tex2);

	GLint location7 = glGetUniformLocation(fsquad_shaderProg, "texture0");
	glUniform1i(location7, 0);

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

	glUseProgram(projector_shaderProg);

	glEnable(GL_BLEND);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
	glBlendFunc(GL_SRC_COLOR, GL_DST_ALPHA);

	widthLoc5 = glGetUniformLocation(projector_shaderProg, "width");
	heightLoc5 = glGetUniformLocation(projector_shaderProg, "height");
	timeLoc5 = glGetUniformLocation(projector_shaderProg, "time");
	alphaLoc5 = glGetUniformLocation(projector_shaderProg, "alpha");

	glUniform1f(widthLoc5, g_Width);
	glUniform1f(heightLoc5, g_Height);
	glUniform1f(timeLoc5, mymillis);
	glUniform1f(alphaLoc5, 1.0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scene_tex);

	location5 = glGetUniformLocation(projector_shaderProg, "texture0");
	glUniform1i(location5, 0);

	glLoadIdentity();
	glTranslatef(-1.2, -1.0, -1.0);

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


void CopScene()
{
	float mymillis = millis;
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
	glBindTexture(GL_TEXTURE_2D, copkiller1_tex);

	GLint location5 = glGetUniformLocation(fsquad_shaderProg, "texture0");
	glUniform1i(location5, 0);

	glLoadIdentity();

	glTranslatef(-1.2, -1.0, -1.0);

	int i=0;
	int j=0;
	glBegin(GL_QUADS);
	glVertex2f(i, j);
	glVertex2f(i + 100, j);
	glVertex2f(i + 100, j + 100);
	glVertex2f(i, j + 100);
	glEnd();

}


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

	glRotatef(45,0.0,0.0,1.0f);
	glTranslatef(-1.2, -1.0, 0.0+(tan(mymillis*0.00001)*0.1)*(millis-155520)*0.0001);

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

	glLoadIdentity();

	glTranslatef(0.0f, -10.0f, -50.0f);	// Move 40 Units And Into The Screen

	glRotatef(kujalla_angle, 1.0f, 0.0f, 0.0f);
	glRotatef(kujalla_angle, 0.0f, 1.0f, 0.0f);
	glRotatef(kujalla_angle, 0.0f, 0.0f, 1.0f);

	recursive_render(scene, scene->mRootNode, 0.5);

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
	int intmillis = (int)millis;

	int i,j = 0;


	for (i=0; i < mapping_count; i++)
	{
		int tracknum = mapping_tracknum[i];
		int trackidx = timeline_trackindex[tracknum];
		MIDI_MSG currentMsg = timeline[tracknum][trackidx];

		// reset trigs
		if (scene_shader_param_type[mapping_paramnum[i]] == 0) scene_shader_params[mapping_paramnum[i]] = -1;

		int dw = (int)currentMsg.dwAbsPos;
		int tarkistus = (int)(dw)*1.212;

		//if (intmillis+155520 < tarkistus*1.212) break;
		if (intmillis < tarkistus) break;

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
					printf("track #%d, len %d, pos %d: dwAbsPos: %d (millis: %d) -> noteon: %d\n", tracknum, timeline_tracklength[tracknum], timeline_trackindex[tracknum], currentMsg.dwAbsPos, intmillis, trigVal);
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
					//printf("setparam track #%d, len %d, pos %d: time: %d -> %d\n", tracknum, timeline_tracklength[tracknum], timeline_trackindex[tracknum], currentMsg.dwAbsPos, paramVal);
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

double min(double a, double b)
{
	if (a<=b) return a;
	else return b;
}

void logic()
{ 	
	if (music_started == -1) { BASS_ChannelPlay(music_channel,FALSE); music_started = 1; }

	QWORD bytepos = BASS_ChannelGetPosition(music_channel, BASS_POS_BYTE);
	double pos = BASS_ChannelBytes2Seconds(music_channel, bytepos);
	millis = (float)pos*1000;

	demo_playlist();
	scene_logic[current_scene](0.0f);
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

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void quit()
{
	printf("--- MIDISYS ENGINE: time to quit()\n");
	glutLeaveMainLoop();
}

void keyPress(unsigned char key, int x, int y)
{
	quit();
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



	glGenTextures(1, &fb_tex2);
	glBindTexture(GL_TEXTURE_2D, fb_tex2);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, g_Width, g_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glGenFramebuffersEXT(1, &fb2);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb2);

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fb_tex2, 0);

	glGenRenderbuffersEXT(1, &depth_rb2);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_rb2);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, g_Width, g_Height);

	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth_rb2);

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

#ifdef  __APPLE__
	int swap_interval = 1;
	CGLContextObj cgl_context = CGLGetCurrentContext();
	CGLSetParameter(cgl_context, kCGLCPSwapInterval, &swap_interval);
#endif

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
	fprintf(stdout,"--- MIDISYS ENGINE: LoadShader(\"%s\") success\n", pFilename);
	#else
	printf(" success\n");
	#endif

	return sp;
}

bool Import3DFromFile(const std::string& pFile)
{
	//check if file exists
	std::ifstream fin(pFile.c_str());
	if(!fin.fail())
	{
		fin.close();
	}
	else
	{
		printf("Could not open file %s\n", pFile.c_str());
		exit(1);
	}

	scene = importer.ReadFile( pFile, aiProcessPreset_TargetRealtime_Quality);

	// If the import failed, report it
	if( !scene)
	{
		printf("Import failed %s\n", pFile.c_str());
		exit(1);
	}

	// We're done. Everything will be cleaned up by the importer destructor
	return true;
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

	projector_shaderProg = LoadShader("projector");
	eye_shaderProg = LoadShader("eye");
	eye_post_shaderProg = LoadShader("eye_post");
	fsquad_shaderProg = LoadShader("fsquad");
	redcircle_shaderProg = LoadShader("redcircle");

	// load textures

	scene_tex = LoadTexture("scene.png");

	copkiller1_tex = LoadTexture("copkiller1.png");

	grayeye_tex = LoadTexture("grayeye.png");

	room_tex[0] = LoadTexture("room1.png");
	room_tex[1] = LoadTexture("room2.png");
	room_tex[2] = LoadTexture("room3.png");

	majictext1_tex = LoadTexture("majictext1.png");


	// load 3d assets

	Import3DFromFile("templeton_peck.obj");
	LoadGLTextures(scene);

	// init MIDI sync and audio

	LoadMIDIEventList("music.mid");
	ParseMIDITimeline("mapping.txt");
	InitAudio("MC3_JAETTIMARSCH.mp3");

	// start mainloop

	StartMainLoop();

	return 0;
}
