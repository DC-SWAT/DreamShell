-- Launch App configuration.
-- Must return a table. Missing fields fall back to built-in defaults.
--
-- Background fog layers are defined in app.xml as circle elements named
-- fog_circle_* (up to 8 layers).
-- Focus glow is defined in app.xml as circle element focus_glow.

return {
	-- Icon grid
	grid_cols = 4,
	grid_rows = 4,
	cell_pad_x = 30,
	cell_pad_y = 18,
	grid_top = 14,
	grid_bottom = 10,

	-- Labels
	icon_label_size = 8,
	icon_label_y_offset = 6.0,
	icon_highlight_padding = 6,

	-- Page slide animation, ms
	page_slide_ms = 320,

	-- Idle icon sway
	sway_amp_y = 1.0,
	sway_amp_x = 0.5,
	sway_rot_deg = 0.6,

	-- Focused icon sway
	focus_sway_amp_y = 4.2,
	focus_sway_amp_x = 2.1,
	focus_sway_speed = 4.3,
	focus_rot_deg = 12.0,
	focus_rot_freq_mul = 0.82,

	-- Focus navigation
	col_match_eps = 4.0,

	-- Focused icon label color
	focus_label_color = "#FFBE0B",

	-- Focus glow pulse (color and base size in app.xml focus_glow circle)
	glow_alpha_min = 0.14,
	glow_alpha_max = 0.42,
	glow_radius_mul = 0.50,
	glow_radius_pulse = 10.0,
}
