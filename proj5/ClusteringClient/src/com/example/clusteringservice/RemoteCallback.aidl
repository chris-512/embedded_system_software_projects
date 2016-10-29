package com.example.clusteringservice;

import com.example.clusteringservice.Message;

import android.graphics.Bitmap;

interface RemoteCallback {
		void onResponse(in Bitmap data);
}