package de.alex2804.libtcctest;

import androidx.appcompat.app.AppCompatActivity;

import android.content.res.AssetManager;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    public boolean copyAssetFolder(String srcName, String dstName) {
        try {
            boolean result = true;
            String fileList[] = getAssets().list(srcName);
            if (fileList == null)
                return false;

            if (fileList.length == 0) {
                result = copyAssetFile(srcName, dstName);
            } else {
                File file = new File(dstName);
                result = file.mkdirs();
                for (String filename : fileList) {
                    result = copyAssetFolder(srcName + File.separator + filename, dstName + File.separator + filename);
                }
            }
            return result;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    public boolean copyAssetFile(String srcName, String dstName) {
        try {
            InputStream in = getAssets().open(srcName);
            File outFile = new File(dstName);
            OutputStream out = new FileOutputStream(outFile);
            byte[] buffer = new byte[1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            in.close();
            out.close();
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        copyAssetFolder("include", getFilesDir().getAbsolutePath() + "/include");

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
        tv.setMovementMethod(new ScrollingMovementMethod());
        AssetManager assetManager = getAssets();
        tv.setText(stringFromJNI(assetManager, getFilesDir().getAbsolutePath() + "/"));
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI(AssetManager assetManager, String filesPath);
}
