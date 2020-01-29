#include <jni.h>

#include <string>

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
    tcc_set_asset_manager(aAssetManager);

    int result;
    const char* string = "int test() {\n"
                         "  return 4*4*4*4;\n"
                         "}";

    std::string basePath = jstring2string(env, filesPath);
    std::string srcPath = "lib_libtcc1/";
    std::string destPath = basePath + "lib/";
    std::string includePath = "include";
    atcc_set_libtcc1_src_path(srcPath.c_str());
    atcc_set_libtcc1_dest_path(destPath.c_str());
    atcc_set_libtcc1_obj_path(destPath.c_str());
    atcc_set_include_path(includePath.c_str());
    atcc_set_error_func(nullptr, errorFunction);
    result = atcc_build_libtcc1();

    TCCState *tccState = atcc_new();
    if(tccState != NULL) {
        result = tcc_add_sysinclude_path(tccState, includePath.c_str());
        result = tcc_add_library_path(tccState, "/system/lib");
        result = tcc_add_library_path(tccState, "/system/lib64");
        tcc_set_lib_path(tccState, destPath.c_str());

        result = tcc_set_output_type(tccState, TCC_OUTPUT_MEMORY);

        result = tcc_compile_string(tccState, string);
        result = tcc_relocate(tccState, TCC_RELOCATE_AUTO);

        int (*func)();
        func = reinterpret_cast<int (*)()>(tcc_get_symbol(tccState, "test"));

        if (func != nullptr)
            result = func(); // 256

        tcc_delete(tccState);

        if (errorString.empty())
            errorString.append("Success if 256: ").append(std::to_string(result));
    } else if(errorString.empty()) {
        errorString.append("Errors Occured!");
    }
    return env->NewStringUTF(errorString.c_str());
}
