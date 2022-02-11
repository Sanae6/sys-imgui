#pragma once

#include <switch/result.h>

#define R_ASSERT(callPoint) {Result __rc = (callPoint);if (R_FAILED(__rc)) abortWithLogResult(__rc, "Failed at %s:%d (%s)", __FILE__, __LINE__, #callPoint);}
#define R_ASSERT_LOG(callPoint, successText, ...) {Result __rc = (callPoint);if (R_FAILED(__rc)) abortWithLogResult(__rc, "Failed at %s:%d (on %s)", __FILE__, __LINE__, #callPoint); else log((successText "\n") __VA_OPT__(,) __VA_ARGS__);}

Result redirectStdoutToLogServer();
void abortWithoutLog(Result res);
void abortWithResult(Result res);
void abortWithLogResult(Result res, const char* text, ...);
void log(const char* text, ...);
void lognf(const char* text, ...); // log with no flush
