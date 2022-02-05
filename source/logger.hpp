#pragma once

#include <switch/result.h>

Result redirectStdoutToLogServer();
void abortWithResult(Result res);
void log(const char* text, ...);