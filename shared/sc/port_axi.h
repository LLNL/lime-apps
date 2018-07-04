
#ifndef _PORT_AXI_H
#define _PORT_AXI_H

#include <iomanip> // hex, setw
#include "systemc.h"

#if defined(__SYNTHESIS__)
#define AXI_AW_T AXI_AW::ui_t
#define AXI_W_T  AXI_W::ui_t
#define AXI_B_T  AXI_B::ui_t
#define AXI_AR_T AXI_AR::ui_t
#define AXI_R_T  AXI_R::ui_t
#else
#define AXI_AW_T AXI_AW
#define AXI_W_T  AXI_W
#define AXI_B_T  AXI_B
#define AXI_AR_T AXI_AR
#define AXI_R_T  AXI_R
#endif

// AXI-Full Interface

#define AXI_AW_CONCAT \
	(awprot,awcache,awlock,awburst,awsize,awlen,awaddr,awid)

class AXI_AW
{
public:
	typedef sc_biguint<(MI+MA+8+3+2+1+4+3)> ui_t;

	sc_uint<MI> awid    ;
	sc_uint<MA> awaddr  ;
	sc_uint<8>  awlen   ;
	sc_uint<3>  awsize  ;
	sc_uint<2>  awburst ;
	sc_uint<1>  awlock  ;
	sc_uint<4>  awcache ;
	sc_uint<3>  awprot  ;
//	sc_uint<4>  awqos   ;
//	sc_uint<4>  awregion;
//	sc_uint<MU> awuser  ;

	AXI_AW(ui_t ui = 0)
	{
		AXI_AW_CONCAT = ui;
	}

	AXI_AW & operator=(ui_t ui)
	{
		AXI_AW_CONCAT = ui;
		return *this;
	}

	operator ui_t() const
	{
		return AXI_AW_CONCAT;
	}
};

inline std::ostream & operator<< (std::ostream &o, const AXI_AW &flit)
{
	return o << flit;
}

#define AXI_W_CONCAT \
	(wlast,wstrb,wdata)

class AXI_W
{
public:
	typedef sc_biguint<(MD+((MD+7)/8)+1)> ui_t;

//	sc_uint<MI>       wid  ;
	sc_biguint<MD>    wdata;
	sc_uint<(MD+7)/8> wstrb;
	sc_uint<1>        wlast;
//	sc_uint<MU>       wuser;

	AXI_W(ui_t ui = 0)
	{
		AXI_W_CONCAT = ui;
	}

	AXI_W & operator=(ui_t ui)
	{
		AXI_W_CONCAT = ui;
		return *this;
	}

	operator ui_t() const
	{
		return AXI_W_CONCAT;
	}
};

inline std::ostream & operator<< (std::ostream &o, const AXI_W &flit)
{
	return o << flit;
}

#define AXI_B_CONCAT \
	(bresp,bid)

class AXI_B
{
public:
	typedef sc_uint<(MI+2)> ui_t;

	sc_uint<MI> bid  ;
	sc_uint<2>  bresp;
//	sc_uint<MU> buser;

	AXI_B(ui_t ui = 0)
	{
		AXI_B_CONCAT = ui;
	}

	AXI_B & operator=(ui_t ui)
	{
		AXI_B_CONCAT = ui;
		return *this;
	}

	operator ui_t() const
	{
		return AXI_B_CONCAT;
	}
};

inline std::ostream & operator<< (std::ostream &o, const AXI_B &flit)
{
	return o << flit;
}

#define AXI_AR_CONCAT \
	(arprot,arcache,arlock,arburst,arsize,arlen,araddr,arid)

class AXI_AR
{
public:
	typedef sc_biguint<(MI+MA+8+3+2+1+4+3)> ui_t;

	sc_uint<MI> arid    ;
	sc_uint<MA> araddr  ;
	sc_uint<8>  arlen   ;
	sc_uint<3>  arsize  ;
	sc_uint<2>  arburst ;
	sc_uint<1>  arlock  ;
	sc_uint<4>  arcache ;
	sc_uint<3>  arprot  ;
//	sc_uint<4>  arqos   ;
//	sc_uint<4>  arregion;
//	sc_uint<MU> aruser  ;

	AXI_AR(ui_t ui = 0)
	{
		AXI_AR_CONCAT = ui;
	}

	AXI_AR & operator=(ui_t ui)
	{
		AXI_AR_CONCAT = ui;
		return *this;
	}

	operator ui_t() const
	{
		return AXI_AR_CONCAT;
	}
};

inline std::ostream & operator<< (std::ostream &o, const AXI_AR &flit)
{
	return o << flit;
}

#define AXI_R_CONCAT \
	(rlast,rresp,rdata,rid)

class AXI_R
{
public:
	typedef sc_biguint<(MI+MD+2+1)> ui_t;

	sc_uint<MI>    rid  ;
	sc_biguint<MD> rdata;
	sc_uint<2>     rresp;
	sc_uint<1>     rlast;
//	sc_uint<MU>    ruser;

	AXI_R(ui_t ui = 0)
	{
		AXI_R_CONCAT = ui;
	}

	AXI_R & operator=(ui_t ui)
	{
		AXI_R_CONCAT = ui;
		return *this;
	}

	operator ui_t() const
	{
		return AXI_R_CONCAT;
	}
};

inline std::ostream & operator<< (std::ostream &o, const AXI_R &flit)
{
	return o << flit;
}

#endif // _PORT_AXI_H
