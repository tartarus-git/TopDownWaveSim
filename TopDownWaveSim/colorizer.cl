__kernel void colorizer(__read_only image2d_t fieldValues, __write_only image2d_t frameOutput, 
						unsigned int windowWidth, 		   unsigned int windowHeight) {
	
	int x = get_global_id(0);															// Standard bounds checking to avoid buffer overflow.
	if (x >= windowWidth) { return; }
	int2 coords = (int2)(x, get_global_id(1));

	int2 scaledCoords = (int2)(coords.x * 2, coords.y * 2);								// Get the field coords from the current window coords.
	float antialiased = read_imagef(fieldValues, scaledCoords).x;						// Use the relevant values to anti-alias.
	scaledCoords.x += 1;
	antialiased += read_imagef(fieldValues, scaledCoords).x;
	scaledCoords.y += 1;
	antialiased += read_imagef(fieldValues, scaledCoords).x;
	scaledCoords.x -= 1;
	antialiased += read_imagef(fieldValues, scaledCoords).x;
	antialiased /= 4;

	// Convert antialiased field value into displayable pixel value with the right color.
	if (antialiased > 0) { write_imageui(frameOutput, coords, (uint4)(0, (uint)(round(fmin(antialiased, 1) * 255)), 0, 0)); return; }
	write_imageui(frameOutput, coords, (uint4)(0, 0, (uint)(round(fmin(-antialiased, 1) * 255)), 0));
}