/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011-2013 Data Differential, http://datadifferential.com/
 *  All rights reserved.
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

/*
  @note This header is internal, and should not be used by external programs.
*/

#pragma once

#include <stdexcept>

namespace org { namespace gearmand { namespace libgearman {

class Worker {
public:
  Worker()
  {
    _worker= gearman_worker_create(NULL);

    if (_worker == NULL)
    {
      throw std::runtime_error("gearman_worker_create() failed");
    }
  }

  Worker(in_port_t arg)
  {
    _worker= gearman_worker_create(NULL);

    if (_worker == NULL)
    {
      throw std::runtime_error("gearman_worker_create() failed");
    }
    gearman_worker_add_server(_worker, "localhost", arg);
  }

  gearman_worker_st* operator&() const
  { 
    return _worker;
  }

  gearman_worker_st* operator->() const
  { 
    return _worker;
  }

  ~Worker()
  {
    gearman_worker_free(_worker);
  }

private:
  gearman_worker_st *_worker;

};

} /* namespace libgearman */ } /* namespace gearmand */ } /* namespace org */ 
