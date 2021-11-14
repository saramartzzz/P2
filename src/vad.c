#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "vad.h"
#include  "pav_analysis.h"

const float FRAME_TIME = 10.0F; /* in ms. */

float power = 0;  // inicilitzem comptador de la potència


const char *state_str[] = {
  "UNDEF", "S", "V", "INIT", "MV", "MS"
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

Features compute_features(const float *x, int N) {
  
  Features feat;
  feat.p = compute_power(x,N);
  feat.zcr = compute_zcr(x,N, 16000);
  
  //SILENCIO -> ZCR alto
  //VOZ -> ZCR bajo

  return feat;
}



VAD_DATA * vad_open(float rate, float alpha1, float alpha2, int n, int wl, int wc, int zcr1, int zcr2, int wlv) { //inicialitza variables
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3; 

  //inicilitzaem comptadors
  vad_data->num_trames = 0;
  vad_data->trames_fons = 1; // per tal de tenir en compte la primera trama
  vad_data->num_trames_maybe_v = 0;
  vad_data->num_trames_maybe_s = 0;
  vad_data->num_trames_v = 0;
  vad_data->num_trames_s = 0;
  vad_data->num_total_s = 0;
  vad_data->num_total_v = 0;

  //inicialitzem llindars
  vad_data->alpha1 = alpha1;
  vad_data->alpha2 = alpha2;
  vad_data->n = n;
  vad_data->wc = wc;
  vad_data->wl = wl;
  vad_data->zcr1 = zcr1;
  vad_data->zcr2 = zcr2;
  vad_data->wlv = wlv;

  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {

  VAD_STATE state = vad_data->state;

  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

VAD_STATE vad(VAD_DATA *vad_data, float *x) {

  Features f = compute_features(x, vad_data->frame_length);
  vad_data->last_feature = f.p; /* save feature, in case you want to show */

  switch (vad_data->state) {

  case ST_INIT: 
    //printf("P0 incial: %f\n",vad_data->p0);
    if(vad_data->trames_fons <= vad_data->n){ //calcular soroll de fons
        if(vad_data->n==1){
          vad_data->p0 = f.p;
          //printf("P0 (n=1): %f\n",vad_data->p0); 
          vad_data->state = ST_SILENCE;
      }else{
        power = power + pow(10,f.p/10);
        vad_data->state = ST_INIT;      
      }
      vad_data->trames_fons += 1;
    }else{
      vad_data->p0=10*log10(power/vad_data->trames_fons);
      vad_data->state = ST_SILENCE;
      //printf("P0 (n=%d): %f\n", vad_data->n,vad_data->p0);
    }
      
    vad_data-> p1 = vad_data->p0 + vad_data->alpha1; //Definimos valor umbral 1
    vad_data-> p2 = vad_data->p1 + vad_data->alpha2; //Definimos valor umbral 2
    
    //printf("P1= %f; P2= %f\n",vad_data->p1 , vad_data->p2);
    //printf("Frame (mostres): %d\n", vad_data->frame_length);
    //printf("Sampling Rate: %f\n", vad_data->sampling_rate);

  
    break;

  case ST_SILENCE:
    //printf("Main ----- Tramas V = %d\n", vad_data->num_total_v); --> possible millora en l'etiquetat
    vad_data->num_total_v = 0;
    vad_data->num_total_s += 1;
    //printf("Estat S  ----- Tramas S = %d\n",vad_data->num_total_s);
    //printf("Estat S  ----- ZCR S = %f\n",f.zcr);

    if (f.p > vad_data->p1)
      vad_data->state = ST_MAYBE_VOICE;
    else
       vad_data->state = ST_SILENCE;
    
    break;

  case ST_VOICE:
    //printf("Main ----- Tramas S = %d\n", vad_data->num_total_s);
    vad_data->num_total_s = 0;
    vad_data->num_total_v += 1;
    //printf("Estat V  ----- Tramas V = %d\n",vad_data->num_total_v);
    //printf("Estat V  ----- ZCR V = %f\n",f.zcr);
    if (f.p < vad_data->p2 && f.zcr>vad_data->zcr1)
      vad_data->state = ST_MAYBE_SILENCE;
    else
      vad_data->state = ST_VOICE;
    break;

  case ST_MAYBE_VOICE: 
    //printf("Estat MV ----- P = %f  Tramas = %d\n", f.p, vad_data->num_trames);
    vad_data->num_trames += 1;

    if(f.p > vad_data->p2){
      vad_data->num_trames_maybe_v += 1;
      //printf("En MV ------> Tramas MV = %d\n", vad_data->num_trames_maybe_v);
      vad_data->state = ST_MAYBE_VOICE;
    }

    if (vad_data->num_trames_maybe_v == vad_data->wc){      
      vad_data->state = ST_VOICE;
      vad_data->num_trames_maybe_v = 0;
      vad_data->num_trames = 0;
    } 

    if(f.p < vad_data->p1 || vad_data->num_trames == vad_data->wlv){ //Si es falsa alarma o si llevo en el limbo demasiados frames
      vad_data->state = ST_SILENCE;
      vad_data->num_trames_maybe_v = 0;
      vad_data->num_trames = 0;
    }
    break;

  case ST_MAYBE_SILENCE: 
    //printf("Estat MS ----- P = %f  Tramas = %d\n", f.p, vad_data->num_trames);
    vad_data->num_trames += 1;
      
    if(f.p < vad_data->p1){       
      vad_data->num_trames_maybe_s += 1;
      //printf("En MS ----> Tramas MS = %d\n", vad_data->num_trames_maybe_s) ;
      vad_data->state = ST_MAYBE_SILENCE;
    }

    if (vad_data->num_trames_maybe_s == vad_data->wc && f.zcr > vad_data->zcr2){ //Supero 3 tramas en silencio
      vad_data->state = ST_SILENCE;
      vad_data->num_trames_maybe_s = 0;
      vad_data->num_trames = 0;
    } 

    if(f.p > vad_data->p2 || vad_data->num_trames == vad_data->wl){ //Si es flsa alarma o si llevo en el limbo demasiados frames
      vad_data->state = ST_VOICE;
      vad_data->num_trames_maybe_s = 0;
      vad_data->num_trames = 0;
    }

    break;    

  case ST_UNDEF:
    break;
  }

  return vad_data->state;
  /*if (vad_data->state == ST_SILENCE ||
      vad_data->state == ST_VOICE || )
    return vad_data->state;
  else 
    return ST_UNDEF;
  */
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}