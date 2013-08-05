//=========================================================================
// Author      : Kambiz Veshgini
// Description : Simple SDL theora plyer, this sample does not use a
//               yuv overlay 
//=========================================================================

#include <oggplayer.h>
#include <SDL/SDL_audio.h>
#include <SDL/SDL.h>
#include <iostream>


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

SDL_Surface* init_video(int w, int h){
  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) return NULL;
  return SDL_SetVideoMode(w, h, 0, SDL_SWSURFACE);
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
		return -1;
	}

	SDL_Surface *screen;
	if(!init_audio((void*)&ogg) || !(screen=init_video(ogg.width(),ogg.height())))
		  return -2;

	ogg.play();

	bool running=true;
	while( ogg.playing() && running ) {
		SDL_Event event;
		while(SDL_PollEvent(&event))
			if(event.type==SDL_QUIT){ running = false; }

	    SDL_LockSurface( screen );
		ogg.video_read((char*)screen->pixels,screen->pitch);
	    SDL_UnlockSurface( screen );
		SDL_Flip(screen);
		SDL_Delay(0);
	}
	return 0;
}
