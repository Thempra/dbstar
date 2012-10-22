package com.dbstar.DbstarDVB.VideoPlayer;

import java.util.Timer;
import java.util.TimerTask;

import com.dbstar.DbstarDVB.R;

import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.TextView;

public class DbVideoInfoDlg extends Dialog {
	private static final String TAG = "DbVideoInfoDlg";

	private static final int MSG_UPDATETIMEOUT = 0;
	private static final int TIMEOUT_IN_MILLIONSECONDS = 5000;
	private static final int TIMEOUT_IN_SECONDS = 5;
	private static final int UpdatePeriodInMills = 1000;
	private static final int UpdatePeriodInSeconds = 1;

	TextView mMovieTitle;
	TextView mMovieDescription;
	TextView mMovieDirector;
	TextView mMovieActors;
	TextView mMovieType;
	TextView mMovieRegion;
	
	String mTitle;
	String mDescription;
	String mDirector;
	String mActors;
	String mType;
	String mRegion;
	
	String mPublicationId = null, mPublicationSetID = null;

	TextView mTimeoutView;
	int mTimeoutInMills = TIMEOUT_IN_MILLIONSECONDS;
	int mTimeoutInSeconds = TIMEOUT_IN_SECONDS;

	Button mCloseButton, mReplayButton, mAddFavouriteButton, mDeleteButton;

	Handler mUIHandler = new Handler() {
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case MSG_UPDATETIMEOUT: {
				if (mTimeoutInMills == 0) {
					closePopupView();
					return;
				}

				mTimeoutInMills -= UpdatePeriodInMills;
				mTimeoutInSeconds -= UpdatePeriodInSeconds;
				updateTimeoutView();
				break;
			}
			default:
				break;
			}
		}
	};

	Timer mTimer = new Timer();
	UpdateTimeoutTask mTask = null;

	class UpdateTimeoutTask extends TimerTask {
		public void run() {
			timeout();
		}
	}

	void timeout() {
		mUIHandler.sendEmptyMessage(MSG_UPDATETIMEOUT);
	}

	void resetTimer() {
		if (mTask != null)
			mTask.cancel();

		mTask = new UpdateTimeoutTask();

		mTimeoutInMills = TIMEOUT_IN_MILLIONSECONDS;
		mTimeoutInSeconds = TIMEOUT_IN_SECONDS;

		updateTimeoutView();

		mTimer.schedule(mTask, UpdatePeriodInMills, UpdatePeriodInMills);
	}

	void stopTimer() {
		mTask.cancel();
		mTimer.cancel();
	}

	void updateTimeoutView() {
		String timeout = String.valueOf(mTimeoutInSeconds);
		mTimeoutView.setText(timeout);
	}

	public DbVideoInfoDlg(Context context) {
		super(context);
	}

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		requestWindowFeature(Window.FEATURE_NO_TITLE);

		setContentView(R.layout.movie_info_view);

		initializeView();
	}

	public void retriveMediaInfo(Intent intent) {
	}
	
	public void onStart() {
		super.onStart();

		resetTimer();
	}

	public void onStop() {
		super.onStop();

		stopTimer();
	}

	public void closePopupView() {
		dismiss();
	}

	public void initializeView() {
		mTimeoutView = (TextView) findViewById(R.id.timeout_view);

		mMovieTitle = (TextView) findViewById(R.id.title_view);
		mMovieDescription = (TextView) findViewById(R.id.description_view);
		mMovieDirector = (TextView) findViewById(R.id.director_view);
		mMovieActors = (TextView) findViewById(R.id.actor_view);
		mMovieType = (TextView) findViewById(R.id.type_view);

		mCloseButton = (Button) findViewById(R.id.close_button);
		mReplayButton = (Button) findViewById(R.id.replay_button);
		mAddFavouriteButton = (Button) findViewById(R.id.add_favourite_button);
		mDeleteButton = (Button) findViewById(R.id.delete_button);

		mCloseButton.setOnClickListener(mClickListener);
		mReplayButton.setOnClickListener(mClickListener);
		mAddFavouriteButton.setOnClickListener(mClickListener);
		mDeleteButton.setOnClickListener(mClickListener);

		// updateMovieInfo(mMovie);
		Log.d(TAG, "++++++++++++++++initializeView++++++++++++++");
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		resetTimer();

		return super.onKeyDown(keyCode, event);
	}

	View.OnClickListener mClickListener = new View.OnClickListener() {

		@Override
		public void onClick(View v) {
			Log.d(TAG, "button clicked");
			buttonClicked((Button) v);
		}
	};

	private void buttonClicked(Button button) {
		Log.d(TAG, "buttonClicked clicked" + button);

		if (button == mCloseButton) {
			closedButtonClicked();
		} else if (button == mReplayButton) {
			repalyButtonClicked();
		} else if (button == mAddFavouriteButton) {
			saveButtonClicked();
		} else if (button == mDeleteButton) {
			deleteButtonClicked();
		} else {

		}
	}

	void closedButtonClicked() {
		Log.d(TAG, "closedButtonClicked");
		closePopupView();
	}

	void repalyButtonClicked() {
		Log.d(TAG, "repalyButtonClicked");
	}

	void saveButtonClicked() {
		Log.d(TAG, "saveButtonClicked");

	}

	void deleteButtonClicked() {
		Log.d(TAG, "deleteButtonClicked");

	}

	void updateView() {
//		if (movie != null) {
//			if (movie.Content.Name != null) {
//				mMovieTitle.setText(movie.Content.Name);
//			}
//
//			if (movie.Description != null) {
//				mMovieDescription.setText(movie.Description);
//			}
//
//			String director = getResources().getString(
//					R.string.property_director);
//			if (movie.Content.Director != null) {
//				director += movie.Content.Director;
//			}
//			mMovieDirector.setText(director);
//
//			String actors = getResources().getString(R.string.property_actors);
//			if (movie.Content.Actors != null) {
//				actors += movie.Content.Actors;
//			}
//			mMovieActors.setText(actors);
//		}
	}
}
