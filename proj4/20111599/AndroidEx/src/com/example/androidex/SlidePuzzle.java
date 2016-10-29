package com.example.androidex;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.TextView;
import android.widget.Toast;

public class SlidePuzzle extends Activity {

	TextView inputText;
	Button makeButton;
	Thread mThread;
	LinearLayout linear, buttonLayout;

	static int dx[] = { -1, 0, 1, 0 };
	static int dy[] = { 0, -1, 0, 1 };
	static MyButton[][] btn;
	static int row, col;
	static int BlankBlockNo;
	static int[] numVal;

	static boolean checkX(int x) {
		return (0 <= x && x < col);
	}

	static boolean checkY(int y) {
		return (0 <= y && y < row);
	}

	static boolean isBlank(int no) {
		return (no == BlankBlockNo);
	}

	static boolean isPuzzleSolved() {
		int manhattan_distance = 0;
		for (int i = 0; i < row; i++) {
			for (int j = 0; j < col; j++) {
				manhattan_distance += (Math.abs(i - ((btn[i][j].no - 1) / col)) + Math
						.abs(j - ((btn[i][j].no - 1) % col)));

			}
		}

		return (manhattan_distance == 0) ? true : false;
	}

	class MyButton extends Button {
		int no;
		int posY, posX;

		public MyButton(Context context, int no) {
			super(context);

			this.no = no;
		}

	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.slide_puzzle_layout);

		linear = (LinearLayout) findViewById(R.id.container);
		inputText = (EditText) findViewById(R.id.inputtext);
		makeButton = (Button) findViewById(R.id.makebutton);
		buttonLayout = (LinearLayout) findViewById(R.id.buttonLayout);

		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT);

		makeButton.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {

				if (buttonLayout.getChildCount() > 0) {
					buttonLayout.removeAllViews();
				}

				String input = inputText.getText().toString();
				String[] s = input.trim().split(" ");
				
				try {
					if (s[0].length() == 0 || s[1].length() == 0) {
						Toast.makeText(SlidePuzzle.this, "Wrong input!", Toast.LENGTH_LONG).show();
						return;
					}
				} 
				catch(java.lang.ArrayIndexOutOfBoundsException e) {
					Toast.makeText(SlidePuzzle.this, "Wrong input!",
							Toast.LENGTH_LONG).show();
					return;
				}

				
				row = Integer.parseInt(s[0]);
				col = Integer.parseInt(s[1]);
				
				if(row > 5 || col > 5) {
					Toast.makeText(SlidePuzzle.this, "Max board size should be 5x5",
							Toast.LENGTH_LONG).show();
					return;
				}

				btn = new MyButton[row][col];
				numVal = new int[row * col];
				for (int i = 0; i < numVal.length; i++)
					numVal[i] = i + 1;

				int curY, curX;
				curY = row - 1; curX = col - 1;
				int randVal = (int) (Math.random() * 500) + 500;
				for (int i = 0; i < randVal; i++) {
					int randDir = (int) (Math.random() * 4);
					int nextY = curY + dy[randDir];
					int nextX = curX + dx[randDir];

					if (checkY(nextY) && checkX(nextX)) {
						int tmp = numVal[nextY * col + nextX];
						numVal[nextY * col + nextX] = numVal[curY * col + curX];
						numVal[curY * col + curX] = tmp;

						curY = nextY;
						curX = nextX;
					}
				}
				
				// Last block has to be the blank one.
				BlankBlockNo = row * col;

				for (int i = 0; i < row; i++) {
					for (int j = 0; j < col; j++) {
						btn[i][j] = new MyButton(SlidePuzzle.this, numVal[i
								* col + j]);
						btn[i][j].setText("" + numVal[i * col + j]);
						final int locY = i;
						final int locX = j;
						btn[i][j]
								.setOnClickListener(new Button.OnClickListener() {
									public void onClick(View v) {
										int dirX, dirY;
										boolean BlankBlockFound = false;

										dirY = dirX = 0;

										for (int i = 0; i < 4; i++) {
											dirY = locY + dy[i];
											dirX = locX + dx[i];
											if (checkY(dirY) && checkX(dirX) && isBlank(btn[dirY][dirX].no)) {
													BlankBlockFound = true;
													break;
											}
										}

										// swap
										if (BlankBlockFound) {
											
											Drawable d = btn[locY][locX].getBackground();
											btn[dirY][dirX].setBackgroundDrawable(d);
											btn[locY][locX].setBackgroundColor(Color.BLACK);
											
											CharSequence text1 = btn[locY][locX].getText();
											CharSequence text2 = btn[dirY][dirX].getText();
											btn[dirY][dirX].setText(text1);
											btn[locY][locX].setText(text2);
											
											int no = btn[locY][locX].no;
											btn[locY][locX].no = btn[dirY][dirX].no;
											btn[dirY][dirX].no = no;

											if (isPuzzleSolved()) {
												// clear the layout
												if (buttonLayout
														.getChildCount() > 0) {
													buttonLayout
															.removeAllViews();
												}
												// print out the toast
												Toast.makeText(
														SlidePuzzle.this,
														"Congratuations!",
														Toast.LENGTH_LONG)
														.show();
											}
										}
									}
								});

						if (numVal[i * col + j] == row * col) {
							btn[i][j].setBackgroundColor(Color.BLACK);
						}

						LinearLayout.LayoutParams bparam = new LinearLayout.LayoutParams(
								LayoutParams.MATCH_PARENT,
								LayoutParams.MATCH_PARENT, 1);
						bparam.setMargins(-4, -5, -4, -6);
						btn[i][j].setLayoutParams(bparam);
					}
				}

				LinearLayout[] rowLayouts = new LinearLayout[row];
				LinearLayout.LayoutParams lparam = new LinearLayout.LayoutParams(
						LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT, 1);

				for (int i = 0; i < row; i++) {
					rowLayouts[i] = new LinearLayout(SlidePuzzle.this);
					rowLayouts[i].setLayoutParams(lparam);

					for (int j = 0; j < col; j++) {

						rowLayouts[i].addView(btn[i][j]);
					}
					buttonLayout.addView(rowLayouts[i]);
				}

			}

		});
	}

}