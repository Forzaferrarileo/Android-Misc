package com.leonardoferraro.touchgame;

import java.util.ArrayList;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;


public class MainView extends View {
	
	private Paint paint;
	
	public static ArrayList<Float> posizioneX = new ArrayList<Float>();
	public static ArrayList<Float> posizioneY = new ArrayList<Float>();
	public static ArrayList<Integer> red = new ArrayList<Integer>();
	public static ArrayList<Integer> green = new ArrayList<Integer>();
	public static ArrayList<Integer> blue = new ArrayList<Integer>();
	
	private boolean staPremendo;
	public static boolean versionClr = false;
	public static int i = 0;
	public static boolean isClear = false;
	
	public MainView(Context context, AttributeSet attrs){
		super(context, attrs);
		
		paint = new Paint();
		paint.setAntiAlias(true);
		paint.setStrokeWidth(6f);
		paint.setStyle(Paint.Style.FILL_AND_STROKE);
		
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		
		if(event.getAction() == MotionEvent.ACTION_DOWN){
			
			staPremendo = true;
		
		}

		if(staPremendo && (event.getAction() == MotionEvent.ACTION_MOVE)){
		
			posizioneX.add(i, event.getX());
			posizioneY.add(i, event.getY());
			
			red.add(i, randomCLR());
			green.add(i, randomCLR());
			blue.add(i, randomCLR());
		}
		
		else if(staPremendo && (event.getAction() == MotionEvent.ACTION_UP )){
				
			posizioneX.add(i, event.getX());
			posizioneY.add(i, event.getY());
			
			i++;
			staPremendo = false;
		}
			
		invalidate();
		return true;
	}
	
	@Override
	protected void onDraw(Canvas canvas) {
		
		if(isClear){
			
			canvas.drawColor(Color.TRANSPARENT);
			invalidate();
			isClear = false;
			
		}else{
		
			for(int a = 0; (i > 0)&&(a <= i) ; a++){

				if(versionClr){
				
					paint.setARGB(255, randomCLR(), randomCLR(), randomCLR());
				
				}else{
				
					paint.setARGB(255, red.get(a), green.get(a), blue.get(a));
				
				}
			
				canvas.drawCircle(posizioneX.get(a), posizioneY.get(a), 100 , paint);
				super.onDraw(canvas);
			}
		}
	
		
	}	
	
	protected int randomCLR(){

		int number = (int) (Math.random()*255) ; 
		Log.d("db", Integer.toString(number));
		return number;
	}
	
}