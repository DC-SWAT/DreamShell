	.section .rodata
	.align 2
	.global _scroll_near_rgb
_scroll_near_rgb:
	.incbin "scroll_near.rgb"

	.global _scroll_sky_rgb
_scroll_sky_rgb:
	.incbin "scroll_sky.rgb"

	.global _scroll_mid_rgb
_scroll_mid_rgb:
	.incbin "scroll_mid.rgb"

	.global _scroll_cloud_rgb
_scroll_cloud_rgb:
	.incbin "scroll_cloud.rgb"

	.global _scroll_floor_rgb
_scroll_floor_rgb:
	.incbin "scroll_floor.rgb"

	.global _scroll_vert_rgb
_scroll_vert_rgb:
	.incbin "scroll_vert.rgb"
