package constantin.video.core;

public class TestFEC {
    static {
        System.loadLibrary("XFEC_lib");
    }

    public static native void nativeTestFec();
}
