package constantin.test;

import android.view.Surface;

public class ColorFormatTester {
    static{
        System.loadLibrary("UVCReceiverDecoder");
    }
    public static native void nativeSetSurface(Surface surface);
    public static native void nativeTestUpdateSurface();
}
