#if !defined(__TTN_GW_PLATFORM_H_)
#define __TTN_GW_PLATFORM_H_

#if defined(__linux__)
	#define _GNU_SOURCE
	#include <stdio.h>
	#include <MQTTLinux.h>
#endif

// TODO: Include platform specific headers here

#endif
