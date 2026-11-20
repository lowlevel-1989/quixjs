/* TransferActivity: Transfer Save en Android, con vistas de Android como el
   menu del juego. Las pantallas son las del launcher de Linux
   (runtime/transfer/transfer.c), una funcion show* cada una:

     showHome        Send Save, Receive Save, Restore Backup
     showDevices     a quien mandarlas: los que estan en Receive Save
     showSending     esperando a que el otro pulse Accept; Cancel
     showReceive     esperando unas partidas: este dispositivo y su firma
     showAsk         llego una oferta: Accept o Decline
     showReceiving   tras Accept, mientras llegan; Cancel
     showRestore     volver a las partidas de antes de la ultima recepcion
     showResult      como termino, con OK

   La firma de 4 letras, grande, es lo que se compara en los dos aparatos.
   La red y las partidas son las de runtime/transfer, en C (sender.c,
   receiver.c, saves.c), por runtime/platform/android/launcher.c, para el
   juego de la carpeta del extra "game". Mientras hay un envio o una
   recepcion abiertos, tick llama a sendPoll o receivePoll cada TICK_MS en
   el thread de la interfaz.

   El bloqueo de multicast, mientras esta abierta: sin el, muchos telefonos
   tiran los broadcast que llegan por Wi-Fi.

   Adaptado de tamatama (android/java/.../TransferActivity.java, MIT):
   https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/android/java/com/lowlevel1989/tamatama/TransferActivity.java */
package com.lowlevel1989.quixjs;

import android.app.Activity;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;

public class TransferActivity extends Activity {
    static final String GAME = "game";
    static final String TITLE = "title";   /* el titulo del juego, debajo de Transfer Save */
    private static final int TICK_MS = 50;

    static {
        System.loadLibrary("SDL2");
        System.loadLibrary("SDL2_ttf");
        System.loadLibrary("SDL2_image");
        System.loadLibrary("quixjs");
    }

    /* runtime/platform/android/launcher.c */
    private static native boolean start(String dir);
    private static native String[] saves(boolean backup);
    private static native boolean restore();
    private static native String[] sendOpen();
    private static native void sendClose();
    private static native boolean sendPoll();
    private static native String[] sendDevices();
    private static native String sendStart(String signature);
    private static native int sendProgress();
    private static native String sendResult();
    private static native void sendCancel();
    private static native String[] receiveOpen();
    private static native void receiveClose();
    private static native void receiveListen(boolean on);
    private static native int receivePoll();
    private static native String[] receiveOffer();
    private static native int receiveProgress();
    private static native void receiveAccept();
    private static native void receiveDecline();
    private static native void receiveCancel();
    private static native String receiveResult();
    private static native String receiveNotice();

    /* Lo que devuelve receivePoll: runtime/transfer/receiver.h */
    private static final int OFFER = 1, CANCELLED = 2, DONE = 3;

    /* Que hace tick y que hace volver */
    private enum Mode { HOME, DEVICES, SENDING, RECEIVE, ASK, RECEIVING, RESTORE, RESULT }

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Runnable tick = this::tick;
    private WifiManager.MulticastLock multicast;
    private TextView subtitle, notice, progress;
    private LinearLayout content;
    private Mode mode = Mode.HOME;
    private boolean sending, receiving; /* sendOpen o receiveOpen abiertos */
    private String me, signature;       /* los de este dispositivo */
    private String saveLine;            /* "3 files  2026-11-20 18:30" */
    private String shownDevices = "";
    private int deviceLines;            /* las vistas de showDevices antes de la lista */
    private Runnable afterResult;

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        String dir = getIntent().getStringExtra(GAME);
        if (dir == null || !start(dir)) {
            finish();
            return;
        }
        WifiManager wifi = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (wifi != null) {
            multicast = wifi.createMulticastLock("quixjs");
            multicast.setReferenceCounted(false);
            multicast.acquire();
        }
        Pad.use(new File(dir, "controller.map").getPath());
        buildScreen();
        showHome();
    }

    /* El mando: Pad.java (la asignacion de controller.map) */
    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        return Pad.key(getWindow(), event, this::onBackPressed) || super.dispatchKeyEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
        return Pad.motion(getWindow(), event, this::onBackPressed) || super.dispatchGenericMotionEvent(event);
    }

    @Override
    protected void onDestroy() {
        handler.removeCallbacks(tick);
        closeAll();
        if (multicast != null && multicast.isHeld()) multicast.release();
        super.onDestroy();
    }

    /* ---- la pantalla ---- */

    private int dp(float value) { return Ui.dp(this, value); }

    private void buildScreen() {
        String game = getIntent().getStringExtra(TITLE);
        LinearLayout screen = Ui.screen(this, Ui.title(this, "Transfer Save", 34),
                Ui.signature(this, game != null ? game : ""));
        ScrollView scroll = new ScrollView(this);
        scroll.setFocusable(false);
        scroll.setVerticalScrollBarEnabled(false);
        Ui.noFocusHighlight(scroll);
        content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(content, Ui.wide());
        screen.addView(scroll, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        notice = Ui.info(this, "", Ui.NOTICE);
        screen.addView(notice, Ui.wide());
        setContentView(screen);
        Ui.darkBars(this);
    }

    /* Una pantalla nueva: su etiqueta, sin lineas ni opciones */
    private void screen(Mode next, String label) {
        mode = next;
        content.removeAllViews();
        content.addView(Ui.section(this, label), Ui.wide());
        say("");
        progress = null;
    }

    private void say(String message) {
        notice.setText(message);
        notice.setVisibility(message.isEmpty() ? View.GONE : View.VISIBLE);
    }

    private TextView line(String text, int color) {
        TextView view = Ui.info(this, text, color);
        content.addView(view, Ui.wide());
        return view;
    }

    /* La firma: grande, que es lo que se compara en los dos aparatos */
    private void signature(String text) {
        content.addView(Ui.big(this, text), Ui.wide());
    }

    private void gap() {
        content.addView(new View(this), new LinearLayout.LayoutParams(1, dp(16)));
    }

    /* Una fila; con una raya si va debajo de otra */
    private View add(View row) {
        int n = content.getChildCount();
        if (n > 0 && content.getChildAt(n - 1).isClickable()) content.addView(Ui.divider(this));
        content.addView(row, Ui.wide());
        return row;
    }

    private View item(String text, String detail, View.OnClickListener action) {
        return add(Ui.link(this, text, detail, action));
    }

    private View primary(String text, View.OnClickListener action) {
        return add(Ui.primary(this, text, null, action));
    }

    /* ---- las pantallas ---- */

    private void showHome() {
        closeAll();
        screen(Mode.HOME, "saves");
        line("Pass your saves to another device on the same network: a PC, a handheld or a phone"
                + " with quixjs.", Ui.MUTED);
        gap();
        item("Send Save", "to a device in Receive Save", v -> showDevices()).requestFocus();
        item("Receive Save", "from a device in Send Save", v -> showReceive());
        item("Restore Backup", "the saves before the last ones received", v -> showRestore());
    }

    private void showDevices() {
        String[] save = saves(false);
        if (save.length == 0) {
            showResult("send save", "there are no saves to send", this::showHome);
            return;
        }
        saveLine = save[0] + "  " + save[1];
        if (!sending) {
            String[] mine = sendOpen();
            if (mine == null) {
                showResult("send save", "cannot use the network", this::showHome);
                return;
            }
            sending = true;
            me = mine[0];
            signature = mine[1];
        }
        screen(Mode.DEVICES, "send save");
        line(saveLine, Ui.TEXT);
        line("this device: " + me, Ui.TEXT);
        signature(signature);
        line("send it to:", Ui.TEXT);
        deviceLines = content.getChildCount();
        shownDevices = null;
        refreshDevices();
        schedule();
    }

    /* Rehace la lista si ha cambiado: cada fila, firma y nombre */
    private void refreshDevices() {
        String[] list = sendDevices();
        String key = TextUtils.join("\n", list);
        if (key.equals(shownDevices)) return;
        shownDevices = key;
        while (content.getChildCount() > deviceLines) content.removeViewAt(deviceLines);
        for (int i = 0; i < list.length; i += 2) {
            final String sig = list[i], name = list[i + 1];
            View row = item(sig, name, v -> startSending(sig, name));
            if (i == 0) row.requestFocus();
        }
        say(list.length == 0 ? "looking for devices in Receive Save..." : "");
    }

    private void startSending(String sig, String name) {
        String problem = sendStart(sig);
        if (problem != null) {
            showResult("send save", problem, this::showDevices);
            return;
        }
        screen(Mode.SENDING, "send save");
        line("waiting for " + name, Ui.TEXT);
        signature(sig);
        line("to accept it", Ui.TEXT);
        progress = line("", Ui.HIGHLIGHT);
        gap();
        item("Cancel", null, v -> cancelSending()).requestFocus();
    }

    private void cancelSending() {
        sendCancel();
        showResult("send save", "cancelled", this::showDevices);
    }

    private void showReceive() {
        if (!receiving) {
            String[] mine = receiveOpen();
            if (mine == null) {
                showResult("receive save", "cannot use the network", this::showHome);
                return;
            }
            receiving = true;
            me = mine[0];
            signature = mine[1];
        }
        screen(Mode.RECEIVE, "receive save");
        line("this device: " + me, Ui.TEXT);
        signature(signature);
        line("waiting for a save...", Ui.TEXT);
        line("choose this device in Send Save", Ui.TEXT);
        String last = receiveNotice();
        say(last != null ? last : "");
        receiveListen(true);
        schedule();
    }

    private void showAsk() {
        receiveListen(false);
        String[] offer = receiveOffer();
        screen(Mode.ASK, "receive save");
        line("a save from " + offer[0], Ui.TEXT);
        signature(offer[1]);
        line(offer[2] + "  " + offer[3], Ui.TEXT);
        line("your saves will be kept as backup", Ui.NOTICE);
        gap();
        primary("Accept", v -> {
            receiveAccept();
            showReceiving();
        }).requestFocus();
        item("Decline", null, v -> {
            receiveDecline();
            showReceive();
        });
    }

    private void showReceiving() {
        screen(Mode.RECEIVING, "receive save");
        progress = line("receiving...", Ui.TEXT);
        gap();
        item("Cancel", null, v -> {
            receiveCancel();
            showReceive();
        }).requestFocus();
    }

    private void showRestore() {
        String[] backup = saves(true);
        if (backup.length == 0) {
            showResult("restore backup", "there is no backup yet", this::showHome);
            return;
        }
        screen(Mode.RESTORE, "restore backup");
        line("back to the saves before", Ui.TEXT);
        line("the last ones received:", Ui.TEXT);
        line(backup[0] + "  " + backup[1], Ui.HIGHLIGHT);
        line("the current ones will be the backup", Ui.NOTICE);
        gap();
        primary("Restore", v -> {
            boolean ok = restore();
            showResult("restore backup", ok ? "backup restored" : "cannot restore the backup", this::showHome);
        }).requestFocus();
    }

    /* Como termino, con OK. La red sigue atendiendo mientras se ve */
    private void showResult(String title, String message, Runnable then) {
        screen(Mode.RESULT, title);
        line(message, Ui.TEXT);
        gap();
        afterResult = then;
        primary("OK", v -> then.run()).requestFocus();
    }

    /* ---- la red ---- */

    private void schedule() {
        handler.removeCallbacks(tick);
        handler.postDelayed(tick, TICK_MS);
    }

    private void tick() {
        if (sending) {
            boolean finished = sendPoll();
            if (mode == Mode.DEVICES) refreshDevices();
            if (mode == Mode.SENDING) {
                int percent = sendProgress();
                if (progress != null) progress.setText(percent >= 0 ? "sending " + percent + "%" : "");
                if (finished) {
                    String result = sendResult();
                    boolean sent = "save sent".equals(result);
                    showResult("send save", result, sent ? this::showHome : this::showDevices);
                }
            }
        }
        if (receiving) {
            int event = receivePoll();
            if (mode == Mode.RECEIVE && event == OFFER) showAsk();
            else if (mode == Mode.ASK && event == CANCELLED) showReceive();
            else if (mode == Mode.RECEIVING && event == DONE) {
                String result = receiveResult();
                boolean received = "save received".equals(result);
                showResult("receive save", result, received ? this::showHome : this::showReceive);
            } else if (mode == Mode.RECEIVING && progress != null) {
                progress.setText("receiving " + receiveProgress() + "%");
            }
            if (mode == Mode.RECEIVE) {
                String last = receiveNotice();
                say(last != null ? last : "");
            }
        }
        if (sending || receiving) handler.postDelayed(tick, TICK_MS);
    }

    private void closeAll() {
        handler.removeCallbacks(tick);
        if (sending) sendClose();
        if (receiving) receiveClose();
        sending = receiving = false;
    }

    /* Volver: a la pantalla de antes, cancelando lo que este en marcha */
    @Override
    public void onBackPressed() {
        switch (mode) {
            case HOME:
                super.onBackPressed();
                break;
            case SENDING:
                cancelSending();
                break;
            case ASK:
                receiveDecline();
                showReceive();
                break;
            case RECEIVING:
                receiveCancel();
                showReceive();
                break;
            case RESULT:
                if (afterResult != null) afterResult.run();
                break;
            default:
                showHome();
        }
    }
}
