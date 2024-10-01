# -*- coding: utf-8 -*-

# Example: https://dmgbuild.readthedocs.io/en/latest/example.html

from __future__ import unicode_literals

import os.path

app = defines.get("app")
app_basename = os.path.basename(app)
format = defines.get("format", "UDBZ")
size = defines.get("size", None)
files = [app]
symlinks = {"Applications": "/Applications"}
icon = os.path.join(app, "Contents/Resources/Volume.icns")
icon_locations = {
    app_basename: (144, 190),
    "Applications": (455, 190),
}
background = os.path.join(app, "Contents/Resources/Background.tiff")
show_status_bar = False
show_tab_view = False
show_toolbar = False
show_pathbar = False
show_sidebar = False
sidebar_width = 180
window_rect = ((200, 120), (620, 420))
default_view = "icon-view"
show_icon_preview = False
include_icon_view_settings = "auto"
include_list_view_settings = "auto"
arrange_by = None
grid_offset = (0, 0)
grid_spacing = 100
scroll_position = (0, 0)
label_pos = "bottom"
text_size = 16
icon_size = 100
list_icon_size = 16
list_text_size = 12
list_scroll_position = (0, 0)
list_sort_by = "name"
list_use_relative_dates = True
list_calculate_all_sizes = (False,)
list_columns = ("name", "date-modified", "size", "kind", "date-added")
list_column_widths = {
    "name": 300,
    "date-modified": 181,
    "date-created": 181,
    "date-added": 181,
    "date-last-opened": 181,
    "size": 97,
    "kind": 115,
    "label": 100,
    "version": 75,
    "comments": 300,
}
list_column_sort_directions = {
    "name": "ascending",
    "date-modified": "descending",
    "date-created": "descending",
    "date-added": "descending",
    "date-last-opened": "descending",
    "size": "descending",
    "kind": "ascending",
    "label": "ascending",
    "version": "ascending",
    "comments": "ascending",
}
