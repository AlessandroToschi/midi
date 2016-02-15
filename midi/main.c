//
//  main.c
//  midi
//
//  Created by Alessandro on 28/01/16.
//  Copyright © 2016 Alessandro. All rights reserved.
//

#include <stdio.h>
#include "midi.h"
/*
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

struct midi_header_struct
{
    unsigned short format;
    unsigned short ntrks;
    unsigned short division;
};

struct midi_event_list_struct
{
    struct midi_event_struct *event;
    struct midi_event_list_struct *next;
};

struct midi_time_signature_struct
{
    unsigned char numerator;
    unsigned char denominator;
    unsigned char clocks_per_tick;
    unsigned char bb;
};

struct midi_event_struct
{
    unsigned int delta;
    unsigned char status_byte;
    struct midi_time_signature_struct time_signature;
    unsigned int tempo;
    unsigned char channel;
    unsigned char key_note;
    unsigned char velocity;
};

unsigned char running_status = 0;

unsigned int buffer_to_integer(unsigned char *buffer)
{
    // Assegno l'ultimo elemento del buffer.
    unsigned int data = buffer[3];
    data |= buffer[2] << 8; // Scorro di 8 bit e assegno il successivo.
    data |= buffer[1] << 16; // Scorro di 16 bit e assegno il successivo.
    data |= buffer[0] << 24; // Scorro di 24 bit e assegno il successivo.
    return data; // Ritorno l'elemento.
}

unsigned short buffert_to_shor(unsigned char *buffer)
{
    // Assegno l'ultimo elemento del buffer.
    unsigned short data = buffer[1];
    data |= buffer[0] << 8; // Scorro di 8 bit e assegno il successivo.
    return data; // Ritorno l'elemento.
}

unsigned int read_variable_length(FILE *midi_file)
{
    unsigned int value = 0; // Valore a lunghezza variabile.
    unsigned char byte = 0; // Byte letto.
    
    // Leggo il primo byte.
    fread(&byte, sizeof(unsigned char), 1, midi_file);
    value = byte; // Assegno il primo byte al valore.
    
    // Se il MSB è 1, continuo a leggere.
    if(byte & 0x80)
    {
        // Azzero l'ottavo bit del valore.
        value &= 0x7F;
        
        do
        {
            // Leggo il byte successivo.
            fread(&byte, sizeof(unsigned char), 1, midi_file);
            
            // Assegno il nuovo valore.
            value = (value << 7) + (byte & 0x7F);
        }while(byte & 0x80); // Finchè il MSB è 1.
    }
    
    // Ritorno il valore.
    return value;
}

void read_header(FILE *midi_file, struct midi_header_struct *midi_header)
{
    char chunk_type[5]; // Tipo di chunk, header.
    unsigned char short_buffer[2]; // Buffer per la lettura di interi a 16 bit.
    unsigned char integer_buffer[4]; // Buffer per la lettura di interi a 32 bit.
    unsigned int size = 0; // Dimensione dell'header.
    unsigned short format = 0; // Formato del file midi.
    unsigned short ntrks = 0; // Numero di tracce nel file midi.
    unsigned short division = 0; // Tipo di suddivisione temporale per i delta.
    
    // Mi posiziono all'inizio del file.
    rewind(midi_file);
    
    // Leggo i primi 4 byte del file, relativi al chunk type.
    fread(chunk_type, sizeof(char), 4, midi_file);
    chunk_type[4] = '\0'; // Terminatore per la stringa.
    printf("chunk type: %s\n", chunk_type);
    
    // Se i primi 4 byte (ASCII) sono diversi da 'MThd' non è un file midi.
    if(strcmp(chunk_type, MIDI_HEADER_CHUNK_TYPE))
    {
        fprintf(stderr, "Invalid chunk type for header: %s instead of 'MThd'\n", chunk_type);
        exit(-1);
    }
    
    // Leggo 4 byte del file, relativi alla lunghezza dell'header.
    fread(&integer_buffer, sizeof(unsigned char), 4, midi_file);
    size = buffer_to_integer(integer_buffer); // Converto il buffer a intero senza segno.
    
    // Se la lunghezza dell'header è diversa da 6 il file midi non è valido.
    if(size != MIDI_HEADER_SIZE)
    {
        fprintf(stderr, "Invalid size for header: %u instead of 6\n", size);
        exit(-1);
    }
    
    // Leggo 2 byte dal file, relativi al formato del file midi.
    fread(&short_buffer, sizeof(unsigned char), 2, midi_file);
    format = buffert_to_shor(short_buffer); // Converto il buffer a mezzo intero senza segno.
    
    // Stampo la descrizione human-readable del formato del file midi.
    switch(format)
    {
        case MIDI_HEADER_FORMAT_SINGLE_TRACK:
        {
            printf("midi format: single track\n");
            break;
        }
        case MIDI_HEADER_FORMAT_MULTI_TRACK:
        {
            printf("midi format: multi track\n");
            break;
        }
        case MIDI_HEADER_FORMAT_MULTI_TRACK_INDIPENDENT:
        {
            printf("midi format: multi track indipendent\n");
            break;
        }
        default:
        {
            // Se ottengo un formato diverso dai precedenti il file midi non è valido.
            fprintf(stderr, "Invalid format for header: %u\n", format);
            exit(-1);
        }
    }
    
    // Leggo 2 byte dal file, relativi al numero di tracce del file midi.
    fread(&short_buffer, sizeof(unsigned char), 2, midi_file);
    ntrks = buffert_to_shor(short_buffer); // Converto il buffer a mezzo intero senza segno.
    
    printf("midi ntrks: %u\n", ntrks);
    
    // Leggo 2 byte dal file, relativi al tipo di suddivisione temporale dei delta.
    fread(&short_buffer, sizeof(unsigned char), 2, midi_file);
    division = buffert_to_shor(short_buffer); // Converto il buffer a mezzo intero senza segno.
    
    // Stampo la descrizione human-readable del tipo di suddivisione temporale dei delta.
    if(division & 0x8000)
    {
        printf("midi header division smpte\n");
    }
    else
    {
        // Numero di tick per quarto di note.
        unsigned short ticks_per_qn = division & 0x7F;
        printf("midi header ticks per quarter-note: %u\n", ticks_per_qn);
    }
    
    // Riempo la struct midi_header_struct con i valori letti.
    midi_header->format = format;
    midi_header->ntrks = ntrks;
    midi_header->division = division;
}

void read_sysex_event(FILE *midi_file)
{
    // Evento sysex non implementato.
    printf("not implemented sysex_event; skipped\n");
    
    // Leggo la lungezza dell'evento.
    unsigned int size = read_variable_length(midi_file);
    fseek(midi_file, size, SEEK_CUR); // Salto all'evento successivo.
}

char *read_meta_text(FILE *midi_file, unsigned int length)
{
    // Array di caratteri.
    char *text = (char *)malloc(sizeof(char) * length + 1);
    fread(text, sizeof(char), length, midi_file); // Leggo la stringa nel file.
    text[length] = '\0'; // Terminatore per la stringa.
    return text; // Ritorno la stringa.
}

struct midi_event_struct *alloc_midi_event_struct()
{
    struct midi_event_struct *midi_event = (struct midi_event_struct *)malloc(sizeof(struct midi_event_struct));
    memset(midi_event, 0, sizeof(struct midi_event_struct));
    return midi_event;
}

void add_midi_event_to_list(struct midi_event_list_struct *midi_tracks_events, struct midi_event_struct *event)
{
    if(!midi_tracks_events->event)
    {
        midi_tracks_events->event = event;
    }
    else
    {
        struct midi_event_list_struct *new_event = (struct midi_event_list_struct *)malloc(sizeof(struct midi_event_list_struct));
        new_event->event = event;
        new_event->next = NULL;
        
        while(midi_tracks_events->next)
        {
            midi_tracks_events = midi_tracks_events->next;
        }
        
        midi_tracks_events->next = new_event;
    }
}

void free_midi_event_list(struct midi_event_list_struct *midi_tracks_events)
{
    while (midi_tracks_events->next)
    {
        struct midi_event_list_struct *next = midi_tracks_events->next;
        free(midi_tracks_events->event);
        free(midi_tracks_events);
        
        midi_tracks_events = next;
    }
}

void read_meta_event(FILE *midi_file, unsigned int delta, struct midi_event_list_struct *midi_track_events)
{
    unsigned char type = 0; // Meta event type.
    unsigned int length = 0; // Lunghezza dell'evento.
    
    fread(&type, sizeof(unsigned char), 1, midi_file); // Leggo il tipo di meta event.
    length = read_variable_length(midi_file); // Leggo la lunghezza dell'evento.
    
    switch (type)
    {
        // Eventi non implementati in quanto non servono.
        case MIDI_MTRK_META_EVENT_SEQUENCE_NUMBER:
        case MIDI_MTRK_META_EVENT_TEXT:
        case MIDI_MTRK_META_EVENT_COPYRIGHT:
        case MIDI_MTRK_META_EVENT_LYRIC:
        case MIDI_MTRK_META_EVENT_MARKER:
        case MIDI_MTRK_META_EVENT_CUE_POINT:
        case MIDI_MTRK_META_EVENT_CHANNEL:
        case MIDI_MTRK_META_EVENT_SMPTE_OFFSET:
        case MIDI_MTRK_META_EVENT_SEQUENCER_SPECIFIC:
        {
            // Salto al prossimo evento.
            fseek(midi_file, length, SEEK_CUR);
            break;
        }
        case MIDI_MTRK_META_EVENT_TRACK_NAME:
        {
#if DEBUG
            char *track_name = read_meta_text(midi_file, length);
            printf("delta: %u, track name: %s\n", delta, track_name);
            free(track_name);
#else
            fseek(midi_file, length, SEEK_CUR);
#endif
            break;
        }
        case MIDI_MTRK_META_EVENT_INSTRUMENT_NAME:
        {
#if DEBUG
            char *instrument_name = read_meta_text(midi_file, length);
            printf("delta: %u, track name: %s\n", delta, instrument_name);
            free(instrument_name);
#else
            fseek(midi_file, length, SEEK_CUR);
#endif
            break;
        }
        case MIDI_MTRK_META_EVENT_END_OF_TRACK:
        {
            struct midi_event_struct *end_of_track_event = alloc_midi_event_struct();
            end_of_track_event->delta = delta;
            end_of_track_event->status_byte = MIDI_MTRK_META_EVENT_END_OF_TRACK;
            
            add_midi_event_to_list(midi_track_events, end_of_track_event);
  
#if DEBUG
            printf("delta: %u, end of the track\n", delta);
#endif
            break;
        }
        case MIDI_MTRK_META_EVENT_TEMPO:
        {
            unsigned char tempo_buffer[3]; // Buffer per il tempo (24 bit).
            unsigned int tempo = 0; // Tempo della traccia in ms.
            fread(tempo_buffer, sizeof(unsigned char), 3, midi_file); // Leggo il tempo.
            tempo = tempo_buffer[2]; // Capovolgo i 3 byte.
            tempo |= tempo_buffer[1] << 8;
            tempo |= tempo_buffer[0] << 16;

            struct midi_event_struct *tempo_event = alloc_midi_event_struct();
            tempo_event->delta = delta;
            tempo_event->status_byte = MIDI_MTRK_META_EVENT_TEMPO;
            tempo_event->tempo = tempo;
            
            add_midi_event_to_list(midi_track_events, tempo_event);
#if DEBUG
            printf("delta: %u, tempo: %u\n", delta, tempo);
#endif
            
            break;
        }
        case MIDI_MTRK_META_EVENT_TIME_SIGNATURE:
        {
            unsigned char time_signature_buffer[4]; // Buffer per il time signature.
            fread(time_signature_buffer, sizeof(unsigned char), 4, midi_file); // Leggo il time signature.
            
            struct midi_event_struct *time_signature_event = alloc_midi_event_struct();
            time_signature_event->time_signature.numerator = time_signature_buffer[0];
            time_signature_event->time_signature.denominator = (unsigned char)powf(2.0, (float)time_signature_buffer[1]);
            time_signature_event->time_signature.clocks_per_tick = time_signature_buffer[2];
            time_signature_event->time_signature.bb = time_signature_buffer[3];
            
            add_midi_event_to_list(midi_track_events, time_signature_event);
            
#if DEBUG
            printf("delta: %u, numerator: %u, denominator: %u, clocks per tick: %u, bb: %u\n", delta, time_signature_buffer[0], time_signature_event->time_signature.denominator, time_signature_buffer[2], time_signature_buffer[3]);
#endif
            
            break;
        }
        case MIDI_MTRK_META_EVENT_KEY_SIGNATURE:
        {
            printf("delta: %u ", delta);
            unsigned char key_signature_buffer[2]; // Chiave del file midi.
            fread(key_signature_buffer, sizeof(unsigned char), 2, midi_file);
            printf("sf: %u\n", key_signature_buffer[0]); // Scala DO.
            printf("mi: %u\n", key_signature_buffer[1]); // Scala MI.
            break;
        }
        default:
        {
            // Evento non riconosciuto o valido.
            printf("delta: %u ", delta);
            fprintf(stderr, "Unrecognized meta event code: %u, length: %u\n", type, length);
            fseek(midi_file, length, SEEK_CUR);
            break;
        }
    }
}

bool is_new_midi_event(unsigned char byte)
{
    // Midi event type.
    unsigned char type = (byte >> 4) & 0x0F;
    
    // Confronto se è fra i tipi validi di midi event.
    for(unsigned char midi_event = 0x8; midi_event <= 0xE; midi_event += 0x1)
    {
        // Se trovo una corrispondenza ritorno true.
        if(type == midi_event)
        {
            return true;
        }
    }
    return false; // Non è stata trovata una corrispondenza.
}

void read_midi_event(FILE *midi_file, char event, unsigned int delta, struct midi_event_list_struct *midi_track_events)
{
    unsigned char channel = event & 0x0F; // Canale.
    unsigned char type = event >> 4 & 0x0F; // Tipo di midi event.
    
    if(type == MIDI_MTRK_MIDI_EVENT_NOTE_OFF)
    {
        unsigned char key_note = 0;
        unsigned char velocity = 0;
        fread(&key_note, sizeof(unsigned char), 1, midi_file);
        fread(&velocity, sizeof(unsigned char), 1, midi_file);
        
        struct midi_event_struct *note_off_event = alloc_midi_event_struct();
        note_off_event->delta = delta;
        note_off_event->status_byte = MIDI_MTRK_MIDI_EVENT_NOTE_OFF;
        note_off_event->key_note = key_note;
        note_off_event->velocity = velocity;
        note_off_event->channel = channel;
        
        add_midi_event_to_list(midi_track_events, note_off_event);

#if DEBUG
        printf("delta: %u, midi event: note off; key note: %u, velocity: %u, channel: %u\n", delta, key_note, velocity, channel);
#endif
    }
    else if(type == MIDI_MTRK_MIDI_EVENT_NOTE_ON || type == MIDI_MTRK_MIDI_EVENT_POLYPHONIC_KEY_PRESSURE)
    {
        printf("delta: %u ", delta);
        char key_note = 0;
        char velocity = 0;
        fread(&key_note, sizeof(char), 1, midi_file);
        fread(&velocity, sizeof(char), 1, midi_file);
        
        struct midi_event_struct *note_on_event = alloc_midi_event_struct();
        note_on_event->delta = delta;
        note_on_event->status_byte = MIDI_MTRK_MIDI_EVENT_NOTE_ON;
        note_on_event->key_note = key_note;
        note_on_event->velocity = velocity;
        note_on_event->channel = channel;
        
        add_midi_event_to_list(midi_track_events, note_on_event);
        
        printf("midi event: note on or polyphinic key pressure; key note: %d, velocity: %d, channel: %d\n", key_note, velocity, channel);
    }
    else if(type == MIDI_MTRK_MIDI_EVENT_CONTROL_CHANGE || type == MIDI_MTRK_MIDI_EVENT_PITCH_WHEEL_CHANGE)
    {
        
        fseek(midi_file, 2, SEEK_CUR);
    }
    else if(type == MIDI_MTRK_MIDI_EVENT_PROGRAM_CHANGE || type == MIDI_MTRK_MIDI_EVENT_CHANNEL_PRESSURE)
    {
        if(type == MIDI_MTRK_MIDI_EVENT_PROGRAM_CHANGE)
        {
            unsigned char instruments = 0;
            fread(&instruments, sizeof(unsigned char), 1, midi_file);
            printf("instruments %u on channel %u\n", instruments, channel);
        }
        else{
        
        //unsigned char c = 0;
        //fread(midi_file, sizeof(unsigned char), 1, midi_file);
        //printf("program change: %u\n", c);
        fseek(midi_file, 1, SEEK_CUR);
        }
    }
    else
    {
        fprintf(stderr, "Unrecognized midi event: %u\n", event);
    }
}

void read_track(FILE *midi_file, struct midi_event_list_struct *midi_track_events)
{
    char chunk_type[5];
    unsigned char integer_buffer[4];
    unsigned int size;
    long current_offset = 0;
    
    fread(chunk_type, sizeof(char), 4, midi_file);
    chunk_type[4] = '\0';
    printf("chunk type: %s\n", chunk_type);
    
    if(strcmp(chunk_type, MIDI_MTRK_CHUNK_TYPE))
    {
        fprintf(stderr, "Invalid chunk type for track: %s instead of 'MTrk\n", chunk_type);
        exit(-1);
    }
    
    fread(integer_buffer, sizeof(unsigned char), 4, midi_file);
    size = buffer_to_integer(integer_buffer);
    printf("midi track size: %u\n", size);
    
    current_offset = ftell(midi_file);
 
    while (ftell(midi_file) < current_offset + size)
    {
        unsigned int delta = 0;
        unsigned char event = 0;
        
        delta = read_variable_length(midi_file);
        fread(&event, sizeof(unsigned char), 1, midi_file);
        
        switch (event)
        {
            case MIDI_MTRK_SYSEX_EVENT:
            {
                read_sysex_event(midi_file);
                break;
            }
            case MIDI_MTRK_META_EVENT:
            {
                read_meta_event(midi_file, delta, midi_track_events);
                
                break;
            }
            default:
            {
                if(!is_new_midi_event(event))
                {
                    event = running_status;
                    fseek(midi_file, -1, SEEK_CUR);
                }
                else
                {
                    running_status = event;
                }
                
                read_midi_event(midi_file, event, delta, midi_track_events);
                
                break;
            }
        }
    }
}

void play_track(struct midi_header_struct midi_header, struct midi_event_list_struct * midi_track_events)
{
    unsigned int tempo = 0;
    float seconds_per_quarter_note = 0.0;
    float seconds_per_tick = 0.0;
    time_t start = time(NULL);
    
    while (midi_track_events->event->status_byte != MIDI_MTRK_META_EVENT_END_OF_TRACK)
    {
        if(midi_track_events->event->delta != 0)
        {
            float delta_time_in_seconds = (float)midi_track_events->event->delta * seconds_per_tick;
            long delta_time_in_nanoseconds = (long)((delta_time_in_seconds - (int)delta_time_in_seconds) * 1E9);
            
            struct timespec delta;
            delta.tv_sec = (long)(delta_time_in_seconds - (int)delta_time_in_seconds);
            delta.tv_nsec = delta_time_in_nanoseconds;
            
            if(nanosleep(&delta, NULL))
            {
                fprintf(stderr, "error on timer, errno: %i\n", errno);
            }
        }
        
        if(midi_track_events->event->status_byte == MIDI_MTRK_META_EVENT_TEMPO)
        {
            tempo = midi_track_events->event->tempo;
            seconds_per_quarter_note = (float)tempo / 1000000.0f;
            seconds_per_tick = seconds_per_quarter_note / (float)(midi_header.division & 0x7F);
        }
        else if (midi_track_events->event->status_byte == MIDI_MTRK_MIDI_EVENT_NOTE_ON)
        {
            printf("playing note: %u\n", midi_track_events->event->key_note);
        }
        else if (midi_track_events->event->status_byte == MIDI_MTRK_MIDI_EVENT_NOTE_OFF)
        {
            
        }
        
        midi_track_events = midi_track_events->next;
    }
    
    time_t end = time(NULL);
    printf("total time: %li\n", end - start);
}

void play_single_track(struct midi_header_struct midi_header, struct midi_event_list_struct * midi_track_events, char channel_preferred)
{
    unsigned int tempo = 0;
    float seconds_per_quarter_note = 0.0;
    float seconds_per_tick = 0.0;
    time_t start = time(NULL);
    bool play_only_channel_preferred = false;
    
    if(channel_preferred >= 0 && channel_preferred <= 127)
    {
        play_only_channel_preferred = true;
    }
    
    while (midi_track_events->event->status_byte != MIDI_MTRK_META_EVENT_END_OF_TRACK)
    {
        if(midi_track_events->event->delta != 0)
        {
            float delta_time_in_seconds = (float)midi_track_events->event->delta * seconds_per_tick;
            long delta_time_in_nanoseconds = (long)((delta_time_in_seconds - (int)delta_time_in_seconds) * 1E9);
            
            struct timespec delta;
            delta.tv_sec = (long)(delta_time_in_seconds - (int)delta_time_in_seconds);
            delta.tv_nsec = delta_time_in_nanoseconds;
            
            if(nanosleep(&delta, NULL))
            {
                fprintf(stderr, "error on timer, errno: %i\n", errno);
            }
        }
        
        if(midi_track_events->event->status_byte == MIDI_MTRK_META_EVENT_TEMPO)
        {
            tempo = midi_track_events->event->tempo;
            seconds_per_quarter_note = (float)tempo / 1000000.0f;
            seconds_per_tick = seconds_per_quarter_note / (float)(midi_header.division & 0x7F);
        }
        else if (midi_track_events->event->status_byte == MIDI_MTRK_MIDI_EVENT_NOTE_ON)
        {
            if(play_only_channel_preferred && midi_track_events->event->channel == channel_preferred)
            {
                printf("playing note: %u\n", midi_track_events->event->key_note);
            }
        }
        else if (midi_track_events->event->status_byte == MIDI_MTRK_MIDI_EVENT_NOTE_OFF)
        {
            
        }
        
        midi_track_events = midi_track_events->next;
    }
    
    time_t end = time(NULL);
    printf("total time: %li\n", end - start);
}

void play_multi_track(struct midi_header_struct header, struct midi_event_list_struct *tempo_track, struct midi_event_list_struct *track)
{
    unsigned int tempo = 0;
    float seconds_per_quarter_note = 0.0;
    float seconds_per_tick = 0.0;
    time_t start = time(NULL);
    
    while(tempo_track->event->status_byte != MIDI_MTRK_META_EVENT_END_OF_TRACK)
    {
        if(tempo_track->event->status_byte == MIDI_MTRK_META_EVENT_TEMPO)
        {
            tempo = tempo_track->event->tempo;
            seconds_per_quarter_note = (float)tempo / 1000000.0f;
            seconds_per_tick = seconds_per_quarter_note / (float)(header.division & 0x7F);
        }
        
        tempo_track = tempo_track->next;
    }
    
    while(track->event->status_byte != MIDI_MTRK_META_EVENT_END_OF_TRACK)
    {
        if(track->event->delta != 0)
        {
            float delta_time_in_seconds = (float)track->event->delta * seconds_per_tick;
            long delta_time_in_nanoseconds = (long)((delta_time_in_seconds - (int)delta_time_in_seconds) * 1E9);
            
            struct timespec delta;
            delta.tv_sec = (long)(delta_time_in_seconds - (int)delta_time_in_seconds);
            delta.tv_nsec = delta_time_in_nanoseconds;
            
            if(nanosleep(&delta, NULL))
            {
                fprintf(stderr, "error on timer, errno: %i\n", errno);
            }
        }
        
        if(track->event->status_byte == MIDI_MTRK_MIDI_EVENT_NOTE_ON)
        {
            printf("playing note: %u at time: %li\n", track->event->key_note, time(NULL) - start);
            printf("%c", 7);
        }
        
        track = track->next;
    }
}
*/
int main(int argc, const char * argv[])
{
    for(int i = 0; i < argc; i++)
    {
        printf("arg %d: %s\n", i, argv[i]);
    }

    FILE *midi_file = fopen("./imperial.mid", "r");
    
    if(!midi_file)
    {
        fprintf(stderr, "Failed to open file.\n");
        exit(-1);
    }
    
    midi_header *header = read_header(midi_file);
    midi_event_list *sequence[header->ntrks];
    
    for(int i = 0; i < header->ntrks; i++)
    {
        sequence[i] = read_track(midi_file);
    }
    
    
    
    play_midi(header, sequence);
    
    for(int i = 0; i < header->ntrks; i++)
    {
        free_midi_event_list(sequence[i]);
    }
    
    free(header);
    fclose(midi_file);
    
    return 0;
}
