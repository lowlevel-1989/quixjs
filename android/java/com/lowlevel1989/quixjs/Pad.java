/* Pad: el mando en las pantallas de Android (MainActivity, GameMenuActivity,
   TransferActivity y sus dialogos), sin quitar nada al tactil:

     arriba / abajo / izquierda / derecha   mueven el foco (cruceta o stick
                                            izquierdo)
     a o start                              toca la fila con el foco
     b                                      vuelve, como Atras

   Los botones son los de la asignacion del juego: la linea de ese mando en
   controller.map (la que guarda Controls) o, si no hay, la que SDL da en
   Android a un mando que no conoce. La busca el C (launcher.c, Pad_load) con
   el GUID de SDL, calculado con lo mismo que SDLControllerManager.java lee del
   InputDevice; asi "a:b12" es el mismo boton en el menu y en el juego, y tras
   asignar en Controls el menu sigue la asignacion nueva al volver (use() en
   onResume vuelve a leerla).

   Lo que no es un mando (un teclado, adb shell input keyevent) va con los
   keycodes de Android: BUTTON_A y BUTTON_START tocan, BUTTON_B vuelve; las
   flechas mueven el foco como siempre.

   La mascara de botones, el orden de los ejes y su mascara son los de SDL
   (SDLControllerManager.java, SDLJoystickHandler_API16 y _API19, zlib),
   para que el GUID sea el mismo. */
package com.lowlevel1989.quixjs;

import android.app.Dialog;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ScrollView;

import org.libsdl.app.SDLControllerManager;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

final class Pad {
    static {
        System.loadLibrary("SDL2");
        System.loadLibrary("SDL2_ttf");
        System.loadLibrary("SDL2_image");
        System.loadLibrary("quixjs");
    }

    /* lo que devuelven key, button y axis (launcher.c) */
    private static final int NONE = 0, UP = 1, DOWN = 2, LEFT = 3, RIGHT = 4, ACCEPT = 5, BACK = 6;

    /* runtime/platform/android/pad.c */
    private static native boolean load(String desc, int vendor, int product, int buttons, int axes, int hats,
                                       String[] maps);
    private static native int key(int keycode);
    private static native int button(int button);
    private static native int axis(int axis, int sign);

    private static String[] maps = new String[0];
    private static int device = Integer.MIN_VALUE;
    private static List<InputDevice.MotionRange> axes = new ArrayList<>(), hats = new ArrayList<>();
    private static int[] axisSign = new int[0];
    private static int hatX, hatY;

    private Pad() {}

    /* Los controller.map donde buscar la asignacion; se leen en el siguiente
       evento (despues de Controls, la nueva) */
    static void use(String... files) {
        maps = files;
        device = Integer.MIN_VALUE;
    }

    /* ---- el mando que se usa ---- */

    private static void prepare(int id) {
        if (id == device) return;
        device = id;
        InputDevice d = InputDevice.getDevice(id);
        if (d == null) return;
        axes = new ArrayList<>();
        hats = new ArrayList<>();
        List<InputDevice.MotionRange> ranges = new ArrayList<>(d.getMotionRanges());
        Collections.sort(ranges, Pad::compareAxes);
        for (InputDevice.MotionRange range : ranges) {
            if ((range.getSource() & InputDevice.SOURCE_CLASS_JOYSTICK) == 0) continue;
            if (range.getAxis() == MotionEvent.AXIS_HAT_X || range.getAxis() == MotionEvent.AXIS_HAT_Y) hats.add(range);
            else axes.add(range);
        }
        axisSign = new int[axes.size()];
        /* en reposo: los centrados en 0, los gatillos (de 0 a 1) en -1, como
           los deja SDL al pasarlos a -1..1 */
        for (int i = 0; i < axisSign.length; i++) axisSign[i] = axes.get(i).getMin() < 0 ? 0 : -1;
        hatX = hatY = 0;
        String desc = d.getDescriptor();
        if (desc == null || desc.isEmpty()) desc = d.getName();
        load(desc, d.getVendorId(), d.getProductId(), buttonMask(d), axisMask(axes), hats.size() / 2, maps);
    }

    /* SDLJoystickHandler_API16.RangeComparator */
    private static int compareAxes(InputDevice.MotionRange a, InputDevice.MotionRange b) {
        return sortKey(a.getAxis()) - sortKey(b.getAxis());
    }

    private static int sortKey(int axis) {
        if (axis == MotionEvent.AXIS_GAS) axis = MotionEvent.AXIS_BRAKE;
        else if (axis == MotionEvent.AXIS_BRAKE) axis = MotionEvent.AXIS_GAS;
        if (axis == MotionEvent.AXIS_Z) axis = MotionEvent.AXIS_RZ - 1;
        else if (axis > MotionEvent.AXIS_Z && axis < MotionEvent.AXIS_RZ) --axis;
        return axis;
    }

    /* SDLJoystickHandler_API19.getAxisMask */
    private static int axisMask(List<InputDevice.MotionRange> ranges) {
        int mask = 0;
        if (ranges.size() >= 2) mask |= 0x0003;
        if (ranges.size() >= 4) mask |= 0x000c;
        if (ranges.size() >= 6) mask |= 0x0030;
        boolean z = false, pastZ = false;
        for (InputDevice.MotionRange range : ranges) {
            int a = range.getAxis();
            if (a == MotionEvent.AXIS_Z) z = true;
            else if (a > MotionEvent.AXIS_Z && a < MotionEvent.AXIS_RZ) pastZ = true;
        }
        if (z && pastZ) mask |= 0x8000;
        return mask;
    }

    /* SDLJoystickHandler_API19.getButtonMask */
    private static final int[] KEYS = {
        KeyEvent.KEYCODE_BUTTON_A, KeyEvent.KEYCODE_BUTTON_B, KeyEvent.KEYCODE_BUTTON_X, KeyEvent.KEYCODE_BUTTON_Y,
        KeyEvent.KEYCODE_BACK, KeyEvent.KEYCODE_MENU, KeyEvent.KEYCODE_BUTTON_MODE, KeyEvent.KEYCODE_BUTTON_START,
        KeyEvent.KEYCODE_BUTTON_THUMBL, KeyEvent.KEYCODE_BUTTON_THUMBR, KeyEvent.KEYCODE_BUTTON_L1,
        KeyEvent.KEYCODE_BUTTON_R1, KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_DPAD_LEFT,
        KeyEvent.KEYCODE_DPAD_RIGHT, KeyEvent.KEYCODE_BUTTON_SELECT, KeyEvent.KEYCODE_DPAD_CENTER,
        KeyEvent.KEYCODE_BUTTON_L2, KeyEvent.KEYCODE_BUTTON_R2, KeyEvent.KEYCODE_BUTTON_C, KeyEvent.KEYCODE_BUTTON_Z,
        KeyEvent.KEYCODE_BUTTON_1, KeyEvent.KEYCODE_BUTTON_2, KeyEvent.KEYCODE_BUTTON_3, KeyEvent.KEYCODE_BUTTON_4,
        KeyEvent.KEYCODE_BUTTON_5, KeyEvent.KEYCODE_BUTTON_6, KeyEvent.KEYCODE_BUTTON_7, KeyEvent.KEYCODE_BUTTON_8,
        KeyEvent.KEYCODE_BUTTON_9, KeyEvent.KEYCODE_BUTTON_10, KeyEvent.KEYCODE_BUTTON_11,
        KeyEvent.KEYCODE_BUTTON_12, KeyEvent.KEYCODE_BUTTON_13, KeyEvent.KEYCODE_BUTTON_14,
        KeyEvent.KEYCODE_BUTTON_15, KeyEvent.KEYCODE_BUTTON_16,
    };
    private static final int[] MASKS = {
        1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 6, 1 << 5, 1 << 6, 1 << 7, 1 << 8, 1 << 9, 1 << 10, 1 << 11,
        1 << 12, 1 << 13, 1 << 14, 1 << 4, 1, 1 << 15, 1 << 16, 1 << 17, 1 << 18, 1 << 20, 1 << 21, 1 << 22,
        1 << 23, 1 << 24, 1 << 25, 1 << 26, 1 << 27, 1 << 28, 1 << 29, 1 << 30, 1 << 31,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    };

    private static int buttonMask(InputDevice d) {
        boolean[] has = d.hasKeys(KEYS);
        int mask = 0;
        for (int i = 0; i < KEYS.length; i++) if (has[i]) mask |= MASKS[i];
        return mask;
    }

    /* ---- los eventos ---- */

    /* Una tecla: true si era del mando (y ya hizo lo suyo) */
    static boolean key(Window window, KeyEvent event, Runnable back) {
        int code = event.getKeyCode(), id = event.getDeviceId(), action;
        if (SDLControllerManager.isDeviceSDLJoystick(id)) {
            prepare(id);
            action = key(code);
            if (action < 0) return false;
        } else if (code == KeyEvent.KEYCODE_BUTTON_A || code == KeyEvent.KEYCODE_BUTTON_START) {
            action = ACCEPT;
        } else if (code == KeyEvent.KEYCODE_BUTTON_B) {
            action = BACK;
        } else {
            return false;
        }
        /* se hace al pulsar; mantener solo repite el movimiento */
        if (event.getAction() == KeyEvent.ACTION_DOWN && (event.getRepeatCount() == 0 || action <= RIGHT))
            act(window, action, back);
        return true;
    }

    /* Los sticks y la cruceta como hat: true si era del mando */
    static boolean motion(Window window, MotionEvent event, Runnable back) {
        if ((event.getSource() & InputDevice.SOURCE_CLASS_JOYSTICK) == 0
                || event.getActionMasked() != MotionEvent.ACTION_MOVE
                || !SDLControllerManager.isDeviceSDLJoystick(event.getDeviceId())) return false;
        prepare(event.getDeviceId());
        for (int i = 0; i < axes.size() && i < axisSign.length; i++) {
            InputDevice.MotionRange range = axes.get(i);
            float value = (event.getAxisValue(range.getAxis()) - range.getMin()) / range.getRange() * 2.0f - 1.0f;
            int was = axisSign[i];
            int sign = value > 0.5f ? 1 : value < -0.5f ? -1 : Math.abs(value) < 0.3f ? 0 : was;
            if (sign == was) continue;
            axisSign[i] = sign;
            if (sign != 0) act(window, axis(i, sign), back);
        }
        if (hats.size() >= 2) {
            int x = Math.round(event.getAxisValue(hats.get(0).getAxis()));
            int y = Math.round(event.getAxisValue(hats.get(1).getAxis()));
            /* como SDL: la cruceta son los botones 11 a 14 */
            if (y != hatY && y != 0) act(window, button(y < 0 ? 11 : 12), back);
            if (x != hatX && x != 0) act(window, button(x < 0 ? 13 : 14), back);
            hatX = x;
            hatY = y;
        }
        return true;
    }

    private static void act(Window window, int action, Runnable back) {
        if (action == NONE) return;
        if (action == BACK) {
            back.run();
            return;
        }
        View root = window.getDecorView();
        View focused = window.getCurrentFocus();
        /* una pantalla que solo se lee (Achievements): arriba y abajo la
           desplazan */
        if (focusables(root).isEmpty()) {
            if (action == UP || action == DOWN) scroll(root, action == UP ? -1 : 1);
            return;
        }
        /* sin foco a la vista (despues de tocar la pantalla): primero se ensena */
        if (focused == null || root.isInTouchMode()) {
            reveal(root, focused);
            return;
        }
        if (action == ACCEPT) {
            focused.performClick();
            return;
        }
        int direction = action == UP ? View.FOCUS_UP : action == DOWN ? View.FOCUS_DOWN
                : action == LEFT ? View.FOCUS_LEFT : View.FOCUS_RIGHT;
        View next = focused.focusSearch(direction);
        if (next != null) next.requestFocus(direction);
        /* sin mas filas: lo que se lee por encima o por debajo */
        else if (action == UP || action == DOWN) scroll(root, action == UP ? -1 : 1);
    }

    /* El primer ScrollView de la pantalla, un tercio de su alto */
    private static void scroll(View view, int sign) {
        if (view instanceof ScrollView) {
            ((ScrollView) view).smoothScrollBy(0, sign * view.getHeight() / 3);
            return;
        }
        if (!(view instanceof ViewGroup)) return;
        ViewGroup group = (ViewGroup) view;
        for (int i = 0; i < group.getChildCount(); i++) scroll(group.getChildAt(i), sign);
    }

    /* El foco que habia, o la primera fila */
    private static void reveal(View root, View focused) {
        if (focused != null && focused.requestFocusFromTouch()) return;
        for (View view : focusables(root)) if (view.requestFocusFromTouch()) return;
    }

    /* Las vistas que toman el foco, tambien en modo tactil (getFocusables
       solo da entonces las que lo toman al tocar) */
    private static ArrayList<View> focusables(View root) {
        ArrayList<View> all = new ArrayList<>();
        root.addFocusables(all, View.FOCUS_DOWN, View.FOCUSABLES_ALL);
        return all;
    }

    /* Un dialogo tambien con el mando: b lo cierra como Atras */
    static void dialog(Dialog dialog) {
        /* los botones, con el fondo del foco de las filas */
        dialog.setOnShowListener(d -> {
            if (!(dialog instanceof android.app.AlertDialog)) return;
            for (int which : new int[] {android.app.AlertDialog.BUTTON_POSITIVE,
                    android.app.AlertDialog.BUTTON_NEGATIVE, android.app.AlertDialog.BUTTON_NEUTRAL}) {
                View button = ((android.app.AlertDialog) dialog).getButton(which);
                if (button != null) Ui.focusBackground(dialog.getContext(), button);
            }
        });
        Window window = dialog.getWindow();
        Window.Callback inner = window.getCallback();
        window.setCallback((Window.Callback) Proxy.newProxyInstance(Window.Callback.class.getClassLoader(),
                new Class<?>[] {Window.Callback.class}, (proxy, method, args) -> {
                    String name = method.getName();
                    if (name.equals("dispatchKeyEvent") && key(window, (KeyEvent) args[0], dialog::onBackPressed))
                        return true;
                    if (name.equals("dispatchGenericMotionEvent")
                            && motion(window, (MotionEvent) args[0], dialog::onBackPressed)) return true;
                    try {
                        return method.invoke(inner, args);
                    } catch (InvocationTargetException e) {
                        throw e.getCause();
                    }
                }));
    }
}
