//
//  midi_time.c
//  midi
//
//  Created by Alessandro on 14/02/16.
//  Copyright © 2016 Alessandro. All rights reserved.
//

#include "midi.h"
/*
float calculate_bpm(midi_time_signature time_signature, unsigned int tempo)
{
    return (60000000.f / (float)tempo) * (time_signature.denominator / time_signature.numerator);
}

float calculate_delta_seconds(unsigned int delta, unsigned int tempo, unsigned int ticks_per_qn)
{
    return (float)delta * (((float)tempo / 1000000) / (float)ticks_per_qn);
}

void calculate_time_single_track(midi_header *header, midi_event_list *track)
{
    unsigned int tempo = 500000;
    midi_time_signature time_signature = {4, 4, 0, 0};
    unsigned int delta_sum = 0;
    
    while(track->next)
    {
        delta_sum += track->event->delta;
        
        if(track->event->status_byte == MIDI_MTRK_META_EVENT_TEMPO)
        {
            tempo = track->event->tempo;
        }
        else if(track->event->status_byte == MIDI_MTRK_META_EVENT_TIME_SIGNATURE)
        {
            time_signature = track->event->time_signature;
        }
        else if (track->event->status_byte == MIDI_MTRK_MIDI_EVENT_NOTE_ON)
        {
            if(track->event->velocity != 0)
            {
                midi_event_list *next = track->next;
                unsigned int delta_duration = next->event->delta;
                
                
                while(next->event->status_byte != MIDI_MTRK_MIDI_EVENT_NOTE_ON && track->event->key_note != next->event->key_note && next->event->velocity != 0 && track->event->channel != next->event->channel)
                {
                    next = next->next;
                    delta_duration += next->event->delta;
                }
                
                printf("at: %f, duration: %f\n", (calculate_delta_seconds(delta_sum, tempo, header->division) * (calculate_bpm(time_signature, tempo) / 60.f)), (calculate_delta_seconds(delta_duration, tempo, header->division) * (calculate_bpm(time_signature, tempo) / 60.f)));
            }
        }
        
        track = track->next;
    }
}

void calculate_time(midi_header *header, midi_event_list **sequence)
{
    if(header->format == MIDI_HEADER_FORMAT_SINGLE_TRACK)
    {
        midi_event_list *track = *sequence;
        calculate_time_single_track(header, track);
    }
    else if (header->format == MIDI_HEADER_FORMAT_MULTI_TRACK)
    {
        
    }
    else
    {
        fprintf(stderr, "unused format.");
    }
}
*/