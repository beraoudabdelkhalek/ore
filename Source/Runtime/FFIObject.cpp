#include "FFIObject.h"
#include "../Ore.h"
#include "fmt/core.h"
#include <dlfcn.h>

namespace Ore {
FFIObject::FFIObject(std::string const& filename)
{
  m_handle = dlopen(filename.c_str(), RTLD_LAZY);
  if (m_handle == nullptr) {
    interpreter().throw_exception(ExceptionObject::file_not_found_exception(), fmt::format("Not a valid shared object: {}", filename));
    return;
  }

  auto* init_addr = dlsym(m_handle, "OreInitialize");
  if (init_addr == nullptr) {
    interpreter().throw_exception(ExceptionObject::reference_exception(), fmt::format("Cannot find \"OreInitialize\" function in {}", filename));
    return;
  }

  auto* init_func = reinterpret_cast<void (*)(std::map<char const*, OreFunctionDecl>&)>(init_addr);
  std::map<char const*, OreFunctionDecl> exports;
  init_func(exports);

  for (auto [name, decl] : exports)
    // FIXME: why does put() and put_native_function() cause a stack overflow?
    m_properties[name] = heap().allocate<NativeFunction>(decl);
}

FFIObject::~FFIObject()
{
  dlclose(m_handle);
}

void FFIObject::put(PropertyKey key, Value value)
{
  ASSERT_NOT_REACHED();
}
}
