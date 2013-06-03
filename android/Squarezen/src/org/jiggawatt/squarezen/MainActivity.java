package org.jiggawatt.squarezen;

import java.io.File;
import java.io.FilenameFilter;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.os.Environment;
import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

public class MainActivity extends Activity {
	private static final int SAMPLE_RATE = 44100;
	
	private AudioTrack audioTrack;
	private int minBufferSize;
	private ByteBuffer[] pcmFromNative;
	private byte[] buf;
	private Runnable audioRunner = null;
	private boolean stopAudioRunner, playing;
	private int bufferToPlay;
	private String[] ymFiles;
	private Thread t = null;
	private byte[] ymState;
	DrawingPanel drawingPanel;
	Window activityWindow = null;

	private class CustomListAdapter extends ArrayAdapter {

	    private Context mContext;
	    private int id;
	    private List <String>items ;

	    public CustomListAdapter(Context context, int textViewResourceId , List<String> list ) 
	    {
	        super(context, textViewResourceId, list);           
	        mContext = context;
	        id = textViewResourceId;
	        items = list ;
	    }

	    @Override
	    public View getView(int position, View v, ViewGroup parent)
	    {
	        View mView = v ;
	        if(mView == null){
	            LayoutInflater vi = (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	            mView = vi.inflate(id, null);
	        }

	        TextView text = (TextView) mView.findViewById(R.id.textView);

	        if(items.get(position) != null )
	        {
	            text.setTextColor(Color.WHITE);
	            text.setText(items.get(position));
	            text.setBackgroundColor(Color.argb(96,0,0,0)); 
	        }

	        return mView;
	    }

	}
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        /*minBufferSize = AudioTrack.getMinBufferSize(SAMPLE_RATE,
        											AudioFormat.CHANNEL_OUT_STEREO,
        											AudioFormat.ENCODING_PCM_16BIT);
        Log.e("YmPlay", "minBufferSize = " + minBufferSize);
        minBufferSize *= 2;
        
        try {
            audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,
            		                    SAMPLE_RATE,
            		                    AudioFormat.CHANNEL_OUT_STEREO,
            		                    AudioFormat.ENCODING_PCM_16BIT,
            		                    minBufferSize,
            		                    AudioTrack.MODE_STREAM);
        } catch (Exception e) {
        	Log.e("YmPlay", "Unable to create AudioTrack: " + e.toString());
        }
        
        pcmFromNative = new ByteBuffer[2];
        pcmFromNative[0] = ByteBuffer.allocateDirect(minBufferSize);
        pcmFromNative[1] = ByteBuffer.allocateDirect(minBufferSize);
        buf = new byte[minBufferSize*2];*/

        File dir = new File(Environment.getExternalStorageDirectory().getPath()+"/YM");
        ymFiles = dir.list(
        	    new FilenameFilter()
        	    {
        	        public boolean accept(File dir, String name)
        	        {
        	        	String nameUpper = name.toUpperCase();
        	            return nameUpper.endsWith(".YM") ||
        	            	   nameUpper.endsWith(".VGM") ||
        	            	   nameUpper.endsWith(".VGZ") ||
        	            	   nameUpper.endsWith(".GBS");
        	        }
        	    });

        ArrayAdapter<String> adapter = new CustomListAdapter(this,
                R.layout.custom_list, Arrays.asList(ymFiles));
        ListView songsListView = (ListView) findViewById(R.id.listView1);
        songsListView.setAdapter(adapter);
        songsListView.setVisibility(View.VISIBLE);
        songsListView.bringToFront();

        Log.e("Squarezen", "songlist = " + ymFiles.length);
        
        songsListView.setOnItemClickListener(new ListView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> a, View v, int i, long l) {
            	stopSong();
            	playSong(ymFiles[i]);
                //Log.e("YmPlay", "Clicked item " + i + " (" + ymFiles[i] + ")");
            }
        });
         
        ymState = new byte[24];
        drawingPanel = (DrawingPanel) findViewById(R.id.surfaceView1);
        drawingPanel.setParent(this);
        
        stopAudioRunner = false;
        playing = false;
        
        final Button button = (Button) findViewById(R.id.button1);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	Log.e("Squarezen", "Button1 clicked");
            	finish();
                // Perform action on click
            }
        });        
    }

    public void stopSong() {
    	if (playing) {
	    	stopAudioRunner = true;
	    	//while (stopAudioRunner) {}
	    	//audioTrack.stop();
	    	Close();
	    	playing = false;
	    	audioRunner = null;
    	}
    }
    
    public void playSong(String songName) {  	
    	Prepare(Environment.getExternalStorageDirectory().getPath() + "/YM/" + songName);
    	playing = true;
        /*Run(minBufferSize>>2, pcmFromNative[0]);
        bufferToPlay = 0;
        
        if (audioRunner == null) {
	    	try {
	            audioTrack.play();
	        } catch (Exception e) {
	        	Log.e("YmPlay", "Unable to start playback: " + e.toString());
	        }

        	t = new Thread(audioRunner = new Runnable()
	        {
	        	@Override
	            public void run() 
	            {
	        		android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
                	pcmFromNative[0].position(0);
                	pcmFromNative[1].position(0);
                	playing = true;
	                while (!stopAudioRunner) {
	                	pcmFromNative[bufferToPlay].get(buf, bufferToPlay*minBufferSize, minBufferSize);
	                	//GetState(ymState);
	                	//DrawingPanel.setYmState(ymState);
	                	audioTrack.write(buf, bufferToPlay*minBufferSize, minBufferSize);
	                	drawingPanel.setPlayingBuffer(pcmFromNative[bufferToPlay]);
	                	bufferToPlay ^= 1;	
	                	pcmFromNative[bufferToPlay].position(0);
	                	Run(minBufferSize>>2, pcmFromNative[bufferToPlay]);
	                }
	                stopAudioRunner = false;
	            }
	        });
        } 	
        t.start();*/
        
        activityWindow = getWindow();
        activityWindow.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        		
    }

    @Override
    public void onDestroy() {
    	stopAudioRunner = true;
        activityWindow.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    	//while (stopAudioRunner) {}
    	//audioTrack.stop();
    	//audioTrack.release();
    	Close();
    	Exit();
    	super.onDestroy();
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }
   
    static {
    	System.loadLibrary("emu-players");
    }
    
    public native void Prepare(String filePath);
    public native void Close();
    public native void Exit();
    public native void Run(int numSamples, ByteBuffer byteBuffer);
    //public native void GetState(byte[] state);    
}
