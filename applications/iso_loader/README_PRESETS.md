# ISO Loader Presets Packing

## Description

The `pack_presets.sh` script allows you to add new presets to the `presets.tar.gz` archive.

## Usage

### Adding Presets

1. Create a `presets_new` directory next to the Makefile (if it doesn't exist)
2. Copy your `.cfg` files into `presets_new/`
3. Run the packing process using one of the following methods:

#### Via Makefile:
```bash
# Add new and replace old
make pack-presets
# Add new only
make pack-presets-add
```

#### Directly:
```bash
# Add new and replace old
./pack_presets.sh
# Add new only
./pack_presets.sh --no-replace
```

### File Naming

Preset files should be named according to the template:
- `ide_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.cfg` - for IDE devices
- `sd_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.cfg` - for SD devices  
- `cd_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.cfg` - for CD devices

Where `XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX` is a unique identifier - MD5 hash of the first sector of IP.BIN with disc metadata.

### Automatic Copying

The script automatically creates preset versions for all devices:

#### If source preset is for IDE:
- IDE: copied without changes
- CD: copied without changes  
- SD: copied with parameters `dma = 0`, `async = 8`

#### If source preset is for SD:
- SD: copied without changes
- IDE: copied with parameters `dma = 1`, `async = 0`
- CD: copied with parameters `dma = 1`, `async = 0`

#### If source preset is for CD:
- CD: copied without changes
- IDE: copied with parameters `dma = 1`, `async = 0` 
- SD: copied with parameters `dma = 0`, `async = 8`

### Result

After script execution:
- New presets will be added to the `presets.tar.gz` archive
- The `presets_new` directory will be removed
- Temporary files will be cleaned up
- Statistics will show added/replaced/skipped presets count

### Notes

- If a preset with the same name already exists in the archive, it will be overwritten (unless `--no-replace` is used)
- The script automatically determines device type by filename prefix
- Files with unknown prefixes will be skipped with a warning
