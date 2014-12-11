package com.example.gstvideo;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class BallView extends View {

	private Paint paint;
	private float[] posX = new float[1];
	private float[] posY = new float[1];
	private boolean staPremendo;
	
	public BallView(Context context, AttributeSet attrs) {
		super(context,attrs);

		paint = new Paint();
		paint.setAntiAlias(false);
		paint.setStrokeWidth(6f);
		paint.setColor(Color.RED);
		paint.setStyle(Paint.Style.FILL_AND_STROKE);
		
	
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		
		if(event.getAction() == MotionEvent.ACTION_DOWN){
			
			staPremendo = true;
		
		}

		if(staPremendo && (event.getAction() == MotionEvent.ACTION_MOVE)){
		
			posX[0]= event.getX();
			posY[0]= event.getY();
			
		}
		
		else if(staPremendo && (event.getAction() == MotionEvent.ACTION_UP )){
				
			posX[0] = event.getX();
			posY[0] = event.getY();
			
			staPremendo = false;
		}
			
		invalidate();
		return true;
	}	
	
	@Override
	protected void onDraw(Canvas canvas) {
		
		canvas.drawCircle(posX[0], posY[0], 100 , paint);
		super.onDraw(canvas);
	}
	
}
