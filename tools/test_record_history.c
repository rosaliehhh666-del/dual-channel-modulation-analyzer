#include "record_history.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    record_history_t h;v32_frame_t f={0};record_entry_t e;
    assert(!record_history_init(&h,1));
    assert(!record_history_copy(&h,0,&e));record_history_free(&h);
    assert(record_history_init(&h,3*sizeof(record_entry_t)));
    f.n=256;f.decimation=16;f.start_index=500;
    for(unsigned i=0;i<10;i++){
        f.frame_id=UINT32_MAX-3+i;f.ch0[0]=(int16_t)(-100-i);
        f.ch1[255]=(int16_t)(200+i);f.start_index+=10000;
        record_history_push(&h,&f,100+i);
    }
    assert(h.count==3 && h.total==10);
    for(unsigned a=0;a<3;a++){
        assert(record_history_copy(&h,a,&e));
        assert(e.serial==10-a && e.received_ms==109-a);
        assert(e.frame.ch0[0]==-109+(int)a && e.frame.ch1[255]==209-(int)a);
        assert(e.frame.decimation==16 && e.discontinuity==1);
    }
    assert(!record_history_copy(&h,3,&e));
    record_history_free(&h);assert(!h.entries && !h.count);
    assert(record_history_init(&h,RECORD_HISTORY_BUDGET));
    assert(h.capacity*sizeof(record_entry_t)<=RECORD_HISTORY_BUDGET);
    printf("PASS history: %zu slots, bounded memory, wrap, signed samples, metadata, independent records\n",h.capacity);
    record_history_free(&h);
}
