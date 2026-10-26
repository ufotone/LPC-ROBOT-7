#pragma once
#include <atomic>
#include <cstdio>
#include "unit.h"
#include "lpc_engine.h"
#include "effects.h"
#include "random_controls.h"

class Synth {
 public:
  Synth() {
    const int defaults[11]={0,24,100,-1,80,0,250,35,0,5,80};
    for(unsigned i=0;i<11;++i) params_[i].store(defaults[i]);
  }
  int8_t Init(const unit_runtime_desc_t* d) {
    if(d->samplerate!=48000) return k_unit_err_samplerate;
    if(d->output_channels!=2) return k_unit_err_geometry;
    engine_.reset(); effects_.reset(); return k_unit_err_none;
  }
  void Teardown(){Reset();}
  void Reset(){trigger_.store(0); off_.store(0); reset_.store(true);}
  void Resume(){Reset();}
  void Suspend(){Reset();}
  void Render(float* out, size_t frames) {
    bool didReset=reset_.exchange(false);
    if(didReset) {engine_.reset();effects_.reset();}
    int p[11]; for(unsigned i=0;i<11;++i) p[i]=params_[i].load();
    engine_.setStutterTime(p[10]);
    engine_.setStutterTempo(tempo_.load());
    unsigned event=trigger_.exchange(0);
    random_controls_.update(p[1]-24,p[2],event!=0);
    engine_.controls(random_controls_.pitch(),random_controls_.speed(),p[3],100,bend_.load());
    if(event) {
      note_=(event>>8)&255;
      engine_.trigger(p[0],note_,event&127);
      engine_.controls(random_controls_.pitch(),random_controls_.speed(),p[3],100,bend_.load());
    }
    unsigned off=off_.exchange(0);
    if(off && p[3]>=0 && (off==256 || off==note_+1)) engine_.release();
    float delayMs=p[8]?syncedDelayMs(p[9],tempo_.load()):float(p[6]);
    effects_.controls(p[5],delayMs,p[7]);
    for(size_t i=0;i<frames;++i) {
      float l,r;effects_.process(engine_.render(),l,r);
      // LEVEL scales the complete output, including effect tails.
      out[i*2]=limit(l)*(p[4]*.01f);out[i*2+1]=limit(r)*(p[4]*.01f);
    }
  }
  void setParameter(uint8_t i,int32_t v) {
    // Nonnegative string keys avoid the hardware's --- display for negative keys.
    static const int lo[11]={0,0,25,-1,0,0,40,0,0,0,20}, hi[11]={int(kWordCount)-1,75,251,101,100,100,750,75,1,9,250};
    if(i<11) params_[i].store(std::max(lo[i],std::min(int(v),hi[i])));
  }
  int32_t getParameterValue(uint8_t i) const {return i<11?params_[i].load():0;}
  const char* getParameterStrValue(uint8_t i,int32_t v) const {
    if(i==0 && v>=0 && unsigned(v)<kWordCount) return kWords[v].name;
    if(i==9 && v>=0 && v<10) {
      static const char* names[]={"1/32","1/16T","1/16","1/8T","1/16D","1/8","1/8D","1/4","1/4D","1/2"};
      if(params_[8].load() && syncedDelayMs(v,tempo_.load())>4000.f)return "MAX 4s";
      return names[v];
    }
    if(i==1) {
      if(v<0 || v>75)return nullptr;
      v-=24;
      if(v==51)return "RND";
      if(v>=-24 && v<=50){std::snprintf(index_text_,sizeof(index_text_),"%+dst",int(v));return index_text_;}
    }
    if(i==2) {
      if(v==251)return "RND";
      if(v>=25 && v<=250){std::snprintf(index_text_,sizeof(index_text_),"%d%%",int(v));return index_text_;}
    }
    if(i==3) {
      if(v==-1) return "OFF";
      if(v==101) return "RND";
      if(v>=0 && v<=100) {std::snprintf(index_text_,sizeof(index_text_),"%d%%",int(v));return index_text_;}
    }
    return nullptr;
  }
  const uint8_t* getParameterBmpValue(uint8_t,int32_t) const {return nullptr;}
  void NoteOn(uint8_t n,uint8_t v) {if(!v){NoteOff(n);return;} off_.store(0); trigger_.store(0x10000u|(unsigned(n==255?60:n)<<8)|std::min(unsigned(v),127u));}
  void NoteOff(uint8_t n){off_.store(n==255?256:unsigned(n)+1);}
  void GateOn(uint8_t v){NoteOn(60,v);}
  void GateOff(){off_.store(256);}
  void AllNoteOff(){Reset();}
  void PitchBend(uint16_t b){bend_.store(std::min(unsigned(b),16383u));}
  void SetTempo(uint32_t t){if(t>=65536u && t<=1000u*65536u)tempo_.store(t);}
  void ChannelPressure(uint8_t){}
  void Aftertouch(uint8_t,uint8_t){}
  void LoadPreset(uint8_t){}
  uint8_t getPresetIndex() const{return 0;}
  static const char* getPresetName(uint8_t){return nullptr;}
 private:
  std::atomic<int32_t> params_[11];
  std::atomic<uint32_t> trigger_{0},off_{0},bend_{8192};
  std::atomic<bool> reset_{false};
  std::atomic<uint32_t> tempo_{120u<<16};
  mutable char index_text_[8]={};
  unsigned note_=60;
  static float limit(float v) {
    float a=std::fabs(v);
    return a<=.8f?v:std::copysign(.8f+.15f*(a-.8f)/(.15f+a-.8f),v);
  }
  RandomVoiceControls random_controls_;
  LpcEngine engine_;
  RobotEffects effects_;
};
