#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <dlfcn.h>
#include "unit.h"
#include "lpc_engine.h"
#include "effects.h"
#include "random_controls.h"

int main(int argc,char** argv) {
  assert(argc==3);
  void* lib=dlopen(argv[1],RTLD_NOW);
  if(!lib){std::fprintf(stderr,"%s\n",dlerror());return 1;}
#define FN(name) auto name = reinterpret_cast<decltype(&::name)>(dlsym(lib,#name)); assert(name)
  FN(unit_init); FN(unit_render); FN(unit_reset); FN(unit_suspend); FN(unit_resume);
  FN(unit_set_param_value); FN(unit_get_param_value); FN(unit_get_param_str_value);
  FN(unit_note_on); FN(unit_note_off); FN(unit_gate_on); FN(unit_gate_off);
  FN(unit_all_note_off); FN(unit_pitch_bend); FN(unit_set_tempo);
  auto header=reinterpret_cast<const unit_header_t*>(dlsym(lib,"unit_header")); assert(header);
  assert(header->num_params==11 && header->target==(UNIT_TARGET_PLATFORM|k_unit_module_synth));
  assert(header->params[0].max==int(kWordCount)-1);
  assert(std::strcmp(header->name,"LPC ROBOT 7")==0);
  assert(unit_init(nullptr)==k_unit_err_undef);
  unit_runtime_desc_t d={}; d.target=header->target; d.api=UNIT_API_VERSION;
  d.samplerate=48000; d.output_channels=2; d.frames_per_buffer=64;
  auto bad=d; bad.samplerate=44100; assert(unit_init(&bad)==k_unit_err_samplerate);
  bad=d; bad.output_channels=1; assert(unit_init(&bad)==k_unit_err_geometry);
  bad=d; bad.target=0; assert(unit_init(&bad)==k_unit_err_target);
  bad=d; bad.api=0; assert(unit_init(&bad)==k_unit_err_api_version);
  assert(unit_init(&d)==k_unit_err_none);
  auto defaults=[&](){int p[11]={0,24,100,-1,80,0,250,35,0,5,80};for(int i=0;i<11;++i)unit_set_param_value(i,p[i]);unit_pitch_bend(8192);};
  auto render=[&](size_t count,unsigned block){
    std::vector<float> y(count*2);
    for(size_t at=0;at<count;at+=block) {
      size_t n=std::min(size_t(block),count-at);
      unit_render(nullptr,y.data()+at*2,n);
    }
    for(size_t i=0;i<count;++i) {assert(std::isfinite(y[i*2]) && std::isfinite(y[i*2+1]));assert(std::fabs(y[i*2])<=.951f && std::fabs(y[i*2+1])<=.951f);if(unit_get_param_value(5)==0)assert(y[i*2]==y[i*2+1]);}
    return y;
  };
  auto silent=[](const std::vector<float>& y){for(float f:y) if(f!=0)return false;return true;};
  auto save=[&](const char* name,const std::vector<float>& y){
    char path[1024];std::snprintf(path,sizeof(path),"%s/%s.f32",argv[2],name);
    auto f=std::fopen(path,"wb");assert(f);assert(std::fwrite(y.data(),sizeof(float),y.size(),f)==y.size());std::fclose(f);
  };
  defaults();unit_reset();assert(silent(render(123,17)));
  for(int word=0;word<int(kWordCount);++word) {
    defaults();unit_reset();unit_set_param_value(0,word);unit_gate_on(127);
    const size_t wordSamples=size_t(kWords[word].samples)*6+14400;
    auto y=render(wordSamples,64); assert(!silent(y));
    for(size_t i=y.size()-4800;i<y.size();++i) assert(y[i]==0);
    char name[32];std::snprintf(name,sizeof(name),"word_%d",word);save(name,y);
    unit_reset();unit_gate_on(127); auto split=render(wordSamples,17);
    assert(y==split); // sample-accurate independent of callback block size
    unit_reset();unit_gate_on(127);unit_gate_off();
    assert(y==render(wordSamples,64)); // ordinary sequencer gate does not truncate a word
  }
  defaults();unit_reset();unit_set_param_value(3,45);unit_note_on(60,127);
  auto frozen=render(144000,64);assert(!silent(frozen));save("freeze",frozen);
  unit_note_off(61); assert(!silent(render(4800,64))); // unrelated MIDI note
  unit_note_off(60);render(4800,64);assert(silent(render(1000,64)));
  unit_gate_off();unit_gate_on(127);render(4800,64);
  assert(!silent(render(4800,64))); // a previous gate-off cannot kill a newer trigger
  for(int word=0;word<int(kWordCount);++word) for(int extreme=0;extreme<2;++extreme) {
    defaults();unit_reset();unit_set_param_value(0,word);
    unit_set_param_value(1,extreme?74:0);unit_set_param_value(2,extreme?250:25);
    unit_set_param_value(3,extreme?100:0);unit_set_param_value(4,100);
    unit_pitch_bend(extreme?16383:0);unit_note_on(extreme?127:0,127);render(48000,31);
    unit_all_note_off();assert(silent(render(300,13)));
  }
  defaults();unit_set_param_value(0,999);assert(unit_get_param_value(0)==int(kWordCount)-1);
  unit_set_param_value(2,-10);assert(unit_get_param_value(2)==25);
  unit_set_param_value(255,999);assert(unit_get_param_value(255)==0);
  assert(unit_get_param_str_value(0,-1)==nullptr);
  defaults();unit_reset();unit_note_on(60,0);assert(silent(render(500,32)));
  unit_gate_on(127);render(1000,64);unit_suspend();assert(silent(render(1000,64)));
  unit_resume();assert(silent(render(1000,64)));assert(unit_get_param_value(2)==100);
  // Pitch and speed must be independent at DSP level.
  LpcEngine e; e.controls(0,100,-1,80);e.trigger(0,60,127);
  for(int i=0;i<24000;++i)e.render();float normal=e.position();
  e.reset();e.controls(-12,100,-1,80);e.trigger(0,60,127);
  for(int i=0;i<24000;++i)e.render();assert(e.position()==normal);
  e.reset();e.controls(0,50,-1,80);e.trigger(0,60,127);
  for(int i=0;i<24000;++i)e.render();assert(std::fabs(e.position()-normal*.5f)<1);
  defaults();unit_reset();unit_set_param_value(1,12);unit_gate_on(127);save("low",render(96000,64));
  defaults();unit_reset();unit_set_param_value(2,60);unit_gate_on(127);save("slow",render(144000,64));
  // INDEX maps only to non-silent frames, monotonically through each word.
  float minimumEnergy=1e9f;
  for(unsigned word=0;word<int(kWordCount);++word) {

    float previous=-1;
    for(int index=0;index<=100;++index) {
      e.reset();e.controls(0,100,index,80);e.trigger(word,60,127);
      float at=e.position();assert(at>=previous);previous=at;
      assert(at>=0 && at<kWords[word].samples);
      float source=at+kWords[word].offset;
      auto frame=unsigned(source/80.f);assert(frame<kWords[word].count);
      assert(source>=kWords[word].offset && source<kWords[word].offset+kWords[word].samples);
      float energy=0;
      for(int i=0;i<2400;++i){float v=e.render();if(i>=1200)energy+=v*v;}
      minimumEnergy=std::min(minimumEnergy,energy/1200.f);
      assert(energy/1200.f>1e-7f);assert(e.position()==at);
    }
  }
  std::printf("INDEX all positions: min held RMS %.6f\n",std::sqrt(minimumEnergy));
  e.reset();e.controls(0,100,25,80);e.trigger(0,60,127);
  float at25=e.position();e.controls(0,100,75,80);float at75=e.position();assert(at75>=at25);
  e.controls(0,100,-1,80);for(int i=0;i<480;++i)e.render();assert(e.position()>at75);
  e.controls(50,100,-1,80);assert(std::fabs(e.pitchHz()-130.f*std::pow(2.f,50.f/12))<.1f);
  e.controls(-24,100,-1,80);assert(std::fabs(e.pitchHz()-32.5f)<.01f);
  e.controls(0,100,101,80);std::vector<float> positions;
  for(int i=0;i<12;++i){e.trigger(0,60,127);positions.push_back(e.position());assert(e.position()<kWords[0].samples);}
  bool varied=false;for(float at:positions)if(at!=positions[0])varied=true;assert(varied);
  // RND traverses a short source interval, wraps within the edited word, and
  // finishes the current grain even if a sequencer gate-off arrives immediately.
  for(unsigned word=0;word<kWordCount;++word) for(int ms:{20,80,250}) for(int speed:{25,100,250}) {
    LpcEngine grain;grain.controls(0,speed,101,100);grain.setStutterTime(ms);grain.trigger(word,60,127);
    float start=grain.position();
    float end=std::min(float(kWords[word].samples),kWords[word].count*80.f-kWords[word].offset);
    float span=std::min(ms*8.f,std::max(1.f,end-start));
    unsigned ticks=std::max(1u,unsigned(std::ceil(span/(speed*.01f))));
    float energy=0;bool advanced=false;
    for(unsigned n=0;n<ticks*6;++n) {
      float v=grain.render();assert(std::isfinite(v));energy+=v*v;
      assert(grain.position()>=start && grain.position()<end);
      advanced|=grain.position()>start;
    }
    assert(advanced && energy>1e-7f);
    float lastPosition=grain.position();
    for(unsigned n=0;n<6000;++n) {
      float v=grain.render();assert(grain.position()==lastPosition);
      if(n>=6)assert(v==0); // exactly 1/16 at the default BPM 120
    }
    grain.render();assert(std::fabs(grain.position()-start)<.01f);
    grain.release();for(unsigned n=0;n<ticks*6+1000;++n)grain.render();assert(!grain.active());
    LpcEngine tinyGate;tinyGate.controls(0,speed,101,100);tinyGate.setStutterTime(ms);tinyGate.trigger(word,60,127);tinyGate.release();
    float shortEnergy=0;
    for(unsigned n=0;n<ticks*6;++n){float v=tinyGate.render();shortEnergy+=v*v;}
    assert(shortEnergy>1e-7f);
    for(unsigned n=0;n<1000;++n)tinyGate.render();assert(!tinyGate.active());
  }
  unit_set_param_value(10,1);assert(unit_get_param_value(10)==20);
  for(unsigned bpm:{60u,120u,240u}) {
    LpcEngine syncGrain;syncGrain.controls(0,100,101,100);syncGrain.setStutterTime(20);syncGrain.setStutterTempo(bpm<<16);syncGrain.trigger(0,60,127);
    float start=syncGrain.position();
    unsigned ticks=unsigned(std::ceil(std::min(160.f,kWords[0].samples-start)));
    for(unsigned n=0;n<ticks*6;++n)syncGrain.render();
    float last=syncGrain.position();unsigned gap=120000u/bpm;
    for(unsigned n=0;n<gap*6;++n){float v=syncGrain.render();if(n>=6)assert(v==0);assert(syncGrain.position()==last);}
    syncGrain.render();assert(std::fabs(syncGrain.position()-start)<.01f);
  }
  unit_set_param_value(10,999);assert(unit_get_param_value(10)==250);
  defaults();unit_reset();unit_set_param_value(3,101);unit_gate_on(127);
  save("stutter80",render(96000,64));unit_gate_off();render(96000,64);assert(silent(render(4800,64)));
  // The redundant output PAN is removed. Out-of-range writes are ignored.
  unit_set_param_value(11,-100);assert(unit_get_param_value(11)==0);
  defaults();unit_reset();unit_gate_on(127);auto noPan=render(48000,64);
  unit_set_param_value(11,100);unit_reset();unit_gate_on(127);assert(noPan==render(48000,64));

  assert(std::strcmp(unit_get_param_str_value(3,-1),"OFF")==0);
  assert(std::strcmp(unit_get_param_str_value(3,101),"RND")==0);
  assert(std::strcmp(unit_get_param_str_value(3,50),"50%")==0);
  unit_set_param_value(1,74);assert(unit_get_param_value(1)==74);
  assert(header->params[1].min==0 && header->params[1].max==75 && header->params[1].init==24);
  for(int key=0;key<=74;++key) {
    char expected[8];std::snprintf(expected,sizeof(expected),"%+dst",key-24);
    assert(std::strcmp(unit_get_param_str_value(1,key),expected)==0);
    unit_set_param_value(1,key);assert(unit_get_param_value(1)==key);
  }
  assert(unit_get_param_str_value(1,-1)==nullptr);
  // LEVEL remains a linear user trim AFTER the new internal source gain/limiter.
  defaults();unit_reset();unit_set_param_value(4,100);unit_gate_on(127);auto loud=render(96000,64);
  defaults();unit_reset();unit_set_param_value(4,50);unit_gate_on(127);auto half=render(96000,64);
  for(size_t i=0;i<loud.size();++i)assert(std::fabs(half[i]-.5f*loud[i])<1e-6f);
  defaults();unit_reset();unit_set_param_value(1,74);unit_gate_on(127);save("high50",render(96000,64));
  std::vector<float> rnd;
  defaults();unit_set_param_value(3,101);
  for(int i=0;i<8;++i){unit_reset();unit_gate_on(127);auto chunk=render(12000,64);rnd.insert(rnd.end(),chunk.begin(),chunk.end());}
  save("random",rnd);
  // Stereo echo pairs: first left and then right at equal volume, then pair feedback.
  RobotEffects fx;fx.controls(100,250,35);fx.reset();
  std::vector<float> impulseL(50000),impulseR(50000),impulseStereo;
  for(int i=0;i<50000;++i)fx.process(i==0?1.f:0.f,impulseL[i],impulseR[i]);
  assert(std::fabs(impulseL[12000]-1.f)<1e-4f && impulseR[12000]==0);
  assert(impulseL[24000]==0 && std::fabs(impulseR[24000]-1.f)<1e-4f);
  assert(std::fabs(impulseL[36000]-.35f)<1e-4f && impulseR[36000]==0);
  assert(impulseL[48000]==0 && std::fabs(impulseR[48000]-.35f)<1e-4f);
  for(size_t i=0;i<impulseL.size();++i){impulseStereo.push_back(impulseL[i]*.5f);impulseStereo.push_back(impulseR[i]*.5f);}
  save("stereo_impulse",impulseStereo);
  fx.reset();for(int i=0;i<550000;++i){float l,r;fx.process(0,l,r);assert(l==0 && r==0);}
  fx.controls(100,40,75);fx.reset();
  for(int i=0;i<600000;++i){float l,r;fx.process(i%48000==0?.5f:0,l,r);assert(std::isfinite(l)&&std::isfinite(r));}
  defaults();unit_reset();unit_set_param_value(5,85);unit_gate_on(127);
  auto delay=render(240000,64);save("pan_delay",delay);
  bool stereo=false;for(size_t i=0;i<delay.size();i+=2)if(std::fabs(delay[i]-delay[i+1])>1e-5f)stereo=true;assert(stereo);
  unit_reset();unit_gate_on(127);assert(delay==render(240000,17));
  unit_all_note_off();assert(silent(render(100000,31)));
  defaults();unit_reset();unit_set_param_value(4,0);unit_set_param_value(5,100);unit_gate_on(127);
  assert(silent(render(100000,64)));
  std::vector<float> sweep;
  for(int index: {0,25,50,75,100}) {
    defaults();unit_reset();unit_set_param_value(3,index);unit_gate_on(127);
    auto y=render(14400,64);sweep.insert(sweep.end(),y.begin(),y.end());
    sweep.insert(sweep.end(),4800,0.f);
  }
  save("index_sweep",sweep);
  assert(std::fabs(syncedDelayMs(5,120u<<16)-250.f)<1e-4f);
  assert(std::fabs(syncedDelayMs(7,120u<<16)-500.f)<1e-4f);
  assert(std::fabs(syncedDelayMs(3,120u<<16)-166.66667f)<.001f);
  assert(std::fabs(syncedDelayMs(6,120u<<16)-375.f)<1e-4f);
  for(unsigned bpm: {56u,120u,240u}) for(unsigned div=0;div<10;++div)assert(syncedDelayMs(div,bpm<<16)<4000);
  defaults();unit_reset();unit_set_param_value(5,85);unit_set_param_value(8,1);unit_set_param_value(9,7);unit_set_tempo(120u<<16);unit_gate_on(127);
  auto sync120=render(240000,64);save("sync120",sync120);
  defaults();unit_reset();unit_set_param_value(5,85);unit_set_param_value(8,1);unit_set_param_value(9,7);unit_set_tempo(180u<<16);unit_gate_on(127);
  auto sync180=render(240000,64);save("sync180",sync180);assert(sync120!=sync180);
  unit_set_tempo(0);unit_set_tempo(0xffffffffu);render(4800,64);
  unit_set_tempo(10u<<16);unit_set_param_value(9,9);assert(std::strcmp(unit_get_param_str_value(9,9),"MAX 4s")==0);
  unit_set_tempo(120u<<16);
  RandomVoiceControls rc, independent;
  bool pitchVaried=false,speedVaried=false;int firstPitch=0,firstSpeed=0;
  for(int n=0;n<1000;++n) {
    rc.update(51,251,true);independent.update(51,100,true);
    int pitch=rc.pitch(),speed=rc.speed();
    assert(pitch>=-24 && pitch<=50 && speed>=25 && speed<=250);
    assert(pitch==independent.pitch()); // SPEED randomization does not alter PITCH sequence.
    if(n==0){firstPitch=pitch;firstSpeed=speed;}
    pitchVaried|=pitch!=firstPitch;speedVaried|=speed!=firstSpeed;
    for(int b=0;b<10;++b){rc.update(51,251,false);assert(rc.pitch()==pitch && rc.speed()==speed);}
  }
  assert(pitchVaried && speedVaried);
  rc.update(12,80,false);assert(rc.pitch()==12 && rc.speed()==80);
  rc.update(51,251,false);assert(rc.pitch()>=-24 && rc.pitch()<=50 && rc.speed()>=25 && rc.speed()<=250);
  assert(std::strcmp(unit_get_param_str_value(1,75),"RND")==0);
  assert(std::strcmp(unit_get_param_str_value(2,251),"RND")==0);
  assert(std::strcmp(unit_get_param_str_value(1,74),"+50st")==0);
  assert(std::strcmp(unit_get_param_str_value(2,250),"250%")==0);
  defaults();unit_reset();unit_set_param_value(1,75);unit_set_param_value(2,251);
  std::vector<float> randomNotes;bool changed=false;std::vector<float> first;
  for(int n=0;n<8;++n) {
    unit_gate_on(127);auto note=render(48000,64);
    if(n==0)first=note;else changed|=note!=first;
    randomNotes.insert(randomNotes.end(),note.begin(),note.end());
    assert(unit_get_param_value(1)==75 && unit_get_param_value(2)==251);
  }
  assert(changed);save("random_pitch_speed",randomNotes);
  dlclose(lib);
  std::puts("PASS: actual shared unit loaded; init rejection; all bank words; stereo; output bounds; block invariance; one-shot gates; INDEX seek/hold/resume; INDEX/PITCH/SPEED RND; latched values and independent random streams; +50 pitch; source gain/LEVEL; tempo sync divisions/follow; chorus removed; equal-level stereo echo pairs; all bank INDEX positions audible; effect resets and tails; release; extreme controls; all-off; suspend; velocity-zero; independent pitch/speed");
}
