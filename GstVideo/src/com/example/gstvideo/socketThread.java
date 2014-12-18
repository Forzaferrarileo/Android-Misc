package com.example.gstvideo;

import java.io.BufferedWriter;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.InetAddress;
import java.net.Socket;

import android.app.Activity;
import android.util.Log;

public class socketThread extends Activity {
	
	private Socket socket;
	private static final int SERVERPORT = 5002;
	private static final String SERVER_IP = "192.168.1.1";
	public static boolean connected;
	
	private static String movX;
	private static String movY;
	
	public static void startThread(){
		
		new Thread(new ClientThread()).start();
		
	}

	public static class ClientThread implements Runnable {
		 
        public void run() {
			try {
                InetAddress serverAddr = InetAddress.getByName(SERVER_IP);

                Socket socket = new Socket(serverAddr, SERVERPORT);
                Log.d("db","Connected");
                connected = true;
                while (connected) {
                    try {
                        Log.d("db","Sending command");
                        PrintWriter out = new PrintWriter(new BufferedWriter(new OutputStreamWriter(socket.getOutputStream())), true);

                        if(BallView.iamDrawn){  
                        	
                        	movX = Integer.toString(BallView.posX[0]);
                        	movY = Integer.toString(BallView.posY[0]);
                        	
                        	out.println("Needs to be filled");
                            Log.d("db", "Sent.");
                            
                        }else{
                        	Log.d("db","Waiting the joytick to live");
                        }
                                                
                    } catch (Exception e) {
                        Log.e("ClientActivity", "S: Error", e);
                    }
                }
                socket.close();
                Log.d("ClientActivity", "Closed");
            } catch (Exception e) {
                Log.e("ClientActivity", "Error : ", e);
                connected = false;
            }
        }
    }
}
