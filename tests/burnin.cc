/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "gear_config.h"
#include <libtest/test.hpp>

using namespace libtest;

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <libgearman/gearman.h>

#include <libtest/test.hpp>

#include "libgearman/client.hpp"
using namespace org::gearmand;

#include <tests/start_worker.h>

#define DEFAULT_WORKER_NAME "burnin"

static gearman_return_t worker_fn(gearman_job_st*, void*)
{
  return GEARMAN_SUCCESS;
}

struct client_test_st {
  libgearman::Client _client;
  worker_handle_st *handle;

  client_test_st():
    _client(libtest::default_port()),
    handle(NULL)
  {
    gearman_function_t func_arg= gearman_function_create(worker_fn);
    handle= test_worker_start(libtest::default_port(), NULL, DEFAULT_WORKER_NAME, func_arg, NULL, gearman_worker_options_t());
  }

  ~client_test_st()
  {
    delete handle;
  }

  gearman_client_st* client()
  {
    return &_client;
  }
};

struct client_context_st {
  int latch;
  size_t min_size;
  size_t max_size;
  size_t num_tasks;
  size_t count;
  char *blob;

  client_context_st():
    latch(0),
    min_size(1024),
    max_size(1024 *2),
    num_tasks(20),
    count(2000),
    blob(NULL)
  { }

  ~client_context_st()
  {
    if (blob)
    {
      free(blob);
    }
  }
};

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static client_test_st *test_client_context= NULL;
static test_return_t burnin_TEST(void*)
{
  gearman_client_st *client= test_client_context->client();
  fatal_assert(client);

  client_context_st *context= (client_context_st *)gearman_client_context(client);
  fatal_assert(context);

  // This sketchy, don't do this in your own code.
  test_true(context->num_tasks > 0);
  std::vector<gearman_task_st> tasks;
  try {
    tasks.resize(context->num_tasks);
  }
  catch (...)
  { }
  ASSERT_EQ(tasks.size(), context->num_tasks);

  ASSERT_EQ(gearman_client_echo(client, test_literal_param("echo_test")), GEARMAN_SUCCESS);

  do
  {
    for (uint32_t x= 0; x < context->num_tasks; x++)
    {
      size_t blob_size= 0;

      if (context->min_size == context->max_size)
      {
        blob_size= context->max_size;
      }
      else
      {
        blob_size= (size_t)rand();

        if (context->max_size > RAND_MAX)
        {
          blob_size*= (size_t)(rand() + 1);
        }

        blob_size= (blob_size % (context->max_size - context->min_size)) + context->min_size;
      }

      gearman_task_st *task_ptr;
      gearman_return_t ret;
      if (context->latch)
      {
        task_ptr= gearman_client_add_task_background(client, &(tasks[x]),
                                                     NULL, DEFAULT_WORKER_NAME, NULL,
                                                     (void *)context->blob, blob_size, &ret);
      }
      else
      {
        task_ptr= gearman_client_add_task(client, &(tasks[x]), NULL,
                                          DEFAULT_WORKER_NAME, NULL, (void *)context->blob, blob_size,
                                          &ret);
      }

      ASSERT_EQ(ret, GEARMAN_SUCCESS);
      test_truth(task_ptr);
    }

    gearman_return_t ret= gearman_client_run_tasks(client);
    for (uint32_t x= 0; x < context->num_tasks; x++)
    {
      ASSERT_EQ(GEARMAN_TASK_STATE_FINISHED, tasks[x].state);
      ASSERT_EQ(GEARMAN_SUCCESS, tasks[x].result_rc);
    }
    test_zero(client->new_tasks);

    ASSERT_EQ(ret, GEARMAN_SUCCESS);

    for (uint32_t x= 0; x < context->num_tasks; x++)
    {
      gearman_task_free(&(tasks[x]));
    }
  } while (context->count--);

  context->latch++;

  return TEST_SUCCESS;
}

static test_return_t burnin_setup(void*)
{
  test_client_context= new client_test_st;
  client_context_st *context= new client_context_st;

  context->blob= (char *)malloc(context->max_size);
  test_true(context->blob);
  memset(context->blob, 'x', context->max_size); 

  gearman_client_set_context(test_client_context->client(), context);

  return TEST_SUCCESS;
}

static test_return_t burnin_cleanup(void*)
{
  client_context_st *context= (struct client_context_st *)gearman_client_context(test_client_context->client());

  delete context;
  delete test_client_context;
  test_client_context= NULL;

  return TEST_SUCCESS;
}

/*********************** World functions **************************************/

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (server_startup(servers, "gearmand", libtest::default_port(), NULL) == false)
  {
    error= TEST_SKIPPED;
    return NULL;
  }

  worker_handles_st *handle= new worker_handles_st;
  if (handle == NULL)
  {
    error= TEST_FAILURE;
    return NULL;
  }

  return handle;
}

static bool world_destroy(void *object)
{
  worker_handles_st *handles= (worker_handles_st *)object;
  delete handles;

  return TEST_SUCCESS;
}

test_st burnin_TESTS[] ={
  {"burnin", 0, burnin_TEST },
  {0, 0, 0}
};

collection_st collection[] ={
  {"burnin", burnin_setup, burnin_cleanup, burnin_TESTS },
  {0, 0, 0, 0}
};

void get_world(libtest::Framework *world)
{
  world->collections(collection);
  world->create(world_create);
  world->destroy(world_destroy);
}
