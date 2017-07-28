/**
 * \file dcs/testbed/detail/jni.hpp
 *
 * \brief Utility functions for Java Native Interface (JNI).
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2016 Marco Guazzone (marco.guazzone@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DCS_TESTBED_DETAIL_JNI_HPP
#define DCS_TESTBED_DETAIL_JNI_HPP


#include <boost/noncopyable.hpp>
#include <jni.h>
#include <stdexcept>


#ifndef DCS_TESTBED_JNI_CLASSPATH
# define DCS_TESTBED_JNI_CLASSPATH "."
#endif // DCS_TESTBED_JNI_CLASSPATH


namespace dcs { namespace testbed { namespace detail { namespace jni {

JavaVM* create_jvm()
{
	// See:
	// - http://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/jniTOC.html
	// See also:
	// - http://www.ict.nsc.ru/win/docs/java/tutorial/native1.1/implementing/error.html
	// - http://adamish.com/blog/archives/327
	// - http://www.ibm.com/developerworks/library/j-jni/
	// - http://www.oracle.com/technetwork/java/javase/clopts-139448.html#gbmtq
	JavaVM* jvm = 0; // denotes a Java VM
	JNIEnv* env = 0; // pointer to native method interface
	JavaVMInitArgs args; // JDK/JRE 6 VM initialization environment
	args.version = JNI_VERSION_1_6;
#ifdef DCS_TESTBED_JNI_ENABLE_DIAGNOSTICS
	args.nOptions = 2;
	JavaVMOption options[2];
	//options[0].optionString = const_cast<char*>("-Djava.class.path=.:thirdparty/tdigest");
	options[0].optionString = const_cast<char*>("-Djava.class.path=" DCS_TESTBED_JNI_CLASSPATH);
	options[1].optionString = const_cast<char*>("-Xcheck:jni");
#else
	args.nOptions = 1;
	JavaVMOption options[1];
	//options[0].optionString = const_cast<char*>("-Djava.class.path=.:thirdparty/tdigest");
	options[0].optionString = const_cast<char*>("-Djava.class.path=" DCS_TESTBED_JNI_CLASSPATH);
#endif // DCS_TESTBED_JNI_ENABLE_DIAGNOSTICS
	args.options = options;
	args.ignoreUnrecognized = JNI_FALSE;

	// Load and initialize a Java VM, return a JNI interface pointer in env
	jint res;
	res = JNI_CreateJavaVM(&jvm, (void **)&env, &args);
	if (res != JNI_OK)
	{
		throw std::runtime_error("Unable to create a new Java VM");
	}


	return jvm;
}

inline void destroy_jvm(JavaVM* jvm)
{
	jvm->DestroyJavaVM();
}

inline void check_exception(JNIEnv* p_env)
{
#ifdef DCS_TESTBED_JNI_ENABLE_DIAGNOSTICS
	if (!p_env)
	{
		std::abort();
	}
	const jthrowable e = p_env->ExceptionOccurred();
	if (e)
	{
		p_env->ExceptionClear();
	}
#else
	(void)p_env;
#endif // DCS_TESTBED_JNI_ENABLE_DIAGNOSTICS
}

JNIEnv* get_env(JavaVM* p_jvm)
{
	JNIEnv* p_env = 0;
	jint ret = p_jvm->GetEnv((void **)&p_env, JNI_VERSION_1_6);
	if (ret != JNI_OK)
	{
		if (ret == JNI_EDETACHED)
		{
			//std::cout << "GetEnv: not attached" << std::endl;
			if (p_jvm->AttachCurrentThread((void **) &p_env, 0) != JNI_OK)
			{
				throw std::runtime_error("Failed to attach current thread to Java VM");
			}
		}
		else if (ret == JNI_EVERSION)
		{
			throw std::runtime_error("JNI version not supported");
		}
	}

	return p_env;
}

// Singleton class used to access to the JNI layer
class jni_helper: boost::noncopyable
{
private:
    jni_helper()
	: p_jvm_(0)
    {
		p_jvm_ = create_jvm();
    }

    ~jni_helper()
    {
        destroy_jvm(p_jvm_);
    }


public:
    static jni_helper& get()
    {
        static jni_helper instance;

        return instance;
    }

    void java_vm(JavaVM* p_vm)
    {
        p_jvm_ = p_vm;
    }

    JavaVM* java_vm()
    {
        return p_jvm_;
    }

    JNIEnv* env()
    {
		return get_env(p_jvm_);
    }


private:
    //typedef std::map<std::string, jclass> ClassMap;
    JavaVM* p_jvm_;
    //JNIEnv* p_env_;
}; // jni_helper

}}}} // Namespace dcs::testbed::detail::jni

#endif // DCS_TESTBED_DETAIL_JNI_HPP
