#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "lpc_bank.h"

// All mutable DSP state is owned by the render thread. 8kHz synthesis / 48kHz output.
class LpcEngine {
 public:
  void reset() {
    active_=false; releasing_=false; pos_=phase_=deemph_=hp_x_=hp_y_=0;
    a_=b_=0; sub_=0; age_=0; env_=0; was_voiced_=false;
    last_=tail_=0; tail_left_=0;
    grain_age_=gap_age_=0; stop_grain_=false;
    for (auto& v: state_) v=0;
    rng_=5220;
  }
  void trigger(unsigned word, unsigned note, unsigned velocity) {
    // Preserve the previous output briefly to soften retrigger discontinuities.
    float previous=last_;
    reset();
    word_=std::min(word,kWordCount-1); note_=std::min(note,127u);
    velocity_=std::min(velocity,127u)/127.f;
    active_=velocity!=0;
    if(index_>=0) seekIndex();
    tail_=previous; tail_left_=240;
  }
  void release() { if(active_) {if(index_==101)stop_grain_=true;else releasing_=true;} }
  void setStutterTime(int ms) {grain_ms_=std::max(20,std::min(ms,250));}
  void setStutterTempo(uint32_t tempo) {
    // One sixteenth-note rest, at the 8 kHz synthesis rate. Independent of SPEED.
    if(tempo>=65536u && tempo<=1000u*65536u)
      gap_ticks_=unsigned(std::round(120000.f*65536.f/tempo));
  }
  void controls(int pitch, int speed, int index, int level, int bend=8192) {
    speed_=std::max(25,std::min(speed,250))*.01f;
    hz_=130.f*std::pow(2.f,(int(note_)-60+std::max(-24,std::min(pitch,50))+(bend-8192)*(2.f/8192.f))/12.f);
    hz_=std::max(32.f,std::min(hz_,3500.f));
    int selected=std::max(-1,std::min(index,101));
    if(selected!=index_) {
      if(stop_grain_) {releasing_=true;stop_grain_=false;}
      index_=selected;
      if(active_ && index_>=0) seekIndex();
    }
    level_=std::max(0,std::min(level,100))*.01f;
  }
  float render() {
    float value=0;
    if(active_) {
      if(sub_==0) { a_=b_; b_=tick(); }
      value=a_+(b_-a_)*(sub_/6.f);
      sub_=(sub_+1)%6;
      if(releasing_) env_=std::max(0.f,env_-1.f/960.f);
      else env_=std::min(1.f,env_+1.f/144.f);
      // +18 dB source gain, then soft peak control and LEVEL.
      value*=env_*velocity_*7.94328235f;
      float magnitude=std::fabs(value);
      if(magnitude>.75f) {
        float excess=magnitude-.75f;
        value=std::copysign(.75f+.2f*excess/(.2f+excess),value);
      }
      value*=level_;
      if(releasing_ && env_<=0) { active_=false; a_=b_=0; }
    }
    if(tail_left_>0) { value+=tail_*(tail_left_/240.f); --tail_left_; }
    // Final bounded output even at extreme live parameter combinations.
    last_=std::max(-.95f,std::min(value,.95f));
    return last_;
  }
  bool active() const {return active_;}
  float position() const {return pos_;}
  float pitchHz() const {return hz_;}
 private:
  void seekIndex() {
    const auto& w=kWords[word_];
    const float stop=std::min(w.offset+w.samples,float(w.count*80));
    unsigned first=std::min(unsigned(w.offset/80.f),unsigned(w.count-1));
    unsigned last=std::min(unsigned(std::max(w.offset,stop-1)/80.f),unsigned(w.count-1));
    auto levelAt=[&](unsigned j) {
      float source=std::max(w.offset,j*80.f);
      unsigned a=std::min(unsigned(source/80.f),unsigned(w.count-1));
      unsigned b=std::min(a+1,unsigned(w.count-1));float t=source/80.f-a;
      return w.frames[a].rms*(1-t)+w.frames[b].rms*t;
    };
    float peak=0;unsigned strongest=first;
    for(unsigned j=first;j<=last;++j) if(levelAt(j)>peak){peak=levelAt(j);strongest=j;}
    float threshold=std::max(peak*.2f,.00015f);unsigned candidates=0;
    for(unsigned j=first;j<=last;++j)if(levelAt(j)>=threshold)++candidates;
    unsigned rank=0;
    if(index_<=100)rank=unsigned(std::round(index_*.01f*(candidates?candidates-1:0)));
    else{random_^=random_<<13;random_^=random_>>17;random_^=random_<<5;rank=candidates?random_%candidates:0;}
    unsigned chosen=strongest;
    for(unsigned j=first;j<=last;++j)if(levelAt(j)>=threshold){if(rank==0){chosen=j;break;}--rank;}
    pos_=std::max(0.f,std::min(float(w.samples-1),chosen*80.f-w.offset));
    grain_start_=pos_;grain_age_=gap_age_=0;
  }

  float tick() {
    const auto& w=kWords[word_];
    float grainGain=1;
    if(index_==101) {
      if(releasing_)return 0;
      float end=std::min(float(w.samples),w.count*80.f-w.offset);
      // Keep a fixed source interval per loop; SPEED controls its traversal rate.
      float span=std::min(grain_ms_*8.f,std::max(1.f,end-grain_start_));
      unsigned duration=std::max(1u,unsigned(std::ceil(span/speed_)));
      if(grain_age_>=duration) {
        if(stop_grain_) {releasing_=true;return 0;}
        if(gap_age_<gap_ticks_) {++gap_age_;return 0;}
        grain_age_=gap_age_=0;
      }
      pos_=std::min(end-1.f,grain_start_+grain_age_*speed_);
      float fade=std::max(1.f,std::min(24.f,duration*.25f));
      grainGain=duration<4?1.f:std::min(1.f,std::min(grain_age_/fade,(duration-1-grain_age_)/fade));
      ++grain_age_;
    }
    if(pos_>=w.samples) { releasing_=true; return 0; }
    float sourcePos=pos_+w.offset;
    if(sourcePos>=w.count*80.f) {if(index_<0 && !releasing_)pos_+=speed_;return 0;}
    unsigned index=std::min(unsigned(sourcePos/80.f),unsigned(w.count-1));
    unsigned next=std::min(index+1,unsigned(w.count-1));
    float alpha=sourcePos/80.f-index;
    const auto& f=w.frames[index]; const auto& n=w.frames[next];
    float k[10], product=1;
    const int levels[10]={31,31,15,15,15,15,15,7,7,7};
    for(unsigned j=0;j<10;++j) {
      float raw=(f.k[j]*(1-alpha)+n.k[j]*alpha)/32767.f;
      float q=std::round((raw+1)*levels[j]*.5f)*2/levels[j]-1;
      k[j]=std::max(-.96f,std::min(.96f,q));
      product*=1-k[j]*k[j];
    }
    float rms=f.rms*(1-alpha)+n.rms*alpha;
    rms=std::expm1(std::round(std::log1p(rms*150.f)/std::log(151.f)*31.f)/31.f*std::log(151.f))/150.f;
    bool voiced=f.voiced!=0;
    float excitation=0;
    if(voiced) {
      if(!was_voiced_) phase_=1;
      phase_+=hz_/8000.f;
      if(phase_>=1) {phase_-=std::floor(phase_); excitation=std::sqrt(8000.f/hz_);}
    } else {
      // Deterministic white noise with unit variance (not the pilot's Gaussian sequence).
      rng_^=rng_<<13; rng_^=rng_>>17; rng_^=rng_<<5;
      excitation=(int32_t(rng_>>8)-8388608)*(1.7320508f/8388608.f);
    }
    was_voiced_=voiced;
    float value=excitation*rms*std::sqrt(std::max(product,1e-12f));
    // Descending update leaves state[m-1] from the previous sample intact.
    for(int m=9;m>=0;--m) {
      value-=k[m]*state_[m];
      if(m<9) state_[m+1]=state_[m]+k[m]*value;
    }
    state_[0]=value;
    deemph_=value+.85f*deemph_;
    float hp=deemph_-hp_x_+.9428f*hp_y_;
    hp_x_=deemph_; hp_y_=hp;
    if(index_<0 && !releasing_) pos_+=speed_;
    ++age_;
    return hp*w.gain*grainGain;
  }
  float state_[10]={};
  float pos_=0,phase_=0,deemph_=0,hp_x_=0,hp_y_=0,a_=0,b_=0,env_=0;
  float speed_=1,hz_=130,level_=.8f,velocity_=1,last_=0,tail_=0;
  unsigned word_=0,note_=60,sub_=0,age_=0; int index_=-1,tail_left_=0;
  uint32_t rng_=5220,random_=0x52205100u;
  float grain_start_=0;int grain_ms_=80;unsigned grain_age_=0,gap_age_=0,gap_ticks_=1000;bool stop_grain_=false;
  bool active_=false,releasing_=false,was_voiced_=false;
};
