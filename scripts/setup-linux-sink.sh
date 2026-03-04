#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# scripts/setup-linux-sink.sh
# Installs the Virtuoso virtual 7.1 PipeWire null-sink (Linux).
# Run as the current user (not root) — PipeWire loads per-user modules.
#
# Usage:
#   chmod +x scripts/setup-linux-sink.sh
#   ./scripts/setup-linux-sink.sh [--remove]

set -euo pipefail

SINK_NAME="VirtuosoVirtual71"
SINK_DESC="Virtuoso Virtual 7.1"
MODULE_CONF="$HOME/.config/pipewire/pipewire.conf.d/99-virtuoso-null-sink.conf"

remove_sink() {
    echo "[Virtuoso] Removing PipeWire null-sink..."
    rm -f "$MODULE_CONF"
    systemctl --user restart pipewire pipewire-pulse 2>/dev/null || true
    echo "[Virtuoso] Null-sink removed. Restart PipeWire if the device is still visible."
    exit 0
}

# Parse args
for arg in "$@"; do
    [[ "$arg" == "--remove" ]] && remove_sink
done

echo "[Virtuoso] Setting up PipeWire null-sink: $SINK_NAME"

# Create config directory
mkdir -p "$(dirname "$MODULE_CONF")"

# Write PipeWire module configuration for a 7.1 null-sink
cat > "$MODULE_CONF" <<EOF
# Virtuoso Audio — Virtual 7.1 null-sink
# This creates a persistent virtual audio device that appears in
# GNOME Sound Settings / KDE Audio settings as "$SINK_DESC".
context.modules = [
    {
        name = libpipewire-module-null-sink
        args = {
            audio.channels    = 8
            audio.position    = [ FL FR FC LFE RL RR SL SR ]
            node.name         = "$SINK_NAME"
            node.description  = "$SINK_DESC"
            media.class       = "Audio/Sink"
            audio.rate        = 48000
            audio.format      = "F32"
        }
    }
]
EOF

echo "[Virtuoso] Configuration written to: $MODULE_CONF"

# Also configure PulseAudio fallback (for PulseAudio-only systems)
PULSE_CONF="$HOME/.config/pulse/default.pa.d/virtuoso-sink.pa"
mkdir -p "$(dirname "$PULSE_CONF")"
cat > "$PULSE_CONF" <<EOF
# Virtuoso Audio — PulseAudio null sink (fallback for non-PipeWire systems)
load-module module-null-sink \
    sink_name=$SINK_NAME \
    sink_properties=device.description="$SINK_DESC" \
    format=float32le \
    rate=48000 \
    channels=8 \
    channel_map=front-left,front-right,front-center,lfe,rear-left,rear-right,side-left,side-right
EOF

echo "[Virtuoso] PulseAudio fallback config written"

# Restart audio subsystem
if systemctl --user is-active pipewire &>/dev/null; then
    echo "[Virtuoso] Restarting PipeWire..."
    systemctl --user restart pipewire pipewire-pulse
    sleep 1
    # Verify the sink is present
    if pw-dump 2>/dev/null | grep -q "$SINK_NAME"; then
        echo "[Virtuoso] ✓ $SINK_DESC is now available in your audio settings."
    else
        echo "[Virtuoso] ⚠ PipeWire restarted but sink not yet visible. Try logging out and back in."
    fi
elif pulseaudio --check 2>/dev/null; then
    echo "[Virtuoso] Reloading PulseAudio..."
    pulseaudio -k && pulseaudio --start
    echo "[Virtuoso] ✓ $SINK_DESC set up via PulseAudio."
else
    echo "[Virtuoso] ⚠ Neither PipeWire nor PulseAudio found running. Install pipewire or pulseaudio."
    exit 1
fi
