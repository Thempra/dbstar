package com.dbstar.app;

import java.util.LinkedList;
import java.util.List;

import com.dbstar.R;
import com.dbstar.model.ContentData;
import com.dbstar.service.GDDataProviderService;
import com.dbstar.model.Movie;
import com.dbstar.model.GDDVBDataContract.Content;
import com.dbstar.widget.GDAdapterView;
import com.dbstar.widget.GDGridView;
import com.dbstar.widget.GDAdapterView.OnItemSelectedListener;
import com.dbstar.widget.GDScrollBar;

//import android.app.Dialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
//import android.view.Window;
//import android.view.animation.Animation;
//import android.view.animation.AnimationUtils;
import android.widget.BaseAdapter;
//import android.widget.Button;
import android.widget.ImageView;
//import android.widget.ScrollView;
import android.widget.TextView;

public class GDHDMovieActivity extends GDBaseActivity {
	private static final String TAG = "GDHDMovieActivity";

	// private static final int DIALOG_MOVIEINFO_ID = 0;

	private static final int COLUMN_ITEMS = 6;
	private static final int PAGE_ITEMS = 12;
	private static final int PageSize = PAGE_ITEMS;
	int mPageNumber = 0;
	int mPageCount = 0;
	int mTotalCount = 0;

	String mColumnId;
	List<Movie[]> mPageDatas;

	// Dialog mMovieInfoViewDlg = null;
	GDGridView mSmallThumbnailView;
	MovieAdapter mAdapter;
	int mSeletedItemIndex = 0;
	GDScrollBar mScrollBar = null;

	// View mSelectedView = null;
	boolean mReachPageEnd = false;
	TextView mPageNumberView;

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.hdmovie_view);

		Intent intent = getIntent();
		mColumnId = intent.getStringExtra(Content.COLUMN_ID);
		mMenuPath = intent.getStringExtra(INTENT_KEY_MENUPATH);
		Log.d(TAG, "column id = " + mColumnId);
		Log.d(TAG, "menu path = " + mMenuPath);
		mPageDatas = new LinkedList<Movie[]>();

		initializeView();
	}

	protected void initializeView() {
		super.initializeView();

		mPageNumberView = (TextView) findViewById(R.id.pageNumberView);

		mScrollBar = (GDScrollBar) findViewById(R.id.scrollbar);

		mSmallThumbnailView = (GDGridView) findViewById(R.id.gridview);

		mSmallThumbnailView
				.setOnItemSelectedListener(new OnItemSelectedListener() {

					@Override
					public void onItemSelected(GDAdapterView<?> parent,
							View view, int position, long id) {
						Log.d(TAG, "mSmallThumbnailView selected = " + position);

						mSeletedItemIndex = position;
					}

					@Override
					public void onNothingSelected(GDAdapterView<?> parent) {

					}

				});

		mAdapter = new MovieAdapter(this);
		mSmallThumbnailView.setAdapter(mAdapter);
		mSmallThumbnailView.setOnKeyListener(new View.OnKeyListener() {

			@Override
			public boolean onKey(View v, int keyCode, KeyEvent event) {
				Log.d(TAG, "onKey " + keyCode);
				boolean ret = false;
				int action = event.getAction();
				if (action == KeyEvent.ACTION_DOWN) {
					switch (keyCode) {

					// case KeyEvent.KEYCODE_F1: // for debug with keyboard
					// case KeyEvent.KEYCODE_F2:
					// case KeyEvent.KEYCODE_INFO:
					// case KeyEvent.KEYCODE_NOTIFICATION:
					// case KeyEvent.KEYCODE_MENU:
					// ret = true;
					// showMovieInfoView();
					// break;

					case KeyEvent.KEYCODE_DPAD_LEFT: {
						int currentItem = mSmallThumbnailView
								.getSelectedItemPosition();
						if (currentItem == PAGE_ITEMS / 2) {
							mSmallThumbnailView.setSelection(currentItem - 1);
							ret = true;
						} else if (currentItem == 0) {
							mSmallThumbnailView.setSelection(mAdapter
									.getCount() - 1);
							ret = true;
						} else {
						}
						break;
					}
					case KeyEvent.KEYCODE_DPAD_RIGHT: {

						int currentItem = mSmallThumbnailView
								.getSelectedItemPosition();

						if (currentItem == (PAGE_ITEMS / 2 - 1)) {
							if ((currentItem + 1) < (mAdapter.getCount() - 1)) {
								mSmallThumbnailView
										.setSelection(currentItem + 1);
								ret = true;
							}
						} else if (currentItem == (mAdapter.getCount() - 1)) {
							mSmallThumbnailView.setSelection(0);
							ret = true;
						} else {
						}
						break;
					}

					case KeyEvent.KEYCODE_DPAD_UP: {
						int currentItem = mSmallThumbnailView
								.getSelectedItemPosition();
						if (currentItem < COLUMN_ITEMS) {
							loadPrevPage();
							ret = true;
						}
						break;
					}

					case KeyEvent.KEYCODE_DPAD_DOWN: {
						int currentItem = mSmallThumbnailView
								.getSelectedItemPosition();
						if (currentItem >= (PAGE_ITEMS - COLUMN_ITEMS)) {
							loadNextPage();
							ret = true;
						}
						break;
					}

					case KeyEvent.KEYCODE_DPAD_CENTER:
					case KeyEvent.KEYCODE_ENTER: {
						playMovie();
						ret = true;
						break;
					}

					default:
						break;
					}

				}
				return ret;
			}
		});

		mSmallThumbnailView.requestFocus();
		mPageNumberView.setText(formPageText(mPageNumber, mPageCount));
	}

	public void onStart() {
		super.onStart();

		if (mAdapter.getCount() > 0) {
			mSmallThumbnailView.setSelection(mSeletedItemIndex);
		}

		showMenuPath(mMenuPath.split(MENU_STRING_DELIMITER));
	}

	public void onDestroy() {
		super.onDestroy();

		for (int i = 0; mPageDatas != null && i < mPageDatas.size(); i++) {
			Movie[] movies = mPageDatas.get(i);
			for (int j = 0; j < movies.length; j++) {
				if (movies[j].Thumbnail != null) {
					movies[j].Thumbnail.recycle();
				}
			}
		}
	}

	public void onServiceStart() {
		super.onServiceStart();
		mService.getAllPublications(this, mColumnId);
	}

	private void loadPrevPage() {
		if (mPageNumber > 0) {
			Log.d(TAG, "loadPrevPage");

			mPageNumber--;
			mPageNumberView.setText(formPageText(mPageNumber, mPageCount));

			Movie[] movies = mPageDatas.get(mPageNumber);
			mAdapter.setDataSet(movies);
			mSmallThumbnailView.setSelection(movies.length - 1);
			mAdapter.notifyDataSetChanged();

			mScrollBar.setPosition(mPageNumber);
		}
	}

	private void loadNextPage() {
		Log.d(TAG, "loadNextPage");

		if ((mPageNumber + 1) < mPageDatas.size()) {
			mPageNumber++;
			mPageNumberView.setText(formPageText(mPageNumber, mPageCount));

			Movie[] movies = mPageDatas.get(mPageNumber);
			mAdapter.setDataSet(movies);
			mSmallThumbnailView.setSelection(0);
			mAdapter.notifyDataSetChanged();

			mScrollBar.setPosition(mPageNumber);
		}
	}

	private Movie getSelectedMovie() {
		int currentItem = mSmallThumbnailView.getSelectedItemPosition();
		Movie[] movies = mPageDatas.get(mPageNumber);
		return movies[currentItem];
	}

	private void playMovie() {
		Log.d(TAG, "playMovie");
		Movie movie = getSelectedMovie();
		String file = mService.getMediaFile(movie.Content);
		Log.d(TAG, "file = " + file);
		if (!file.equals("")) {
			Intent intent = new Intent();
			// intent.putExtra("Uri", file);
			// intent.setClass(this, GDVideoPlayer.class);

			Uri uri = Uri.parse("file://" + file);
			Log.d(TAG, "play = " + uri.toString());

			intent.setData(uri);
			intent.setComponent(new ComponentName("com.farcore.videoplayer",
					"com.farcore.videoplayer.playermenu"));
			intent.setAction("android.intent.action.View");
			startActivity(intent);
		}
	}

	public void updateData(int type, Object key, Object data) {
		if (type == GDDataProviderService.REQUESTTYPE_GETALLPUBLICATIONS) {

			ContentData[] contents = (ContentData[]) data;
			Log.d(TAG, "update ");
			if (contents != null && contents.length > 0) {
				Log.d(TAG, "update " + contents.length);

				mTotalCount = contents.length;
				mPageCount = mTotalCount / PageSize;
				int index = 0;
				for (int i = 0; i < mPageCount; i++) {
					Movie[] movies = new Movie[PageSize];
					for (int j = 0; j < PageSize; j++, index++) {
						movies[j] = new Movie();
						movies[j].Content = contents[index];
					}
					mPageDatas.add(i, movies);
				}

				int remain = mTotalCount % PageSize;
				if (remain > 0) {
					mPageCount += 1;
					Movie[] movies = new Movie[remain];
					for (int i = 0; i < remain; i++, index++) {
						movies[i] = new Movie();
						movies[i].Content = contents[index];
					}

					mPageDatas.add(movies);
				}

				mPageNumber = 0;

				// update views
				mPageNumberView.setText(formPageText(mPageNumber, mPageCount));
				mScrollBar.setRange(mPageCount);
				mScrollBar.setPosition(mPageNumber);

				mAdapter.setDataSet(mPageDatas.get(mPageNumber));
				mSmallThumbnailView.setSelection(0);
				mAdapter.notifyDataSetChanged();

				mScrollBar.setPosition(mPageNumber);

				Log.d(TAG, "update mPageCount " + mPageCount);
				
				for (int i = 0; i < mPageCount; i++) {
					Movie[] movies = mPageDatas.get(i);
					for (int j = 0; j < movies.length; j++) {
						mService.getDetailsData(this, i, j, movies[j].Content);
					}
				}

			}
		}
	}

	public void updateData(int type, int param1, int param2, Object data) {

		if (type == GDDataProviderService.REQUESTTYPE_GETDETAILSDATA) {
			int pageNumber = param1;
			int index = param2;
			Log.d(TAG, "updateData page number = " + pageNumber + " index = "
					+ index);

			mService.getImage(this, pageNumber, index, (ContentData) data);

		} else if (type == GDDataProviderService.REQUESTTYPE_GETIMAGE) {
			int pageNumber = param1;
			int index = param2;
			Log.d(TAG, "updateData page number = " + pageNumber + " index = "
					+ index);

			Movie[] movies = mPageDatas.get(pageNumber);
			movies[index].Thumbnail = (Bitmap) data;

			if (pageNumber == mPageNumber)
				mAdapter.notifyDataSetChanged();
		}
	}

	private class MovieAdapter extends BaseAdapter {

		private Movie[] mDataSet = null;

		public class ViewHolder {
			TextView titleView;
			ImageView thumbnailView;
		}

		public MovieAdapter(Context context) {
		}

		public void setDataSet(Movie[] dataSet) {
			mDataSet = dataSet;
		}

		@Override
		public int getCount() {
			int count = 0;
			if (mDataSet != null) {
				count = mDataSet.length;
			}

			return count;
		}

		@Override
		public Object getItem(int position) {
			return null;
		}

		@Override
		public long getItemId(int position) {
			return 0;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {

			ViewHolder holder = new ViewHolder();

			// if (mSeletedItemIndex == position) {
			// if (mSelectedView == null) {
			// LayoutInflater inflater = getLayoutInflater();
			// mSelectedView =
			// inflater.inflate(R.layout.small_thumbnail_item_focused,
			// parent, false);
			// holder.titleView = (TextView) mSelectedView
			// .findViewById(R.id.item_text);
			// holder.thumbnailView = (ImageView) mSelectedView
			// .findViewById(R.id.thumbnail);
			//
			// mSelectedView.setTag(holder);
			// }
			//
			// if (convertView != mSelectedView) {
			// convertView = mSelectedView;
			// }
			// } else {
			// if (convertView == mSelectedView) {
			// convertView = null;
			// }
			// }

			if (null == convertView) {
				LayoutInflater inflater = getLayoutInflater();
				convertView = inflater.inflate(
						R.layout.small_thumbnail_item_normal, parent, false);
				holder.titleView = (TextView) convertView
						.findViewById(R.id.item_text);
				holder.thumbnailView = (ImageView) convertView
						.findViewById(R.id.thumbnail);

				convertView.setTag(holder);
			} else {
				holder = (ViewHolder) convertView.getTag();
			}

			Bitmap thumbnail = mDataSet[position].Thumbnail;
			holder.thumbnailView.setImageBitmap(thumbnail);
			holder.titleView.setText(mDataSet[position].Content.Name);

			return convertView;
		}
	}

}
