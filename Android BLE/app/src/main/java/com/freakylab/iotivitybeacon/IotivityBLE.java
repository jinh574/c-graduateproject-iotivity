package com.freakylab.iotivitybeacon;

import android.Manifest;
import android.app.Application;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.TaskStackBuilder;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.PowerManager;
import android.os.Vibrator;
import android.support.v4.app.NotificationCompat;
import android.util.Log;

import org.altbeacon.beacon.Beacon;
import org.altbeacon.beacon.BeaconConsumer;
import org.altbeacon.beacon.BeaconManager;
import org.altbeacon.beacon.BeaconParser;
import org.altbeacon.beacon.BeaconTransmitter;
import org.altbeacon.beacon.Identifier;
import org.altbeacon.beacon.Region;
import org.altbeacon.beacon.powersave.BackgroundPowerSaver;
import org.altbeacon.beacon.startup.BootstrapNotifier;
import org.altbeacon.beacon.startup.RegionBootstrap;

import java.util.Arrays;
import java.util.HashMap;

// 비콘이 쓰이는 클래스는 BeaconConsumer 인터페이스를 구현해야한다.
public class IotivityBLE extends Application implements BootstrapNotifier, BeaconConsumer {
    @Override
    public void onBeaconServiceConnect() {

    }

    private static final String TAG = ".MainAcitivity";
    private RegionBootstrap regionBootstrap;
    private BackgroundPowerSaver backgroundPowerSaver;
    BeaconManager beaconManager;
    BeaconTransmitter beaconTransmitter;
    Beacon beacon;
    public static final String IBEACON_FORMAT = "m:2-3=0215,i:4-19,i:20-21,i:22-23,p:24-24";
    public static Vibrator vibe;
    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "App started up");
        beaconManager = BeaconManager.getInstanceForApplication(this);
        beaconManager.getBeaconParsers().add(new BeaconParser().setBeaconLayout(IBEACON_FORMAT));
        // To detect proprietary beacons, you must add a line like below corresponding to your beacon
        // type.  Do a web search for "setBeaconLayout" to get the proper expression.
        // beaconManager.getBeaconParsers().add(new BeaconParser().
        //        setBeaconLayout("m:2-3=beac,i:4-19,i:20-21,i:22-23,p:24-24,d:25-25"));

        // wake up the app when any beacon is seen (you can specify specific id filers in the parameters below)
        Region region = new Region("com.freakylab.iotivitybeacon.iotivityBLE", Identifier.parse("91558c37-f1d3-4b3a-9248-00271ddc6e42"), null, null);
        regionBootstrap = new RegionBootstrap(this, region);

        backgroundPowerSaver = new BackgroundPowerSaver(this);
        vibe = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
        beaconManager.setBackgroundScanPeriod(3000l);
        beaconManager.setForegroundScanPeriod(3000l);
        beaconManager.setBackgroundBetweenScanPeriod(5000l);
        beaconManager.setForegroundBetweenScanPeriod(5000l);
        beaconManager.bind(this);

    }

    @Override
    public void didDetermineStateForRegion(int arg0, Region arg1) {
        // Don't care
    }

    @Override
    public void didEnterRegion(Region arg) {
        Log.d(TAG, "Got a did EnterRegion call");
        // This call to disable will make it so the activity below only gets launched the first time a beacon is seen (until the next time the app is launched)
        // if you want the Activity to launch every single time beacons come into view, remove this call.
        //regionBootstrap.disable();
        Log.d("js", arg.toString()+ "");
        SharedPreferences pref =getSharedPreferences("pref", MODE_PRIVATE);
        SharedPreferences.Editor editor = pref.edit();
        editor.putInt("alarm", 1);
        editor.commit();


        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        boolean isScreenOn = pm.isScreenOn();
        Log.e("js", ""+isScreenOn);
        if(isScreenOn) {
            Intent intent = new Intent(this, MainActivity.class);
            // IMPORTANT: in the AndroidManifest.xml definition of this activity, you must set android:launchMode="singleInstance" or you will get two instances
            // created when a user launches the activity manually and it gets launched from here.
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            this.startActivity(intent);
        }
        else {
            sendNotification("경보가 발생했습니다! 안전한 곳으로 대피하세요!");
        }
        //Trasmiiter
        beacon = new Beacon.Builder()
                .setId1("c368ad15dd7249ecb5f9b163bc1fc254")
                .setId2("20")
                .setId3("30")
                .setManufacturer(0x004c)
                .setTxPower(-59)
                .build();
        BeaconParser beaconParser = new BeaconParser()
                .setBeaconLayout(IBEACON_FORMAT);
        beaconTransmitter = new BeaconTransmitter(getApplicationContext(), beaconParser);
        beaconTransmitter.startAdvertising(beacon);
    }

    @Override
    public void didExitRegion(Region arg0) {
        // Don't care
        SharedPreferences pref = getSharedPreferences("pref", MODE_PRIVATE);
        SharedPreferences.Editor editor = pref.edit();
        editor.putInt("alarm", 0);
        editor.apply();

        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        boolean isScreenOn = pm.isScreenOn();
        Log.e("js", ""+isScreenOn);
        if(isScreenOn) {
            Intent intent = new Intent(this, MainActivity.class);
            // IMPORTANT: in the AndroidManifest.xml definition of this activity, you must set android:launchMode="singleInstance" or you will get two instances
            // created when a user launches the activity manually and it gets launched from here.
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            this.startActivity(intent);
        }
        else {
            sendNotification("이제 이곳은 안전합니다.");
        }
        beaconTransmitter.stopAdvertising();
    }

    private void sendNotification(String text) {
        NotificationCompat.Builder builder =
                new NotificationCompat.Builder(this)
                        .setContentTitle("재난안전시스템")
                        .setContentText(text)
                        .setSmallIcon(R.mipmap.ic_launcher)
                        .setDefaults(Notification.DEFAULT_SOUND | Notification.DEFAULT_LIGHTS);

        TaskStackBuilder stackBuilder = TaskStackBuilder.create(this);
        stackBuilder.addNextIntent(new Intent(this, MainActivity.class));
        PendingIntent resultPendingIntent =
                stackBuilder.getPendingIntent(
                        0,
                        PendingIntent.FLAG_UPDATE_CURRENT
                );
        builder.setContentIntent(resultPendingIntent);
        NotificationManager notificationManager =
                (NotificationManager) this.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.notify(1, builder.build());
        vibe.vibrate(500);

    }
}