/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <xenia/kernel/xboxkrnl_debug.h>

#include <xenia/kernel/kernel_state.h>
#include <xenia/kernel/xboxkrnl_private.h>
#include <xenia/kernel/objects/xthread.h>
#include <xenia/kernel/util/shim_utils.h>


using namespace xe;
using namespace xe::kernel;
using namespace xe::kernel::xboxkrnl;


namespace xe {
namespace kernel {


// TODO: clean me up!
SHIM_CALL DbgPrint_shim(
    PPCContext* ppc_state, KernelState* state) {

  uint32_t format_ptr = SHIM_GET_ARG_32(0);
  if (format_ptr == 0) {
    SHIM_SET_RETURN(-1);
    return;
  }

  const char *format = (const char *)SHIM_MEM_ADDR(format_ptr);

  int arg_index = 0;

  char buffer[512]; // TODO: ensure it never writes past the end of the buffer...
  char *b = buffer;
  for (; *format != '\0'; ++format) {
    const char *start = format;

    if (*format != '%') {
      *b++ = *format;
      continue;
    }

    ++format;
    if (*format == '\0') {
      break;
    }

    if (*format == '%') {
      *b++ = *format;
      continue;
    }

    const char *end;
    end = format;

    // skip flags
    while (*end == '-' ||
           *end == '+' ||
           *end == ' ' ||
           *end == '#' ||
           *end == '0') {
      ++end;
    }

    if (*end == '\0') {
      break;
    }

    int arg_extras = 0;

    // skip width
    if (*end == '*') {
      ++end;
      arg_extras++;
    }
    else {
      while (*end >= '0' && *end <= '9') {
        ++end;
      }
    }

    if (*end == '\0') {
      break;
    }

    // skip precision
    if (*end == '.') {
      ++end;

      if (*end == '*') {
        ++end;
        ++arg_extras;
      }
      else {
        while (*end >= '0' && *end <= '9') {
          ++end;
        }
      }
    }

    if (*end == '\0') {
      break;
    }

    // get length
    int arg_size = 4;

    if (*end == 'h') {
      ++end;
      arg_size = 4;
      if (*end == 'h') {
        ++end;
      }
    }
    else if (*end == 'l') {
      ++end;
      arg_size = 4;
      if (*end == 'l') {
        ++end;
        arg_size = 8;
      }
    }
    else if (*end == 'j') {
      arg_size = 8;
      ++end;
    }
    else if (*end == 'z') {
      arg_size = 4;
      ++end;
    }
    else if (*end == 't') {
      arg_size = 8;
      ++end;
    }
    else if (*end == 'L') {
      arg_size = 8;
      ++end;
    }

    if (*end == '\0') {
      break;
    }

    if (*end == 'd' ||
        *end == 'i' ||
        *end == 'u' ||
        *end == 'o' ||
        *end == 'x' ||
        *end == 'X' ||
        *end == 'f' ||
        *end == 'F' ||
        *end == 'e' ||
        *end == 'E' ||
        *end == 'g' ||
        *end == 'G' ||
        *end == 'a' ||
        *end == 'A' ||
        *end == 'c') {
      char local[512];
      local[0] = '\0';
      strncat(local, start, end + 1 - start);

      XEASSERT(arg_size == 8 || arg_size == 4);
      if (arg_size == 8) {
        if (arg_extras == 0) {
          uint64_t value = arg_index < 7
            ? SHIM_GET_ARG_64(1 + arg_index)
            : SHIM_MEM_32(SHIM_GPR_32(1) + 16 + ((1 + arg_index) * 8));
          int result = sprintf(b, local, value);
          b += result;
          arg_index++;
        }
        else {
          XEASSERT(false);
        }
      }
      else if (arg_size == 4) {
        if (arg_extras == 0) {
          uint64_t value = arg_index < 7
            ? SHIM_GET_ARG_64(1 + arg_index)
            : SHIM_MEM_32(SHIM_GPR_32(1) + 16 + ((1 + arg_index) * 8));
          int result = sprintf(b, local, (uint32_t)value);
          b += result;
          arg_index++;
        }
        else {
          XEASSERT(false);
        }
      }
    }
    else if (*end == 's' ||
             *end == 'p' ||
             *end == 'n') {
      char local[512];
      local[0] = '\0';
      strncat(local, start, end + 1 - start);

      XEASSERT(arg_size == 4);
      if (arg_extras == 0) {
        uint32_t value = arg_index < 7
          ? SHIM_GET_ARG_32(1 + arg_index)
          : (uint32_t)SHIM_MEM_64(SHIM_GPR_32(1) + 16 + ((1 + arg_index) * 8));
        const char *pointer = (const char *)SHIM_MEM_ADDR(value);
        int result = sprintf(b, local, pointer);
        b += result;
        arg_index++;
      }
      else {
        XEASSERT(false);
      }
    }
    else {
      XEASSERT(false);
      break;
    }

    format = end;
  }
  *b++ = '\0';

  XELOGD("(DbgPrint) %s", buffer);
}


void xeDbgBreakPoint() {
  DebugBreak();
}


SHIM_CALL DbgBreakPoint_shim(
    PPCContext* ppc_state, KernelState* state) {
  XELOGD("DbgBreakPoint()");
}


SHIM_CALL RtlRaiseException_shim(
    PPCContext* ppc_state, KernelState* state) {
  uint32_t record_ptr = SHIM_GET_ARG_32(0);

  uint32_t code = SHIM_MEM_32(record_ptr + 0);
  uint32_t flags = SHIM_MEM_32(record_ptr + 4);
  // ...
  uint32_t param_count = SHIM_MEM_32(record_ptr + 16);

  XELOGD(
      "RtlRaiseException(%.8X(%.8X))",
      record_ptr, code);

  if (code == 0x406D1388) {
    // SetThreadName. FFS.
    uint32_t thread_info_ptr = record_ptr + 20;
    uint32_t type = SHIM_MEM_32(thread_info_ptr + 0);
    XEASSERT(type == 0x1000);
    uint32_t name_ptr = SHIM_MEM_32(thread_info_ptr + 4);
    uint32_t thread_id = SHIM_MEM_32(thread_info_ptr + 8);

    const char* name = (const char*)SHIM_MEM_ADDR(name_ptr);

    XThread* thread = NULL;
    if (thread_id == -1) {
      // Current thread.
      thread = XThread::GetCurrentThread();
      thread->Retain();
    } else {
      // Lookup thread by ID.
      thread = state->GetThreadByID(thread_id);
    }

    if (thread) {
      thread->set_name(name);
      thread->Release();
    }
  }

  // TODO(benvanik): unwinding.
  // This is going to suck.
  DebugBreak();
}


}  // namespace kernel
}  // namespace xe


void xe::kernel::xboxkrnl::RegisterDebugExports(
    ExportResolver* export_resolver, KernelState* state) {
  SHIM_SET_MAPPING("xboxkrnl.exe", DbgPrint, state);
  SHIM_SET_MAPPING("xboxkrnl.exe", DbgBreakPoint, state);
  SHIM_SET_MAPPING("xboxkrnl.exe", RtlRaiseException, state);
}
