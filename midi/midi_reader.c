//
//  midi_reader.c
//  midi
//
//  Created by Alessandro on 13/02/16.
//  Copyright © 2016 Alessandro. All rights reserved.
//

#include "midi.h"

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

midi_header *read_header(FILE *midi_file)
{
    char chunk_type[5]; // Tipo di chunk, header.
    unsigned char short_buffer[2]; // Buffer per la lettura di interi a 16 bit.
    unsigned char integer_buffer[4]; // Buffer per la lettura di interi a 32 bit.
    unsigned int size = 0; // Dimensione dell'header.
    unsigned short format = 0; // Formato del file midi.
    unsigned short ntrks = 0; // Numero di tracce nel file midi.
    unsigned short division = 0; // Tipo di suddivisione temporale per i delta.
    midi_header *header = (midi_header *)malloc(sizeof(midi_header));
    memset(header, 0, sizeof(midi_header));
    
    // Mi posiziono all'inizio del file.
    rewind(midi_file);
    
    // Leggo i primi 4 byte del file, relativi al chunk type.
    fread(chunk_type, sizeof(char), 4, midi_file);
    chunk_type[4] = '\0'; // Terminatore per la stringa.
    //printf("chunk type: %s\n", chunk_type);
    
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
            //printf("midi format: single track\n");
            break;
        }
        case MIDI_HEADER_FORMAT_MULTI_TRACK:
        {
            //printf("midi format: multi track\n");
            break;
        }
        case MIDI_HEADER_FORMAT_MULTI_TRACK_INDIPENDENT:
        {
            //printf("midi format: multi track indipendent\n");
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
    
    // Riempo la struct midi_header_struct con i valori letti.
    header->format = format;
    header->ntrks = ntrks;
    header->division = division;
    return header;
}

void read_sysex_event(FILE *midi_file)
{
    // Evento sysex non implementato.
    printf("not implemented sysex_event; skipped\n");
    
    // Leggo la lungezza dell'evento.
    unsigned int size = read_variable_length(midi_file);
    fseek(midi_file, size, SEEK_CUR); // Salto all'evento successivo.
}

void read_meta_event(FILE *midi_file, unsigned int delta, midi_event_list *track)
{
    unsigned char type = 0; // Meta event type.
    unsigned int length = 0; // Lunghezza dell'evento.
    
    fread(&type, sizeof(unsigned char), 1, midi_file); // Leggo il tipo di meta event.
    length = read_variable_length(midi_file); // Leggo la lunghezza dell'evento.
    
    if(type == MIDI_MTRK_META_EVENT_TEMPO)
    {
        unsigned char tempo_buffer[3]; // Buffer per il tempo (24 bit).
        unsigned int tempo = 0; // Tempo della traccia in ms.
        fread(tempo_buffer, sizeof(unsigned char), 3, midi_file); // Leggo il tempo.
        tempo = tempo_buffer[2]; // Capovolgo i 3 byte.
        tempo |= tempo_buffer[1] << 8;
        tempo |= tempo_buffer[0] << 16;
        
        midi_event *tempo_event = alloc_midi_event();
        tempo_event->status_byte = MIDI_MTRK_META_EVENT_TEMPO;
        tempo_event->delta = delta;
        tempo_event->tempo = tempo;
        
        add_midi_event_list(track, tempo_event);
        
        //printf("delta: %u tempo: %u\n",delta, tempo);
    }
    else if (type == MIDI_MTRK_META_EVENT_TIME_SIGNATURE)
    {
        unsigned char time_signature_buffer[4]; // Buffer per il time signature.
        fread(time_signature_buffer, sizeof(unsigned char), 4, midi_file); // Leggo il time signature.
        
        midi_event *time_signature_event = alloc_midi_event();
        time_signature_event->status_byte = MIDI_MTRK_META_EVENT_TIME_SIGNATURE;
        time_signature_event->delta = delta;
        time_signature_event->time_signature.numerator = time_signature_buffer[0];
        time_signature_event->time_signature.denominator = time_signature_buffer[1];
        time_signature_event->time_signature.clocks_per_tick = time_signature_buffer[2];
        time_signature_event->time_signature.bb = time_signature_buffer[3];
        
        add_midi_event_list(track, time_signature_event);
        
        //printf("delta: %u num: %u, den: %u\n",delta, time_signature_event->time_signature.numerator, time_signature_event->time_signature.denominator);
    }
    else
    {
        midi_event *meta_event = alloc_midi_event();
        meta_event->delta = delta;
        
        add_midi_event_list(track, meta_event);
        
        // Gli altri meta event non interessano saltiamo al successivo evento.
        fseek(midi_file, length, SEEK_CUR);
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
        
        struct midi_event_struct *note_off_event = alloc_midi_event();
        note_off_event->delta = delta;
        note_off_event->status_byte = MIDI_MTRK_MIDI_EVENT_NOTE_OFF;
        note_off_event->key_note = key_note;
        note_off_event->velocity = velocity;
        note_off_event->channel = channel;
        
        add_midi_event_list(midi_track_events, note_off_event);
        
        //printf("delta: %u, midi event: note off; key note: %u, velocity: %u, channel: %u\n", delta, key_note, velocity, channel);
    }
    else if(type == MIDI_MTRK_MIDI_EVENT_NOTE_ON || type == MIDI_MTRK_MIDI_EVENT_POLYPHONIC_KEY_PRESSURE)
    {
        char key_note = 0;
        char velocity = 0;
        fread(&key_note, sizeof(char), 1, midi_file);
        fread(&velocity, sizeof(char), 1, midi_file);
        
        struct midi_event_struct *note_on_event = alloc_midi_event();
        note_on_event->delta = delta;
        note_on_event->status_byte = MIDI_MTRK_MIDI_EVENT_NOTE_ON;
        note_on_event->key_note = key_note;
        note_on_event->velocity = velocity;
        note_on_event->channel = channel;
        
        add_midi_event_list(midi_track_events, note_on_event);
        
        //printf("delta: %u, midi note on; key note: %d, velocity: %d, channel: %d\n", delta, key_note, velocity, channel);
    }
    else if(type == MIDI_MTRK_MIDI_EVENT_CONTROL_CHANGE || type == MIDI_MTRK_MIDI_EVENT_PITCH_WHEEL_CHANGE)
    {
        midi_event *mid_event = alloc_midi_event();
        mid_event->delta = delta;
        
        add_midi_event_list(midi_track_events, mid_event);
        
        fseek(midi_file, 2, SEEK_CUR);
    }
    else if(type == MIDI_MTRK_MIDI_EVENT_PROGRAM_CHANGE || type == MIDI_MTRK_MIDI_EVENT_CHANNEL_PRESSURE)
    {
        midi_event *mid_event = alloc_midi_event();
        mid_event->delta = delta;
        
        add_midi_event_list(midi_track_events, mid_event);
        
        fseek(midi_file, 1, SEEK_CUR);
    }
    else
    {
        fprintf(stderr, "Unrecognized midi event: %u\n", event);
    }
}

void add_midi_event_list(midi_event_list *midi_events, midi_event *event)
{
    if(!midi_events->event)
    {
        midi_events->event = event;
    }
    else
    {
        midi_event_list *new_event = (midi_event_list *)malloc(sizeof(midi_event_list));
        new_event->event = event;
        new_event->next = NULL;
        
        while(midi_events->next)
        {
            midi_events = midi_events->next;
        }
        
        midi_events->next = new_event;
    }
}

void free_midi_event_list(midi_event_list *midi_events)
{
    while (midi_events->next)
    {
        midi_event_list *next = midi_events->next;
        free(midi_events->event);
        free(midi_events);
        
        midi_events = next;
    }
}

midi_event *alloc_midi_event()
{
    midi_event *event = (midi_event *)malloc(sizeof(midi_event));
    memset(event, 0, sizeof(midi_event));
    return event;
}

midi_event_list *read_track(FILE *midi_file)
{
    char chunk_type[5];
    unsigned char integer_buffer[4];
    unsigned int size;
    long current_offset = 0;
    midi_event_list *track_events = (midi_event_list *)malloc(sizeof(midi_event_list));
    memset(track_events, 0, sizeof(midi_event_list));
    
    running_status = 0;
    
    fread(chunk_type, sizeof(char), 4, midi_file);
    chunk_type[4] = '\0';
    //printf("chunk type: %s\n", chunk_type);
    
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
                read_meta_event(midi_file, delta, track_events);
                
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
                
                read_midi_event(midi_file, event, delta, track_events);
                
                break;
            }
        }
    }
    return track_events;
}