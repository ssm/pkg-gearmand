/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
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



#include <config.h>

#include <libtest/test.hpp>

#include <cassert>
#include <cstring>
#include <memory>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>


#include <cstdio>

#include <tests/start_worker.h>
#include <util/instance.hpp>

using namespace libtest;
using namespace datadifferential;

#include <tests/worker.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#define CONTEXT_MAGIC_MARKER 45

struct context_st {
  struct worker_handle_st *handle;
  in_port_t port;
  gearman_worker_options_t options;
  gearman_function_t worker_fn;
  std::string namespace_key;
  std::string function_name;
  void *context;
  int magic;
  boost::barrier* _sync_point;

  context_st(worker_handle_st* handle_arg,
             gearman_function_t& arg,
             in_port_t port_arg,
             const std::string& namespace_key_arg,
             const std::string& function_name_arg,
             void *context_arg,
             gearman_worker_options_t& options_arg) :
    port(port_arg),
    handle(handle_arg),
    options(options_arg),
    worker_fn(arg),
    namespace_key(namespace_key_arg),
    function_name(function_name_arg),
    context(context_arg),
    _sync_point(handle_arg->sync_point()),
    magic(CONTEXT_MAGIC_MARKER)
  {
  }

  void wait(void)
  {
    if (_sync_point)
    {
      _sync_point->wait();
      _sync_point= NULL;
    }
  }

  void fail(void)
  {
    handle->failed_startup= true;
    handle->wait();
  }

  ~context_st()
  {
  }

};

static void thread_runner(context_st* con)
{
  std::auto_ptr<context_st> context(con);

  assert(context.get());
  if (context.get() == NULL)
  {
    Error << "context_st passed to function was NULL";
    return;
  }

  assert (context->magic == CONTEXT_MAGIC_MARKER);
  if (context->magic != CONTEXT_MAGIC_MARKER)
  {
    Error << "context_st had bad magic";
    return;
  }

  Worker worker;
  if (&worker == NULL)
  {
    Error << "Failed to create Worker";
    return;
  }

  assert(context->handle);
  if (context->handle == NULL)
  {
    Error << "Progammer error, no handle found";
    return;
  }
  context->handle->set_worker_id(&worker);

  if (context->namespace_key.empty() == false)
  {
    gearman_worker_set_namespace(&worker, context->namespace_key.c_str(), context->namespace_key.length());
  }

  if (gearman_failed(gearman_worker_add_server(&worker, NULL, context->port)))
  {
    Error << "gearman_worker_add_server()";
    return;
  }

  // Check for a working server by "asking" it for an option
  {
    size_t count= 5;
    bool success= false;
    while (--count and success == false)
    {
      success= gearman_worker_set_server_option(&worker, test_literal_param("exceptions"));
    }

    if (success == false)
    {
      Error << "gearman_worker_set_server_option() failed";
      return;
    }
  }

  if (gearman_failed(gearman_worker_define_function(&worker,
                                                    context->function_name.c_str(), context->function_name.length(),
                                                    context->worker_fn,
                                                    0, 
                                                    context->context)))
  {
    Error << "Failed to add function " << context->function_name << "(" << gearman_worker_error(&worker) << ")";
    return;
  }

  if (context->options != gearman_worker_options_t())
  {
    gearman_worker_add_options(&worker, context->options);
  }

  context->handle->wait();

  gearman_return_t ret= GEARMAN_SUCCESS;
  while (context->handle->is_shutdown() == false or ret != GEARMAN_SHUTDOWN)
  {
    ret= gearman_worker_work(&worker);
  }
}


worker_handle_st *test_worker_start(in_port_t port, 
                                    const char *namespace_key,
                                    const char *function_name,
                                    gearman_function_t &worker_fn,
                                    void *context_arg,
                                    gearman_worker_options_t options)
{
  worker_handle_st *handle= new worker_handle_st();
  fatal_assert(handle);

  context_st *context= new context_st(handle, worker_fn, port,
                                      namespace_key ? namespace_key : "",
                                      function_name,
                                      context_arg, options);
  fatal_assert(context);

  handle->_thread= new boost::thread(thread_runner, context);
  if (handle->_thread == NULL)
  {
    delete context;
    delete handle;

    return NULL;
  }

  handle->wait();

  return handle;
}

boost::barrier* worker_handle_st::sync_point()
{
  return &_sync_point;
}

void worker_handle_st::set_worker_id(gearman_worker_st* worker)
{
  _worker_id= gearman_worker_id(worker);
}

worker_handle_st::worker_handle_st() :
  failed_startup(false),
  _shutdown(false),
  _worker_id(gearman_id_t()),
  _sync_point(2)
{
}

worker_handle_st::~worker_handle_st()
{
  shutdown();
}

void worker_handle_st::wait()
{
  _sync_point.wait();
}

void worker_handle_st::set_shutdown()
{
  boost::mutex::scoped_lock(_shutdown_lock);
  _shutdown= true;
}

bool worker_handle_st::is_shutdown()
{
  boost::mutex::scoped_lock(_shutdown_lock);
  return _shutdown;
}


bool worker_handle_st::shutdown()
{
  if (is_shutdown())
  {
    return true;
  }

  set_shutdown();

  gearman_return_t rc;
  if (gearman_failed(rc=  gearman_kill(_worker_id, GEARMAN_KILL)))
  {
    Error << "failed to shutdown " << rc;
    return false;
  }

  _thread->join();
  delete _thread;

  return true;
}