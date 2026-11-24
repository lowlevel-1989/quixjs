/* MainActivity: la pantalla de inicio de la app, con vistas de Android y los
   colores del launcher de quixjs: filas grandes, scroll y foco del mando.

     quixjs
     una fila por juego       su menu (GameMenuActivity, con el extra
                              "game": la carpeta del juego)
     Import game              el selector de archivos: un .zip con un port
     Licenses

   La app no trae juegos ni ports: cada juego se importa desde un .zip con
   la carpeta completa de su port, como queda en una portatil con
   PortMaster (port.toml, la rom del juego, mods/, partidas...). Asi
   la misma app juega cualquier port de quixjs. Abrir un .zip con la app,
   desde un gestor de archivos, importa igual. Importar es Importer.java.

   Adaptado de tamatama (android/java/.../MainActivity.java, MIT):
   https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/android/java/com/lowlevel1989/tamatama/MainActivity.java */
package com.lowlevel1989.quixjs;

import android.app.Activity;
import android.Manifest;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.net.Uri;
import android.os.Bundle;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class MainActivity extends Activity {
    private static final int MENU = 2;   /* Importer.PICK es 1 */

    private final Importer importer = new Importer(this);

    private TextView notice;
    private LinearLayout games;
    private View importRow;
    private boolean busy;
    private String pending;   /* el aviso de GameMenuActivity (un juego borrado) */

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        // las capturas (Select + R1) van a la galeria: en Android 9 y antes
        // eso pide este permiso (desde Android 10, MediaStore no lo necesita)
        if (Build.VERSION.SDK_INT <= 28
                && checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED)
            requestPermissions(new String[] {Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0);
        buildScreen();
        Uri uri = incoming(getIntent());
        if (uri != null) importer.start(uri);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        Uri uri = incoming(intent);
        if (uri != null) importer.start(uri);
    }

    @Override
    protected void onResume() {
        super.onResume();
        Pad.use(mapFiles());
        if (!busy) refresh(pending);
        pending = null;
    }

    /* El mando en la lista de juegos: la asignacion de cualquier juego (la
       mas reciente, ver Pad.java) */
    private String[] mapFiles() {
        List<File> found = installed();
        String[] maps = new String[found.size()];
        for (int i = 0; i < maps.length; i++) maps[i] = new File(found.get(i), "controller.map").getPath();
        return maps;
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

    /* El archivo con el que se ha abierto la app, si lo hay */
    private static Uri incoming(Intent intent) {
        if (intent == null) return null;
        if (Intent.ACTION_VIEW.equals(intent.getAction())) return intent.getData();
        if (Intent.ACTION_SEND.equals(intent.getAction())) return intent.getParcelableExtra(Intent.EXTRA_STREAM);
        return null;
    }

    /* ---- la pantalla ---- */

    private int dp(float value) { return Ui.dp(this, value); }

    private void buildScreen() {
        TextView title = Ui.title(this, "quixjs", 56);
        /* la version de quixjs, pequena, como la firma del launcher de Linux */
        LinearLayout screen = Ui.screen(this, title, Ui.signature(this, "version " + version()));
        title.setLetterSpacing(0.12f);

        ScrollView scroll = new ScrollView(this);
        scroll.setFocusable(false);
        scroll.setVerticalScrollBarEnabled(false);
        Ui.noFocusHighlight(scroll);
        games = new LinearLayout(this);
        games.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(games, Ui.wide());
        screen.addView(scroll, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

        notice = Ui.info(this, "", Ui.NOTICE);
        screen.addView(notice, Ui.wide());
        screen.addView(Ui.divider(this));
        importRow = Ui.primary(this, "Import game", "a .zip with the folder of a port", v -> pick());
        screen.addView(importRow, Ui.wide());
        /* un enlace pequeno, como en tamatama */
        TextView licenses = Ui.text(this, "Licenses", 13, Ui.MUTED, false);
        licenses.setGravity(android.view.Gravity.CENTER);
        licenses.setPaintFlags(licenses.getPaintFlags() | android.graphics.Paint.UNDERLINE_TEXT_FLAG);
        licenses.setPadding(0, dp(12), 0, dp(4));
        licenses.setFocusable(true);
        Ui.focusBackground(this, licenses);
        licenses.setOnClickListener(v -> showLicenses());
        screen.addView(licenses, Ui.wide());

        setContentView(screen);
        Ui.darkBars(this);
    }

    /* La version de la app: la de quixjs (VERSION, en cmake/android/apk.cmake) */
    private String version() {
        try {
            return getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
        } catch (android.content.pm.PackageManager.NameNotFoundException e) {
            return "";
        }
    }

    /* Los juegos importados: {files}/games/<id>/port.toml con su carpeta del
       juego ([game] game_dir). message va al aviso */
    void refresh(String message) {
        busy = false;
        importRow.setEnabled(true);
        say(message != null ? message : "");
        games.removeAllViews();
        List<File> found = installed();
        games.addView(Ui.section(this, "games"), Ui.wide());
        for (File dir : found) {
            final String id = dir.getName();
            String version = Files.gameValue(Files.read(new File(dir, "port.toml")), "version");
            if (games.getChildCount() > 1) games.addView(Ui.divider(this));
            games.addView(Ui.link(this, Files.titleOf(dir), version != null ? "port " + version : id, v -> play(id)),
                    Ui.wide());
        }
        if (found.isEmpty()) {
            games.addView(Ui.info(this, "No games yet.", Ui.TEXT), Ui.wide());
            games.addView(Ui.info(this, "quixjs plays games made with JavaScript (RPG Maker MV and MZ...)"
                    + " that you own, with a gamepad.\n\n"
                    + "Import a .zip with the folder of a quixjs port, as on a PortMaster handheld:"
                    + " its port.toml, and the game files inside gamedata/.", Ui.MUTED), Ui.wide());
            importRow.post(importRow::requestFocus);
            return;
        }
        /* despues de onResume: antes, la ventana aun no tiene las vistas */
        View first = games.getChildAt(1);
        first.post(first::requestFocus);
    }

    void say(String message) {
        notice.setText(message);
        notice.setVisibility(message.isEmpty() ? View.GONE : View.VISIBLE);
    }

    /* Las carpetas de {files}/games con port.toml y el juego dentro: su rom
       ([game] rom, un .zip o una carpeta junto al port.toml: quixjs la monta
       al jugar) o, sin rom, la carpeta de game_dir */
    List<File> installed() {
        List<File> found = new ArrayList<>();
        File[] dirs = new File(getFilesDir(), "games").listFiles(File::isDirectory);
        if (dirs == null) return found;
        Arrays.sort(dirs);
        for (File dir : dirs) {
            String toml = Files.read(new File(dir, "port.toml"));
            String rom = Files.gameValue(toml, "rom"), gameDir = Files.value(toml, "game_dir");
            if (rom != null ? new File(dir, rom).exists() : gameDir != null && new File(dir, gameDir).isDirectory())
                found.add(dir);
        }
        return found;
    }

    private void play(String id) {
        if (busy) return;
        File dir = new File(new File(getFilesDir(), "games"), id);
        try {
            installFont();   /* la de las pantallas de quixjs */
        } catch (IOException e) {
            say("cannot prepare " + id);
            return;
        }
        Intent intent = new Intent(this, GameMenuActivity.class);
        intent.putExtra(GameMenuActivity.GAME, dir.getAbsolutePath());
        startActivityForResult(intent, MENU);
    }

    /* la fuente de las pantallas de quixjs: {files}/fonts (QUIXJS_FONT) */
    private void installFont() throws IOException {
        File font = new File(new File(getFilesDir(), "fonts"), "DejaVuSans-Bold.ttf");
        if (font.exists()) return;
        font.getParentFile().mkdirs();
        try (InputStream in = getAssets().open("fonts/DejaVuSans-Bold.ttf");
             OutputStream out = new FileOutputStream(font)) {
            Files.copy(in, out);
        }
    }

    /* ---- licencias ---- */

    /* Las licencias de lo que lleva la app (assets/licenses, de cmake/android/apk.cmake),
       una detras de otra, con el nombre de cada una */
    private void showLicenses() {
        StringBuilder all = new StringBuilder();
        try {
            String[] names = getAssets().list("licenses");
            if (names != null) {
                Arrays.sort(names);
                for (String name : names) {
                    all.append("==== ").append(name.replace(".txt", "")).append(" ====\n\n");
                    all.append(Files.asset(this, "licenses/" + name)).append("\n\n");
                }
            }
        } catch (IOException e) {
            all.append("cannot read the licenses");
        }
        TextView body = Ui.text(this, all.toString(), 11, Ui.TEXT, false);
        body.setGravity(Gravity.START);
        body.setPadding(dp(16), dp(16), dp(16), dp(16));
        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(Ui.BACKGROUND);
        scroll.addView(body);
        AlertDialog dialog = new AlertDialog.Builder(this, android.R.style.Theme_Material_Dialog_Alert)
                .setTitle("Licenses")
                .setView(scroll)
                .setPositiveButton(android.R.string.ok, null)
                .create();
        Pad.dialog(dialog);
        dialog.show();
    }

    /* ---- importar (Importer.java) ---- */

    private void pick() {
        if (!busy) importer.pick();
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request == Importer.PICK && result == RESULT_OK && data != null && data.getData() != null)
            importer.start(data.getData());
        if (request == MENU && data != null) pending = data.getStringExtra(GameMenuActivity.DELETED);
    }

    /* Empieza un import: hasta refresh, sin jugar ni importar otro */
    void importing() {
        busy = true;
        importRow.setEnabled(false);
        say("importing...");
    }
}
