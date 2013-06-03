#include <jni.h>
#include <android/log.h>
#include <string>
#include <GbsPlayer.h>
#include <NsfPlayer.h>
#include <VgmPlayer.h>
#include <YmPlayer.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <sys/time.h>

static MusicPlayer *player = NULL;
JavaVM *gJavaVM;

static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

static SLObjectItf outputMixObject = NULL;

static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
//static SLEffectSendItf bqPlayerEffectSend;
static SLVolumeItf bqPlayerVolume;

#define BUFFER_SIZE_BYTES (3072*2*2)

static short *pcmBuffer[3];
int bufferToEnqueue, bufferToFill;


extern "C"
{
JNIEXPORT void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Prepare(JNIEnv *ioEnv, jobject ioThis, jstring filePath);
JNIEXPORT void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Close(JNIEnv *ioEnv, jobject ioThis);
JNIEXPORT void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Exit(JNIEnv *ioEnv, jobject ioThis);
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


// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	static char temp[128];

    assert(bq == bqPlayerBufferQueue);
    assert(NULL == context);

   	SLresult result;
   	result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, pcmBuffer[bufferToEnqueue], BUFFER_SIZE_BYTES);
   	// the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
   	// which for this code example would indicate a programming error
   	assert(SL_RESULT_SUCCESS == result);
   	(void)result;

    if (player) {
    	struct timeval t1, t2;
    	gettimeofday(&t1, NULL);
    	player->Run(BUFFER_SIZE_BYTES/4, pcmBuffer[bufferToFill]);
    	gettimeofday(&t2, NULL);
#ifdef HAVE_NEON
    	sprintf(temp, "Player::Run took %d us for %d samples", t2.tv_usec-t1.tv_usec, BUFFER_SIZE_BYTES/4);
    	__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", temp);
#endif
    }

    bufferToEnqueue++;
    if (bufferToEnqueue > 2) bufferToEnqueue = 0;
    bufferToFill++;
    if (bufferToFill > 2) bufferToFill = 0;
}



void CreateAudioEngine()
{
    SLresult result;

    // create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // create output mix
    const SLInterfaceID ids[1] = {SL_IID_VOLUME};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
}


// create buffer queue audio player
void CreateBufferQueueAudioPlayer()
{
    SLresult result;

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // create audio player
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
            2, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the player
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the play interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the buffer queue interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
            &bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // register callback on the buffer queue
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the effect send interface
    /*result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
            &bqPlayerEffectSend);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;*/

    // get the volume interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // set the player's state to playing
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
}


void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Close(JNIEnv *ioEnv, jobject ioThis)
{
	if (player) {
		delete player;
		player = NULL;
	}
}


void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Exit(JNIEnv *ioEnv, jobject ioThis)
{
	if (player) {
		delete player;
		player = NULL;
	}

	if (bqPlayerObject) {
		(*bqPlayerObject)->Destroy(bqPlayerObject);
		bqPlayerObject = NULL;
	}

	if (outputMixObject) {
		(*outputMixObject)->Destroy(outputMixObject);
		outputMixObject = NULL;
	}

	if (engineObject) {
		(*engineObject)->Destroy(engineObject);
		engineObject = NULL;
	}

	delete [] pcmBuffer[0];
	delete [] pcmBuffer[1];
	delete [] pcmBuffer[2];
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

	if (!engineObject) {
		CreateAudioEngine();
		CreateBufferQueueAudioPlayer();
		pcmBuffer[0] = new short[BUFFER_SIZE_BYTES/2];
		pcmBuffer[1] = new short[BUFFER_SIZE_BYTES/2];
		pcmBuffer[2] = new short[BUFFER_SIZE_BYTES/2];
		if (!pcmBuffer[0] || !pcmBuffer[2]) {
			__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", "failed to allocate PCM buffers");
		}
	}
	player->Run(BUFFER_SIZE_BYTES/4, pcmBuffer[0]);
	player->Run(BUFFER_SIZE_BYTES/4, pcmBuffer[1]);
	player->Run(BUFFER_SIZE_BYTES/4, pcmBuffer[2]);

	SLresult result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, pcmBuffer[0], BUFFER_SIZE_BYTES);
	result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, pcmBuffer[1], BUFFER_SIZE_BYTES);
	(void) result;

	bufferToEnqueue = 2;
	bufferToFill = 0;
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
