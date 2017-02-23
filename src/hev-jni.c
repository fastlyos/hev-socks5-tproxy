/*
 ============================================================================
 Name        : hev-jni.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description : JNI
 ============================================================================
 */

#if defined(ANDROID)
#include <jni.h>
#include <pthread.h>

#include <stdio.h>
#include <signal.h>

#include "hev-jni.h"
#include "hev-main.h"

#define N_ELEMENTS(arr)		(sizeof (arr) / sizeof ((arr)[0]))

static JavaVM *java_vm;
static pthread_t service;
static pthread_key_t current_jni_env;

static void native_start_service (JNIEnv *env, jobject thiz, jstring conig_path);
static void native_stop_service (JNIEnv *env, jobject thiz);

static JNINativeMethod native_methods[] =
{
	{ "TProxyStartService", "(Ljava/lang/String;)V", (void *) native_start_service },
	{ "TProxyStopService", "()V", (void *) native_stop_service },
};

static void
detach_current_thread (void *env)
{
	(*java_vm)->DetachCurrentThread (java_vm);
}

jint
JNI_OnLoad (JavaVM *vm, void *reserved)
{
	JNIEnv *env = NULL;
	jclass klass;

	java_vm = vm;
	if (JNI_OK != (*vm)->GetEnv (vm, (void**) &env, JNI_VERSION_1_4)) {
		return 0;
	}

	klass = (*env)->FindClass (env, "hev/htproxy/TProxyService");
	(*env)->RegisterNatives (env, klass, native_methods, N_ELEMENTS (native_methods));
	(*env)->DeleteLocalRef (env, klass);

	pthread_key_create (&current_jni_env, detach_current_thread);

	return JNI_VERSION_1_4;
}

static void *
thread_handler (void *data)
{
	char **argv = data;

	argv[0] = "hev-socks5-client";

	main (2, argv);

	free (argv[1]);
	free (argv);

	return NULL;
}

static void
native_start_service (JNIEnv *env, jobject thiz, jstring config_path)
{
	char **argv;
	const jbyte *bytes;

	argv = malloc (sizeof (char *) * 2);

	bytes = (*env)->GetStringUTFChars (env, config_path, NULL);
	argv[1] = strdup (bytes);
	(*env)->ReleaseStringUTFChars (env, config_path, bytes);

	pthread_create (&service, NULL, thread_handler, argv);
}

static void
native_stop_service (JNIEnv *env, jobject thiz)
{
	if (service)
	  pthread_kill (service, SIGINT);
}

#endif

