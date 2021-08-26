package com.video.editor.capturedemo;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.video.editor.capturedemo.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private ActivityMainBinding binding;

    private long mRecordFd;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());
        binding.btnRecordStart.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mRecordFd = createAudioRecorder();
                String saveFilPath = MainActivity.this.getExternalCacheDir() + "/record.pcm";
                Log.i("CHH", "start record audio save to " + saveFilPath);
                startRecordAudio(mRecordFd, saveFilPath);
            }
        });
        binding.btnRecordStop.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                stopRecordAudio(mRecordFd);
            }
        });
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public native long createAudioRecorder();

    public native int startRecordAudio(long fd, String path);

    public native int stopRecordAudio(long fd);
}