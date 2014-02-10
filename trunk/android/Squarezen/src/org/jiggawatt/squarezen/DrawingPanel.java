package org.jiggawatt.squarezen;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Shader;
import android.media.audiofx.Visualizer;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;


class DrawingPanel extends SurfaceView implements SurfaceHolder.Callback {
	private PanelThread _thread;
	private Paint paint;
	public ByteBuffer[] playingBuffer = null;
	public
	float lastY = 0;
	static byte[] ymState = null;
	byte[] prevLevel = {0,0,0};
	LinearGradient grad;
	MainActivity mainActivity = null;
	int bufferPos = 2048;
	int bufferToReadFrom = 0;
	Float lastBufferX = 2048.0f, bufferX = 2048.0f;
	long prevDrawTime;
	private Visualizer visualizer;
	private boolean showWaveform = true;
	private int[] dbvalues;
	private static final int[] FFT_INDICES = {1,2,3,5,7,11,15,26};
	byte[] fft;
	
	class PanelThread extends Thread {
        private SurfaceHolder _surfaceHolder;
        private DrawingPanel _panel;
        private boolean _run = false;


        public PanelThread(SurfaceHolder surfaceHolder, DrawingPanel panel) {
            _surfaceHolder = surfaceHolder;
            _panel = panel;
        }


        public void setRunning(boolean run) { //Allow us to stop the thread
            _run = run;
        }


        @Override
        public void run() {
            Canvas c;
            //TextView text = (TextView) mView.findViewById(R.id.textView);
            while (_run) {     //When setRunning(false) occurs, _run is 
                c = null;      //set to false and loop ends, stopping thread

                try {
                    c = _surfaceHolder.lockCanvas(null);
                    synchronized (_surfaceHolder) {

                     //Insert methods to modify positions of items in onDraw()
                     _panel.postInvalidate();
                    }
                } finally {
                    if (c != null) {
                        _surfaceHolder.unlockCanvasAndPost(c);
                    }
                }
            }
        }
    }
	
    public DrawingPanel(Context context) { 
        super(context);
        setZOrderOnTop(true);
        getHolder().addCallback(this);
        getHolder().setFormat(PixelFormat.TRANSLUCENT);
        paint = new Paint();
        paint.setColor(android.graphics.Color.GRAY);
    }

    public DrawingPanel(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        // TODO Auto-generated constructor stub
        setZOrderOnTop(true);
        getHolder().addCallback(this);
        getHolder().setFormat(PixelFormat.TRANSLUCENT);
        paint = new Paint();
        paint.setColor(android.graphics.Color.GRAY);
    }

    public DrawingPanel(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
        setZOrderOnTop(true);
        getHolder().addCallback(this);
        getHolder().setFormat(PixelFormat.TRANSLUCENT);
        paint = new Paint();
        paint.setColor(android.graphics.Color.GRAY);
    }
    
    public void setPlayingBuffer(ByteBuffer buffer) {
    	synchronized (bufferX) {
	        //playingBuffer = buffer;
	        bufferX = 0.0f;
	 	}
    }
    
    static public void setYmState(byte[] state) {
    	//ymState = state;
    }
    
    public void setParent(MainActivity mainAct) {
    	mainActivity = mainAct;
    }
    
    @Override
    public boolean onTouchEvent(MotionEvent me) {
    	Log.e("DrawPanel", me.toString());
    	if (me.getAction() == MotionEvent.ACTION_DOWN) {
    		mainActivity.NextSubSong();
    	}
    	return false;
    }
    
    public void updateVisualizerWithFft(byte[] bytes) {
    	Log.e("DrawingPanel", "dbValue capture size: " + bytes.length);
    	
		//int fftSample = 8;
		for (int i = 0; i < 8; i++) {
			byte realPart = bytes[FFT_INDICES[i]*2];
		    byte imagPart = bytes[FFT_INDICES[i]*2 + 1];
		    float magnitude = (realPart*realPart + imagPart*imagPart);
		    int dbValue = (int) (10 * Math.log10(magnitude));
		    if (dbValue >= 0) dbvalues[i] = dbValue;
		    //fftSample += 16;
		    Log.e("DrawPanel", "dbValue["+i+"] = " + dbValue);
		}    	
    }
    
    
    @Override 
    public void onDraw(Canvas canvas) { 
    	super.onDraw(canvas);
    	final int CHN_GRID_X = 160;
    	final int VOL_BAR_X = 20;
    	
		int width = canvas.getWidth();

		if (playingBuffer == null) {
			playingBuffer = new ByteBuffer[2];
			playingBuffer[0] = ByteBuffer.allocateDirect(2048*4);	// Must match the size used in emu-players.cpp
			playingBuffer[1] = ByteBuffer.allocateDirect(2048*4);
			prevDrawTime = System.nanoTime();
			if (!showWaveform) {
				visualizer = new Visualizer(0);
				dbvalues = new int[8];
				for (int i = 0; i < 8; i++) dbvalues[i] = 0;
				int[] capSizes = Visualizer.getCaptureSizeRange();
				Log.e("DrawingPanel", "Valid capture size range: " + capSizes[0] +"-" + capSizes[1]);
				fft = new byte[128];
	
				visualizer.setCaptureSize(Visualizer
	                    .getCaptureSizeRange()[0]);
	            visualizer.setDataCaptureListener(
	                    new Visualizer.OnDataCaptureListener() {
	                        public void onWaveFormDataCapture(
	                                Visualizer vis, byte[] bytes,
	                                int samplingRate) {
	                            // mVisualizerView.updateVisualizer(bytes);
	                        }
	
	                        public void onFftDataCapture(Visualizer visualizer,
	                                byte[] bytes, int samplingRate) {
	
	                            DrawingPanel.this.updateVisualizerWithFft(bytes);
	                        }
	                    }, Visualizer.getMaxCaptureRate() / 2, false, true);
	            
				visualizer.setEnabled(true);
				Log.e("DrawingPanel", "samplign freq = " + visualizer.getSamplingRate());
			}
		}
		
		if (true) { //ymState != null) {
			//mainActivity.GetState(ymState);
	
			long currDrawTime = System.nanoTime();
			long timeDiffMillis = (currDrawTime - prevDrawTime)/1000000;		
			float dx = 735/(width/4); //(44100*timeDiffMillis/1000) / (width/2);   //((7526*2)/(width*15));
			bufferX = lastBufferX + 735.0f*(float)(44100*timeDiffMillis/1000);
			float y,lastX = (width/2)+1;
			
			int barWidth = width/16;
			
			int alpha = 0x80;
			int alphaCount = 0;
			paint.setColor(android.graphics.Color.argb(alpha, 0x70, 0xD4, 0xE0));
			paint.setAntiAlias(true);
	
			if (playingBuffer != null) {
				if (showWaveform) {
					synchronized (bufferX) {
						playingBuffer[bufferToReadFrom].order(ByteOrder.LITTLE_ENDIAN);
						for (int i = (width/2)+1; i < width; i += 2, alphaCount += 1) {
							if (bufferX.intValue() >= 2048) {
								mainActivity.GetBuffer(playingBuffer[1-bufferToReadFrom]);
								bufferToReadFrom = 1-bufferToReadFrom;
								playingBuffer[bufferToReadFrom].order(ByteOrder.LITTLE_ENDIAN);
								bufferX = 0.0f;
							}
							short smp = playingBuffer[bufferToReadFrom].getShort(bufferX.intValue()*4);
							y = (smp+32768)*0.0012f;
							canvas.drawLine(lastX, lastY+28, (float)i, y+28, paint);
							lastX = (float)i;
							lastY = y;
							bufferX += dx;
							if (alphaCount < 30) {
								alpha += 4;
								paint.setColor(android.graphics.Color.argb(alpha, 0x70, 0xD4, 0xE0));
							} else if (i >= width-60) {
								alpha -= 4;
								paint.setColor(android.graphics.Color.argb(alpha, 0x70, 0xD4, 0xE0));						
							}
						}
					}
				} // end if (showWaveform)
				else {
					for (int i = 0; i < 8; i++) {
						canvas.drawLine((width/2)+barWidth*i+1, 60-dbvalues[i], (width/2)+barWidth*(i+1), 60-dbvalues[i], paint);
					}
					
					//Log.e("DrawPanel", "fft[80] = " + (fft[20]&0xFF)+","+(fft[21]&0xFF));
				}
				prevDrawTime = currDrawTime;
			} else {
				for (int i = 0; i < 3; i++) {
					ymState[i] = 1;
					ymState[i+3] = 1;
					ymState[i+6] = 0;
					ymState[i+9] = 0;			
				}
			}
			
			/*paint.setColor(android.graphics.Color.WHITE);	
			canvas.drawText("A", CHN_GRID_X-16, 56+12, paint);
			canvas.drawText("B", CHN_GRID_X-16, 56+15+12, paint);
			canvas.drawText("C", CHN_GRID_X-16, 56+30+12, paint);
	
			canvas.drawLine(CHN_GRID_X, 46, CHN_GRID_X+4, 46, paint);
			canvas.drawLine(CHN_GRID_X+4, 46, CHN_GRID_X+4, 50, paint);
			canvas.drawLine(CHN_GRID_X+4, 50, CHN_GRID_X+8, 50, paint);
	
			canvas.drawLine(CHN_GRID_X+30, 50, CHN_GRID_X+34, 46, paint);
			canvas.drawLine(CHN_GRID_X+34, 46, CHN_GRID_X+34, 50, paint);
			canvas.drawLine(CHN_GRID_X+34, 50, CHN_GRID_X+38, 50, paint);
			
			canvas.drawPoint(CHN_GRID_X+15, 46, paint);
			canvas.drawPoint(CHN_GRID_X+17, 46, paint);
			canvas.drawPoint(CHN_GRID_X+19, 46, paint);
			canvas.drawPoint(CHN_GRID_X+21, 46, paint);
			//
			canvas.drawPoint(CHN_GRID_X+16, 47, paint);
			canvas.drawPoint(CHN_GRID_X+18, 47, paint);
			canvas.drawPoint(CHN_GRID_X+20, 47, paint);
			//
			canvas.drawPoint(CHN_GRID_X+15, 48, paint);
			canvas.drawPoint(CHN_GRID_X+17, 48, paint);
			canvas.drawPoint(CHN_GRID_X+19, 48, paint);
			canvas.drawPoint(CHN_GRID_X+21, 48, paint);
			//
			canvas.drawPoint(CHN_GRID_X+16, 49, paint);
			canvas.drawPoint(CHN_GRID_X+18, 49, paint);
			canvas.drawPoint(CHN_GRID_X+20, 49, paint);*/
			
			paint.setAntiAlias(false);
		
			/*for (int i = 0; i < 3; i++) {
	    		paint.setColor(android.graphics.Color.argb(0xff, 0, 80, 0));
	    		if (ymState[i] == 0)
	        		paint.setColor(android.graphics.Color.GREEN);	
	  			canvas.drawRect(CHN_GRID_X, i*15+56, CHN_GRID_X+14, i*15+14+56, paint);
	  			
	    		paint.setColor(android.graphics.Color.argb(0xff, 80, 80, 0));
	    		if (ymState[i+3] == 0)
	        		paint.setColor(android.graphics.Color.YELLOW);	
	  			canvas.drawRect(CHN_GRID_X+15, i*15+56, CHN_GRID_X+29, i*15+14+56, paint);
	    		
	    		paint.setColor(android.graphics.Color.argb(0xff, 80, 0, 0));
	    		if (ymState[i+6] != 0)
	        		paint.setColor(android.graphics.Color.RED);	
	  			canvas.drawRect(CHN_GRID_X+30, i*15+56, CHN_GRID_X+44, i*15+14+56, paint);
	  			
	    		paint.setColor(android.graphics.Color.WHITE);
	    		paint.setShader(grad);
	  			canvas.drawRect(i*15+VOL_BAR_X, 100-(ymState[i+9]), i*15+VOL_BAR_X+14, 100, paint);
	  			paint.setShader(null);
			}*/
		}
		
		paint.setColor(android.graphics.Color.GRAY);		
		canvas.drawLine((width/2)-1, 0, (width/2)-1, canvas.getHeight(), paint);
		paint.setColor(android.graphics.Color.WHITE);	
		canvas.drawLine((width/2), 0, (width/2), canvas.getHeight(), paint);

		paint.setColor(android.graphics.Color.GRAY);	
		canvas.drawLine(0, 0, width, 0, paint);
		canvas.drawLine(0, canvas.getHeight()-1, width, canvas.getHeight()-1, paint);
		
		paint.setColor(android.graphics.Color.argb(0xFF, 0x70, 0xD4, 0xE0));
	}


    @Override 
    public void surfaceCreated(SurfaceHolder holder) { 
    	setWillNotDraw(false); //Allows us to use invalidate() to call onDraw()

		grad = new LinearGradient(0,0,0,100, 0xFFFFFF00, 0xFFDF0000, Shader.TileMode.CLAMP);
		//ymState = new byte[20];
		
        _thread = new PanelThread(getHolder(), this); //Start the thread that
        _thread.setRunning(true);                     //will make calls to 
        _thread.start();                              //onDraw()
		//holder.setFormat(PixelFormat.TRANSLUCENT);
    }

    @Override 
    public void surfaceDestroyed(SurfaceHolder holder) { 
        try {
            _thread.setRunning(false);                //Tells thread to stop
            _thread.join();                           //Removes thread from mem.
            if (visualizer != null) {
            	visualizer.setEnabled(false);
            }
        } catch (InterruptedException e) {}	    	
    }

    @Override 
    public void surfaceChanged(SurfaceHolder holder, int format, int width,
        int height) { 

    }
}

