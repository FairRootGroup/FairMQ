### `fairmq-top`

`htop`-like TUI to monitor/control a running FairMQ topology.

EXPERIMENTAL - Currently mainly used as internal development tool.

Enable building by passing `-DBUILD_FAIRMQ=ON -DBUILD_SDK_COMMANDS=ON -DBUILD_SDK=ON -DBUILD_DDS_PLUGIN=ON -DBUILD_SDK_TOP_TOOL=ON`.

---

How to build [imtui](https://github.com/ggerganov/imtui) (requires ncurses devel package installed):

```
git clone https://github.com/ggerganov/imtui
cmake -S imtui -B <builddir> -DIMTUI_INSTALL_IMGUI_HEADERS=ON -DIMTUI_SUPPORT_NCURSES=ON -DCMAKE_INSTALL_PREFIX=<installdir>
cmake --build <builddir> --target install
```

and then pass `-Dimtui_ROOT=<installdir>` to the FairMQ configure above.
