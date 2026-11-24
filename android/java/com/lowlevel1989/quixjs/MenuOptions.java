/* MenuOptions: la pantalla Options del menu de un juego (GameMenuActivity):
   FPS, Show FPS, Profiler, Vibration (si el juego vibra), Save log y Save
   profile (si estan) y Reset achievements. Cada cambio se guarda enseguida
   en quixjs-settings.toml (por JNI).

   log.txt y quixjs-profile.txt quedan dentro de la app: Save log y Save
   profile los guardan donde se elija (ACTION_CREATE_DOCUMENT, sin permisos
   ni FileProvider), para mandarlos. */
package com.lowlevel1989.quixjs;

import android.app.Activity;
import android.content.Intent;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

final class MenuOptions {
    /* el codigo de startActivityForResult del selector */
    private static final int EXPORT = 1;

    private final GameMenuActivity m;
    private File exporting;
    private String exportKey;   /* la fila que recupera el foco al volver del selector */

    MenuOptions(GameMenuActivity menu) {
        m = menu;
    }

    /* Al volver a la app (del selector, por ejemplo): el foco donde estaba */
    void redraw() {
        show(exportKey);
        exportKey = null;
    }

    /* key: la fila que se queda con el foco */
    void show(String key) {
        boolean[] on = GameMenuActivity.options();
        m.screen(GameMenuActivity.Mode.OPTIONS, "Options", m.gameTitle);
        m.section("game");
        m.add(Ui.link(m, "FPS: " + (on[4] ? "30" : "60"), "30 saves battery", v -> {
            GameMenuActivity.toggleOption(3);
            show("FPS: " + (on[4] ? "60" : "30"));   /* el foco, en la misma fila */
        }));
        m.add(Ui.toggle(m, "Show FPS", "the frame counter, top right", on[0], v -> {
            GameMenuActivity.toggleOption(0);
            show("Show FPS");
        }));
        m.add(Ui.toggle(m, "Profiler", "writes quixjs-profile.txt", on[1], v -> {
            GameMenuActivity.toggleOption(1);
            show("Profiler");
        }));
        /* solo si el juego vibra ([controls] vibration en su port.toml) */
        if (on[3]) m.add(Ui.toggle(m, "Vibration", "the controller, or the phone if it cannot", on[2], v -> {
            GameMenuActivity.toggleOption(2);
            show("Vibration");
        }));
        File log = new File(m.dir, "log.txt"), profile = new File(m.dir, "quixjs-profile.txt");
        if (log.exists() || profile.exists()) m.section("files");
        if (log.exists()) m.add(Ui.link(m, "Save log", "log.txt of the last game, " + Files.size(log), v -> {
            exportKey = "Save log";
            export(log);
        }));
        if (profile.exists()) m.add(Ui.link(m, "Save profile", "quixjs-profile.txt, " + Files.size(profile), v -> {
            exportKey = "Save profile";
            export(profile);
        }));
        String[] list = GameMenuActivity.achievements();
        if (list.length > 0) {
            final int got = MenuAchievements.unlocked(list);
            m.section("achievements");
            /* Hardcore: sin trucos; lo que se consigue asi lleva la estrella */
            m.add(Ui.link(m, "Achievements: " + (on[5] ? "Hardcore" : "Normal"),
                    "hardcore: no cheats, and a gold star on what you unlock", v -> {
                GameMenuActivity.toggleOption(4);
                show("Achievements: " + (on[5] ? "Normal" : "Hardcore"));
            }));
            m.add(Ui.danger(m, "Reset achievements", got + " unlocked", v ->
                    Ui.confirm(m, "Reset achievements?",
                            "Forget " + got + " unlocked achievements. This cannot be undone.", "Reset", () -> {
                                GameMenuActivity.resetAchievements();
                                show("Reset achievements");
                                m.say("Done: no achievements unlocked.");
                            })));
        }
        m.focus(key);
    }

    /* ---- guardar un archivo donde se elija ---- */

    private void export(File file) {
        exporting = file;
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("text/plain");
        intent.putExtra(Intent.EXTRA_TITLE, new File(m.dir).getName() + "-" + file.getName());
        try {
            m.startActivityForResult(intent, EXPORT);
        } catch (RuntimeException e) {
            m.say("cannot open the file chooser");
        }
    }

    /* onActivityResult de GameMenuActivity: copia el archivo a lo elegido */
    void exported(int request, int result, Intent data) {
        if (request != EXPORT || exporting == null) return;
        File file = exporting;
        exporting = null;
        if (result != Activity.RESULT_OK || data == null || data.getData() == null) return;
        String message;
        try (InputStream in = new FileInputStream(file);
             OutputStream out = m.getContentResolver().openOutputStream(data.getData(), "wt")) {
            if (out == null) throw new IOException();
            Files.copy(in, out);
            message = "Done: " + file.getName() + " saved.";
        } catch (IOException | RuntimeException e) {
            message = "cannot save " + file.getName();
        }
        /* onResume redibuja despues: el aviso, cuando ya se ve */
        final String done = message;
        m.content.post(() -> m.say(done));
    }
}
