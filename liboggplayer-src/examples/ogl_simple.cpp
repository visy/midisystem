//=========================================================================
// Author      : Kambiz Veshgini
// Description : Simple opengl theora plyer
//=========================================================================

#include <oggplayer.h>
#include <SDL/SDL_audio.h>
#include <SDL/SDL.h>
#ifdef _WINDOWS
	#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <algorithm>
#include <vector>

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
	// OggPlayer will automatically convert the audio and video
	// to the spcified formats
	OggPlayer ogg("../sample video/trailer_400p.ogg",AF_S16,2,44100,VF_BGRA);
	if(ogg.fail()) {
		SDL_SetError("Could not open ../sample video/trailer_400p.ogg");
		return -1;
	}

	if(!init_audio((void*)&ogg) || !init_video(ogg.width(),ogg.height()))
		return -2;

	// creat an opengl texture, we assume non power of two textures are
	// supported (opengl 2.0)
	GLuint tex;
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1,&tex);
	glBindTexture(GL_TEXTURE_2D,tex);
	glTexImage2D(GL_TEXTURE_2D,0,4,ogg.width(),ogg.height(),0,GL_BGRA_EXT,GL_BYTE,NULL);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	
	// start the playback
	ogg.play();
	
	// the quad_? variables are used to draw our textured quad
	const SDL_VideoInfo* vi = SDL_GetVideoInfo();
	float max = std::max<float>((float)ogg.width()/vi->current_w,(float)ogg.height()/vi->current_h);
	float quad_w = ogg.width()/max;
	float quad_h = ogg.height()/max;
	float quad_x = (vi->current_w-quad_w)/2.0;
	float quad_y = (vi->current_h-quad_h)/2.0;

	bool running=true;
	while( ogg.playing() && running ) {
		std::vector<char> bgra_buffer(ogg.width()*ogg.height()*4);
		
		// exit on key down
		SDL_Event event;
		while(SDL_PollEvent(&event))
			if(event.type==SDL_QUIT || event.type==SDL_KEYDOWN){ running = false; }

		glBindTexture(GL_TEXTURE_2D,tex);
		
		// read a frame, video_read(...) returns false if the last read frame is
		// still up to date, thus we only opdate the texture if we get new data
		if(ogg.video_read(&bgra_buffer[0]))
			glTexSubImage2D(GL_TEXTURE_2D,0,0,0,ogg.width(),ogg.height(),GL_BGRA_EXT,GL_UNSIGNED_BYTE,&bgra_buffer[0]);
		
		// draw a textured quad
		glBegin(GL_QUADS);
		glTexCoord2f(0,1);glVertex2f(quad_x,quad_y);
		glTexCoord2f(0,0);glVertex2f(quad_x,quad_y+quad_h);
		glTexCoord2f(1,0);glVertex2f(quad_x+quad_w,quad_y+quad_h);
		glTexCoord2f(1,1);glVertex2f(quad_x+quad_w,quad_y);
		glEnd();

		SDL_GL_SwapBuffers();
		
		// let the play thread decode some data
		SDL_Delay(0);
	}
	glDeleteTextures(1,&tex);
	return 0;
}
