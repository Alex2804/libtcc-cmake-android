#include <jni.h>

#include <string>
#include <cmath>

#include <android/asset_manager_jni.h>

#include "tcc/libtcc_ext.h"

std::string jstring2string(JNIEnv *env, jstring jStr) {
    if (!jStr)
        return "";

    const jclass stringClass = env->GetObjectClass(jStr);
    const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
    const jbyteArray stringJbytes = (jbyteArray) env->CallObjectMethod(jStr, getBytes, env->NewStringUTF("UTF-8"));

    size_t length = (size_t) env->GetArrayLength(stringJbytes);
    jbyte* pBytes = env->GetByteArrayElements(stringJbytes, NULL);

    std::string ret = std::string((char *)pBytes, length);
    env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

    env->DeleteLocalRef(stringJbytes);
    env->DeleteLocalRef(stringClass);
    return ret;
}

std::string errorString;
void errorFunction(void* opaque, const char* errorString)
{
    ::errorString.append(errorString).append("\n");
}
extern "C" JNIEXPORT jstring JNICALL
Java_de_alex2804_libtcctest_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */,
        jobject assetManager,
        jstring filesPath)
{
    AAssetManager* aAssetManager = AAssetManager_fromJava(env, assetManager);
    atcc_set_asset_manager(aAssetManager);

    const char* string = "#include <math.h>"
                         "#include <string.h>"
                         "#include <stdlib.h>"
                         "double test() {"
                         "  return pow(4, 4);"
                         "}"
                         "char* test2(const char* string) {"
                         "  size_t len = strlen(string);"
                         "  char* copy = (void*) malloc(sizeof(char) * (len + 2));"
                         "  memcpy(copy, string, len + 1);"
                         "  strcat(copy, \"!\");"
                         "  return copy;"
                         "}";

    std::string basePath = jstring2string(env, filesPath);
    std::string destPath = basePath + "lib";
    atcc_set_libtcc1_dest_path(destPath.c_str());
    atcc_set_libtcc1_obj_path(destPath.c_str());
    atcc_set_error_func(nullptr, errorFunction);
    atcc_build_libtcc1();

    TCCState *tccState = atcc_new();
    if(tccState != NULL) {
        double testResult = -1;
        char* test2Result = nullptr;

        tcc_set_output_type(tccState, TCC_OUTPUT_MEMORY);

        tcc_compile_string(tccState, string);
        tcc_relocate(tccState, TCC_RELOCATE_AUTO);

        double (*testFunc)();
        testFunc = reinterpret_cast<double (*)()>(tcc_get_symbol(tccState, "test"));
        char* (*test2Func)(const char*);
        test2Func = reinterpret_cast<char*(*)(const char*)>(tcc_get_symbol(tccState, "test2"));

        if (testFunc != nullptr)
            testResult = testFunc(); // 256
        if(test2Func != nullptr)
            test2Result = test2Func("Hallo Welt");

        tcc_delete(tccState);

        if (errorString.empty())
            errorString.append("Test Success if 256: ").append(std::to_string(testResult)).append("\nTest2 Success if \"Hallo Welt!\": ").append(test2Result);
        std::free(test2Result);
    } else if(errorString.empty()) {
        errorString.append("Errors Occured!");
    }
    return env->NewStringUTF(errorString.c_str());
}
