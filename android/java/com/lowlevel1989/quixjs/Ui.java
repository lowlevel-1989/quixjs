/* Ui: lo que comparten las pantallas de Android (MainActivity,
   GameMenuActivity y TransferActivity): los colores del launcher de quixjs
   (runtime/launcher/launcher.c) y un lenguaje plano y sencillo, el mismo en
   todas:

     header     el titulo, grande y centrado, y debajo una linea pequena
                (la firma: quixjs y su version, o el juego)
     section    una etiqueta corta en mayusculas, gris, que separa grupos
     info       texto que solo se lee: a la izquierda, sin nada a la derecha
     row        algo que se toca: a la izquierda su nombre (y debajo, en
                gris, una linea mas), a la derecha "›"; todas de la misma
                altura, separadas por una raya fina
     toggle     una fila que se enciende y se apaga: a la derecha On (en el
                color de lo elegido) u Off (en gris)
     danger     una fila que borra: en rojo, y siempre con confirmacion
     big        la letra grande de lo que hay que comparar (la firma de
                Transfer Save)

   Una sola familia de letra (monoespaciada, como tamatama), en dos pesos:
   normal y negrita para titulos y acciones principales. Al tocar una fila, un
   fondo suave; con el foco del mando (Pad.java), el azul del elegido en el
   launcher; sin bordes ni tarjetas.

   Adaptado de tamatama (android/java/.../Ui.java, MIT):
   https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/android/java/com/lowlevel1989/tamatama/Ui.java */
package com.lowlevel1989.quixjs;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.StateListDrawable;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

final class Ui {
    /* los del launcher: background, text, accent, muted, notice, highlight;
       y el rojo de borrar y la raya entre filas */
    static final int BACKGROUND = 0xff101014, TEXT = 0xffe8e8e8, ACCENT = 0xff2c4a7a, MUTED = 0xff8a8a96,
            NOTICE = 0xfff3a060, HIGHLIGHT = 0xff7ab8ff, DANGER = 0xffff6b6b, LINE = 0xff24242c,
            PRESSED = 0xff1c2433;

    private Ui() {}

    static int dp(Context activity, float value) {
        return Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value,
                activity.getResources().getDisplayMetrics()));
    }

    static TextView text(Activity activity, String value, float sp, int color, boolean bold) {
        TextView view = new TextView(activity);
        view.setText(value);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        view.setTextColor(color);
        view.setTypeface(Typeface.MONOSPACE, bold ? Typeface.BOLD : Typeface.NORMAL);
        return view;
    }

    static LinearLayout.LayoutParams wide() {
        return new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
    }

    /* La pantalla: el fondo, los margenes y la cabecera. Devuelve la columna
       donde va todo lo demas; title y line se cambian despues con setText */
    static LinearLayout screen(Activity activity, TextView title, TextView line) {
        LinearLayout screen = new LinearLayout(activity);
        screen.setOrientation(LinearLayout.VERTICAL);
        screen.setBackgroundColor(BACKGROUND);
        screen.setPadding(dp(activity, 20), dp(activity, 40), dp(activity, 20), dp(activity, 20));
        title.setGravity(Gravity.CENTER);
        title.setLetterSpacing(0.06f);
        screen.addView(title, wide());
        line.setGravity(Gravity.CENTER);
        line.setLetterSpacing(0.05f);
        line.setPadding(0, dp(activity, 4), 0, dp(activity, 16));
        screen.addView(line, wide());
        return screen;
    }

    static TextView title(Activity activity, String value, float sp) {
        return text(activity, value, sp, 0xffffffff, true);
    }

    static TextView signature(Activity activity, String value) {
        return text(activity, value, 13, MUTED, false);
    }

    /* Una etiqueta de grupo: corta, en mayusculas, gris */
    static TextView section(Activity activity, String value) {
        TextView view = text(activity, value.toUpperCase(), 12, MUTED, true);
        view.setLetterSpacing(0.15f);
        view.setPadding(dp(activity, 4), dp(activity, 24), dp(activity, 4), dp(activity, 8));
        return view;
    }

    /* Texto que solo se lee */
    static TextView info(Activity activity, String value, int color) {
        TextView view = text(activity, value, 15, color, false);
        view.setLineSpacing(0, 1.2f);
        view.setPadding(dp(activity, 4), dp(activity, 4), dp(activity, 4), dp(activity, 4));
        return view;
    }

    /* La letra grande de lo que se compara a simple vista (una firma) */
    static TextView big(Activity activity, String value) {
        TextView view = text(activity, value, 44, HIGHLIGHT, true);
        view.setGravity(Gravity.CENTER);
        view.setLetterSpacing(0.15f);
        view.setPadding(0, dp(activity, 8), 0, dp(activity, 8));
        return view;
    }

    /* Sin el gris con que Android marca el foco: ya lo marca el fondo */
    static void noFocusHighlight(View view) {
        if (android.os.Build.VERSION.SDK_INT >= 26) view.setDefaultFocusHighlightEnabled(false);
    }

    /* Al tocar, un fondo suave; con el foco del mando, el del elegido en el
       launcher (ACCENT), que se ve de lejos */
    private static Drawable rowBackground(Context activity) {
        GradientDrawable pressed = new GradientDrawable();
        pressed.setColor(PRESSED);
        pressed.setCornerRadius(dp(activity, 8));
        GradientDrawable focused = new GradientDrawable();
        focused.setColor(ACCENT);
        focused.setCornerRadius(dp(activity, 8));
        StateListDrawable states = new StateListDrawable();
        states.addState(new int[] {android.R.attr.state_pressed}, pressed);
        states.addState(new int[] {android.R.attr.state_focused}, focused);
        states.addState(new int[] {}, new ColorDrawable(0));
        return states;
    }

    /* El fondo de las filas en otra vista que se toca (un enlace) */
    static void focusBackground(Context activity, View view) {
        view.setBackground(rowBackground(activity));
        noFocusHighlight(view);
    }

    /* Una fila que se toca: name (color, negrita si bold), detail debajo en
       gris (o null) y a la derecha side (en sideColor). Devuelve la fila; su
       etiqueta (tag) es name, para encontrarla al redibujar */
    static LinearLayout row(Activity activity, String name, String detail, int color, boolean bold, String side,
                            int sideColor, View.OnClickListener action) {
        LinearLayout row = new LinearLayout(activity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setMinimumHeight(dp(activity, 56));
        row.setPadding(dp(activity, 4), dp(activity, 8), dp(activity, 8), dp(activity, 8));
        row.setBackground(rowBackground(activity));
        noFocusHighlight(row);
        row.setFocusable(true);
        row.setClickable(true);
        row.setOnClickListener(action);
        row.setTag(name);
        LinearLayout texts = new LinearLayout(activity);
        texts.setOrientation(LinearLayout.VERTICAL);
        texts.addView(text(activity, name, 17, color, bold), wide());
        if (detail != null && !detail.isEmpty()) {
            TextView below = text(activity, detail, 13, MUTED, false);
            below.setPadding(0, dp(activity, 2), 0, 0);
            texts.addView(below, wide());
        }
        row.addView(texts, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        if (side != null) {
            TextView right = text(activity, side, side.length() == 1 ? 22 : 15, sideColor, side.length() > 1);
            right.setPadding(dp(activity, 12), 0, 0, 0);
            row.addView(right);
        }
        return row;
    }

    /* Una fila que lleva a otra pantalla o hace algo: "›" a la derecha */
    static LinearLayout link(Activity activity, String name, String detail, View.OnClickListener action) {
        return row(activity, name, detail, TEXT, false, "›", MUTED, action);
    }

    /* La accion principal de una pantalla (Play, Import game) */
    static LinearLayout primary(Activity activity, String name, String detail, View.OnClickListener action) {
        return row(activity, name, detail, HIGHLIGHT, true, "›", HIGHLIGHT, action);
    }

    /* Una fila On / Off */
    static LinearLayout toggle(Activity activity, String name, String detail, boolean on,
                               View.OnClickListener action) {
        return row(activity, name, detail, TEXT, false, on ? "On" : "Off", on ? HIGHLIGHT : MUTED, action);
    }

    /* Una fila que borra: en rojo */
    static LinearLayout danger(Activity activity, String name, String detail, View.OnClickListener action) {
        return row(activity, name, detail, DANGER, false, null, 0, action);
    }

    /* Una raya fina entre filas */
    static View divider(Activity activity) {
        View view = new View(activity);
        view.setBackgroundColor(LINE);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 1);
        params.setMargins(dp(activity, 4), 0, dp(activity, 4), 0);
        view.setLayoutParams(params);
        return view;
    }

    /* Confirmar algo que no se deshace: el boton que lo hace, en rojo */
    static void confirm(Activity activity, String title, String message, String yes, Runnable then) {
        AlertDialog dialog = new AlertDialog.Builder(activity, android.R.style.Theme_Material_Dialog_Alert)
                .setTitle(title)
                .setMessage(message)
                .setNegativeButton("Cancel", null)
                .setPositiveButton(yes, (d, w) -> then.run())
                .create();
        Pad.dialog(dialog);
        dialog.show();
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setTextColor(DANGER);
        View cancel = dialog.getButton(AlertDialog.BUTTON_NEGATIVE);
        ((TextView) cancel).setTextColor(TEXT);
        /* el texto es corto: no se lleva el foco */
        View text = dialog.findViewById(android.R.id.message);
        if (text != null && text.getParent() instanceof View) ((View) text.getParent()).setFocusable(false);
        /* con el mando, el foco empieza en Cancel: borrar pide moverse */
        cancel.post(cancel::requestFocus);
    }

    static void darkBars(Activity activity) {
        activity.getWindow().setStatusBarColor(BACKGROUND);
        activity.getWindow().setNavigationBarColor(BACKGROUND);
    }
}
