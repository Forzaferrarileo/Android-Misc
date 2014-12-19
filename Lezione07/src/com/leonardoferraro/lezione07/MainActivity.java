package com.leonardoferraro.lezione07;

import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;

public class MainActivity extends Activity {

	private MainView myView;
	private final int FPS = 60;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
  
    	myView = new MainView(this, null);
    	
    	super.onCreate(savedInstanceState);
        setContentView(myView);
    	
        Timer timer = new Timer();
        
        TimerTask viewRefresh = new refreshUI();
        timer.scheduleAtFixedRate(viewRefresh, 0, 1000/FPS);

    }//onCreate
    
    class refreshUI extends TimerTask {
    	   
    	   public void run() {

    		   myView.postInvalidate();
    		   
    	   }
    	}
    

}//MainActivity
