/* TransferActivity: Transfer Save en Android, con vistas de Android como el
   menu del juego. Las pantallas son las del launcher de Linux
   (runtime/transfer/transfer.c):

     showHome          Send Save, Receive Save, Restore Backup, Delete Save
     TransferSend      Send Save: a quien mandarlas y el envio
     TransferReceive   Receive Save: esperar, aceptar o rechazar, recibir
     showRestore       volver a las partidas de antes de la ultima recepcion
     showDelete        las partidas al backup (el juego empieza de cero)
     showResult        como termino, con OK

   La firma de 4 letras, grande, es lo que se compara en los dos aparatos.
   La red y las partidas son las de runtime/transfer, en C (sender.c,
   receiver.c, saves.c), por runtime/platform/android/transfer.c, para el
   juego de la carpeta del extra "game". Mientras hay un envio o una
   recepcion abiertos, tick llama a su poll cada TICK_MS en el thread de la
   interfaz.

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

    /* runtime/platform/android/transfer.c */
    private static native boolean start(String dir);
    static native String[] saves(boolean backup);
    private static native boolean restore();
    private static native int deleteSave();
    static native String[] sendOpen();
    static native void sendClose();
    static native boolean sendPoll();
    static native String[] sendDevices();
    static native String sendStart(String signature);
    static native int sendProgress();
    static native String sendResult();
    static native void sendCancel();
    static native String[] receiveOpen();
    static native void receiveClose();
    static native void receiveListen(boolean on);
    static native int receivePoll();
    static native String[] receiveOffer();
    static native int receiveProgress();
    static native void receiveAccept();
    static native void receiveDecline();
    static native void receiveCancel();
    static native String receiveResult();
    static native String receiveNotice();

    /* Que hace tick y que hace volver */
    enum Mode { HOME, DEVICES, SENDING, RECEIVE, ASK, RECEIVING, RESTORE, DELETE, RESULT }

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Runnable tick = this::tick;
    private final TransferSend send = new TransferSend(this);
    private final TransferReceive receive = new TransferReceive(this);
    private WifiManager.MulticastLock multicast;
    private TextView notice;
    TextView progress;   /* el porcentaje del envio o la recepcion */
    LinearLayout content;
    Mode mode = Mode.HOME;
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
    void screen(Mode next, String label) {
        mode = next;
        content.removeAllViews();
        content.addView(Ui.section(this, label), Ui.wide());
        say("");
        progress = null;
    }

    void say(String message) {
        notice.setText(message);
        notice.setVisibility(message.isEmpty() ? View.GONE : View.VISIBLE);
    }

    TextView line(String text, int color) {
        TextView view = Ui.info(this, text, color);
        content.addView(view, Ui.wide());
        return view;
    }

    /* La firma: grande, que es lo que se compara en los dos aparatos */
    void signature(String text) {
        content.addView(Ui.big(this, text), Ui.wide());
    }

    void gap() {
        content.addView(new View(this), new LinearLayout.LayoutParams(1, dp(16)));
    }

    /* Una fila; con una raya si va debajo de otra */
    private View add(View row) {
        int n = content.getChildCount();
        if (n > 0 && content.getChildAt(n - 1).isClickable()) content.addView(Ui.divider(this));
        content.addView(row, Ui.wide());
        return row;
    }

    View item(String text, String detail, View.OnClickListener action) {
        return add(Ui.link(this, text, detail, action));
    }

    View primary(String text, View.OnClickListener action) {
        return add(Ui.primary(this, text, null, action));
    }

    /* ---- las pantallas ---- */

    void showHome() {
        closeAll();
        screen(Mode.HOME, "saves");
        line("Pass your saves to another device on the same network: a PC, a handheld or a phone"
                + " with quixjs.", Ui.MUTED);
        gap();
        item("Send Save", "to a device in Receive Save", v -> send.showDevices()).requestFocus();
        item("Receive Save", "from a device in Send Save", v -> receive.show());
        item("Restore Backup", "the saves before the last ones received", v -> showRestore());
        item("Delete Save", "start the game from zero", v -> showDelete());
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

    /* Las partidas pasan a ser el backup (Restore Backup las devuelve); los
       logros se quedan. Back viene elegido: borrar pide moverse a Delete */
    private void showDelete() {
        if (saves(false).length == 0) {
            showResult("delete save", "there are no saves", this::showHome);
            return;
        }
        screen(Mode.DELETE, "delete save");
        line("the game starts from zero;", Ui.TEXT);
        line("restore backup brings", Ui.TEXT);
        line("the saves back", Ui.TEXT);
        line("the old backup is lost", Ui.NOTICE);
        gap();
        item("Back", null, v -> showHome()).requestFocus();
        primary("Delete", v -> {
            int r = deleteSave();
            showResult("delete save", r > 0 ? "saves deleted" : r < 0 ? "there are no saves" : "cannot delete the saves",
                       this::showHome);
        });
    }

    /* Como termino, con OK. La red sigue atendiendo mientras se ve */
    void showResult(String title, String message, Runnable then) {
        screen(Mode.RESULT, title);
        line(message, Ui.TEXT);
        gap();
        afterResult = then;
        primary("OK", v -> then.run()).requestFocus();
    }

    /* ---- la red ---- */

    void schedule() {
        handler.removeCallbacks(tick);
        handler.postDelayed(tick, TICK_MS);
    }

    private void tick() {
        if (send.open) send.poll();
        if (receive.open) receive.poll();
        if (send.open || receive.open) handler.postDelayed(tick, TICK_MS);
    }

    private void closeAll() {
        handler.removeCallbacks(tick);
        send.close();
        receive.close();
    }

    /* Volver: a la pantalla de antes, cancelando lo que este en marcha */
    @Override
    public void onBackPressed() {
        switch (mode) {
            case HOME:
                super.onBackPressed();
                break;
            case SENDING:
                send.cancel();
                break;
            case ASK:
                receive.decline();
                break;
            case RECEIVING:
                receive.cancel();
                break;
            case RESULT:
                if (afterResult != null) afterResult.run();
                break;
            default:
                showHome();
        }
    }
}
