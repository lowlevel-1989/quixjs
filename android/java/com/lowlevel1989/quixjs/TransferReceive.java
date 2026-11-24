/* TransferReceive: Receive Save de TransferActivity. Abre la recepcion
   (receiveOpen: el nombre y la firma de este dispositivo) y espera una
   oferta; al llegar, Accept o Decline; tras Accept, como van llegando.
   TransferActivity.tick llama a poll cada TICK_MS mientras esta abierta.

     show        esperando unas partidas: este dispositivo y su firma
     ask         llego una oferta: Accept o Decline
     receiving   tras Accept, mientras llegan; Cancel */
package com.lowlevel1989.quixjs;

final class TransferReceive {
    /* Lo que devuelve receivePoll: runtime/transfer/receiver.h */
    private static final int OFFER = 1, CANCELLED = 2, DONE = 3;

    private final TransferActivity t;
    boolean open;                   /* receiveOpen abierto */
    private String me, signature;   /* los de este dispositivo */

    TransferReceive(TransferActivity transfer) {
        t = transfer;
    }

    void show() {
        if (!open) {
            String[] mine = TransferActivity.receiveOpen();
            if (mine == null) {
                t.showResult("receive save", "cannot use the network", t::showHome);
                return;
            }
            open = true;
            me = mine[0];
            signature = mine[1];
        }
        t.screen(TransferActivity.Mode.RECEIVE, "receive save");
        t.line("this device: " + me, Ui.TEXT);
        t.signature(signature);
        t.line("waiting for a save...", Ui.TEXT);
        t.line("choose this device in Send Save", Ui.TEXT);
        notice();
        TransferActivity.receiveListen(true);
        t.schedule();
    }

    private void ask() {
        TransferActivity.receiveListen(false);
        String[] offer = TransferActivity.receiveOffer();
        t.screen(TransferActivity.Mode.ASK, "receive save");
        t.line("a save from " + offer[0], Ui.TEXT);
        t.signature(offer[1]);
        t.line(offer[2] + "  " + offer[3], Ui.TEXT);
        t.line("your saves will be kept as backup", Ui.NOTICE);
        t.gap();
        t.primary("Accept", v -> {
            TransferActivity.receiveAccept();
            receiving();
        }).requestFocus();
        t.item("Decline", null, v -> decline());
    }

    private void receiving() {
        t.screen(TransferActivity.Mode.RECEIVING, "receive save");
        t.progress = t.line("receiving...", Ui.TEXT);
        t.gap();
        t.item("Cancel", null, v -> cancel()).requestFocus();
    }

    void decline() {
        TransferActivity.receiveDecline();
        show();
    }

    void cancel() {
        TransferActivity.receiveCancel();
        show();
    }

    /* Lo ultimo que paso (una oferta rechazada...), en el aviso */
    private void notice() {
        String last = TransferActivity.receiveNotice();
        t.say(last != null ? last : "");
    }

    /* Cada TICK_MS: una oferta, si se cancelo, o como va la recepcion */
    void poll() {
        int event = TransferActivity.receivePoll();
        TransferActivity.Mode mode = t.mode;
        if (mode == TransferActivity.Mode.RECEIVE && event == OFFER) ask();
        else if (mode == TransferActivity.Mode.ASK && event == CANCELLED) show();
        else if (mode == TransferActivity.Mode.RECEIVING && event == DONE) {
            String result = TransferActivity.receiveResult();
            boolean received = "save received".equals(result);
            t.showResult("receive save", result, received ? t::showHome : this::show);
        } else if (mode == TransferActivity.Mode.RECEIVING && t.progress != null) {
            t.progress.setText("receiving " + TransferActivity.receiveProgress() + "%");
        }
        if (t.mode == TransferActivity.Mode.RECEIVE) notice();
    }

    void close() {
        if (open) TransferActivity.receiveClose();
        open = false;
    }
}
