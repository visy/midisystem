//=========================================================================
// Author      : Kambiz Veshgini
// Description : Opengl theora plyer; this sample uses a glsl shader for
//               yuv to rgb conversion. See yuv2rgb.frag .
//=========================================================================

#include <oggplayer.h>
#include <SDL/SDL_audio.h>
#include <SDL/SDL.h>
#ifdef _WINDOWS
#include <windows.h>
#endif
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>

class YUV2RGBShader {
public:
	YUV2RGBShader() {
        vert = glCreateShader(GL_VERTEX_SHADER);
        frag = glCreateShader(GL_FRAGMENT_SHADER);
    
		std::string vs = read_file("yuv2rgb.vert");
		std::string fs = read_file("yuv2rgb.frag");
		const char* v_source = vs.c_str();
		const char* f_source = fs.c_str();

        glShaderSource(vert, 1, &v_source,NULL);
        glShaderSource(frag, 1, &f_source,NULL);

        glCompileShader(vert);
        glCompileShader(frag);
        
		prog = glCreateProgram();
		glAttachShader(prog,vert);
		glAttachShader(prog,frag);
	
        glLinkProgram(prog);

		y_pos = glGetUniformLocation(prog,"y_tex");
		u_pos = glGetUniformLocation(prog,"u_tex");
		v_pos = glGetUniformLocation(prog,"v_tex");
	}
	~YUV2RGBShader() {
        glDeleteShader(vert);
        glDeleteShader(frag);
        glDeleteProgram(prog);
	}
	void bind_textures(GLuint y_tex, GLuint u_tex, GLuint v_tex) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, y_tex);
		glUniform1i(y_pos, 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, u_tex);
		glUniform1i(u_pos, 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, v_tex);
		glUniform1i(v_pos, 2);
	}
    void use()
    {
        glUseProgram(prog);
    }
private:

    std::string read_file(std::string path){
        std::ifstream ifs(path.c_str(), std::ifstream::in);
        if(!ifs.is_open()){
            return "";
        }
        std::string s((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        ifs.close();
        return s;
    }
    GLuint vert, frag, prog;
	GLuint y_pos,u_pos,v_pos;
};

class YUVFrame {
public:
	YUVFrame(OggPlayer oggstream):ogg(oggstream), yuv2rgb_shader() {
		width = ogg.width(); height = ogg.height();
		// The textures are created when rendering the first frame
		y_tex = u_tex = v_tex = -1 ;
		// Thes variables help us render a fullscreen quad
		// with the right aspect ration
		const SDL_VideoInfo* vi = SDL_GetVideoInfo();
		float max = std::max<float>((float)width/vi->current_w,(float)height/vi->current_h);
		quad_w = width/max;
		quad_h = height/max;
		quad_x = (vi->current_w-quad_w)/2.0;
		quad_y = (vi->current_h-quad_h)/2.0;
	}
	~YUVFrame() {
		glDeleteTextures(1,&y_tex);
		glDeleteTextures(1,&u_tex);
		glDeleteTextures(1,&v_tex);
	}
	void render() {
		update();
		if(-1==y_tex) return; // not ready yet
		yuv2rgb_shader.use();
		yuv2rgb_shader.bind_textures(y_tex,u_tex,v_tex);
		glBegin(GL_QUADS);
		glTexCoord2f(0,1);glVertex2f(quad_x,quad_y);
		glTexCoord2f(0,0);glVertex2f(quad_x,quad_y+quad_h);
		glTexCoord2f(1,0);glVertex2f(quad_x+quad_w,quad_y+quad_h);
		glTexCoord2f(1,1);glVertex2f(quad_x+quad_w,quad_y);
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
			y_tex = gen_texture(yuv.y_width,yuv.y_height);
			u_tex = gen_texture(yuv.uv_width,yuv.uv_height);
			v_tex = gen_texture(yuv.uv_width,yuv.uv_height);
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
	YUV2RGBShader yuv2rgb_shader;
	OggPlayer ogg;
	GLuint y_tex,u_tex,v_tex;
	int width,height;
	float quad_w,quad_h,quad_x,quad_y;
};

void mixaudio(void *data, Uint8 *stream, int len) {
	OggPlayer* ogg=(OggPlayer*)data;
	ogg->audio_read((char*)stream,len);
}

bool init_audio(void* user_data) {
	SDL_AudioSpec fmt;
	fmt.freq = 44100;
	fmt.format = AUDIO_S16;
	fmt.channels = 2;
	fmt.samples = 512;
	fmt.callback = mixaudio;
	fmt.userdata = user_data;

	if (SDL_OpenAudio(&fmt, NULL) < 0) return false;
	SDL_PauseAudio(0);

	return true;
}

bool init_video(int w, int h){
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) return false;
	const SDL_VideoInfo* vi = SDL_GetVideoInfo();
	SDL_SetVideoMode(vi->current_w, vi->current_h, 0, SDL_OPENGL | SDL_FULLSCREEN);
	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0,vi->current_w,0,vi->current_h);
	return true;
}

void on_exit(){
	std::string error(SDL_GetError());
	if(!error.empty())
		std::cerr << "Error: "<< error << std::endl;
	SDL_CloseAudio();
	SDL_Quit();
}

int main( int argc, char* argv[] ) {
	atexit(on_exit);

	OggPlayer ogg("../sample video/trailer_400p.ogg",AF_S16,2,44100,VF_BGRA);
	if(ogg.fail()) {
		SDL_SetError("Could not open ../sample video/trailer_400p.ogg");
		return -2;
	}

	if(!init_audio((void*)&ogg) || !init_video(ogg.width(),ogg.height()))
		return -3;

	glewInit();

	if (!GLEW_VERSION_2_0){
		SDL_SetError("OpenGL 2.0 not supported");
		return -1;
	}

	YUVFrame yuv_frame(ogg);

	ogg.play();

	bool running=true;

	while( ogg.playing() && running ) {
		SDL_Event event;
		while(SDL_PollEvent(&event)){
			if(event.type==SDL_QUIT || event.type==SDL_KEYDOWN){ running = false; }
		}
		yuv_frame.render();
		SDL_GL_SwapBuffers();
	}
	return 0;
}
