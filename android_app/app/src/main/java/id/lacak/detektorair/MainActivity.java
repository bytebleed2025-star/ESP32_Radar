package id.lacak.detektorair;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ClipData;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.webkit.JsPromptResult;
import android.webkit.JsResult;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {

    private static final int REQ_FILE = 1001;

    private WebView webView;
    private ScrollView settingsPanel;
    private ProgressBar progress;
    private EditText ipInput;
    private TextView status;

    private SharedPreferences prefs;
    private ValueCallback<Uri[]> fileCallback;

    @SuppressLint("SetJavaScriptEnabled")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        prefs = getSharedPreferences("detektorair", MODE_PRIVATE);
        webView = findViewById(R.id.webview);
        settingsPanel = findViewById(R.id.settings);
        progress = findViewById(R.id.progress);
        ipInput = findViewById(R.id.ip_input);
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

        String saved = prefs.getString("url", "http://192.168.4.1");
        ipInput.setText(displayHost(saved));

        setupWebView();

        findViewById(R.id.open).setOnClickListener(v -> openRadar());
        findViewById(R.id.wifi).setOnClickListener(v -> {
            try {
                startActivity(new Intent(Settings.ACTION_WIFI_SETTINGS));
            } catch (Exception e) {
                Toast.makeText(this, R.string.wifi_settings, Toast.LENGTH_SHORT).show();
            }
        });

        hideSystemBars();
    }

    @SuppressLint("SetJavaScriptEnabled")
    private void setupWebView() {
        WebSettings s = webView.getSettings();
        s.setJavaScriptEnabled(true);
        s.setDomStorageEnabled(true);
        s.setDatabaseEnabled(true);
        s.setLoadWithOverviewMode(true);
        s.setUseWideViewPort(true);
        s.setSupportZoom(false);
        s.setBuiltInZoomControls(false);
        s.setDisplayZoomControls(false);
        s.setMediaPlaybackRequiresUserGesture(false);
        s.setAllowFileAccess(false);
        s.setAllowContentAccess(true);
        s.setCacheMode(WebSettings.LOAD_DEFAULT);
        s.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);

        webView.setBackgroundColor(Color.BLACK);
        webView.setOverScrollMode(View.OVER_SCROLL_NEVER);

        if ((getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0) {
            WebView.setWebContentsDebuggingEnabled(true);
        }

        webView.setWebViewClient(new WebViewClient() {
            @Override
            public void onPageStarted(WebView view, String url, Bitmap favicon) {
                progress.setVisibility(View.VISIBLE);
            }

            @Override
            public void onPageFinished(WebView view, String url) {
                progress.setVisibility(View.GONE);
            }

            @Override
            public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
                if (request.isForMainFrame()) {
                    String host = request.getUrl().getHost();
                    String msg = getString(R.string.connection_failed)
                            + (host != null ? " (" + host + ")" : "")
                            + "\n" + getString(R.string.connection_hint);
                    showFormError(msg);
                }
            }

            @Override
            public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
                Uri uri = request.getUrl();
                String scheme = uri.getScheme();
                if ("http".equals(scheme) || "https".equals(scheme)) return false;
                try {
                    startActivity(new Intent(Intent.ACTION_VIEW, uri));
                } catch (Exception ignored) {
                }
                return true;
            }
        });

        webView.setWebChromeClient(new WebChromeClient() {
            @Override
            public void onProgressChanged(WebView view, int newProgress) {
                progress.setVisibility(newProgress < 100 ? View.VISIBLE : View.GONE);
            }

            @Override
            public boolean onShowFileChooser(WebView view, ValueCallback<Uri[]> callback,
                                             FileChooserParams params) {
                if (fileCallback != null) {
                    fileCallback.onReceiveValue(null);
                    fileCallback = null;
                }
                fileCallback = callback;
                try {
                    Intent intent = params.createIntent();
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    startActivityForResult(Intent.createChooser(intent,
                            getString(R.string.choose_firmware)), REQ_FILE);
                } catch (Exception e) {
                    fileCallback = null;
                    Toast.makeText(MainActivity.this, R.string.choose_firmware, Toast.LENGTH_SHORT).show();
                    return false;
                }
                return true;
            }

            @Override
            public boolean onJsAlert(WebView view, String url, String message, final JsResult result) {
                new AlertDialog.Builder(MainActivity.this, android.R.style.Theme_Material_Dialog_Alert)
                        .setTitle(R.string.app_name)
                        .setMessage(message)
                        .setPositiveButton(R.string.ok, (d, w) -> result.confirm())
                        .setOnCancelListener(d -> result.cancel())
                        .show();
                return true;
            }

            @Override
            public boolean onJsConfirm(WebView view, String url, String message, final JsResult result) {
                new AlertDialog.Builder(MainActivity.this, android.R.style.Theme_Material_Dialog_Alert)
                        .setTitle(R.string.app_name)
                        .setMessage(message)
                        .setPositiveButton(R.string.ok, (d, w) -> result.confirm())
                        .setNegativeButton(R.string.cancel, (d, w) -> result.cancel())
                        .setOnCancelListener(d -> result.cancel())
                        .show();
                return true;
            }

            @Override
            public boolean onJsPrompt(WebView view, String url, String message, String defaultValue,
                                      final JsPromptResult result) {
                final EditText input = new EditText(MainActivity.this);
                input.setSingleLine(true);
                input.setText(defaultValue != null ? defaultValue : "");
                new AlertDialog.Builder(MainActivity.this, android.R.style.Theme_Material_Dialog_Alert)
                        .setTitle(R.string.app_name)
                        .setMessage(message)
                        .setView(input)
                        .setPositiveButton(R.string.ok,
                                (d, w) -> result.confirm(input.getText().toString()))
                        .setNegativeButton(R.string.cancel, (d, w) -> result.cancel())
                        .setOnCancelListener(d -> result.cancel())
                        .show();
                return true;
            }
        });
    }

    private String displayHost(String url) {
        try {
            Uri uri = Uri.parse(url);
            String host = uri.getHost();
            if (host != null && !host.isEmpty()) {
                return uri.getPort() > 0 ? host + ":" + uri.getPort() : host;
            }
        } catch (Exception ignored) {
        }
        return url;
    }

    private void openRadar() {
        String raw = ipInput.getText().toString().trim();
        if (raw.isEmpty()) raw = getString(R.string.ip_hint);
        if (!raw.startsWith("http://") && !raw.startsWith("https://")) {
            raw = "http://" + raw;
        }

        Uri uri;
        try {
            uri = Uri.parse(raw);
        } catch (Exception e) {
            showFormError(getString(R.string.invalid_address));
            return;
        }

        String scheme = uri.getScheme();
        String host = uri.getHost();
        if (host == null || host.isEmpty()
                || (!"http".equals(scheme) && !"https".equals(scheme))) {
            showFormError(getString(R.string.invalid_address));
            return;
        }

        String url = uri.toString();
        prefs.edit().putString("url", url).apply();

        InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
        if (imm != null) imm.hideSoftInputFromWindow(ipInput.getWindowToken(), 0);

        status.setText("");
        settingsPanel.setVisibility(View.GONE);
        webView.setVisibility(View.VISIBLE);
        progress.setVisibility(View.VISIBLE);
        webView.loadUrl(url);
        hideSystemBars();
    }

    private void showFormError(String message) {
        progress.setVisibility(View.GONE);
        webView.setVisibility(View.GONE);
        settingsPanel.setVisibility(View.VISIBLE);
        status.setTextColor(getColor(R.color.red));
        status.setText(message);
        hideSystemBars();
    }

    @Override
    @SuppressWarnings("deprecation")
    public void onBackPressed() {
        if (settingsPanel.getVisibility() == View.VISIBLE) {
            super.onBackPressed();
            return;
        }
        if (webView.canGoBack()) {
            webView.goBack();
            return;
        }

        webView.stopLoading();
        webView.setVisibility(View.GONE);
        progress.setVisibility(View.GONE);
        settingsPanel.setVisibility(View.VISIBLE);
        status.setTextColor(getColor(R.color.dim));
        status.setText(R.string.paused_hint);
        hideSystemBars();
        Toast.makeText(this, R.string.back_again, Toast.LENGTH_SHORT).show();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQ_FILE) {
            if (fileCallback == null) return;
            Uri[] result = null;
            if (resultCode == RESULT_OK && data != null) {
                ClipData clip = data.getClipData();
                if (clip != null) {
                    result = new Uri[clip.getItemCount()];
                    for (int i = 0; i < clip.getItemCount(); i++) {
                        result[i] = clip.getItemAt(i).getUri();
                    }
                } else if (data.getData() != null) {
                    result = new Uri[]{data.getData()};
                }
            }
            fileCallback.onReceiveValue(result);
            fileCallback = null;
            return;
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) hideSystemBars();
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
        if (webView != null) {
            webView.loadUrl("about:blank");
            webView.destroy();
            webView = null;
        }
        fileCallback = null;
        super.onDestroy();
    }
}
