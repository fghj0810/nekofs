package com.pixelneko;

import android.content.res.AssetManager;
import com.unity3d.player.UnityPlayer;

public class FileSystem{
    static {
        System.loadLibrary("nekofs");
    }

    public static void InitAssetManager(){
        AssetManager assetmanager = UnityPlayer.currentActivity.getResources().getAssets();
        initAssetManager(assetmanager);
    }

    private static native void initAssetManager(Object assetmanager);
}
