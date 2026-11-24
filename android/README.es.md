# android

[English](README.md) | Español

La app de Android de quixjs: `build/quixjs.apk`, para teléfonos y tablets arm64
con Android 9 o más nuevo. Juega cualquier port de quixjs (juegos de RPG
Maker MV y MZ...) con los archivos de tu copia. La app no trae juegos ni
ports. El juego se juega con mando.

## Construir

Hace falta el SDK de Android (`ANDROID_HOME`, por defecto `~/Android/Sdk`)
con un NDK, unas build-tools y una plataforma, y un JDK. Sin Gradle.

    sdkmanager "ndk;28.2.13676358" "platforms;android-34" "build-tools;36.1.0"
    make android

La primera vez descarga SDL2, SDL2_ttf y SDL2_image en `obj/deps/android/` y
construye JavaScriptCore para Android (`make android-jsc`, tarda, solo una vez). Sale `build/quixjs.apk`, firmado con la clave de depuración
(`~/.android/debug.keystore`; si no existe, se crea).

Para el emulador de Android del PC (x86_64), solo para probar: el build de
arm64 no corre ahí (la traducción ARM del emulador se para en algunas
instrucciones de JavaScriptCore). Un `.apk` de x86_64, `build/quixjs-x86_64.apk`
(la primera vez construye también JavaScriptCore para x86_64):

    make android ABI=x86_64

## Instalar

    adb install -r build/quixjs.apk

O copiar `build/quixjs.apk` al teléfono y abrirlo.

## Uso

1. Preparar el port en el PC, como en una portátil con PortMaster: la
   carpeta del port (por ejemplo `fearandhunger/`, con su `port.toml` y
   `mods/`) con la rom del juego junto a `port.toml` (como diga el README
   del port, por ejemplo `www.zip`). Comprimir la carpeta entera en un
   `.zip`.
2. "Import game" abre el selector de archivos: elegir ese `.zip`. Abrirlo
   con quixjs desde un gestor de archivos también lo importa. La app busca
   la carpeta con `port.toml` y la copia a la app, sin lo que Android no
   usa (`quixjs.aarch64`, `libs.*`, `.so`, `.sh`, `licenses/`). Importar
   otra vez el port sobre el mismo juego conserva sus partidas y
   `quixjs-settings.toml`, salvo que el `.zip` traiga los suyos.
   Un `.zip` con solo un mod (una traducción, un parche: sus carpetas
   `js/`, `img/`, `data/`...) se importa como el mod `<nombre del zip>` de
   un juego; con más de un juego, la app pregunta para cuál.
3. La pantalla de inicio enseña los juegos importados. Al tocar uno sale
   su menú, el mismo que el launcher de quixjs en una portátil:
   - Play: arranca el juego.
   - Achievements: los logros del port, conseguidos o no.
   - Mods: cada mod On u Off. Si dos mods cambian el mismo archivo, gana el
     de número más alto.
   - Options: Show FPS, Profiler (escribe `quixjs-profile.txt`), Save log
     y Save profile (copian `log.txt` o `quixjs-profile.txt` de la última
     partida donde elijas, para mandarlos) y Reset achievements.
   - Cheats: si el port tiene trucos.
   - Transfer Save: mandar las partidas a otro dispositivo con quixjs en la
     misma red (un PC, una portátil, otro teléfono), recibirlas, o volver a
     las de antes de la última recepción. Los dos dispositivos enseñan una
     firma de 4 letras: hay que comprobar que coincide.
   - Controls: asignar todos los botones del mando. Abre la pantalla
     Controls de quixjs, la misma que en una portátil, y guarda
     `controller.map` en la carpeta del juego; "Use the default mapping"
     lo borra.
   - Manage: Delete mods (uno a uno) y Delete game (sus archivos, mods,
     partidas y logros). Los dos preguntan antes.
   Lo que se cambia se guarda en `quixjs-settings.toml`, en la carpeta del
   juego, el mismo archivo que en una portátil.
4. En el juego: el mando. Select + Start, o el botón (o el gesto) Atrás
   del teléfono dos veces, cierran el juego y vuelven a su menú. Los
   combos son los de una portátil: Select + R1 hace una captura (también
   en la galería) y Select + L1 abre la lupa (Start la fija y la desfija;
   fija, el mando vuelve al juego); en los juegos que los tienen, Select + X
   guarda un savestate, Select + Y lo carga y Select + izquierda / derecha
   cambia de ranura.

Las pantallas de la app se usan con el touch o con el mando, con la misma
asignación que el juego (`controller.map`): arriba y abajo mueven, a o
Start abre, b vuelve. Atrás vuelve a la anterior.

Las partidas quedan en la carpeta de la app: importar otra vez el juego no
las borra. Los mensajes de quixjs van a logcat (`adb logcat -s quixjs`) y a
`log.txt` en la carpeta del juego.

## Archivos

| archivo | qué es |
|---------|--------|
| `../cmake/android/native.cmake` | `make android`, la parte nativa con el NDK: SDL2, SDL2_ttf, SDL2_image y `libquixjs.so` |
| `../cmake/android/apk.cmake` | el Java, los recursos, los assets y el `.apk` firmado |
| `AndroidManifest.xml` | las actividades y los permisos (la red local, para Transfer Save; el vibrador, cuando el mando no vibra) |
| `java/.../MainActivity.java` | la lista de juegos y las licencias |
| `java/.../Importer.java` | importar un `.zip`: un port, o un mod para un juego importado |
| `java/.../Files.java` | leer, copiar y borrar archivos; los valores sencillos de un `port.toml` |
| `java/.../GameMenuActivity.java` | el menú de un juego: la portada y lo que comparten sus pantallas (`runtime/platform/android/menu.c` por JNI) |
| `java/.../Menu*.java` | las pantallas del menú: `MenuAchievements`, `MenuMods` (y Delete mods), `MenuOptions`, `MenuCheats` y `MenuControls` |
| `java/.../TransferActivity.java` | Transfer Save: la portada, Restore Backup y Delete Save (`runtime/platform/android/transfer.c` por JNI) |
| `java/.../TransferSend.java`, `TransferReceive.java` | Send Save y Receive Save |
| `java/.../GameActivity.java` | el juego: SDL con quixjs, derecho en el juego |
| `java/.../Ui.java` | los colores y las filas que comparten las pantallas |
| `java/.../Pad.java` | el mando en las pantallas, con la asignación de `controller.map` |

Adaptado de tamatama (MIT):
https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/android
