package constantin.video.core.gl;

import android.graphics.SurfaceTexture;
import android.view.Surface;

public interface ISurfaceAvailable {
    // Always called on the UI thread and while activity is in state == resumed
    void XSurfaceCreated(final SurfaceTexture surfaceTexture, final Surface surface);
    void XSurfaceDestroyed();
}
