#define NODE_EQUALIZATION_STRENGTH 0.15f						// Strength with which nodes are pulled toward each other.
#define FIELD_PULL 0.1f									// Strength at which nodes are pulled back to the field zero level.

#define SQRT_2 1.41421356237f								// These defines optimize the kernel by storing precalculated square roots.
#define SQRT_5 2.2360679775f								// These are used while handling the influence area.
#define SQRT_8 2.82842712475f
#define SQRT_10 3.16227766017f

// Area that should be influenced through equalization. __constant instead of const because it is better optimized across all platforms.
__constant int influenceAreaX[] = { 1, -1,  0,  0,  1, -1,  1, -1,  0, -2,  2,  0, -1,  1, -1,  1,  2,  2, -2, -2, 
2,  2, -2, -2,  3,  3,  3, -3, -3, -3,  0,  1, -1,  0,  1, -1 };
__constant int influenceAreaY[] = { 0,  0, -1,  1, -1, -1,  1,  1, -2,  0,  0,  2, -2, -2,  2,  2, -1,  1, -1,  1, 
2, -2,  2, -2,  0,  1, -1,  0,  1, -1, -3, -3, -3,  3,  3,  3 };

// Precalculated distances (square roots) are stored with as constants so the kernel can avoid unnecessary computation.
__constant float influenceAreaDists[] = { 1, 1, 1, 1, SQRT_2, SQRT_2, SQRT_2, SQRT_2, 2, 2, 2, 2, SQRT_5, SQRT_5, SQRT_5, SQRT_5, SQRT_5, 
SQRT_5, SQRT_5, SQRT_5, SQRT_8, SQRT_8, SQRT_8, SQRT_8, 3, SQRT_10, SQRT_10, 3, SQRT_10, SQRT_10, 3, SQRT_10, SQRT_10, 3, SQRT_10, SQRT_10 };

#define INFLUENCE_AREA_COUNT (sizeof(influenceAreaX) / sizeof(int))

// Equalization function contains math for the equalization (nodes pulling each other together) of nodes.
float equalize(float thisValue, float otherValue, float dist) {
	return (thisValue - otherValue) * NODE_EQUALIZATION_STRENGTH / dist;
}

// Main kernel takes more parameters than theoretically necessary because of the read and write limitations of OpenCL.
__kernel void wavePropagator(__read_only image2d_t lastFieldValues, __read_only image2d_t lastFieldVels, 
							 __write_only image2d_t fieldValues,	__write_only image2d_t fieldVels, 
							 unsigned int fieldWidth, 				unsigned int fieldHeight) {

	int x = get_global_id(0);												// Get this work-item's coords and exit early if out of bounds.
	if (x >= fieldWidth) { return; }										// Reason for extra instances of kernel: global size must be
	int2 coords = (int2)(x, get_global_id(1));								// multiple of work group size.

	float prevValue = read_imagef(lastFieldValues, coords).x;				// Get the previous value and velocity at current position.
	float prevVel = read_imagef(lastFieldVels, coords).x;

	float vel;																// Calculate new velocity by using FIELD_PULL.
	if (prevValue > 0) { vel = prevVel - FIELD_PULL; }		// Use FLOAT_ZERO_WINDOW to do comparison against 0.
	else if (prevValue < 0) { vel = prevVel + FIELD_PULL; }
	else { vel = prevVel; }

		for (int i = 0; i < INFLUENCE_AREA_COUNT; i++) {						// Calculate equalization for all nodes in influence area.
			int relativeX = influenceAreaX[i];									// Check if the node in question is out of bounds.
			int absoluteX = coords.x + relativeX;
			if (absoluteX >= fieldWidth || absoluteX < 0) { continue; }
			int relativeY = influenceAreaY[i];
			int absoluteY = coords.y + relativeY;
			if (absoluteY >= fieldHeight || absoluteY < 0) { continue; }

			// Pull this node towards equalization. Pass in values of the two nodes and distance between them.
			float thing = read_imagef(lastFieldValues, (int2)(absoluteX, absoluteY)).x;
			float thing2 = equalize(prevValue, thing, influenceAreaDists[i]);
			vel -= thing2;
		}

		// TODO: See if adding friction reduces artifacts. You'll want to do that anyway to make it more usable.

		// (float4)(x, 0, 0, 0) would construct a temporary each time. Instead we make our own temporary and use it twice, better in theory.
		float4 colorHull = (float4)(vel, 0, 0, 0);								// Velocity was already calculated so just put it in here.
		write_imagef(fieldVels, coords, colorHull);

		colorHull.x += prevValue;												// Add prevValue to vel, thereby calculating new value.
		write_imagef(fieldValues, coords, colorHull);
}