package com.example.clusteringservice;


import java.util.ArrayList;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Toast;

import com.example.logclient.R;

public class MainActivity extends Activity {
	private static final String TAG = "MainActivity";
	static int searchIdx;
	
	RemoteService mService;
	boolean mIsBound;
	Intent intent;
	ImageView imageview;
	
	public void onCreate(Bundle savedInstanceState) {
		
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.main);
		
		intent = new Intent("com.example.clusteringservice.RemoteService");
		intent.setAction(RemoteService.class.getName());
		
		((Button) findViewById(R.id.startService)).setOnClickListener(bindListener);
		((Button) findViewById(R.id.stopService)).setOnClickListener(unbindListener);
		((Button) findViewById(R.id.getImage)).setOnClickListener(actionListener);
		((Button) findViewById(R.id.deleteAllRecords)).setOnClickListener(deleteListener);
		
		imageview = (ImageView) findViewById(R.id.imageView);
	}
	
	private ServiceConnection mConnection = new ServiceConnection() {
		
		public void onServiceConnected(ComponentName name, IBinder service) {
			mService = RemoteService.Stub.asInterface(service);
			Log.i(TAG, "connected");
		}
		public void onServiceDisconnected(ComponentName name) {
			mService = null;
			Log.i(TAG, "disconnected");
		}
	};
	
	protected RemoteCallback mCallback = new RemoteCallback.Stub() {
		public void onResponse(Bitmap destBitmap) throws RemoteException {
			
			if(destBitmap != null) {
				Toast.makeText(MainActivity.this, "Callback!", Toast.LENGTH_SHORT);
				imageview.setVisibility(View.VISIBLE);
				imageview.setImageBitmap(destBitmap);
			} else {
				Toast.makeText(MainActivity.this, "NULL!", Toast.LENGTH_SHORT);
			}
		}
	};
	
	private OnClickListener bindListener = new OnClickListener() {
		public void onClick(View v) {
			// open connection + oncreate remote service
			bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
			mIsBound = true;
		}
	};
	
	private OnClickListener unbindListener = new OnClickListener() {
		public void onClick(View v) {
			if(mIsBound) {
				if(mService != null) {
					// close connection
					unbindService(mConnection);
					stopService(intent);
					mService = null;
					mIsBound = false;
				}
			}
		}
	};
	
	private OnClickListener actionListener = new OnClickListener() {
		public void onClick(View v) {
			if(mService != null) {
				try {
					int numOfRecords = mService.getSize();
					int randIdx = (int) (Math.random() * numOfRecords);
					searchIdx = randIdx;
					
					LinearLayout imglayout = (LinearLayout) findViewById(R.id.imgLayout);
					
					Bitmap bmp = mService.getImage(randIdx);
					
					imageview.setVisibility(View.VISIBLE);
					imageview.setImageBitmap(bmp);
					
					imglayout.addView(imageview);
					
					//ArrayList<Integer> bmpList = mService.getSimilarImages(randIdx);
				
					
					double[] rgb = mService.getImageRGB(randIdx);
					if(rgb != null) {
						Toast.makeText(MainActivity.this, rgb[0] + " " + rgb[1] + " " + rgb[2], Toast.LENGTH_LONG).show();
					}
				} catch(RemoteException e) {
					Log.e(TAG, "onClick failed", e);
				}
			}
		}
	};
	
	private OnClickListener deleteListener = new OnClickListener() {
		public void onClick(View v) {
			if(mService != null) {
				try {
					mService.deleteAllRecords();
				} catch (RemoteException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}
	};
	
	public void onDestroy() {
		super.onDestroy();
		
		unbindService(mConnection);
		mIsBound = false;
		mService = null;
	}
}
