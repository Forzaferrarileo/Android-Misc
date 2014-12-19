package com.leonardoferraro.lezione07;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

public class MainView extends View implements SensorEventListener {

	private Paint paint;
	private float rX , rY , firstX, firstY;
	
	private SensorManager   mSensorManager;
	private Sensor 			mSensor;
	private Sensor			mSensorLight;
	
	private int quadDim;
	private float light = 0;
	private float lightMax = 0;
	private int c;
	
	private boolean isFirst = true;
	
	private float[] gravity , linear_acceleration;
	
	public MainView(Context context, AttributeSet attrs) {
		super(context, attrs);
		
		paint = new Paint();
		paint.setAntiAlias(false);
		paint.setStrokeWidth(6f);
		paint.setStyle(Paint.Style.FILL_AND_STROKE);
		paint.setColor(Color.BLACK);		
		
		mSensorManager = (SensorManager) this.getContext().getSystemService(Context.SENSOR_SERVICE);
		mSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		mSensorLight = mSensorManager.getDefaultSensor(Sensor.TYPE_LIGHT);
		mSensorManager.registerListener(this, mSensor, SensorManager.SENSOR_DELAY_GAME);
		mSensorManager.registerListener(this, mSensorLight, SensorManager.SENSOR_DELAY_GAME);

		
		gravity = new float[3];
		linear_acceleration = new float[3];
		
	}//MainView costruttore

	@Override
	protected void onDraw(Canvas canvas) {
		
		firstX = this.getWidth()/2;
		firstY = this.getHeight()/2;
		
		quadDim = this.getWidth()/8;
		
		c = (int) ((light * 255)/lightMax) ;
		paint.setARGB(255, c, c, c);
		
		if(isFirst){
			canvas.drawRect(firstX-quadDim, firstY-quadDim, firstX+quadDim, firstY+quadDim, paint);
			isFirst=false;
			rX = firstX;
			rY = firstY;
		}else{
			canvas.drawRect(rX-quadDim, rY-quadDim, rX+quadDim, rY+quadDim, paint);
		}
		
		super.onDraw(canvas);
		
	}//onDraw

	@Override
	public boolean onTouchEvent(MotionEvent event) {


		if(event.getAction() == MotionEvent.ACTION_DOWN){
		
			rX = event.getX();
			rY = event.getY();
			
		}
		
		if(event.getAction() == MotionEvent.ACTION_MOVE){
			
			rX = event.getX();
			rY = event.getY();
			
		}
				
		return true;
		
	}//onTouchEvent

	@Override
	public void onAccuracyChanged(Sensor arg0, int arg1) {

		
		
	}//onAccuracyChanged

	@Override
	public void onSensorChanged(SensorEvent arg0) {
		
		Sensor sensor = arg0.sensor;
		
		if(sensor.getType() == Sensor.TYPE_ACCELEROMETER){
		
			final float alpha = 0.8f;

			// Isolate the force of gravity with the low-pass filter.
			gravity[0] = alpha * gravity[0] + (1 - alpha) * arg0.values[0];
			gravity[1] = alpha * gravity[1] + (1 - alpha) * arg0.values[1];
			gravity[2] = alpha * gravity[2] + (1 - alpha) * arg0.values[2];

			// Remove the gravity contribution with the high-pass filter.
			linear_acceleration[0] = arg0.values[0] - gravity[0];
			linear_acceleration[1] = arg0.values[1] - gravity[1];
			linear_acceleration[2] = arg0.values[2] - gravity[2];
		  		  
			rX = firstX - (linear_acceleration[0]*100);
			rY = firstY + (linear_acceleration[1]*100);
		
		}
		else if(sensor.getType() == Sensor.TYPE_LIGHT){
			
			light = arg0.values[0];
			
			if(light > lightMax){
				
				lightMax = light;
				
			}
		}
		
		
	}//onSensorChanged
	

}//MainView classe