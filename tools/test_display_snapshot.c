#include "../src/wave_source_v32.c"
#include <assert.h>
static int incoming;
int v32_uart_baud(void){return 2000000;}
int v32_init_file(const char *p){(void)p;return 0;}
int v32_init_uart(const char *p,int b){(void)p;(void)b;return 0;}
const char *v32_last_error(void){return "";}
const char *v32_source_name(void){return "test";}
bool v32_is_live(void){return false;}
void v32_shutdown(void){}
int v32_copy_latest(v32_frame_t *f){
    memset(f,0,sizeof *f);f->frame_id=++incoming;f->n=4096;f->decimation=incoming==1?1:32768;
    for(int i=0;i<4096;i++){f->ch0[i]=incoming;f->ch1[i]=-incoming;}
    return 1;
}
int main(void){
    float a[4096],b[4096];uint32_t fid;
    s_inited=1;
    assert(wave_source_v32_begin_frame(&fid) && fid==1);
    assert(v32_src_read2(a,b,1024)==1024 && a[0]==1 && b[0]==-1);
    assert(v32_src_read2(a,b,4096)==4096 && a[4095]==1 && b[4095]==-1);
    assert(incoming==1);wave_source_v32_end_frame();
    assert(wave_source_v32_frame_rate_hz()==65000000.0);
    assert(wave_source_v32_begin_frame(&fid) && fid==2);
    assert(v32_src_read2(a,b,4096)==4096 && a[4095]==2);
    assert(wave_source_v32_frame_rate_hz()==65000000.0/32768.0);
    wave_source_v32_end_frame();puts("PASS: condensed/full channels share one snapshot; next cycle advances");
}
