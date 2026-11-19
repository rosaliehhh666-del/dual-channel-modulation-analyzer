/* Independent analytic waveform oracle: record boundaries, polarity and time
 * direction. This is an algorithm test, not an ADC accuracy certification. */
#include "bl_interp.h"
#include "scope_processing.h"
#include <math.h>
#include <stdio.h>
#include <assert.h>
static float signal(float t) {
    return 80.f*sinf(.071f*t)+23.f*cosf(.139f*t+.7f)+11.f*sinf(.211f*t+.2f);
}
int main(void) {
    float x[1024],y[2048];
    for(int i=0;i<1024;i++)x[i]=signal(i);
    bl_resample(x,1024,y,2048,100.25f,.3f,0);
    double e=0,opposite=0,reverse=0;
    for(int i=0;i<2048;i++){
        float truth=signal(100.25f+.3f*i);
        e+=(y[i]-truth)*(y[i]-truth);
        opposite+=(y[i]+truth)*(y[i]+truth);
        float backwards=signal(100.25f+.3f*(2047-i));
        reverse+=(y[i]-backwards)*(y[i]-backwards);
    }
    double rms=sqrt(e/2048);
    assert(rms<.2 && e*100<opposite && e*100<reverse);
    bl_resample(x,1024,y,4,-1.f,1.f,0);
    assert(isnan(y[0]) && y[1]==x[0] && y[2]==x[1]);
    bl_resample(x,1024,y,3,1023.f,1.f,0);
    assert(y[0]==x[1023] && isnan(y[1]) && isnan(y[2]));
    bl_resample(x,1024,y,1,NAN,1.f,0);assert(isnan(y[0]));
    bl_resample(x,1024,y,1,INFINITY,1.f,0);assert(isnan(y[0]));
    /* A crossing near the end cannot wrap around to obtain post-trigger data. */
    for(int i=0;i<1024;i++)x[i]=i<1000?-100.f:100.f;
    assert(scope_trigger_start(x,1024,400,0,0)==-1);
    printf("PASS asymmetric waveform RMS %.6f ADC counts; no inversion/time reversal; no extrapolation/trigger wrap\n",rms);
}
