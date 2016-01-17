#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "tdigestx.hxx"
#include <jni.h>

static JavaVM* create_jvm()
{
	// See http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/invocation.html#wp9502

	JavaVM* jvm = 0; // denotes a Java VM
	JNIEnv* env = 0; // pointer to native method interface
	JavaVMInitArgs args; // JDK/JRE 6 VM initialization environment
	args.version = JNI_VERSION_1_6;
#ifdef NDEBUG
	args.nOptions = 1;
	JavaVMOption options[1];
	options[0].optionString = const_cast<char*>("-Djava.class.path=.");
#else
	args.nOptions = 2;
	JavaVMOption options[2];
	options[0].optionString = const_cast<char*>("-Djava.class.path=.");
	options[1].optionString = const_cast<char*>("-Xcheck:jni");
#endif // NDEBUG
	args.options = options;
	args.ignoreUnrecognized = JNI_FALSE;

	// Load and initialize a Java VM, return a JNI interface pointer in env
	JNI_CreateJavaVM(&jvm, (void **)&env, &args);

	return jvm;
}

static void destroy_jvm(JavaVM* jvm)
{
	jvm->DestroyJavaVM();
}

static void jni_check_exception(JNIEnv * env) {
    if (!env) {
        abort();
    }
    const jthrowable e = env->ExceptionOccurred();
    if (e) {
        env->ExceptionClear();
    }
}

static JNIEnv* jni_get_env(JavaVM* p_jvm)
{
    JNIEnv* p_env = 0;
    int getEnvStat = p_jvm->GetEnv((void **)&p_env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
		std::cout << "GetEnv: not attached" << std::endl;
		if (p_jvm->AttachCurrentThread((void **) &p_env, 0) != 0) {
			std::cout << "Failed to attach" << std::endl;
    	}
    } else if (getEnvStat == JNI_OK) {

    } else if (getEnvStat == JNI_EVERSION) {
    	std::cout << "GetEnv: version not supported" << std::endl;
    }

	return p_env;
}
/*
void callback(int val) {
    JNIEnv * g_env;
    // double check it's all ok
    int getEnvStat = g_vm->GetEnv((void **)&g_env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
		std::cout << "GetEnv: not attached" << std::endl;
		if (g_vm->AttachCurrentThread((void **) &g_env, NULL) != 0) {
			std::cout << "Failed to attach" << std::endl;
    	}
    } else if (getEnvStat == JNI_OK) {

    } else if (getEnvStat == JNI_EVERSION) {
    	std::cout << "GetEnv: version not supported" << std::endl;
    }

    g_env->CallVoidMethod(g_obj, g_mid, val);

    if (g_env->ExceptionCheck()) {
    	g_env->ExceptionDescribe();
    }

    g_vm->DetachCurrentThread();
}
*/
 
int main()
{
	const std::string fname = "data/test.csv";
	const double prob = 0.95;

  	JavaVM* jvm = create_jvm();
	tdigestx::TDigestProxy* proxy = new tdigestx::TDigestProxy(jvm);
	
	proxy->init();
#ifndef NDEBUG
	jni_check_exception(jni_get_env(jvm));
#endif

//	proxy->add(1.0);
//	jni_check_exception(jni_get_env(jvm));
//	proxy->add(1.5);
//	jni_check_exception(jni_get_env(jvm));
//	proxy->add(2.0);
//	jni_check_exception(jni_get_env(jvm));

    std::ifstream ifs(fname.c_str());
    if (!ifs)
    {
        throw std::runtime_error("[ERROR] Unable to open data file");
    }

    while (ifs.good() && ifs.is_open())
    {
        std::string line;

        std::getline(ifs, line);

        double rt = -1;
        std::istringstream iss(line);
        iss >> rt;

        if (rt >= 0)
        {
            proxy->add(rt);
#ifndef NDEBUG
			jni_check_exception(jni_get_env(jvm));
#endif

            std::cout << proxy->quantile(prob) << std::endl;
#ifndef NDEBUG
			jni_check_exception(jni_get_env(jvm));
#endif
        }
    }

    ifs.close();

	std::cout << "Final Quantile: "  <<  proxy->quantile(prob) << std::endl;

	destroy_jvm(jvm);

	return 0;	
}
