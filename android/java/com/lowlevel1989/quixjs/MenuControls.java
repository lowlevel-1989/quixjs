/* MenuControls: la pantalla Controls del menu de un juego
   (GameMenuActivity): la asignacion del mando de este juego. La pantalla
   que asigna los botones es la de quixjs (GameActivity con --controls), con
   el mismo codigo y el mismo controller.map que en Linux. */
package com.lowlevel1989.quixjs;

import java.io.File;

final class MenuControls {
    private final GameMenuActivity m;

    MenuControls(GameMenuActivity menu) {
        m = menu;
    }

    void show() {
        boolean mine = new File(m.dir, "controller.map").exists();
        m.screen(GameMenuActivity.Mode.CONTROLS, "Controls", m.gameTitle);
        m.info(mine ? "Your gamepad uses your mapping (controller.map)."
                : "Your gamepad uses its default mapping.", Ui.TEXT);
        m.info("Map buttons: connect the gamepad, then press each button it asks for."
                + " Start skips what your gamepad does not have; Back cancels.", Ui.MUTED);
        m.section("gamepad");
        m.add(Ui.link(m, "Map buttons", "the quixjs controls screen", v -> m.play(true)));
        if (mine) m.add(Ui.danger(m, "Use the default mapping", "forget controller.map", v ->
                Ui.confirm(m, "Use the default mapping?", "Your mapping of this game is deleted.", "Delete",
                        () -> {
                            new File(m.dir, "controller.map").delete();
                            show();
                            m.say("Done: the default mapping.");
                        })));
        m.focus(null);
    }
}
