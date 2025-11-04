/*
 * Runtime - API Export/Import Macro
 *
 * Not being used but defining for future use.
 */

#ifndef _RUNTIME_API_H_
#define _RUNTIME_API_H_

#ifdef RUNTIME_EXPORTS
#define RUNTIME_API __declspec(dllexport)
#else
#ifdef RUNTIME_STATIC
#define RUNTIME_API
#else
#define RUNTIME_API __declspec(dllimport)
#endif
#endif

#endif