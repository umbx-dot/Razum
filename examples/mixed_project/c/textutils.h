#pragma once

#ifdef TEXTUTILS_BUILD
#define TEXTUTILS_API __declspec(dllexport)
#else
#define TEXTUTILS_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

TEXTUTILS_API long long count_vowels(const char *text);
TEXTUTILS_API void shout(char *text);

#ifdef __cplusplus
}
#endif
