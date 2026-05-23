SawOsc bass => LPF bassFilter => Gain bassGain => Gain master => dac;
TriOsc chime => ADSR chimeEnv => Gain chimeGain => master;
Noise hat => BPF hatFilter => ADSR hatEnv => Gain hatGain => master;

0.8 => bassFilter.Q;
5200.0 => hatFilter.freq;
7.0 => hatFilter.Q;
chimeEnv.set (5::ms, 60::ms, 0.18, 160::ms);
hatEnv.set (1::ms, 18::ms, 0.0, 12::ms);

[0, 3, 7, 10, 12, 10, 7, 3] @=> int tones[];
0 => int step;

while (true)
{
    36.0 + hostParam1 * 24.0 => float root;
    Std.mtof (root + tones[step]) => bass.freq;
    Std.mtof (root + 24 + tones[(step * 3) % tones.size()]) => chime.freq;
    180.0 + hostParam2 * 4200.0 => bassFilter.freq;
    hostParamGain * 0.75 => master.gain;
    0.35 + hostParam3 * 0.55 => bassGain.gain;
    0.18 + hostParam2 * 0.22 => chimeGain.gain;
    0.11 => hatGain.gain;

    chimeEnv.keyOn();
    if (step % 2 == 0) hatEnv.keyOn();

    (60000.0 / Math.max(20.0, hostTempo) / 4.0)::ms => dur tick;
    tick * 0.45 => now;

    chimeEnv.keyOff();
    hatEnv.keyOff();
    tick * 0.55 => now;

    (step + 1) % tones.size() => step;
}
