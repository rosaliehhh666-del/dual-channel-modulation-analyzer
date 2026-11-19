#include "../src/scope_processing.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    float a[256];
    /* Cross at 100 but only exceed hysteresis far outside the 40-point view. */
    for(int i=0;i<256;i++)a[i]=i<100?-100.f:(i<200?1.f:100.f);
    assert(scope_trigger_start(a,256,40,0,0)==-1);
    /* A valid full transition inside the same view remains accepted. */
    for(int i=100;i<256;i++)a[i]=100.f;
    assert(scope_trigger_start(a,256,40,0,0)==92);
    for(int i=0;i<256;i++)a[i]=-a[i];
    assert(scope_trigger_start(a,256,40,0,1)==92);
    assert(scope_trigger_start(a,256,40,NAN,1)==-1);
    assert(scope_trigger_start(a,256,257,0,1)==-1);
    const float steady[]={10.f,10.1f,9.9f,10.2f,9.8f};
    const float jumping[]={10.f,10.1f,9.9f,10.2f,8.5f};
    assert(!scope_weak_fit_repeatable(steady,4,10.f));
    assert(scope_weak_fit_repeatable(steady,5,10.f));
    assert(!scope_weak_fit_repeatable(jumping,5,10.f));
    puts("PASS trigger confirmation within record, both edges and invalid settings");
}
