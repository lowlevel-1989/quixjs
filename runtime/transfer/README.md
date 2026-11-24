# transfer

English | [Español](README.es.md)

Transfer Save: passes the saves of a game to another device on the same
network (a PC, a handheld), without a keyboard and without opening ports. It
is opened from the menu of `--launcher`. It comes from tamatama
(https://github.com/lowlevel-1989/tamatama/tree/780b7e723f3dbe8a58bc54ed571f1518dac9191a, `src/transfer`), which passes one
file of 768 bytes; here they are all the files of `[saves] files`, in one
package sent in pieces.

```
[game]
title = "Fear & Hunger"
id    = "fear-and-hunger"    # only between the same game (default: the title)

[saves]
dir   = "save"   # the folder of the saves, from game_dir (absolute: as is; without dir: game_dir)
files = ["*.rpgsave"]        # patterns inside dir
```

The achievements file (`[saves] achievements`) goes in the package with the
saves. After receiving it, the achievements are read again.

The logic, without screens:

| file          | what it does |
|---------------|--------------|
| `net.c`       | the UDP socket, the announcements and the format of the packets |
| `sender.c`    | the sending, with its state |
| `receiver.c`  | the receiving, with its state |
| `saves/`      | the saves: `saves.c` (what a save is and finding them), `pack.c` (pack them and store the ones that arrive) and `backup.c` (the backup, Restore Backup and Delete Save) |
| `identity.c`  | the name (user@hostname (system)) and the 4-character signature, new each time Send Save or Receive Save opens |

And the screens, on `launcher/ui/`:

| file         | what it does |
|--------------|--------------|
| `transfer.c` | Transfer Save: Send Save, Receive Save, Restore Backup and Delete Save |

## The protocol

UDP text packets on port 47474, all with the same first line,
`QUIX 1 <type> <signature of the sender> [words]`, and sometimes something
after it:

```
HELLO S|R <name>       every second, to each address of the network and by
                       broadcast; S in Send Save, R in Receive Save
HERE S|R <name>        the answer to HELLO
OFFER <n>              the number of the offer, and after it, in lines:
                       name, game, size, date (unix) and summary
WAIT                   the offer has arrived: the other is asked
OK                     Accept
NO <reason>            declined, version (another game), format, busy or cancelled
DATA <i> <pieces> <bytes> <crc32>   and after it piece i of the package
                       (16 KB each)
ACK <i>                piece i has arrived: the next one
DONE / FAIL <reason>   saved, or not (format, crc, write)
CANCEL                 the sender does not wait any more
```

The sender repeats OFFER every second and each piece every 250 ms until it is
answered; the receiver answers each repetition with its last answer. So a lost
packet does not matter. The CRC32 is of the whole package.

The package is text and then the bytes:

```
QUIXSAVE 1
<bytes> <relative path>     one line per file
<empty line>
<the files, one after another>
```

Why there is no need to open ports: a firewall lets through the answer to
what its device has sent. Both sides send HELLO to each address of their
network from port 47474 to port 47474, so what comes from the other already
counts as an answer. It only fails on networks that do not let devices talk
to each other (guest networks, some hotels).

## What it protects

- It only listens while Receive Save is open, and nothing is written without
  Accept.
- Only between the same game (`[game] id`).
- Only the files that match `[saves] files` (and the achievements file) are
  written, and never outside `[saves] dir`: a path with `..` or an absolute
  one makes the package invalid.
- The new files are written apart (`<saves dir>/.quixjs/incoming`) and moved
  with rename. The saves that were there go to `<saves dir>/.quixjs/backup`,
  and Restore Backup swaps them back.
- Delete Save moves the saves to that backup too (the achievements file
  stays), so the game starts from zero and Restore Backup brings them back.

## Tests

`tests/transfer.sh` starts two quixjs processes on this computer, one in Receive Save
and the other in Send Save, driven with `--script`. With
`QUIXJS_TRANSFER_HOSTS=127.0.0.1` they only talk through that address,
without going out to the network.
