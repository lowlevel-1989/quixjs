/* MenuCheats: la pantalla Cheats del menu de un juego (GameMenuActivity):
   cada truco, On u Off. Una partida jugada con uno activo pierde los
   logros. Cada truco llega del JNI como 3 textos: nombre, descripcion y
   "1" o "0". */
package com.lowlevel1989.quixjs;

final class MenuCheats {
    private final GameMenuActivity m;

    MenuCheats(GameMenuActivity menu) {
        m = menu;
    }

    /* Cuantos activos en la lista de GameMenuActivity.cheats() */
    static int on(String[] cheats) {
        int on = 0;
        for (int i = 2; i < cheats.length; i += 3) if ("1".equals(cheats[i])) on++;
        return on;
    }

    /* key: la fila que se queda con el foco */
    void show(String key) {
        String[] list = GameMenuActivity.cheats();
        m.screen(GameMenuActivity.Mode.CHEATS, "Cheats", m.gameTitle);
        m.info("A playthrough that uses a cheat can't unlock achievements. Other playthroughs still can.",
                Ui.NOTICE);
        m.section("cheats");
        for (int i = 0; i < list.length; i += 3) {
            final int index = i / 3;
            final String name = list[i];
            m.add(Ui.toggle(m, name, list[i + 1], "1".equals(list[i + 2]), v -> {
                GameMenuActivity.toggleCheat(index);
                show(name);
            }));
        }
        m.focus(key);
    }
}
