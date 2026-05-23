SinOsc kick => ADSR kickEnv => Gain kickGain => Gain master => dac;
SawOsc bass => LPF bassFilter => ADSR bassEnv => Gain bassGain => master;
TriOsc padA => Gain padGain => master;
TriOsc padB => padGain;
SqrOsc lead => ADSR leadEnv => LPF leadFilter => Gain leadGain => master;
Noise hat => BPF hatFilter => ADSR hatEnv => Gain hatGain => master;

kickEnv.set (1::ms, 42::ms, 0.0, 24::ms);
bassEnv.set (5::ms, 70::ms, 0.34, 95::ms);
leadEnv.set (3::ms, 42::ms, 0.18, 120::ms);
hatEnv.set (1::ms, 14::ms, 0.0, 10::ms);

0.7 => bassFilter.Q;
0.5 => leadFilter.Q;
6200.0 => hatFilter.freq;
8.0 => hatFilter.Q;

[0, 0, 7, 0, 10, 0, 7, 0, 3, 3, 10, 3, 7, 5, 3, 0] @=> int bassLine[];
[12, 15, 19, 22, 24, 22, 19, 15, 17, 20, 24, 27, 29, 27, 24, 20] @=> int melody[];
[0, 3, 7, 10] @=> int chordA[];
[5, 9, 12, 15] @=> int chordB[];

0 => int step;

while (true)
{
    36.0 + hostParam1 * 12.0 => float root;
    (step / 8) % 2 => int chordSide;
    if (chordSide == 0)
    {
        Std.mtof (root + chordA[0] + 12) => padA.freq;
        Std.mtof (root + chordA[2] + 12) => padB.freq;
    }
    else
    {
        Std.mtof (root + chordB[0] + 7) => padA.freq;
        Std.mtof (root + chordB[2] + 7) => padB.freq;
    }

    Std.mtof (root + bassLine[step]) => bass.freq;
    160.0 + hostParam2 * 3600.0 => bassFilter.freq;
    700.0 + hostParam2 * 5200.0 => leadFilter.freq;
    hostParamGain * 0.82 => master.gain;
    0.34 + hostParam3 * 0.22 => bassGain.gain;
    0.16 + hostParam2 * 0.18 => padGain.gain;
    0.18 + hostParam3 * 0.26 => leadGain.gain;
    0.08 => hatGain.gain;

    bassEnv.keyOn();
    if (step == 0 || step == 8)
    {
        74.0 => kick.freq;
        0.82 => kickGain.gain;
        kickEnv.keyOn();
    }
    if (step % 2 == 1) hatEnv.keyOn();
    if (step % 4 == 2 || step % 8 == 7)
    {
        Std.mtof (root + melody[step]) => lead.freq;
        leadEnv.keyOn();
    }

    (60000.0 / Math.max(20.0, hostTempo) / 4.0)::ms => dur sixteenth;
    sixteenth * 0.45 => now;

    bassEnv.keyOff();
    kickEnv.keyOff();
    hatEnv.keyOff();
    leadEnv.keyOff();
    sixteenth * 0.55 => now;

    (step + 1) % 16 => step;
}
