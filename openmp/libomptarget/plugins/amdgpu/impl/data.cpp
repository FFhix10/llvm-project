//===--- amdgpu/impl/data.cpp ------------------------------------- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "atmi_runtime.h"
#include "hsa_api.h"
#include "internal.h"
#include "machine.h"
#include "rt.h"
#include <cassert>
#include <stdio.h>
#include <string.h>
#include <vector>

using core::TaskImpl;
extern ATLMachine g_atl_machine;

namespace core {
extern __attribute__((weak)) hsa_status_t fort_ptr_assign_i8(void *arg0, void *arg1, void *arg2, void *arg3, void *arg4);
hsa_status_t Runtime::FtnAssignWrapper(void *arg0, void *arg1, void *arg2, void *arg3, void *arg4) {
  return fort_ptr_assign_i8(arg0, arg1, arg2, arg3, arg4);
}

namespace {
ATLProcessor &get_processor_by_mem_place(int DeviceId,
                                         atmi_devtype_t DeviceType) {
  switch (DeviceType) {
  case ATMI_DEVTYPE_CPU:
    return g_atl_machine.processors<ATLCPUProcessor>()[DeviceId];
  case ATMI_DEVTYPE_GPU:
    return g_atl_machine.processors<ATLGPUProcessor>()[DeviceId];
  }
}

hsa_amd_memory_pool_t get_memory_pool_by_mem_place(int DeviceId,
                                                   atmi_devtype_t DeviceType) {
  ATLProcessor &proc = get_processor_by_mem_place(DeviceId, DeviceType);
  return get_memory_pool(proc, 0 /*Memory Type (always zero) */);
}
} // namespace

hsa_status_t Runtime::DeviceMalloc(void **ptr, size_t size, int DeviceId) {
  return Runtime::Malloc(ptr, size, DeviceId, ATMI_DEVTYPE_GPU);
}

hsa_status_t Runtime::HostMalloc(void **ptr, size_t size) {
  hsa_status_t Err = Runtime::Malloc(ptr, size, 0, ATMI_DEVTYPE_CPU);
  if (Err == HSA_STATUS_SUCCESS) {
    Err = core::allow_access_to_all_gpu_agents(*ptr);
  }
  return Err;
}

hsa_status_t Runtime::Malloc(void **ptr, size_t size, int DeviceId,
                             atmi_devtype_t DeviceType) {
  hsa_amd_memory_pool_t pool =
      get_memory_pool_by_mem_place(DeviceId, DeviceType);
  hsa_status_t err = hsa_amd_memory_pool_allocate(pool, size, 0, ptr);
  DEBUG_PRINT("Malloced [%s %d] %p\n",
              DeviceType == ATMI_DEVTYPE_CPU ? "CPU" : "GPU", DeviceId, *ptr);
  return (err == HSA_STATUS_SUCCESS) ? HSA_STATUS_SUCCESS : HSA_STATUS_ERROR;
}

hsa_status_t Runtime::Memfree(void *ptr) {
  hsa_status_t err = hsa_amd_memory_pool_free(ptr);
  DEBUG_PRINT("Freed %p\n", ptr);

  return (err == HSA_STATUS_SUCCESS) ? HSA_STATUS_SUCCESS : HSA_STATUS_ERROR;
}

} // namespace core
