#ifndef PACKED_H
#define PACKED_H

#if defined(_MSC_VER)
  // MSVC: pragma-based packing
  #define PACKED_STRUCT_BEGIN __pragma(pack(push, 1))
  #define PACKED_STRUCT_END   __pragma(pack(pop))
#elif defined(__GNUC__) || defined(__clang__)
  // GCC/Clang: attribute-based packing
  #define PACKED_STRUCT_BEGIN
  #define PACKED_STRUCT_END __attribute__((packed))
#else
  #error "Unsupported compiler for PACKED_STRUCT"
#endif

#endif // PACKED_H
