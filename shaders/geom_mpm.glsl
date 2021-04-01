#version 450

layout(points) in;
layout(points, max_vertices = 3) out;

layout(set = 1, binding = 0) uniform GridMetrics{
	int z_scale;
} grid_metrics;

void main()
{
	vec4 pos = gl_in[0].gl_Position;
	int point_size = int(gl_in[0].gl_PointSize);
	const int layer_range = (point_size - 1) / 2;
	const int layer_mid = int((pos.z + 1.f) * (grid_metrics.z_scale / 2));
	
	const int layer_low = max(0, layer_mid - layer_range);
	const int layer_high = max(grid_metrics.z_scale - 1, layer_mid + layer_range);

	for (int layer_idx = layer_low; layer_idx < layer_high; layer_idx++)
	{
		gl_Position = vec4(pos.xy, 0.f, 1.f);
		gl_Layer = layer_idx;
		gl_PointSize = point_size;

		EmitVertex();
		EndPrimitive();
	}
}