SinOsc carrier => Gain out => dac;
SinOsc vibrato => blackhole;

5.0 => vibrato.freq;

while (true)
{
    110.0 + hostParam1 * 880.0 => float base;
    hostParam2 * 14.0 => float depth;

    base + vibrato.last() * depth => carrier.freq;
    hostParamGain * (0.4 + hostParam3 * 0.6) => out.gain;

    1::samp => now;
}
