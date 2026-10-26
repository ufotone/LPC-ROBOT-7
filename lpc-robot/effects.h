#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

inline float syncedDelayMs(unsigned division,uint32_t tempo) {
  static const float beats[]={.125f,1.f/6,.25f,1.f/3,.375f,.5f,.75f,1.f,1.5f,2.f};
  return 60000.f*beats[std::min(division,9u)]/std::max(tempo/65536.f,1.f);
}

// Equal-level left/right echo pairs. Mono input, explicitly stereo output.
// A shared mono ring uses the same 2 MiB as the previous stereo delay.
class RobotEffects {
 public:
  void reset() {write_=valid_=0;mix_=0;time_=target_time_;feedback_=target_feedback_;}
  void controls(int mix,float time,int feedback) {
    target_mix_=std::max(0,std::min(mix,100))*.01f;
    target_time_=std::max(5.f,std::min(time,4000.f))*48.f;
    target_feedback_=std::max(0,std::min(feedback,75))*.01f;
  }
  void process(float mono,float& left,float& right) {
    mix_+=(target_mix_-mix_)*.001f;
    time_+=(target_time_-time_)*.001f;
    feedback_+=(target_feedback_-feedback_)*.001f;
    const float dl=read(time_),dr=read(time_*2.f);
    float value=mono+dr*feedback_;
    buffer_[write_]=std::fabs(value)>1e-12f?value:0;
    // At high settings, reduce the centered source so the sides remain distinct.
    float dry=mono*(1.f-.65f*mix_);
    left=dry+dl*mix_;right=dry+dr*mix_;
    write_=(write_+1)&524287;
    if(valid_<524288)++valid_;
  }
 private:
  float read(float delay) const {
    if(valid_<unsigned(std::ceil(delay)))return 0;
    float p=write_-delay;if(p<0)p+=524288;
    unsigned i=unsigned(p);float fraction=p-i;
    return buffer_[i]+(buffer_[(i+1)&524287]-buffer_[i])*fraction;
  }
  float buffer_[524288]={};
  unsigned write_=0,valid_=0;
  float mix_=0,time_=12000,feedback_=.35f;
  float target_mix_=0,target_time_=12000,target_feedback_=.35f;
};
