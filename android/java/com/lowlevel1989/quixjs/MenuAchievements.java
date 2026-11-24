/* MenuAchievements: la pantalla Achievements del menu de un juego
   (GameMenuActivity). Cuantos van de cada tipo y la lista (solo se leen):
   los conseguidos en el color del texto, con su fecha; los que faltan en
   gris. Cada logro llega del JNI como 5 textos: titulo, descripcion, tipo,
   color (0xRRGGBB en decimal) y fecha ("" si falta). */
package com.lowlevel1989.quixjs;

import android.graphics.drawable.GradientDrawable;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

final class MenuAchievements {
    private final GameMenuActivity m;

    MenuAchievements(GameMenuActivity menu) {
        m = menu;
    }

    /* GameMenuActivity.achievements(): seis textos por logro (titulo,
       descripcion, tipo, color, fecha o "", "1" si fue en hardcore) */
    static final int FIELDS = 6;
    /* La estrella de un logro conseguido en hardcore (runtime/backend/star.h) */
    static final int STAR = 0xfff2c14e;

    /* Cuantos conseguidos en la lista de GameMenuActivity.achievements() */
    static int unlocked(String[] achievements) {
        int got = 0;
        for (int i = 4; i < achievements.length; i += FIELDS) if (!achievements[i].isEmpty()) got++;
        return got;
    }

    /* Cuantos en hardcore */
    static int hardcore(String[] achievements) {
        int got = 0;
        for (int i = 5; i < achievements.length; i += FIELDS) if (!achievements[i].isEmpty()) got++;
        return got;
    }

    /* text y, detras, la estrella dorada */
    static CharSequence starred(String text) {
        /* un espacio ancho (en space): la estrella, algo separada del nombre */
        SpannableString s = new SpannableString(text + "\u2002\u2605");
        s.setSpan(new ForegroundColorSpan(STAR), text.length() + 1, s.length(), 0);
        return s;
    }

    void show() {
        String[] list = GameMenuActivity.achievements();
        int count = list.length / FIELDS, hard = hardcore(list);
        m.screen(GameMenuActivity.Mode.ACHIEVEMENTS, "Achievements", m.gameTitle);
        m.info(unlocked(list) + " of " + count + " unlocked" + (hard > 0 ? ", " + hard + " hardcore" : ""), Ui.TEXT);
        List<String> colors = new ArrayList<>();
        for (int i = 0; i < count; i++) if (!colors.contains(list[FIELDS * i + 3])) colors.add(list[FIELDS * i + 3]);
        for (String color : colors) {
            int n = 0, done = 0;
            String type = "";
            for (int i = 0; i < count; i++) {
                if (!list[FIELDS * i + 3].equals(color)) continue;
                n++;
                if (!list[FIELDS * i + 4].isEmpty()) done++;
                if (type.isEmpty()) type = list[FIELDS * i + 2];
            }
            TextView legend = m.info((type.isEmpty() ? "other" : type) + "  " + done + "/" + n, Ui.MUTED);
            legend.setTextSize(13);
            badge(legend, (int) Long.parseLong(color));
        }
        m.section("list");
        for (int i = 0; i < count; i++) {
            boolean got = !list[FIELDS * i + 4].isEmpty();
            LinearLayout entry = new LinearLayout(m);
            entry.setOrientation(LinearLayout.VERTICAL);
            entry.setPadding(dp(4), dp(10), dp(4), dp(10));
            TextView name = Ui.text(m, list[FIELDS * i], 17, got ? Ui.TEXT : Ui.MUTED, false);
            /* conseguido en hardcore: la estrella dorada despues del nombre */
            if (!list[FIELDS * i + 5].isEmpty()) name.setText(starred(list[FIELDS * i]));
            badge(name, (int) Long.parseLong(list[FIELDS * i + 3]));
            entry.addView(name, Ui.wide());
            String more = list[FIELDS * i + 1];
            String when = got ? "unlocked " + list[FIELDS * i + 4] : "locked";
            TextView below = Ui.text(m, (more.isEmpty() ? "" : more + "\n") + when, 13, Ui.MUTED, false);
            below.setLineSpacing(0, 1.15f);
            below.setPadding(dp(20), dp(3), 0, 0);
            entry.addView(below, Ui.wide());
            if (i > 0) m.content.addView(Ui.divider(m));
            m.content.addView(entry, Ui.wide());
        }
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

    private int dp(float value) { return Ui.dp(m, value); }
}
