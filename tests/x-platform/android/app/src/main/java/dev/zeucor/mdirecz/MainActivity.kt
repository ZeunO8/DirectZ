package dev.zeucor.mdirecz

import android.app.Activity
import android.content.res.AssetManager
import android.graphics.PixelFormat
import android.os.Bundle
import android.view.MotionEvent
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.view.ViewTreeObserver


class MainActivity : Activity()
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
        setFullscreen(window.decorView);
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
            }
        })

        surfaceView.holder.addCallback(object : SurfaceHolder.Callback
        {
            override fun surfaceCreated(holder: SurfaceHolder)
            {
                if (!surfaceInitialized)
                    init();
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

    fun setFullscreen(decorView: View) {
        val ui_Options = (View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_FULLSCREEN
                or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY)
        decorView.systemUiVisibility = ui_Options
    }
}