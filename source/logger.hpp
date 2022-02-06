#pragma once

#include <switch/result.h>

Result redirectStdoutToLogServer();
void abortWithResult(Result res);
void abortWithLogResult(Result res, const char* text, ...);
void log(const char* text, ...);