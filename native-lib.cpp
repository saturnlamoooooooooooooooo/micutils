/*
   Source Code For "libmicutils.so"
   Feel Free To Give It Out, I Made This Public So People Dont Think That The Lib Is A Virus
   Used For "Scream To Scare"
*/

#include <jni.h>
#include <string>
#include <aaudio/AAudio.h>
#include <algorithm>
#include <cmath>
#include <atomic>
#include <vector>

struct MicContext
{
    AAudioStream* stream = nullptr;
    std::atomic<float> last_intensity{0.0f};

    void updateIntensity(const float* data, int32_t numFrames)
    {
        float maxAbs = 0.0f;
        for (int32_t i = 0; i < numFrames; ++i)
        {
            maxAbs = std::max(maxAbs, std::abs(data[i]));
        }
        last_intensity.store(maxAbs, std::memory_order_relaxed);
    }
};

static MicContext* gContext = nullptr;

aaudio_data_callback_result_t micDataCallback(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames)
{
    auto* context = static_cast<MicContext*>(userData);
    context->updateIntensity(static_cast<const float*>(audioData), numFrames);

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

extern "C"
{
    __attribute__((visibility("default")))
    bool Mic_Start()
    {
        if (gContext != nullptr) return true;

        gContext = new MicContext();

        AAudioStreamBuilder* builder;
        aaudio_result_t result = AAudio_createStreamBuilder(&builder);
        if (result != AAUDIO_OK) return false;

        AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
        AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
        AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
        AAudioStreamBuilder_setChannelCount(builder, 1);
        AAudioStreamBuilder_setDataCallback(builder, micDataCallback, gContext);
        AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);

        result = AAudioStreamBuilder_openStream(builder, &gContext->stream);
        AAudioStreamBuilder_delete(builder);

        if (result != AAUDIO_OK)
        {
            delete gContext;
            gContext = nullptr;
            return false;
        }

        result = AAudioStream_requestStart(gContext->stream);
        if (result != AAUDIO_OK)
        {
            AAudioStream_close(gContext->stream);
            delete gContext;
            gContext = nullptr;
            return false;
        }

        return true;
    }

    __attribute__((visibility("default")))
    void Mic_Stop()
    {
        if (gContext != nullptr)
        {
            if (gContext->stream != nullptr)
            {
                AAudioStream_requestStop(gContext->stream);
                AAudioStream_close(gContext->stream);
            }
            delete gContext;
            gContext = nullptr;
        }
    }

    __attribute__((visibility("default")))
    float Mic_GetIntensity()
    {
        if (gContext != nullptr)
        {
            return gContext->last_intensity.load(std::memory_order_relaxed);
        }
        return 0.0f;
    }

    JNIEXPORT jstring JNICALL
    Java_com_solar_micutils_MainActivity_stringFromJNI(JNIEnv* env, jobject /* this */)
    {
        std::string hello = "Hello From C++!";
        return env->NewStringUTF(hello.c_str());
    }
}