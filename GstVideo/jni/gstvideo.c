#include <string.h>
#include <stdint.h>
#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <pthread.h>

GST_DEBUG_CATEGORY_STATIC (debug_category);
#define GST_CAT_DEFAULT debug_category

#if GLIB_SIZEOF_VOID_P == 8
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(*env)->GetLongField (env, thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)data)
#else
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(jint)(*env)->GetLongField (env, thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)(jint)data)
#endif

typedef struct _CustomData {
	jobject app;
	GstElement *pipeline;
	GMainContext *context;
	GMainLoop *main_loop;
	gboolean initialized;
	GstElement *video_sink;
	ANativeWindow *native_window;
} CustomData;

static pthread_t gst_app_thread;
static pthread_key_t current_jni_env;
static JavaVM *java_vm;
static jfieldID custom_data_field_id;
static jmethodID set_message_method_id;
static jmethodID on_gstreamer_initialized_method_id;

static JNIEnv *attach_current_thread (void) {
	JNIEnv *env;
	JavaVMAttachArgs args;

	GST_DEBUG ("Collegando il thread %p", g_thread_self ());
	args.version = JNI_VERSION_1_4;
	args.name = NULL;
	args.group = NULL;

	if ((*java_vm)->AttachCurrentThread (java_vm, &env, &args) < 0) {
		GST_ERROR ("Collegamento thread fallito");
		return NULL;
	}

	return env;
}

static void detach_current_thread (void *env) {
	GST_DEBUG ("Scollegando il thread %p", g_thread_self ());
	(*java_vm)->DetachCurrentThread (java_vm);
}

static JNIEnv *get_jni_env (void) {
	JNIEnv *env;

	if ((env = pthread_getspecific (current_jni_env)) == NULL) {
		env = attach_current_thread ();
		pthread_setspecific (current_jni_env, env);
	}

	return env;
}

static void check_initialization_complete (CustomData *data) {
	JNIEnv *env = get_jni_env ();
	if (!data->initialized && data->native_window && data->main_loop) {
		GST_DEBUG ("Inizializzazione completata, informo l'app. Finestra nativa:%p main_loop:%p", data->native_window, data->main_loop);

		/* Il loop principale è in esecuzione e abbiamo ricevuto la finestra per il video. informalo */
		gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (data->video_sink), (guintptr)data->native_window);

		(*env)->CallVoidMethod (env, data->app, on_gstreamer_initialized_method_id);
		if ((*env)->ExceptionCheck (env)) {
			GST_ERROR ("Chiamata al metodo java fallita");
			(*env)->ExceptionClear (env);
		}
		data->initialized = TRUE;
	}
}

static jboolean gst_native_class_init (JNIEnv* env, jclass klass) {
	custom_data_field_id = (*env)->GetFieldID (env, klass, "native_custom_data", "J");
	on_gstreamer_initialized_method_id = (*env)->GetMethodID (env, klass, "onGStreamerInitialized", "()V");

	if (!custom_data_field_id || !on_gstreamer_initialized_method_id) {
		__android_log_print (ANDROID_LOG_ERROR, "db" ,"Non tutti i metodi necessari sono implementati");
		return JNI_FALSE;
	}
	return JNI_TRUE;
}

static void gst_native_destroy (JNIEnv* env, jobject thiz) {
	CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
	if (!data) return;
	GST_DEBUG ("Uscendo dal loop principale...");
	g_main_loop_quit (data->main_loop);
	GST_DEBUG ("Aspettando che il thread termini...");
	pthread_join (gst_app_thread, NULL);
	GST_DEBUG ("Eliminando il riferimento globale dell'app a %p", data->app);
	(*env)->DeleteGlobalRef (env, data->app);
	GST_DEBUG ("Liberando CustomData a %p", data);
	g_free (data);
	SET_CUSTOM_DATA (env, thiz, custom_data_field_id, NULL);
	GST_DEBUG ("Distruzione completata");
}

static void gst_native_play (JNIEnv* env, jobject this) {
	CustomData *data = GET_CUSTOM_DATA (env, this, custom_data_field_id);
	if (!data) return;
	GST_DEBUG ("Impostato lo stato su Riproduzione");
	gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
}

static void gst_native_pause (JNIEnv* env, jobject this) {
	CustomData *data = GET_CUSTOM_DATA (env, this, custom_data_field_id);
	if (!data) return;
	GST_DEBUG ("Impostando lo stato su Pausa");
	gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
}

static void gst_native_surface_init (JNIEnv *env, jobject this, jobject surface) {
	CustomData *data = GET_CUSTOM_DATA (env, this, custom_data_field_id);
	if (!data) return;
	ANativeWindow *new_native_window = ANativeWindow_fromSurface(env, surface);
	GST_DEBUG ("Superifce ricevuta %p (finestra nativa %p)", surface, new_native_window);

	if (data->native_window) {
		ANativeWindow_release (data->native_window);

		if (data->native_window == new_native_window) {
			GST_DEBUG ("La nuova finestra nativa è la stessa di quella precedente %p", data->native_window);

			if (data->video_sink) {
				gst_video_overlay_expose(GST_VIDEO_OVERLAY (data->video_sink));
				gst_video_overlay_expose(GST_VIDEO_OVERLAY (data->video_sink));
			}
			return;
		} else {
			GST_DEBUG ("Rilasciata la finestra precedente %p", data->native_window);
			data->initialized = FALSE;
		}
	}

	data->native_window = new_native_window;

  check_initialization_complete (data);
}

static void gst_native_surface_destroy (JNIEnv *env, jobject this) {
	CustomData *data = GET_CUSTOM_DATA (env, this, custom_data_field_id);
	if (!data) return;
	GST_DEBUG ("Rilasciando la finestra nativa %p", data->native_window);

	if (data->video_sink) {
		gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (data->video_sink), (guintptr)NULL);
		gst_element_set_state (data->pipeline, GST_STATE_READY);
	}

	ANativeWindow_release (data->native_window);
	data->native_window = NULL;
	data->initialized = FALSE;
}

static void error_cb (GstBus *bus, GstMessage *msg, CustomData *data) {
	GError *err;
	gchar *debug_info;
	gchar *message_string;

	gst_message_parse_error (msg, &err, &debug_info);
	message_string = g_strdup_printf ("Errore ricevuto dall'elemento: %s: %s", GST_OBJECT_NAME (msg->src), err->message);
	g_clear_error (&err);
	g_free (debug_info);
	//__android_log_print(ANDROID_LOG_ERROR, "db" ,message_string);
	g_free (message_string);
	gst_element_set_state (data->pipeline, GST_STATE_NULL);
}

static void state_changed_cb (GstBus *bus, GstMessage *msg, CustomData *data) {
	GstState old_state, new_state, pending_state;
	gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);

	/* Fai attenzione solo ai messaggi provenienti dalla pipeline , non dai suoi figli */
	if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
		gchar *message = g_strdup_printf("Stato cambiato in %s", gst_element_state_get_name(new_state));
		//__android_log_print(ANDROID_LOG_DEBUG,"db" , message);
		g_free (message);
	}
}


static void *app_function (void *userdata) {
	JavaVMAttachArgs args;
	GstBus *bus;
	CustomData *data = (CustomData *)userdata;
	GSource *bus_source;
	GError *error = NULL;

	GST_DEBUG ("Creando la pipeline in CustomData a :  %p", data);

	/* Crea il nostro contesto glib e fallo diventare il principale */
	data->context = g_main_context_new ();
	g_main_context_push_thread_default(data->context);

	/* Costruisci la pipeline */
	data->pipeline = gst_parse_launch("udpsrc port=5000 caps=\"application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264\" ! rtph264depay ! h264parse ! decodebin ! glimagesink sync=false" , &error);
	if (error) {
		gchar *message = g_strdup_printf("Impossibile costruire la pipeline: %s", error->message);
		g_clear_error (&error);
		//___android_log_print(ANDROID_LOG_ERROR,message);
		g_free (message);
		return NULL;
	}

	/* Imposta la pipeline su pronta , in modo da accettare un gestore di una finestra se fornito */
	gst_element_set_state(data->pipeline, GST_STATE_READY);

	data->video_sink = gst_bin_get_by_interface(GST_BIN(data->pipeline), GST_TYPE_VIDEO_OVERLAY);
	if (!data->video_sink) {
		GST_ERROR ("Impossibili avere il video sink");
		return NULL;
	}

	/* Istruisci il bus ad emettere messaggi ad ogni segnale interessante a noi */
	bus = gst_element_get_bus (data->pipeline);
	bus_source = gst_bus_create_watch (bus);
	g_source_set_callback (bus_source, (GSourceFunc) gst_bus_async_signal_func, NULL, NULL);
	g_source_attach (bus_source, data->context);
	g_source_unref (bus_source);
	g_signal_connect (G_OBJECT (bus), "message::error", (GCallback)error_cb, data);
	g_signal_connect (G_OBJECT (bus), "message::state-changed", (GCallback)state_changed_cb, data);
	gst_object_unref (bus);

	/* Crea un loop glib e fallo partire */
	GST_DEBUG ("Entrando nel loop principale... (CustomData:%p)", data);
	data->main_loop = g_main_loop_new (data->context, FALSE);
	check_initialization_complete (data);
	g_main_loop_run (data->main_loop);
	GST_DEBUG ("Exited main loop");
	g_main_loop_unref (data->main_loop);
	data->main_loop = NULL;

	/* Pulisci le risorse */
	g_main_context_pop_thread_default(data->context);
	g_main_context_unref (data->context);
	gst_element_set_state (data->pipeline, GST_STATE_NULL);
	gst_object_unref (data->video_sink);
	gst_object_unref (data->pipeline);

	return NULL;
}

static void gst_native_init (JNIEnv* env, jobject thiz) {
	CustomData *data = g_new0 (CustomData, 1);
	SET_CUSTOM_DATA (env, thiz, custom_data_field_id, data);
	GST_DEBUG_CATEGORY_INIT (debug_category, "db", 0, "GstVideo");
	gst_debug_set_threshold_for_name("tutorial-3", GST_LEVEL_DEBUG);
	GST_DEBUG ("CustomData creata a %p", data);
	data->app = (*env)->NewGlobalRef (env, thiz);
	GST_DEBUG ("Rifermiento globale all'app creato a %p", data->app);
	pthread_create (&gst_app_thread, NULL, &app_function, data);
}

static JNINativeMethod native_methods[] = {
	{ "nativeInit", "()V", (void *) gst_native_init},
	{ "nativeDestroy", "()V", (void *) gst_native_destroy},
	{ "nativePlay", "()V", (void *) gst_native_play},
  	{ "nativePause", "()V", (void *) gst_native_pause},
  	{ "nativeSurfaceInit", "(Ljava/lang/Object;)V", (void *) gst_native_surface_init},
  	{ "nativeSurfaceDestroy", "()V", (void *) gst_native_surface_destroy},
  	{ "nativeClassInit", "()Z", (void *) gst_native_class_init}
};

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
	JNIEnv *env = NULL;

	java_vm = vm;

	if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		return 0;
	}
	jclass class = (*env)->FindClass (env, "com/example/gstvideo/MainActivity");
	(*env)->RegisterNatives (env, class, native_methods, G_N_ELEMENTS(native_methods));

	pthread_key_create (&current_jni_env, detach_current_thread);

	return JNI_VERSION_1_4;
}

