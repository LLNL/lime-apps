#ifndef _enumerations_h
#define _enumerations_h
/**
 * @file enumerations.h
 * @author mfh
 * @since 26 May 2005
 *
 * An include file for all the enums and #defines that are globally useful.
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

/**
 * Enum for base of array indices:  either zero-based (C-style) or one-based 
 * (Fortran-style).
 */
enum
index_base_t {ZERO = 0, ONE = 1};

/**
 * Symmetry type of the matrix.  Unfortunately MatrixMarket doesn't have a 
 * skew-Hermitian type.
 */
enum
symmetry_type_t 
{ 
  UNSYMMETRIC = 0, SYMMETRIC, SKEW_SYMMETRIC, HERMITIAN 
};

/**
 * For matrices with a type of symmetry:  Where the elements of the matrix 
 * are actually stored: in the lower triangle or the upper triangle.
 */
enum
symmetric_storage_location_t
{
  UPPER_TRIANGLE, LOWER_TRIANGLE
};

/**
 * REAL means that the entries of the matrix are real-valued (IEEE 754 double-precision).
 * COMPLEX means that they are complex-valued (using C99 "double _Complex").
 * PATTERN means that no values are stored.
 */
enum
value_type_t
{
  REAL, COMPLEX, PATTERN
};

#endif /* _enumerations_h */
