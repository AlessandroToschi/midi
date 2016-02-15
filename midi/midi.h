//
//  midi.h
//  midi
//
//  Created by Alessandro on 11/02/16.
//  Copyright © 2016 Alessandro. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <errno.h>

#define MIDI_HEADER_CHUNK_TYPE "MThd"
#define MIDI_HEADER_SIZE 6

#define MIDI_HEADER_FORMAT_SINGLE_TRACK 0
#define MIDI_HEADER_FORMAT_MULTI_TRACK 1
#define MIDI_HEADER_FORMAT_MULTI_TRACK_INDIPENDENT 2

#define MIDI_MTRK_CHUNK_TYPE "MTrk"

#define MIDI_MTRK_SYSEX_EVENT 0xF0
#define MIDI_MTRK_META_EVENT 0xFF

#define MIDI_MTRK_META_EVENT_SEQUENCE_NUMBER 0
#define MIDI_MTRK_META_EVENT_TEXT 1
#define MIDI_MTRK_META_EVENT_COPYRIGHT 2
#define MIDI_MTRK_META_EVENT_TRACK_NAME 3
#define MIDI_MTRK_META_EVENT_INSTRUMENT_NAME 4
#define MIDI_MTRK_META_EVENT_LYRIC 5
#define MIDI_MTRK_META_EVENT_MARKER 6
#define MIDI_MTRK_META_EVENT_CUE_POINT 7
#define MIDI_MTRK_META_EVENT_CHANNEL 0x20
#define MIDI_MTRK_META_EVENT_END_OF_TRACK 0x2F
#define MIDI_MTRK_META_EVENT_TEMPO 0x51
#define MIDI_MTRK_META_EVENT_SMPTE_OFFSET 0x54
#define MIDI_MTRK_META_EVENT_TIME_SIGNATURE 0x58
#define MIDI_MTRK_META_EVENT_KEY_SIGNATURE 0x59
#define MIDI_MTRK_META_EVENT_SEQUENCER_SPECIFIC 0x7F

#define MIDI_MTRK_MIDI_EVENT_NOTE_OFF 0x8
#define MIDI_MTRK_MIDI_EVENT_NOTE_ON 0x9
#define MIDI_MTRK_MIDI_EVENT_POLYPHONIC_KEY_PRESSURE 0xA
#define MIDI_MTRK_MIDI_EVENT_CONTROL_CHANGE 0xB
#define MIDI_MTRK_MIDI_EVENT_PROGRAM_CHANGE 0xC
#define MIDI_MTRK_MIDI_EVENT_CHANNEL_PRESSURE 0xD
#define MIDI_MTRK_MIDI_EVENT_PITCH_WHEEL_CHANGE 0xE

typedef struct midi_header_struct
{
    unsigned short format;
    unsigned short ntrks;
    unsigned short division;
}midi_header;

typedef struct midi_time_signature_struct
{
    unsigned char numerator;
    unsigned char denominator;
    unsigned char clocks_per_tick;
    unsigned char bb;
}midi_time_signature;

typedef struct midi_event_struct
{
    unsigned int delta;
    unsigned char status_byte;
    unsigned int tempo;
    unsigned char channel;
    unsigned char key_note;
    unsigned char velocity;
    midi_time_signature time_signature;
}midi_event;

typedef struct midi_event_list_struct
{
    midi_event *event;
    struct midi_event_list_struct *next;
}midi_event_list;

midi_event *alloc_midi_event();
void add_midi_event_list(midi_event_list *midi_events, midi_event *event);
void free_midi_event_list(midi_event_list *midi_events);

midi_header *read_header(FILE *midi_file);
midi_event_list *read_track(FILE *midi_file);

void calculate_time(midi_header *header, midi_event_list **sequence);
void play_midi(midi_header *header, midi_event_list **sequence);