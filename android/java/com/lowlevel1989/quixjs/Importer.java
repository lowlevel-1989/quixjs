/* Importer: importar un .zip en la app (Import game de MainActivity, o un
   .zip abierto con la app desde un gestor de archivos).

   Lo elegido se copia en un thread aparte a un archivo temporal (ZipFile
   necesita un archivo) y se importa (ver importFile):
     - un port: la carpeta del .zip con port.toml (la de ruta mas corta), a
       {files}/games/<nombre de la carpeta>/, sin lo que Android no usa
       (los binarios de PortMaster, libs.*, .so, .sh y licenses/). Sobre un
       juego ya importado no se borra nada: lo que trae el .zip pisa a lo
       que habia y el resto (las partidas, quixjs-settings.toml) se queda
     - un mod suelto: un .zip sin port.toml pero con las carpetas de un
       juego (js/, img/, data/...), a mods/<nombre del .zip>/ de un juego
       importado (si hay varios, se elige cual)

   Al terminar, MainActivity.refresh con el aviso.

   Adaptado de tamatama (android/java/.../MainActivity.java, MIT):
   https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/android/java/com/lowlevel1989/tamatama/MainActivity.java */
package com.lowlevel1989.quixjs;

import android.app.AlertDialog;
import android.content.Intent;
import android.net.Uri;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Enumeration;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

final class Importer {
    /* el codigo de startActivityForResult del selector de archivos */
    static final int PICK = 1;

    private final MainActivity app;

    Importer(MainActivity app) {
        this.app = app;
    }

    /* El selector de archivos de Android: un .zip */
    void pick() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[] {"application/zip", "application/x-zip-compressed",
                "application/octet-stream"});
        try {
            app.startActivityForResult(intent, PICK);
        } catch (RuntimeException e) {
            app.say("cannot open the file chooser");
        }
    }

    /* Lo que hace un import que aun no termina: el aviso, o el mod que
       espera a que se elija el juego */
    private static final class Pending {
        String message;
        String modName, modRoot;
        List<File> choices;
    }

    void start(Uri uri) {
        app.importing();
        File temporary = new File(app.getCacheDir(), "import.zip");
        new Thread(() -> {
            Pending result = new Pending();
            try {
                copyIn(uri, temporary);
                importFile(temporary, zipName(uri), result);
            } catch (IOException | RuntimeException e) {
                result.message = "cannot import it";
            }
            if (result.choices == null) temporary.delete();
            app.runOnUiThread(() -> {
                if (result.choices != null) chooseGame(temporary, result);
                else app.refresh(result.message);
            });
        }).start();
    }

    private void copyIn(Uri uri, File temporary) throws IOException {
        try (InputStream in = app.getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(temporary)) {
            if (in == null) throw new IOException("cannot open the file");
            Files.copy(in, out);
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
            List<File> found = app.installed();
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
        for (int i = 0; i < titles.length; i++) titles[i] = Files.titleOf(pending.choices.get(i));
        AlertDialog dialog = new AlertDialog.Builder(app, android.R.style.Theme_Material_Dialog_Alert)
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
                    app.runOnUiThread(() -> app.refresh(done));
                }).start())
                .setOnCancelListener(d -> {
                    temporary.delete();
                    app.refresh("nothing imported");
                })
                .create();
        Pad.dialog(dialog);
        dialog.show();
    }

    /* ---- un port ---- */

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
        if (rel.equals("quixjs.aarch64")) return true;
        if (top.startsWith("libs.") || top.equals("licenses")) return true;
        if (top.equals("fonts") && !fontsUsed) return true;
        return rel.endsWith(".so") || rel.endsWith(".sh");
    }

    private String importPort(ZipFile zip, String root) throws IOException {
        String toml = Files.read(zip, zip.getEntry(root + "port.toml"));
        String title = Files.value(toml, "title");
        String id = root.isEmpty() ? "" : Files.clean(new File(root).getName());
        if (id.isEmpty()) id = Files.clean(Files.gameValue(toml, "id"));
        if (id.isEmpty()) id = Files.clean(title);
        if (id.isEmpty()) id = "game";
        String gameDir = Files.gameValue(toml, "game_dir");
        if (gameDir == null) return "port.toml has no [game] game_dir";
        boolean fontsUsed = toml != null && toml.contains("fonts/");
        File target = new File(new File(app.getFilesDir(), "games"), id);
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
                Files.copy(in, o);
            }
            files++;
        }
        String shown = title != null ? title : id;
        /* el juego: en la rom ([game] rom, www.zip junto a port.toml) o, sin
           rom, en la carpeta game_dir */
        String rom = Files.gameValue(toml, "rom");
        if (rom != null && !rom.isEmpty()) {
            if (!new File(target, rom).exists())
                return shown + ": no game: copy " + rom + " next to port.toml and zip the folder again";
        } else if (!new File(target, gameDir).isDirectory()) {
            return shown + ": no game in " + gameDir + " (copy the game files there and zip the folder again)";
        }
        return shown + ": " + files + " files imported";
    }

    /* ---- un mod suelto ---- */

    /* La carpeta del .zip que tiene las del juego (js/, img/, data/,
       audio/...), o null */
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
        return "mod " + name + " for " + Files.titleOf(game) + ": " + files + " files (turn it on in Mods)";
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
                Files.copy(in, o);
            }
            files++;
        }
        return files;
    }

    /* el nombre del .zip elegido sin .zip (el nombre del mod), o "mod" */
    private String zipName(Uri uri) {
        String name = null;
        try (android.database.Cursor cursor = app.getContentResolver().query(uri, null, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int column = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME);
                if (column >= 0) name = cursor.getString(column);
            }
        } catch (RuntimeException ignored) {
        }
        if (name == null) name = uri.getLastPathSegment();
        if (name == null) return "mod";
        name = Files.clean(new File(name).getName().replaceAll("(?i)\\.zip$", ""));
        return name.isEmpty() ? "mod" : name;
    }
}
