bmp_common_sources = [
    'budgie-control-bar.c',
    'budgie-media-label.c',
    'budgie-media-view.c',
    'budgie-settings-view.c',
    'budgie-status-area.c',
    'budgie-window.c',
    'main.c',
    'util.c',
    'db/budgie-db.c',
]

bmp_name = 'budgie-media-player'

bmp = executable(
    bmp_name,
    include_directories : configuration_inc,
    sources: bmp_common_sources,
    dependencies: [
        dep_glib,
        dep_mpv,
        dep_gtk3,
        dep_sqlite3,
        dep_taglib,
        dep_gl,
    ],
)
