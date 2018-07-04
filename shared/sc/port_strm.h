
#ifndef _PORT_STRM_H
#define _PORT_STRM_H

#include <iomanip> // hex, setw
#include "systemc.h"

// TODO: move somewhere else?
#define RLEVEL true

// control stream
#define CR 8  // number of registers
#define CA 3  // RACC address width
#define CD 32 // data width
#define CU 8  // user width
#define CI 4  // id width

// data stream
#if 1
#define DD 64 // data width
#define DU 1  // user width
#define DI 4  // id width
#else
#define DD 32 // data width
#define DU 1  // user width
#define DI 4  // id width
#endif

// AXI-Stream Interface

#if defined(__SYNTHESIS__)
#define RACC_T   RACC::ui_t
#define AXI_TC_T AXI_TC::ui_t
#define AXI_TD_T AXI_TD::ui_t
#else
#define RACC_T   RACC
#define AXI_TC_T AXI_TC
#define AXI_TD_T AXI_TD
#endif

#define RACC_CONCAT \
	(go,wr,sel,len)

class RACC { // register access command
public:
	typedef sc_uint<(2*CA+1+1)> ui_t;

	sc_uint<CA> len;
	sc_uint<CA> sel;
	sc_uint<1> wr ;
	sc_uint<1> go ;

	RACC(ui_t ui = 0)
	{
		RACC_CONCAT = ui;
	}

	RACC & operator=(ui_t ui)
	{
		RACC_CONCAT = ui;
		return *this;
	}

	operator ui_t() const
	{
		return RACC_CONCAT;
	}
};

inline std::ostream & operator<< (std::ostream &o, const RACC &flit)
{
	return o << hex << flit.go << ':' << flit.wr << ':' << flit.sel << ':' << flit.len;
}

#define AXI_TC_CONCAT \
	(tlast,tuser.go,tuser.wr,tuser.sel,tuser.len,tdest,tid,tdata)

class AXI_TC {
public:
	typedef sc_biguint<(CD+2*CI+CU+1)> ui_t;

	sc_biguint<CD>    tdata;
	sc_uint<CI>       tid  ;
	sc_uint<CI>       tdest;
//	sc_uint<(CD+7)/8> tkeep;
//	sc_uint<(CD+7)/8> tstrb;
	RACC              tuser;
	sc_uint<1>        tlast;

	AXI_TC(ui_t ui = 0)
	{
		AXI_TC_CONCAT = ui;
	}

	AXI_TC & operator=(ui_t ui)
	{
		AXI_TC_CONCAT = ui;
		return *this;
	}

	operator ui_t() const
	{
		return AXI_TC_CONCAT;
	}
};

inline std::ostream & operator<< (std::ostream &o, const AXI_TC &flit)
{
	return o << hex << flit.tlast << ':' << flit.tuser << ':' << flit.tdest << ':' << flit.tid << ':' << flit.tdata.to_uint();
}

#define AXI_TD_CONCAT \
	(tlast,tuser,tkeep,tdest,tid,tdata)

class AXI_TD {
public:
	typedef sc_biguint<(DD+2*DI+((DD+7)/8)+DU+1)> ui_t;

	sc_biguint<DD>    tdata;
	sc_uint<DI>       tid  ;
	sc_uint<DI>       tdest;
	sc_uint<(DD+7)/8> tkeep;
//	sc_uint<(DD+7)/8> tstrb;
	sc_uint<DU>       tuser;
	sc_uint<1>        tlast;

	AXI_TD(ui_t ui = 0)
	{
		AXI_TD_CONCAT = ui;
	}

	AXI_TD & operator=(ui_t ui)
	{
		AXI_TD_CONCAT = ui;
		return *this;
	}

	operator ui_t() const
	{
		return AXI_TD_CONCAT;
	}
};

inline std::ostream & operator<< (std::ostream &o, const AXI_TD &flit)
{
	return o << hex << flit.tlast << ':' << flit.tuser << ':' << flit.tkeep << ':'  << flit.tdest << ':' << flit.tid << ':' << flit.tdata;
}

#endif // _PORT_STRM_H
