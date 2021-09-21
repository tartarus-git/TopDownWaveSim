__kernel void colorizer(__read_only image2d_t fieldValues, __write_only image2d_t frameOutput, 
unsigned int windowWidth, unsigned int windowHeight) {
	int x = get_global_id(0);
	if (x >= windowWidth) { return; }
	int2 coords = (int2)(x, get_global_id(1));

	int2 scaledCoords = (int2)(coords.x * 2, coords.y * 2);
	float value1 = read_imagef(fieldValues, scaledCoords).x;
	scaledCoords.x += 1;
	float value2 = read_imagef(fieldValues, scaledCoords).x;
	scaledCoords.y += 1;
	float value3 = read_imagef(fieldValues, scaledCoords).x;
	scaledCoords.x -= 1;
	float value4 = read_imagef(fieldValues, scaledCoords).x;

	float antialiased = (value1 + value2 + value3 + value4) / 4;

	// TODO: Should we be averaging before clamping. Would it not be better to average after clamping to 100? That way you're actually averaging pixels and not wave values.

	if (antialiased < 0) {
		antialiased = -antialiased;
		if (antialiased > 100) { antialiased = 100; }
		antialiased /= 100;
		write_imagef(frameOutput, coords, (float4)(0, 0, antialiased, 1));
		return;
	} else if (antialiased > 0) {
		if (antialiased > 100) { antialiased = 100; }
		antialiased /= 100;
		write_imagef(frameOutput, coords, (float4)(0, antialiased, 0, 1));
		return;
	}
	write_imagef(frameOutput, coords, (float4)(0, 0, 0, 1));
}