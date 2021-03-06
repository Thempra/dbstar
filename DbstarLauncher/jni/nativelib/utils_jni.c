#include "jni.h"
#include "JNIHelp.h"

#include <linux/fb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <cutils/properties.h>
#include <android/log.h>

#include "utils_jni.h"

#define LOG_TAG "NativeJNI"
#define MIN(x,y) ((x)<(y)?(x):(y))

#if 1
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#define  LOGD(...)
#define  LOGE(...)
#endif




#ifdef __cplusplus
extern "C"
{
#endif

// jstring to char*
static char* jstring2string(JNIEnv* env, jstring jstr)
{
	char* rtn = NULL;

	jclass clsstring = (*env)->FindClass(env, "java/lang/String");

	jstring strencode = (*env)->NewStringUTF(env, "utf-8");
	jmethodID mid = (*env)->GetMethodID(env, clsstring, "getBytes", "(Ljava/lang/String;)[B");
	jbyteArray barr= (jbyteArray)(*env)->CallObjectMethod(env, jstr, mid, strencode);
	jsize alen = (*env)->GetArrayLength(env, barr);
	jbyte* ba = (*env)->GetByteArrayElements(env, barr, JNI_FALSE);

	if(alen > 0)
	{
		rtn = (char*)malloc(alen + 1);
		memcpy(rtn, ba, alen);
		rtn[alen] = 0;
	}

	(*env)->ReleaseByteArrayElements(env, barr, ba, 0);

	return rtn;
}

/*
 * Class:     com_dbstar_util_NativeUtil
 * Method:    writeFile
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_dbstar_util_NativeUtil_writeFile
	(JNIEnv *env, jclass clazz, jstring file, jstring str)
{
	char* fileName = jstring2string(env, file);
	char* cstr = jstring2string(env, str);

	int fd = 0;
	int count = 0;

	LOGI("fileName is: %s\n", fileName);
	LOGI("cstr is: %s\n", cstr);

	fd = open(fileName, O_WRONLY);
	if(fd == -1)
	{
		LOGI("open file %s failure!\n", fileName);

		return -1;
	}

	count = write(fd, cstr, strlen(cstr));

	LOGI("count of written chars is: %d\n", count);

	close(fd);

	if (fileName != NULL) {
		free(fileName);
	}

	if (cstr != NULL) {
		free(cstr);
	}

	return count;
}

/*
 * Class:     com_dbstar_util_NativeUtil
 * Method:    writeFile
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_dbstar_util_NativeUtil_runSystem
	(JNIEnv *env, jclass clazz, jstring command)
{
	int ret = 0;
	char* cmd = jstring2string(env, command);

	ret = system(cmd);
	LOGD("command: %s, ret=%d\n", cmd, ret);

	return ret;
}

static JNINativeMethod gMethods[] = {
	{"writeFile", "(Ljava/lang/String;Ljava/lang/String;)I", (void*)Java_com_dbstar_util_NativeUtil_writeFile},
	{"runSystem", "(Ljava/lang/String;)I", (void*)Java_com_dbstar_util_NativeUtil_runSystem}
};

int registerNativeMethods(JNIEnv* env,
                          const char* className,
                          const JNINativeMethod* gMethods,
                          int numMethods)
{
	jclass clazz;

	LOGI("Registering %s natives\n", className);
	clazz = (*env)->FindClass(env, className);
	if (clazz == NULL) {
		LOGE("Native registration unable to find class '%s'\n", className);
		return -1;
	}
	if ((*env)->RegisterNatives(env, clazz, gMethods, numMethods) < 0) {
		LOGE("RegisterNatives failed for '%s'\n", className);
		return -1;
	}
	return 0;
}

int register_com_dbstar_util_NativeUtil(JNIEnv *env)
{
	const char* const kClassPathName = "com/dbstar/util/NativeUtil";

	return registerNativeMethods(env, kClassPathName , gMethods, sizeof(gMethods) / sizeof(gMethods[0]));
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	jint result = -1;
	JNIEnv* env = NULL;

	if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		LOGE("GetEnv failed!");
		return -1;
	}

	LOGI("GetEnv ok");
	result = JNI_VERSION_1_4;
	register_com_dbstar_util_NativeUtil(env);
	return result;
}

#ifdef __cplusplus
}
#endif

