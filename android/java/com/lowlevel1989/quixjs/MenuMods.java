/* MenuMods: las pantallas Mods y Delete mods del menu de un juego
   (GameMenuActivity). Mods: cada carpeta de mods/, On u Off, los activos
   con su prioridad (si dos traen el mismo archivo, gana el de numero mas
   alto). Delete mods: cada mod, para borrarlo, con confirmacion. Cada mod
   llega del JNI como 2 textos: nombre y prioridad ("" si esta apagado). */
package com.lowlevel1989.quixjs;

import java.io.File;

final class MenuMods {
    private final GameMenuActivity m;

    MenuMods(GameMenuActivity menu) {
        m = menu;
    }

    /* key: la fila que se queda con el foco */
    void show(String key) {
        String[] list = GameMenuActivity.mods();
        m.screen(GameMenuActivity.Mode.MODS, "Mods", m.gameTitle);
        m.info("If two mods change the same file, the one with the higher priority wins."
                + " Changes apply on Play.", Ui.MUTED);
        m.section("mods");
        for (int i = 0; i < list.length; i += 2) {
            final String name = list[i];
            boolean on = !list[i + 1].isEmpty();
            m.add(Ui.toggle(m, name, on ? "priority " + list[i + 1] : null, on, v -> {
                GameMenuActivity.toggleMod(name);
                show(name);
            }));
        }
        m.focus(key);
    }

    void showDelete() {
        String[] list = GameMenuActivity.mods();
        if (list.length == 0) {
            m.showHome();
            return;
        }
        m.screen(GameMenuActivity.Mode.DELETE_MODS, "Delete mods", m.gameTitle);
        m.info("To use a deleted mod again, import it again.", Ui.MUTED);
        m.section("mods");
        for (int i = 0; i < list.length; i += 2) {
            final String name = list[i];
            final boolean on = !list[i + 1].isEmpty();
            m.add(Ui.danger(m, name, on ? "on" : "off", v ->
                    Ui.confirm(m, "Delete " + name + "?", "Its files are removed from this phone.", "Delete",
                            () -> {
                                /* si estaba activo, primero fuera de quixjs-settings.toml */
                                if (on) GameMenuActivity.toggleMod(name);
                                boolean ok = Files.delete(new File(new File(m.dir, "mods"), name));
                                showDelete();
                                m.say(ok ? "Done: " + name + " deleted." : "cannot delete " + name);
                            })));
        }
        m.focus(null);
    }
}
