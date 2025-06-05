#!/bin/bash
#
# DreamShell IP.BIN updater
# Copyright (C) 2025 SWAT
# http://www.dc-swat.ru
#
# This script is used to update the IP.BIN file with the current version and date.
#

if [ $# -lt 6 ] || [ $# -gt 7 ]; then
    echo "Usage: $0 <source_ip_bin> <target_ip_bin> <major> <minor> <micro> <build_type> [type]"
    echo "Example: $0 resources/IP.BIN build/IP.BIN 4 0 2 30"
    echo "         $0 resources/IP.BIN build/IP.BIN 2 8 0 30 bootloader"
    exit 1
fi

SOURCE_IP="$1"
TARGET_IP="$2"
VER_MAJOR="$3"
VER_MINOR="$4"
VER_MICRO="$5"
BUILD_TYPE="$6"
IP_TYPE="${7:-main}"

if [ ! -f "$SOURCE_IP" ]; then
    echo "Error: Source file $SOURCE_IP not found"
    exit 1
fi

# Create target directory if needed
TARGET_DIR="$(dirname "$TARGET_IP")"
if [ "$TARGET_DIR" != "." ] && [ ! -d "$TARGET_DIR" ]; then
    mkdir -p "$TARGET_DIR"
fi
cp "$SOURCE_IP" "$TARGET_IP"

BUILD_TYPE_STR=""
case "$BUILD_TYPE" in
    0x0*) BUILD_TYPE_STR="ALP" ;;
    0x1*) BUILD_TYPE_STR="BET" ;;
    0x2*) BUILD_TYPE_STR="RC " ;;
    0x3*) BUILD_TYPE_STR="REL" ;;
    *) BUILD_TYPE_STR="REL" ;;
esac

CURRENT_DATE=$(date +%Y%m%d)

# Create version string with null byte separator
if [ "$IP_TYPE" = "bootloader" ]; then
    VERSION_FIELD="DS-BOOTLD"
    BIN_NAME_FIELD="1DS_BOOT.BIN    "
    DESC_FIELD="Bootloader ${VER_MAJOR}.${VER_MINOR}"
else
    VERSION_FIELD="DS-${VER_MAJOR}${VER_MINOR}${VER_MICRO}${BUILD_TYPE_STR}"
    BIN_NAME_FIELD="1DS_CORE.BIN    "
    DESC_FIELD="DreamShell ${VER_MAJOR}.${VER_MINOR}"
fi

# Create version string with null byte separator using printf directly
VERSION_PART2="V${VER_MAJOR}.${VER_MINOR}${VER_MICRO}0"

# Write version field (0x40-0x4F) with null byte separator
printf "${VERSION_FIELD}\x00${VERSION_PART2}%*s" $((16 - ${#VERSION_FIELD} - 1 - ${#VERSION_PART2})) "" | \
    dd of="$TARGET_IP" bs=1 seek=64 count=16 conv=notrunc 2>/dev/null
# Write other fields
printf "%-16.16s" "$CURRENT_DATE" | dd of="$TARGET_IP" bs=1 seek=80 count=16 conv=notrunc 2>/dev/null  
printf "%-16.16s" "$BIN_NAME_FIELD" | dd of="$TARGET_IP" bs=1 seek=96 count=16 conv=notrunc 2>/dev/null
printf "%-16.16s" "$DESC_FIELD" | dd of="$TARGET_IP" bs=1 seek=128 count=16 conv=notrunc 2>/dev/null

if [ "$IP_TYPE" = "bootloader" ]; then
    VERSION_STRING="DS-BOOTLD.V${VER_MAJOR}.${VER_MINOR}${VER_MICRO}0"
    BIN_NAME="1DS_BOOT.BIN"
    DESCRIPTION_STRING="Bootloader ${VER_MAJOR}.${VER_MINOR}"
else
    VERSION_STRING="DS-${VER_MAJOR}${VER_MINOR}${VER_MICRO}${BUILD_TYPE_STR}.V${VER_MAJOR}.${VER_MINOR}${VER_MICRO}0"
    BIN_NAME="1DS_CORE.BIN"
    DESCRIPTION_STRING="DreamShell ${VER_MAJOR}.${VER_MINOR}"
fi

echo "Updated IP.BIN:"
echo "  Type: $IP_TYPE"
echo "  Version: $VERSION_STRING"
echo "  Date: $CURRENT_DATE"
echo "  Binary: $BIN_NAME"
echo "  Description: $DESCRIPTION_STRING"
echo "  Target: $TARGET_IP"
