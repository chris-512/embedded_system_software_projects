package com.example.clusteringservice;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.util.EntityUtils;
import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.wifi.WifiManager;
import android.os.IBinder;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.util.Log;
import android.widget.Toast;


public class ClusteringService extends Service 
{
	static final String TAG = "ClusteringService";
	static final String imgsearchURL = "http://www.google.com/search?tbm=isch&q=";
	static final String textsearchURL = "http://www.google.com/search?q=";
	static boolean deleteAllRecordFlag = false;
	static boolean isCrawling;
	static final int K = 15;
	
	static ArrayList<String> keywordQueue;

	private static final Object lock = new Object();
	private FNDThread fndThread;
	private CrawlingThread crawlThread;
	private DBEventHandler dbthread;
	private static DBHelper imgdb;
	private RemoteCallbackList<RemoteCallback> mCallbacks;
	ArrayList<ArrayList<Integer>> Clusters;
	
	public native static void writeFPGAFND(int num);
	
	public void onCreate() 
	{
		
		WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);
		if(wifi.isWifiEnabled()) 
		{
			Toast.makeText(this, "Wifi Alive", Toast.LENGTH_LONG).show();
		}
		
		// Load FPGA FND library
		System.loadLibrary("fnd");
		
		keywordQueue = new ArrayList<String>();
		imgdb 			 = new DBHelper(this);
		
		/*
		Clusters 		 = new ArrayList<ArrayList<Integer>>();
		for(int i = 0; i < K; i++) {
			Clusters.add(new ArrayList<Integer>());
		}
		
		
		int n = imgdb.numberOfRows();
		int size = n / K;
		
		for(int i = 0; i < n; i += size) {
			for(int j = i; j < i + size; j++) {
				Clusters.get(i / size).add(j);
			}
		}*/
		
		fndThread 			 = new FNDThread();
		crawlThread		   = new CrawlingThread();
		dbthread				 = new DBEventHandler();
		
		dbthread.start();
		crawlThread.start();
		fndThread.start();
		
	}
	
	public void onDestroy() {
		
		crawlThread.interrupt();
		fndThread.interrupt();
	 
		crawlThread = null;
		fndThread = null;
		
	}
	
	/*
	 * 
	 * Begin exposing a service
	 * @see android.app.Service#onBind(android.content.Intent)
	 */
	
	@Override
	public IBinder onBind(Intent intent) {
		//final String version = intent.getExtras().getString("version");
		
		if(RemoteService.class.getName().equals(intent.getAction())) {
			return mBinder;
		}
		return null;
	}
	
	/*
	 *  The ILogService is defined through IDL
	  *  
	  */
	
	private final RemoteService.Stub mBinder = new RemoteService.Stub() {
		public Bitmap getImage(int num) {
			return imgdb.getImage(num);
			/*
			Log.d(TAG, "getImage");
			if(mCallbacks != null && imgdb != null) {
				int length = mCallbacks.beginBroadcast();
				for(int i = 0; i < length; i++) {
					try {
						mCallbacks.getBroadcastItem(i).onResponse(imgdb.getImage(num));
					} catch(RemoteException e) {
						e.printStackTrace();
					}
				}
			}*/
		}
		public int getSize() {
			Log.d(TAG, "deleteAllRecords");
			return imgdb.numberOfRows();
		}
		public void deleteAllRecords()
		{
			Log.d(TAG, "deleteAllRecords");
			deleteAllRecordFlag = true;
			
		}
		public double[] getImageRGB(int id)
		{
			Log.d(TAG, "getImageRGB");
			return imgdb.getImageRGB(id);
		}
		@Override
		public void registerCallback(RemoteCallback callback)
				throws RemoteException {
			// TODO Auto-generated method stub
			if(callback != null) {
				Log.e(TAG, "registerCallback");
				mCallbacks.register(callback);
			}
		}
		@Override
		public void unRegisterCallback(RemoteCallback callback)
				throws RemoteException {
			// TODO Auto-generated method stub
			if(callback != null) {
				Log.e(TAG, "unRegisterCallback");
				mCallbacks.unregister(callback);
			}
		}
	};
	
	static ArrayList<String> sortByValue(final Map<String, Integer> map)
	{
		ArrayList<String> list = new ArrayList();
		list.addAll(map.keySet());
		
		Collections.sort(list, new Comparator() {
			public int compare(Object o1, Object o2) {
				Object v1 = map.get(o1);
				Object v2 = map.get(o2);
				
				return ((Comparable) v1).compareTo(v2);
			}
		});
		
		return list;
	}
	
	static List getChildKeywordList(String name) throws IOException
	{
		ArrayList<String> res;
		Map<String, Integer> map = new HashMap<String, Integer>();
		
		String html = getHTMLDocument(textsearchURL + name);
		Document doc = Jsoup.parse(html);
		
		String htmltext = doc.text();
		String[] words = htmltext.split(" ");
		for(int i = 0; i < words.length; i++) {
			if(map.containsKey(words[i])) {
				map.put(words[i], map.get(words[i]) + 1);
			} else {
				map.put(words[i], 0);
			}
		}
		
		res = sortByValue(map);
		
		return res.subList(0, 10); // fetch top 10 keywords 
	}
	
	static void DownloadImageToSQliteDB(String urlString) throws IOException
	{
		URL url = new URL(urlString);
		
		String filename = urlString.substring(urlString.lastIndexOf(':')+1, urlString.length());
		
		InputStream in = new BufferedInputStream(url.openStream());
		Bitmap bmp = BitmapFactory.decodeStream(in);
		
		if(imgdb != null) {
			imgdb.insertImage(filename, bmp);
		}
	}
	
	static String getHTMLDocument(String url) throws ClientProtocolException, IOException 
	{
		String html = null;
		
		HttpClient httpClient = new DefaultHttpClient();
		HttpGet httpget = new HttpGet(url);
		HttpResponse response = httpClient.execute(httpget);
		HttpEntity entity = response.getEntity();
		if(entity != null) {
			html = EntityUtils.toString(entity);
		}
		return html;
	}
	
	public static class CrawlingThread extends Thread {
		public void run()
		{
			keywordQueue.add("bag image");
		
			while(!keywordQueue.isEmpty() && !Thread.currentThread().isInterrupted())
			{	
				String keyword = keywordQueue.get(0);
				keywordQueue.remove(0);
				
				// "shopping bag" -> "shopping+bag"
				keyword = URLEncoder.encode(keyword);
				
				
				String html;
				Document doc;
				try {
					html = getHTMLDocument(imgsearchURL + keyword);
					doc = Jsoup.parse(html);
					
					Elements links = doc.getElementsByTag("img");
					
					for(Element link : links) {
						String linksrc = link.attr("src");
						Log.d(TAG, linksrc);
						if(linksrc.startsWith("http")) {
							try {
								DownloadImageToSQliteDB(linksrc);
							} catch (Exception e) {
								break;
							}
						}
					}
				} catch (Exception e) {
					break;
				}
				
				
				try {
					List keywords = getChildKeywordList(keyword);
					keywordQueue.addAll(keywords);
				} catch (Exception e) {
					break;
				}
			}
		}
	}

	public static class FNDThread extends Thread {
		public void run()
		{
			while(!Thread.currentThread().isInterrupted()) {
					try {
						synchronized(lock) {
							writeFPGAFND(imgdb.numberOfRows());
						}
						this.sleep(500);
					} catch (Exception e) {
						break;
					}
			}
		}
	}
	
	public static class DBEventHandler extends Thread {
		public void run()
		{
			while(!Thread.currentThread().isInterrupted()) 
			{
				try {
					while(deleteAllRecordFlag) {
						synchronized(lock) {
							Log.d(TAG, "!");
							imgdb.deleteAllRecords();
							deleteAllRecordFlag = false;
						}
					}
				} catch(Exception e) {
					break;
				}
			}
		}
	}

	public static class ClusteringThread extends Thread {
		public void run()
		{
			while(true) {
				//adjustClusters();
			}
		}
	}
		
	double getDistance(double[] rgb1, double[] rgb2) 
	{
		double distance = 0.0f;
		
		// calculate euclidean distance
		for(int i = 0; i < 3; i++) {
			distance += (rgb1[i]-rgb2[i]) * (rgb1[i]-rgb2[i]);
		}
		return distance;
	}

	void adjustClusters() {
		
		int n 	 = imgdb.numberOfRows();
		int size = n / K;
		
		ArrayList<double[]> prev, cur;
		
		ArrayList<double[]> centroids = new ArrayList<double[]>();

		double[] avgRGB = new double[3];
		// initial clusters
		for(int i = 0; i < K; i++) {
			avgRGB[0] = avgRGB[1] = avgRGB[2] = 0; 
			if(Clusters.get(i) != null) {
				for(int j = 0; j < Clusters.get(i).size(); j++) {
					double[] temp = imgdb.getImageRGB(Clusters.get(i).get(j)); 
					avgRGB[0] += temp[0] / size;
					avgRGB[1] += temp[1] / size;
					avgRGB[2] += temp[2] / size;
				}
			}
			// K centroids
			centroids.add(avgRGB); 
		}
		
		prev = centroids;
		
		
		do {
			
			// adjust
			Clusters 		 = new ArrayList<ArrayList<Integer>>();
			for(int i = 0; i < K; i++) {
				Clusters.add(new ArrayList<Integer>());
			}
			
			for(int i = 0; i < n; i++) {
				double mindist = Double.MAX_VALUE;
				int minidx = 0;
				// k centroids
				for(int j = 0; j < K; j++) {
					double dist = getDistance(imgdb.getImageRGB(i), centroids.get(j));
					if(mindist > dist)
					{
						mindist = dist;
						minidx  = j;
					}
				}
				
				Clusters.get(minidx).add(i);
			}
			
			centroids = new ArrayList<double[]>();

			avgRGB = new double[3];
			// initial clusters
			for(int i = 0; i < K; i++) {
				avgRGB[0] = avgRGB[1] = avgRGB[2] = 0;
				if(Clusters.get(i) != null) {
					for(int j = 0; j < Clusters.get(i).size(); j++) {
						double[] temp = imgdb.getImageRGB(Clusters.get(i).get(j)); 
						avgRGB[0] += temp[0] / size;
						avgRGB[1] += temp[1] / size;
						avgRGB[2] += temp[2] / size;
					}
				}
				// K centroids
				centroids.add(avgRGB);
			}
			
			cur = centroids;
			if(prev.equals(cur)) break;
			prev = cur;
		
		} while (true);
	}
}
