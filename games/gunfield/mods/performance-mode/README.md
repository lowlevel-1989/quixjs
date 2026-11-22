English | [Español](README.es.md)

# performance-mode

Faster with many enemies on screen: objects that need two colour effects at
once, or an effect with transparency, are drawn without them (on a handheld,
each one is drawn apart and that costs more than the rest of the frame). You
lose some colours: an enemy's flash when it is hit, the tint of some
platforms.

It comes on. To turn it off: `performance-mode: Off` in the Mods option of the launcher
of the port.
