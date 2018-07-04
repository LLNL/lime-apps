#ifndef _PULSE_H
#define _PULSE_H

#include "systemc.h"

template <bool polarity>
SC_MODULE(pulse)
{
	sc_in<bool> clk;
	sc_out<bool> sig;

	void ct_pulse()
	{
		sig = !polarity;
		wait(1);
		sig = polarity;
		wait(2);
		sig = !polarity;
	}

	SC_CTOR(pulse)
	{
		SC_CTHREAD(ct_pulse, clk.pos());
	}
};

#endif // _PULSE_H
