/* MainActivity: la pantalla de inicio de la app, con vistas de Android y los
   colores del launcher de quixjs: filas grandes, scroll y foco del mando.

     quixjs
     una fila por juego       su menu (GameMenuActivity, con el extra
                              "game": la carpeta del juego)
     Import game              el selector de archivos: un .zip con un port
     Licenses

   La app no trae juegos ni ports: cada juego se importa desde un .zip con
   la carpeta completa de su port, como queda en una portatil con
   PortMaster (port.toml, la rom del juego, mods/, partidas...). Asi
   la misma app juega cualquier port de quixjs. Abrir un .zip con la app,
   desde un gestor de archivos, importa igual.

   Importar: lo elegido se copia en un thread aparte a un archivo temporal
   (ZipFile necesita un archivo) y se importa (ver importFile):
     - un port: la carpeta del .zip con port.toml (la de ruta mas corta), a
       {files}/games/<nombre de la carpeta>/, sin lo que Android no usa
       (los binarios de PortMaster, libs.*, .so, .sh y licenses/). Sobre un
       juego ya importado no se borra nada: lo que trae el .zip pisa a lo
       que habia y el resto (las partidas, quixjs-settings.toml) se queda
     - un mod suelto: un .zip sin port.toml pero con las carpetas de un
       juego (js/, img/, data/...), a mods/<nombre del .zip>/ de un juego
       importado (si hay varios, se elige cual)

   Adaptado de tamatama (android/java/.../MainActivity.java, MIT):
   https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/android/java/com/lowlevel1989/tamatama/MainActivity.java */
package com.lowlevel1989.quixjs;

import android.app.Activity;
import android.Manifest;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.net.Uri;
import android.os.Bundle;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class MainActivity extends Activity {
    private static final int PICK = 1, MENU = 2;

    private TextView notice;
    private LinearLayout games;
    private View importRow;
    private boolean busy;
    private String pending;   /* el aviso de GameMenuActivity (un juego borrado) */

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        // las capturas (Select + R1) van a la galeria: en Android 9 y antes
        // eso pide este permiso (desde Android 10, MediaStore no lo necesita)
        if (Build.VERSION.SDK_INT <= 28
                && checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED)
            requestPermissions(new String[] {Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0);
        buildScreen();
        Uri uri = incoming(getIntent());
        if (uri != null) importAsync(uri);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        Uri uri = incoming(intent);
        if (uri != null) importAsync(uri);
    }

    @Override
    protected void onResume() {
        super.onResume();
        Pad.use(mapFiles());
        if (!busy) refresh(pending);
        pending = null;
    }

    /* El mando en la lista de juegos: la asignacion de cualquier juego (la
       mas reciente, ver Pad.java) */
    private String[] mapFiles() {
        List<File> found = installed();
        String[] maps = new String[found.size()];
        for (int i = 0; i < maps.length; i++) maps[i] = new File(found.get(i), "controller.map").getPath();
        return maps;
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

    /* El archivo con el que se ha abierto la app, si lo hay */
    private static Uri incoming(Intent intent) {
        if (intent == null) return null;
        if (Intent.ACTION_VIEW.equals(intent.getAction())) return intent.getData();
        if (Intent.ACTION_SEND.equals(intent.getAction())) return intent.getParcelableExtra(Intent.EXTRA_STREAM);
        return null;
    }

    /* ---- la pantalla ---- */

    private int dp(float value) { return Ui.dp(this, value); }

    private void buildScreen() {
        TextView title = Ui.title(this, "quixjs", 56);
        /* la version de quixjs, pequena, como la firma del launcher de Linux */
        LinearLayout screen = Ui.screen(this, title, Ui.signature(this, "version " + version()));
        title.setLetterSpacing(0.12f);

        ScrollView scroll = new ScrollView(this);
        scroll.setFocusable(false);
        scroll.setVerticalScrollBarEnabled(false);
        Ui.noFocusHighlight(scroll);
        games = new LinearLayout(this);
        games.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(games, Ui.wide());
        screen.addView(scroll, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

        notice = Ui.info(this, "", Ui.NOTICE);
        screen.addView(notice, Ui.wide());
        screen.addView(Ui.divider(this));
        importRow = Ui.primary(this, "Import game", "a .zip with the folder of a port", v -> pick());
        screen.addView(importRow, Ui.wide());
        /* un enlace pequeno, como en tamatama */
        TextView licenses = Ui.text(this, "Licenses", 13, Ui.MUTED, false);
        licenses.setGravity(android.view.Gravity.CENTER);
        licenses.setPaintFlags(licenses.getPaintFlags() | android.graphics.Paint.UNDERLINE_TEXT_FLAG);
        licenses.setPadding(0, dp(12), 0, dp(4));
        licenses.setFocusable(true);
        Ui.focusBackground(this, licenses);
        licenses.setOnClickListener(v -> showLicenses());
        screen.addView(licenses, Ui.wide());

        setContentView(screen);
        Ui.darkBars(this);
    }

    /* La version de la app: la de quixjs (VERSION, en android.mk) */
    private String version() {
        try {
            return getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
        } catch (android.content.pm.PackageManager.NameNotFoundException e) {
            return "";
        }
    }

    /* Los juegos importados: {files}/games/<id>/port.toml con su carpeta del
       juego ([game] game_dir). message va al aviso */
    private void refresh(String message) {
        busy = false;
        importRow.setEnabled(true);
        say(message != null ? message : "");
        games.removeAllViews();
        List<File> found = installed();
        games.addView(Ui.section(this, "games"), Ui.wide());
        for (File dir : found) {
            final String id = dir.getName();
            String version = gameValue(read(new File(dir, "port.toml")), "version");
            if (games.getChildCount() > 1) games.addView(Ui.divider(this));
            games.addView(Ui.link(this, titleOf(dir), version != null ? "port " + version : id, v -> play(id)),
                    Ui.wide());
        }
        if (found.isEmpty()) {
            games.addView(Ui.info(this, "No games yet.", Ui.TEXT), Ui.wide());
            games.addView(Ui.info(this, "quixjs plays games made with JavaScript (RPG Maker MV and MZ...)"
                    + " that you own, with a gamepad.\n\n"
                    + "Import a .zip with the folder of a quixjs port, as on a PortMaster handheld:"
                    + " its port.toml, and the game files inside gamedata/.", Ui.MUTED), Ui.wide());
            importRow.post(importRow::requestFocus);
            return;
        }
        /* despues de onResume: antes, la ventana aun no tiene las vistas */
        View first = games.getChildAt(1);
        first.post(first::requestFocus);
    }

    private void say(String message) {
        notice.setText(message);
        notice.setVisibility(message.isEmpty() ? View.GONE : View.VISIBLE);
    }

    /* Las carpetas de {files}/games con port.toml y el juego dentro: su rom
       ([game] rom, un .zip o una carpeta junto al port.toml: quixjs la monta
       al jugar) o, sin rom, la carpeta de game_dir */
    private List<File> installed() {
        List<File> found = new ArrayList<>();
        File[] dirs = new File(getFilesDir(), "games").listFiles(File::isDirectory);
        if (dirs == null) return found;
        Arrays.sort(dirs);
        for (File dir : dirs) {
            String toml = read(new File(dir, "port.toml"));
            String rom = gameValue(toml, "rom"), gameDir = value(toml, "game_dir");
            if (rom != null ? new File(dir, rom).exists() : gameDir != null && new File(dir, gameDir).isDirectory())
                found.add(dir);
        }
        return found;
    }

    /* [game] title del port.toml, o el nombre de la carpeta */
    private static String titleOf(File dir) {
        String title = gameValue(read(new File(dir, "port.toml")), "title");
        return title != null ? title : dir.getName();
    }

    private void play(String id) {
        if (busy) return;
        File dir = new File(new File(getFilesDir(), "games"), id);
        try {
            installFont();   /* la de las pantallas de quixjs */
        } catch (IOException e) {
            say("cannot prepare " + id);
            return;
        }
        Intent intent = new Intent(this, GameMenuActivity.class);
        intent.putExtra(GameMenuActivity.GAME, dir.getAbsolutePath());
        startActivityForResult(intent, MENU);
    }

    /* la fuente de las pantallas de quixjs: {files}/fonts (QUIXJS_FONT) */
    private void installFont() throws IOException {
        File font = new File(new File(getFilesDir(), "fonts"), "DejaVuSans-Bold.ttf");
        if (font.exists()) return;
        font.getParentFile().mkdirs();
        try (InputStream in = getAssets().open("fonts/DejaVuSans-Bold.ttf");
             OutputStream out = new FileOutputStream(font)) {
            copy(in, out);
        }
    }

    /* ---- licencias ---- */

    /* Las licencias de lo que lleva la app (assets/licenses, de android.mk),
       una detras de otra, con el nombre de cada una */
    private void showLicenses() {
        StringBuilder all = new StringBuilder();
        try {
            String[] names = getAssets().list("licenses");
            if (names != null) {
                Arrays.sort(names);
                for (String name : names) {
                    all.append("==== ").append(name.replace(".txt", "")).append(" ====\n\n");
                    all.append(asset("licenses/" + name)).append("\n\n");
                }
            }
        } catch (IOException e) {
            all.append("cannot read the licenses");
        }
        TextView body = Ui.text(this, all.toString(), 11, Ui.TEXT, false);
        body.setGravity(Gravity.START);
        body.setPadding(dp(16), dp(16), dp(16), dp(16));
        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(Ui.BACKGROUND);
        scroll.addView(body);
        AlertDialog dialog = new AlertDialog.Builder(this, android.R.style.Theme_Material_Dialog_Alert)
                .setTitle("Licenses")
                .setView(scroll)
                .setPositiveButton(android.R.string.ok, null)
                .create();
        Pad.dialog(dialog);
        dialog.show();
    }

    /* ---- importar ---- */

    private void pick() {
        if (busy) return;
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[] {"application/zip", "application/x-zip-compressed",
                "application/octet-stream"});
        try {
            startActivityForResult(intent, PICK);
        } catch (RuntimeException e) {
            say("cannot open the file chooser");
        }
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request == PICK && result == RESULT_OK && data != null && data.getData() != null)
            importAsync(data.getData());
        if (request == MENU && data != null) pending = data.getStringExtra(GameMenuActivity.DELETED);
    }

    /* Lo que hace un import que aun no termina: el aviso, o el mod que
       espera a que se elija el juego */
    private static final class Pending {
        String message;
        String modName, modRoot;
        List<File> choices;
    }

    private void importAsync(Uri uri) {
        busy = true;
        importRow.setEnabled(false);
        say("importing...");
        File temporary = new File(getCacheDir(), "import.zip");
        new Thread(() -> {
            Pending result = new Pending();
            try {
                copyIn(uri, temporary);
                importFile(temporary, zipName(uri), result);
            } catch (IOException | RuntimeException e) {
                result.message = "cannot import it";
            }
            if (result.choices == null) temporary.delete();
            runOnUiThread(() -> {
                if (result.choices != null) chooseGame(temporary, result);
                else refresh(result.message);
            });
        }).start();
    }

    private void copyIn(Uri uri, File temporary) throws IOException {
        try (InputStream in = getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(temporary)) {
            if (in == null) throw new IOException("cannot open the file");
            copy(in, out);
        }
    }

    /* Un port (con port.toml) o un mod suelto. Deja el aviso en result, o
       los juegos entre los que elegir para el mod */
    private void importFile(File temporary, String name, Pending result) throws IOException {
        try (ZipFile zip = new ZipFile(temporary)) {
            String root = portRoot(zip);
            if (root != null) {
                result.message = importPort(zip, root);
                return;
            }
            String modRoot = modRoot(zip);
            if (modRoot == null) {
                result.message = "no port.toml and no mod in it";
                return;
            }
            List<File> found = installed();
            if (found.isEmpty()) {
                result.message = "import the game first, then its mods";
                return;
            }
            if (found.size() == 1) {
                result.message = importMod(zip, modRoot, name, found.get(0));
                return;
            }
            result.modName = name;
            result.modRoot = modRoot;
            result.choices = found;
        }
    }

    /* Varios juegos: para cual es el mod */
    private void chooseGame(File temporary, Pending pending) {
        String[] titles = new String[pending.choices.size()];
        for (int i = 0; i < titles.length; i++) titles[i] = titleOf(pending.choices.get(i));
        AlertDialog dialog = new AlertDialog.Builder(this, android.R.style.Theme_Material_Dialog_Alert)
                .setTitle("mod " + pending.modName + " for")
                .setItems(titles, (d, which) -> new Thread(() -> {
                    String message;
                    try (ZipFile zip = new ZipFile(temporary)) {
                        message = importMod(zip, pending.modRoot, pending.modName, pending.choices.get(which));
                    } catch (IOException | RuntimeException e) {
                        message = "cannot import it";
                    }
                    temporary.delete();
                    final String done = message;
                    runOnUiThread(() -> refresh(done));
                }).start())
                .setOnCancelListener(d -> {
                    temporary.delete();
                    refresh("nothing imported");
                })
                .create();
        Pad.dialog(dialog);
        dialog.show();
    }

    /* La carpeta del port: la que tiene port.toml, la de ruta mas corta ("" si
       esta en la raiz), o null */
    private static String portRoot(ZipFile zip) {
        String root = null;
        Enumeration<? extends ZipEntry> entries = zip.entries();
        while (entries.hasMoreElements()) {
            String name = entries.nextElement().getName();
            if (!name.equals("port.toml") && !name.endsWith("/port.toml")) continue;
            String prefix = name.substring(0, name.length() - "port.toml".length());
            if (root == null || prefix.length() < root.length()) root = prefix;
        }
        return root;
    }

    /* Lo del port que Android no usa: los binarios y las librerias de
       PortMaster, los scripts, las licencias del port y fonts/ si el
       port.toml no la nombra. rel: la ruta dentro del port */
    private static boolean skipped(String rel, boolean fontsUsed) {
        String top = rel.contains("/") ? rel.substring(0, rel.indexOf('/')) : "";
        if (rel.equals("quixjs.aarch64") || rel.equals("quixjs.armhf")) return true;
        if (top.startsWith("libs.") || top.equals("licenses")) return true;
        if (top.equals("fonts") && !fontsUsed) return true;
        return rel.endsWith(".so") || rel.endsWith(".sh");
    }

    private String importPort(ZipFile zip, String root) throws IOException {
        String toml = read(zip, zip.getEntry(root + "port.toml"));
        String title = value(toml, "title");
        String id = root.isEmpty() ? "" : clean(new File(root).getName());
        if (id.isEmpty()) id = clean(gameValue(toml, "id"));
        if (id.isEmpty()) id = clean(title);
        if (id.isEmpty()) id = "game";
        String gameDir = value(toml, "game_dir");
        if (gameDir == null) return "port.toml has no [game] game_dir";
        boolean fontsUsed = toml != null && toml.contains("fonts/");
        File target = new File(new File(getFilesDir(), "games"), id);
        String base = target.getCanonicalPath() + File.separator;
        int files = 0;
        Enumeration<? extends ZipEntry> entries = zip.entries();
        while (entries.hasMoreElements()) {
            ZipEntry entry = entries.nextElement();
            String name = entry.getName();
            if (!name.startsWith(root) || entry.isDirectory()) continue;
            String rel = name.substring(root.length());
            if (skipped(rel, fontsUsed)) continue;
            File out = new File(target, rel);
            if (!out.getCanonicalPath().startsWith(base)) continue;   /* nada fuera */
            out.getParentFile().mkdirs();
            try (InputStream in = zip.getInputStream(entry); OutputStream o = new FileOutputStream(out)) {
                copy(in, o);
            }
            files++;
        }
        String shown = title != null ? title : id;
        if (!new File(target, gameDir).isDirectory())
            return shown + ": no game in " + gameDir + " (copy the game files there and zip the folder again)";
        return shown + ": " + files + " files imported";
    }

    /* Un mod suelto: la carpeta del .zip que tiene las del juego (js/, img/,
       data/, audio/...), o null */
    private static String modRoot(ZipFile zip) {
        String root = null;
        Pattern game = Pattern.compile("^(.*?)(js|img|data|audio|fonts|movies|icon)/");
        Enumeration<? extends ZipEntry> entries = zip.entries();
        while (entries.hasMoreElements()) {
            Matcher m = game.matcher(entries.nextElement().getName());
            if (m.find() && (root == null || m.group(1).length() < root.length())) root = m.group(1);
        }
        return root;
    }

    private static String importMod(ZipFile zip, String root, String name, File game) throws IOException {
        int files = extract(zip, root, new File(new File(game, "mods"), name));
        return "mod " + name + " for " + titleOf(game) + ": " + files + " files (turn it on in Mods)";
    }

    /* Saca lo que hay bajo prefix a target; nada fuera de target. Devuelve
       cuantos archivos */
    private static int extract(ZipFile zip, String prefix, File target) throws IOException {
        String base = target.getCanonicalPath() + File.separator;
        int files = 0;
        Enumeration<? extends ZipEntry> entries = zip.entries();
        while (entries.hasMoreElements()) {
            ZipEntry entry = entries.nextElement();
            String name = entry.getName();
            if (!name.startsWith(prefix) || entry.isDirectory()) continue;
            File out = new File(target, name.substring(prefix.length()));
            if (!out.getCanonicalPath().startsWith(base)) continue;
            out.getParentFile().mkdirs();
            try (InputStream in = zip.getInputStream(entry); OutputStream o = new FileOutputStream(out)) {
                copy(in, o);
            }
            files++;
        }
        return files;
    }

    /* el nombre del .zip elegido sin .zip (el nombre del mod), o "mod" */
    private String zipName(Uri uri) {
        String name = null;
        try (android.database.Cursor cursor = getContentResolver().query(uri, null, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int column = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME);
                if (column >= 0) name = cursor.getString(column);
            }
        } catch (RuntimeException ignored) {
        }
        if (name == null) name = uri.getLastPathSegment();
        if (name == null) return "mod";
        name = clean(new File(name).getName().replaceAll("(?i)\\.zip$", ""));
        return name.isEmpty() ? "mod" : name;
    }

    /* Un nombre de carpeta seguro: letras, numeros, . _ - y espacios; nunca
       "." ni ".." */
    private static String clean(String name) {
        if (name == null) return "";
        String out = name.replaceAll("[^A-Za-z0-9._ -]", "_").trim();
        return out.matches("\\.*") ? "" : out;
    }

    /* ---- texto ---- */

    /* key = "valor" de un toml (la primera vez que sale, al principio de una
       linea), o null */
    private static String value(String toml, String key) {
        if (toml == null) return null;
        Matcher m = Pattern.compile("(?m)^\\s*" + key + "\\s*=\\s*\"([^\"]*)\"").matcher(toml);
        return m.find() ? m.group(1) : null;
    }

    /* key = "valor" dentro de [game], o null */
    private static String gameValue(String toml, String key) {
        if (toml == null) return null;
        Matcher m = Pattern.compile("(?ms)^\\[game\\]\\s*$(.*?)(?=^\\[|\\z)").matcher(toml);
        return m.find() ? value(m.group(1), key) : null;
    }

    private String asset(String path) {
        try (InputStream in = getAssets().open(path)) {
            return readAll(in);
        } catch (IOException e) {
            return null;
        }
    }

    private static String read(File file) {
        try (InputStream in = new java.io.FileInputStream(file)) {
            return readAll(in);
        } catch (IOException e) {
            return null;
        }
    }

    private static String read(ZipFile zip, ZipEntry entry) throws IOException {
        if (entry == null) return null;
        try (InputStream in = zip.getInputStream(entry)) {
            return readAll(in);
        }
    }

    private static String readAll(InputStream in) throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        copy(in, bytes);
        String text = new String(bytes.toByteArray(), StandardCharsets.UTF_8);
        return text.startsWith("\uFEFF") ? text.substring(1) : text;
    }

    private static void copy(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1 << 16];
        for (int n; (n = in.read(buffer)) > 0;) out.write(buffer, 0, n);
    }
}
