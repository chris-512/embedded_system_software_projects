package com.example.clusteringservice;

import com.example.clusteringservice.Message;
import com.example.clusteringservice.RemoteCallback;

import android.graphics.Bitmap;

interface RemoteService {
		void registerCallback(in RemoteCallback callback);
		void unRegisterCallback(in RemoteCallback callback);
		Bitmap getImage(int num);
		int getSize();
		void deleteAllRecords();
		double[] getImageRGB(int id);
}