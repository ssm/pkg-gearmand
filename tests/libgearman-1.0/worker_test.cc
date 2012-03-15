/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <config.h>
#include <libtest/test.hpp>

using namespace libtest;

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <libgearman/gearman.h>
#include <libgearman/connection.hpp>
#include "libgearman/packet.hpp"
#include "libgearman/universal.hpp"

#include <tests/client.h>
#include <tests/worker.h>

static test_return_t init_test(void *)
{
  gearman_worker_st worker;

  test_truth(gearman_worker_create(&worker));

  gearman_worker_free(&worker);

  return TEST_SUCCESS;
}

static test_return_t allocation_test(void *)
{
  gearman_worker_st *worker;

  test_truth(worker= gearman_worker_create(NULL));

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static test_return_t gearman_worker_clone_NULL_NULL(void *)
{
  gearman_worker_st *worker= gearman_worker_clone(NULL, NULL);

  test_truth(worker);
  test_compare(true, worker->options.allocated);

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_clone_NULL_SOURCE(void *)
{
  Worker source;

  gearman_worker_st *worker= gearman_worker_clone(NULL, &source);
  test_truth(worker);
  test_compare(true, worker->options.allocated);
  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_timeout_default_test(void *)
{
  Worker worker;

  test_compare(-1, gearman_worker_timeout(&worker));

  return TEST_SUCCESS;
}

static test_return_t option_test(void *)
{
  gearman_worker_st *gear;
  gearman_worker_options_t default_options;

  gear= gearman_worker_create(NULL);
  test_truth(gear);
  { // Initial Allocated, no changes
    test_truth(gear->options.allocated);
    test_false(gear->options.non_blocking);
    test_truth(gear->options.packet_init);
    test_false(gear->options.change);
    test_true(gear->options.grab_uniq);
    test_false(gear->options.timeout_return);
  }

  /* Set up for default options */
  default_options= gearman_worker_options(gear);

  /*
    We take the basic options, and push
    them back in. See if we change anything.
  */
  gearman_worker_set_options(gear, default_options);
  { // Initial Allocated, no changes
    test_truth(gear->options.allocated);
    test_false(gear->options.non_blocking);
    test_truth(gear->options.packet_init);
    test_false(gear->options.change);
    test_true(gear->options.grab_uniq);
    test_false(gear->options.timeout_return);
  }

  /*
    We will trying to modify non-mutable options (which should not be allowed)
  */
  {
    gearman_worker_remove_options(gear, GEARMAN_WORKER_ALLOCATED);
    { // Initial Allocated, no changes
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_true(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }

    gearman_worker_remove_options(gear, GEARMAN_WORKER_PACKET_INIT);
    { // Initial Allocated, no changes
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_true(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
  }

  /*
    We will test modifying GEARMAN_WORKER_NON_BLOCKING in several manners.
  */
  {
    gearman_worker_remove_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_true(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_add_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_true(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_set_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_set_options(gear, GEARMAN_WORKER_GRAB_UNIQ);
    { // Everything is now set to false except GEARMAN_WORKER_GRAB_UNIQ, and non-mutable options
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_truth(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    /*
      Reset options to default. Then add an option, and then add more options. Make sure
      the options are all additive.
    */
    {
      gearman_worker_set_options(gear, default_options);
      { // See if we return to defaults
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_true(gear->options.grab_uniq);
        test_false(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_true(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, (gearman_worker_options_t)(GEARMAN_WORKER_NON_BLOCKING|GEARMAN_WORKER_GRAB_UNIQ));
      { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
        test_truth(gear->options.allocated);
        test_truth(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_truth(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
    }
    /*
      Add an option, and then replace with that option plus a new option.
    */
    {
      gearman_worker_set_options(gear, default_options);
      { // See if we return to defaults
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_true(gear->options.grab_uniq);
        test_false(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_true(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, (gearman_worker_options_t)(GEARMAN_WORKER_TIMEOUT_RETURN|GEARMAN_WORKER_GRAB_UNIQ));
      { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_truth(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
    }
  }

  gearman_worker_free(gear);

  return TEST_SUCCESS;
}

static test_return_t echo_test(void*)
{
  Worker worker;

  test_compare_hint(GEARMAN_SUCCESS,
                    gearman_worker_echo(&worker, test_literal_param("This is my echo test")),
                    gearman_worker_error(&worker));

  return TEST_SUCCESS;
}

static test_return_t echo_multi_test(void *)
{
  Worker worker;

  const char *value[]= {
    "This is my echo test",
    "This land is my land",
    "This land is your land",
    "We the people",
    "in order to form a more perfect union",
    "establish justice",
    NULL
  };
  const char **ptr= value;

  while (*ptr)
  {
    test_compare_hint(GEARMAN_SUCCESS,
                      gearman_worker_echo(&worker, test_string_make_from_cstr(*ptr)),
                      gearman_worker_error(&worker));
    ptr++;
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_server_GEARMAN_INVALID_ARGUMENT_TEST(void *)
{
  test_compare(GEARMAN_INVALID_ARGUMENT,
               gearman_worker_add_server(NULL, "nonexist.gearman.info", libtest::default_port()));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_server_GEARMAN_GETADDRINFO_TEST(void *)
{
  gearman_worker_st *worker= gearman_worker_create(NULL);
  test_true(worker);
  test_compare(GEARMAN_GETADDRINFO, gearman_worker_add_server(worker, "nonexist.gearman.info", libtest::default_port()));
  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

static test_return_t echo_max_test(void *)
{
  Worker worker;

  gearman_worker_add_server(&worker, NULL, libtest::default_port());

  test_compare(GEARMAN_ARGUMENT_TOO_LARGE,
               gearman_worker_echo(&worker, "This is my echo test", GEARMAN_MAX_ECHO_SIZE +1));

  return TEST_SUCCESS;
}

static test_return_t abandoned_worker_test(void *)
{
  gearman_job_handle_t job_handle;
  const void *args[2];
  size_t args_size[2];

  {
    Client client;
    gearman_client_add_server(&client, NULL, libtest::default_port());
    test_compare_hint(GEARMAN_SUCCESS,
                      gearman_client_do_background(&client, "abandoned_worker", NULL, NULL, 0, job_handle), gearman_client_error(&client));
  }

  /* Now take job with one worker. */
  gearman_universal_st universal;
  gearman_universal_initialize(universal);

  gearman_connection_st *worker1;
  test_truth(worker1= gearman_connection_create(universal, NULL));

  worker1->set_host(NULL, libtest::default_port());

  gearman_packet_st packet;
  args[0]= "abandoned_worker";
  args_size[0]= strlen("abandoned_worker");
  test_compare(GEARMAN_SUCCESS,
               gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                          GEARMAN_COMMAND_CAN_DO,
                                          args, args_size, 1));

  test_compare_hint(GEARMAN_SUCCESS,
                    worker1->send_packet(packet, true), gearman_universal_error(universal));

  gearman_packet_free(&packet);

  gearman_return_t ret;
  test_compare(GEARMAN_SUCCESS,
               gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_GRAB_JOB, NULL, NULL, 0));

  test_compare(GEARMAN_SUCCESS, worker1->send_packet(packet, true));

  gearman_packet_free(&packet);

  worker1->receiving(packet, ret, false);
  test_truth(not (ret != GEARMAN_SUCCESS or packet.command != GEARMAN_COMMAND_JOB_ASSIGN));

  test_strcmp(job_handle, packet.arg[0]); // unexepcted job

  gearman_packet_free(&packet);

  gearman_connection_st *worker2;
  test_truth(worker2= gearman_connection_create(universal, NULL));

  worker2->set_host(NULL, libtest::default_port());

  args[0]= "abandoned_worker";
  args_size[0]= strlen("abandoned_worker");
  test_compare(GEARMAN_SUCCESS, gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                           GEARMAN_COMMAND_CAN_DO,
                                                           args, args_size, 1));

  test_compare(GEARMAN_SUCCESS, worker2->send_packet(packet, true));

  gearman_packet_free(&packet);

  args[0]= job_handle;
  args_size[0]= strlen(job_handle) + 1;
  args[1]= "test";
  args_size[1]= 4;
  test_compare(GEARMAN_SUCCESS, gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                           GEARMAN_COMMAND_WORK_COMPLETE,
                                                           args, args_size, 2));

  test_compare(GEARMAN_SUCCESS, worker2->send_packet(packet, true));

  gearman_packet_free(&packet);

  gearman_universal_set_timeout(universal, 1000);
  worker2->receiving(packet, ret, false);
  test_truth(not (ret != GEARMAN_SUCCESS or packet.command != GEARMAN_COMMAND_ERROR));

  delete worker1;
  delete worker2;
  gearman_packet_free(&packet);
  gearman_universal_free(universal);

  return TEST_SUCCESS;
}

static void *no_unique_worker(gearman_job_st *job,
                              void *, size_t *size,
                              gearman_return_t *ret_ptr)
{
  if (gearman_job_unique(job) and strlen(gearman_job_unique(job)))
  {
    *ret_ptr= GEARMAN_WORK_FAIL;
  }
  else
  {
    *ret_ptr= GEARMAN_SUCCESS;
  }
  *size= 0;

  return NULL;
}

static void *check_unique_worker(gearman_job_st *job,
                                 void *context, size_t *size,
                                 gearman_return_t *ret_ptr)
{
  if (gearman_job_unique(job))
  {
    size_t length= strlen(gearman_job_unique(job));
    if (length ==  gearman_job_workload_size(job))
    {
      if (not memcmp(gearman_job_unique(job), gearman_job_workload(job),length))
      {
        bool *success= (bool *)context;
        if (success)
          *success= true;

        *ret_ptr= GEARMAN_SUCCESS;
        *size= length;
        return strdup((char*)gearman_job_unique(job));
      }
    }
  }

  *size= 0;
  *ret_ptr= GEARMAN_WORK_FAIL;

  return NULL;
}

static void *fail_worker(gearman_job_st *,
                         void *, size_t *size,
                         gearman_return_t *ret_ptr)
{
  *ret_ptr= GEARMAN_WORK_FAIL;
  *size= 0;

  return NULL;
}

static test_return_t gearman_worker_add_function_test(void *)
{
  Worker worker;

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  test_compare_hint(GEARMAN_SUCCESS,
                    gearman_worker_add_function(&worker, function_name,0, fail_worker, NULL),
                    gearman_worker_error(&worker));

  test_compare(true, gearman_worker_function_exist(&worker, test_string_make_from_array(function_name)));

  test_compare_hint(GEARMAN_SUCCESS,
                    gearman_worker_unregister(&worker, function_name),
                    gearman_worker_error(&worker));

  test_compare(false, gearman_worker_function_exist(&worker, function_name, strlen(function_name)));

  /* Make sure we have removed it */
  test_compare_hint(GEARMAN_NO_REGISTERED_FUNCTION, 
                    gearman_worker_unregister(&worker, function_name),
                    gearman_worker_error(&worker));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_function_multi_test(void *)
{
  Worker worker;

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];
    snprintf(buffer, 1024, "%u%s", x, __func__);

    test_compare_hint(GEARMAN_SUCCESS,
                      gearman_worker_add_function(&worker, buffer, 0, fail_worker, NULL),
                      gearman_worker_error(&worker));
  }

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, 1024, "%u%s", x, __func__);
    test_compare_hint(GEARMAN_SUCCESS,
                      gearman_worker_unregister(&worker, buffer),
                      gearman_worker_error(&worker));
  }

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, 1024, "%u%s", x, __func__);
    test_compare_hint(GEARMAN_NO_REGISTERED_FUNCTION,
                      gearman_worker_unregister(&worker, buffer),
                      gearman_worker_error(&worker));
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_unregister_all_test(void *)
{
  Worker worker;
  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%u%s", x, __func__);
    gearman_return_t rc= gearman_worker_add_function(&worker,
						     buffer,
						     0, fail_worker, NULL);

    test_compare_hint(GEARMAN_SUCCESS, rc, gearman_strerror(rc));
  }

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_unregister_all(&worker));

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, sizeof(buffer), "%u%s", x, __func__);
    gearman_return_t rc= gearman_worker_unregister(&worker, buffer);
    test_true_got(rc == GEARMAN_NO_REGISTERED_FUNCTION, gearman_strerror(rc));
  }

  test_compare_hint(GEARMAN_NO_REGISTERED_FUNCTIONS,
                    gearman_worker_unregister_all(&worker),
                    gearman_worker_error(&worker));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_work_with_test(void *)
{
  Worker worker;

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  test_compare_hint(GEARMAN_SUCCESS,
                    gearman_worker_add_function(&worker,
                                                function_name,
                                                0, fail_worker, NULL),
                    gearman_worker_error(&worker));

  gearman_worker_set_timeout(&worker, 0);

  test_compare(GEARMAN_TIMEOUT,
               gearman_worker_work(&worker));

  test_compare_hint(GEARMAN_TIMEOUT,
                    gearman_worker_work(&worker),
                    gearman_worker_error(&worker));

  /* Make sure we have removed the worker function */
  test_compare_hint(GEARMAN_SUCCESS,
                    gearman_worker_unregister(&worker, function_name),
                    gearman_worker_error(&worker));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_context_test(void *)
{
  Worker worker;

  test_false(gearman_worker_context(&worker));

  int value= 5;
  gearman_worker_set_context(&worker, &value);

  int *ptr= (int *)gearman_worker_context(&worker);

  test_truth(ptr == &value);
  test_truth(*ptr == value);
  gearman_worker_set_context(&worker, NULL);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_check_options_GEARMAN_WORKER_GRAB_UNIQ(void *)
{
  Worker worker;

  test_true(worker->options.grab_uniq);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_remove_options_GEARMAN_WORKER_GRAB_UNIQ(void *)
{
  Worker worker;

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  char unique_name[GEARMAN_MAX_UNIQUE_SIZE];
  snprintf(unique_name, GEARMAN_MAX_UNIQUE_SIZE, "_%s%d", __func__, int(random())); 

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_server(&worker, NULL, libtest::default_port()));

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_function(&worker, function_name, 0, no_unique_worker, NULL));

  {
    Client client;
    test_compare(GEARMAN_SUCCESS,
                 gearman_client_add_server(&client, NULL, libtest::default_port()));
    test_compare_hint(GEARMAN_SUCCESS,
                      gearman_client_do_background(&client, function_name, unique_name,
                                                   test_string_make_from_array(unique_name), NULL),
                      gearman_client_error(&client));
  }

  gearman_worker_remove_options(&worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_false(worker->options.grab_uniq);

  gearman_worker_set_timeout(&worker, 4);

  gearman_return_t rc;
  gearman_job_st *job= gearman_worker_grab_job(&worker, NULL, &rc);
  test_compare_got(GEARMAN_SUCCESS, rc, gearman_strerror(rc));
  test_truth(job);

  size_t size= 0;
  void *result= no_unique_worker(job, NULL, &size, &rc);
  test_compare_got(GEARMAN_SUCCESS,
                   rc, gearman_strerror(rc));
  test_false(result);
  test_false(size);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ(void *)
{
  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  char unique_name[GEARMAN_MAX_UNIQUE_SIZE];
  snprintf(unique_name, GEARMAN_MAX_UNIQUE_SIZE, "_%s%d", __func__, int(random())); 

  {
    Client client;
    test_compare(GEARMAN_SUCCESS,
                 gearman_client_add_server(&client, NULL, libtest::default_port()));

    test_compare_got(GEARMAN_SUCCESS, 
                     gearman_client_do_background(&client, function_name, unique_name,
                                                  test_string_make_from_array(unique_name), NULL), 
                     gearman_client_error(&client));
  }

  Worker worker;

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_server(&worker, NULL, libtest::default_port()));

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_function(&worker, function_name, 0, check_unique_worker, NULL));

  gearman_worker_add_options(&worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_true(worker->options.grab_uniq);

  gearman_return_t rc;
  gearman_job_st *job= gearman_worker_grab_job(&worker, NULL, &rc);
  test_compare(GEARMAN_SUCCESS, rc);
  test_truth(job);

  size_t size= 0;
  void *result= check_unique_worker(job, NULL, &size, &rc);
  test_compare(GEARMAN_SUCCESS, rc);
  test_truth(result);
  test_truth(size);
  free(result);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_set_identifier_TEST(void *)
{
  Worker worker;

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_server(&worker, NULL, libtest::default_port()));

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_set_identifier(&worker, test_literal_param(__func__)));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ_worker_work(void *)
{
  Worker worker;

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  char unique_name[GEARMAN_MAX_UNIQUE_SIZE];
  snprintf(unique_name, GEARMAN_MAX_UNIQUE_SIZE, "_%s%d", __func__, int(random())); 

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_server(&worker, NULL, libtest::default_port()));

  bool success= false;
  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_function(&worker, function_name, 0, check_unique_worker, &success));

  {
    Client client;
    test_compare(GEARMAN_SUCCESS,
                 gearman_client_add_server(&client, NULL, libtest::default_port()));
    test_compare_hint(GEARMAN_SUCCESS,
                      gearman_client_do_background(&client, function_name, unique_name,
                                                   test_string_make_from_array(unique_name), NULL),
                      gearman_client_error(&client));
  }

  test_true(worker->options.grab_uniq);
  gearman_worker_add_options(&worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_truth(worker->options.grab_uniq);

  gearman_worker_set_timeout(&worker, 4);
  test_compare(GEARMAN_SUCCESS, gearman_worker_work(&worker));

  test_truth(success);


  return TEST_SUCCESS;
}

static test_return_t gearman_worker_failover_test(void *)
{
  Worker worker;

  test_compare(GEARMAN_SUCCESS, gearman_worker_add_server(&worker, NULL, libtest::default_port()));
  test_compare(GEARMAN_SUCCESS, gearman_worker_add_server(&worker, NULL, libtest::default_port() +1));

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  test_compare(GEARMAN_SUCCESS, 
               gearman_worker_add_function(&worker, function_name, 0, fail_worker, NULL));

  gearman_worker_set_timeout(&worker, 2);

  test_compare(GEARMAN_TIMEOUT, gearman_worker_work(&worker));

  /* Make sure we have remove worker function */
  test_compare(GEARMAN_SUCCESS,
               gearman_worker_unregister(&worker, function_name));

  return TEST_SUCCESS;
}

/*********************** World functions **************************************/

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (server_startup(servers, "gearmand", libtest::default_port(), 0, NULL) == false)
  {
    error= TEST_FAILURE;
    return NULL;
  }

  return NULL;
}

test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"gearman_worker_clone(NULL, NULL)", 0, gearman_worker_clone_NULL_NULL },
  {"gearman_worker_clone(NULL, source)", 0, gearman_worker_clone_NULL_SOURCE },
  {"gearman_worker_add_server(GEARMAN_GETADDRINFO)", false, gearman_worker_add_server_GEARMAN_GETADDRINFO_TEST },
  {"gearman_worker_add_server(GEARMAN_INVALID_ARGUMENT)", false, gearman_worker_add_server_GEARMAN_INVALID_ARGUMENT_TEST },
  {"echo", 0, echo_test },
  {"echo_multi", 0, echo_multi_test },
  {"options", 0, option_test },
  {"gearman_worker_add_function()", 0, gearman_worker_add_function_test },
  {"gearman_worker_add_function() multi", 0, gearman_worker_add_function_multi_test },
  {"gearman_worker_unregister_all()", 0, gearman_worker_unregister_all_test },
  {"gearman_worker_work() with timout", 0, gearman_worker_work_with_test },
  {"gearman_worker_context", 0, gearman_worker_context_test },
  {"gearman_worker_failover", 0, gearman_worker_failover_test },
  {"gearman_worker_check_options(GEARMAN_WORKER_GRAB_UNIQ)", 0, gearman_worker_check_options_GEARMAN_WORKER_GRAB_UNIQ },
  {"gearman_worker_remove_options(GEARMAN_WORKER_GRAB_UNIQ)", 0, gearman_worker_remove_options_GEARMAN_WORKER_GRAB_UNIQ },
  {"gearman_worker_add_options(GEARMAN_WORKER_GRAB_UNIQ)", 0, gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ },
  {"gearman_worker_add_options(GEARMAN_WORKER_GRAB_UNIQ) worker_work()", 0, gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ_worker_work },
  {"echo_max", 0, echo_max_test },
  {"abandoned_worker", 0, abandoned_worker_test },
  {0, 0, 0}
};

test_st worker_defaults[] ={
  {"gearman_worker_timeout()", 0, gearman_worker_timeout_default_test },
  {0, 0, 0}
};

test_st gearman_worker_set_identifier_TESTS[] ={
#if 0
  {"gearman_worker_set_identifier()", 0, gearman_worker_set_identifier_TEST },
#endif
  {0, 0, 0}
};

collection_st collection[] ={
  {"worker", 0, 0, tests},
  {"worker defaults", 0, 0, worker_defaults},
  {"gearman_worker_set_identifier()", 0, 0, gearman_worker_set_identifier_TESTS},
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  (void)gearman_worker_set_identifier_TEST;
  world->collections= collection;
  world->_create= world_create;
}
