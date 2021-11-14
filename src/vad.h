#ifndef _VAD_H
#define _VAD_H
#include <stdio.h>

/* TODO: add the needed states */
typedef enum {ST_UNDEF=0,ST_SILENCE, ST_VOICE,ST_INIT,ST_MAYBE_VOICE, ST_MAYBE_SILENCE}VAD_STATE; //ST_FIN ??

/* Return a string label associated to each state */
const char *state2str(VAD_STATE st);

/* TODO: add the variables needed to control the VAD 
   (counts, thresholds, etc.) */

typedef struct {
  VAD_STATE state;
  float sampling_rate;
  unsigned int frame_length;
  float last_feature; /* for debuggin purposes */
  //variables creades
  float p0,p1,p2;

  //comptadors
  unsigned int num_trames; // per comptar nivell de soroll de fons
  unsigned int num_trames_maybe_v; 
  unsigned int num_trames_maybe_s; 
  unsigned int num_trames_v; 
  unsigned int num_trames_s;
  unsigned int num_total_v; 
  unsigned int num_total_s;
  
  unsigned int trames_fons;
  //lindars
  int n;
  float alpha1, alpha2;
  int wl, wc; //paràmetres dels maybe

} VAD_DATA;

/* Call this function before using VAD: 
   It should return allocated and initialized values of vad_data

   sampling_rate: ... the sampling rate */
VAD_DATA *vad_open(float rate, float alpha1, float alpha2, int n, int wl, int wc);

/* vad works frame by frame.
   This function returns the frame size so that the program knows how
   many samples have to be provided */
unsigned int vad_frame_size(VAD_DATA *);

/* Main function. For each 'time', compute the new state 
   It returns:
    ST_UNDEF   (0) : undefined; it needs more frames to take decission
    ST_SILENCE (1) : silence
    ST_VOICE   (2) : voice

    x: input frame
       It is assumed the length is frame_length */
VAD_STATE vad(VAD_DATA *vad_data, float *x);

/* Free memory
   Returns the state of the last (undecided) states. */
VAD_STATE vad_close(VAD_DATA *vad_data);

/* Print actual state of vad, for debug purposes */
void vad_show_state(const VAD_DATA *, FILE *);

#endif
