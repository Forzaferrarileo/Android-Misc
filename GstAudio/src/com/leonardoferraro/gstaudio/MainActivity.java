package com.leonardoferraro.gstaudio;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import org.freedesktop.gstreamer.GStreamer;

public class MainActivity extends Activity {

	private native void nativeInit();
	private native void nativePlay();
	private native void nativePause();
	private native void nativeDestroy();
	private static native boolean nativeClassInit(); 
	/*
	*	metodo per conservare il puntatore per le callback
	*/
	private long native_custom_data;
	private boolean is_playing_desired;
	
	
	private ToggleButton togglePL;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        try {
            GStreamer.init(this);
        } catch (Exception e) {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
            finish();
            return;
        }        
        
        setContentView(R.layout.activity_main);
        
        togglePL = (ToggleButton) this.findViewById(R.id.toggleBT);
        togglePL.setEnabled(false);
        
        
        if(savedInstanceState != null){
        	is_playing_desired = savedInstanceState.getBoolean("riproduzione");
        	Log.d("db", "Activity creata, lo stato della riproduzione è :" + is_playing_desired);
        }else{
        	is_playing_desired = false;
        	Log.d("db", "Activity creata, nessuno stato della riproduzione salvato");
        }
        
        togglePL.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {

				if(isChecked){
					is_playing_desired = true;
	                nativePlay();
				}else{
					is_playing_desired = false;
	                nativePause();
				}
				
			}
		});        
        
        nativeInit();
        
    }
    
    protected void onSaveinstanceState(Bundle outState) {
    	Log.d("db","Salvo lo stato attuale della riproduzione :" + is_playing_desired);
    	outState.putBoolean("riproduzione", is_playing_desired);
    }
    
    protected void onDestroy(){
    	nativeDestroy();
    	super.onDestroy();
    }
    
    private void setMessage(final String message){
    	final TextView nativeTV = (TextView)this.findViewById(R.id.nativeTV);
    	runOnUiThread(new Runnable(){
    		public void run() {
                nativeTV.setText(message);    		
    		}
    	});
    }
    
    private void onGStreamerInitialized(){
    	
    	Log.d("db","Gstreamer inizializzato , stato della riproduzione :" + is_playing_desired);
    	if(is_playing_desired){
    		nativePlay();
    	}else{
    		nativePause();
    	}
    	
    	final Activity activity = this ;
    	runOnUiThread(new Runnable() {
    		
    		public void run() {
    			togglePL.setEnabled(true);
    		}
    	});
    }
    
    static {
        System.loadLibrary("gstreamer_android");
        System.loadLibrary("GstAudio");
        nativeClassInit();
    }
    
}

