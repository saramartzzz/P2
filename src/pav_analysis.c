#include <math.h>
#include "pav_analysis.h"
#include <stdio.h>

float compute_power(const float *x, unsigned int N) { // x és vector
    int i;
    float op, power_DB;
    for (i = 0; i < N; i++) op = op + pow(x[i],2);
    power_DB = 10*log10(op/N);
    return power_DB;
}
//*x = puntero al vector x de datos
//N = # muestras de la trama actual
float compute_am(const float *x, unsigned int N) {
    int i;
    float op, am;
    for (i = 0; i < N; i++) op = op + fabs(x[i]);
    am = op/N;
    return am;
}

float compute_zcr(const float *x, unsigned int N, float fm) {
    int i,sum,factor,zeros;
    sum=0;
    zeros=0;
    for (i=0; i<=N-1; i++){
        if(x[i]==0){
            zeros++;
         }
        if(x[i]*x[i+1]<=0){ //punt zero es compta com a creuament.
            sum++;
            }
    }
    factor=fm/2*(N-1);
    return((sum-zeros));
}
