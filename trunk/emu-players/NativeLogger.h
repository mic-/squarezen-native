/*
 * NativeLogger.h
 *
 *  Created on: Jun 3, 2013
 *
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

#ifndef NATIVELOGGER_H_
#define NATIVELOGGER_H_

enum
{
#ifdef __ANDROID__
	NATIVELOG_VERBOSE = ANDROID_LOG_VERBOSE,
	NATIVELOG_DEBUG = ANDROID_LOG_DEBUG,
	NATIVELOG_WARNING = ANDROID_LOG_WARNING,
	NATIVELOG_ERROR = ANDROID_LOG_ERROR,
#else
	NATIVELOG_VERBOSE = 0,
	NATIVELOG_DEBUG,
	NATIVELOG_WARNING,
	NATIVELOG_ERROR,
#endif
};

void NativeLog(int level, const char *tag, const char *fmt, ...);

#ifdef __TIZEN__
#include <FBase.h>
#define NativeLogImpl(level, tag, fmt, ...) AppLog(fmt, ##__VA_ARGS__)
#else
#define NativeLogImpl(level, tag, fmt, ...) NativeLog(level, tag, fmt, ##__VA_ARGS__)
#endif

#if defined(NLOG_LEVEL_VERBOSE) && NLOG_LEVEL_VERBOSE == 0
#define NLOGV(tag, fmt, ...) NativeLogImpl(NATIVELOG_VERBOSE, tag, fmt, ##__VA_ARGS__)
#define NLOGD(tag, fmt, ...) NativeLogImpl(NATIVELOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define NLOGW(tag, fmt, ...) NativeLogImpl(NATIVELOG_WARNING, tag, fmt, ##__VA_ARGS__)
#define NLOGE(tag, fmt, ...) NativeLogImpl(NATIVELOG_ERROR, tag, fmt, ##__VA_ARGS__)
#else
#define NLOGV(tag, fmt, ...)
#endif

#if defined(NLOG_LEVEL_DEBUG) && NLOG_LEVEL_DEBUG == 0
#define NLOGD(tag, fmt, ...) NativeLogImpl(NATIVELOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define NLOGW(tag, fmt, ...) NativeLogImpl(NATIVELOG_WARNING, tag, fmt, ##__VA_ARGS__)
#define NLOGE(tag, fmt, ...) NativeLogImpl(NATIVELOG_ERROR, tag, fmt, ##__VA_ARGS__)
#else
#ifndef NLOGD
#define NLOGD(tag, fmt, ...)
#endif
#endif

#if defined(NLOG_LEVEL_WARNING) && NLOG_LEVEL_WARNING == 0
#define NLOGW(tag, fmt, ...) NativeLogImpl(NATIVELOG_WARNING, tag, fmt, ##__VA_ARGS__)
#define NLOGE(tag, fmt, ...) NativeLogImpl(NATIVELOG_ERROR, tag, fmt, ##__VA_ARGS__)
#else
#ifndef NLOGW
#define NLOGW(tag, fmt, ...)
#endif
#endif

#if defined(NLOG_LEVEL_ERROR) && NLOG_LEVEL_ERROR == 0
#define NLOGE(tag, fmt, ...) NativeLogImpl(NATIVELOG_ERROR, tag, fmt, ##__VA_ARGS__)
#else
#ifndef NLOGE
#define NLOGE(tag, fmt, ...)
#endif
#endif

#endif /* NATIVELOGGER_H_ */
