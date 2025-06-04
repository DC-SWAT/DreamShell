#!/bin/bash
#
# DreamShell ISO Loader presets packer
# Copyright (C) 2025 SWAT
# http://www.dc-swat.ru
#
# This script is used to pack new presets into a tar.gz archive.
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRESETS_NEW_DIR="$SCRIPT_DIR/presets_new"
PRESETS_DIR="$SCRIPT_DIR/presets"
ARCHIVE_FILE="$SCRIPT_DIR/presets.tar.gz"

NO_REPLACE=false
IDE_ADDED=0
IDE_REPLACED=0
IDE_SKIPPED=0
SD_ADDED=0
SD_REPLACED=0
SD_SKIPPED=0
CD_ADDED=0
CD_REPLACED=0
CD_SKIPPED=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --no-replace)
            NO_REPLACE=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [--no-replace] [--help]"
            echo "  --no-replace    Skip existing presets instead of replacing them"
            echo "  --help, -h      Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

check_preset_device() {
    local filename="$1"
    if [[ "$filename" == ide_* ]]; then
        echo "ide"
    elif [[ "$filename" == sd_* ]]; then
        echo "sd"
    elif [[ "$filename" == cd_* ]]; then
        echo "cd"
    else
        echo "unknown"
    fi
}

create_preset_if_not_exists() {
    local source_file="$1"
    local base_name="$2"
    local target_device="$3"
    local source_device="$4"
    local modify_params="$5"

    local target_filename="${target_device}_${base_name}"
    local target_path="$PRESETS_DIR/$target_device/$target_filename"

    if [[ "$modify_params" == "true" ]]; then
        modify_preset_params "$source_file" "$target_path" "$target_device" "$source_device"
    else
        copy_preset_with_stats "$source_file" "$target_path" "$target_device"
    fi
}

copy_preset_with_stats() {
    local source_file="$1"
    local dest_file="$2"
    local device="$3"

    if [[ -f "$dest_file" ]]; then
        if [[ "$NO_REPLACE" == true ]]; then
            case "$device" in
                "ide") ((IDE_SKIPPED++)) ;;
                "sd") ((SD_SKIPPED++)) ;;
                "cd") ((CD_SKIPPED++)) ;;
            esac
            return 0
        else
            case "$device" in
                "ide") ((IDE_REPLACED++)) ;;
                "sd") ((SD_REPLACED++)) ;;
                "cd") ((CD_REPLACED++)) ;;
            esac
        fi
    else
        case "$device" in
            "ide") ((IDE_ADDED++)) ;;
            "sd") ((SD_ADDED++)) ;;
            "cd") ((CD_ADDED++)) ;;
        esac
    fi

    cp "$source_file" "$dest_file"
}

modify_preset_params() {
    local input_file="$1"
    local output_file="$2"
    local target_device="$3"
    local source_device="$4"

    copy_preset_with_stats "$input_file" "$output_file" "$target_device"

    if [[ "$NO_REPLACE" == true ]] && [[ -f "$output_file" ]]; then
        return 0
    fi

    if [[ "$source_device" == "ide" || "$source_device" == "cd" ]] && [[ "$target_device" == "sd" ]]; then
        local temp_file="${output_file}.tmp"
        > "$temp_file"
        while IFS= read -r line; do
            if [[ "$line" =~ ^dma\ =\ [0-9]* ]]; then
                echo "dma = 0" >> "$temp_file"
            elif [[ "$line" =~ ^async\ =\ [0-9]* ]]; then
                echo "async = 8" >> "$temp_file"
            else
                echo "$line" >> "$temp_file"
            fi
        done < "$output_file"
        mv "$temp_file" "$output_file"

    elif [[ "$source_device" == "sd" ]] && [[ "$target_device" == "ide" || "$target_device" == "cd" ]]; then
        local temp_file="${output_file}.tmp"
        > "$temp_file"
        while IFS= read -r line; do
            if [[ "$line" =~ ^dma\ =\ [0-9]* ]]; then
                echo "dma = 1" >> "$temp_file"
            elif [[ "$line" =~ ^async\ =\ [0-9]* ]]; then
                echo "async = 0" >> "$temp_file"
            else
                echo "$line" >> "$temp_file"
            fi
        done < "$output_file"
        mv "$temp_file" "$output_file"
    fi
}

echo "Checking for new presets in $PRESETS_NEW_DIR..."

if [[ ! -d "$PRESETS_NEW_DIR" ]]; then
    echo "Directory $PRESETS_NEW_DIR does not exist. Creating it..."
    mkdir -p "$PRESETS_NEW_DIR"
    echo "Please add your preset files to $PRESETS_NEW_DIR and run this script again."
    exit 0
fi

if [[ ! "$(ls -A "$PRESETS_NEW_DIR")" ]]; then
    echo "No preset files found in $PRESETS_NEW_DIR"
    exit 0
fi

if [[ -f "$ARCHIVE_FILE" ]]; then
    echo "Extracting existing presets..."
    tar -xzf "$ARCHIVE_FILE"
fi

if [[ ! -d "$PRESETS_DIR" ]]; then
    echo "Creating presets directories..."
    mkdir -p "$PRESETS_DIR/ide" "$PRESETS_DIR/sd" "$PRESETS_DIR/cd"
fi

# Build list of all preset files for priority checking
preset_devices_list=""
for preset_file in "$PRESETS_NEW_DIR"/*.cfg; do
    if [[ ! -f "$preset_file" ]]; then
        continue
    fi
    filename=$(basename "$preset_file")
    device=$(check_preset_device "$filename")
    if [[ "$device" != "unknown" ]]; then
        base_name="${filename#*_}"
        preset_devices_list="$preset_devices_list|$base_name:$device"
    fi
done

get_available_devices() {
    local base_name="$1"
    local result=""
    local entry
    
    for entry in $(echo "$preset_devices_list" | tr '|' '\n'); do
        if [[ "$entry" == "$base_name:"* ]]; then
            device_name="${entry#*:}"
            result="$result $device_name"
        fi
    done
    echo "$result"
}

for preset_file in "$PRESETS_NEW_DIR"/*.cfg; do
    if [[ ! -f "$preset_file" ]]; then
        continue
    fi

    filename=$(basename "$preset_file")
    title=""
    while IFS= read -r line; do
        if [[ "$line" =~ ^title\ =\ (.*)$ ]]; then
            title="${BASH_REMATCH[1]}"
            break
        fi
    done < "$preset_file"
    if [[ -z "$title" ]]; then
        title="no title"
    fi
    echo "Processing $filename $title"

    device=$(check_preset_device "$filename")

    if [[ "$device" == "unknown" ]]; then
        echo "Warning: Cannot determine device type for $filename. Skipping."
        continue
    fi

    base_name="${filename#*_}"
    available_devices=$(get_available_devices "$base_name")
    
    if [[ "$device" == "ide" ]]; then
        copy_preset_with_stats "$preset_file" "$PRESETS_DIR/ide/$filename" "ide"

        # Create CD only if no CD-specific preset exists
        if [[ ! "$available_devices" =~ "cd" ]]; then
            create_preset_if_not_exists "$preset_file" "$base_name" "cd" "ide" "false"
        fi

        # Create SD only if no SD-specific preset exists
        if [[ ! "$available_devices" =~ "sd" ]]; then
            create_preset_if_not_exists "$preset_file" "$base_name" "sd" "ide" "true"
        fi
        
    elif [[ "$device" == "sd" ]]; then
        copy_preset_with_stats "$preset_file" "$PRESETS_DIR/sd/$filename" "sd"

        # Create IDE only if no IDE-specific preset exists
        if [[ ! "$available_devices" =~ "ide" ]]; then
            create_preset_if_not_exists "$preset_file" "$base_name" "ide" "sd" "true"
        fi

        # Create CD only if no CD-specific and no IDE-specific preset exists
        if [[ ! "$available_devices" =~ "cd" ]] && [[ ! "$available_devices" =~ "ide" ]]; then
            create_preset_if_not_exists "$preset_file" "$base_name" "cd" "sd" "true"
        fi

    elif [[ "$device" == "cd" ]]; then
        copy_preset_with_stats "$preset_file" "$PRESETS_DIR/cd/$filename" "cd"

        # Create IDE only if no IDE-specific and no SD-specific preset exists
        if [[ ! "$available_devices" =~ "ide" ]] && [[ ! "$available_devices" =~ "sd" ]]; then
            create_preset_if_not_exists "$preset_file" "$base_name" "ide" "cd" "true"
        fi

        # Create SD only if no SD-specific and no IDE-specific preset exists  
        if [[ ! "$available_devices" =~ "sd" ]] && [[ ! "$available_devices" =~ "ide" ]]; then
            create_preset_if_not_exists "$preset_file" "$base_name" "sd" "cd" "true"
        fi
    fi
done

echo "Creating new archive..."
tar -czf "$ARCHIVE_FILE" presets/

echo "Cleaning up..."
rm -rf "$PRESETS_DIR"
rm -rf "$PRESETS_NEW_DIR"

echo ""
if [[ "$NO_REPLACE" == true ]]; then
    echo "IDE: $IDE_ADDED added, $IDE_SKIPPED skipped"
    echo "SD:  $SD_ADDED added, $SD_SKIPPED skipped"
    echo "CD:  $CD_ADDED added, $CD_SKIPPED skipped"
else
    echo "IDE: $IDE_ADDED added, $IDE_REPLACED replaced"
    echo "SD:  $SD_ADDED added, $SD_REPLACED replaced"
    echo "CD:  $CD_ADDED added, $CD_REPLACED replaced"
fi
echo ""
echo "Successfully packed new presets into $ARCHIVE_FILE"
