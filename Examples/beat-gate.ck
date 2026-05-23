SawOsc osc => LPF filter => Gain gate => dac;

0.35 => filter.Q;

while (true)
{
    80.0 + hostParam1 * 640.0 => osc.freq;
    300.0 + hostParam2 * 3600.0 => filter.freq;

    if (hostBeat % 1.0 < 0.5)
        hostParamGain => gate.gain;
    else
        0.0 => gate.gain;

    8::ms => now;
}
