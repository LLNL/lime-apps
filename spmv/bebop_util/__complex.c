/**
 * @file __complex.c
 * @author mfh
 * @since 2 June 2005
 *
 * Replaces "double _Complex" for compilers not supporting the C99 standard.
 *
 * Copyright (c) 2006, Regents of the University of California 
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the
 * following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in 
 *   the documentation and/or other materials provided with the 
 *   distribution. 
 *
 * * Neither the name of the University of California, Berkeley, nor
 *   the names of its contributors may be used to endorse or promote
 *   products derived from this software without specific prior
 *   written permission.  
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <__complex.h>

#ifndef HAVE__COMPLEX

#include <math.h>

#ifndef MAX
#  define MAX(a,b)  (((a)>(b))? (a) : (b))
#endif /* NOT MAX */

const double_Complex double_Complex_I = {0.0, 1.0};
const double_Complex double_Complex_ZERO = {0.0, 0.0};

double_Complex 
double_Complex_add (const double_Complex a, const double_Complex b)
{
  double_Complex c;
  c.real = a.real + b.real;
  c.imag = a.imag + b.imag;
  return c;
}

double_Complex 
double_Complex_negate (const double_Complex a)
{
  double_Complex c;
  c.real = -(a.real);
  c.imag = -(a.imag);
  return c;
}

double_Complex 
double_Complex_multiply (const double_Complex a, const double_Complex b)
{
  double_Complex c;
  c.real = a.real * b.real - a.imag * b.imag;
  c.imag = a.real * b.imag + a.imag * b.real;
  return c;
}

int 
double_Complex_not_equal (const double_Complex a, const double_Complex b)
{
  return !(a.real == b.real && a.imag == b.imag);  
}

int 
double_Complex_equal (const double_Complex a, const double_Complex b)
{
  return (a.real == b.real && a.imag == b.imag);  
}

double_Complex
new_double_Complex (const double a, const double b)
{
  double_Complex z;
  z.real = a;
  z.imag = b;
  return z;
}

double 
double_Complex_real_part (const double_Complex a)
{
  return a.real;
}

double 
double_Complex_imag_part (const double_Complex a)
{
  return a.imag;
}

double_Complex 
double_Complex_conj (const double_Complex a)
{
  double_Complex c;
  c.real = a.real;
  c.imag = -(a.imag);
  return c;
}

double
double_Complex_cabs (const double_Complex a)
{
  /* We scale the computation to avoid overflow. */
  double re = double_Complex_real_part (a);
  double im = double_Complex_imag_part (a);

  double d = MAX( fabs(re), fabs(im) );
  if (d == 0.0)
    return d;
  else
    {
      re = re / d;
      im = im / d;
      return d * sqrt (re*re + im*im);
    }
}

#endif /* HAVE__COMPLEX */

