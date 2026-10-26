#pragma once
#include <cstdint>

// Audio-thread-owned sample-and-hold random values, independent of INDEX/noise.
class RandomVoiceControls {
 public:
  void update(int pitch,int speed,bool trigger) {
    if(pitch==51) {
      if(trigger || last_pitch_!=51) pitch_=-24+int(next(pitch_rng_)%75);
    } else pitch_=pitch;
    if(speed==251) {
      if(trigger || last_speed_!=251) speed_=25+int(next(speed_rng_)%226);
    } else speed_=speed;
    last_pitch_=pitch;last_speed_=speed;
  }
  int pitch() const {return pitch_;}
  int speed() const {return speed_;}
 private:
  static uint32_t next(uint32_t& s) {s^=s<<13;s^=s>>17;s^=s<<5;return s;}
  uint32_t pitch_rng_=0x50495443u,speed_rng_=0x53504545u;
  int pitch_=0,speed_=100,last_pitch_=0,last_speed_=100;
};
