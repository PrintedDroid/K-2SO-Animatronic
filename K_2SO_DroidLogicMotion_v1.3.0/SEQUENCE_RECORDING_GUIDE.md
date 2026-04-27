# Performance Recording Guide -- moved

The full guide for the K-2SO sequence recording system has been folded into the
**System Documentation PDF** (chapters 14 -- Sequence Recording, 15 -- Playlist
Chaining, 16 -- IR-to-Sequence Mapping):

`generate docs/K-2SO_Animatronic_Controller_System_Documentation_v1.3.0.pdf`

The original 18 KB markdown guide is preserved as `SEQUENCE_RECORDING_GUIDE.md.bak`
next to this file.

## Quick CLI cheat sheet

```
seq new "name"          Start a recording
seq frame <ms>          Capture a frame with the given duration
seq save                Persist the recording to LittleFS (crash-safe temp/backup/replace flow)
seq cancel              Drop the in-progress recording
seq play "name"         Play once
seq loop "name"         Play in loop
seq stop                Stop playback
seq list                List saved sequences
seq info "name"         Frame count, duration
seq rename "old" "new"
seq delete "name"       (with confirmation)
seq status              Current recording / playback state
seq format              Reformat LittleFS (with confirmation)

seq playlist add "name"
seq playlist clear
seq playlist play / loop
seq playlist remove <n>           NEW v1.3.0
seq playlist move <from> <to>     NEW v1.3.0
seq playlist save "name"          NEW v1.3.0 -- persist named playlist
seq playlist load "name"          NEW v1.3.0 -- restore named playlist
seq playlist list                 NEW v1.3.0 -- list saved playlists

seq verify "name"       NEW v1.3.0 -- check JSON integrity
seq verify all          NEW v1.3.0 -- bulk verify before a show
seq export "name"       NEW v1.3.0 -- print sequence JSON to serial
seq import              NEW v1.3.0 -- paste sequence JSON to import
seq duplicate "old" "new"   NEW v1.3.0 -- copy a sequence to a new name
seq stats               NEW v1.3.0 -- LittleFS / sequences / playlists overview

seq map <button> "name"       Map IR button to sequence
seq map <button> clear
seq map                       Show all mappings
```

## Source and Support

- Repository: https://github.com/PrintedDroid/K-2SO-Animatronic
- Printed Droid builders: https://www.facebook.com/groups/printeddroid/
- Droid Division builders: https://www.facebook.com/groups/2505708886350784/
- Droid Division shop: https://www.droiddivision.com/
