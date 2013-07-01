/*
 * Copyright 2013 Mic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdarg.h>
#include <stdio.h>
#include "NativeLogger.h"
#ifdef __ANDROID__
#include <android/log.h>
#endif
#ifdef __TIZEN__
#include <FBase.h>
#endif


void NativeLog(int level, const char *tag, const char *fmt, ...)
{
	static char buffer[256];

	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 256, fmt, args);
	va_end(args);

#ifdef __TIZEN__
	AppLog(buffer);
#endif
#ifdef  __ANDROID__
	__android_log_write(level, tag, buffer);
#endif
}
