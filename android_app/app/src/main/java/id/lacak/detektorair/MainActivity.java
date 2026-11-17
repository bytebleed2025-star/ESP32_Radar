package id.lacak.detektorair;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.view.View;
import android.view.WindowManager;
import android.webkit.JavascriptInterface;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends Activity {

    private static final int REQ_BT_CONNECT = 2001;
    private static final UUID SPP_UUID =
            UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    private WebView webView;
    private ScrollView settingsPanel;
    private ProgressBar progress;
    private Spinner deviceList;
    private TextView status;

    private BluetoothAdapter adapter;
    private final List<BluetoothDevice> devices = new ArrayList<>();
    private final List<String> deviceNames = new ArrayList<>();
    private ArrayAdapter<String> listAdapter;

    private BluetoothSocket socket;
    private InputStream input;
    private OutputStream output;
    private Thread readerThread;
    private final Object socketLock = new Object();
    private boolean connecting = false;

    @SuppressLint("SetJavaScriptEnabled")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        webView = findViewById(R.id.webview);
        settingsPanel = findViewById(R.id.settings);
        progress = findViewById(R.id.progress);
        deviceList = findViewById(R.id.device_list);
        status = findViewById(R.id.status);

        TextView title = findViewById(R.id.title);
        String titleText = getString(R.string.app_name).toUpperCase();
        SpannableString span = new SpannableString(titleText);
        int air = titleText.lastIndexOf("AIR");
        if (air >= 0) {
            span.setSpan(new ForegroundColorSpan(getColor(R.color.green)),
                    air, titleText.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        title.setText(span);

        setupWebView();

        listAdapter = new ArrayAdapter<>(this, R.layout.item_device, deviceNames);
        listAdapter.setDropDownViewResource(R.layout.item_device);
        deviceList.setAdapter(listAdapter);
        deviceList.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view,
                                       int position, long id) {
                if (position >= 0 && position < deviceNames.size()) {
                    setStatus(deviceNames.get(position), R.color.dim);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        BluetoothManager manager = (BluetoothManager) getSystemService(BLUETOOTH_SERVICE);
        adapter = manager != null ? manager.getAdapter() : null;

        findViewById(R.id.open).setOnClickListener(v -> connectSelected());
        findViewById(R.id.wifi).setOnClickListener(v -> openBluetoothSettings());

        requestBtPermission();
        hideSystemBars();
    }

    /* ------------------------------------------------------------------ */
    /*  Bluetooth permissions / device list                               */
    /* ------------------------------------------------------------------ */

    private boolean hasBtPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            return checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)
                    == PackageManager.PERMISSION_GRANTED;
        }
        return true;
    }

    private void requestBtPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && !hasBtPermission()) {
            requestPermissions(new String[]{Manifest.permission.BLUETOOTH_CONNECT},
                    REQ_BT_CONNECT);
        } else {
            loadDevices();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
                                           int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQ_BT_CONNECT) {
            if (grantResults.length > 0
                    && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                loadDevices();
            } else {
                setStatus(getString(R.string.bt_permission), R.color.red);
            }
        }
    }

    @SuppressLint("MissingPermission")
    private void loadDevices() {
        if (adapter == null || !adapter.isEnabled()) {
            setStatus(getString(R.string.bt_off), R.color.red);
            return;
        }
        if (!hasBtPermission()) {
            setStatus(getString(R.string.bt_permission), R.color.red);
            return;
        }

        devices.clear();
        deviceNames.clear();

        try {
            Set<BluetoothDevice> bonded = adapter.getBondedDevices();
            for (BluetoothDevice device : bonded) {
                devices.add(device);
                deviceNames.add(safeName(device) + "\n" + device.getAddress());
            }
        } catch (SecurityException e) {
            setStatus(getString(R.string.bt_permission), R.color.red);
            return;
        }

        listAdapter.notifyDataSetChanged();

        int select = -1;
        for (int i = 0; i < devices.size(); i++) {
            if (safeName(devices.get(i)).toLowerCase().contains("detektorair")) {
                select = i;
                break;
            }
        }
        if (select >= 0) {
            deviceList.setSelection(select);
        }

        if (devices.isEmpty()) {
            setStatus(getString(R.string.bt_no_devices), R.color.dim);
        } else {
            setStatus(getString(R.string.status_idle), R.color.dim);
        }
    }

    @SuppressLint("MissingPermission")
    private String safeName(BluetoothDevice device) {
        try {
            String name = device.getName();
            return (name == null || name.trim().isEmpty()) ? "Bluetooth device" : name;
        } catch (SecurityException e) {
            return "Bluetooth device";
        }
    }

    private void connectSelected() {
        int position = deviceList.getSelectedItemPosition();
        if (position < 0 || position >= devices.size()) {
            setStatus(getString(R.string.invalid_address), R.color.red);
            return;
        }
        connectTo(devices.get(position));
    }

    @SuppressLint("MissingPermission")
    private void connectTo(BluetoothDevice device) {
        if (connecting) return;
        connecting = true;

        String name = safeName(device);
        setStatus(getString(R.string.bt_connecting, name), R.color.dim);
        progress.setVisibility(View.VISIBLE);
        findViewById(R.id.open).setEnabled(false);

        new Thread(() -> {
            try {
                adapter.cancelDiscovery();
                BluetoothSocket s = openSocket(device);

                synchronized (socketLock) {
                    socket = s;
                    input = s.getInputStream();
                    output = s.getOutputStream();
                }

                runOnUiThread(() -> {
                    connecting = false;
                    progress.setVisibility(View.GONE);
                    findViewById(R.id.open).setEnabled(true);
                    setStatus(getString(R.string.bt_connected, name), R.color.green);
                    settingsPanel.setVisibility(View.GONE);
                    webView.setVisibility(View.VISIBLE);
                    webView.loadUrl("file:///android_asset/index.html");
                    hideSystemBars();
                });

                startReader();
            } catch (Exception e) {
                closeSocket();
                runOnUiThread(() -> {
                    connecting = false;
                    progress.setVisibility(View.GONE);
                    findViewById(R.id.open).setEnabled(true);
                    webView.setVisibility(View.GONE);
                    settingsPanel.setVisibility(View.VISIBLE);
                    setStatus(getString(R.string.bt_failed, e.getMessage()), R.color.red);
                });
            }
        }).start();
    }

    @SuppressLint("MissingPermission")
    private BluetoothSocket openSocket(BluetoothDevice device) throws Exception {
        try {
            BluetoothSocket s = device.createRfcommSocketToServiceRecord(SPP_UUID);
            s.connect();
            return s;
        } catch (Exception secure) {
            try {
                BluetoothSocket s = device.createInsecureRfcommSocketToServiceRecord(SPP_UUID);
                s.connect();
                return s;
            } catch (Exception insecure) {
                Method m = device.getClass().getMethod("createRfcommSocket", int.class);
                BluetoothSocket s = (BluetoothSocket) m.invoke(device, 1);
                s.connect();
                return s;
            }
        }
    }

    private void startReader() {
        readerThread = new Thread(() -> {
            try {
                InputStream in;
                synchronized (socketLock) {
                    in = input;
                }
                BufferedReader reader = new BufferedReader(
                        new InputStreamReader(in, StandardCharsets.UTF_8));
                String line;
                while ((line = reader.readLine()) != null) {
                    final String data = line;
                    runOnUiThread(() -> {
                        if (webView != null) {
                            webView.evaluateJavascript(
                                    "window.__btDeliver(" + JSONObject.quote(data) + ")",
                                    null);
                        }
                    });
                }
            } catch (Exception ignored) {
            } finally {
                runOnUiThread(this::onBluetoothDisconnected);
            }
        });
        readerThread.start();
    }

    private void disconnect() {
        closeSocket();
        if (webView != null) {
            webView.loadUrl("about:blank");
            webView.setVisibility(View.GONE);
        }
        settingsPanel.setVisibility(View.VISIBLE);
        setStatus(getString(R.string.bt_disconnected), R.color.dim);
        loadDevices();
        hideSystemBars();
    }

    private void onBluetoothDisconnected() {
        if (settingsPanel.getVisibility() == View.GONE) {
            disconnect();
        }
    }

    private void closeSocket() {
        synchronized (socketLock) {
            try { if (input != null) input.close(); } catch (Exception ignored) {}
            try { if (output != null) output.close(); } catch (Exception ignored) {}
            try { if (socket != null) socket.close(); } catch (Exception ignored) {}
            input = null;
            output = null;
            socket = null;
        }
    }

    private void openBluetoothSettings() {
        try {
            startActivity(new Intent(Settings.ACTION_BLUETOOTH_SETTINGS));
        } catch (Exception e) {
            Toast.makeText(this, getString(R.string.wifi_settings), Toast.LENGTH_SHORT).show();
        }
    }

    /* ------------------------------------------------------------------ */
    /*  WebView + Bluetooth bridge                                        */
    /* ------------------------------------------------------------------ */

    @SuppressLint("SetJavaScriptEnabled")
    private void setupWebView() {
        WebSettings s = webView.getSettings();
        s.setJavaScriptEnabled(true);
        s.setDomStorageEnabled(true);
        s.setAllowFileAccess(true);
        s.setAllowFileAccessFromFileURLs(true);
        s.setAllowUniversalAccessFromFileURLs(true);
        s.setMediaPlaybackRequiresUserGesture(false);
        s.setCacheMode(WebSettings.LOAD_NO_CACHE);

        webView.setBackgroundColor(Color.BLACK);
        webView.setOverScrollMode(View.OVER_SCROLL_NEVER);
        webView.addJavascriptInterface(new BtBridge(), "AndroidBT");

        webView.setWebViewClient(new WebViewClient() {
            @Override
            public void onReceivedError(WebView view, int errorCode,
                                        String description, String failingUrl) {
                setStatus(description, R.color.red);
            }
        });

        webView.setWebChromeClient(new WebChromeClient() {
            @Override
            public boolean onJsAlert(WebView view, String url, String message,
                                     android.webkit.JsResult result) {
                result.confirm();
                return true;
            }
        });
    }

    private class BtBridge {
        @JavascriptInterface
        public void send(String command) {
            try {
                OutputStream out;
                synchronized (socketLock) {
                    out = output;
                }
                if (out != null) {
                    out.write((command + "\n").getBytes(StandardCharsets.UTF_8));
                    out.flush();
                }
            } catch (Exception ignored) {
            }
        }
    }

    /* ------------------------------------------------------------------ */
    /*  UI helpers                                                        */
    /* ------------------------------------------------------------------ */

    private void setStatus(String text, int colorRes) {
        if (status == null) return;
        status.setTextColor(getColor(colorRes));
        status.setText(text);
    }

    @Override
    public void onBackPressed() {
        if (webView != null && webView.getVisibility() == View.VISIBLE) {
            disconnect();
            return;
        }
        super.onBackPressed();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) hideSystemBars();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (settingsPanel != null && settingsPanel.getVisibility() == View.VISIBLE) {
            requestBtPermission();
        }
    }

    @SuppressWarnings("deprecation")
    private void hideSystemBars() {
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
    }

    @Override
    protected void onDestroy() {
        closeSocket();
        if (webView != null) {
            webView.loadUrl("about:blank");
            webView.destroy();
            webView = null;
        }
        super.onDestroy();
    }
}
