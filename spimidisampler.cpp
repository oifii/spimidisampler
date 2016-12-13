/*
 * Copyright (c) 2010-2016 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

////////////////////////////////////////////////////////////////
//nakedsoftware.org, spi@oifii.org or stephane.poirier@oifii.org
//
//
//2012june18, creation from spimonitor (portmidi mm.c) for 
//				manually playing wavset library instrument
//
//nakedsoftware.org, spi@oifii.org or stephane.poirier@oifii.org
////////////////////////////////////////////////////////////////

#include "stdafx.h"

//2012june18, spi, begin
#include <ctime>
#include <string>
#include <map>
using namespace std;
#include "portaudio.h" 
#include "spiws_WavSet.h"
#include "spiws_Instrument.h"
#include "spiws_InstrumentSet.h"
#include <assert.h>
#include <windows.h>

//#define SPIMIDISAMPLER_INSTRUMENT_MAXNUMBEROFWAVSET		127
#define SPIMIDISAMPLER_INSTRUMENT_MAXNUMBEROFWAVSET			128

// Select sample format. 
#if 1
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 1
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt8
typedef char SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#else
#define PA_SAMPLE_TYPE  paUInt8
typedef unsigned char SAMPLE;
#define SAMPLE_SILENCE  (128)
#define PRINTF_S_FORMAT "%d"
#endif
//2012june18, spi, end


#include "stdlib.h"
#include "ctype.h"
//2012june18, spi, begin
//#include "string.h"
//2012june18, spi, end
#include "stdio.h"
#include "porttime.h"
#include "portmidi.h"

#define STRING_MAX 80

#define MIDI_CODE_MASK  0xf0
#define MIDI_CHN_MASK   0x0f
//#define MIDI_REALTIME   0xf8
//  #define MIDI_CHAN_MODE  0xfa 
#define MIDI_OFF_NOTE   0x80
#define MIDI_ON_NOTE    0x90
#define MIDI_POLY_TOUCH 0xa0
#define MIDI_CTRL       0xb0
#define MIDI_CH_PROGRAM 0xc0
#define MIDI_TOUCH      0xd0
#define MIDI_BEND       0xe0

#define MIDI_SYSEX      0xf0
#define MIDI_Q_FRAME	0xf1
#define MIDI_SONG_POINTER 0xf2
#define MIDI_SONG_SELECT 0xf3
#define MIDI_TUNE_REQ	0xf6
#define MIDI_EOX        0xf7
#define MIDI_TIME_CLOCK 0xf8
#define MIDI_START      0xfa
#define MIDI_CONTINUE	0xfb
#define MIDI_STOP       0xfc
#define MIDI_ACTIVE_SENSING 0xfe
#define MIDI_SYS_RESET  0xff

#define MIDI_ALL_SOUND_OFF 0x78
#define MIDI_RESET_CONTROLLERS 0x79
#define MIDI_LOCAL	0x7a
#define MIDI_ALL_OFF	0x7b
#define MIDI_OMNI_OFF	0x7c
#define MIDI_OMNI_ON	0x7d
#define MIDI_MONO_ON	0x7e
#define MIDI_POLY_ON	0x7f


#define private static

#ifndef false
#define false 0
#define true 1
#endif


PmStream* global_pPmStreamMIDIIN;      // midi input 
boolean global_active = false;     // set when global_pPmStreamMIDIIN is ready for reading 

int inputmidideviceid =  11; //alesis q49 midi port id (when midi yoke installed)
map<string,int> global_inputmididevicemap;
string inputmididevicename = "Q49"; //"In From MIDI Yoke:  1", "In From MIDI Yoke:  2", ... , "In From MIDI Yoke:  8"


//typedef int boolean;

int debug = false;	// never set, but referenced by userio.c 
boolean in_sysex = false;   // we are reading a sysex message 
boolean inited = false;     // suppress printing during command line parsing 
boolean done = false;       // when true, exit 
boolean notes = true;       // show notes? 
boolean controls = true;    // show continuous controllers 
boolean bender = true;      // record pitch bend etc.? 
boolean excldata = true;    // record system exclusive data? 
boolean verbose = true;     // show text representation? 
boolean realdata = true;    // record real time messages? 
boolean clksencnt = true;   // clock and active sense count on 
boolean chmode = true;      // show channel mode messages 
boolean pgchanges = true;   // show program changes 
boolean flush = false;	    // flush all pending MIDI data 

uint32_t filter = 0;            // remember state of midi filter 

uint32_t clockcount = 0;        // count of clocks 
uint32_t actsensecount = 0;     // cout of active sensing bytes 
uint32_t notescount = 0;        // #notes since last request 
uint32_t notestotal = 0;        // total #notes 

char val_format[] = "    Val %d\n";


//The event signaled when the app should be terminated.
HANDLE g_hTerminateEvent = NULL;
//Handles events that would normally terminate a console application. 
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType);



///////////////////////////////////////////////////////////////////////////////
//    Imported variables
///////////////////////////////////////////////////////////////////////////////

extern  int     abort_flag;

///////////////////////////////////////////////////////////////////////////////
//    Routines local to this module
///////////////////////////////////////////////////////////////////////////////

//private    void    mmexit(int code);
private void output(PmMessage data);
private int  put_pitch(int p);
private void showhelp();
private void showbytes(PmMessage data, int len, boolean newline);
private void showstatus(boolean flag);
private void doascii(char c);
private int  get_number(char *prompt);

// read a number from console
//
int get_number(char *prompt)
{
    char line[STRING_MAX];
    int n = 0, i;
    printf(prompt);
    while (n != 1) {
        n = scanf("%d", &i);
        fgets(line, STRING_MAX, stdin);

    }
    return i;
}


void receive_poll(PtTimestamp timestamp, void *userData)
{
    PmEvent event;
    int count; 
    if (!global_active) return;
    while ((count = Pm_Read(global_pPmStreamMIDIIN, &event, 1))) 
	{
        if (count == 1) 
		{
			//1) output message
			output(event.message);
			//2) play note
			Instrument* pInstrument = (Instrument*) userData;
			int msgstatus = Pm_MessageStatus(event.message);
			//if(msgstatus==MIDI_ON_NOTE)
			if(msgstatus>=MIDI_ON_NOTE && msgstatus<(MIDI_ON_NOTE+16) && Pm_MessageData2(event.message)!=0)
			{
				int notenumber = Pm_MessageData1(event.message);
				WavSet* pWavSet = pInstrument->GetWavSetFromMidiNoteNumber(notenumber);
				assert(pWavSet);
				pWavSet->Play(USING_SOX, 1.0f); //warning, don't know how to control play time length with sox
				//pWavSet->Play(USING_SPIPLAY, 1.0f);
				//pWavSet->Play(USING_SPISPECTRUMPLAY, 1.0f);
				//pWavSet->Play(USING_SPIPLAYSTREAM, 1.0f);
			}
		}
        else            
		{
			printf(Pm_GetErrorText((PmError)count)); //spi a cast as (PmError)
		}
    }
}

int Terminate();
///////////////////////////////////////////////////////////////////////////////
//               main
// Effect: prompts for parameters, starts monitor
///////////////////////////////////////////////////////////////////////////////
Instrument* global_pInstrument=NULL;
int main(int argc, char **argv)
{
	int nShowCmd = false;
	ShellExecuteA(NULL, "open", "begin.bat", "", NULL, nShowCmd);

	string instrumentnamepattern="";
	if(argc>1)
	{
		instrumentnamepattern=argv[1];
	}
	//int inputmididevice =  11; //alesis q49 midi port id (when midi yoke installed)
	//int inputmididevice =  1; //midi yoke 1 (when midi yoke installed)
	if(argc>2)
	{
		//inputmididevice=atoi(argv[2]);
		inputmididevicename=argv[2]; //"In From MIDI Yoke:  1", "In From MIDI Yoke:  2", ... , "In From MIDI Yoke:  8"
	}

	//2012june18, spi, begin
    //Auto-reset, initially non-signaled event 
    g_hTerminateEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    //Add the break handler
    ::SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);


	//////////////////////
	//initialize portaudio
	//////////////////////
	PaStreamParameters outputParameters;
    PaStream* stream;
    PaError myPaError;

	//////////////////////////
	//initialize random number
	//////////////////////////
	srand((unsigned)time(0));

	////////////////////////
	// initialize port audio 
	////////////////////////
    myPaError = Pa_Initialize();
    if( myPaError != paNoError ) //goto error;
	{
		Terminate();
		fprintf( stderr, "An error occured while using the portaudio stream\n" );
		fprintf( stderr, "Error number: %d\n", myPaError );
		fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( myPaError ) );
		return -1;
	}

	outputParameters.device = Pa_GetDefaultOutputDevice(); // default output device 
	if (outputParameters.device == paNoDevice) 
	{
		fprintf(stderr,"Error: No default output device.\n");
		//goto error;
		Terminate();
		fprintf( stderr, "An error occured while using the portaudio stream\n" );
		fprintf( stderr, "Error number: %d\n", myPaError );
		fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( myPaError ) );
		return -1;
	}
	outputParameters.channelCount = 2;//pWavSet->numChannels;
	outputParameters.sampleFormat =  PA_SAMPLE_TYPE;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	////////////////////////
	//populate InstrumentSet
	////////////////////////
	//InstrumentSet* pInstrumentSet=new InstrumentSet;
	//Instrument* pInstrument = new Instrument;
	global_pInstrument = new Instrument;
	//pInstrument->CreateFromName("piano", SPIMIDISAMPLER_INSTRUMENT_MAXNUMBEROFWAVSET);
	//pInstrument->CreateFromName("violin", SPIMIDISAMPLER_INSTRUMENT_MAXNUMBEROFWAVSET);
	//global_pInstrument->CreateFromWavFolder("C:\\Program Files (x86)\\Native Instruments\\Sample Libraries\\Kontakt 3 Library\\Orchestral\\Z - Samples\\03 Cello ensemble - 8\\VC-8_mV_0aT1-legsus_p", SPIMIDISAMPLER_INSTRUMENT_MAXNUMBEROFWAVSET);
	//global_pInstrument->CreateFromWavFilenamesFilter(NULL, SPIMIDISAMPLER_INSTRUMENT_MAXNUMBEROFWAVSET); //selects instrument randomly out of the "wavfolder*.txt" files
	if(instrumentnamepattern.empty())
	{
		//global_pInstrument->CreateWavSynth(INSTRUMENT_SYNTH_SAWWAV);
		global_pInstrument->CreateWavSynth(INSTRUMENT_SYNTH_SINWAV);
		//WriteWavFiles() so sox can play them
		global_pInstrument->WriteWavFiles(INSTRUMENT_TEMPFOLDER);
	}
	else
	{
		global_pInstrument->CreateFromWavFilenamesFilter(instrumentnamepattern.c_str(), SPIMIDISAMPLER_INSTRUMENT_MAXNUMBEROFWAVSET); //selects instrument out of the "wavfolder*.txt" files (based on the supplied instrument name pattern)
	}
	global_pInstrument->Play(&outputParameters, INSTRUMENT_WAVSETALLATONCE);
	//pInstrumentSet->instrumentvector.push_back(pInstrument);
	//2012june18, spi, end

	/*
    char *argument;
    int inp;
    PmError err;
    int i;
    if (argc > 1) { // first arg can change defaults 
        argument = argv[1];
        while (*argument) doascii(*argument++);
    }
    */
    PmError err;
	Pm_Initialize(); //2013dec09, added by spi, was not there

	/////////////////////////////
	//input midi device selection
	/////////////////////////////
	const PmDeviceInfo* deviceInfo;
    int numDevices = Pm_CountDevices();
    for( int i=0; i<numDevices; i++ )
    {
        deviceInfo = Pm_GetDeviceInfo( i );
		if (deviceInfo->input)
		{
			string devicenamestring = deviceInfo->name;
			global_inputmididevicemap.insert(pair<string,int>(devicenamestring,i));
		}
	}
	map<string,int>::iterator it;
	it = global_inputmididevicemap.find(inputmididevicename);
	if(it!=global_inputmididevicemap.end())
	{
		inputmidideviceid = (*it).second;
		printf("%s maps to %d\n", inputmididevicename.c_str(), inputmidideviceid);
		deviceInfo = Pm_GetDeviceInfo(inputmidideviceid);
	}
	else
	{
		assert(false);
		for(it=global_inputmididevicemap.begin(); it!=global_inputmididevicemap.end(); it++)
		{
			printf("%s maps to %d\n", (*it).first.c_str(), (*it).second);
		}
		printf("input midi device not found\n");
	}

    // use porttime callback to empty midi queue and print 
    Pt_Start(1, receive_poll, global_pInstrument); //Pt_Start(1, receive_poll, 0);
    // list device information 
    printf("MIDI input devices:\n");
    for (int i = 0; i < Pm_CountDevices(); i++) 
	{
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if (info->input) printf("%d: %s, %s\n", i, info->interf, info->name);
    }
    //inputmididevice = get_number("Type input device number: ");
	printf("device %d selected\n", inputmidideviceid);
	showhelp();

    err = Pm_OpenInput(&global_pPmStreamMIDIIN, inputmidideviceid, NULL, 512, NULL, NULL);
    if (err) 
	{
        printf(Pm_GetErrorText(err));
        Pt_Stop();
		Terminate();
        //mmexit(1);
    }
    Pm_SetFilter(global_pPmStreamMIDIIN, filter);
    inited = true; // now can document changes, set filter 
    printf("Midi Monitor ready.\n");

	/*
	global_active = true;
    //Wait indefinitely for user to break the app.
    ::WaitForSingleObject(g_hTerminateEvent, INFINITE);
	*/
	
    global_active = true;
    while (!done) 
	{
        char s[100];
        if (fgets(s, 100, stdin)) 
		{
            doascii(s[0]);
        }
    }
	
exit:
	Terminate();
	return 0;
	
	/*
error:
    //Pa_Terminate();
	
	Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", myPaError );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( myPaError ) );
    return -1;
	*/
//2012june18, spi, end
}

int Terminate()
{
	////////////////////
	//terminate portmidi
	////////////////////
    global_active = false;
    Pm_Close(global_pPmStreamMIDIIN);
    Pt_Stop();
    Pm_Terminate();
    //mmexit(0);
//2012june18, spi, begin
	/////////////////////
	//terminate portaudio
	/////////////////////
	Pa_Terminate();
	//if(pInstrumentSet) delete pInstrumentSet;
	//if(pWavSet) delete pWavSet;
	if(global_pInstrument) delete global_pInstrument;
	printf("Exiting!\n"); fflush(stdout);

	int nShowCmd = false;
	ShellExecuteA(NULL, "open", "end.bat", "", NULL, nShowCmd);
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//               doascii
// Inputs:
//    char c: input character
// Effect: interpret to revise flags
///////////////////////////////////////////////////////////////////////////////

private void doascii(char c)
{
    if (isupper(c)) c = tolower(c);
    if (c == 'q') done = true;
    else if (c == 'b') {
        bender = !bender;
        filter ^= PM_FILT_PITCHBEND;
        if (inited)
            printf("Pitch Bend, etc. %s\n", (bender ? "ON" : "OFF"));
    } else if (c == 'c') {
        controls = !controls;
        filter ^= PM_FILT_CONTROL;
        if (inited)
            printf("Control Change %s\n", (controls ? "ON" : "OFF"));
    } else if (c == 'h') {
        pgchanges = !pgchanges;
        filter ^= PM_FILT_PROGRAM;
        if (inited)
            printf("Program Changes %s\n", (pgchanges ? "ON" : "OFF"));
    } else if (c == 'n') {
        notes = !notes;
        filter ^= PM_FILT_NOTE;
        if (inited)
            printf("Notes %s\n", (notes ? "ON" : "OFF"));
    } else if (c == 'x') {
        excldata = !excldata;
        filter ^= PM_FILT_SYSEX;
        if (inited)
            printf("System Exclusive data %s\n", (excldata ? "ON" : "OFF"));
    } else if (c == 'r') {
        realdata = !realdata;
        filter ^= (PM_FILT_PLAY | PM_FILT_RESET | PM_FILT_TICK | PM_FILT_UNDEFINED);
        if (inited)
            printf("Real Time messages %s\n", (realdata ? "ON" : "OFF"));
    } else if (c == 'k') {
        clksencnt = !clksencnt;
        filter ^= PM_FILT_CLOCK;
        if (inited)
            printf("Clock and Active Sense Counting %s\n", (clksencnt ? "ON" : "OFF"));
        if (!clksencnt) clockcount = actsensecount = 0;
    } else if (c == 's') {
        if (clksencnt) {
            if (inited)
                printf("Clock Count %ld\nActive Sense Count %ld\n", 
                        (long) clockcount, (long) actsensecount);
        } else if (inited) {
            printf("Clock Counting not on\n");
        }
    } else if (c == 't') {
        notestotal+=notescount;
        if (inited)
            printf("This Note Count %ld\nTotal Note Count %ld\n",
                    (long) notescount, (long) notestotal);
        notescount=0;
    } else if (c == 'v') {
        verbose = !verbose;
        if (inited)
            printf("Verbose %s\n", (verbose ? "ON" : "OFF"));
    } else if (c == 'm') {
        chmode = !chmode;
        if (inited)
            printf("Channel Mode Messages %s", (chmode ? "ON" : "OFF"));
    } else {
        if (inited) {
            if (c == ' ') {
                PmEvent event;
                while (Pm_Read(global_pPmStreamMIDIIN, &event, 1)) ;	// flush midi input 
                printf("...FLUSHED MIDI INPUT\n\n");
            } else showhelp();
        }
    }
    if (inited) Pm_SetFilter(global_pPmStreamMIDIIN, filter);
}


/*
private void mmexit(int code)
{
    // if this is not being run from a console, maybe we should wait for
    // the user to read error messages before exiting
    //
    exit(code);
}
*/

//Called by the operating system in a separate thread to handle an app-terminating event. 
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT ||
        dwCtrlType == CTRL_BREAK_EVENT ||
        dwCtrlType == CTRL_CLOSE_EVENT)
    {
        // CTRL_C_EVENT - Ctrl+C was pressed 
        // CTRL_BREAK_EVENT - Ctrl+Break was pressed 
        // CTRL_CLOSE_EVENT - Console window was closed 
		Terminate();
        // Tell the main thread to exit the app 
        ::SetEvent(g_hTerminateEvent);
        return TRUE;
    }

    //Not an event handled by this function.
    //The only events that should be able to
	//reach this line of code are events that
    //should only be sent to services. 
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//               output
// Inputs:
//    data: midi message buffer holding one command or 4 bytes of sysex msg
// Effect: format and print  midi data
///////////////////////////////////////////////////////////////////////////////

char vel_format[] = "    Vel %d\n";

private void output(PmMessage data)
{
    int command;    // the current command 
    int chan;   // the midi channel of the current event 
    int len;    // used to get constant field width 

    // printf("output data %8x; ", data); 

    command = Pm_MessageStatus(data) & MIDI_CODE_MASK;
    chan = Pm_MessageStatus(data) & MIDI_CHN_MASK;

    if (in_sysex || Pm_MessageStatus(data) == MIDI_SYSEX) {
#define sysex_max 16
        int i;
        PmMessage data_copy = data;
        in_sysex = true;
        // look for MIDI_EOX in first 3 bytes 
        // if realtime messages are embedded in sysex message, they will
        // be printed as if they are part of the sysex message
        //
        for (i = 0; (i < 4) && ((data_copy & 0xFF) != MIDI_EOX); i++) 
            data_copy >>= 8;
        if (i < 4) {
            in_sysex = false;
            i++; // include the EOX byte in output 
        }
        showbytes(data, i, verbose);
        if (verbose) printf("System Exclusive\n");
    } else if (command == MIDI_ON_NOTE && Pm_MessageData2(data) != 0) {
        notescount++;
        if (notes) {
            showbytes(data, 3, verbose);
            if (verbose) {
                printf("NoteOn  Chan %2d Key %3d ", chan, Pm_MessageData1(data));
                len = put_pitch(Pm_MessageData1(data));
                printf(vel_format + len, Pm_MessageData2(data));
            }
        }
    } else if ((command == MIDI_ON_NOTE // && Pm_MessageData2(data) == 0
                || command == MIDI_OFF_NOTE) && notes) {
        showbytes(data, 3, verbose);
        if (verbose) {
            printf("NoteOff Chan %2d Key %3d ", chan, Pm_MessageData1(data));
            len = put_pitch(Pm_MessageData1(data));
            printf(vel_format + len, Pm_MessageData2(data));
        }
    } else if (command == MIDI_CH_PROGRAM && pgchanges) {
        showbytes(data, 2, verbose);
        if (verbose) {
            printf("  ProgChg Chan %2d Prog %2d\n", chan, Pm_MessageData1(data) + 1);
        }
    } else if (command == MIDI_CTRL) {
               // controls 121 (MIDI_RESET_CONTROLLER) to 127 are channel
               // mode messages. 
        if (Pm_MessageData1(data) < MIDI_ALL_SOUND_OFF) {
            showbytes(data, 3, verbose);
            if (verbose) {
                printf("CtrlChg Chan %2d Ctrl %2d Val %2d\n",
                       chan, Pm_MessageData1(data), Pm_MessageData2(data));
            }
        } else if (chmode) { // channel mode 
            showbytes(data, 3, verbose);
            if (verbose) {
                switch (Pm_MessageData1(data)) {
                  case MIDI_ALL_SOUND_OFF:
                      printf("All Sound Off, Chan %2d\n", chan);
                    break;
                  case MIDI_RESET_CONTROLLERS:
                    printf("Reset All Controllers, Chan %2d\n", chan);
                    break;
                  case MIDI_LOCAL:
                    printf("LocCtrl Chan %2d %s\n",
                            chan, Pm_MessageData2(data) ? "On" : "Off");
                    break;
                  case MIDI_ALL_OFF:
                    printf("All Off Chan %2d\n", chan);
                    break;
                  case MIDI_OMNI_OFF:
                    printf("OmniOff Chan %2d\n", chan);
                    break;
                  case MIDI_OMNI_ON:
                    printf("Omni On Chan %2d\n", chan);
                    break;
                  case MIDI_MONO_ON:
                    printf("Mono On Chan %2d\n", chan);
                    if (Pm_MessageData2(data))
                        printf(" to %d received channels\n", Pm_MessageData2(data));
                    else
                        printf(" to all received channels\n");
                    break;
                  case MIDI_POLY_ON:
                    printf("Poly On Chan %2d\n", chan);
                    break;
                }
            }
        }
    } else if (command == MIDI_POLY_TOUCH && bender) {
        showbytes(data, 3, verbose);
        if (verbose) {
            printf("P.Touch Chan %2d Key %2d ", chan, Pm_MessageData1(data));
            len = put_pitch(Pm_MessageData1(data));
            printf(val_format + len, Pm_MessageData2(data));
        }
    } else if (command == MIDI_TOUCH && bender) {
        showbytes(data, 2, verbose);
        if (verbose) {
            printf("  A.Touch Chan %2d Val %2d\n", chan, Pm_MessageData1(data));
        }
    } else if (command == MIDI_BEND && bender) {
        showbytes(data, 3, verbose);
        if (verbose) {
            printf("P.Bend  Chan %2d Val %2d\n", chan,
                    (Pm_MessageData1(data) + (Pm_MessageData2(data)<<7)));
        }
    } else if (Pm_MessageStatus(data) == MIDI_SONG_POINTER) {
        showbytes(data, 3, verbose);
        if (verbose) {
            printf("    Song Position %d\n",
                    (Pm_MessageData1(data) + (Pm_MessageData2(data)<<7)));
        }
    } else if (Pm_MessageStatus(data) == MIDI_SONG_SELECT) {
        showbytes(data, 2, verbose);
        if (verbose) {
            printf("    Song Select %d\n", Pm_MessageData1(data));
        }
    } else if (Pm_MessageStatus(data) == MIDI_TUNE_REQ) {
        showbytes(data, 1, verbose);
        if (verbose) {
            printf("    Tune Request\n");
        }
    } else if (Pm_MessageStatus(data) == MIDI_Q_FRAME && realdata) {
        showbytes(data, 2, verbose);
        if (verbose) {
            printf("    Time Code Quarter Frame Type %d Values %d\n",
                    (Pm_MessageData1(data) & 0x70) >> 4, Pm_MessageData1(data) & 0xf);
        }
    } else if (Pm_MessageStatus(data) == MIDI_START && realdata) {
        showbytes(data, 1, verbose);
        if (verbose) {
            printf("    Start\n");
        }
    } else if (Pm_MessageStatus(data) == MIDI_CONTINUE && realdata) {
        showbytes(data, 1, verbose);
        if (verbose) {
            printf("    Continue\n");
        }
    } else if (Pm_MessageStatus(data) == MIDI_STOP && realdata) {
        showbytes(data, 1, verbose);
        if (verbose) {
            printf("    Stop\n");
        }
    } else if (Pm_MessageStatus(data) == MIDI_SYS_RESET && realdata) {
        showbytes(data, 1, verbose);
        if (verbose) {
            printf("    System Reset\n");
        }
    } else if (Pm_MessageStatus(data) == MIDI_TIME_CLOCK) {
        if (clksencnt) clockcount++;
        else if (realdata) {
            showbytes(data, 1, verbose);
            if (verbose) {
                printf("    Clock\n");
            }
        }
    } else if (Pm_MessageStatus(data) == MIDI_ACTIVE_SENSING) {
        if (clksencnt) actsensecount++;
        else if (realdata) {
            showbytes(data, 1, verbose);
            if (verbose) {
                printf("    Active Sensing\n");
            }
        }
    } else showbytes(data, 3, verbose);
    fflush(stdout);
}


/////////////////////////////////////////////////////////////////////////////
//               put_pitch
// Inputs:
//    int p: pitch number
// Effect: write out the pitch name for a given number
/////////////////////////////////////////////////////////////////////////////

private int put_pitch(int p)
{
    char result[8];
    static char *ptos[] = {
        "c", "cs", "d", "ef", "e", "f", "fs", "g",
        "gs", "a", "bf", "b"    };
    // note octave correction below 
    sprintf(result, "%s%d", ptos[p % 12], (p / 12) - 1);
    printf(result);
    return strlen(result);
}


/////////////////////////////////////////////////////////////////////////////
//               showbytes
// Effect: print hex data, precede with newline if asked
/////////////////////////////////////////////////////////////////////////////

char nib_to_hex[] = "0123456789ABCDEF";

private void showbytes(PmMessage data, int len, boolean newline)
{
    int count = 0;
    int i;

//    if (newline) {
//        putchar('\n');
//        count++;
//    } 
    for (i = 0; i < len; i++) {
        putchar(nib_to_hex[(data >> 4) & 0xF]);
        putchar(nib_to_hex[data & 0xF]);
        count += 2;
        if (count > 72) {
            putchar('.');
            putchar('.');
            putchar('.');
            break;
        }
        data >>= 8;
    }
    putchar(' ');
}



/////////////////////////////////////////////////////////////////////////////
//               showhelp
// Effect: print help text
/////////////////////////////////////////////////////////////////////////////

private void showhelp()
{
    printf("\n");
    printf("   Item Reported  Range     Item Reported  Range\n");
    printf("   -------------  -----     -------------  -----\n");
    printf("   Channels       1 - 16    Programs       1 - 128\n");
    printf("   Controllers    0 - 127   After Touch    0 - 127\n");
    printf("   Loudness       0 - 127   Pitch Bend     0 - 16383, center = 8192\n");
    printf("   Pitches        0 - 127, 60 = c4 = middle C\n");
    printf(" \n");
    printf("n toggles notes");
    showstatus(notes);
    printf("t displays noteon count since last t\n");
    printf("b toggles pitch bend, aftertouch");
    showstatus(bender);
    printf("c toggles continuous control");
    showstatus(controls);
    printf("h toggles program changes");
    showstatus(pgchanges);
    printf("x toggles system exclusive");
    showstatus(excldata);
    printf("k toggles clock and sense counting only");
    showstatus(clksencnt);
    printf("r toggles other real time messages & SMPTE");
    showstatus(realdata);
    printf("s displays clock and sense count since last k\n");
    printf("m toggles channel mode messages");
    showstatus(chmode);
    printf("v toggles verbose text");
    showstatus(verbose);
    printf("q quits\n");
    printf("\n");
}

/////////////////////////////////////////////////////////////////////////////
//               showstatus
// Effect: print status of flag
/////////////////////////////////////////////////////////////////////////////

private void showstatus(boolean flag)
{
    printf(", now %s\n", flag ? "ON" : "OFF" );
}
