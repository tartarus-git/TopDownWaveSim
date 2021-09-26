#define VALUE_OUTPUT_ROOF 100		// Value at which to call the pixel full and clamp the field value. Use define to avoid function overhead.
#define CLAMP(x) if (x > VALUE_OUTPUT_ROOF) { x = VALUE_OUTPUT_ROOF; } else if (x < -VALUE_OUTPUT_ROOF) { x = -VALUE_OUTPUT_ROOF; }

#define FLOAT_ZERO_WINDOW 0.00001f


#define ZERO(x) if (x >= -FLOAT_ZERO_WINDOW && x <= FLOAT_ZERO_WINDOW) { x = 0; }

__kernel void colorizer(__read_only image2d_t fieldValues, __write_only image2d_t frameOutput, 
						unsigned int windowWidth, 		   unsigned int windowHeight) {
	
	int x = get_global_id(0);													// Standard bounds checking to avoid overflows and such.
	if (x >= windowWidth) { return; }
	int2 coords = (int2)(x, get_global_id(1));

	int2 scaledCoords = (int2)(coords.x * 2, coords.y * 2);						// Get the field coords from the current window coords.
	float topLeft = read_imagef(fieldValues, scaledCoords).x;					// Go through the relevant field coords and clamp them.
	CLAMP(topLeft);
	scaledCoords.x += 1;
	float topRight = read_imagef(fieldValues, scaledCoords).x;
	CLAMP(topRight);
	scaledCoords.y += 1;
	float bottomRight = read_imagef(fieldValues, scaledCoords).x;
	CLAMP(bottomRight);
	scaledCoords.x -= 1;
	float bottomLeft = read_imagef(fieldValues, scaledCoords).x;
	CLAMP(bottomLeft);

	float antialiased = (topLeft + topRight + bottomRight + bottomLeft) / 4;	// Calculate antialiased value from 4 field values.
	antialiased = topLeft;

	if (antialiased < -FLOAT_ZERO_WINDOW) {										// Set color of pixel based on antialiased value.
		//if (antialiased < -100) { antialiased = -100; }
		antialiased /= -100;
		ZERO(antialiased);
		write_imagef(frameOutput, coords, (float4)(0, 0, antialiased, 1));
		return;
	}
	if (antialiased > FLOAT_ZERO_WINDOW) {
		//if (antialiased > 100) { antialiased = 100; }
		antialiased /= 100;
		ZERO(antialiased);
		write_imagef(frameOutput, coords, (float4)(0, antialiased, 0, 1));
		return;
	}
	write_imagef(frameOutput, coords, (float4)(0, 0, 0, 1));
}