package com.example.gstvideo;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.graphics.Point;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.Window;
import android.view.WindowManager;
import android.widget.CompoundButton;
import android.widget.Toast;
import android.widget.ToggleButton;

import org.freedesktop.gstreamer.GStreamer;

public class MainActivity extends Activity implements Callback {

	private native void nativeInit();
	private native void nativePlay();
	private native void nativePause();
	private native void nativeDestroy();
	private static native boolean nativeClassInit();
	private native void nativeSurfaceInit(Object surface);
	private native void nativeSurfaceDestroy();
	private long native_custom_data;
	
	private boolean is_playing_desired;
	
	private ToggleButton play_tb;
	private SurfaceView videoSurface;
	private SurfaceHolder sh;
	
    private JoystickView joystick;
	
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);        
        this.requestWindowFeature(Window.FEATURE_NO_TITLE);
        
        try {
            GStreamer.init(this);
        } catch (Exception e) {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
            finish(); 
            return;
        }
        
        setContentView(R.layout.activity_main);
        
        getWindow().setBackgroundDrawable(null);
        
        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        GStreamerSurfaceView.media_width = size.x;
        GStreamerSurfaceView.media_height = size.y;
        

        //Referencing also other views
        joystick = (JoystickView) findViewById(R.id.MainView);

        
        
        play_tb = (ToggleButton) this.findViewById(R.id.play_tb);
        play_tb.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			
			@Override
			public void onCheckedChanged(CompoundButton arg0, boolean arg1) {
				
				if (arg1){
					is_playing_desired = true;
					nativePlay();
				}
				else{
					is_playing_desired = false;
					nativePause();
				}
				
			}
		});
        
        videoSurface = (SurfaceView) this.findViewById(R.id.videoSurface);
        sh = videoSurface.getHolder();
        sh.addCallback(this);
        
        if (savedInstanceState != null) {
            is_playing_desired = savedInstanceState.getBoolean("stato");
        } else {
            is_playing_desired = false;
        }
        
        play_tb.setEnabled(false);
        nativeInit();
        
        
    }
    
    protected void onSaveInstanceState(Bundle outState) {
    	outState.putBoolean("stato", is_playing_desired);
    }
    
    protected void onDestroy() {
        nativeDestroy();
        super.onDestroy();
    }
    
    //Viene chiamata dal codice nativo una volta che gstreamer è pronto per la riproduzione
    private void onGStreamerInitialized(){
    	
    	if (is_playing_desired) {
            nativePlay();
        } else {
            nativePause();
        }
    	
    	runOnUiThread(new Runnable() {
            public void run() {
                play_tb.setEnabled(true);
            }
        });
    }
    
    static {
        System.loadLibrary("gstreamer_android");
        System.loadLibrary("gstvideo");
        nativeClassInit();
    }
    
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d("db", "Superfice modificata");
    	nativeSurfaceInit(holder.getSurface());
    }
    
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d("db", "Superfice creata " + holder.getSurface());
    }
    
    public void surfaceDestroyed(SurfaceHolder holder){
        Log.d("db","Superfice distrutta");
    	nativeSurfaceDestroy();
    }
}
