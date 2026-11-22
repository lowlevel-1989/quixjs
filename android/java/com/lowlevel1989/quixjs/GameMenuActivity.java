/* GameMenuActivity: el menu de un juego, el launcher de quixjs con vistas de
   Android (el lenguaje de Ui.java). Las mismas opciones y en el mismo orden
   que el de Linux (runtime/launcher/launcher.c). Arriba el titulo y la
   firma; una funcion show* por pantalla:

     showHome           Play; GAME: Achievements (si hay [[achievements]]),
                        Mods (si hay carpetas en mods/), Options, Cheats (si
                        hay [[cheat]]), Transfer Save (si hay [saves] files),
                        Controls (si [controls] gamepad); MANAGE: Delete mods,
                        Delete game
     showAchievements   cuantos van, por tipo, y la lista con su descripcion
     showMods           cada carpeta de mods/, On u Off (y su prioridad)
     showOptions        Show FPS, Profiler, Vibration, Save log y Save profile (si
                        estan: log.txt y quixjs-profile.txt, a donde se
                        elija) y Reset achievements
     showCheats         cada truco, On u Off
     showControls       que asignacion usa el mando; Map buttons abre la
                        pantalla Controls de quixjs (GameActivity con
                        --controls): el mismo codigo y el mismo controller.map
                        que en Linux
     showDeleteMods     cada mod, para borrarlo

   Borrar y Reset achievements piden confirmacion (Ui.confirm). No hay
   filas Back ni Exit: se vuelve con el gesto o el boton Atras de Android,
   o con b en el mando (Pad.java).

   Lo que se ve y lo que se cambia sale del C del launcher, por JNI
   (runtime/platform/android/launcher.c): el toml del juego, los logros,
   los trucos y quixjs-settings.toml, que se guarda al cambiar algo, como en
   Linux. El juego lo lee al arrancar.

   Adaptado de tamatama (android/java/.../TransferActivity.java, MIT):
   https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/android/java/com/lowlevel1989/tamatama/TransferActivity.java */
package com.lowlevel1989.quixjs;

import android.app.Activity;
import android.content.Intent;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class GameMenuActivity extends Activity {
    static final String GAME = "game";
    /* lo que vuelve a MainActivity al borrar el juego: el aviso */
    static final String DELETED = "deleted";

    static {
        System.loadLibrary("SDL2");
        System.loadLibrary("SDL2_ttf");
        System.loadLibrary("SDL2_image");
        System.loadLibrary("quixjs");
    }

    /* runtime/platform/android/launcher.c */
    private static native boolean open(String dir);
    private static native String[] header();
    private static native String[] menu();
    private static native String[] achievements();
    private static native void resetAchievements();
    private static native boolean[] options();
    private static native void toggleOption(int which);
    private static native String[] mods();
    private static native void toggleMod(String name);
    private static native String[] cheats();
    private static native void toggleCheat(int index);

    /* Que hace volver */
    private enum Mode { HOME, ACHIEVEMENTS, MODS, OPTIONS, CHEATS, CONTROLS, DELETE_MODS }

    private String dir, gameTitle, version;
    private TextView title, signature, notice;
    private LinearLayout content;
    private ScrollView scroll;
    private Mode mode = Mode.HOME;
    private String focusKey;   /* la fila del menu que recupera el foco al volver */

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        dir = getIntent().getStringExtra(GAME);
        buildScreen();
    }

    @Override
    protected void onResume() {
        super.onResume();
        /* al volver del juego, de Controls o de Transfer Save: los logros,
           las partidas y los ajustes pueden haber cambiado */
        if (dir == null || !new File(dir).isDirectory() || !open(dir)) {
            finish();
            return;
        }
        Pad.use(new File(dir, "controller.map").getPath());
        String[] head = header();
        gameTitle = head[0];
        version = head[1];
        redraw();
    }

    /* ---- la pantalla ---- */

    private int dp(float value) { return Ui.dp(this, value); }

    private void buildScreen() {
        title = Ui.title(this, "", 34);
        signature = Ui.signature(this, "");
        LinearLayout screen = Ui.screen(this, title, signature);
        scroll = new ScrollView(this);
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

    /* Una pantalla nueva: el titulo y la linea de debajo, sin contenido */
    private void screen(Mode next, String head, String line) {
        mode = next;
        title.setText(head);
        signature.setText(line);
        content.removeAllViews();
        say("");
        scroll.scrollTo(0, 0);
    }

    private void say(String message) {
        notice.setText(message);
        notice.setVisibility(message.isEmpty() ? View.GONE : View.VISIBLE);
    }

    private void section(String name) {
        content.addView(Ui.section(this, name), Ui.wide());
    }

    private TextView info(String text, int color) {
        TextView view = Ui.info(this, text, color);
        content.addView(view, Ui.wide());
        return view;
    }

    /* Una fila; con una raya si va debajo de otra */
    private void add(View row) {
        int n = content.getChildCount();
        if (n > 0 && content.getChildAt(n - 1).isClickable()) content.addView(Ui.divider(this));
        content.addView(row, Ui.wide());
    }

    /* El foco: en la fila key si esta, si no en la primera. Despues de
       onResume: antes, la ventana aun no tiene las vistas */
    private void focus(String key) {
        content.post(() -> focusNow(key));
    }

    private void focusNow(String key) {
        View first = null;
        for (int i = 0; i < content.getChildCount(); i++) {
            View child = content.getChildAt(i);
            if (!child.isFocusable()) continue;
            if (first == null) first = child;
            if (key != null && key.equals(child.getTag())) {
                child.requestFocus();
                return;
            }
        }
        if (first != null) first.requestFocus();
    }

    /* Rehace la pantalla de ahora (al volver a la app) */
    private void redraw() {
        switch (mode) {
            case ACHIEVEMENTS: showAchievements(); break;
            case MODS: showMods(null); break;
            case OPTIONS: showOptions(exportKey); exportKey = null; break;
            case CHEATS: showCheats(null); break;
            case CONTROLS: showControls(); break;
            case DELETE_MODS: showDeleteMods(); break;
            default: showHome();
        }
    }

    private static int unlocked(String[] achievements) {
        int got = 0;
        for (int i = 4; i < achievements.length; i += 5) if (!achievements[i].isEmpty()) got++;
        return got;
    }

    /* ---- las pantallas ---- */

    private void showHome() {
        screen(Mode.HOME, gameTitle, version);
        String[] list = achievements();
        String[] mods = mods();
        int on = 0;
        for (int i = 1; i < mods.length; i += 2) if (!mods[i].isEmpty()) on++;
        boolean[] opts = options();

        for (String name : menu()) {
            if (name.equals("Play")) {
                add(Ui.primary(this, "Play", null, v -> play(false)));
                section("game");
                continue;
            }
            String detail;
            View.OnClickListener action;
            switch (name) {
                case "Achievements":
                    detail = unlocked(list) + " of " + list.length / 5 + " unlocked";
                    action = v -> showAchievements();
                    break;
                case "Mods":
                    detail = on + " of " + mods.length / 2 + " on";
                    action = v -> showMods(null);
                    break;
                case "Options":
                    detail = (opts[4] ? "30" : "60") + " fps, show fps " + (opts[0] ? "on" : "off") + ", profiler " + (opts[1] ? "on" : "off")
                            + (opts[3] ? ", vibration " + (opts[2] ? "on" : "off") : "");
                    action = v -> showOptions(null);
                    break;
                case "Cheats":
                    detail = cheatsOn() + " of " + cheats().length / 3 + " on";
                    action = v -> showCheats(null);
                    break;
                case "Transfer Save":
                    detail = "send or receive your saves";
                    action = v -> transfer();
                    break;
                case "Controls":
                    detail = new File(dir, "controller.map").exists() ? "your mapping" : "default mapping";
                    action = v -> showControls();
                    break;
                default:
                    continue;
            }
            final String key = name;
            final View.OnClickListener go = action;
            add(Ui.link(this, name, detail, v -> {
                focusKey = key;
                go.onClick(v);
            }));
        }
        section("manage");
        if (mods.length > 0) add(Ui.link(this, "Delete mods", mods.length / 2 + " installed", v -> {
            focusKey = "Delete mods";
            showDeleteMods();
        }));
        add(Ui.danger(this, "Delete game", "its files, mods and saves", v -> deleteGame()));
        focus(focusKey);
    }

    private int cheatsOn() {
        String[] list = cheats();
        int on = 0;
        for (int i = 2; i < list.length; i += 3) if ("1".equals(list[i])) on++;
        return on;
    }

    /* El juego, o solo la pantalla Controls de quixjs */
    private void play(boolean controls) {
        focusKey = controls ? "Controls" : "Play";
        Intent intent = new Intent(this, GameActivity.class);
        intent.putExtra(GameActivity.GAME, dir);
        intent.putExtra(GameActivity.CONTROLS, controls);
        startActivity(intent);
    }

    private void transfer() {
        Intent intent = new Intent(this, TransferActivity.class);
        intent.putExtra(TransferActivity.GAME, dir);
        intent.putExtra(TransferActivity.TITLE, gameTitle);
        startActivity(intent);
    }

    /* Un cuadrado del color de un tipo de logro, delante del texto */
    private void badge(TextView view, int rgb) {
        GradientDrawable square = new GradientDrawable();
        square.setColor(0xff000000 | rgb);
        square.setCornerRadius(dp(2));
        square.setSize(dp(10), dp(10));
        view.setCompoundDrawablesRelativeWithIntrinsicBounds(square, null, null, null);
        view.setCompoundDrawablePadding(dp(10));
    }

    /* Los logros: cuantos van de cada tipo y la lista (solo se leen): los
       conseguidos en el color del texto, con su fecha; los que faltan en
       gris */
    private void showAchievements() {
        String[] list = achievements();
        int count = list.length / 5;
        screen(Mode.ACHIEVEMENTS, "Achievements", gameTitle);
        info(unlocked(list) + " of " + count + " unlocked", Ui.TEXT);
        List<String> colors = new ArrayList<>();
        for (int i = 0; i < count; i++) if (!colors.contains(list[5 * i + 3])) colors.add(list[5 * i + 3]);
        for (String color : colors) {
            int n = 0, done = 0;
            String type = "";
            for (int i = 0; i < count; i++) {
                if (!list[5 * i + 3].equals(color)) continue;
                n++;
                if (!list[5 * i + 4].isEmpty()) done++;
                if (type.isEmpty()) type = list[5 * i + 2];
            }
            TextView legend = info((type.isEmpty() ? "other" : type) + "  " + done + "/" + n, Ui.MUTED);
            legend.setTextSize(13);
            badge(legend, (int) Long.parseLong(color));
        }
        section("list");
        for (int i = 0; i < count; i++) {
            boolean got = !list[5 * i + 4].isEmpty();
            LinearLayout entry = new LinearLayout(this);
            entry.setOrientation(LinearLayout.VERTICAL);
            entry.setPadding(dp(4), dp(10), dp(4), dp(10));
            TextView name = Ui.text(this, list[5 * i], 17, got ? Ui.TEXT : Ui.MUTED, false);
            badge(name, (int) Long.parseLong(list[5 * i + 3]));
            entry.addView(name, Ui.wide());
            String more = list[5 * i + 1];
            String when = got ? "unlocked " + list[5 * i + 4] : "locked";
            TextView below = Ui.text(this, (more.isEmpty() ? "" : more + "\n") + when, 13, Ui.MUTED, false);
            below.setLineSpacing(0, 1.15f);
            below.setPadding(dp(20), dp(3), 0, 0);
            entry.addView(below, Ui.wide());
            if (i > 0) content.addView(Ui.divider(this));
            content.addView(entry, Ui.wide());
        }
    }

    /* Mods: los activos con su prioridad; si dos traen el mismo archivo, gana
       el de numero mas alto */
    private void showMods(String key) {
        String[] list = mods();
        screen(Mode.MODS, "Mods", gameTitle);
        info("If two mods change the same file, the one with the higher priority wins."
                + " Changes apply on Play.", Ui.MUTED);
        section("mods");
        for (int i = 0; i < list.length; i += 2) {
            final String name = list[i];
            boolean on = !list[i + 1].isEmpty();
            add(Ui.toggle(this, name, on ? "priority " + list[i + 1] : null, on, v -> {
                toggleMod(name);
                showMods(name);
            }));
        }
        focus(key);
    }

    private void showOptions(String key) {
        boolean[] on = options();
        screen(Mode.OPTIONS, "Options", gameTitle);
        section("game");
        add(Ui.link(this, "FPS: " + (on[4] ? "30" : "60"), "30 saves battery", v -> {
            toggleOption(3);
            showOptions("FPS: " + (on[4] ? "60" : "30"));   /* el foco, en la misma fila */
        }));
        add(Ui.toggle(this, "Show FPS", "the frame counter, top right", on[0], v -> {
            toggleOption(0);
            showOptions("Show FPS");
        }));
        add(Ui.toggle(this, "Profiler", "writes quixjs-profile.txt", on[1], v -> {
            toggleOption(1);
            showOptions("Profiler");
        }));
        /* solo si el juego vibra ([controls] vibration en su port.toml) */
        if (on[3]) add(Ui.toggle(this, "Vibration", "the controller, or the phone if it cannot", on[2], v -> {
            toggleOption(2);
            showOptions("Vibration");
        }));
        /* log.txt y quixjs-profile.txt quedan dentro de la app: se guardan
           donde se elija (el selector de Android), para mandarlos */
        File log = new File(dir, "log.txt"), profile = new File(dir, "quixjs-profile.txt");
        if (log.exists() || profile.exists()) section("files");
        if (log.exists()) add(Ui.link(this, "Save log", "log.txt of the last game, " + size(log), v -> {
            exportKey = "Save log";
            export(log);
        }));
        if (profile.exists()) add(Ui.link(this, "Save profile", "quixjs-profile.txt, " + size(profile), v -> {
            exportKey = "Save profile";
            export(profile);
        }));
        String[] list = achievements();
        if (list.length > 0) {
            final int got = unlocked(list);
            section("achievements");
            add(Ui.danger(this, "Reset achievements", got + " unlocked", v ->
                    Ui.confirm(this, "Reset achievements?",
                            "Forget " + got + " unlocked achievements. This cannot be undone.", "Reset", () -> {
                                resetAchievements();
                                showOptions("Reset achievements");
                                say("Done: no achievements unlocked.");
                            })));
        }
        focus(key);
    }

    private static String size(File file) {
        long bytes = file.length();
        return bytes < 1024 ? bytes + " bytes" : (bytes + 1023) / 1024 + " KB";
    }

    /* Guardar un archivo de la app donde se elija: ACTION_CREATE_DOCUMENT
       (sin permisos ni FileProvider); onActivityResult lo copia */
    private static final int EXPORT = 1;
    private File exporting;
    private String exportKey;   /* la fila que recupera el foco al volver del selector */

    private void export(File file) {
        exporting = file;
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("text/plain");
        intent.putExtra(Intent.EXTRA_TITLE, new File(dir).getName() + "-" + file.getName());
        try {
            startActivityForResult(intent, EXPORT);
        } catch (RuntimeException e) {
            say("cannot open the file chooser");
        }
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != EXPORT || exporting == null) return;
        File file = exporting;
        exporting = null;
        if (result != RESULT_OK || data == null || data.getData() == null) return;
        String message;
        try (java.io.InputStream in = new java.io.FileInputStream(file);
             java.io.OutputStream out = getContentResolver().openOutputStream(data.getData(), "wt")) {
            if (out == null) throw new java.io.IOException();
            byte[] buffer = new byte[65536];
            for (int n; (n = in.read(buffer)) > 0; ) out.write(buffer, 0, n);
            message = "Done: " + file.getName() + " saved.";
        } catch (java.io.IOException | RuntimeException e) {
            message = "cannot save " + file.getName();
        }
        /* onResume redibuja despues: el aviso, cuando ya se ve */
        final String done = message;
        content.post(() -> say(done));
    }

    /* Cheats: una partida jugada con uno activo pierde los logros */
    private void showCheats(String key) {
        String[] list = cheats();
        screen(Mode.CHEATS, "Cheats", gameTitle);
        info("A playthrough that uses a cheat can't unlock achievements. Other playthroughs still can.",
                Ui.NOTICE);
        section("cheats");
        for (int i = 0; i < list.length; i += 3) {
            final int index = i / 3;
            final String name = list[i];
            add(Ui.toggle(this, name, list[i + 1], "1".equals(list[i + 2]), v -> {
                toggleCheat(index);
                showCheats(name);
            }));
        }
        focus(key);
    }

    /* Controls: la asignacion del mando de este juego. La pantalla que
       asigna los botones es la de quixjs (GameActivity con --controls) */
    private void showControls() {
        boolean mine = new File(dir, "controller.map").exists();
        screen(Mode.CONTROLS, "Controls", gameTitle);
        info(mine ? "Your gamepad uses your mapping (controller.map)."
                : "Your gamepad uses its default mapping.", Ui.TEXT);
        info("Map buttons: connect the gamepad, then press each button it asks for."
                + " Start skips what your gamepad does not have; Back cancels.", Ui.MUTED);
        section("gamepad");
        add(Ui.link(this, "Map buttons", "the quixjs controls screen", v -> play(true)));
        if (mine) add(Ui.danger(this, "Use the default mapping", "forget controller.map", v ->
                Ui.confirm(this, "Use the default mapping?", "Your mapping of this game is deleted.", "Delete",
                        () -> {
                            new File(dir, "controller.map").delete();
                            showControls();
                            say("Done: the default mapping.");
                        })));
        focus(null);
    }

    /* Borrar mods: cada uno con confirmacion */
    private void showDeleteMods() {
        String[] list = mods();
        if (list.length == 0) {
            showHome();
            return;
        }
        screen(Mode.DELETE_MODS, "Delete mods", gameTitle);
        info("To use a deleted mod again, import it again.", Ui.MUTED);
        section("mods");
        for (int i = 0; i < list.length; i += 2) {
            final String name = list[i];
            final boolean on = !list[i + 1].isEmpty();
            add(Ui.danger(this, name, on ? "on" : "off", v ->
                    Ui.confirm(this, "Delete " + name + "?", "Its files are removed from this phone.", "Delete",
                            () -> {
                                /* si estaba activo, primero fuera de quixjs-settings.toml */
                                if (on) toggleMod(name);
                                boolean ok = delete(new File(new File(dir, "mods"), name));
                                showDeleteMods();
                                say(ok ? "Done: " + name + " deleted." : "cannot delete " + name);
                            })));
        }
        focus(null);
    }

    private void deleteGame() {
        Ui.confirm(this, "Delete " + gameTitle + "?",
                "This removes the game files, its mods, its saves and its achievements from this phone."
                        + " To keep your saves, send them first with Transfer Save.", "Delete", () -> {
                    boolean ok = delete(new File(dir));
                    Intent result = new Intent();
                    result.putExtra(DELETED, ok ? gameTitle + " deleted" : "cannot delete all of " + gameTitle);
                    setResult(RESULT_OK, result);
                    finish();
                });
    }

    /* Borra una carpeta con todo lo que tiene (sin seguir enlaces) */
    private static boolean delete(File file) {
        File[] children = file.isDirectory() && !java.nio.file.Files.isSymbolicLink(file.toPath())
                ? file.listFiles() : null;
        if (children != null) for (File child : children) delete(child);
        return file.delete() || !file.exists();
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

    /* Volver: al menu, y desde el menu a la lista de juegos */
    @Override
    public void onBackPressed() {
        if (mode == Mode.HOME) super.onBackPressed();
        else showHome();
    }
}
