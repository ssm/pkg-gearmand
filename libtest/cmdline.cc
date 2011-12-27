/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  libtest
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <libtest/common.h>

using namespace libtest;

#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>

extern "C" {
  static int exited_successfully(int status)
  {
    if (status == 0)
    {
      return EXIT_SUCCESS;
    }

    if (WIFEXITED(status) == true)
    {
      return WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status) == true)
    {
      return WTERMSIG(status);
    }

    return -1;
  }
}

namespace libtest {

int exec_cmdline(const std::string& executable, const char *args[])
{
  std::stringstream arg_buffer;

  arg_buffer << libtool();

  if (getenv("PWD"))
  {
    arg_buffer << getenv("PWD");
    arg_buffer << "/";
  }

  arg_buffer << executable;
  for (const char **ptr= args; *ptr; ++ptr)
  {
    arg_buffer << " " << *ptr;
  }

  int ret= system(arg_buffer.str().c_str());

  return exited_successfully(ret);
}

const char *gearmand_binary() 
{
  return GEARMAND_BINARY;
}

} // namespace exec_cmdline
