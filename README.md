# StupiSaw

StupiSaw is a stupid saw wave synth that I am using to make sure I understand the basics of the
CLAP API (https://github.com/free-audio/clap.git) and perhaps help others navigate it also.

To build it

```shell
mkdir ignore
cmake -Bignore/build
cmake --build ignore/build
```

should do it on mac and linux. Haven't tested on windows yet.

The synth is a (badly implemented) unison saw synth. It has 8 parameters

- unison count and detune control the unison
- amp attach and release run the simple ASR envelope for the amplitude
- cutoff and resonance control the per-voice biquad LPF 
- filterDecay and filter ModDepth controls a simple decay-from-1-to-0 envelope applied to cutoff
  at depth moddepth
  
No attempt is made to oversample, reduce aliasing, make self resonant filters, or any such thing.
That's not the point. It makes sound and responds to parameters was the goal here. 

No gui yet.

Questions either as github issues or on one of the discords. PRs welcomed.