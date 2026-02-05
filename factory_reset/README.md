Factory Reset – Short Documentation

This program manages camera configuration resets.

All configuration files are stored in:
- /mnt/flash/vienna/m5s_config/


The program can save these files as defaults and later restore them.

Commands:
- factory_reset_bin capture_defaults
Saves all current config files into a snapshot file
/mnt/flash/vienna/default_snapshot.txt
(Creates key=value entries for every config file.)

- factory_reset_bin factory
Restores all config files to the values saved in the snapshot file.

- factory_reset_bin config_reset
Restores only Wi-Fi related configuration files using the snapshot values.

Flow Summary:
1. capture_defaults → save current settings
2. factory → restore all settings
3. config_reset → restore Wi-Fi settings only

The snapshot must be created once before resets can be used.
