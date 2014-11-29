package com.leonardoferraro.touchgame;

import android.content.Context;
import android.content.SharedPreferences;

public class Utility {

	public static String getStringSaved(Context context, String key){
		
		SharedPreferences settings = context.getSharedPreferences("settings", Context.MODE_PRIVATE);
		
		return settings.getString(key, "0");
		
	}// getStringSaved
	
	public static void setStringToSave(Context context, String key, String value){
		
		SharedPreferences settings = context.getSharedPreferences("settings", Context.MODE_PRIVATE);
		SharedPreferences.Editor editor = settings.edit();
		editor.putString(key, value);
		editor.commit();
		
	}
	
	
}
