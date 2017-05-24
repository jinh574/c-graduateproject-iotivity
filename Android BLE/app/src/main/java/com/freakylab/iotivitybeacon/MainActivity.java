package com.freakylab.iotivitybeacon;

import android.Manifest;
import android.annotation.TargetApi;
import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.PowerManager;
import android.support.annotation.NonNull;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.AppCompatImageView;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {
    public final int PERMISSION_REQUEST_COARSE_LOCATION = 1;
    public final String TAG = ".MainAcitivity";
    /**
     * ATTENTION: This was auto-generated to implement the App Indexing API.
     * See https://g.co/AppIndexing/AndroidStudio for more information.
     */

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int curID = item.getItemId();
        switch (curID) {
            case R.id.action_develeoper:
                //Toast.makeText(this, "만든이", Toast.LENGTH_SHORT).show();
                Intent intent = new Intent(getApplicationContext(), DeveloperActivity.class);
                startActivity(intent);
                return true;
            case R.id.action_info:
                //Toast.makeText(this, "정보", Toast.LENGTH_SHORT).show();
                Intent intent2 = new Intent(getApplicationContext(), InfoActivity.class);
                startActivity(intent2);
                break;
            default:
                break;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.menu_main, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    protected void onResume() {
        refreshScreen();
        //releaseWakeLock();
        super.onResume();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final BluetoothAdapter bluetooth = BluetoothAdapter.getDefaultAdapter();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (this.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle("이 어플은 위치 정보가 필요합니다 ");
                builder.setMessage("비콘을 사용하실려면 위치정보 권한이 필요합니다.");
                builder.setPositiveButton(android.R.string.ok, null);
                builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                    @TargetApi(Build.VERSION_CODES.M)
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_REQUEST_COARSE_LOCATION);
                    }
                });
                builder.show();
            }
        }
        if (!bluetooth.isEnabled()) {
            startActivityForResult(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), 0);
        }

        SharedPreferences.OnSharedPreferenceChangeListener spChanged = new SharedPreferences.OnSharedPreferenceChangeListener() {
            @Override
            public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
                // your stuff here
                refreshScreen();
            }
        };
        //acquireWakeLock(getApplicationContext());
    }

    public void refreshScreen() {
        TextView main_text = (TextView) findViewById(R.id.main_text);
        AppCompatImageView main_image = (AppCompatImageView) findViewById(R.id.main_image);
        RelativeLayout main_background = (RelativeLayout) findViewById(R.id.main_background);

        SharedPreferences pref = getSharedPreferences("pref", MODE_PRIVATE);
        int select = pref.getInt("alarm", 0);

        switch(select) {
            case 0:
                main_text.setText("당신은 안전합니다.");
                main_image.setImageResource(R.drawable.cloudy_icon);
                main_background.setBackgroundResource(R.color.main_normal);
                break;
            case 1:
                main_text.setText("화재가 발생하였습니다.\n신속하게 밖으로 대피해주세요!");
                main_image.setImageResource(R.drawable.bell_icon);
                main_background.setBackgroundResource(R.color.main_alert);
                break;
            case 2:
                main_text.setText("건물 관리자가 대피 신호를 보냈습니다\n신속하게 밖으로 대피해주세요!");
                main_image.setImageResource(R.drawable.clipboard_plan_icon);
                main_background.setBackgroundResource(R.color.main_alert);
                break;
            default:
                main_text.setText("당신은 안전합니다.");
                main_image.setImageResource(R.drawable.cloudy_icon);
                main_background.setBackgroundResource(R.color.main_normal);
                break;
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        switch (requestCode) {
            case PERMISSION_REQUEST_COARSE_LOCATION: {
                if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.d(TAG, "위치 정보 권한을 허용했습니다.");
                } else {
                    final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    builder.setTitle("기능 제한이 있습니다");
                    builder.setMessage("위치 정보 권한을 얻지 못해 비콘 정보를 받아 올 수 없습니다.");
                    builder.setPositiveButton(android.R.string.ok, null);
                    builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                        @Override
                        public void onDismiss(DialogInterface dialogInterface) {

                        }
                    });
                    builder.show();
                }
                return;
            }
        }

        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }

}
