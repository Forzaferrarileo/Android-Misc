package com.leonardoferraro.touchgame;

import android.app.Activity;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.widget.TextView;

public class MainActivity extends Activity {

	private TextView time_TV;
	private TextView record_TV;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
    	
    	
    	//String newRecord = Integer.toString(Integer.parseInt(Utility.getStringSaved(this, "record")) + 50);
    	//Utility.setStringToSave(this, "record", newRecord);
    	
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        time_TV = (TextView)this.findViewById(R.id.time_TV);
        record_TV = (TextView)this.findViewById(R.id.record_TV);
        
        record_TV.setText("Record : " + Utility.getStringSaved(this, "record"));

        new CountDownTimer(10000, 1000){
        	
        	public void onTick(long millisUntilFinished) {
        		time_TV.setText("Time : "+ millisUntilFinished /1000);
        	}
        	
        	public void onFinish(){
        		time_TV.setText("Fine!");
        	}
        }.start();

    }
    
    

    
}
