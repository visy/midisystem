// MIDISYS-ENGINE for windows and osx

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fstream>
#include <map>
#include <wchar.h>
#include <time.h>

#include "freetype-gl.h"
#include "vertex-buffer.h"
#include "markup.h"
#include "shader.h"
#include "mat4.h"

#if defined(_WIN32) || defined(_WIN64)
#  define wcpncpy wcsncpy
#  define wcpcpy  wcscpy
#endif

#ifndef  __APPLE__
#include <malloc.h>
#endif

#include "midifile.h"
#include "midiutil.h"

int quitflag = 0;

// GLUT window handle (1 for windowed display, 0 for fullscreen gamemode)
GLuint window = 0;

// remove for non-debug build
int debugmode = 0;
// jump to demo position; 0 for whole demo
int jump_to = 0;
// some debugging flags
bool load_textures = true;

// midi sync

MIDI_MSG timeline[64][100000] = {NULL};
char timeline_trackname[64][512] = {-1};
int timeline_trackindex[64] = { 0 };
int timeline_tracklength[64] = { -1 };
int timeline_trackcount = 0;

// midi track number of mapping data
int mapping_tracknum[1000] = {-1};
// midi to shader param map from mapping.txt
int mapping_paramnum[1000] = {-1};
// track to map from
char mapping_trackname[1000][512] = {-1};
// map type: 0 == trig (noteon / off), 1 == param (modwheel / cc value)
int mapping_type[1000] = {-1};
// number of active mappings from midi to shader param
int mapping_count = 0;

// current shader param values 
int scene_shader_params[16] = {-1};
int scene_shader_param_type[16] = {-1};

float millis = 0;

// scene globals
// vhs
float vhsbeat = 0.0;
float vhsbeat_start = 0;

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

#include <oggplayer.h>
#include <algorithm>

#include "ggets.h"
#include "ggets.c"

// fbo

GLuint fb;
GLuint fb_tex;
GLuint fb2;
GLuint fb_tex2;

GLuint fake_framebuffer;
GLuint fake_framebuffer_tex;


#define KEYEVENTS_COUNT 507
unsigned char keyrec[507] = {98,105,108,111,116,114,105,112,32,111,112,101,114,97,116,105,110,103,32,115,121,115,116,101,109,32,52,46,50,48,13,97,108,108,32,108,101,102,116,115,32,97,110,100,32,114,105,103,104,116,115,32,114,101,118,101,114,115,101,100,13,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,13,99,97,116,32,110,111,116,101,46,116,120,116,13,49,54,58,51,48,32,98,101,32,97,116,32,116,104,101,32,97,103,114,101,101,100,32,112,108,97,99,101,13,49,56,58,51,48,32,115,119,97,108,108,111,119,32,99,97,112,115,117,108,101,115,13,97,102,116,101,114,32,101,102,102,101,99,116,58,32,112,114,111,101,99,8,8,116,101,120,8,99,116,32,109,101,116,97,108,115,13,119,97,105,116,32,102,111,114,32,97,109,97,115,8,8,8,8,109,97,115,107,32,115,105,103,110,97,108,13,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,13,99,97,116,32,98,105,108,111,116,114,105,112,95,99,114,101,100,105,116,115,46,116,120,116,13,97,101,103,105,115,13,100,101,112,13,101,101,118,8,8,118,101,97,103,101,110,8,8,8,110,103,101,108,13,104,97,100,100,97,115,13,111,97,115,105,122,13,112,97,104,97,109,111,107,97,13,115,112,105,105,107,107,105,13,118,105,115,121,13,122,111,118,13,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,69,68,69,83,8,65,57,65,105,39,109,13,103,101,116,116,105,110,103,32,115,111,109,101,116,104,105,110,103,32,110,111,119,13,105,8,46,46,46,105,39,109,32,102,101,101,108,105,110,103,32,105,116,32,116,111,111,46,13,105,116,39,115,46,46,46,46,32,105,116,39,115,46,46,46,46,46,46,46,46,46,13,105,116,39,115,32,107,105,99,107,107,107,105,105,105,105,105,105,105,105,105,105,110,110,110,110,110,110,110,110,110,110,103,103,103,103,103,103,103,103,105,105,105,105,105,105,105,105,105,110,110,110,110,110,110,110,110,110,13};
int keymillis[507] = {0,84,133,278,414,574,624,667,780,1045,1104,1185,1249,1329,1456,1537,1701,1753,1801,1891,2009,2099,2205,2672,2762,2905,3013,3103,3204,3317,3593,3837,3945,4079,4202,4273,4359,4429,4574,4610,4648,4729,4858,4905,4986,5080,5180,5253,5366,5440,5689,5820,6004,6079,6238,6299,6416,6495,6624,6796,7149,7291,7814,7845,7878,7910,7942,7975,8007,8040,8071,8104,8137,8169,8204,8237,8269,8301,8334,8367,8398,8431,8465,8496,8528,8561,8591,8623,8656,8688,8719,9198,9403,9460,9667,9777,9938,10014,10051,10089,10279,10618,10774,10873,11230,15891,16480,16917,17262,17987,18708,21302,21803,21920,22036,22135,22216,22353,22491,22549,22706,22821,22939,23096,23150,23303,23400,23454,23891,23930,23956,24016,24077,24387,28893,29377,29730,29985,30654,30829,33772,34315,34383,34521,34655,34791,34877,34972,35140,35179,35220,35322,35413,35498,35551,35627,36409,43630,44014,44155,44228,44372,44460,44596,44661,44794,44840,44950,45009,45217,45280,45398,45456,45610,45713,45846,46112,46237,46286,46340,46702,46935,47033,47087,47160,47284,47324,47393,47463,47519,47636,47790,49371,49417,49446,49539,49612,49721,49769,49867,49926,49998,50091,50164,50245,50420,50544,50669,50780,50856,50899,50921,51003,51075,51352,51420,51568,51663,51717,51844,52608,52905,53437,53469,53501,53533,53567,53598,53631,53663,53694,53728,53759,53792,53823,53855,53889,53920,53954,53987,54019,54051,54082,54115,54146,54179,54211,54244,54544,54700,54893,55387,56136,56247,56430,56502,56766,56831,56896,57004,57070,57206,57249,57271,57667,57831,57975,58045,58209,58315,58384,58444,58630,58736,58822,58927,59433,59758,59833,59952,60038,60133,60697,60984,61047,61106,61560,61885,62091,62106,62626,62757,62786,62832,63009,63115,63211,63228,63557,63679,63805,63980,64059,64099,64215,64663,65414,65494,65620,65795,65820,65902,66303,66604,66666,66725,66845,66929,67443,67820,67886,68047,68105,68236,68290,68416,68496,68865,69309,69422,69535,69677,69846,69965,70153,70608,71440,71520,71620,71718,71943,72258,72359,72941,73284,73559,74087,74120,74151,74183,74214,74249,74281,74315,74347,74378,74411,74442,74578,74703,74820,75021,75184,75338,75491,75983,76650,76976,77252,77294,77624,78108,78162,78932,79324,79777,80101,80598,80997,81157,81230,81367,81487,81976,82114,82182,82253,82550,82641,82690,82765,83021,83131,83184,83374,83422,83518,83829,83895,84017,84901,85534,85972,86047,86189,86325,86642,87409,87637,87710,87790,87866,87986,88086,88134,88307,88350,88405,88894,89235,89320,89406,89571,89715,90247,91658,91929,92036,92609,92713,93112,93263,93393,93539,94091,94249,94433,94711,94806,95220,95405,95541,95707,95992,96150,96306,96442,96600,97012,98638,98805,99329,99462,100393,100771,101191,102156,102308,102553,102984,103505,103675,103864,104079,104212,104361,104545,104890,105043,105235,105456,105682,105811,105979,106157,106330,106509,106670,106838,106997,107165,107573,107753,107952,108102,108259,108415,108591,108696,108847,108987,109123,109261,109418,109553,109665,109864,109969,110112,110232,110372,110506,110648,110781,110936,111062,111063};


#define KEYEVENTS_COUNT2 212
unsigned char keyrec2[212] = {85,115,32,104,105,103,104,32,112,114,101,99,105,115,105,111,110,32,116,111,111,115,108,44,32,101,8,8,8,8,8,108,115,32,44,8,8,44,32,101,110,103,105,110,101,101,114,101,100,32,116,111,32,101,120,101,99,117,116,101,13,116,104,101,32,115,104,111,114,116,45,115,105,103,8,103,104,116,101,100,32,105,100,101,111,108,111,103,105,99,97,108,32,102,108,97,118,111,117,114,32,111,102,32,116,111,100,97,121,13,65,115,32,108,111,110,103,32,97,115,32,116,104,101,114,101,32,105,115,32,97,32,99,111,109,109,111,110,32,101,118,8,110,121,8,101,109,121,32,116,111,32,117,110,105,116,101,32,117,115,13,119,101,39,108,108,32,107,101,101,108,8,112,32,109,97,114,99,104,105,110,103,44,32,108,105,107,101,32,97,32,119,101,108,108,45,111,105,108,101,100,32,109,97,99,104,105,110,101,46,46,46};
int keymillis2[212] = {0,157,253,413,518,573,654,794,940,1045,1110,1315,1406,1502,1599,1666,2283,2362,2972,3058,3189,3358,3459,3677,3747,3859,4049,4184,4320,4475,4589,4628,4677,4762,4907,5201,5327,5375,5456,5556,5802,5911,6008,6244,6380,6536,6855,6920,7061,7142,7254,7338,7535,8560,8680,8812,9067,9201,9284,9357,10544,11841,11961,12027,12091,12216,12402,12514,12634,12791,12933,13357,13472,13589,13857,14062,14490,14688,14751,15278,15395,15707,15812,15893,15953,16162,16316,16359,16463,16577,16632,16762,16899,17085,17180,17643,17815,17911,17972,18791,18888,19011,19219,19303,19516,19628,19771,19876,19895,20575,21102,21344,21518,21926,22072,22158,22208,22559,22659,22756,22866,23123,23290,23374,23457,23539,23605,23691,23767,23859,23955,24060,24357,24433,24502,24651,24700,24839,24981,25542,25672,25903,26080,26194,26632,26738,26835,26920,27000,27101,27164,27233,27448,27785,27899,27969,28050,28118,28234,28355,28892,29245,29372,29555,29730,29871,29973,30140,30219,30360,30476,30773,30817,30886,31118,31193,31264,31454,31494,31542,31893,31955,32193,32267,32405,32485,32638,32661,32745,32834,32927,33070,33166,33273,33401,33648,33995,34047,34253,34327,34421,34500,34629,34725,34861,34936,34999,35231,35315,35415,35515,35615};



const int __SIGNAL_ACTIVATE__     = 0;
const int __SIGNAL_COMPLETE__     = 1;
const int __SIGNAL_HISTORY_NEXT__ = 2;
const int __SIGNAL_HISTORY_PREV__ = 3;
#define MAX_LINE_LENGTH  511


const int MARKUP_NORMAL      = 0;
const int MARKUP_DEFAULT     = 0;
const int MARKUP_ERROR       = 1;
const int MARKUP_WARNING     = 2;
const int MARKUP_OUTPUT      = 3;
const int MARKUP_BOLD        = 4;
const int MARKUP_ITALIC      = 5;
const int MARKUP_BOLD_ITALIC = 6;
const int MARKUP_FAINT       = 7;
#define   MARKUP_COUNT         8


// ------------------------------------------------------- typedef & struct ---
typedef struct {
    float x, y, z;
    float s, t;
    float r, g, b, a;
} vertex_t;

struct _console_t {
    vector_t *     lines;
    wchar_t *      prompt;
    wchar_t        killring[MAX_LINE_LENGTH+1];
    wchar_t        input[MAX_LINE_LENGTH+1];
    size_t         cursor;
    markup_t       markup[MARKUP_COUNT];
    vertex_buffer_t * buffer;
    vec2           pen; 
    void (*handlers[4])( struct _console_t *, wchar_t * );
};
typedef struct _console_t console_t;

// ------------------------------------------------------- global variables ---
static console_t * console;
texture_atlas_t *atlas;
GLuint shader;
mat4   model, view, projection;


// ------------------------------------------------------------ console_new ---
console_t *
console_new( void )
{
    console_t *self = (console_t *) malloc( sizeof(console_t) );
    if( !self )
    {
        return self;
    }
    self->lines = vector_new( sizeof(wchar_t *) );
    self->prompt = (wchar_t *) wcsdup( L"" );
    self->cursor = 0;
    self->buffer = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );
    self->input[0] = L'\0';
    self->killring[0] = L'\0';
    self->handlers[__SIGNAL_ACTIVATE__]     = 0;
    self->handlers[__SIGNAL_COMPLETE__]     = 0;
    self->handlers[__SIGNAL_HISTORY_NEXT__] = 0;
    self->handlers[__SIGNAL_HISTORY_PREV__] = 0;
    self->pen.x = self->pen.y = 0;

    atlas = texture_atlas_new( 512, 512, 1 );

    vec4 white = {{0.2,1,0.2,0.7}};
    vec4 black = {{0,0,0,1}};
    vec4 none = {{0,0,1,0}};

    markup_t normal;
    normal.family  = "nauhoitin_fonts/VeraMono.ttf";
    normal.size    = 23.0;
    normal.bold    = 0;
    normal.italic  = 0;
    normal.rise    = 0.0;
    normal.spacing = 0.0;
    normal.gamma   = 1.0;
    normal.foreground_color    = white;
    normal.foreground_color.r = 0.45;
    normal.foreground_color.g = 0.65;
    normal.foreground_color.b = 0.45;

    normal.font = texture_font_new( atlas, "nauhoitin_fonts/term.ttf", 33 );

    markup_t bold = normal;
    bold.bold = 1;
    bold.font = texture_font_new( atlas, "nauhoitin_fonts/VeraMoBd.ttf", 23 );

    markup_t italic = normal;
    italic.italic = 1;
    bold.font = texture_font_new( atlas, "nauhoitin_fonts/VeraMoIt.ttf", 23 );

    markup_t bold_italic = normal;
    bold.bold = 1;
    italic.italic = 1;
    italic.font = texture_font_new( atlas, "nauhoitin_fonts/VeraMoBI.ttf", 13 );

    markup_t faint = normal;
    faint.foreground_color.r = 0.25;
    faint.foreground_color.g = 0.45;
    faint.foreground_color.b = 0.25;

    markup_t error = normal;
    error.foreground_color.r = 1.00;
    error.foreground_color.g = 0.00;
    error.foreground_color.b = 0.00;

    markup_t warning = normal;
    warning.foreground_color.r = 1.00;
    warning.foreground_color.g = 0.50;
    warning.foreground_color.b = 0.50;

    markup_t output = normal;
    output.foreground_color.r = 0.00;
    output.foreground_color.g = 0.00;
    output.foreground_color.b = 1.00;

    self->markup[MARKUP_NORMAL] = normal;
    self->markup[MARKUP_ERROR] = error;
    self->markup[MARKUP_WARNING] = warning;
    self->markup[MARKUP_OUTPUT] = output;
    self->markup[MARKUP_FAINT] = faint;
    self->markup[MARKUP_BOLD] = bold;
    self->markup[MARKUP_ITALIC] = italic;
    self->markup[MARKUP_BOLD_ITALIC] = bold_italic;

    return self;
}



// -------------------------------------------------------- console_delete ---
void
console_delete( console_t *self )
{ }



// ----------------------------------------------------- console_add_glyph ---
void
console_add_glyph( console_t *self,
                   wchar_t current,
                   wchar_t previous,
                   markup_t *markup )
{
    texture_glyph_t *glyph  = texture_font_get_glyph( markup->font, current );
    if( previous != L'\0' )
    {
        self->pen.x += texture_glyph_get_kerning( glyph, previous );
    }
    float r = markup->foreground_color.r;
    float g = markup->foreground_color.g;
    float b = markup->foreground_color.b;
    float a = markup->foreground_color.a;
    int x0  = self->pen.x + glyph->offset_x;
    int y0  = self->pen.y + glyph->offset_y;
    int x1  = x0 + glyph->width;
    int y1  = y0 - glyph->height;
    float s0 = glyph->s0;
    float t0 = glyph->t0;
    float s1 = glyph->s1;
    float t1 = glyph->t1;

    GLuint indices[] = {0,1,2, 0,2,3};
    vertex_t vertices[] = { { x0,y0,0,  s0,t0,  r,g,b,a },
                            { x0,y1,0,  s0,t1,  r,g,b,a },
                            { x1,y1,0,  s1,t1,  r,g,b,a },
                            { x1,y0,0,  s1,t0,  r,g,b,a } };
    vertex_buffer_push_back( self->buffer, vertices, 4, indices, 6 );
    
    self->pen.x += glyph->advance_x;
    self->pen.y += glyph->advance_y;
}



// -------------------------------------------------------- console_render ---
void
console_render( console_t *self )
{
    int viewport[4];
    glGetIntegerv( GL_VIEWPORT, viewport );

    size_t i, index;
    self->pen.x = 0;
    self->pen.y = viewport[3];
    vertex_buffer_clear( console->buffer );

    int cursor_x = self->pen.x;
    int cursor_y = self->pen.y;

    markup_t markup;

    // console_t buffer
    markup = self->markup[MARKUP_FAINT];
    self->pen.y -= markup.font->height;

    for( i=0; i<self->lines->size; ++i )
    {
        wchar_t *text = * (wchar_t **) vector_get( self->lines, i ) ;
        if( wcslen(text) > 0 )
        {
            console_add_glyph( console, text[0], L'\0', &markup );
            for( index=1; index < wcslen(text)-1; ++index )
            {
                console_add_glyph( console, text[index], text[index-1], &markup );
            }
        }
        self->pen.y -= markup.font->height - markup.font->linegap;
        self->pen.x = 0;
        cursor_x = self->pen.x;
        cursor_y = self->pen.y;
    }

    // Prompt
    markup = self->markup[MARKUP_BOLD];
    if( wcslen( self->prompt ) > 0 )
    {
        console_add_glyph( console, self->prompt[0], L'\0', &markup );
        for( index=1; index < wcslen(self->prompt); ++index )
        {
            console_add_glyph( console, self->prompt[index], self->prompt[index-1], &markup );
        }
    }
    cursor_x = (int) self->pen.x;

    // Input
    markup = self->markup[MARKUP_NORMAL];
    if( wcslen(self->input) > 0 )
    {
        console_add_glyph( console, self->input[0], L'\0', &markup );
        if( self->cursor > 0)
        {
            cursor_x = (int) self->pen.x;
        }
        for( index=1; index < wcslen(self->input); ++index )
        {
            console_add_glyph( console, self->input[index], self->input[index-1], &markup );
            if( index < self->cursor )
            {
                cursor_x = (int) self->pen.x;
            }
        }
    }

    float cursorblink = 0.7+abs(cos(millis*0.05)+0.3);

    // Cursor (we use the black character (-1) as texture )
    texture_glyph_t *glyph  = texture_font_get_glyph( markup.font, -1 );
    float r = markup.foreground_color.r;
    float g = markup.foreground_color.g;
    float b = markup.foreground_color.b;
    float a = markup.foreground_color.a*cursorblink;
    int x0  = cursor_x+1;
    int y0  = cursor_y + markup.font->descender;
    int x1  = cursor_x+14;
    int y1  = y0 + markup.font->height - markup.font->linegap;

    if (y0 == 714) { y0 = 648+33; y1 = y0 + markup.font->height - markup.font->linegap; }

    float s0 = glyph->s0;
    float t0 = glyph->t0;
    float s1 = glyph->s1;
    float t1 = glyph->t1;
    GLuint indices[] = {0,1,2, 0,2,3};
    vertex_t vertices[] = { { x0,y0,0,  s0,t0,  r,g,b,a },
                            { x0,y1,0,  s0,t1,  r,g,b,a },
                            { x1,y1,0,  s1,t1,  r,g,b,a },
                            { x1,y0,0,  s1,t0,  r,g,b,a } };
    vertex_buffer_push_back( self->buffer, vertices, 4, indices, 6 );
    glEnable( GL_TEXTURE_2D );
    glEnable(GL_BLEND);
	glActiveTexture(GL_TEXTURE0);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glBindTexture(GL_TEXTURE_2D, atlas->id);

    glUseProgram( shader );
    {
        glUniform1i( glGetUniformLocation( shader, "texture" ),
                     0 );
        glUniformMatrix4fv( glGetUniformLocation( shader, "model" ),
                            1, 0, model.data);
        glUniformMatrix4fv( glGetUniformLocation( shader, "view" ),
                            1, 0, view.data);
        glUniformMatrix4fv( glGetUniformLocation( shader, "projection" ),
                            1, 0, projection.data);
        vertex_buffer_render( console->buffer, GL_TRIANGLES );
    }


}



// ------------------------------------------------------- console_connect ---
void
console_connect( console_t *self,
                  const char *signal,
                  void (*handler)(console_t *, wchar_t *))
{
    if( strcmp( signal,"activate" ) == 0 )
    {
        self->handlers[__SIGNAL_ACTIVATE__] = handler;
    }
    else if( strcmp( signal,"complete" ) == 0 )
    {
        self->handlers[__SIGNAL_COMPLETE__] = handler;
    }
    else if( strcmp( signal,"history-next" ) == 0 )
    {
        self->handlers[__SIGNAL_HISTORY_NEXT__] = handler;
    }
    else if( strcmp( signal,"history-prev" ) == 0 )
    {
        self->handlers[__SIGNAL_HISTORY_PREV__] = handler;
    }
}



// --------------------------------------------------------- console_print ---
void
console_print( console_t *self, wchar_t *text )
{
    // Make sure there is at least one line
    if( self->lines->size == 0 )
    {
        wchar_t *line = wcsdup( L"" );
        vector_push_back( self->lines, &line );
    }

    if (self->lines->size > 10)
    {
        vector_erase(self->lines,0);
    }

    // Make sure last line does not end with '\n'
    wchar_t *last_line = *(wchar_t **) vector_get( self->lines, self->lines->size-1 ) ;
    if( wcslen( last_line ) != 0 )
    {
        if( last_line[wcslen( last_line ) - 1] == L'\n' )
        {
            wchar_t *line = wcsdup( L"" );
            vector_push_back( self->lines, &line );
        }
    }
    last_line = *(wchar_t **) vector_get( self->lines, self->lines->size-1 ) ;

    wchar_t *start = text;
    wchar_t *end   = wcschr(start, L'\n');
    size_t len = wcslen( last_line );
    if( end != NULL)
    {
        wchar_t *line = (wchar_t *) malloc( (len + end - start + 2)*sizeof( wchar_t ) );
        wcpncpy( line, last_line, len );
        wcpncpy( line + len, text, end-start+1 );

        line[len+end-start+1] = L'\0';

        vector_set( self->lines, self->lines->size-1, &line );
        free( last_line );
        if( (end-start)  < (wcslen( text )-1) )
        {
            console_print(self, end+1 );
        }
        return;
    }
    else
    {
        wchar_t *line = (wchar_t *) malloc( (len + wcslen(text) + 1) * sizeof( wchar_t ) );
        wcpncpy( line, last_line, len );
        wcpcpy( line + len, text );
        vector_set( self->lines, self->lines->size-1, &line );
        free( last_line );
        return;
    }
}



// ------------------------------------------------------- console_process ---
void
console_process( console_t *self,
                  const char *action,
                  const unsigned char key )
{
    size_t len = wcslen(self->input);

    //printf("console_process:%d\n", key);

    if( strcmp(action, "type") == 0 )
    {
        if( len < MAX_LINE_LENGTH )
        {
            memmove( self->input + self->cursor+1,
                     self->input + self->cursor, 
                     (len - self->cursor+1)*sizeof(wchar_t) );
            self->input[self->cursor] = (wchar_t) key;
            self->cursor++;
        }
        else
        {
            fprintf( stderr, "Input buffer is full\n" );
        }
    }
    else
    {
        if( strcmp( action, "enter" ) == 0 )
        {
            if( console->handlers[__SIGNAL_ACTIVATE__] )
            {
                (*console->handlers[__SIGNAL_ACTIVATE__])(console, console->input);
            }
            console_print( self, self->prompt );
            console_print( self, self->input );
            console_print( self, L"\n" );
            self->input[0] = L'\0';
            self->cursor = 0;
        }
        else if( strcmp( action, "right" ) == 0 )
        {
            if( self->cursor < wcslen(self->input) )
            {
                self->cursor += 1;
            }
        }
        else if( strcmp( action, "left" ) == 0 )
        {
            if( self->cursor > 0 )
            {
                self->cursor -= 1;
            }
        }
        else if( strcmp( action, "delete" ) == 0 )
        {
            memmove( self->input + self->cursor,
                     self->input + self->cursor+1, 
                     (len - self->cursor)*sizeof(wchar_t) );
        }
        else if( strcmp( action, "backspace" ) == 0 )
        {
            if( self->cursor > 0 )
            {
                memmove( self->input + self->cursor-1,
                         self->input + self->cursor, 
                         (len - self->cursor+1)*sizeof(wchar_t) );
                self->cursor--;
            }
        }
        else if( strcmp( action, "kill" ) == 0 )
        {
            if( self->cursor < len )
            {
                wcpcpy(self->killring, self->input + self->cursor );
                self->input[self->cursor] = L'\0';
                fwprintf(stderr, L"Kill ring: %ls\n", self->killring);
            }
            
        }
        else if( strcmp( action, "yank" ) == 0 )
        {
            size_t l = wcslen(self->killring);
            if( (len + l) < MAX_LINE_LENGTH )
            {
                memmove( self->input + self->cursor + l,
                         self->input + self->cursor, 
                         (len - self->cursor)*sizeof(wchar_t) );
                memcpy( self->input + self->cursor,
                        self->killring, l*sizeof(wchar_t));
                self->cursor += l;
            }
        }
        else if( strcmp( action, "home" ) == 0 )
        {
            self->cursor = 0;
        }
        else if( strcmp( action, "end" ) == 0 )
        {
            self->cursor = wcslen( self->input );
        }
        else if( strcmp( action, "clear" ) == 0 )
        {
        }
        else if( strcmp( action, "history-prev" ) == 0 )
        {
            if( console->handlers[__SIGNAL_HISTORY_PREV__] )
            {
                (*console->handlers[__SIGNAL_HISTORY_PREV__])(console, console->input);
            }
        }
        else if( strcmp( action, "history-next" ) == 0 )
        {
            if( console->handlers[__SIGNAL_HISTORY_NEXT__] )
            {
                (*console->handlers[__SIGNAL_HISTORY_NEXT__])(console, console->input);
            }
        }
        else if( strcmp( action, "complete" ) == 0 )
        {
            if( console->handlers[__SIGNAL_COMPLETE__] )
            {
                (*console->handlers[__SIGNAL_COMPLETE__])(console, console->input);
            }
        }
    }
}








// debug

int mouseX;
int mouseY;

// shaders

GLuint shaders[10];
const char* shaderss[] = {  "nauhoitin_shaders/v3f-t2f-c4f.frag",
                            "data/shaders/projector",
                            "data/shaders/eye",
                            "data/shaders/eye_post",
                            "data/shaders/fsquad",
                            "data/shaders/copquad",
                            "data/shaders/redcircle",
                            "data/shaders/vhs",
                            "data/shaders/yuv2rgb",
                            "data/shaders/hex"};
enum shaderi { nannanna, projector, eye, eye_post, fsquad, copquad, redcircle, vhs, yuv2rgb, hex };

GLuint depth_rb = 0;
GLuint depth_rb2 = 0;
GLuint depth_rb3 = 0;
// textures

int textures[65] = {-1};
const char* texturess[] = {"data/gfx/scene.jpg",
                    "data/gfx/dude1.jpg",
                    "data/gfx/dude2.jpg",
                    "data/gfx/mask.jpg",
                    "data/gfx/note.jpg",
                    "data/gfx/exit.jpg",
                    "data/gfx/v0.png",
                    "data/gfx/v1.png",
                    "data/gfx/v2.png",
                    "data/gfx/v3.png",
                    "data/gfx/v4.png",
                    "data/gfx/v5.png",
                    "data/gfx/v6.png",
                    "data/gfx/v7.png",
                    "data/gfx/v8.png",
                    "data/gfx/v9.png",
                    "data/gfx/v9a.png",
                    "data/gfx/v9b.png",
                    "data/gfx/v9c.png",
                    "data/gfx/copkiller1.jpg",
                    "data/gfx/prip1.jpg",
                    "data/gfx/copkiller2.jpg",
                    "data/gfx/prip2.jpg",
                    "data/gfx/copkiller3.jpg",
                    "data/gfx/prip3.jpg",
                    "data/gfx/copkiller4.jpg",
                    "data/gfx/prip4.jpg",
                    "data/gfx/copkiller5.jpg",
                    "data/gfx/prip5.jpg",
                    "data/gfx/copkiller6.jpg",
                    "data/gfx/prip6.jpg",
                    "data/gfx/copkiller7.jpg",
                    "data/gfx/prip7.jpg",
                    "data/gfx/copkiller8.jpg",
                    "data/gfx/prip8.jpg",
                    "data/gfx/copkiller9.jpg",
                    "data/gfx/prip9.jpg",
                    "data/gfx/copkiller10.jpg",
                    "data/gfx/prip10.jpg",
                    "data/gfx/copkiller11.jpg",
                    "data/gfx/prip11.jpg",
                    "data/gfx/copkiller12.jpg",
                    "data/gfx/prip12.jpg",
                    "data/gfx/copkiller13.jpg",
                    "data/gfx/prip13.jpg",
                    "data/gfx/copkiller14.jpg",
                    "data/gfx/prip14.jpg",
                    "data/gfx/copkiller15.jpg",
                    "data/gfx/prip15.jpg",
                    "data/gfx/aegis.jpg",
                    "data/gfx/ll1.png",
                    "data/gfx/ll2.png",
                    "data/gfx/ll3.png",
                    "data/gfx/ll4.png",
                    "data/gfx/ll5.png",
                    "data/gfx/grayeye.jpg",
                    "data/gfx/room1.jpg",
                    "data/gfx/room2.jpg",
                    "data/gfx/room3.jpg",
                    "data/gfx/majic1.jpg",
                    "data/gfx/majic2.jpg",
                    "data/gfx/majic3.jpg",
                    "data/gfx/majic4.jpg",
                    "data/gfx/bilogon.png",
                    "data/gfx/noise.jpg"};
enum texturi { tex_scene, tex_dude, tex_dude2, tex_mask, tex_note, tex_exit,
                tex_v0,tex_v1,tex_v2,tex_v3,tex_v4,tex_v5,tex_v6,tex_v7,tex_v8,tex_v9,tex_v9a,tex_v9b,tex_v9c,
                tex_copkiller, tex_prip,
                tex_copkiller2, tex_prip2,
                tex_copkiller3, tex_prip3,
                tex_copkiller4, tex_prip4,
                tex_copkiller5, tex_prip5,
                tex_copkiller6, tex_prip6,
                tex_copkiller7, tex_prip7,
                tex_copkiller8, tex_prip8,
                tex_copkiller9, tex_prip9,
                tex_copkiller10,tex_prip10,
                tex_copkiller11,tex_prip11,
                tex_copkiller12,tex_prip12,
                tex_copkiller13,tex_prip13,
                tex_copkiller14,tex_prip14,
                tex_copkiller15,tex_prip15,

                tex_aegis, tex_ll1,tex_ll2,tex_ll3,tex_ll4,tex_ll5,
                tex_grayeye, tex_room, tex_room2, tex_room3,
                tex_majestic1, tex_majestic2, tex_majestic3, tex_majestic4,
                tex_bilogon,
                tex_noise};

// texture switchers

int room_texnum = 0;

// assimp scenes

Assimp::Importer importer[7];

aiScene* kapsule = NULL;
aiScene* bilothree = NULL;
aiScene* brieflycase = NULL;
aiScene* bilothorn = NULL;
aiScene* biloflat = NULL;
aiScene* bilotetra = NULL;

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
static int c_Width = 640;
static int c_Height = 360;
static int g_Width = 1280;
static int g_Height = 720;

class YUVFrame {
public:
	YUVFrame(OggPlayer oggstream):ogg(oggstream) {
		width = ogg.width(); height = ogg.height();
		printf("yuvframe: ogg reports: %dx%d\n",width,height);
		// The textures are created when rendering the first frame
		y_tex = u_tex = v_tex = -1 ;
	}
	~YUVFrame() {
		glDeleteTextures(1,&y_tex);
		glDeleteTextures(1,&u_tex);
		glDeleteTextures(1,&v_tex);
	}
	void play() {
		printf("render(): ogg.play()\n");
		ogg.play();
		printf("render(): ogg.play() done\n");
	}
	void render() {
		update();
		if(-1==y_tex) return; // not ready yet
		glUseProgram(shaders[yuv2rgb]);

		GLint widthLoc5 = glGetUniformLocation(shaders[yuv2rgb], "width");
		GLint heightLoc5 = glGetUniformLocation(shaders[yuv2rgb], "height");
		glUniform1f(widthLoc5, g_Width);
		glUniform1f(heightLoc5, g_Height);

		GLint y_pos = glGetUniformLocation(shaders[yuv2rgb],"y_tex");
		GLint u_pos = glGetUniformLocation(shaders[yuv2rgb],"u_tex");
		GLint v_pos = glGetUniformLocation(shaders[yuv2rgb],"v_tex");

		glDisable(GL_BLEND);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, y_tex);
		glUniform1i(y_pos, 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, u_tex);
		glUniform1i(u_pos, 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, v_tex);
		glUniform1i(v_pos, 2);

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
private:
	void update() {
		YUVBuffer yuv;
		// yuv_buffer_try_lock(...) returns false if the last read frame is
		// still up to date, in this case we can simply retender the 
		// last frame without an update
		// We don't need to call unlock unless the lock operation was successfull
		if(!ogg.yuv_buffer_try_lock(&yuv)) return;
		// Create the textures if needed, at this point we 
		// know how big the textures should be.
		// The sample plyer that comes with the official SDK
		// assummes uv_width=y_width/2 , uv_height=y_height/2
		// but I'm not sure whether that is always true
		if(-1==y_tex){
		printf("update(): gen texture\n");
			y_tex = gen_texture(yuv.y_width,yuv.y_height);
			u_tex = gen_texture(yuv.uv_width,yuv.uv_height);
			v_tex = gen_texture(yuv.uv_width,yuv.uv_height);
		printf("update(): gen texture done\n");
		}
		int y_offset = ogg.offset_x() + yuv.y_stride * ogg.offset_y();
		int uv_offset = ogg.offset_x()/(yuv.y_width/yuv.uv_width)+
			yuv.uv_stride * ogg.offset_y()/(yuv.y_height/yuv.uv_height);
		update_texture(y_tex,yuv.y+y_offset,yuv.y_width,yuv.y_height,yuv.y_stride);
		update_texture(u_tex,yuv.u+uv_offset,yuv.uv_width,yuv.uv_height,yuv.uv_stride);
		update_texture(v_tex,yuv.v+uv_offset,yuv.uv_width,yuv.uv_height,yuv.uv_stride);
		ogg.yuv_buffer_unlock();
	}
	GLuint gen_texture(int w,int h) {
		GLuint tex;
		glGenTextures(1,&tex);
		glBindTexture(GL_TEXTURE_2D,tex);
		glTexImage2D(GL_TEXTURE_2D,0,1,w,h,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,NULL);
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
		return tex;
	}
	void update_texture(GLuint tex, unsigned char* buf,int w,int h, int stride) {
		glPixelStorei(GL_UNPACK_ROW_LENGTH,stride);
		glBindTexture(GL_TEXTURE_2D,tex);
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,w,h,GL_LUMINANCE,GL_UNSIGNED_BYTE,buf);
		// set GL_UNPACK_ROW_LENGTH back to 0 to avoid bugs 
		glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	}
	OggPlayer ogg;
	GLuint y_tex,u_tex,v_tex;
	int width,height;
};

YUVFrame* myVideoFrame;

void InitFBO();

int frame = 0;

// effect pointers && logic && state

int current_scene;

void dummy(float dt) {}

void Loader();
void LeadMaskScene();
void CopScene();
void MarssiScene();
void LongScene();
void EyeScene();
void RedCircleScene();
void BiloThreeScene();
void KolmeDeeScene();

void KolmeDeeLogic(float dt);
//void LoaderLogic(float dt);
void ConsoleLogic(float dt);
void ConsoleLogic2(float dt);

typedef void (*SceneRenderCallback)();
SceneRenderCallback scene_render[] = {
										&Loader,
                                        &LeadMaskScene,
										&CopScene,
										&MarssiScene,
                                        &LongScene,
										&EyeScene, 
										&RedCircleScene,
										&BiloThreeScene
									 };

typedef void (*SceneLogicCallback)(float);
SceneLogicCallback scene_logic[] = {
                                        &dummy,
                                        &ConsoleLogic,
										&dummy,
										&ConsoleLogic2,
                                        &dummy,
										&dummy,
										&dummy,
										&dummy
									 };

// audio

float scene_start_millis = 0;

int music_started = -1;

DWORD music_channel = 0;


/////////////////////////////////////////////////////////
//////////////////////// PLAYLIST ////////////////////////
///////////////////////////////////////////////////////////

int demo_playlist()
{
	int sc = current_scene;
	if (millis >= 0 && millis < 111844) // 55922
	{
		current_scene = 1; // lead masks
	}
	else if (millis >= 111844 && millis < 148800)
	{
		current_scene = 2; // cops
	}
	else if (millis >= 148800 && millis < 184000)
	{
		current_scene = 3; // marssi
	}
    else if (millis >= 184000 && millis < 188737)
    {
        current_scene = 4; // "long live the new flesh"
    }
	else if (millis >= 188737 && millis < 264000)
	{
		current_scene = 5; // eye horror
	}
	else if (millis >= 264000 && millis < 300200)
	{
		current_scene = 6; // outro 1 / redcircle
	}
	else if (millis >= 300200 && millis < 320000)
	{
		current_scene = 7; // outro 2 / bilothree
	}

	if (sc != current_scene)
	{
        /*if(current_scene == 1) 
        {
            //printf("xx XX xxxxxxxRESHAPING HAXXXXxxxxx xxx XX xx");
            //glutLeaveMainLoop();
            //glutReshapeWindow(g_Width, g_Height);
            //glutFullScreen();
            //glutMainLoop();
         }*/
		scene_start_millis = millis;
		vector_clear(console->lines);
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

	if (!BASS_Init(-1,44100,0,0,NULL)) BassError("InitAudio() - can't initialize device\n");

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
		printf("InitAudio() error! could not load file.\n");
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
		printf("\n%s\n",infoLog);
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
    if(!load_textures) return;

	if (strcmp(pFilename,"") == 0) return 99999;
	printf(" - LoadTexture(\"%s\")", pFilename);
	GLuint tex_2d;

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

GLuint LoadShader(const char* pFilename)
{
    fprintf(stdout," - LoadShader(\"%s\")", pFilename);

    #ifdef SUPERVERBOSE
    printf("\n");
    #endif

    char vsName[256] = "";
    strcpy(vsName, pFilename);
    strcat(vsName, ".vs");

    char fsName[256] = "";
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
    PrintShaderLog(vs);
    #ifdef SUPERVERBOSE 
    printf("\tLoadShader(): vs glShaderSource\n");
    #endif
    glShaderSource(vs, 1, (const GLchar**)&vsSource, NULL);
    PrintShaderLog(vs);
    #ifdef SUPERVERBOSE 
    printf("\tLoadShader(): vs glCompileShader\n");
    #endif
    glCompileShader(vs);
    PrintShaderLog(vs);
    #ifdef SUPERVERBOSE 
    printf("\tLoadShader(): vs compiled\n");
    #endif

    #ifdef SUPERVERBOSE 
    printf("\tLoadShader(): fs glCreateShader\n");
    #endif
    fs = glCreateShader(GL_FRAGMENT_SHADER);
    PrintShaderLog(fs);
    #ifdef SUPERVERBOSE 
    printf("\tLoadShader(): fs glShaderSource\n");
    #endif
    glShaderSource(fs, 1, (const GLchar**)&fsSource, NULL);
    PrintShaderLog(fs);
    #ifdef SUPERVERBOSE 
    printf("\tLoadShader(): fs glCompileShader\n");
    #endif
    glCompileShader(fs);
    PrintShaderLog(fs);
    #ifdef SUPERVERBOSE 
    printf("\tLoadShader(): fs compiled\n");
    #endif

    free(vsSource);
    free(fsSource);

    #ifdef SUPERVERBOSE 
    printf("\tLoadShader(): glCreateProgram\n");
    #endif
    sp = glCreateProgram();
    PrintShaderLog(sp);
    #ifdef SUPERVERBOSE 
    printf("\tLoadShader(): glAttachShader vs\n");
    #endif
    glAttachShader(sp, vs);
    PrintShaderLog(sp);
    #ifdef SUPERVERBOSE 
    printf("\tLoadShader(): glAttachShader fs\n");
    #endif
    glAttachShader(sp, fs);
    PrintShaderLog(sp);
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
	const struct aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];

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
float jormymillis = 0.0;
float startti = 0;
float startti2 = 0;
float pantime = 0;
void BiloThreeScene()
{
float mymillis = (millis-scene_start_millis);
glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb2); // default
glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
glClearDepth(1.0f); // Depth Buffer Setup
//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

glUseProgram(0);

glDisable(GL_TEXTURE_2D);
glDisable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR);

glShadeModel(GL_SMOOTH);    // Enables Smooth Shading
glEnable(GL_DEPTH_TEST);    // Enables Depth Testing
glDepthFunc(GL_LEQUAL); // The Type Of Depth Test To Do
glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Really Nice Perspective Calculation

glEnable(GL_LIGHTING);
glEnable(GL_LIGHT0); // Uses default lighting parameters
glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
glEnable(GL_NORMALIZE);

bool beatflag = false;
float tmp; 
float zoom = -300.0f+(((mymillis-jormymillis)*atan(mymillis*0.005))*0.05*0.18);

if (scene_shader_params[2] == 36) { beatflag = true; vhsbeat = 1.0f; vhsbeat_start = mymillis; }
vhsbeat-=((mymillis-vhsbeat_start)*0.00005);

if (zoom > -1.5 && startti == 0) { startti = mymillis; }
if (zoom >= -1.5 && beatflag) { zoom = -1.5; jormymillis+=1090; beatflag = false; }

float li_am = sin(mymillis/12)*vhsbeat;
float li_di = cos(mymillis/12)*0.65f*vhsbeat;
GLfloat LightAmbient[]= { startti == 0 ? 0.25 : li_am, startti == 0 ? 0.25 : li_am, startti == 0 ? 0.25 : li_am, 1.0f };
GLfloat LightDiffuse[]= { startti == 0 ? 0.25 : li_di, startti == 0 ? 0.25 : li_di, startti == 0 ? 0.25 : li_di, 1.0f };
GLfloat LightPosition[]= { sin(mymillis*0.02), cos(mymillis*0.02), 15.0f*cos(mymillis*0.01), 1.0f };

glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
glEnable(GL_LIGHT1);

if (jormymillis > 0) {
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

glLoadIdentity();

    if (jormymillis > 300*60 && startti2 == 0)
    {
        startti2 = mymillis;
    }

    if (jormymillis > 300*60)
    {
        pantime = mymillis-startti2;
    }

glTranslatef(0.0f, startti2 > 0 ? -5.5f-pantime*0.0008 : startti == 0 ? -11.5f : -5.5, zoom+pantime*0.001*atan(pantime*0.005));
if (jormymillis > 0) {
glRotatef(jormymillis*0.0026,-1.0,0.0,0.0);
glRotatef(zoom, 0.02, -0.01, -0.1*(mymillis-startti2)/100);
}


    float zoomfactor = 2.0+jormymillis*0.0001;
    if (zoomfactor > 4.0) zoomfactor = 4.0;

    if (startti > 0) recursive_render(bilothree, bilothree->mRootNode, zoomfactor-0.5);
recursive_render(bilothree, bilothree->mRootNode, zoomfactor);
    recursive_render(bilothree, bilothree->mRootNode, 6.0-zoomfactor);


glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); // default
glDisable(GL_BLEND);
glEnable(GL_TEXTURE_2D);
    if (jormymillis > 0) glBlendFunc(GL_SRC_COLOR,GL_ONE_MINUS_DST_COLOR);
    else glBlendFunc(GL_SRC_COLOR,GL_DST_ALPHA);
if (startti == mymillis) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

glUseProgram(shaders[hex]);
float widthLoc5 = glGetUniformLocation(shaders[hex], "width");
float heightLoc5 = glGetUniformLocation(shaders[hex], "height");
float timeLoc5 = glGetUniformLocation(shaders[hex], "time");
float effuLoc5 = glGetUniformLocation(shaders[hex], "effu");

glUniform1f(widthLoc5, g_Width);
glUniform1f(heightLoc5, g_Height);
glUniform1f(timeLoc5, mymillis/100);
glUniform1f(effuLoc5, 0.0);

glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, fb_tex2);

float location5 = glGetUniformLocation(shaders[hex], "texture0");
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




void KolmeDeeLogic(float dt)
{
	kujalla_angle += dt*0.01;
}

void kapsule_render()
{

    glEnable(GL_BLEND);
    glUseProgram(0);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);	// Enables Smooth Shading
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

	glLoadIdentity();

	glTranslatef(0.0f, 0.0f, -50.0f+cos(millis*0.05)*25);	// Move 40 Units And Into The Screen
    glRotatef(millis*0.01,1.0,0,0);
    glRotatef(millis*0.008,0.0,0.0,1.0);
    glRotatef(millis*0.006,0.0,1.0,0.0);

	recursive_render(kapsule, kapsule->mRootNode, 2.5);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_NORMALIZE);
}

float brimillis = 0.0;
float bristart = 0.0;
float brixrot = 0.0;
float briyrot = 0.0;

void brieflycase_render()
{
    if (bristart == 0) {bristart = millis;}
    brimillis = millis-bristart;

    glEnable(GL_BLEND);
    glUseProgram(0);
    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);    // Enables Smooth Shading
    glEnable(GL_DEPTH_TEST);    // Enables Depth Testing
    glDepthFunc(GL_LEQUAL); // The Type Of Depth Test To Do
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Really Nice Perspective Calculation

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0); // Uses default lighting parameters
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glEnable(GL_NORMALIZE);

    float brifade = 1.0f;

    if (brimillis <= 1000) {
        brifade = brimillis*0.001;
        if (brifade > 1.0f) brifade = 1.0f;
    }
    GLfloat LightAmbient[]= { 0.5f*brifade, 0.5f*brifade, 0.5f*brifade, 1.0f*brifade };
    GLfloat LightDiffuse[]= { 1.0f*brifade, 1.0f*brifade, 1.0f*brifade, 1.0f*brifade };
    GLfloat LightPosition[]= { 0.0f, 0.0f, 15.0f*brifade, 1.0f*brifade };

    glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
    glEnable(GL_LIGHT1);

    float tmp;

    glLoadIdentity();

    //nosto
    if (brimillis > 1000 && brimillis < 3000)
    {
        briyrot=(brimillis-1000)*0.047*atan(brimillis);
        if (briyrot > 90.0f) briyrot = 90.0f;
    }

    if (brimillis > 5000)
    {
        brixrot=(brimillis-5000)*0.047*2*atan(brimillis-4000);
//        if (brixrot > 180.0f) brixrot = 180.0f;
    }

    glTranslatef(0.0f, 0.0f, -50.0f+brixrot*0.2);   // Move 40 Units And Into The Screen
    //glRotatef(millis*0.001,1.0,0,0);
    glRotatef(-briyrot,1.0,0.0,0.0);
    glRotatef(brixrot,0.0,0.0,1.0);

    recursive_render(brieflycase, brieflycase->mRootNode, 2.5);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_NORMALIZE);
}


void
on_key_press ( unsigned char key)
{
    if (key == 1)
    {
        console_process( console, "home", 0 );
    }
    else if (key == 4)
    { 
        console_process( console, "delete", 0 );
    }
    else if (key == 5)
    { 
        console_process( console, "end", 0 );
    }
    else if (key == 8)
    { 
        console_process( console, "backspace", 0 );
    }
    else if (key == 9)
    {
        console_process( console, "complete", 0 );
    }
    else if (key == 11)
    {
        console_process( console, "kill", 0 );
    }
    else if (key == 12)
    {
        console_process( console, "clear", 0 );
    }
    else if (key == 13)
    {
        console_process( console, "enter", 0 );
    }
    else if (key == 25)
    {
        console_process( console, "yank", 0 );
    }
    else if (key == 27)
    {
        console_process( console, "escape", 0 );
    }
    else if (key == 127)
    {
        console_process( console, "backspace", 0 );
    }
    else if( key > 31)
    {
        console_process( console, "type", key );
    }
}

/*void LoaderLogic(float dt)
{
    printf("DEBUG: LoaderLogic(%f)\n", dt);
}*/

int keyindex = 0;
int nextmillis = 0;
void ConsoleLogic(float dt)
{
//	int kmillis = (int)(millis-15750);
    int kmillis = (int)(millis-scene_start_millis);

	//printf("kmillis:%d\n",kmillis);
	if (kmillis >= 0 && kmillis >= keymillis[keyindex])
	{
		if(keyindex >= 0 && keyindex < KEYEVENTS_COUNT)
		{
			on_key_press(keyrec[keyindex]);
		}

		keyindex++;
	}
}

int keyindex2 = 0;

void ConsoleLogic2(float dt)
{
	int kmillis = (int)((millis-scene_start_millis)*1.1);

	//printf("kmillis:%d\n",kmillis);
	if (kmillis >= 0 && kmillis >= keymillis2[keyindex2])
	{
		if(keyindex2 >= 0 && keyindex2 < KEYEVENTS_COUNT2)
		{
			on_key_press(keyrec2[keyindex2]);
		}

		keyindex2++;
	}
}

int kolmedeeindex = 0;

aiScene* Import3DFromFile(const std::string& pFile)
{
    fprintf(stdout," - Import3DFromFile(\"%s\")", pFile.c_str());

    //check if file exists
    std::ifstream fin(pFile.c_str());
    if(!fin.fail())
    {
        fin.close();
    }
    else
    {
        printf(" could not open file %s\n", pFile.c_str());
        exit(1);
    }

    aiScene* scener = importer[kolmedeeindex].ReadFile( pFile, aiProcessPreset_TargetRealtime_Quality);

    // If the import failed, report it
    if( !scener)
    {
        printf(" import failed %s\n", pFile.c_str());
        exit(1);
    }

    kolmedeeindex++;

    fprintf(stdout," success\n");

    // We're done. Everything will be cleaned up by the importer destructor
    return scener;
}

int shader_index = 1; // 1 not 0 because console shader is loaded separately
bool LoadShaders() 
{
    if(shader_index == sizeof(shaders) / sizeof(shaders[0])) return false;
    shaders[shader_index] = LoadShader(shaderss[shader_index]);
    shader_index++;

    return true;
}
int texture_index = 0;
bool LoadTextures()
{
    if(texture_index == sizeof(textures) / sizeof(textures[0])) return false;
    textures[texture_index] = LoadTexture(texturess[texture_index]);
    texture_index++;

    return true;
}
int assets_3dmodel_total = 1;
bool Load3DAssets()
{
    kapsule = Import3DFromFile("data/models/kapsule.obj");
    LoadGLTextures(kapsule);

    return true;
}

bool assets_loaded = false;
int skip_frames = 10;
int skip_frames_count = 0;
clock_t t_loader_begin = NULL, t_loader_d;
int assets_index = -1, assets_total = -1;
int loader_phase = -1;
float loading_time = NULL;
void Loader()
{
    if(t_loader_begin == NULL) { current_scene = 0; t_loader_begin = clock(); assets_total = ((sizeof(shaders) / sizeof(shaders[0])) + (sizeof(textures) / sizeof(textures[0])) + assets_3dmodel_total ); } else { loading_time = (float)((((float)t_loader_d - (float)t_loader_begin) / 1000000.0F ) * 1000); }
    if(assets_total == -1) {
        printf('ERROR: Loader(): No assets to load and/or something is just terribly wrong! Terminating...');
        exit(1);
    }

    float phase = (float)((float)(assets_index) / (float)(assets_total));
    aiScene* loaderscene = bilothorn;

    //glClearColor (phase,phase,phase,1.0);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float mymillis = phase*4020;
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb); // default
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(0);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);

    glShadeModel(GL_SMOOTH);    // Enables Smooth Shading
    //glClearColor(1.0f-phase,1.0f-phase,1.0f-phase, 1.0f);
    glClearDepth(1.0f); // Depth Buffer Setup
    glEnable(GL_DEPTH_TEST);    // Enables Depth Testing
    glDepthFunc(GL_LEQUAL); // The Type Of Depth Test To Do
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Really Nice Perspective Calculation

    glDisable(GL_LIGHTING);
    glEnable(GL_LIGHT0); // Uses default lighting parameters
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glDisable(GL_NORMALIZE);

    float r, g, b, a = 1.0f;
    if(loader_phase < 1) {
        r = g = b = sin(loading_time*100);
    } else {
        r = g = b = a;
    }
    GLfloat LightAmbient[]= { r, g, b, a };
    GLfloat LightDiffuse[]= { r, g, b, a };
    GLfloat LightPosition[]= { 0.0f, 0.0f, 15.0f, 1.0f};
    //GLfloat LightAmbient[]= { 0.0f, 1.0f-phase, 0.0f, 1.0f };
   // GLfloat LightDiffuse[]= { 0.0f, 1.0f-phase, 0.0f, 1.0f };
  //  GLfloat LightPosition[]= { 0.0f, 0.0f, 15.0f, 1.0f };

    glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
    glEnable(GL_LIGHT1);

    float tmp;
    float zoom = -50.0f+((mymillis)*0.05*0.2);

    if (zoom > -0.5 && startti == 0) { startti = mymillis; }
    if (zoom >= -0.5) { zoom = -0.5; jormymillis=(mymillis-startti); }


    if (jormymillis > 0) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    glLoadIdentity();

    glRotatef(180.0f*phase*0.974,0.0,0.0,1.0);
    glTranslatef(0.0f, 0.0f, zoom);
    /*if (jormymillis > 0) {
        glRotatef(jormymillis*0.0026,-1.0,0.0,0.0);
    }*/

    skip_frames_count++;
    if(skip_frames_count == skip_frames) {
        skip_frames_count = 0;
        assets_index++;
        printf("--- MIDISYS-ENGINE: Loading asset #%i", assets_index);

        // begin loading animation

        if(loader_phase == -1) {
        	loader_phase = 0;
        } else if(loader_phase == 0) {
        	if(!LoadShaders()) {
        		loader_phase = 1;
        	} else {
        	}
        } else if(loader_phase == 1) {
            if(!LoadTextures()) {
            	loader_phase = 2;
        	} else {
        	}
        } else if(loader_phase == 2) {
            printf("..%i", assets_total);
            // load all 3d models; after that all asset loading is done
            Load3DAssets();
            loader_phase = 3;
        } else {
            assets_loaded = true;
        }
    } else {
        // format bilotrip terminal 1.6.2.0

    	/*if(phase > 0.975f) {
    		phase = 1.0f-phase;
    	}*/
        glClearColor ((1.0f-phase)*0.86,(1.0f-phase)*0.86,(1.0f-phase)*0.86,1.0);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glFlush();
        glutSwapBuffers();
    }

    //printf("n:%i\n",(int)((float)((float)(assets_index) / (float)(assets_total)) * 3.0f));
    /*glBegin(GL_TRIANGLES);
        glVertex3f(-25.0f,-25.0f,-50.0f);
        glVertex3f(-25.0f,25.0f,-50.0f);
        glVertex3f(25.0f,25.0f,-50.0f);
    glEnd();*/
    for(int n = 0; n < ((int)(phase*3.0) + 1); n++) {
    	glRotatef(120.0f,0.0,0.0,(float)(n));
	    recursive_render(loaderscene, loaderscene->mRootNode, 2.0+jormymillis*0.001);
    	//if (jormymillis > 0)recursive_render(loaderscene, loaderscene->mRootNode, 4.0-jormymillis*0.001);    	
    }


    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fake_framebuffer); // default
    glDisable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glUseProgram(shaders[hex]);
    float widthLoc5 = glGetUniformLocation(shaders[hex], "width");
    float heightLoc5 = glGetUniformLocation(shaders[hex], "height");
    float timeLoc5 = glGetUniformLocation(shaders[hex], "time");
    float effuLoc5 = glGetUniformLocation(shaders[hex], "effu");

    glUniform1f(widthLoc5, g_Width);
    glUniform1f(heightLoc5, g_Height);
    glUniform1f(timeLoc5, mymillis/100);
    glUniform1f(effuLoc5, jormymillis > 0 ? 1.0 : 0.0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fb_tex);

    float location5 = glGetUniformLocation(shaders[hex], "texture0");
    glUniform1i(location5, 0);

    glLoadIdentity();

    glTranslatef(-1.2, -1.0, -1.0);
    //glTranslatef(0.0, 0.0, -1.0);

    int i=0;
    int j=0;
    glBegin(GL_QUADS);
    glVertex2f(i, j);
    glVertex2f(i + 100, j);
    glVertex2f(i + 100, j + 100);
    glVertex2f(i, j + 100);
    glEnd();

    glFlush();
    glutSwapBuffers();

    glDisable(GL_DEPTH_TEST);    // Enables Depth Testing
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb); // default
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL); // The Type Of Depth Test To Do
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Really Nice Perspective Calculation
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); // default
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    if (quitflag == 0) glutPostRedisplay();
}

int vieterframe = 0;
float vieterstart = 0;

void LeadMaskScene()
{
glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb); // fbo
glClear(GL_DEPTH_BUFFER_BIT);
float mymillis = millis;
glUseProgram(shaders[projector]);

int kuvaflag = 0;

if (millis >= 0 && millis < 25000) {
kuvaflag = 0;
}
else if (millis >= 25000 && millis < 37000) {
kuvaflag = 1;
}
else if (millis >= 37000 && millis < 48500) {
kuvaflag = 2;
}
else if (millis >= 48500 && millis < 64000) {
kuvaflag = 3;
}
else if (millis >= 64000 && millis < 76100) {
kuvaflag = 4;
}
else if (millis >= 76100 && millis < 84000) {
kuvaflag = 5;
}
else if (millis >= 84000) {
kuvaflag = 6;
if (vieterstart == 0) vieterstart = mymillis;
vieterframe = (int)((mymillis-vieterstart)*0.004);
if (vieterframe > 12) { vieterframe = 0; vieterstart = 0; }
}

glEnable(GL_BLEND);
glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);

GLint widthLoc5 = glGetUniformLocation(shaders[projector], "width");
GLint heightLoc5 = glGetUniformLocation(shaders[projector], "height");
GLint timeLoc5 = glGetUniformLocation(shaders[projector], "time");
GLint alphaLoc5 = glGetUniformLocation(shaders[projector], "alpha");

glUniform1f(widthLoc5, g_Width);
glUniform1f(heightLoc5, g_Height);
glUniform1f(timeLoc5, mymillis - (millis < 64000 ? 0 : 50000));
glUniform1f(alphaLoc5, mymillis*0.0001+0.2-cos(mymillis*0.0005)*0.15);

glActiveTexture(GL_TEXTURE0);
if (kuvaflag == 0)
glBindTexture(GL_TEXTURE_2D, textures[tex_scene]);
else if (kuvaflag == 1)
glBindTexture(GL_TEXTURE_2D, textures[tex_dude]);
else if (kuvaflag == 2)
glBindTexture(GL_TEXTURE_2D, textures[tex_dude2]);
else if (kuvaflag == 3)
glBindTexture(GL_TEXTURE_2D, textures[tex_mask]);
else if (kuvaflag == 4)
glBindTexture(GL_TEXTURE_2D, textures[tex_note]);
else if (kuvaflag == 5)
glBindTexture(GL_TEXTURE_2D, textures[tex_exit]);
else if (kuvaflag == 6)
glBindTexture(GL_TEXTURE_2D, textures[tex_v0+vieterframe]);

GLint location5 = glGetUniformLocation(shaders[projector], "texture0");
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

glUseProgram(shaders[fsquad]);

glEnable(GL_BLEND);
glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_ONE_MINUS_DST_COLOR);

GLint widthLoc6 = glGetUniformLocation(shaders[fsquad], "width");
GLint heightLoc6 = glGetUniformLocation(shaders[fsquad], "height");
GLint timeLoc6 = glGetUniformLocation(shaders[fsquad], "time");
GLint alphaLoc6 = glGetUniformLocation(shaders[fsquad], "alpha");
GLint gammaLoc = glGetUniformLocation(shaders[fsquad], "gamma");
GLint gridLoc6 = glGetUniformLocation(shaders[fsquad], "grid");
GLint alphamodeLoc5 = glGetUniformLocation(shaders[fsquad], "alphamode");
glUniform1f(alphamodeLoc5, 0.0f);

glUniform1f(gridLoc6, 0.001+cos(mymillis)*0.0005);


glUniform1f(widthLoc6, g_Width);
glUniform1f(heightLoc6, g_Height);
glUniform1f(timeLoc6, mymillis/100);
glUniform1f(alphaLoc6, 0.1+abs(cos(mymillis*0.08)*0.05));
glUniform1f(gammaLoc, 0.0f);

glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, fb_tex);

GLint location6 = glGetUniformLocation(shaders[fsquad], "texture0");
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

glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fake_framebuffer); // default

glUseProgram(shaders[fsquad]);

glDisable(GL_BLEND);

glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

GLint widthLoc7 = glGetUniformLocation(shaders[fsquad], "width");
GLint heightLoc7 = glGetUniformLocation(shaders[fsquad], "height");
GLint timeLoc7 = glGetUniformLocation(shaders[fsquad], "time");
GLint alphaLoc7 = glGetUniformLocation(shaders[fsquad], "alpha");
GLint gammaLoc2 = glGetUniformLocation(shaders[fsquad], "gamma");
GLint gridLoc = glGetUniformLocation(shaders[fsquad], "grid");
GLint alphamodeLoc7 = glGetUniformLocation(shaders[fsquad], "alphamode");
glUniform1f(alphamodeLoc7, 0.0f);

glUniform1f(widthLoc7, g_Width);
glUniform1f(heightLoc7, g_Height);
glUniform1f(timeLoc7, mymillis/100);
glUniform1f(alphaLoc7, 1.0);
glUniform1f(gammaLoc2, 4.0f);
glUniform1f(gridLoc, 1.0f+tan(mymillis*10)*0.3);

glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, fb_tex2);

GLint location7 = glGetUniformLocation(shaders[fsquad], "texture0");
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

glUseProgram(shaders[projector]);

glEnable(GL_BLEND);
glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
glBlendFunc(GL_SRC_COLOR, GL_DST_ALPHA);

widthLoc5 = glGetUniformLocation(shaders[projector], "width");
heightLoc5 = glGetUniformLocation(shaders[projector], "height");
timeLoc5 = glGetUniformLocation(shaders[projector], "time");
alphaLoc5 = glGetUniformLocation(shaders[projector], "alpha");

glUniform1f(widthLoc5, g_Width);
glUniform1f(heightLoc5, g_Height);
glUniform1f(timeLoc5, mymillis);
glUniform1f(alphaLoc5, 1.0);

glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, textures[tex_scene]);

location5 = glGetUniformLocation(shaders[projector], "texture0");
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


if (millis > 37400 && millis < 37600) kapsule_render();


}

int copbeatcounter = -1;
int coptexid = 0;
void CopScene()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fake_framebuffer); // default

	float mymillis = (millis-scene_start_millis);
	glUseProgram(shaders[copquad]);

	glEnable(GL_BLEND);


	float joo = cos(mymillis);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, joo < 0.5 ? GL_MODULATE : GL_SUBTRACT );
	glBlendFunc(GL_SRC_COLOR, joo < 0.5 ? GL_DST_ALPHA : GL_ONE_MINUS_DST_COLOR);

	GLint widthLoc5 = glGetUniformLocation(shaders[copquad], "width");
	GLint heightLoc5 = glGetUniformLocation(shaders[copquad], "height");
	GLint timeLoc5 = glGetUniformLocation(shaders[copquad], "time");
	GLint alphaLoc5 = glGetUniformLocation(shaders[copquad], "alpha");
	GLint gridLoc6 = glGetUniformLocation(shaders[copquad], "grid");
	glUniform1f(gridLoc6, tan(mymillis));

	glUniform1f(widthLoc5, g_Width);
	glUniform1f(heightLoc5, g_Height);
	glUniform1f(timeLoc5, mymillis/100);
	glUniform1f(alphaLoc5, cos(mymillis*0.1)*0.01);

    if (scene_shader_params[2] == 36) { copbeatcounter++; }
    if (copbeatcounter > 0) { coptexid++; copbeatcounter = 0;}

	//int texind = (int)(mymillis*(0.001/2));
    int texind = coptexid;
	if (texind > 30) texind = 30;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[tex_copkiller + texind]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures[texind == tex_copkiller ? tex_copkiller+17 :
              tex_copkiller + (abs(texind-1))]);

	GLint location5 = glGetUniformLocation(shaders[copquad], "texture0");
	glUniform1i(location5, 0);

	GLint location6 = glGetUniformLocation(shaders[copquad], "texture1");
	glUniform1i(location6, 1);

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

static int video_started = 0;

void MarssiScene()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fake_framebuffer); // default
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	if (video_started == 0)
	{
		myVideoFrame->play();
		video_started = 1;
	}

	myVideoFrame->render();
}

void LongScene()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fake_framebuffer); // default
    glDisable(GL_BLEND);
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaders[fsquad]);
    float mymillis = (((millis)-scene_start_millis));

    GLint widthLoc5 = glGetUniformLocation(shaders[fsquad], "width");
    GLint heightLoc5 = glGetUniformLocation(shaders[fsquad], "height");
    GLint timeLoc5 = glGetUniformLocation(shaders[fsquad], "time");
    GLint alphaLoc5 = glGetUniformLocation(shaders[fsquad], "alpha");
    GLint gammaLoc = glGetUniformLocation(shaders[fsquad], "gamma");
    GLint gridLoc6 = glGetUniformLocation(shaders[fsquad], "grid");
    glUniform1f(gridLoc6, 0.0f);
    glUniform1f(gammaLoc, 0.0f);
    GLint alphamodeLoc5 = glGetUniformLocation(shaders[fsquad], "alphamode");
    glUniform1f(alphamodeLoc5, 0.0f);

    glUniform1f(widthLoc5, g_Width);
    glUniform1f(heightLoc5, g_Height);
    glUniform1f(timeLoc5, mymillis/100);
    glUniform1f(alphaLoc5, 0.0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[tex_aegis]);

    GLint location5 = glGetUniformLocation(shaders[fsquad], "texture0");
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

//
    if (mymillis > 2500) {
        int plussati = 0;
        if (mymillis > 2800)plussati=400;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        int lloff = (int)(((plussati+mymillis)-2500)*0.0025);
        if (lloff >= 4) lloff = 4;
        widthLoc5 = glGetUniformLocation(shaders[fsquad], "width");
        heightLoc5 = glGetUniformLocation(shaders[fsquad], "height");
        timeLoc5 = glGetUniformLocation(shaders[fsquad], "time");
        alphaLoc5 = glGetUniformLocation(shaders[fsquad], "alpha");
        float alphamodeLoc = glGetUniformLocation(shaders[fsquad], "alphamode");

        glUniform1f(widthLoc5, g_Width);
        glUniform1f(heightLoc5, g_Height);
        glUniform1f(timeLoc5, mymillis/100);
        glUniform1f(alphaLoc5, 1.0);
        glUniform1f(alphamodeLoc, 1.0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[tex_ll1+lloff]);

        GLint location5 = glGetUniformLocation(shaders[fsquad], "texture0");
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

}

int majic_texnum = 0;
int noclearframes = 0;
void EyeScene()
{
	int i, j;

	int lisuri = 0;

	//printf("millis:%f\n",millis);
	if (millis > 225200) lisuri = 50000+(millis-225200)*0.5;
	if (lisuri > 150000) lisuri = 150000;

	float mymillis = (((millis)-scene_start_millis)*100);
	float mymillis2 = (((millis+lisuri)-scene_start_millis)*100);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb); // fbo

	glClearColor((float)(scene_shader_params[1]/127)*0.7,(float)(scene_shader_params[1]/127)*0.4,(float)(scene_shader_params[1]/127)*0.8,0.9-0.005*(float)(scene_shader_params[1]/127));

	if ((scene_shader_params[0] == 65 && scene_shader_param_type[0] == 0) || noclearframes > 200)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		beatmode = -beatmode;
		noclearframes = 0;
	}
	else
	{
		noclearframes++;
	}

	// render fbo copy to fbo
	glEnable(GL_BLEND);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_DST_COLOR);

	glUseProgram(shaders[eye_post]);

//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLint widthLoc3 = glGetUniformLocation(shaders[eye_post], "width");
	GLint heightLoc3 = glGetUniformLocation(shaders[eye_post], "height");
	GLint timeLoc3 = glGetUniformLocation(shaders[eye_post], "time");

	glUniform1f(widthLoc3, g_Width);
	glUniform1f(heightLoc3, g_Height);
	glUniform1f(timeLoc3, mymillis/100);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fb_tex);

	GLint location3 = glGetUniformLocation(shaders[eye_post], "texture0");
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

	glUseProgram(shaders[eye]);

	GLfloat waveTime = 1+atan(mymillis*0.0001)*0.1,
			waveWidth = cos(mymillis*0.000001)*1000+atan(mymillis*0.00001)*2.0,
			waveHeight = sin(mymillis*0.0001)*100*atan(mymillis*0.00001)*2.0,
			waveFreq = 0.0001+cos(mymillis*0.000003)*1000.1;
	GLint waveTimeLoc = glGetUniformLocation(shaders[eye], "waveTime");
	GLint waveWidthLoc = glGetUniformLocation(shaders[eye], "waveWidth");
	GLint waveHeightLoc = glGetUniformLocation(shaders[eye], "waveHeight");

	GLint widthLoc = glGetUniformLocation(shaders[eye], "width");
	GLint heightLoc = glGetUniformLocation(shaders[eye], "height");

	GLint timeLoc = glGetUniformLocation(shaders[eye], "time");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[tex_grayeye]);

	GLuint location;
	location = glGetUniformLocation(shaders[eye], "texture0");
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

	glUniform1f(timeLoc, mymillis2/100);


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

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fake_framebuffer); // default

	glUseProgram(shaders[eye_post]);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_BLEND);

	GLint widthLoc2 = glGetUniformLocation(shaders[eye_post], "width");
	GLint heightLoc2 = glGetUniformLocation(shaders[eye_post], "height");
	GLint timeLoc2 = glGetUniformLocation(shaders[eye_post], "time");

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
	glBindTexture(GL_TEXTURE_2D, textures[tex_room + room_texnum]);

	GLint location2 = glGetUniformLocation(shaders[eye_post], "texture0");
	glUniform1i(location2, 0);

	GLint location4 = glGetUniformLocation(shaders[eye_post], "texture1");
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
		glUseProgram(shaders[fsquad]);

		glEnable(GL_BLEND);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_SUBTRACT);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR);

		GLint widthLoc5 = glGetUniformLocation(shaders[fsquad], "width");
		GLint heightLoc5 = glGetUniformLocation(shaders[fsquad], "height");
		GLint timeLoc5 = glGetUniformLocation(shaders[fsquad], "time");
		GLint alphaLoc5 = glGetUniformLocation(shaders[fsquad], "alpha");
        GLint alphamodeLoc5 = glGetUniformLocation(shaders[fsquad], "alphamode");
        glUniform1f(alphamodeLoc5, 0.0f);

		glUniform1f(widthLoc5, g_Width);
		glUniform1f(heightLoc5, g_Height);
		glUniform1f(timeLoc5, mymillis/100);
		glUniform1f(alphaLoc5, cos(mymillis*0.1)*0.1);

		glActiveTexture(GL_TEXTURE0);
        
        majic_texnum = (int)(rand() % 4);

		glBindTexture(GL_TEXTURE_2D, textures[tex_majestic1+majic_texnum]);

		GLint location5 = glGetUniformLocation(shaders[fsquad], "texture0");
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

int redcounter = 1;

void RedCircleScene()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fake_framebuffer); // fbo

	glUseProgram(shaders[redcircle]);
	float mymillis = (millis-scene_start_millis)*160;

    if (scene_shader_params[2] == 36) { redcounter++; }
    if (redcounter > 4) { redcounter = 0; glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

	glEnable(GL_BLEND);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLint widthLoc2 = glGetUniformLocation(shaders[redcircle], "width");
	GLint heightLoc2 = glGetUniformLocation(shaders[redcircle], "height");

	glUniform1f(widthLoc2, g_Width);
	glUniform1f(heightLoc2, g_Height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[tex_grayeye]);

	GLint location2 = glGetUniformLocation(shaders[redcircle], "texture0");
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

void VHSPost(float effuon)
{
    if (current_scene == 6) effuon = 2.0f;
	float mymillis = (millis-scene_start_millis);

    if (current_scene == 1 || current_scene == 2)
    {
        if (millis > 105000 && millis < 113000) brieflycase_render();
    }

    if (current_scene == 1 || current_scene == 3) 
    {
// console crap
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fake_framebuffer); // default

		glLoadIdentity();

	    mat4_set_identity( &projection );
	    mat4_set_identity( &model );
	    mat4_set_identity( &view );

		mat4_set_orthographic( &projection, 0, g_Width, 0, g_Height, -1, 1);

		console_render( console );
	}	


	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); // default
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);	// Depth Buffer Setup

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaders[vhs]);

	glDisable(GL_BLEND);
	float widthLoc5 = glGetUniformLocation(shaders[vhs], "width");
	float heightLoc5 = glGetUniformLocation(shaders[vhs], "height");
	float timeLoc5 = glGetUniformLocation(shaders[vhs], "time");
	float effuLoc5 = glGetUniformLocation(shaders[vhs], "effu");

    float beatLoc = glGetUniformLocation(shaders[vhs], "beat");

    if ((current_scene == 2 || current_scene == 1 || current_scene == 4) && (millis > 55000 && millis < 186000))
    {
        if (scene_shader_params[2] == 36) { vhsbeat = 1.0f; vhsbeat_start = mymillis; }
        vhsbeat-=((mymillis-vhsbeat_start)*0.00005);
        if (vhsbeat <= (current_scene == 2 ? 0.2 : 0.1)) vhsbeat = (current_scene == 2 ? 0.2 : 0.1);
    }
    else if (current_scene == 3) {
        if (millis >= 182500 && vhsbeat_start < 182500) { 
            vhsbeat_start = millis;
            //printf("START!\n");
        }

        if (vhsbeat_start >= 182500)
        {
            vhsbeat = (millis-vhsbeat_start)*(0.02/30);
            //printf("vhsbeat:%f\n", vhsbeat);
        }
    }
    else if (current_scene == 6)
    {
       if (scene_shader_params[2] == 36) { vhsbeat = 1.0f; vhsbeat_start = mymillis; }
        vhsbeat-=((mymillis-vhsbeat_start)*0.0005);
        if (vhsbeat < 0.0) vhsbeat = 0.0;

    }
    else{
        vhsbeat = 0.0;
    }


	glUniform1f(widthLoc5, g_Width);
	glUniform1f(heightLoc5, g_Height);
	glUniform1f(timeLoc5, mymillis/100);
	glUniform1f(effuLoc5, effuon);
    glUniform1f(beatLoc, vhsbeat);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fake_framebuffer_tex);

	float location5 = glGetUniformLocation(shaders[vhs], "texture0");
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
		if (timeline_trackindex[tracknum] >= timeline_tracklength[tracknum]) continue;

		MIDI_MSG currentMsg = timeline[tracknum][trackidx];


		int dw = (int)currentMsg.dwAbsPos;
		int tarkistus = (int)(dw)*1.212;

        // flush midi to correct position if debugging
        if (debugmode == 1)
        {
            while (tarkistus < intmillis)
            {
                timeline_trackindex[tracknum]++;
                trackidx = timeline_trackindex[tracknum];
                MIDI_MSG currentMsg2 = timeline[tracknum][trackidx];
                dw = (int)currentMsg2.dwAbsPos;
                tarkistus = (int)(dw)*1.212;
            }
            printf("DEBUG: midi track %d flushed to position: %d\n", tracknum, tarkistus);
        }

        // reset trigs
        if (scene_shader_param_type[mapping_paramnum[i]] == 0) scene_shader_params[mapping_paramnum[i]] = -1;

		//if (intmillis+155520 < tarkistus*1.212) break;
		if (intmillis < tarkistus) continue;

		timeline_trackindex[tracknum]++;

		int ev = 0;

		if (currentMsg.bImpliedMsg) { ev = currentMsg.iImpliedMsg; }
		else { ev = currentMsg.iType; }

//        DebugPrintEvent(ev, currentMsg);

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
//					printf("track #%d (%s), len %d, pos %d: dwAbsPos: %d (millis: %d) -> noteon: %d (shadermap num %d)\n", tracknum, timeline_trackname[tracknum], timeline_tracklength[tracknum], timeline_trackindex[tracknum], currentMsg.dwAbsPos, intmillis, trigVal, mapping_paramnum[i]);
//                    printf("shader param %d trig: %d\n", mapping_paramnum[i], trigVal);
				}
				else if (ev == msgNoteOff)
				{
					trigVal = -1;
				}

				scene_shader_params[mapping_paramnum[i]] = trigVal;
				scene_shader_param_type[mapping_paramnum[i]] = 0;
				if (ev == msgNoteOn) printf("sync (%s): %d: trig %d to: %d\n", timeline_trackname[tracknum], intmillis, mapping_paramnum[i], trigVal);
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

				printf("sync (%s): %d: param %d to: %d\n", timeline_trackname[tracknum], intmillis, mapping_paramnum[i], paramVal);
				break;
			}
		}
	}
    debugmode = 0;

}
///////////////////////////////////////////////////////////////// MAIN LOGIC

void quit()
{
    quitflag = 1;
    printf("--- MIDISYS ENGINE: time to quit()\n");
    if(!window) {
        glutLeaveGameMode();
        glutLeaveMainLoop();
    } else {
        glutDestroyWindow(window);
        glutLeaveMainLoop();
    }
}

double min(double a, double b)
{
	if (a<=b) return a;
	else return b;
}

GLint gFramesPerSecond = 0;
 
void FPS(void) {
  static GLint Frames = 0;         // frames averaged over 1000mS
  static GLuint Clock;             // [milliSeconds]
  static GLuint PreviousClock = 0; // [milliSeconds]
  static GLuint NextClock = 0;     // [milliSeconds]
 
  ++Frames;
  Clock = glutGet(GLUT_ELAPSED_TIME); //has limited resolution, so average over 1000mS
  if ( Clock < NextClock ) return;
 
  gFramesPerSecond = Frames/1; // store the averaged number of frames per second
 
  PreviousClock = Clock;
  NextClock = Clock+1000; // 1000mS=1S in the future
  Frames=0;
}

void logic()
{ 	
    if (assets_loaded) {
        if (music_started == -1) {
            printf("--- MIDISYS-ENGINE: total loading time: %f\n", loading_time);
            printf("--- MIDISYS-ENGINE: demo startup\n");
            BASS_ChannelPlay(music_channel,FALSE); music_started = 1;
            if(jump_to) { BASS_ChannelSetPosition(music_channel, jump_to, BASS_POS_BYTE); }
        } 

	    QWORD bytepos = BASS_ChannelGetPosition(music_channel, BASS_POS_BYTE);
	    double pos = BASS_ChannelBytes2Seconds(music_channel, bytepos);
	    millis = (float)pos*1000;

        if (millis > 367000) quit();

	    demo_playlist();
	    scene_logic[current_scene](0.0f);
    } else {
        t_loader_d = clock();
        //scene_logic[current_scene]((float)((float)(assets_index) / (float)(assets_total)));
    }

//	glutPostRedisplay();
}
 
void timer(int value)
{
  const int desiredFPS=60;
  glutTimerFunc(1000/desiredFPS, timer, ++value);
 
  logic();

  FPS(); //only call once per frame loop to measure FPS 
  if (quitflag == 0) glutPostRedisplay();
}


///////////////////////////////////////////////////////////// RENDER FUNCTION

void display(void)
{
    UpdateShaderParams();
	scene_render[current_scene]();
	if (current_scene != 7) VHSPost(assets_loaded && current_scene <= 4 ? 1.0 : 0.0);

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

	glDeleteTextures(1, &fb_tex2);
	glDeleteRenderbuffersEXT(1, &depth_rb2);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glDeleteFramebuffersEXT(1, &fb2);

	glDeleteTextures(1, &fake_framebuffer_tex);
	glDeleteRenderbuffersEXT(1, &depth_rb3);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glDeleteFramebuffersEXT(1, &fake_framebuffer);

	InitFBO();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void keyPress(unsigned char key, int x, int y)
{
	if (key == 27) quit();
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

//----------

	glGenTextures(1, &fake_framebuffer_tex);
	glBindTexture(GL_TEXTURE_2D, fake_framebuffer_tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, g_Width, g_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glGenFramebuffersEXT(1, &fake_framebuffer);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fake_framebuffer);

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fake_framebuffer_tex, 0);

	glGenRenderbuffersEXT(1, &depth_rb3);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_rb3);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, g_Width, g_Height);

	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth_rb3);

	glClearColor(0.0,0.0,0.0,1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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


    if(!window) {
	    glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
        // 1280x720, 32bit pixel depth, 60Hz refresh rate
        glutGameModeString( "1280x720:32@60" );

        // start fullscreen game mode
        glutEnterGameMode();
    } else {
        window = glutCreateWindow("majestic twelve by bilotrip");   
        glutReshapeWindow(c_Width, c_Height);
    }

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
	glutTimerFunc(0,timer,0);

	fprintf(stdout, "--- MIDISYS ENGINE: InitGraphics() success\n");
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

	// init console

    console = console_new();
    shader = shader_load("nauhoitin_shaders/v3f-t2f-c4f.vert",
                         shaderss[0]);

	// load & init video

	printf("--- nu laddar vi en videofilmen, det aer jaetteroligt att fuska poe Assembly\n");	

    OggPlayer ogg("data/video/video.ogg",AF_S16,2,44100,VF_BGRA);
	if(ogg.fail()) {
	   printf("could not open video file \"%s\"\n", "data/video/video.ogg\n");
	   return -2;
	}
    YUVFrame yuv_frame(ogg);
    myVideoFrame = &yuv_frame;

	// init MIDI sync and audio

	LoadMIDIEventList("data/music/testi.mid");
	ParseMIDITimeline("data/music/mapping.txt");
	InitAudio("data/music/EhkaValmisMC3.mp3");

    // Loader assets

    bilothorn = Import3DFromFile("data/models/bilotrip_logo_thorn.3ds");
    LoadGLTextures(bilothorn);
    biloflat = Import3DFromFile("data/models/bilotrip_logo_flat.3ds");
    LoadGLTextures(biloflat);
    bilothree = Import3DFromFile("data/models/bilotrip.3ds");
    LoadGLTextures(bilothree);
    bilotetra = Import3DFromFile("data/models/bilotrip_logo_tetra.3ds");
    LoadGLTextures(bilotetra);
    brieflycase = Import3DFromFile("data/models/brieflycase.obj");
    LoadGLTextures(brieflycase);

	// start mainloop

	StartMainLoop();

	return 0;
}