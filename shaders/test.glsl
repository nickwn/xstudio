#version 460

vec4 fn2(vec4 a)
{
	return a;
}

void main()
{
	vec4 a = fn2(vec4(1));
}