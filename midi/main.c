//
//  main.c
//  midi
//
//  Created by Alessandro on 28/01/16.
//  Copyright © 2016 Alessandro. All rights reserved.
//

#include <stdio.h>
#include "midi.h"

int main(int argc, const char * argv[])
{
    for(int i = 0; i < argc; i++)
    {
        printf("arg %d: %s\n", i, argv[i]);
    }

    FILE *midi_file = fopen("./star_wars3.mid", "r");
    
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
