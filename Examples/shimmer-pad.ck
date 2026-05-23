SinOsc low => Gain lowGain => Gain master => dac;
TriOsc mid => Gain midGain => master;
TriOsc high => Gain highGain => master;
SinOsc lfo => blackhole;

0.05 => lfo.freq;

while (true)
{
    42.0 + hostParam1 * 18.0 => float root;
    Std.mtof (root) => float base;
    base => low.freq;
    base * 1.501 => mid.freq;
    base * 2.006 + lfo.last() * (2.0 + hostParam2 * 18.0) => high.freq;

    hostParamGain * 0.42 => master.gain;
    0.55 => lowGain.gain;
    0.25 + hostParam2 * 0.2 => midGain.gain;
    0.12 + hostParam3 * 0.28 => highGain.gain;
    5::ms => now;
}
