bmp_common_sources = [
    'budgie-control-bar.c'
    'budgie-media-label.c'
    'budgie-settings-view.c'
    'budgie-status-area.c'
    'budgie-window.c'
    'main.c'
    'util.c'
    'db/budgie-db.c'
]

bmp_name = 'budgie-media-player'

bmp = executable(
    bmp_name,
    sources: bmp_common_sources
    dependencies: [
        dep_gtk3,
        dep_sqlite3,
    ],
    install: true,
)
