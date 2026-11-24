/* Files: lo que las pantallas hacen con archivos: leerlos (del disco, de un
   .zip o de los assets de la app), copiarlos, borrarlos, su tamano, un
   nombre de carpeta seguro y los valores sencillos de un port.toml. */
package com.lowlevel1989.quixjs;

import android.content.Context;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

final class Files {
    private Files() {}

    /* ---- el port.toml ---- */

    /* key = "valor" de un toml (la primera vez que sale, al principio de una
       linea), o null */
    static String value(String toml, String key) {
        if (toml == null) return null;
        Matcher m = Pattern.compile("(?m)^\\s*" + key + "\\s*=\\s*\"([^\"]*)\"").matcher(toml);
        return m.find() ? m.group(1) : null;
    }

    /* key = "valor" dentro de [game], o null */
    static String gameValue(String toml, String key) {
        if (toml == null) return null;
        Matcher m = Pattern.compile("(?ms)^\\[game\\]\\s*$(.*?)(?=^\\[|\\z)").matcher(toml);
        return m.find() ? value(m.group(1), key) : null;
    }

    /* [game] title del port.toml de dir, o el nombre de la carpeta */
    static String titleOf(File dir) {
        String title = gameValue(read(new File(dir, "port.toml")), "title");
        return title != null ? title : dir.getName();
    }

    /* Un nombre de carpeta seguro: letras, numeros, . _ - y espacios; nunca
       "." ni ".." */
    static String clean(String name) {
        if (name == null) return "";
        String out = name.replaceAll("[^A-Za-z0-9._ -]", "_").trim();
        return out.matches("\\.*") ? "" : out;
    }

    /* ---- leer ---- */

    static String read(File file) {
        try (InputStream in = new FileInputStream(file)) {
            return readAll(in);
        } catch (IOException e) {
            return null;
        }
    }

    static String read(ZipFile zip, ZipEntry entry) throws IOException {
        if (entry == null) return null;
        try (InputStream in = zip.getInputStream(entry)) {
            return readAll(in);
        }
    }

    /* Un archivo de los assets de la app (las licencias), o null */
    static String asset(Context context, String path) {
        try (InputStream in = context.getAssets().open(path)) {
            return readAll(in);
        } catch (IOException e) {
            return null;
        }
    }

    /* Todo el texto, en UTF-8 y sin la marca BOM */
    static String readAll(InputStream in) throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        copy(in, bytes);
        String text = new String(bytes.toByteArray(), StandardCharsets.UTF_8);
        return text.startsWith("\uFEFF") ? text.substring(1) : text;
    }

    /* ---- copiar y borrar ---- */

    static void copy(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1 << 16];
        for (int n; (n = in.read(buffer)) > 0;) out.write(buffer, 0, n);
    }

    /* Borra una carpeta con todo lo que tiene (sin seguir enlaces) */
    static boolean delete(File file) {
        File[] children = file.isDirectory() && !java.nio.file.Files.isSymbolicLink(file.toPath())
                ? file.listFiles() : null;
        if (children != null) for (File child : children) delete(child);
        return file.delete() || !file.exists();
    }

    /* "512 bytes" o "12 KB" */
    static String size(File file) {
        long bytes = file.length();
        return bytes < 1024 ? bytes + " bytes" : (bytes + 1023) / 1024 + " KB";
    }
}
