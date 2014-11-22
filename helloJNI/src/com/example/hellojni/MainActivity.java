package com.example.hellojni;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {

    private TextView nativeTV;
	
    public native String getMessage();
	
	static {
		System.loadLibrary("hellojni");
	}
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        nativeTV = (TextView)this.findViewById(R.id.native_TV);
        nativeTV.setText(getMessage());
                
    }
	    
}
