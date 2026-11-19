#include "auto_modulation.h"
#include "angle_reference.h"
#include "modulation_view.h"
#include "auto_binary.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    float i[256],q[256],a[256],df[256];
    for(int kind=0;kind<5;kind++){
        double phase=0;
        for(int k=0;k<256;k++){
            const double t=6.28318530718*500*k/31738.28125;
            const double amplitude=kind==4?2:1000*(1+(kind==1||kind==3?.5*sin(t):0));
            phase+=6.28318530718*(900+(kind==2||kind==3?2000*sin(t):0))/31738.28125;
            i[k]=(float)(amplitude*cos(phase));q[k]=(float)(amplitude*sin(phase));
        }
        auto_mod_result_t r=auto_mod_analyze(i,q,256,31738.28125f,a,df);
        const enum auto_mod_kind expected[]={AUTO_MOD_CARRIER,AUTO_MOD_AM,AUTO_MOD_ANGLE,AUTO_MOD_WAIT,AUTO_MOD_WAIT};
        printf("case %d: %d\n",kind,r.kind);
        assert(r.kind==expected[kind]);
    }

    /* An 8 ms frame cannot establish a 150 Hz repetition. A continuous
       history can recover it even at low modulation depth or deviation. */
    static float hi[8192],hq[8192],ha[1024],hd[1024];
    const double fs=31738.28125;
    const float fs_standard=65000000.f/4096;
    assert(mod_view_auto_ms(1000.,4)==4.);
    assert(mod_view_auto_ms(500.,3)==6.);
    assert(mod_view_auto_ms(200.,3)==15.);
    assert(mod_view_auto_ms(1000.,8)==8.);
    assert(mod_view_auto_ms(500.,8)==16.);
    assert(mod_view_auto_ms(0.,3)==16.);
    assert(mod_view_auto_ms(1.,8)==1024.);
    assert(mod_view_count(30,fs_standard,4.)==30);
    assert(mod_view_count(1024,fs_standard,4.)==64);
    for(int k=0;k<1024;k++)ha[k]=(float)(200.+900.*sin(6.28318530718*200.*k/fs_standard));
    assert(fabs(auto_mod_frequency_rate(ha,1024,fs_standard)-200.)<5.);
    for(int k=0;k<1024;k++)ha[k]=200.f;
    assert(auto_mod_frequency_rate(ha,1024,fs_standard)==0.);
    for(int k=0;k<8192;k++){
        const double amplitude=22.*(1.+.5*sin(6.28318530718*1000.*k/fs_standard));
        const double carrier_phase=6.28318530718*120.*k/fs_standard;
        hi[k]=(float)(amplitude*cos(carrier_phase));
        hq[k]=(float)(amplitude*sin(carrier_phase));
    }
    auto_mod_result_t am=auto_mod_analyze(hi,hq,8192,fs_standard,ha,hd);
    assert(am.kind==AUTO_MOD_AM);
    auto_mod_refine_am(&am,hi,hq,8192,fs_standard,ha,hd);
    printf("AM 1 kHz: modulation=%.1f Hz, depth=%.1f%%\n",am.modulation_hz,am.depth_pct);
    assert(fabsf(am.modulation_hz-1000.f)<20.f && fabsf(am.depth_pct-50.f)<1.f);
    /* Replay the shallow-AM conditions with small, quantized I/Q values. */
    for(int k=0;k<8192;k++){
        const double t=k/(double)fs_standard;
        const double amplitude=12.*(1.+.25*sin(6.28318530718*500.*t));
        hi[k]=roundf(amplitude*cos(6.28318530718*83.*t));
        hq[k]=roundf(amplitude*sin(6.28318530718*83.*t));
    }
    am=auto_mod_analyze(hi,hq,8192,fs_standard,ha,hd);
    auto_mod_refine_am(&am,hi,hq,8192,fs_standard,ha,hd);
    auto_mod_refine_tone(&am,hi,hq,8192,fs_standard);
    assert(am.kind==AUTO_MOD_AM && am.tone_quality>=AUTO_MOD_AM_FIT_MIN);
    assert(fabsf(am.depth_pct-25.f)<1.f);
    for(int fm=0;fm<=1;fm++){
        double phase=0.;
        for(int k=0;k<8192;k++){
            const double t=6.28318530718*150*k/fs;
            const double amplitude=12.*(1.+(fm?0.:.07*sin(t)));
            phase+=6.28318530718*(900.+(fm?100.*sin(t):0.))/fs;
            hi[k]=(float)(amplitude*cos(phase));
            hq[k]=(float)(amplitude*sin(phase));
        }
        auto_mod_result_t r=auto_mod_analyze(hi,hq,8192,(float)fs,ha,hd);
        assert(r.kind==(fm?AUTO_MOD_ANGLE:AUTO_MOD_AM));
    }
    /* Match the current instrument setting: 1 kHz FM, 2 kHz deviation. */
    double phase=0.;
    for(int k=0;k<8192;k++){
        phase+=6.28318530718*(120.+2000.*sin(6.28318530718*1000.*k/fs))/fs;
        hi[k]=(float)(12.*cos(phase));
        hq[k]=(float)(12.*sin(phase));
    }
    auto_mod_result_t fm=auto_mod_analyze(hi,hq,8192,(float)fs,ha,hd);
    auto_mod_refine_tone(&fm,hi,hq,8192,(float)fs);
    printf("FM 1 kHz: kind=%d, modulation=%.1f Hz, deviation=%.1f Hz\n",
           fm.kind,fm.modulation_hz,fm.deviation_hz);
    assert(fm.kind==AUTO_MOD_ANGLE);
    assert(fabsf(fm.modulation_hz-1000.f)<70.f);
    assert(fabsf(fm.deviation_hz-2000.f)<25.f);
    /* A 1 kHz message exceeds Nyquist after averaging by eight at R=4096. */
    for(int k=0;k<8192;k++){
        const double t=k/(double)fs_standard;
        phase=6.28318530718*120.*t-.05*cos(6.28318530718*1000.*t);
        hi[k]=(float)(1000.*cos(phase));
        hq[k]=(float)(1000.*sin(phase));
    }
    fm=auto_mod_analyze(hi,hq,8192,fs_standard,ha,hd);
    auto_mod_refine_tone(&fm,hi,hq,8192,fs_standard);
    printf("Narrow FM: kind=%d, modulation=%.1f Hz, deviation=%.1f Hz\n",
           fm.kind,fm.modulation_hz,fm.deviation_hz);
    assert(fm.kind==AUTO_MOD_ANGLE);
    assert(fabsf(fm.modulation_hz-1000.f)<10.f);
    assert(fabsf(fm.deviation_hz-50.f)<.2f);
    for(int k=0;k<8192;k++){
        hi[k]=roundf(hi[k]*.032f);
        hq[k]=roundf(hq[k]*.032f);
    }
    fm=auto_mod_analyze(hi,hq,8192,fs_standard,ha,hd);
    auto_mod_refine_tone(&fm,hi,hq,8192,fs_standard);
    printf("Quantized narrow FM: kind=%d, modulation=%.1f Hz, deviation=%.1f Hz\n",
           fm.kind,fm.modulation_hz,fm.deviation_hz);
    assert(fm.kind==AUTO_MOD_ANGLE);
    assert(fabsf(fm.modulation_hz-1000.f)<10.f);
    assert(fabsf(fm.deviation_hz-50.f)<2.f);
    for(int k=1;k<1024;k++){
        const float cross=hi[k-1]*hq[k]-hq[k-1]*hi[k];
        const float dot=hi[k-1]*hi[k]+hq[k-1]*hq[k];
        hd[k-1]=atan2f(cross,dot)*fs_standard/6.28318530718f;
    }
    const auto_mod_tone_t manual_fm=auto_mod_fit_narrow_fm(hd,1023,fs_standard,992.);
    assert(manual_fm.quality>.93);
    assert(fabs(manual_fm.rate-1000.)<5. && fabs(manual_fm.peak-50.)<2.);
    const auto_mod_tone_t noisy_rate=auto_mod_fit_narrow_fm(hd,1023,fs_standard,167.);
    assert(noisy_rate.quality>.93);
    assert(fabs(noisy_rate.rate-1000.)<5. && fabs(noisy_rate.peak-50.)<2.);
    /* Known PM: 60 degrees of phase deviation and a residual carrier offset. */
    for(int k=0;k<8192;k++){
        const double t=k/fs;
        phase=6.28318530718*120.*t + 1.0471975512*sin(6.28318530718*200.*t);
        hi[k]=(float)(12.*cos(phase));
        hq[k]=(float)(12.*sin(phase));
    }
    const auto_mod_result_t pm=auto_mod_analyze(hi,hq,8192,(float)fs,ha,hd);
    printf("PM 200 Hz: kind=%d, phase deviation=%.1f deg\n",pm.kind,pm.phase_deviation_deg);
    assert(pm.kind==AUTO_MOD_ANGLE);
    assert(fabsf(pm.phase_deviation_deg-60.f)<3.f);
    /* Both the PM phase samples and FM interval frequencies remove drift
       before zooming. A final window shorter than one cycle keeps its slope. */
    hi[0]=NAN;
    for(int k=1;k<=1024;k++){
        const double t=(k-1)/fs;
        hi[k]=(float)remainder(149.+360.*120.*t+90.*sin(6.28318530718*1000.*t),360.);
        hq[k]=(float)(120.+90.*(sin(6.28318530718*1000.*t)-
                       sin(6.28318530718*1000.*(t-1./fs)))*fs/360.);
    }
    assert(mod_view_relative_phase(hi,1025,fs,0,ha)==1024);
    assert(mod_view_relative_phase(hq+1,1024,fs,1,hd)==1024);
    for(int k=1015;k<1024;k++){
        const double expected=90.*sin(6.28318530718*1000.*k/fs);
        assert(fabs(ha[k]-expected)<1.5);
        assert(fabs(hd[k]-ha[k])<.01);
    }
    hi[1024]=NAN;
    assert(mod_view_relative_phase(hi,1025,fs,0,ha)==0);
    /* Period alignment suppresses asynchronous noise while preserving a
       real harmonic. The derivative must share the phase trace's time axis. */
    for(int k=0;k<1024;k++){
        const double t=6.28318530718*1000.*k/fs_standard;
        hi[k]=(float)(2.865*sin(t)+.25*sin(2.*t)+.60*sin(1.386*t));
    }
    assert(mod_view_average_angle(hi,1024,ha,hd,48,fs_standard,1000.)==16);
    double phase_err=0.,freq_err=0.;
    for(int k=0;k<48;k++){
        const double t=6.28318530718*1000.*(1024-48-2+k)/fs_standard;
        const double expected_phase=2.865*sin(t)+.25*sin(2.*t);
        const double expected_freq=(2.865*cos(t)+.5*cos(2.*t))*6.28318530718*1000./360.;
        phase_err+=pow(ha[k+2]-expected_phase,2.);
        freq_err+=pow(hd[k]-expected_freq,2.);
    }
    assert(sqrt(phase_err/48.)<.09);
    assert(sqrt(freq_err/48.)<2.);
    hi[925]=NAN;
    assert(!mod_view_average_angle(hi,1024,ha,hd,48,fs_standard,1000.));
    /* Reference matching uses one simultaneous 4096-point RAW frame. */
    const double raw_fs=65000000./16.;
    assert(angle_reference_decimation(65000000.,1000000.)==16);
    for(int mode=0;mode<6;mode++){
        for(int k=0;k<4096;k++){
            const double t=k/raw_fs+.000163;
            const double x=6.283185307179586*1000.*t;
            const double deviation=mode==0?-1.5*cos(x):mode==1?1.5*sin(x):mode==5?-.05*cos(x):0.;
            hi[k]=roundf((float)(80.*cos(1000.*x+deviation)+4.));
            hq[k]=mode==3?0.f:roundf((float)(-80.*sin(x+(mode==4?.7853981633974483:0.))-7.));
            if(mode==4)hi[k]=roundf((float)(80.*cos(1000.*x+1.5*sin(x))+4.));
        }
        const enum angle_reference_kind expect[]={ANGLE_REF_FM,ANGLE_REF_PM,ANGLE_REF_UNKNOWN,
                                                 ANGLE_REF_UNKNOWN,ANGLE_REF_UNKNOWN,ANGLE_REF_FM};
        assert(angle_reference_classify(hi,hq,4096,raw_fs,999950.,1000.)==expect[mode]);
    }
    assert(angle_reference_classify(hi,hq,1024,raw_fs,1e6,1000.)==ANGLE_REF_UNKNOWN);
    const int rates[]={100,250,500,1000,2000};
    float features[AUTO_BINARY_SAMPLES];
    const char pattern[]="0010111010010110010101101";
    for(int fsk=0;fsk<2;fsk++){
        phase=0.;
        for(int k=0;k<4096;k++){
            const int bit=pattern[(int)(k*1000/fs)%25]-'0';
            const double magnitude=fsk || bit?40.:0.;
            phase+=6.28318530718*(fsk?(bit?1200.:-1200.):80.)/fs;
            hi[k]=(float)(magnitude*cos(phase));
            hq[k]=(float)(magnitude*sin(phase));
        }
        const auto_binary_result_t rx=auto_binary_analyze(hi,hq,4096,(float)fs,rates,5,features);
        printf("digital %d: %d Bd, %s\n",fsk,rx.baud,rx.bits);
        assert(rx.baud==1000 && rx.fsk==fsk && strlen(rx.bits)==32);
        assert(rx.bit_count==32);
        for(int k=0;k<rx.bit_count;k++){
            const int at=(int)((rx.edges[k]+rx.edges[k+1])*.5f);
            assert(rx.edges[k]<rx.edges[k+1]);
            assert((features[at]>(fsk?0.f:20.f))==(rx.bits[k]=='1'));
        }
        char repeated[76];
        snprintf(repeated,sizeof repeated,"%s%s%s",pattern,pattern,pattern);
        assert(strstr(repeated,rx.bits));
    }
    for(int k=0;k<4096;k++){
        const double t=6.28318530718*500*k/fs;
        phase+=6.28318530718*(120.+1500.*sin(t))/fs;
        hi[k]=(float)(40.*cos(phase));hq[k]=(float)(40.*sin(phase));
    }
    assert(!auto_binary_analyze(hi,hq,4096,(float)fs,rates,5,features).baud);
    const float curve[]={0.f,2.f,3.f,2.f,NAN,1.f};
    float smooth[51];
    mod_view_smooth(curve,6,smooth,51,0.f,3.f);
    assert(smooth[0]==0.f && smooth[10]==2.f/3.f && smooth[20]==1.f);
    for(int k=0;k<30;k++){
        if(k<20)assert(smooth[k]<=smooth[k+1]);
        else assert(smooth[k]>=smooth[k+1]);
    }
    for(int k=31;k<50;k++)assert(isnan(smooth[k]));
}
