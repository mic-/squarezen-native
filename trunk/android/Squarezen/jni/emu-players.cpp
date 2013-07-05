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
#include <pthread.h>

static MusicPlayer *player = NULL;
JavaVM *gJavaVM;

static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

static SLObjectItf outputMixObject = NULL;

static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLVolumeItf bqPlayerVolume;

#define BUFFER_SIZE_BYTES (2048*2*2)
#define NUM_BUFFERS 4

static pthread_mutex_t playerMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t bufferFillCond = PTHREAD_COND_INITIALIZER;
static pthread_t bufferFillThread;
static bool playing = false;

static short *pcmBuffer[NUM_BUFFERS];
static int bufferToEnqueue, bufferToFill;


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


void *BufferFillThread(void *parm)
{
	static char temp[128];

	while (playing) {
		pthread_mutex_lock(&playerMutex);
		pthread_cond_wait(&bufferFillCond, &playerMutex);

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

	    bufferToFill++;
	    bufferToFill = (bufferToFill < NUM_BUFFERS) ? bufferToFill : 0;

	    pthread_mutex_unlock(&playerMutex);
	}

	return NULL;
}


// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	static char temp[128];

	pthread_mutex_lock(&playerMutex);

	assert(bq == bqPlayerBufferQueue);
    assert(NULL == context);

   	SLresult result;
   	result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, pcmBuffer[bufferToEnqueue], BUFFER_SIZE_BYTES);
   	// the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT, which for this code example would indicate a programming error
   	assert(SL_RESULT_SUCCESS == result);
   	(void)result;

    bufferToEnqueue++;
    bufferToEnqueue = (bufferToEnqueue < NUM_BUFFERS) ? bufferToEnqueue : 0;

	if (player) {
    	struct timeval t1, t2;
    	gettimeofday(&t1, NULL);
    	player->Run(BUFFER_SIZE_BYTES/4, pcmBuffer[bufferToFill]);
    	gettimeofday(&t2, NULL);
#ifdef HAVE_NEON
    	sprintf(temp, "Player::Run took %d us for %d samples", t2.tv_usec-t1.tv_usec, BUFFER_SIZE_BYTES/4);
    	//__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", temp);
#endif
    }

    bufferToFill++;
    bufferToFill = (bufferToFill < NUM_BUFFERS) ? bufferToFill : 0;

	pthread_mutex_unlock(&playerMutex);

   	//pthread_cond_signal(&bufferFillCond);
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
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, NUM_BUFFERS};
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
	pthread_mutex_lock(&playerMutex);

	if (player) {
		delete player;
		player = NULL;
	}

	pthread_mutex_unlock(&playerMutex);

	/*if (playing) {
		__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", "signalling bufferFillCond");
		pthread_cond_signal(&bufferFillCond);
		__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", "joining bufferFillThread");
		playing = false;
		pthread_join(bufferFillThread, NULL);
		__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", "joined thread");
	}*/
}


void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Exit(JNIEnv *ioEnv, jobject ioThis)
{
	pthread_mutex_lock(&playerMutex);

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

	for (int i = 0; i < NUM_BUFFERS; i++) {
		delete [] pcmBuffer[i];
	}

	pthread_mutex_unlock(&playerMutex);

	/*if (playing) {
		pthread_cond_signal(&bufferFillCond);
		playing = false;
		pthread_join(bufferFillThread, NULL);
	}*/
}


void JNICALL Java_org_jiggawatt_squarezen_MainActivity_Prepare(JNIEnv *ioEnv, jobject ioThis, jstring filePath)
{
	uint32_t  i;
	uint32_t sampleBytes;
	FILE *file;
	size_t fileSize, readBytes;
	static char temp[256];

	// Assumes that Close() has been called unless this is the first time Prepare() is called

	const char *str;
	str = ioEnv->GetStringUTFChars(filePath, NULL);
	if (str == NULL) {
		__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", "failed to convert jstring to char*");
		return;
	}

	pthread_mutex_lock(&playerMutex);

	if (strstr(str, ".gbs") || strstr(str, ".GBS")) {
		player = new GbsPlayer;
	} else if (strstr(str, ".vgm") || strstr(str, ".VGM") || strstr(str, ".vgz") || strstr(str, ".VGZ")) {
		player = new VgmPlayer;
	} else if (strstr(str, ".ym") || strstr(str, ".YM")) {
		player = new YmPlayer;
	} else {
		__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", "unrecognized file type");
		ioEnv->ReleaseStringUTFChars(filePath, str);
		pthread_mutex_unlock(&playerMutex);
		return;
	}
	std::string *s = new std::string(str);
	if (player->Prepare(*s)) {
		__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", "failed to prepare player");
		ioEnv->ReleaseStringUTFChars(filePath, str);
		delete player;
		player = NULL;
		pthread_mutex_unlock(&playerMutex);
		return;
	}

	ioEnv->ReleaseStringUTFChars(filePath, str);

	if (!engineObject) {
		CreateAudioEngine();
		CreateBufferQueueAudioPlayer();
		for (int i = 0; i < NUM_BUFFERS; i++) {
			pcmBuffer[i] = new short[BUFFER_SIZE_BYTES/2];
		}
		if (!pcmBuffer[0] || !pcmBuffer[NUM_BUFFERS-1]) {
			__android_log_write(ANDROID_LOG_VERBOSE, "squarezen", "failed to allocate PCM buffers");
		}
	}
	for (int i = 0; i < NUM_BUFFERS; i++) {
		player->Run(BUFFER_SIZE_BYTES/4, pcmBuffer[i]);
	}

	playing = true;
	//pthread_create(&bufferFillThread, NULL, BufferFillThread, NULL);

	SLresult result;
	for (int i = 0; i < NUM_BUFFERS-1; i++) {
		result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, pcmBuffer[i], BUFFER_SIZE_BYTES);
	}

	bufferToEnqueue = NUM_BUFFERS-1;
	bufferToFill = 0;

	pthread_mutex_unlock(&playerMutex);
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
