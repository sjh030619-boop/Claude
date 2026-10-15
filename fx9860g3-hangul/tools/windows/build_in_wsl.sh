#!/usr/bin/env bash
# Runs inside WSL Ubuntu: installs fxSDK + gint (first time only) and builds Hangul.g1a.
set -eu   # no pipefail on purpose: `yes | giteapc ...` ends with SIGPIPE (141) once giteapc stops reading

SRC="${1:?usage: build_in_wsl.sh <project dir> [--no-open]}"
OPEN=1; [ "${2:-}" = "--no-open" ] && OPEN=0
WORK="$HOME/hangul-build"
MARK="$HOME/.hangul_toolchain_ok"   # created after the first full setup; delete it to redo setup
export PATH="$HOME/.local/bin:$PATH"

if [ ! -f "$MARK" ]; then

echo "== [1/5] system packages"
sudo apt-get update
sudo apt-get install -y build-essential cmake git python3 python3-pip python3-pil pkg-config \
  rsync texinfo bison flex libgmp-dev libmpfr-dev libmpc-dev libisl-dev libpng-dev \
  libusb-1.0-0-dev libudev-dev libsdl2-dev libncurses-dev libfreetype-dev libssl-dev libudisks2-dev libglib2.0-dev

# The Gitea host moved from gitea.planet-casio.fr to git.planet-casio.com.
git config --global url."https://git.planet-casio.com/".insteadOf "https://gitea.planet-casio.fr/"

echo "== [2/5] GiteaPC"
if ! command -v giteapc >/dev/null 2>&1; then
  rm -rf "$HOME/GiteaPC"
  git clone --depth 1 https://git.planet-casio.com/Lephenixnoir/GiteaPC.git "$HOME/GiteaPC"
  (cd "$HOME/GiteaPC" && make install)
  grep -q '.local/bin' "$HOME/.bashrc" || echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
fi
command -v giteapc >/dev/null 2>&1 || { echo "giteapc not found after install -- check GiteaPC README"; exit 1; }

echo "== [3/5] toolchain (slow the first time)"
command -v sh-elf-gcc >/dev/null 2>&1 || yes | giteapc install Lephenixnoir/sh-elf-gcc
command -v fxsdk      >/dev/null 2>&1 || yes | giteapc install Lephenixnoir/fxsdk
yes | giteapc install Lephenixnoir/OpenLibm Lephenixnoir/gint

touch "$MARK"
else
  echo "== [1-3/5] toolchain already set up ($MARK exists; delete it to redo)"
fi

echo "== [4/5] build"
mkdir -p "$WORK"
rsync -a --delete --exclude dist --exclude build-fx "$SRC/" "$WORK/"
cd "$WORK"
python3 tools/build_assets.py
fxsdk build-fx

echo "== [5/5] copy result"
G1A="$(find "$WORK" -name '*.g1a' -print -quit)"
if [ -z "$G1A" ]; then
  echo "fxsdk build-fx left no .g1a; converting the ELF by hand"
  mkdir -p dist
  sh-elf-objcopy -O binary -R .bss -R .gint_bss build-fx/hangul build-fx/hangul.bin
  fxgxa --g1a build-fx/hangul.bin -n Hangul -i assets-fx/icon.png -o dist/Hangul.g1a
  G1A="$WORK/dist/Hangul.g1a"
fi
echo "found: $G1A"
mkdir -p "$SRC/dist"
cp -v "$G1A" "$SRC/dist/Hangul.g1a"
if [ "$OPEN" = 1 ] && command -v explorer.exe >/dev/null 2>&1; then
  explorer.exe "$(wslpath -w "$SRC/dist")" || true
fi
echo "DONE: $SRC/dist/Hangul.g1a"
