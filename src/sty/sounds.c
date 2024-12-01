       #include <unistd.h>
#ifdef WIN32
#include "SDL/include/SDL.h"
#undef main
#include "SDL/SDL_mixer.h"
#include "SDL/SDL_image.h"
#endif
#ifndef WIN32
#include "SDL.h"
#include "SDL_mixer.h"
#include "SDL_image.h"
#else
#include "Windows.h"
#endif

#define true 1
#define false 0

static int  lid = 0;  // Teletype lid is up(1) or down(0), down is a lot more quiet
#define DELTA_UP 20 // Sound with lid up are 20 higher up in the list
#define KEY_BASE 5  // there are 7 samples of a keypress, the first is sample #5 (+20 for closed lid)
#define CR_BASE 1 // and 3 CR samples, starting from #1
#define CHAR_BASE 16 // and 2 print character samples,
#define SPACE_BASE 18 // and 2 print space samples,

// === STY ====
#define TTYMUTE 0
#define TTYSOUNDCLOSED 1
#define TTYSOUNDOPEN 2
extern int tty_soundmode; // 0=mute, 1=lid_closed, 2=lid_open
// ============


#define NUM_WAVEFORMS 40
const char* _waveFileNames[] =
{
  "/opt/pidp11/bin/sounds/down-bell.wav",   // 0
  "/opt/pidp11/bin/sounds/down-cr-01.wav",  // 1
  "/opt/pidp11/bin/sounds/down-cr-02.wav",  // 2
  "/opt/pidp11/bin/sounds/down-cr-03.wav",  // 3
  "/opt/pidp11/bin/sounds/down-hum.wav",    // 4
  "/opt/pidp11/bin/sounds/down-key-01.wav", // 5
  "/opt/pidp11/bin/sounds/down-key-02.wav", // 6
  "/opt/pidp11/bin/sounds/down-key-03.wav", // 7
  "/opt/pidp11/bin/sounds/down-key-04.wav", // 8
  "/opt/pidp11/bin/sounds/down-key-05.wav", // 9
  "/opt/pidp11/bin/sounds/down-key-06.wav", // 10
  "/opt/pidp11/bin/sounds/down-key-07.wav", // 11
  "/opt/pidp11/bin/sounds/down-lid.wav",    // 12
  "/opt/pidp11/bin/sounds/down-motor-off.wav",  // 13
  "/opt/pidp11/bin/sounds/down-motor-on.wav",   // 14
  "/opt/pidp11/bin/sounds/down-platen.wav",     // 15
  "/opt/pidp11/bin/sounds/down-print-chars-01.wav",     // 16
  "/opt/pidp11/bin/sounds/down-print-chars-02.wav",     // 17
  "/opt/pidp11/bin/sounds/down-print-spaces-01.wav",    // 18
  "/opt/pidp11/bin/sounds/down-print-spaces-02.wav",    // 19
  "/opt/pidp11/bin/sounds/up-bell.wav",     // 20
  "/opt/pidp11/bin/sounds/up-cr-01.wav",    // 21
  "/opt/pidp11/bin/sounds/up-cr-02.wav",    // 22
  "/opt/pidp11/bin/sounds/up-cr-03.wav",    // 23
  "/opt/pidp11/bin/sounds/up-hum.wav",      // 24
  "/opt/pidp11/bin/sounds/up-key-01.wav",   // 25
  "/opt/pidp11/bin/sounds/up-key-02.wav",   // 26
  "/opt/pidp11/bin/sounds/up-key-03.wav",   // 27
  "/opt/pidp11/bin/sounds/up-key-04.wav",   // 28
  "/opt/pidp11/bin/sounds/up-key-05.wav",   // 29
  "/opt/pidp11/bin/sounds/up-key-06.wav",   // 30
  "/opt/pidp11/bin/sounds/up-key-07.wav",   // 31
  "/opt/pidp11/bin/sounds/up-lid.wav",      // 32
  "/opt/pidp11/bin/sounds/up-motor-off.wav",    // 33
  "/opt/pidp11/bin/sounds/up-motor-on.wav",     // 34
  "/opt/pidp11/bin/sounds/up-platen.wav",       // 35
  "/opt/pidp11/bin/sounds/up-print-chars-01.wav",   // 36
  "/opt/pidp11/bin/sounds/up-print-chars-02.wav",   // 37
  "/opt/pidp11/bin/sounds/up-print-spaces-01.wav",  // 38
  "/opt/pidp11/bin/sounds/up-print-spaces-02.wav"   // 39
};

Mix_Chunk* _sample[NUM_WAVEFORMS]; // was 2

// Initializes the application data
int sound_init(void) 
{
    // Initialize the SDL library with the Video subsystem
    SDL_Init(SDL_INIT_AUDIO );
    atexit(SDL_Quit);

    memset(_sample, 0, sizeof(Mix_Chunk*) * 2);

    // Set up the audio stream
    //int result = Mix_OpenAudio(48000, AUDIO_S16SYS, 2, 512);
    int result = Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 512);
    if( result < 0 )
    {
        fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
        exit(-1);
    }

    result = Mix_AllocateChannels(4);
    if( result < 0 )
    {
        fprintf(stderr, "Unable to allocate mixing channels: %s\n", SDL_GetError());
        exit(-1);
    }

    // Load waveforms
    for( int i = 0; i < NUM_WAVEFORMS; i++ )
    {
        _sample[i] = Mix_LoadWAV(_waveFileNames[i]);
        if( _sample[i] == NULL )
        {
            fprintf(stderr, "Unable to load wave file: %s\n", _waveFileNames[i]);
        }
    }
//fprintf(stderr, "sound_init done\r");

    Mix_PlayChannel(1, _sample[14 + lid*DELTA_UP ], 0 );
    while (Mix_Playing(1) != 0) {
    SDL_Delay(100); // wait 200 milliseconds
    }
    Mix_PlayChannel(1, _sample[4 + lid*DELTA_UP ], -1); // only runs the sample 65k, so not good but ok
    //sleep(10);
    return true;
}

int sound(char c)
{

            //Mix_PlayChannel(-1, _sample[5], 0);
            //return 1;
        if (c<32)
            return 0;

        lid = (tty_soundmode==2?1:0);       // sounds are different with the Teletype case opened.

        if (c==32)
        {
            Mix_PlayChannelTimed(-1, _sample[SPACE_BASE + lid*DELTA_UP + rand() % 2], 0, 100);
            //Mix_PlayChannel(-1, _sample[SPACE_BASE + lid*DELTA_UP + rand() % 2], 0);
            //printf("__ %c %d - %d\r\n", c , c, SPACE_BASE + lid*DELTA_UP + rand() % 2);
        }

        if ((c>=33) && (c<=95))
        {
            if (c==13)
            {
                Mix_PlayChannelTimed(-1, _sample[CR_BASE + lid*DELTA_UP + rand() % 2], 0,100);
                //Mix_PlayChannel(-1, _sample[CR_BASE + lid*DELTA_UP + rand() % 2], 0);
                //printf("CR %c %d - %d\r\n", c , c, CR_BASE + lid*DELTA_UP + rand() % 2);
                //usleep( rand() % 200);
            }
            else
            {
                Mix_PlayChannelTimed(-1, _sample[CHAR_BASE + lid*DELTA_UP + rand() % 2], 0,100);
                //Mix_PlayChannel(-1, _sample[CHAR_BASE + lid*DELTA_UP + rand() % 2], 0);
                //printf("CH %c %d - %d\r\n", c , c, CHAR_BASE + lid*DELTA_UP + rand() % 2);
            }
        }
        
        if ((c>=96) && (c<=127))    // surely, lowercase we can add as a Teletype option.
        {
            Mix_PlayChannelTimed(-1, _sample[CHAR_BASE + lid*DELTA_UP + rand() % 2], 0,100);
            //Mix_PlayChannel(-1, _sample[CHAR_BASE + lid + rand()*DELTA_UP % 2], 0);
            //printf("CH %c %d - %d\r\n", c , c, CHAR_BASE + lid*DELTA_UP + rand() % 2);
        }
        
        return 1;
}

int sound_close(void)
{

    Mix_PlayChannel(1, _sample[13 + lid*DELTA_UP ], 0); 

    while (Mix_Playing(1) != 0) {
    SDL_Delay(100); // wait 200 milliseconds
    }

    for( int i = 0; i < NUM_WAVEFORMS; i++ )
    {
        Mix_FreeChunk(_sample[i]);
    }

    Mix_CloseAudio();
    SDL_Quit();
    return 0;
}

int tty_backgroundhum(int hum)
{
if (hum)
    Mix_PlayChannel(1, _sample[4 + lid*DELTA_UP ], -1); // only runs the sample 65k, so not good but ok
else
    Mix_PlayChannel(1, _sample[4 + lid*DELTA_UP ], 0); // only runs the sample one more time
}
