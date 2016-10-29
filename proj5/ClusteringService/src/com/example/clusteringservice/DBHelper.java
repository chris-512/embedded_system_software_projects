package com.example.clusteringservice;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteStatement;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.util.Log;

public class DBHelper extends SQLiteOpenHelper {

	public static final String DATABASE_NAME = "CLUSTERING.db";
	public static final String IMAGES_TABLE_NAME = "images";
	public static final String IMAGES_COLUMN_ID = "id";
	public static final String IMAGES_COLUMN_NAME = "fname";
	public static final String IMAGES_COLUMN_CLUSTERNO = "clusterno";
	public static final String IMAGES_COLUMN_DATA = "rawdata";

	public static final String IMAGES_COLUMN_RED = "red";
	public static final String IMAGES_COLUMN_GREEN = "green";
	public static final String IMAGES_COLUMN_BLUE = "blue";

	public DBHelper(Context context) {
		super(context, DATABASE_NAME, null, 1);
	}

	@Override
	public void onCreate(SQLiteDatabase db) {
		// TODO Auto-generated method stub
		db.execSQL("create table images "
				+ "(id integer primary key, fname text, clusterno integer, rawdata blob, red real, green real, blue real)");
	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		// TODO Auto-generated method stub
		db.execSQL("DROP TABLE IF EXISTS images");
		onCreate(db);
	}
	
	public void dropTable()
	{
		SQLiteDatabase db = this.getWritableDatabase();
		db.execSQL("DROP TABLE IF EXISTS images");
	}
	

	public void deleteAllRecords() {
		SQLiteDatabase db = this.getWritableDatabase();
		db.execSQL("delete from images");
	}

	public static byte[] getBitmapAsByteArray(Bitmap bitmap) {
		ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
		bitmap.compress(CompressFormat.PNG, 0, outputStream);
		return outputStream.toByteArray();
	}

	public boolean insertImage(String name, Bitmap img) {
		SQLiteDatabase db = this.getWritableDatabase();

		double r, g, b;
		double h = img.getHeight();
		double w = img.getWidth();
		double size = h * w;
		
		r = g = b = 0.0;
		
		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				int color = img.getPixel(j, i);
				r += ((color >> 16) & 0xff) / size;
				g += ((color >> 8) & 0xff) / size;
				b += (color & 0xff) / size;
			}
		}

		byte[] data = getBitmapAsByteArray(img);

		String sql = "INSERT INTO images (fname, clusterno, rawdata, red, green, blue) VALUES(?,?,?,?,?,?)";
		SQLiteStatement insertStmt = db.compileStatement(sql);
		insertStmt.clearBindings();
		insertStmt.bindString(1, name);
		insertStmt.bindLong(2, -1);
		insertStmt.bindBlob(3, data);
		insertStmt.bindDouble(4, r);
		insertStmt.bindDouble(5, g);
		insertStmt.bindDouble(6, b);
		insertStmt.executeInsert();
		return true;
	}

	public Cursor get(int id) {
		SQLiteDatabase db = this.getReadableDatabase();
		Cursor res = db.rawQuery("select * from images where id=" + id + "",
				null);
		return res;
	}

	public Bitmap getImage(int id) {
		SQLiteDatabase db = this.getReadableDatabase();
		Cursor cur = db.rawQuery("select * from images where id=" + id + "",
				null);
		if (cur.moveToFirst()) {
			byte[] imgByte = cur.getBlob(3);
			cur.close();
			return BitmapFactory.decodeByteArray(imgByte, 0, imgByte.length);
		}
		if (cur != null && !cur.isClosed()) {
			cur.close();
		}
		return null;
	}
	
	public double[] getImageRGB(int id) {
		SQLiteDatabase db = this.getReadableDatabase();
		Cursor cur = db.rawQuery("select * from images where id=" + id + "",
				null);
		double[] res = null;
		if(cur.moveToFirst()) {
			res = new double[3];
			res[0] = cur.getDouble(4);
			res[1] = cur.getDouble(5);
			res[2] = cur.getDouble(6);
		}
		return res;
	}

	public int numberOfRows() {
		SQLiteDatabase db = this.getReadableDatabase();
		int numRows = (int) DatabaseUtils
				.queryNumEntries(db, IMAGES_TABLE_NAME);
		return numRows;
	}

	public boolean updateClusterNo(Integer id, Integer clusterNo) {
		SQLiteDatabase db = this.getWritableDatabase();
		ContentValues contentValues = new ContentValues();
		contentValues.put("clusterno", clusterNo);
		db.update("images", contentValues, "id = ? ",
				new String[] { Integer.toString(id) });
		return true;
	}

	public Integer deleteImage(Integer id) {
		SQLiteDatabase db = this.getWritableDatabase();
		return db.delete("images", "id = ? ",
				new String[] { Integer.toString(id) });
	}

	public ArrayList<String> getAllImages() {
		ArrayList<String> array_list = new ArrayList<String>();

		// hp = new HashMap();
		SQLiteDatabase db = this.getReadableDatabase();
		Cursor res = db.rawQuery("select * from images", null);
		res.moveToFirst();

		while (res.isAfterLast() == false) {
			array_list
					.add(res.getString(res.getColumnIndex(IMAGES_COLUMN_NAME)));
			res.moveToNext();
		}
		return array_list;
	}
}