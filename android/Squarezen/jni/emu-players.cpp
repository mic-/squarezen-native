#include <jni.h>
#include <android/log.h>
#include <string>
#include <GbsPlayer.h>
#include <NsfPlayer.h>
#include <VgmPlayer.h>
#include <YmPlayer.h>

MusicPlayer *player = NULL;
JavaVM *gJavaVM;


extern "C"
{
JNIEXPORT void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Prepare(JNIEnv *ioEnv, jobject ioThis, jstring filePath);
JNIEXPORT void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Close(JNIEnv *ioEnv, jobject ioThis);
//JNIEXPORT void JNICALL Java_org_jiggawatt_squarezen_MainActivity_GetState(JNIEnv *ioEnv, jobject ioThis, jbyteArray state);
JNIEXPORT void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Run(JNIEnv *ioEnv, jobject ioThis, jint numSamples, jobject byteBuffer);
};


jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv *env;
	gJavaVM = vm;
	if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
		return -1;
	}
	return JNI_VERSION_1_6;
}


void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Close(JNIEnv *ioEnv, jobject ioThis)
{
	if (player) {
		delete player;
		player = NULL;
	}
}


void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Prepare(JNIEnv *ioEnv, jobject ioThis, jstring filePath)
{
	uint32_t  i;
	uint32_t sampleBytes;
	FILE *file;
	size_t fileSize, readBytes;
	static char temp[256];

	const char *str;
	str = ioEnv->GetStringUTFChars(filePath, NULL);
	if (str == NULL) {
		__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", "failed to convert jstring to char*");
		return;
	}

	if (player) {
		player->Reset();
		delete player;
	}

	//__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", str);

	if (strstr(str, ".gbs") || strstr(str, ".GBS")) {
		player = new GbsPlayer;
	} else if (strstr(str, ".vgm") || strstr(str, ".VGM") || strstr(str, ".vgz") || strstr(str, ".VGZ")) {
		player = new VgmPlayer;
	} else if (strstr(str, ".ym") || strstr(str, ".YM")) {
		player = new YmPlayer;
	} else {
		__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", "unrecognized file type");
	}
	std::string *s = new std::string(str);
	if (player->Prepare(*s)) {
		__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", "failed to prepare player");
		ioEnv->ReleaseStringUTFChars(filePath, str);
		return;
	}

	ioEnv->ReleaseStringUTFChars(filePath, str);
}



JNIEXPORT void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Run(JNIEnv *ioEnv, jobject ioThis, jint numSamples, jobject byteBuffer)
{
    int16_t *buffer;
    static char temp[128];

    buffer = (int16_t*)(ioEnv->GetDirectBufferAddress(byteBuffer));
    if (buffer == NULL) {
    	__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", "failed to get NIO buffer address");
    	return;
    }

    if (player) {
    	player->Run(numSamples, buffer);
    }
}
