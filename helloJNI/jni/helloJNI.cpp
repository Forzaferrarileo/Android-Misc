#include <string.h>
#include <jni.h>

extern "C" {

	jstring
	Java_com_example_hellojni_MainActivity_getMessage(JNIEnv *env, jobject obj)
	{
		return env->NewStringUTF("Ciao mondo da una stringa nativa ");
	}

}
/*
	 * Nominiamo la funzione secondo un regola precisa :
	 * Java_package_classe_metodo
	 * ogni spazio deve essere sostituito con un underscore
	 */
//definiamo il valore di ritorno per java : stringa
