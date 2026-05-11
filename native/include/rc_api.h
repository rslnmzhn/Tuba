#ifndef RC_API_H_
#define RC_API_H_

#include <stdint.h>

#if defined(_WIN32)
#if defined(RC_NATIVE_BUILDING_LIBRARY)
#define RC_API __declspec(dllexport)
#else
#define RC_API __declspec(dllimport)
#endif
#else
#define RC_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

RC_API int32_t rc_native_abi_version(void);

#ifdef __cplusplus
}
#endif

#endif  // RC_API_H_
