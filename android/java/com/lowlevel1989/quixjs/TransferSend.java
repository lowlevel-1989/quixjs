/* TransferSend: Send Save de TransferActivity. Abre el envio (sendOpen: el
   nombre y la firma de este dispositivo), enseña los dispositivos que estan
   en Receive Save y, al elegir uno, espera a que acepte mientras se mandan
   las partidas. TransferActivity.tick llama a poll cada TICK_MS mientras
   esta abierto.

     showDevices   a quien mandarlas: los que estan en Receive Save
     sending       esperando a que el otro pulse Accept; Cancel */
package com.lowlevel1989.quixjs;

import android.text.TextUtils;
import android.view.View;

final class TransferSend {
    private final TransferActivity t;
    boolean open;                   /* sendOpen abierto */
    private String me, signature;   /* los de este dispositivo */
    private String saveLine;        /* "3 files  2026-11-20 18:30" */
    private String shownDevices = "";
    private int deviceLines;        /* las vistas de showDevices antes de la lista */

    TransferSend(TransferActivity transfer) {
        t = transfer;
    }

    void showDevices() {
        String[] save = TransferActivity.saves(false);
        if (save.length == 0) {
            t.showResult("send save", "there are no saves to send", t::showHome);
            return;
        }
        saveLine = save[0] + "  " + save[1];
        if (!open) {
            String[] mine = TransferActivity.sendOpen();
            if (mine == null) {
                t.showResult("send save", "cannot use the network", t::showHome);
                return;
            }
            open = true;
            me = mine[0];
            signature = mine[1];
        }
        t.screen(TransferActivity.Mode.DEVICES, "send save");
        t.line(saveLine, Ui.TEXT);
        t.line("this device: " + me, Ui.TEXT);
        t.signature(signature);
        t.line("send it to:", Ui.TEXT);
        deviceLines = t.content.getChildCount();
        shownDevices = null;
        refreshDevices();
        t.schedule();
    }

    /* Rehace la lista si ha cambiado: cada fila, firma y nombre */
    private void refreshDevices() {
        String[] list = TransferActivity.sendDevices();
        String key = TextUtils.join("\n", list);
        if (key.equals(shownDevices)) return;
        shownDevices = key;
        while (t.content.getChildCount() > deviceLines) t.content.removeViewAt(deviceLines);
        for (int i = 0; i < list.length; i += 2) {
            final String sig = list[i], name = list[i + 1];
            View row = t.item(sig, name, v -> start(sig, name));
            if (i == 0) row.requestFocus();
        }
        t.say(list.length == 0 ? "looking for devices in Receive Save..." : "");
    }

    private void start(String sig, String name) {
        String problem = TransferActivity.sendStart(sig);
        if (problem != null) {
            t.showResult("send save", problem, this::showDevices);
            return;
        }
        t.screen(TransferActivity.Mode.SENDING, "send save");
        t.line("waiting for " + name, Ui.TEXT);
        t.signature(sig);
        t.line("to accept it", Ui.TEXT);
        t.progress = t.line("", Ui.HIGHLIGHT);
        t.gap();
        t.item("Cancel", null, v -> cancel()).requestFocus();
    }

    void cancel() {
        TransferActivity.sendCancel();
        t.showResult("send save", "cancelled", this::showDevices);
    }

    /* Cada TICK_MS: la lista de dispositivos, o como va el envio */
    void poll() {
        boolean finished = TransferActivity.sendPoll();
        if (t.mode == TransferActivity.Mode.DEVICES) refreshDevices();
        if (t.mode != TransferActivity.Mode.SENDING) return;
        int percent = TransferActivity.sendProgress();
        if (t.progress != null) t.progress.setText(percent >= 0 ? "sending " + percent + "%" : "");
        if (finished) {
            String result = TransferActivity.sendResult();
            boolean sent = "save sent".equals(result);
            t.showResult("send save", result, sent ? t::showHome : this::showDevices);
        }
    }

    void close() {
        if (open) TransferActivity.sendClose();
        open = false;
    }
}
