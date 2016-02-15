//
//  midi_player.c
//  midi
//
//  Created by Alessandro on 14/02/16.
//  Copyright © 2016 Alessandro. All rights reserved.
//

#include "midi.h"
#include <AudioToolbox/AudioToolbox.h>

float calculate_bpm(unsigned int tempo, midi_time_signature time_signature)
{
    return (60000000.f / (float)tempo) * (powf(2,time_signature.denominator) / (float)time_signature.numerator);
}

float calculate_seconds_per_tick(midi_header *header, unsigned int tempo)
{
    return ((float)tempo / 1000000.f) / (float)header->division;
}

void play_sequence(MusicSequence *sequence)
{
    MusicPlayer player;
    NewMusicPlayer(&player);
    
    MusicPlayerSetSequence(player, *sequence);
    MusicPlayerPreroll(player);
    
    uint track_count = 0;
    MusicSequenceGetTrackCount(*sequence, &track_count);
    MusicTimeStamp length = 0.0;
    uint size = sizeof(length);
    
    for(int i = 0; i < track_count; i++)
    {
        MusicTimeStamp len;
        MusicTrack current_track;
        MusicSequenceGetIndTrack(*sequence, i, &current_track);
        MusicTrackGetProperty(current_track, kSequenceTrackProperty_TrackLength, &len, &size);
        
        if(len > length)
        {
            length = len;
        }
    }
    
    printf("len: %f\n", length);
    /*
    for(int i = 0; i < track_count; i++)
    {
        MusicTrack track;
        MusicEventIterator track_iterator;
        MusicSequenceGetIndTrack(*sequence, i, &track);
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
    */
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
        MusicSequenceGetIndTrack(*sequence, i, &t);
        MusicSequenceDisposeTrack(*sequence, t);
    }
    
    DisposeMusicSequence(*sequence);
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
    
    play_sequence(sequence);
}

void play_midi(midi_header *header, midi_event_list **sequence)
{
    MusicSequence *midi_sequence = (MusicSequence *)malloc(sizeof(MusicSequence));
    NewMusicSequence(midi_sequence);
    MusicSequenceSetSequenceType(*midi_sequence, kMusicSequenceType_Beats);
    
    if(header->format == MIDI_HEADER_FORMAT_SINGLE_TRACK)
    {
        play_single_track(header, *sequence, midi_sequence);
    }
    else if (header->format == MIDI_HEADER_FORMAT_MULTI_TRACK)
    {
    }
}