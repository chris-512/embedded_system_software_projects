package com.example.androidex;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

public class StartScreen extends Activity {
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.start_layout);
		
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT);
		
		Button gameButton = (Button) findViewById(R.id.gamebutton);
		gameButton.setOnClickListener(new Button.OnClickListener() {
					public void onClick(View v) {
						Intent gameIntent = new Intent(StartScreen.this, SlidePuzzle.class);
						startActivity(gameIntent);
					}
		});
		
	}
}
