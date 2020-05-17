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

    const char* string = "#include <string.h>"
                         "#include <stdlib.h>"
                         "#include <math.h>"
                         "double internalTestFunction(double x) {"
                         "  return x+1;"
                         "}"
                         "int test1() {"
                         "  return (int) internalTestFunction(4);"
                         "}"
                         "float test2() {"
                         "  return (float) internalTestFunction(4);"
                         "}"
                         "float* test2Ptr() {"
                         "  float *resultPtr = malloc(sizeof(float));"
                         "  *resultPtr = (float) internalTestFunction(4);"
                         "  return resultPtr;"
                         "}"
                         "double test3() {"
                         "  return internalTestFunction(4);"
                         "}"
                         "double* test3Ptr() {"
                         "  double *resultPtr = malloc(sizeof(double));"
                         "  *resultPtr = internalTestFunction(4);"
                         "  return resultPtr;"
                         "}"
                         "char* test4(const char* string) {"
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
        int test1Result = -1;
        float test2Result = -1;
        float *test2PtrResult = nullptr;
        double test3Result = -1;
        double *test3PtrResult = nullptr;
        char *test4Result = nullptr;

        tcc_set_output_type(tccState, TCC_OUTPUT_MEMORY);

        tcc_compile_string(tccState, string);
        tcc_relocate(tccState, TCC_RELOCATE_AUTO);

        int(*test1Func)();
        test1Func = reinterpret_cast<decltype(test1Func)>(tcc_get_symbol(tccState, "test1"));
        float(*test2Func)();
        test2Func = reinterpret_cast<decltype(test2Func)>(tcc_get_symbol(tccState, "test2"));
        float*(*test2PtrFunc)();
        test2PtrFunc = reinterpret_cast<decltype(test2PtrFunc)>(tcc_get_symbol(tccState, "test2Ptr"));
        double(*test3Func)();
        test3Func = reinterpret_cast<decltype(test3Func)>(tcc_get_symbol(tccState, "test3"));;
        double*(*test3PtrFunc)();
        test3PtrFunc = reinterpret_cast<decltype(test3PtrFunc)>(tcc_get_symbol(tccState, "test3Ptr"));
        char*(*test4Func)(const char*);
        test4Func = reinterpret_cast<decltype(test4Func)>(tcc_get_symbol(tccState, "test4"));

        if (test1Func != nullptr)
            test1Result = test1Func();
        if (test2Func != nullptr)
            test2Result = test2Func();
        if (test2PtrFunc != nullptr)
            test2PtrResult = test2PtrFunc();
        if(test3Func != nullptr)
            test3Result = test3Func();
        if(test3PtrFunc != nullptr)
            test3PtrResult = test3PtrFunc();
        if(test4Func != nullptr)
            test4Result = test4Func("Hallo Welt");

        tcc_delete(tccState);

        if (errorString.empty()) {
            errorString.append("Test 1 Success if 5: ").append(std::to_string(test1Result))
                       .append("\nTest 2 Success if 5: ").append(std::to_string(test2Result))
                       .append("\nTest 2 Pointer Success if 5: ").append(std::to_string(*test2PtrResult))
                       .append("\nTest 3 Success if 5: ").append(std::to_string(test3Result))
                       .append("\nTest 3 Pointer Success if 5: ").append(std::to_string(*test3PtrResult))
                       .append("\nTest 4 Success if \"Hallo Welt!\": ").append(test4Result);
        }
        std::free(test2PtrResult);
        std::free(test3PtrResult);
        std::free(test4Result);
    } else if(errorString.empty()) {
        errorString.append("Errors Occured!");
    }
    return env->NewStringUTF(errorString.c_str());
}
