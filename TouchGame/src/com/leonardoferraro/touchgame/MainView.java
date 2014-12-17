package com.leonardoferraro.touchgame;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class MainView extends View {
	
	private Paint paint;

	private float[] posizioneX = new float[10];
	private float[] posizioneY = new float[10];
	private boolean staPremendo;
	private int i = 0;

	public MainView(Context context, AttributeSet attrs){
		super(context, attrs);
		
		paint = new Paint();
		paint.setAntiAlias(true);
		paint.setStrokeWidth(6f);
		paint.setColor(Color.BLACK);
		paint.setStyle(Paint.Style.FILL_AND_STROKE);
		
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		
		if(event.getAction() == MotionEvent.ACTION_DOWN){
			
			staPremendo = true;
		
		}

			
		if(staPremendo && (event.getAction() == MotionEvent.ACTION_UP )){
				
			posizioneX[i] = event.getX();
			posizioneY[i] = event.getY();
				
			if( i < posizioneX.length - 1){
					
				i++;
					
			}else{
					
				i = 0;
					
			}
		}
			
		invalidate();
		return true;
	}
	
	@Override
	protected void onDraw(Canvas canvas) {
		
		for(int a = 0; a < posizioneX.length ; a++){
			
			canvas.drawCircle(posizioneX[a], posizioneY[a], 100 , paint);
			super.onDraw(canvas);
		}
	
	}	
	
}
