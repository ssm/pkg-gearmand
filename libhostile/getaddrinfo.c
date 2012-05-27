/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Data Differential's libhostle
 *
 *  Copyright (C) 2012 Data Differential, http://datadifferential.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <config.h>

/*
  Random getaddrinfo failing library for testing getaddrinfo() failures.
  LD_PRELOAD="/usr/lib/libdl.so ./util/libhostile_getaddrinfo.so" ./binary
*/

#include <dlfcn.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <libhostile/initialize.h>

__thread bool is_in_getaddrinfo= 0;

static int not_until= 500;

static struct function_st __function;

static pthread_once_t function_lookup_once = PTHREAD_ONCE_INIT;
static void set_local(void)
{
  __function= set_function("getaddrinfo", "HOSTILE_GETADDRINFO");
}

bool is_getaddrinfo(void)
{
  return is_in_getaddrinfo;
}

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res)
{

  hostile_initialize();

  (void) pthread_once(&function_lookup_once, set_local);

  if (__function.frequency)
  {
    if (--not_until < 0 && random() % __function.frequency)
    {
#if 0
      perror("HOSTILE CLOSE() of socket during getaddrinfo()");
#endif
      return EAI_FAIL;
    }
  }

  is_in_getaddrinfo= true;
  int ret= __function.function.getaddrinfo(node, service, hints, res);
  is_in_getaddrinfo= false;

  return ret;
}


