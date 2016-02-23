//
//  midi_player.c
//  midi
//
//  Created by Alessandro on 14/02/16.
//  Copyright © 2016 Alessandro. All rights reserved.
//

#include "midi.h"
#include <AudioToolbox/AudioToolbox.h>

typedef struct scheduled_event_struct
{
    unsigned int at;
    midi_event *event;
}scheduled_event;

float calculate_bpm(unsigned int tempo, midi_time_signature time_signature)
{
    return (60000000.f / (float)tempo) * (powf(2,time_signature.denominator) / (float)time_signature.numerator);
}

float calculate_seconds_per_tick(midi_header *header, unsigned int tempo)
{
    return ((float)tempo / 1000000.f) / (float)header->division;
}

unsigned int find_closing_event(midi_event *event, midi_event_list *track)
{
    unsigned int event_duration = 0;
    
    while(true)
    {
        event_duration += track->event->delta;
        
        if(track->event->status_byte == MIDI_MTRK_MIDI_EVENT_NOTE_OFF)
        {
            if(track->event->channel == event->channel)
            {
                if(track->event->key_note == event->key_note)
                {
                    return event_duration;
                }
            }
        }
        
        if(track->event->status_byte == MIDI_MTRK_MIDI_EVENT_NOTE_ON)
        {
            if(track->event->velocity == 0)
            {
                if(track->event->channel == event->channel && track->event->key_note == event->key_note)
                {
                    return event_duration;
                }
            }
        }
        
        track = track->next;
    }
}

void addTempoToTrack(midi_header *header, midi_event_list *tempo_track, midi_event_list *track, MusicTrack music_track)
{
    unsigned int tempo = 500000;
    midi_time_signature time_signature = {4, 2, 0, 0};
    unsigned int delta_sum = 0;
    float bpm = calculate_bpm(tempo, time_signature);
    float seconds_per_tick = calculate_seconds_per_tick(header, tempo);

    
    int tempo_event_count = 0;
    midi_event_list *tempo_track_pointer = tempo_track;
    
    while(tempo_track_pointer)
    {
        if(tempo_track_pointer->event->status_byte == MIDI_MTRK_META_EVENT_TEMPO || tempo_track_pointer->event->status_byte == MIDI_MTRK_META_EVENT_TIME_SIGNATURE)
        {
            tempo_event_count++;

        }
        tempo_track_pointer = tempo_track_pointer->next;
    }
    
    scheduled_event scheduled_tempo[tempo_event_count];
    memset(scheduled_tempo, 0, sizeof(scheduled_event) * tempo_event_count);
    
    tempo_track_pointer = tempo_track;
    int index = 0;
    
    while(tempo_track_pointer)
    {
        delta_sum += tempo_track_pointer->event->delta;
        
        if(tempo_track_pointer->event->status_byte == MIDI_MTRK_META_EVENT_TEMPO)
        {
            tempo = tempo_track_pointer->event->tempo;
            bpm = calculate_bpm(tempo, time_signature);
            seconds_per_tick = calculate_seconds_per_tick(header, tempo);
            
            scheduled_tempo[index].at = delta_sum;//(delta_sum * seconds_per_tick) * (bpm / 60.f);
            scheduled_tempo[index++].event = tempo_track_pointer->event;
        }
        else if(tempo_track_pointer->event->status_byte == MIDI_MTRK_META_EVENT_TIME_SIGNATURE)
        {
            time_signature = tempo_track_pointer->event->time_signature;
            bpm = calculate_bpm(tempo, time_signature);
            
            scheduled_tempo[index].at = delta_sum;//(delta_sum * seconds_per_tick) * (bpm / 60.f);
            scheduled_tempo[index++].event = tempo_track_pointer->event;
        }

        tempo_track_pointer = tempo_track_pointer->next;
    }

    index = 0;
    
    while(scheduled_tempo[index].at == 0.0 && index < tempo_event_count)
    {
        if(scheduled_tempo[index].event->status_byte == MIDI_MTRK_META_EVENT_TEMPO)
        {
            tempo = scheduled_tempo[index].event->tempo;
            bpm = calculate_bpm(tempo, time_signature);
            seconds_per_tick = calculate_seconds_per_tick(header, tempo);
        }
        else
        {
            time_signature = scheduled_tempo[index].event->time_signature;
            bpm = calculate_bpm(tempo, time_signature);
        }
        
        index++;
    }
    
    delta_sum = 0;
    
    while(track->next)
    {
        delta_sum += track->event->delta;
        float now = (delta_sum * seconds_per_tick) * (bpm / 60.f);
        
        if(delta_sum > scheduled_tempo[index].at)
        {
            if(scheduled_tempo[index].event->status_byte == MIDI_MTRK_META_EVENT_TEMPO)
            {
                tempo = scheduled_tempo[index].event->tempo;
                bpm = calculate_bpm(tempo, time_signature);
                seconds_per_tick = calculate_seconds_per_tick(header, tempo);
            }
            else
            {
                time_signature = scheduled_tempo[index].event->time_signature;
                bpm = calculate_bpm(tempo, time_signature);
            }
            
            index++;
        }

        if(track->event->status_byte == MIDI_MTRK_MIDI_EVENT_NOTE_ON && track->event->velocity != 0)
        {
            unsigned int note_duration = find_closing_event(track->event, track->next);

            MIDINoteMessage *note = (MIDINoteMessage *)malloc(sizeof(MIDINoteMessage));
            note->channel = track->event->channel;
            note->velocity = track->event->velocity;
            note->note = track->event->key_note;
            note->duration = (note_duration * seconds_per_tick) * (bpm / 60.f);
            
            MusicTrackNewMIDINoteEvent(music_track, now, note);
            
            printf("delta: %f, channel: %u, note: %u, duration: %f\n", now, note->channel, note->note, note->duration);
        }
        
        track = track->next;
    }
}

void play_sequence(MusicSequence sequence)
{
    MusicPlayer player;
    NewMusicPlayer(&player);
    
    MusicPlayerSetSequence(player, sequence);
    MusicPlayerPreroll(player);
    
    uint track_count = 0;
    MusicSequenceGetTrackCount(sequence, &track_count);
    MusicTimeStamp length = 0.0;
    uint size = sizeof(length);
    
    for(int i = 0; i < track_count; i++)
    {
        MusicTimeStamp len;
        MusicTrack current_track;
        MusicSequenceGetIndTrack(sequence, i, &current_track);
        MusicTrackGetProperty(current_track, kSequenceTrackProperty_TrackLength, &len, &size);
        
        if(len > length)
        {
            length = len;
        }
    }
    
    printf("len: %f\n", length);
    
    for(int i = 0; i < track_count; i++)
    {
        MusicTrack track;
        MusicEventIterator track_iterator;
        MusicSequenceGetIndTrack(sequence, i, &track);
        NewMusicEventIterator(track, &track_iterator);
        unsigned char has_event = true;
        
        while(has_event)
        {
            MusicTimeStamp delta = 0.0;
            MusicEventType type = 0;
            void *data = NULL;
            uint dataSize = 0;
            
            MusicEventIteratorGetEventInfo(track_iterator, &delta, &type, (const void **)&data, &dataSize);
            
            if(type == kMusicEventType_ExtendedTempo)
            {
                MusicTimeStamp *tempo = (MusicTimeStamp *)data;
                printf("delta: %f tempo: %f\n",delta, *tempo);
            }
            else if (type == kMusicEventType_Meta)
            {
                MIDIMetaEvent *metaEvent = (MIDIMetaEvent *)data;
                printf("delta: %f meta: %u, data: %u, size: %u\n",delta, metaEvent->metaEventType, metaEvent->data[0], metaEvent->dataLength);
                
                for(int i = 0; i < metaEvent->dataLength; i++)
                {
                    printf("%u\n", *(((UInt8 *)data) + i + 8));
                }
            }
            else if (type == kMusicEventType_MIDINoteMessage)
            {
                MIDINoteMessage *noteMessage = (MIDINoteMessage *)data;
                printf("delta: %f, channel: %u, note: %u, duration: %f\n",delta, noteMessage->channel, noteMessage->note, noteMessage->duration);
            }
            
            MusicEventIteratorHasNextEvent(track_iterator, &has_event);
            if(has_event)
            {
                MusicEventIteratorNextEvent(track_iterator);
            }
        }
    }
    
    time_t start = time(NULL);
    MusicPlayerStart(player);
    
    while(true)
    {
        sleep(3);
        MusicTimeStamp now;
        MusicPlayerGetTime(player, &now);
        
        printf("time: %li\n", time(NULL) - start);
        
        if(now >= length)
        {
            MusicPlayerStop(player);
            break;
        }
    }
    
    for(int i = 0; i < track_count; i++)
    {
        MusicTrack t;
        MusicSequenceGetIndTrack(sequence, i, &t);
        MusicSequenceDisposeTrack(sequence, t);
    }
    
    DisposeMusicSequence(sequence);
    DisposeMusicPlayer(player);
}

void play_single_track(midi_header *header, midi_event_list *track, MusicSequence *sequence)
{
    unsigned int tempo = 500000;
    unsigned int delta_sum = 0;
    midi_time_signature time_signature = {4, 2, 0, 0};
    float bpm = calculate_bpm(tempo, time_signature);
    float seconds_per_tick = calculate_seconds_per_tick(header, tempo);
    
    MusicTrack tempo_track;
    MusicTrack midi_track;
    MusicSequenceGetTempoTrack(*sequence, &tempo_track);
    MusicSequenceNewTrack(*sequence, &midi_track);
    
    while(track->next)
    {
        delta_sum += track->event->delta;
        
        if(track->event->status_byte == MIDI_MTRK_META_EVENT_TEMPO)
        {
            MusicTimeStamp now = (delta_sum * seconds_per_tick) * (bpm / 60.f);
            tempo = track->event->tempo;
            bpm = calculate_bpm(tempo, time_signature);
            seconds_per_tick = calculate_seconds_per_tick(header, tempo);
            printf("bpm: %f\n", bpm);
            MusicTrackNewExtendedTempoEvent(tempo_track, now, bpm);
        }
        else if(track->event->status_byte == MIDI_MTRK_META_EVENT_TIME_SIGNATURE)
        {
            MusicTimeStamp now = (delta_sum * seconds_per_tick) * (bpm / 60.f);
            time_signature = track->event->time_signature;
            bpm = calculate_bpm(tempo, time_signature);
            
            
            MIDIMetaEvent *meta = (MIDIMetaEvent *)malloc(sizeof(MIDIMetaEvent) + 4);
            meta->metaEventType = 88;
            meta->dataLength = 4;
            memcpy(meta + 8, &track->event->time_signature, 4);
            printf("num: %u, den: %u\n", time_signature.numerator, time_signature.denominator);
            MusicTrackNewMetaEvent(tempo_track, now, meta);
            MusicTrackNewExtendedTempoEvent(tempo_track, now, bpm);
        }
        else if(track->event->status_byte == MIDI_MTRK_MIDI_EVENT_NOTE_ON)
        {
            if(track->event->velocity != 0)
            {
                midi_event_list *next_event = track->next;
                unsigned int note_duration = next_event->event->delta;
                
                while((next_event->event->status_byte == MIDI_MTRK_META_EVENT_TEMPO || next_event->event->status_byte == MIDI_MTRK_META_EVENT_TIME_SIGNATURE) || ((next_event->event->status_byte != MIDI_MTRK_MIDI_EVENT_NOTE_ON || next_event->event->status_byte != MIDI_MTRK_MIDI_EVENT_NOTE_OFF) && track->event->key_note != next_event->event->key_note && next_event->event->velocity != 0 && track->event->channel != next_event->event-> channel))
                {
                    next_event = next_event->next;
                    note_duration += next_event->event->delta;
                }
                
                MIDINoteMessage *note = (MIDINoteMessage *)malloc(sizeof(MIDINoteMessage));
                note->channel = track->event->channel;
                note->velocity = track->event->velocity;
                note->note = track->event->key_note;
                note->duration = (note_duration * seconds_per_tick) * (bpm / 60.f);
                
                MusicTrackNewMIDINoteEvent(midi_track, (delta_sum * seconds_per_tick) * (bpm / 60.f), note);
                
                printf("delta: %f, channel: %u, note: %u, duration: %f\n", (delta_sum * seconds_per_tick) * (bpm / 60.f), note->channel, note->note, note->duration);

            }
        }
        
        track = track->next;
    }
    
    play_sequence(*sequence);
}

void play_multi_track(midi_header *header, midi_event_list **tracks, MusicSequence music_sequence)
{
    unsigned int tempo = 500000;
    unsigned int delta_sum = 0;
    midi_time_signature time_signature = {4, 2, 0, 0};
    float bpm = calculate_bpm(tempo, time_signature);
    float seconds_per_tick = calculate_seconds_per_tick(header, tempo);
    
    midi_event_list *tempo_track = *tracks;
    MusicTrack music_tempo_track;
    MusicSequenceGetTempoTrack(music_sequence, &music_tempo_track);
    
    
    while(tempo_track)
    {
        delta_sum += tempo_track->event->delta;
        if(tempo_track->event->status_byte == MIDI_MTRK_META_EVENT_TEMPO)
        {
            tempo = tempo_track->event->tempo;
            bpm = calculate_bpm(tempo, time_signature);
            seconds_per_tick = calculate_seconds_per_tick(header, tempo);
            MusicTrackNewExtendedTempoEvent(music_tempo_track, (delta_sum * seconds_per_tick) * (bpm / 60.f), bpm);
            printf("%f tempo: %f\n", (delta_sum * seconds_per_tick) * (bpm / 60.f), bpm);
        }
        else if (tempo_track->event->status_byte == MIDI_MTRK_META_EVENT_TIME_SIGNATURE)
        {
            time_signature = tempo_track->event->time_signature;
            bpm = calculate_bpm(tempo, time_signature);
            
            MIDIMetaEvent *meta = (MIDIMetaEvent *)malloc(sizeof(MIDIMetaEvent) + 4);
            meta->metaEventType = 88;
            meta->dataLength = 4;
            meta->data[0] = time_signature.numerator;
            meta->data[1] = time_signature.denominator;
            meta->data[2] = time_signature.clocks_per_tick;
            meta->data[3] = time_signature.bb;
            printf("%f num: %u, den: %u\n", (delta_sum * seconds_per_tick) * (bpm / 60.f), time_signature.numerator, time_signature.denominator);
            MusicTrackNewMetaEvent(music_tempo_track, (delta_sum * seconds_per_tick) * (bpm / 60.f), meta);
        }
        
        tempo_track = tempo_track->next;
    }
    
    
    for(int i = 0; i < header->ntrks; i++)
    {
        MusicTrack music_track;
        MusicSequenceNewTrack(music_sequence, &music_track);
        
        addTempoToTrack(header, *tracks, tracks[i], music_track);
    }
    
    play_sequence(music_sequence);
}

void play_midi(midi_header *header, midi_event_list **sequence)
{
    MusicSequence midi_sequence;
    NewMusicSequence(&midi_sequence);
    
    if(header->format == MIDI_HEADER_FORMAT_SINGLE_TRACK)
    {
        //play_single_track(header, *sequence, midi_sequence);
    }
    else if (header->format == MIDI_HEADER_FORMAT_MULTI_TRACK)
    {
        play_multi_track(header, sequence, midi_sequence);
    }
}