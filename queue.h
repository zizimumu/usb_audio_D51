#ifndef __QUEUE_H___
#define __QUEUE_H___

void init_playbackQueue(void);
void add_playbackQueue(unsigned char value);
int queue_playbackEmpty();
unsigned char get_playbackQueue(void);

void init_queue(void);
void add_queue(unsigned char value);
int queue_empty();
unsigned char get_queue(void);

#endif