/* GameActivity: el juego. Es la actividad de SDL: carga SDL2, SDL2_ttf,
   SDL2_image y quixjs, y SDL llama a SDL_main (runtime/platform/android) con
   la carpeta del juego ({files}/games/<id>, con su port.toml) y la de la
   app. quixjs arranca derecho en el juego, sin su launcher: el menu es
   GameMenuActivity, que abre esta con Play. Se juega con mando.

   Va en su propio proceso (":game" en el manifiesto): al terminar, el
   proceso se cierra y queda GameMenuActivity. Atras (el gesto o el boton
   del telefono, no el mando) tambien sale: en el juego, pulsado dos veces
   seguidas, para no perder la partida sin querer; en Controls, una. SDL se
   queda esa tecla (KEYCODE_BACK llega al juego como AC_BACK): se mira antes.

   vibrate, para platform_vibrate (runtime/platform/android): vibra el
   telefono como vibracion de un juego (USAGE_GAME), cuando no vibra ningun
   mando. La de SDL no dice para que es, Android la toma por respuesta al
   tacto y, si esa esta apagada en los ajustes, no vibra.

   Adaptado de tamatama (android/java/.../ArenaActivity.java, MIT):
   https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/android/java/com/lowlevel1989/tamatama/ArenaActivity.java */
package com.lowlevel1989.quixjs;

import android.content.Context;
import android.media.AudioAttributes;
import android.os.Build;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.view.KeyEvent;
import android.widget.Toast;

import org.libsdl.app.SDL;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLControllerManager;

public class GameActivity extends SDLActivity {
    static final String GAME = "game";
    /* true: solo Controls (quixjs --controls), para asignar el mando */
    static final String CONTROLS = "controls";

    @Override
    protected String[] getLibraries() {
        return new String[] {"SDL2", "SDL2_ttf", "SDL2_image", "quixjs"};
    }

    @Override
    protected String[] getArguments() {
        String game = getIntent().getStringExtra(GAME);
        String files = getFilesDir().getAbsolutePath();
        if (game == null) return new String[0];
        if (getIntent().getBooleanExtra(CONTROLS, false)) return new String[] {game, files, "controls"};
        return new String[] {game, files};
    }

    /* Vibra ms milisegundos con fuerza amplitude (1 a 255; si el telefono no
       la puede cambiar, la suya): true si tiene vibrador. 0 ms para */
    public static boolean vibrate(int ms, int amplitude) {
        Context context = SDL.getContext();
        Vibrator vibrator = context != null ? (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE) : null;
        if (vibrator == null || !vibrator.hasVibrator()) return false;
        if (ms <= 0) {
            vibrator.cancel();
            return true;
        }
        AudioAttributes game = new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).build();
        if (Build.VERSION.SDK_INT >= 26)
            vibrator.vibrate(VibrationEffect.createOneShot(ms, vibrator.hasAmplitudeControl()
                    ? Math.max(1, Math.min(255, amplitude)) : VibrationEffect.DEFAULT_AMPLITUDE), game);
        else
            vibrator.vibrate(ms, game);
        return true;
    }

    private long lastBack;

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK
                && !SDLControllerManager.isDeviceSDLJoystick(event.getDeviceId())) {
            if (event.getAction() != KeyEvent.ACTION_UP) return true;
            long now = android.os.SystemClock.uptimeMillis();
            if (getIntent().getBooleanExtra(CONTROLS, false) || now - lastBack < 3000) {
                finish();
            } else {
                lastBack = now;
                Toast.makeText(this, "Press back again to quit the game", Toast.LENGTH_SHORT).show();
            }
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        /* quixjs no se descarga: el proceso entero fuera */
        if (isFinishing()) System.exit(0);
    }
}
