#!/usr/bin/env bash
# virtuoso-pipewire-sink.sh
# Installs the Virtuoso Audio virtual 7.1 sink in PipeWire (primary) or
# PulseAudio (fallback) and registers it to auto-start on login.
#
# Usage:
#   ./virtuoso-pipewire-sink.sh install    # Create virtual sink
#   ./virtuoso-pipewire-sink.sh remove     # Remove virtual sink
#   ./virtuoso-pipewire-sink.sh status     # Check if sink is present

set -euo pipefail

SINK_NAME="VirtuosoVirtual71"
SINK_DESCRIPTION="Virtuoso Virtual 7.1"
RATE=48000
CHANNELS=8
# 7.1 surround channel map (PipeWire / PulseAudio format)
CHANNEL_MAP="front-left,front-right,front-center,lfe,rear-left,rear-right,side-left,side-right"
PIPEWIRE_CONFIG_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/pipewire"
AUTOSTART_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/autostart"

# -----------------------------------------------------------
# Detect runtime: PipeWire vs PulseAudio
# -----------------------------------------------------------
detect_runtime() {
    if command -v pw-cli &>/dev/null && pw-cli info 0 &>/dev/null 2>&1; then
        echo "pipewire"
    elif command -v pactl &>/dev/null; then
        echo "pulseaudio"
    else
        echo "none"
    fi
}

# -----------------------------------------------------------
# PipeWire: create virtual sink via pw-loopback
# -----------------------------------------------------------
pipewire_install() {
    mkdir -p "$PIPEWIRE_CONFIG_DIR/filter-chain.conf.d"
    cat > "$PIPEWIRE_CONFIG_DIR/filter-chain.conf.d/virtuoso-virtual-71.conf" << EOF
context.modules = [
  {
    name = libpipewire-module-null-sink
    args = {
      node.name        = "${SINK_NAME}"
      node.description = "${SINK_DESCRIPTION}"
      audio.rate       = ${RATE}
      audio.channels   = ${CHANNELS}
      audio.position   = [ FL FR FC LFE RL RR SL SR ]
    }
  }
]
EOF
    echo "[Virtuoso] PipeWire virtual sink config installed."
    echo "[Virtuoso] Run 'systemctl --user restart pipewire' to activate."
}

pipewire_remove() {
    rm -f "$PIPEWIRE_CONFIG_DIR/filter-chain.conf.d/virtuoso-virtual-71.conf"
    echo "[Virtuoso] PipeWire virtual sink config removed."
    echo "[Virtuoso] Run 'systemctl --user restart pipewire' to deactivate."
}

pipewire_status() {
    if pw-cli list-objects Node 2>/dev/null | grep -q "${SINK_NAME}"; then
        echo "[Virtuoso] PipeWire virtual sink '${SINK_NAME}' is ACTIVE."
        return 0
    else
        echo "[Virtuoso] PipeWire virtual sink '${SINK_NAME}' is NOT active."
        return 1
    fi
}

# -----------------------------------------------------------
# PulseAudio fallback: module-null-sink
# -----------------------------------------------------------
pulseaudio_install() {
    pactl load-module module-null-sink \
        sink_name="${SINK_NAME}" \
        sink_properties="device.description='${SINK_DESCRIPTION}'" \
        rate=${RATE} \
        channels=${CHANNELS} \
        channel_map=${CHANNEL_MAP}
    echo "[Virtuoso] PulseAudio virtual sink loaded (session only — add to /etc/pulse/default.pa for persistence)."
}

pulseaudio_remove() {
    pactl unload-module module-null-sink 2>/dev/null || true
    echo "[Virtuoso] PulseAudio virtual sink unloaded."
}

# -----------------------------------------------------------
# Main
# -----------------------------------------------------------
RUNTIME=$(detect_runtime)
COMMAND="${1:-status}"

case "$COMMAND" in
    install)
        case "$RUNTIME" in
            pipewire)   pipewire_install ;;
            pulseaudio) pulseaudio_install ;;
            none)       echo "ERROR: Neither PipeWire nor PulseAudio found." >&2; exit 1 ;;
        esac
        ;;
    remove)
        case "$RUNTIME" in
            pipewire)   pipewire_remove ;;
            pulseaudio) pulseaudio_remove ;;
            none)       echo "No audio runtime found." ;;
        esac
        ;;
    status)
        case "$RUNTIME" in
            pipewire)   pipewire_status ;;
            *) pactl list sinks 2>/dev/null | grep -q "${SINK_NAME}" && echo "ACTIVE" || echo "NOT ACTIVE" ;;
        esac
        ;;
    *)
        echo "Usage: $0 {install|remove|status}"
        exit 1
        ;;
esac
