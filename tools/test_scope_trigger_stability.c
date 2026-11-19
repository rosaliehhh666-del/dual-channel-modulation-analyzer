#include "../src/scope_processing.h"
#include <assert.h>
#include <stdio.h>
int main(void){float a[1024];for(int p=0;p<100;p++){for(int i=0;i<1024;i++)a[i]=((i+p)%100<50)?100:-100;for(int e=0;e<2;e++){int k=scope_trigger_start(a,1024,400,0,e);assert(k>=0);assert(e?a[k+80]<0:a[k+80]>0);}}for(int i=0;i<1024;i++)a[i]=-100;a[200]=1;a[400]=100;assert(scope_trigger_start(a,1024,400,0,0)==320);puts("PASS trigger phases and ringing");}
