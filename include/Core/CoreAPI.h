/*
 * Core - API Export/Import Macros
 *
 * Not being used yet but preemptively defined for future use.
 */

#ifndef _CORE_API_H_
#define _CORE_API_H_

#ifdef CORE_EXPORTS
#define CORE_API __declspec(dllexport)
#else
#ifdef CORE_STATIC
#define CORE_API
#else
#define CORE_API __declspec(dllimport)
#endif
#endif

#endif