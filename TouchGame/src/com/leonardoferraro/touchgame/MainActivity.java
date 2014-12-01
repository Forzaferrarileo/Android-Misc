package com.leonardoferraro.touchgame;

import android.app.Activity;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.TextView;
import android.widget.ToggleButton;

public class MainActivity extends Activity {

	private TextView time_TV;
	private TextView record_TV;
	private Button clear_VW;
	private ToggleButton version_VW;
	
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

		version_VW = (ToggleButton)findViewById(R.id.versione_VW);
		clear_VW = (Button)findViewById(R.id.clear_VW);
		version_VW.setEnabled(true);
		version_VW.setChecked(false);
		version_VW.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener(){

			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				
				if(isChecked){
	
					MainView.versionClr = true;
				
				}else{
					
					MainView.versionClr = false;
					
				}
			}
		});
		
		clear_VW.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
			
				MainView.posizioneX.clear();
				MainView.posizioneY.clear();
				MainView.red.clear();
				MainView.green.clear();
				MainView.blue.clear();
				MainView.i = 0;		
				MainView.isClear = true;
				Log.d("db","premuto");
				
			}
		});
        
    }
    
    

    
}
