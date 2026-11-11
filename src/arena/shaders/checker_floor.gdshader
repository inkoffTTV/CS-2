shader_type spatial;
// World-aligned checker floor: 4 m tiles make distance and speed readable. Thin grout lines between tiles.

uniform vec3 color_a : source_color = vec3(0.60, 0.55, 0.47);
uniform vec3 color_b : source_color = vec3(0.45, 0.41, 0.36);
uniform vec3 grout : source_color = vec3(0.36, 0.34, 0.32);
uniform float tile_size = 4.0;
uniform float grout_width = 0.04;
uniform float roughness_value = 0.85;

varying vec3 world_pos;

void vertex() {
	world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
}

void fragment() {
	vec2 p = world_pos.xz / tile_size;
	vec2 cell = floor(p);
	float checker = mod(cell.x + cell.y, 2.0);
	vec3 col = mix(color_a, color_b, checker);
	// Subtle per-tile tint variation so the floor does not look printed.
	float n = fract(sin(dot(cell, vec2(12.9898, 78.233))) * 43758.5453);
	col *= 0.94 + 0.08 * n;
	vec2 f = fract(p) * tile_size;
	float edge = min(min(f.x, tile_size - f.x), min(f.y, tile_size - f.y));
	float line = 1.0 - smoothstep(grout_width * 0.5, grout_width, edge);
	ALBEDO = mix(col, grout, line);
	ROUGHNESS = mix(roughness_value, 1.0, line);
	METALLIC = 0.0;
}
