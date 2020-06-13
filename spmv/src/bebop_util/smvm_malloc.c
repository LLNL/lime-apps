/**
 * @file smvm_malloc.c
 * @author Mark Hoemmen
 * @date 01 Mar 2006
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
 *************************************************************************/
#include <smvm_malloc.h>
#include <smvm_util.h>

/*
 * FIXME: POSIX specs say that systems are not even required to
 * support malloc anymore!!!  They should be in stdlib.h instead.  But
 * some older systems may look for them in malloc.h.  This should be a
 * configuration check.
 */
/* #include <malloc.h> */    /* malloc, realloc */
#include <stdio.h>     /* the usual output stuff */
#include <stdlib.h>    /* exit(), malloc(), realloc() */
#include <string.h>    /* memset() */
#include "config.h"
#include "alloc.h"     /* NALLOC(), NFREE() */


void*
mfh_malloc (size_t size, char* srcfile, int linenum)
{
  void *ptr = NALLOC(char, size);

  if ((ptr == NULL) && (size > 0))
    {
      fprintf (stderr, "*** ERROR:  at line %d of %s:  failed to "
	       "malloc %lu bytes! ***\n", linenum, srcfile, 
	       (unsigned long) size);
      smvm_exit (EXIT_FAILURE);
    }
  return ptr;
}


void*
mfh_calloc (size_t num_elements, size_t element_size, char* srcfile, 
	   int linenum)
{
  void *ptr = NALLOC(char, num_elements*element_size);
  /* 
   * If we tried to allocate non-zero amount of memory and it failed,
   * abort with an error message.
   */
  if ((ptr == NULL) && (num_elements > 0) && (element_size > 0))
    {
      fprintf (stderr, "*** ERROR:  at line %d of %s:  failed to "
	       "calloc %lu element array of %lu byte elements ***\n", 
	       linenum, srcfile, (unsigned long) num_elements, 
	       (unsigned long) element_size);
      smvm_exit (EXIT_FAILURE);
    }
  memset(ptr, 0, num_elements*element_size);
  return ptr;
}


void*
mfh_realloc (void* ptr, size_t size, char* srcfile, int linenum)
{
  ptr = NULL; // realloc(ptr, size); // TODO: need aligned version

  if ((ptr == NULL) && (size > 0))
    {
      fprintf (stderr, "ERROR:  at line %d of %s:  failed to reallocate "
	       "%lu bytes!\n", linenum, srcfile, (unsigned long) size);
      smvm_exit (EXIT_FAILURE);
    }

  return ptr;
}


void
mfh_free (void* ptr, char* srcfile, int linenum)
{
  NFREE(ptr);
}

