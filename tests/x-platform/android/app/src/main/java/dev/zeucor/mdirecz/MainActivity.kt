package dev.zeucor.mdirecz

import android.app.*
import android.content.res.AssetManager
import android.graphics.PixelFormat
import android.os.*
import android.view.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat

class MainActivity : AppCompatActivity()
{
    private lateinit var surfaceView: SurfaceView
    private var lastWidth = 0
    private var lastHeight = 0
    private var surfaceInitialized = false

    companion object
    {
        init
        {
            System.loadLibrary("glue")
        }

        @JvmStatic
        external fun nativeInit(surface: Surface, assetManager: AssetManager)

        @JvmStatic
        external fun nativeOnTouch(
            action: Int,
            pointerIndex: Int,
            pointerId: Int,
            x: Float,
            y: Float,
            pressure: Float,
            size: Float
        )

        @JvmStatic
        external fun nativeDestroy()
    }

    override fun onCreate(savedInstanceState: Bundle?)
    {
        super.onCreate(savedInstanceState)

        surfaceView = object : SurfaceView(this)
        {
            override fun onTouchEvent(event: MotionEvent): Boolean
            {
                val actionMasked = event.actionMasked
                // val pointerIndex = event.actionIndex
                val pointerCount = event.pointerCount

                for (i in 0 until pointerCount)
                {
                    val id = event.getPointerId(i)
                    val x = event.getX(i)
                    val y = event.getY(i)
                    val pressure = event.getPressure(i)
                    val size = event.getSize(i)

                    onTouch(
                        actionMasked,
                        i,
                        id,
                        x,
                        y,
                        pressure,
                        size
                    )
                }
                return true
            }
        }
        surfaceView.holder.setFormat(PixelFormat.RGBA_8888)

        setContentView(surfaceView)

        surfaceView.viewTreeObserver.addOnGlobalLayoutListener(object : ViewTreeObserver.OnGlobalLayoutListener
        {
            override fun onGlobalLayout()
            {
                surfaceView.viewTreeObserver.removeOnGlobalLayoutListener(this)
                hideSystemUI()
            }
        })

        surfaceView.holder.addCallback(object : SurfaceHolder.Callback
        {
            override fun surfaceCreated(holder: SurfaceHolder)
            {
                //init();
            }

            override fun surfaceDestroyed(holder: SurfaceHolder)
            {
                destroy()
            }

            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int)
            {
                if (width != lastWidth || height != lastHeight)
                {
                    lastWidth = width
                    lastHeight = height
                    if (surfaceInitialized)
                        destroy()
                    init()
                }
            }
        })
    }

    override fun onResume()
    {
        super.onResume()
        hideSystemUI()
    }

    private fun init()
    {
        nativeInit(surfaceView.holder.surface, assets)
        surfaceInitialized = true
    }

    private fun destroy()
    {
        nativeDestroy();
        surfaceInitialized = false
    }

    private fun onTouch(
        action: Int,
        pointerIndex: Int,
        pointerId: Int,
        x: Float,
        y: Float,
        pressure: Float,
        size: Float
    )
    {
        nativeOnTouch(action, pointerIndex, pointerId, x, y, pressure, size);
    }

    private fun hideSystemUI()
    {
        surfaceView.post {
            val controller = WindowCompat.getInsetsController(window, surfaceView)
            controller?.systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            controller?.hide(WindowInsetsCompat.Type.systemBars())
        }
    }
}