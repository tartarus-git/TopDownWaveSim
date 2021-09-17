#define NODE_EQUALIZATION_STRENGTH 0.00000001f						// Strength with which nodes are pulled toward each other.
#define FIELD_PULL 0.01f									// Strength at which nodes are pulled back to the field zero level.

// Area that should be influenced through equalization. __constant instead of const because it is better optimized across all platforms.
__constant int influenceAreaX[] = { 1, -1,  0,  0,  1, -1,  1, -1,  0, -2,  2,  0 };
__constant int influenceAreaY[] = { 0,  0, -1,  1, -1, -1,  1,  1, -2,  0,  0,  2 };
#define INFLUENCE_AREA_COUNT (sizeof(influenceAreaX) / sizeof(int))

// Equalization function contains math for the equalization (nodes pulling each other together) of nodes.
float equalize(float thisValue, float otherValue, float dist) {
	return (thisValue - otherValue) * NODE_EQUALIZATION_STRENGTH / dist;
}

// Main kernel takes more parameters than theoretically necessary because of the read and write limitations of OpenCL.
__kernel void wavePropagator(__read_only image2d_t lastFieldValues, __read_only image2d_t lastFieldVels, 
							 __write_only image2d_t fieldValues,	__write_only image2d_t fieldVels, 
							 unsigned int windowWidth, 				unsigned int windowHeight) {

		int x = get_global_id(0);												// Get this work-item's coords and exit early if out of bounds.
		if (x >= windowWidth) { return; }										// Reason for extra instances of kernel: global size must be
		int2 coords = (int2)(x, get_global_id(1));								// multiple of work group size.
		
		float prevValue = read_imagef(lastFieldValues, coords).x;				// Get the previous value and velocity at current position.
		float prevVel = read_imagef(lastFieldVels, coords).x;

		float vel;																// Calculate new velocity by using FIELD_PULL.
		if (prevValue > 0) { vel = prevVel - FIELD_PULL; }
		else if (prevValue < 0) { vel = prevVel + FIELD_PULL; }
		else { vel = prevVel; }

		for (int i = 0; i < INFLUENCE_AREA_COUNT; i++) {						// Calculate equalization for all nodes in influence area.
			int relativeX = influenceAreaX[i];									// Check if the node in question is out of bounds.
			int absoluteX = coords.x + relativeX;
			if (absoluteX >= windowWidth || absoluteX < 0) { continue; }
			int relativeY = influenceAreaY[i];
			int absoluteY = coords.y + relativeY;
			if (absoluteY >= windowHeight || absoluteY < 0) { continue; }

			// Pull this node towards equalization. Pass in absolute coords and distance.
			vel -= equalize(prevValue, read_imagef(lastFieldValues, (int2)(absoluteX, absoluteY)).x, 
					sqrt((float)(relativeX * relativeX + relativeY * relativeY)));
		}

		// (float4)(x, 0, 0, 0) would construct a temporary each time. Instead we make our own temporary and use it twice, better in theory.
		float4 colorHull = (float4)(vel, 0, 0, 0);								// Velocity was already calculated so just put it in here.
		write_imagef(fieldVels, coords, colorHull);
		colorHull.x += prevValue;													// Add prevPos to vel, thereby calculating new position.
		write_imagef(fieldValues, coords, colorHull);
}