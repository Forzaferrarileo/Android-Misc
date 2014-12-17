package com.example.gstvideo;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

public class BallView extends View {

	private Paint joystickBase;
	private Paint joystickPad;
	private static int[] posX = new int[2];	
	private static int[] posY = new int[2];
	
	private static int[] baseX = new int[2];
	private static int[] baseY = new int[2];

	
	private boolean staPremendo;

	private static int rBase , rPad , r , width;
	
	public BallView(Context context, AttributeSet attrs) {
		super(context,attrs);

		joystickBase = new Paint();
		joystickBase.setAntiAlias(false);
		joystickBase.setStrokeWidth(6f);
		joystickBase.setColor(Color.RED);
		joystickBase.setStyle(Paint.Style.STROKE);

		joystickPad = new Paint();
		joystickPad.setAntiAlias(false);
		joystickPad.setStrokeWidth(5f);
		joystickPad.setColor(Color.rgb(255, 230, 0));
		joystickPad.setStyle(Paint.Style.STROKE);
	
	}

	
	@Override
	public boolean onTouchEvent(MotionEvent event) {
		
		int action = event.getAction() & MotionEvent.ACTION_MASK;
		
		if ( action == MotionEvent.ACTION_MOVE ) {
				int touchCounter = event.getPointerCount();
			
				int[] x = new int[2];	
				int[] y = new int[2];
				
				for (int t = 0; t < touchCounter; t++) {
					
					int id = event.getPointerId(t);
					
					x[id] = (int) event.getX(t);
					y[id] = (int) event.getY(t);
					
					if (isOnBase1(x[id],y[id])){
						if( r > rBase){
							posX[0] = (x[id] - baseX[0]) * rBase / r + baseX[0] ;
							posY[0] = (y[id] - baseY[0]) * rBase / r + baseY[0] ;
						}else{
							posX[0] = x[id];
							posY[0] = y[id];
						}
						staPremendo = true;
					}
					Log.d("db", "Muovo il Tocco: "+posX+" "+posY);
				}
				
		}
		if ( action == MotionEvent.ACTION_UP ) {
			staPremendo = false;
		}
		
		invalidate(); 
		return true;
		
	}	
	
	
	private int getIndex(MotionEvent event) {
		 
		  int idx = (event.getAction() & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
		  return idx;

	} 
		  
	@Override
	protected void onDraw(Canvas canvas) {

		if(!staPremendo){
			posX[0] = baseX[0];
			posY[0] = baseY[0];
		}
		
		canvas.drawCircle(baseX[0], baseY[0], rBase , joystickBase);
		canvas.drawCircle(posX[0], posY[0], rPad , joystickPad);
		super.onDraw(canvas);
	}
	
	public static void setScreenSize(int width, int height){
		
		BallView.width = width;
		baseX[0] = width / 6 ;
		baseY[0] = height - ( height / 6 );
		rBase = width / 12 ;
		rPad = rBase * 70 / 100;
		
	}

	private boolean isOnBase1(int x, int y){

		r = (int) Math.sqrt(((baseX[0]-x)*(baseX[0]-x))+((baseY[0]-y)*(baseY[0]-y)));
		if ( r <= rBase && ( x <= width/2)){
			return true;
		}
		else if( x <= width/2 ){
			return true;
		}
		else{
			return false;
		}
		
	}		
}
