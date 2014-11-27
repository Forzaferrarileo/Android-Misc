#include <string.h>
#include <jni.h>
#include <android/log.h>
#include <gst/gst.h>
#include <pthread.h>

GST_DEBUG_CATEGORY_STATIC (debug_category);
#define GST_CAT_DEFAULT debug_category

/*
 * ATTENZIONE , ROBA CONTORTA , PER CERVELLI SCOPPIATI
 *
 * Queste macro forniscono una funzione per settare e leggere il puntatore della struttura di dati ( CustomData),
 * che viene salvata nell'applicazione java per poi essere usata per delle callback , nel nostro caso per
 * mettere in pausa e in riproduzione il tono audio.
 *
 * Inoltre , convertono il puntatore in long del codice nativo , che può essere a 32bit o 64bit , in un jlong ( long di java),
 * che è sempre a 64bit.
 *
 * Servono per evitare i warning durante la compilazione (32-64bit), ma al momento l'ndk compilando a 32bit , non produce problemi nell'esecuzione
 *
 */
#if GLIB_SIZEOF_VOID_P == 8 //se il sistema è a 64bit
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(*env)->GetLongField (env, thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)data)
#else
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(jint)(*env)->GetLongField (env, thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)(jint)data)
#endif

/* Questa struttura contiene i dati che dovranno essere usati nelle callback */
typedef struct _CustomData {
  jobject app;           /* Riferimento globale all'istanza dell'appilcazione , per chiamare i suoi metodi e i suoi campi. */
  GstElement *pipeline;  /* La pipeline ( comandi audio-video ) */
  GMainContext *context; /* Il contesto in cui Glib deve eseguire il suo loop principale */
  GMainLoop *main_loop;  /* Loop principale di Glib */
  gboolean initialized;  /* Informa l'UI dell'effettiva inizializazzione */
} CustomData;

/* Variabili globali che non variano nell'esecuzione */
static pthread_t gst_app_thread;						// Dichiaro il thread da creare
static pthread_key_t current_jni_env;					// Ambiente jni attuale( dove deve essere eseguito il thread)
static JavaVM *java_vm;									// Puntatore alla macchina virtuale
static jfieldID custom_data_field_id;					// Id del campo java native_custom_data
static jmethodID set_message_method_id;					// Id del metodo setMessage
static jmethodID on_gstreamer_initialized_method_id;	// Id del metodo onGstreamerInitialized


/* Inizializzatore della libreria */
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  JNIEnv *env = NULL;	//ambiente jni = nullo

  java_vm = vm;			//salva il puntatore alla macchina virtuale , passata come argomento

  if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) { 					//Se non è possibile richiamare l'ambiente JNI
    __android_log_print (ANDROID_LOG_ERROR, "tutorial-2", "Could not retrieve JNIEnv");
    return 0; 																			//Termina l'esecuzione
  }
  jclass class = (*env)->FindClass (env, "com/leonardoferraro/gstaudio/MainActivity");	//Trova la classe dell'app
  (*env)->RegisterNatives (env, class, native_methods, G_N_ELEMENTS(native_methods));	//Registra gli elementi nativi

  pthread_key_create (&current_jni_env, detach_current_thread);							//Crea una chiave identificativa del thread (puntatore) , salvandola nel primo argomento , e fornendo un metodo distruttore come secondo

  return JNI_VERSION_1_4;
}// JNI_Onload



/*	Array dei metodi nativi implementati
 *
 *	"funzione java", "firma della funzione(cambia a seconda del valore di ritorno)" , funzione nativa
 *
 */
static JNINativeMethod native_methods[] = {
  { "nativeInit", "()V", (void *) gst_native_init},
  { "nativeDestroy", "()V", (void *) gst_native_finalize},
  { "nativePlay", "()V", (void *) gst_native_play},
  { "nativePause", "()V", (void *) gst_native_pause},
  { "nativeClassInit", "()Z", (void *) gst_native_class_init}
};





/* Inizializzatore statico della codice nativo. Registra gli id dei vari metodi nativi nel codice java */
static jboolean gst_native_class_init (JNIEnv* env, jclass class) {
  custom_data_field_id = (*env)->GetFieldID (env, class, "native_custom_data", "J");
  set_message_method_id = (*env)->GetMethodID (env, class, "setMessage", "(Ljava/lang/String;)V");
  on_gstreamer_initialized_method_id = (*env)->GetMethodID (env, class, "onGStreamerInitialized", "()V");

  if (!custom_data_field_id || !set_message_method_id || !on_gstreamer_initialized_method_id) {		//Controlla se tutto sia registrato , altrimenti termina l'esecuzione e fai il log
    __android_log_print (ANDROID_LOG_ERROR, "tutorial-2", "The calling class does not implement all necessary interface methods");
    return JNI_FALSE;
  }
  return JNI_TRUE;
}



/* Inizializza la struttura di dati e crea un nuovo thread */
static void gst_native_init (JNIEnv* env, jobject this) {
  CustomData *data = g_new0 (CustomData, 1);										//Inizializza la struttura CustomData usando glib e salva il puntatore.
  SET_CUSTOM_DATA (env, this, custom_data_field_id, data);							//Salva l'id del puntatore in java, passandogli l'ambiente , l'istanza, il puntatore della variabile in java , e infine la struttura inizializzata.
  GST_DEBUG_CATEGORY_INIT (debug_category, "tutorial-2", 0, "Android tutorial 2");	//DEBUG
  gst_debug_set_threshold_for_name("tutorial-2", GST_LEVEL_DEBUG);					//DEBUG
  GST_DEBUG ("Created CustomData at %p", data);										//DEBUG
  data->app = (*env)->NewGlobalRef (env, this);										//Dichiara il campo "app" di data con riferimento a questo ambiente e a questa istanza dell'applicazione
  GST_DEBUG ("Created GlobalRef for app object at %p", data->app);					//DEBUG
  pthread_create (&gst_app_thread, NULL, &app_function, data);						//Inizializza il thread "gst_app_thread" . passandogli la funzione iniziale e la struttura "data" come argomenti
}



/* Metodo principale della classe nativa. Questo è eseguito in un suo thread
 *
 * Vengono utilizzate vari metodi di Glib , una libreri C per la gestione della memoria e dei loop,
 * che consente tra le altre cose di captare gli eventi che accadono durante l'esecuzione del loop.
 *
 */
static void *app_function (void *userdata) {

  GstBus *bus;										//Puntatore al bus di Gstreamer
  CustomData *data = (CustomData *)userdata;		//Puntatore alla  struttura passata come argomento ( cioè data , nell'inizializzazione del thread)
  GSource *bus_source;								//Puntatore alla sorgente del bus
  GError *error = NULL;								//Puntatore agli errori che Gstreamer emette

  GST_DEBUG ("Creating pipeline in CustomData at %p", data);			//DEBUG

  // Crea il nostro contesto per Glib e usalo come quello di default //
  data->context = g_main_context_new ();					//Salva il contesto nella struttura "data"
  g_main_context_push_thread_default(data->context);		//Imposta questo contesto e il suo thread come quello di default

  // Costruiamo la pipeline
  data->pipeline = gst_parse_launch("audiotestsrc ! audioconvert ! audioresample ! autoaudiosink", &error); //Costruiamo la pipeline e salviamola in data
  if (error) {
    gchar *message = g_strdup_printf("Unable to build pipeline: %s", error->message);	//Salva il messaggio dell'errore
    g_clear_error (&error);																//Azzera la variabile error
    set_ui_message(message, data);														//Manda il messaggio di errore alla UI
    g_free (message);																	//Elimina lo spazio allocato per "message"
    return NULL;																		//Non ritorna nulla
  }

  /* Imposta il bus ad emettere un segnale ad ogni messaggio in ingresso, e connettilo ad ogni segnale */
  bus = gst_element_get_bus (data->pipeline);														//Prendi il bus per la pipeline dichiarata
  bus_source = gst_bus_create_watch (bus);															//Crea un watchdog per il bus
  g_source_set_callback (bus_source, (GSourceFunc) gst_bus_async_signal_func, NULL, NULL);			//Imposta una callback passandogli il watchdog e la funzione da eseguire
  g_source_attach (bus_source, data->context);														//Collega il bus e il contesto salvato in data
  g_source_unref (bus_source);
  g_signal_connect (G_OBJECT (bus), "message::error", (GCallback)error_cb, data);
  g_signal_connect (G_OBJECT (bus), "message::state-changed", (GCallback)state_changed_cb, data);
  gst_object_unref (bus);

  /* Create a GLib Main Loop and set it to run */
  GST_DEBUG ("Entering main loop... (CustomData:%p)", data);
  data->main_loop = g_main_loop_new (data->context, FALSE);
  check_initialization_complete (data);
  g_main_loop_run (data->main_loop);
  GST_DEBUG ("Exited main loop");
  g_main_loop_unref (data->main_loop);
  data->main_loop = NULL;

  /* Free resources */
  g_main_context_pop_thread_default(data->context);
  g_main_context_unref (data->context);
  gst_element_set_state (data->pipeline, GST_STATE_NULL);
  gst_object_unref (data->pipeline);

  return NULL;
}


/* Register this thread with the VM */
static JNIEnv *attach_current_thread (void) {
  JNIEnv *env;
  JavaVMAttachArgs args;

  GST_DEBUG ("Attaching thread %p", g_thread_self ());
  args.version = JNI_VERSION_1_4;
  args.name = NULL;
  args.group = NULL;

  if ((*java_vm)->AttachCurrentThread (java_vm, &env, &args) < 0) {
    GST_ERROR ("Failed to attach current thread");
    return NULL;
  }

  return env;
}

/* Unregister this thread from the VM */
static void detach_current_thread (void *env) {
  GST_DEBUG ("Detaching thread %p", g_thread_self ());
  (*java_vm)->DetachCurrentThread (java_vm);
}

/* Retrieve the JNI environment for this thread */
static JNIEnv *get_jni_env (void) {
  JNIEnv *env;

  if ((env = pthread_getspecific (current_jni_env)) == NULL) {
    env = attach_current_thread ();
    pthread_setspecific (current_jni_env, env);
  }

  return env;
}

/* Change the content of the UI's TextView */
static void set_ui_message (const gchar *message, CustomData *data) {
  JNIEnv *env = get_jni_env ();
  GST_DEBUG ("Setting message to: %s", message);
  jstring jmessage = (*env)->NewStringUTF(env, message);
  (*env)->CallVoidMethod (env, data->app, set_message_method_id, jmessage);
  if ((*env)->ExceptionCheck (env)) {
    GST_ERROR ("Failed to call Java method");
    (*env)->ExceptionClear (env);
  }
  (*env)->DeleteLocalRef (env, jmessage);
}

/* Retrieve errors from the bus and show them on the UI */
static void error_cb (GstBus *bus, GstMessage *msg, CustomData *data) {
  GError *err;
  gchar *debug_info;
  gchar *message_string;

  gst_message_parse_error (msg, &err, &debug_info);
  message_string = g_strdup_printf ("Error received from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
  g_clear_error (&err);
  g_free (debug_info);
  set_ui_message (message_string, data);
  g_free (message_string);
  gst_element_set_state (data->pipeline, GST_STATE_NULL);
}

/* Notify UI about pipeline state changes */
static void state_changed_cb (GstBus *bus, GstMessage *msg, CustomData *data) {
  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
  /* Only pay attention to messages coming from the pipeline, not its children */
  if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
    gchar *message = g_strdup_printf("State changed to %s", gst_element_state_get_name(new_state));
    set_ui_message(message, data);
    g_free (message);
  }
}

/* Check if all conditions are met to report GStreamer as initialized.
 * These conditions will change depending on the application */
static void check_initialization_complete (CustomData *data) {
  JNIEnv *env = get_jni_env ();
  if (!data->initialized && data->main_loop) {
    GST_DEBUG ("Initialization complete, notifying application. main_loop:%p", data->main_loop);
    (*env)->CallVoidMethod (env, data->app, on_gstreamer_initialized_method_id);
    if ((*env)->ExceptionCheck (env)) {
      GST_ERROR ("Failed to call Java method");
      (*env)->ExceptionClear (env);
    }
    data->initialized = TRUE;
  }
}

/* Main method for the native code. This is executed on its own thread. */
static void *app_function (void *userdata) {
  JavaVMAttachArgs args;
  GstBus *bus;
  CustomData *data = (CustomData *)userdata;
  GSource *bus_source;
  GError *error = NULL;

  GST_DEBUG ("Creating pipeline in CustomData at %p", data);

  /* Create our own GLib Main Context and make it the default one */
  data->context = g_main_context_new ();
  g_main_context_push_thread_default(data->context);

  /* Build pipeline */
  data->pipeline = gst_parse_launch("audiotestsrc ! audioconvert ! audioresample ! autoaudiosink", &error);
  if (error) {
    gchar *message = g_strdup_printf("Unable to build pipeline: %s", error->message);
    g_clear_error (&error);
    set_ui_message(message, data);
    g_free (message);
    return NULL;
  }

  /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
  bus = gst_element_get_bus (data->pipeline);
  bus_source = gst_bus_create_watch (bus);
  g_source_set_callback (bus_source, (GSourceFunc) gst_bus_async_signal_func, NULL, NULL);
  g_source_attach (bus_source, data->context);
  g_source_unref (bus_source);
  g_signal_connect (G_OBJECT (bus), "message::error", (GCallback)error_cb, data);
  g_signal_connect (G_OBJECT (bus), "message::state-changed", (GCallback)state_changed_cb, data);
  gst_object_unref (bus);

  /* Create a GLib Main Loop and set it to run */
  GST_DEBUG ("Entering main loop... (CustomData:%p)", data);
  data->main_loop = g_main_loop_new (data->context, FALSE);
  check_initialization_complete (data);
  g_main_loop_run (data->main_loop);
  GST_DEBUG ("Exited main loop");
  g_main_loop_unref (data->main_loop);
  data->main_loop = NULL;

  /* Free resources */
  g_main_context_pop_thread_default(data->context);
  g_main_context_unref (data->context);
  gst_element_set_state (data->pipeline, GST_STATE_NULL);
  gst_object_unref (data->pipeline);

  return NULL;
}

/*
 * Java Bindings
 */

/* Quit the main loop, remove the native thread and free resources */
static void gst_native_finalize (JNIEnv* env, jobject thiz) {
  CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
  if (!data) return;
  GST_DEBUG ("Quitting main loop...");
  g_main_loop_quit (data->main_loop);
  GST_DEBUG ("Waiting for thread to finish...");
  pthread_join (gst_app_thread, NULL);
  GST_DEBUG ("Deleting GlobalRef for app object at %p", data->app);
  (*env)->DeleteGlobalRef (env, data->app);
  GST_DEBUG ("Freeing CustomData at %p", data);
  g_free (data);
  SET_CUSTOM_DATA (env, thiz, custom_data_field_id, NULL);
  GST_DEBUG ("Done finalizing");
}

/* Set pipeline to PLAYING state */
static void gst_native_play (JNIEnv* env, jobject thiz) {
  CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
  if (!data) return;
  GST_DEBUG ("Setting state to PLAYING");
  gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
}

/* Set pipeline to PAUSED state */
static void gst_native_pause (JNIEnv* env, jobject thiz) {
  CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
  if (!data) return;
  GST_DEBUG ("Setting state to PAUSED");
  gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
}
