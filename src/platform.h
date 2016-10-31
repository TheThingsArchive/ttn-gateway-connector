#if !defined(__TTN_GW_PLATFORM_H_)
#define __TTN_GW_PLATFORM_H_

#define MQTT_TASK 1

#if defined(__linux__)
	#define _GNU_SOURCE
	#include <stdio.h>
	#include <MQTTLinux.h>
#endif

#if defined(__APPLE__)
	#define _GNU_SOURCE
	#include <stdio.h>
	#include <MQTTLinux.h>
#endif

// TODO: Include platform specific headers here

#endif
