TriOsc osc => ADSR env => Gain master => dac;

env.set (6::ms, 35::ms, 0.35, 80::ms);

[0, 4, 7, 12, 7, 4] @=> int intervals[];
48 => int root;
0 => int step;

while (true)
{
    Math.max(0.0, Math.min(hostParamGain, 1.0)) => master.gain;
    Std.mtof (root + intervals[step]) => osc.freq;
    env.keyOn();

    (60000.0 / Math.max(20.0, hostTempo) / 4.0)::ms => dur sixteenth;
    sixteenth * 0.55 => now;

    env.keyOff();
    sixteenth * 0.45 => now;

    (step + 1) % intervals.size() => step;
}
