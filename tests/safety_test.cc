#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <climits>
#include <memory>
#include <vector>
#include "synth.h"

// Host-only adversarial API test; never part of the drumlogue unit.
int main() {
  auto synth=std::unique_ptr<Synth>(new Synth());
  unit_runtime_desc_t desc={};desc.samplerate=48000;desc.output_channels=2;
  assert(synth->Init(&desc)==k_unit_err_none);
  uint32_t rng=0x17105100u;
  auto next=[&](){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;};
  const int32_t hostile[]={INT_MIN,-100000,-1,0,1,50,101,251,100000,INT_MAX};
  const unsigned sizes[]={0,1,17,64,127,256,1024};
  for(unsigned iteration=0;iteration<3000;++iteration) {
    uint8_t id=uint8_t(next());int32_t value=hostile[next()%10];
    synth->setParameter(id,value);
    (void)synth->getParameterValue(id);
    (void)synth->getParameterStrValue(id,value);
    synth->SetTempo(next());synth->PitchBend(uint16_t(next()));
    switch(next()%9) {
      case 0:synth->NoteOn(uint8_t(next()),uint8_t(next()));break;
      case 1:synth->NoteOff(uint8_t(next()));break;
      case 2:synth->GateOn(uint8_t(next()));break;
      case 3:synth->GateOff();break;
      case 4:synth->Reset();break;
      case 5:synth->Suspend();break;
      case 6:synth->Resume();break;
      case 7:synth->AllNoteOff();break;
      default:break;
    }
    unsigned count=sizes[next()%7];
    std::vector<float> buffer(count*2+32,12345.f);
    synth->Render(buffer.data()+16,count);
    for(unsigned i=0;i<16;++i)assert(buffer[i]==12345.f && buffer[count*2+16+i]==12345.f);
    for(unsigned i=16;i<count*2+16;++i)assert(std::isfinite(buffer[i]) && std::fabs(buffer[i])<=.951f);
  }
  synth->AllNoteOff();float tail[512]={};synth->Render(tail,256);
  for(float value:tail)assert(value==0);
  std::puts("PASS: 3000 adversarial control/event cases; bounded finite stereo; guarded render buffers");
}
