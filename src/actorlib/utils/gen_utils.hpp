#pragma once
#include <iostream>
#include <string>
#include <cxxabi.h>

#define __get_class_name__() ({                                                 \
            int status;                                                         \
            char * d = abi::__cxa_demangle(typeid(port).name(),0,0,&status);    \
            std::string dd = std::string(d);                                    \
            auto find = dd.find('<');                                           \
            if (find)                                                           \
                dd.erase(find);                                                 \
            free(d);                                                            \
            dd;                                                                 \
})

#define __class_name__ __get_class_name__()

namespace utils {
template <typename T> std::string to_string(const T &object) {
  std::ostringstream ss;
  ss << object;
  return ss.str();
}

static void debug(const std::string &text) {
#if DEBUG
  std::cout << text << std::endl;
#endif
}

} // namespace utils
