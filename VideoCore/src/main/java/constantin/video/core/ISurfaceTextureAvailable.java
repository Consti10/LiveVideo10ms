package constantin.video.core;

import android.graphics.SurfaceTexture;

/**
 * With api < 26 we cannot create a SurfaceTexture object until the OpenGL thread creates
 * Its underlying texture image
 * This callback is called by the OpenGL thread with a valid SurfaceTexture
 */
public interface ISurfaceTextureAvailable{
    /**
     *@param surfaceTexture: A valid surface texture created by the OpenGL thread
     */
    public void onSurfaceTextureAvailable(final SurfaceTexture surfaceTexture);
}