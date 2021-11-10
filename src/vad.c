#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "vad.h"
#include  "pav_analysis.h"

const float FRAME_TIME = 10.0F; /* in ms. */

float power = 0;

/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct {
  float zcr;
  float p;
  float am;
} Features;

/* 
 * TODO: Delete and use your own features!
 */

Features compute_features(const float *x, int N) {
  
  Features feat;
  feat.p = compute_power(x,N);
  //feat.zcr = feat.am = (float) rand()/RAND_MAX;
  return feat;
}

/* 
 * TODO: Init the values of vad_data
 */

VAD_DATA * vad_open(float rate,float alpha1, float alpha2, int total_trames) { //inicialitza variables
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;

  vad_data->num_trames = 0;
  vad_data->num_trames_maybe_v = 0;
  vad_data->num_trames_maybe_s = 0;



  vad_data->alpha1 = alpha1;
  vad_data->alpha2 = alpha2;
  vad_data->total_trames=total_trames;
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
  /* 
   * TODO: decide what to do with the last undecided frames
   */
  VAD_STATE state = vad_data->state;

  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * TODO: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x) {

  /* 
   * TODO: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  Features f = compute_features(x, vad_data->frame_length);
  vad_data->last_feature = f.p; /* save feature, in case you want to show */

  switch (vad_data->state) {
  case ST_INIT: //s'executen les ordres fins arribar al break
    if(vad_data->num_trames <vad_data->total_trames){ //calcular soroll de fons
      power = power + pow(10,f.p/10);
      vad_data->state = ST_INIT;
    }else{
      
    vad_data->p0 = 10*log10(power/vad_data->num_trames);
    printf("%f\n",vad_data->p0);
    vad_data-> p1 = vad_data->p0 + 0.3;//vad_data->alpha1; //definimos valor umbral 1
    vad_data-> p2 = vad_data->p1 + 0.6;//vad_data->alpha2; //definimos valor umbral 2

    vad_data->state = ST_SILENCE;
    vad_data->num_trames = 0;
   
    } 
    break;

  case ST_SILENCE:
    printf("%f\n",f.p);
    if (f.p > vad_data->p1)
      vad_data->state = ST_MAYBE_VOICE;
    else
       vad_data->state = ST_SILENCE;
    
    break;

  case ST_VOICE:
    if (f.p < vad_data->p2)
      vad_data->state = ST_MAYBE_SILENCE;
    else
      vad_data->state = ST_VOICE;
    break;

    case ST_MAYBE_VOICE: //per sortir s'ha de complir condició temporal
      //printf("Entro v\n");
      if (vad_data->num_trames_maybe_v < 3){ //5 representa la ventana de análisis del maybe
          if(f.p < vad_data->p1){
            vad_data->state = ST_SILENCE;
            vad_data->num_trames_maybe_v=0;
          } else if (f.p > vad_data->p2 && vad_data->num_trames_maybe_v >= 1){ //3 representa el núm de tramas necesarias para confirmar voice
             vad_data->state = ST_VOICE;
             vad_data->num_trames_maybe_v=0;
          } else {
            vad_data->state = ST_MAYBE_VOICE;
          }
      }
    break;

    case ST_MAYBE_SILENCE: //per sortir s'ha de complir condició temporal
     //printf("Entro s\n");
      if (vad_data->num_trames_maybe_s < 3){ //5 representa la ventana de análisis del maybe
          if(f.p > vad_data->p2){
            vad_data->state = ST_VOICE;
            vad_data->num_trames_maybe_s=0;
          } else if (f.p < vad_data->p1 && vad_data->num_trames_maybe_s >= 1){ //3 representa el núm de tramas necesarias para confirmar silenci (idem valor veu)
             vad_data->state = ST_SILENCE;
             vad_data->num_trames_maybe_s=0;
          } else {
            vad_data->state = ST_MAYBE_SILENCE;
          }
      }
    break;    

  case ST_UNDEF:
    break;
  }

  if (vad_data->state == ST_SILENCE ||
      vad_data->state == ST_VOICE)
    return vad_data->state;
  else
    return ST_UNDEF;
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}