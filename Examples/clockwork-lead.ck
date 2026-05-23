SqrOsc lead => ADSR env => LPF filter => Gain master => dac;

env.set (2::ms, 30::ms, 0.28, 65::ms);
0.45 => filter.Q;

[0, 2, 7, 9, 12, 14, 12, 7, 5, 9, 7, 2] @=> int notes[];
0 => int step;

while (true)
{
    48.0 + hostParam1 * 24.0 => float root;
    Std.mtof (root + notes[step]) => lead.freq;
    550.0 + hostParam2 * 5200.0 => filter.freq;
    hostParamGain * (0.35 + hostParam3 * 0.45) => master.gain;

    env.keyOn();
    (60000.0 / Math.max(20.0, hostTempo) / 8.0)::ms => dur thirtySecond;
    thirtySecond * (1.0 + hostParam3 * 3.0) => now;
    env.keyOff();
    thirtySecond => now;

    (step + 1) % notes.size() => step;
}
