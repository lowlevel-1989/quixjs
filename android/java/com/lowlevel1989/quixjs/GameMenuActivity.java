/* GameMenuActivity: el menu de un juego, el launcher de quixjs con vistas de
   Android (el lenguaje de Ui.java). Las mismas opciones y en el mismo orden
   que el de Linux (runtime/launcher/launcher.c). Arriba el titulo y la
   firma; debajo, la pantalla de ahora:

     showHome           (aqui) Play; GAME: Achievements (si hay
                        [[achievements]]), Mods (si hay carpetas en mods/),
                        Options, Cheats (si hay [[cheat]] y no es hardcore), Transfer Save (si
                        hay [saves] files), Controls (si [controls] gamepad);
                        MANAGE: Delete mods, Delete game
     MenuAchievements   cuantos van, por tipo, y la lista con su descripcion
     MenuMods           cada carpeta de mods/, On u Off (y su prioridad), y
                        Delete mods: cada mod, para borrarlo
     MenuOptions        FPS, Show FPS, Profiler, Vibration, Save log y Save
                        profile (si estan: log.txt y quixjs-profile.txt, a
                        donde se elija) y Reset achievements
     MenuCheats         cada truco, On u Off
     MenuControls       que asignacion usa el mando; Map buttons abre la
                        pantalla Controls de quixjs (GameActivity con
                        --controls): el mismo codigo y el mismo controller.map
                        que en Linux

   Aqui estan lo que comparten: la pantalla (screen, section, info, add,
   focus, say) y el JNI. Borrar y Reset achievements piden confirmacion
   (Ui.confirm). No hay filas Back ni Exit: se vuelve con el gesto o el
   boton Atras de Android, o con b en el mando (Pad.java).

   Lo que se ve y lo que se cambia sale del C del launcher, por JNI
   (runtime/platform/android/menu.c): el toml del juego, los logros, los
   trucos y quixjs-settings.toml, que se guarda al cambiar algo, como en
   Linux. El juego lo lee al arrancar.

   Adaptado de tamatama (android/java/.../TransferActivity.java, MIT):
   https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/android/java/com/lowlevel1989/tamatama/TransferActivity.java */
package com.lowlevel1989.quixjs;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;

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

    /* runtime/platform/android/menu.c */
    private static native boolean open(String dir);
    private static native String[] header();
    private static native String[] menu();
    static native String[] achievements();
    static native void resetAchievements();
    static native boolean[] options();
    static native void toggleOption(int which);
    static native String[] mods();
    static native void toggleMod(String name);
    static native String[] cheats();
    static native void toggleCheat(int index);

    /* Que hace volver */
    enum Mode { HOME, ACHIEVEMENTS, MODS, OPTIONS, CHEATS, CONTROLS, DELETE_MODS }

    String dir, gameTitle;
    private String version;
    private TextView title, signature, notice;
    LinearLayout content;
    private ScrollView scroll;
    private Mode mode = Mode.HOME;
    String focusKey;   /* la fila del menu que recupera el foco al volver */

    private final MenuAchievements achievementsMenu = new MenuAchievements(this);
    private final MenuMods modsMenu = new MenuMods(this);
    private final MenuOptions optionsMenu = new MenuOptions(this);
    private final MenuCheats cheatsMenu = new MenuCheats(this);
    private final MenuControls controlsMenu = new MenuControls(this);

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
    void screen(Mode next, String head, String line) {
        mode = next;
        title.setText(head);
        signature.setText(line);
        content.removeAllViews();
        say("");
        scroll.scrollTo(0, 0);
    }

    void say(String message) {
        notice.setText(message);
        notice.setVisibility(message.isEmpty() ? View.GONE : View.VISIBLE);
    }

    void section(String name) {
        content.addView(Ui.section(this, name), Ui.wide());
    }

    TextView info(String text, int color) {
        TextView view = Ui.info(this, text, color);
        content.addView(view, Ui.wide());
        return view;
    }

    /* Una fila; con una raya si va debajo de otra */
    void add(View row) {
        int n = content.getChildCount();
        if (n > 0 && content.getChildAt(n - 1).isClickable()) content.addView(Ui.divider(this));
        content.addView(row, Ui.wide());
    }

    /* El foco: en la fila key si esta, si no en la primera. Despues de
       onResume: antes, la ventana aun no tiene las vistas */
    void focus(String key) {
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
            case ACHIEVEMENTS: achievementsMenu.show(); break;
            case MODS: modsMenu.show(null); break;
            case OPTIONS: optionsMenu.redraw(); break;
            case CHEATS: cheatsMenu.show(null); break;
            case CONTROLS: controlsMenu.show(); break;
            case DELETE_MODS: modsMenu.showDelete(); break;
            default: showHome();
        }
    }

    /* ---- la portada del juego ---- */

    void showHome() {
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
                    int total = list.length / MenuAchievements.FIELDS, hard = MenuAchievements.hardcore(list);
                    detail = MenuAchievements.unlocked(list) + " of " + total + " unlocked"
                            + (hard == total ? ", all hardcore" : hard > 0 ? ", " + hard + " hardcore" : "");
                    action = v -> achievementsMenu.show();
                    break;
                case "Mods":
                    detail = on + " of " + mods.length / 2 + " on";
                    action = v -> modsMenu.show(null);
                    break;
                case "Options":
                    detail = (opts[4] ? "30" : "60") + " fps, show fps " + (opts[0] ? "on" : "off") + ", profiler " + (opts[1] ? "on" : "off")
                            + (opts[3] ? ", vibration " + (opts[2] ? "on" : "off") : "");
                    action = v -> optionsMenu.show(null);
                    break;
                case "Cheats":
                    detail = MenuCheats.on(cheats()) + " of " + cheats().length / 3 + " on";
                    action = v -> cheatsMenu.show(null);
                    break;
                case "Transfer Save":
                    detail = "send or receive your saves";
                    action = v -> transfer();
                    break;
                case "Controls":
                    detail = new File(dir, "controller.map").exists() ? "your mapping" : "default mapping";
                    action = v -> controlsMenu.show();
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
            modsMenu.showDelete();
        }));
        add(Ui.danger(this, "Delete game", "its files, mods and saves", v -> deleteGame()));
        focus(focusKey);
    }

    /* El juego, o solo la pantalla Controls de quixjs */
    void play(boolean controls) {
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

    /* Exportar un archivo (MenuOptions) */
    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        optionsMenu.exported(request, result, data);
    }

    private void deleteGame() {
        Ui.confirm(this, "Delete " + gameTitle + "?",
                "This removes the game files, its mods, its saves and its achievements from this phone."
                        + " To keep your saves, send them first with Transfer Save.", "Delete", () -> {
                    boolean ok = Files.delete(new File(dir));
                    Intent result = new Intent();
                    result.putExtra(DELETED, ok ? gameTitle + " deleted" : "cannot delete all of " + gameTitle);
                    setResult(RESULT_OK, result);
                    finish();
                });
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
