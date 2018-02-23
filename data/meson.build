icons = [
    'budgie.svg',
]

# Install our icons
iconsdir = join_paths(path_datadir, 'icons', 'hicolor', 'scalable', 'actions')
install_data(
    icons,
    install_dir: iconsdir,
)

gschemadir = join_paths(path_datadir, 'glib-2.0', 'schemas')
install_data(
    'com.evolve-os.budgie.media-player.gschema.xml',
    install_dir: gschemadir,
)
