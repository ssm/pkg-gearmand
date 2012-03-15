/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2008 Brian Aker, Eric Day
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
#include <libgearman/common.h>
#include <libgearman/universal.hpp>

#include <libgearman/add.hpp>
#include <libgearman/packet.hpp>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#include <uuid/uuid.h>

gearman_task_st *add_task(gearman_client_st& client,
                          void *context,
                          gearman_command_t command,
                          const gearman_string_t &function,
                          const gearman_unique_t &unique,
                          const gearman_string_t &workload,
                          time_t when,
                          const gearman_actions_t &actions)
{
  return add_task(client, NULL, context, command, function, unique, workload, when, actions);
}

gearman_task_st *add_task(gearman_client_st& client,
                          gearman_task_st *task,
                          void *context,
                          gearman_command_t command,
                          const char *function_name,
                          const char *unique,
                          const void *workload_str, size_t workload_size,
                          time_t when,
                          gearman_return_t *ret_ptr,
                          const gearman_actions_t &actions)
{
  gearman_return_t unused;
  if (ret_ptr == NULL)
  {
    ret_ptr= &unused;
  }

  gearman_string_t function= { gearman_string_param_cstr(function_name) };
  gearman_unique_t local_unique= gearman_unique_make(unique, unique ? strlen(unique) : 0);
  gearman_string_t workload= { static_cast<const char *>(workload_str), workload_size };

  task= add_task(client, task, context, command, function, local_unique, workload, when, actions);
  if (task == NULL)
  {
    *ret_ptr= gearman_universal_error_code(client.universal);
    return NULL;
  }

  *ret_ptr= GEARMAN_SUCCESS;

  return task;
}

gearman_task_st *add_task(gearman_client_st& client,
                          gearman_task_st *task,
                          void *context,
                          gearman_command_t command,
                          const gearman_string_t &function,
                          const gearman_unique_t &unique,
                          const gearman_string_t &workload,
                          time_t when,
                          const gearman_actions_t &actions)
{
  uuid_t uuid;
  char uuid_string[37];
  const void *args[4];
  size_t args_size[4];

  if (gearman_size(function) == 0 or gearman_c_str(function) == NULL or gearman_size(function) > GEARMAN_FUNCTION_MAX_SIZE)
  {
    if (gearman_size(function) > GEARMAN_FUNCTION_MAX_SIZE)
    {
      gearman_error(client.universal, GEARMAN_INVALID_ARGUMENT, "function name longer then GEARMAN_MAX_FUNCTION_SIZE");
    } 
    else
    {
      gearman_error(client.universal, GEARMAN_INVALID_ARGUMENT, "invalid function");
    }

    return NULL;
  }

  if (gearman_size(unique) > GEARMAN_MAX_UNIQUE_SIZE)
  {
    gearman_error(client.universal, GEARMAN_INVALID_ARGUMENT, "unique name longer then GEARMAN_MAX_UNIQUE_SIZE");

    return NULL;
  }

  if ((gearman_size(workload) && gearman_c_str(workload) == NULL) or (gearman_size(workload) == 0 && gearman_c_str(workload)))
  {
    gearman_error(client.universal, GEARMAN_INVALID_ARGUMENT, "invalid workload");
    return NULL;
  }

  task= gearman_task_internal_create(&client, task);
  if (task == NULL)
  {
    gearman_error(client.universal, GEARMAN_MEMORY_ALLOCATION_FAILURE, "");
    return NULL;
  }

  task->context= context;
  task->func= actions;

  /**
    @todo fix it so that NULL is done by default by the API not by happenstance.
  */
  char function_buffer[1024];
  if (client.universal._namespace)
  {
    char *ptr= function_buffer;
    memcpy(ptr, gearman_string_value(client.universal._namespace), gearman_string_length(client.universal._namespace)); 
    ptr+= gearman_string_length(client.universal._namespace);

    memcpy(ptr, gearman_c_str(function), gearman_size(function) +1);
    ptr+= gearman_size(function);

    args[0]= function_buffer;
    args_size[0]= ptr -function_buffer +1;
  }
  else
  {
    args[0]= gearman_c_str(function);
    args_size[0]= gearman_size(function) +1;
  }

  if (gearman_size(unique))
  {
    args[1]= gearman_c_str(unique);
    args_size[1]= gearman_size(unique) +1;
  }
  else
  {
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_string);
    uuid_string[36]= 0;
    args[1]= uuid_string;
    args_size[1]= 36 +1; // +1 is for the needed null
  }

  gearman_return_t rc;
  if (command == GEARMAN_COMMAND_SUBMIT_JOB_EPOCH)
  {
    char time_string[30];
    int length= snprintf(time_string, sizeof(time_string), "%lld", static_cast<long long>(when));
    args[2]= time_string;
    args_size[2]= length +1;
    args[3]= gearman_c_str(workload);
    args_size[3]= gearman_size(workload);

    rc= gearman_packet_create_args(client.universal, task->send,
                                   GEARMAN_MAGIC_REQUEST, command,
                                   args, args_size,
                                   4);
  }
  else
  {
    args[2]= gearman_c_str(workload);
    args_size[2]= gearman_size(workload);

    rc= gearman_packet_create_args(client.universal, task->send,
                                   GEARMAN_MAGIC_REQUEST, command,
                                   args, args_size,
                                   3);
  }

  if (gearman_success(rc))
  {
    client.new_tasks++;
    client.running_tasks++;
    task->options.send_in_use= true;

    return task;
  }

  gearman_task_free(task);
  gearman_gerror(client.universal, rc);

  return NULL;
}

gearman_task_st *add_reducer_task(gearman_client_st *client,
                                  gearman_command_t command,
                                  const gearman_job_priority_t,
                                  const gearman_string_t &function,
                                  const gearman_string_t &reducer,
                                  const gearman_unique_t &unique,
                                  const gearman_string_t &workload,
                                  const gearman_actions_t &actions,
                                  const time_t,
                                  void *context)
{
  uuid_t uuid;
  char uuid_string[37];
  const void *args[5];
  size_t args_size[5];

  if (gearman_size(function) == 0 or gearman_c_str(function) == NULL or gearman_size(function) > GEARMAN_FUNCTION_MAX_SIZE)
  {
    if (gearman_size(function) > GEARMAN_FUNCTION_MAX_SIZE)
    {
      gearman_error(client->universal, GEARMAN_INVALID_ARGUMENT, "function name longer then GEARMAN_MAX_FUNCTION_SIZE");
    } 
    else
    {
      gearman_error(client->universal, GEARMAN_INVALID_ARGUMENT, "invalid function");
    }

    return NULL;
  }

  if (gearman_size(unique) > GEARMAN_MAX_UNIQUE_SIZE)
  {
    gearman_error(client->universal, GEARMAN_INVALID_ARGUMENT, "unique name longer then GEARMAN_MAX_UNIQUE_SIZE");

    return NULL;
  }

  if ((gearman_size(workload) and not gearman_c_str(workload)) or (gearman_size(workload) == 0 && gearman_c_str(workload)))
  {
    gearman_error(client->universal, GEARMAN_INVALID_ARGUMENT, "invalid workload");
    return NULL;
  }

  gearman_task_st *task= gearman_task_internal_create(client, NULL);
  if (task == NULL)
  {
    gearman_error(client->universal, GEARMAN_MEMORY_ALLOCATION_FAILURE, "");
    return NULL;
  }

  task->context= context;
  task->func= actions;

  /**
    @todo fix it so that NULL is done by default by the API not by happenstance.
  */
  char function_buffer[1024];
  if (client->universal._namespace)
  {
    char *ptr= function_buffer;
    memcpy(ptr, gearman_string_value(client->universal._namespace), gearman_string_length(client->universal._namespace)); 
    ptr+= gearman_string_length(client->universal._namespace);

    memcpy(ptr, gearman_c_str(function), gearman_size(function) +1);
    ptr+= gearman_size(function);

    args[0]= function_buffer;
    args_size[0]= ptr- function_buffer +1;
  }
  else
  {
    args[0]= gearman_c_str(function);
    args_size[0]= gearman_size(function) + 1;
  }

  if (gearman_size(unique))
  {
    args[1]= gearman_c_str(unique);
    args_size[1]= gearman_size(unique) + 1;
  }
  else
  {
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_string);
    uuid_string[36]= 0;
    args[1]= uuid_string;
    args_size[1]= 36 +1; // +1 is for the needed null
  }

  assert_msg(command == GEARMAN_COMMAND_SUBMIT_REDUCE_JOB or command == GEARMAN_COMMAND_SUBMIT_REDUCE_JOB_BACKGROUND,
             "Command was not appropriate for request");

  char reducer_buffer[1024];
  if (client->universal._namespace)
  {
    char *ptr= reducer_buffer;
    memcpy(ptr, gearman_string_value(client->universal._namespace), gearman_string_length(client->universal._namespace)); 
    ptr+= gearman_string_length(client->universal._namespace);

    memcpy(ptr, gearman_c_str(reducer), gearman_size(reducer) +1);
    ptr+= gearman_size(reducer);

    args[2]= reducer_buffer;
    args_size[2]= ptr- reducer_buffer +1;
  }
  else
  {
    args[2]= gearman_c_str(reducer);
    args_size[2]= gearman_size(reducer) +1;
  }

  char aggregate[1];
  aggregate[0]= 0;
  args[3]= aggregate;
  args_size[3]= 1;

  assert_msg(gearman_c_str(workload), "Invalid workload (NULL)");
  assert_msg(gearman_size(workload), "Invalid workload of zero");
  args[4]= gearman_c_str(workload);
  args_size[4]= gearman_size(workload);

  gearman_return_t rc;
  if (gearman_success(rc= gearman_packet_create_args(client->universal, task->send,
                                                     GEARMAN_MAGIC_REQUEST, command,
                                                     args, args_size,
                                                     5)))
  {
    client->new_tasks++;
    client->running_tasks++;
    task->options.send_in_use= true;
  }
  else
  {
    gearman_gerror(client->universal, rc);
    gearman_task_free(task);
    task= NULL;
  }
  task->type= GEARMAN_TASK_KIND_EXECUTE;

  return task;
}
