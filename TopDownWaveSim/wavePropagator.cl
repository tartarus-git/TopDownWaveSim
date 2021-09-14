#define NODE_EQUALIZATION_STRENGTH 0.1f						// Strength with which nodes are pulled toward each other.
#define FIELD_PULL 0.01f									// Strength at which nodes are pulled back to the field zero level.

#define TO_COLOR(x) ((float4)(x, 0, 0, 0))					// Converter define because OpenCL expects the value in the red channel.

// Equalization function contains math for the equalization (nodes pulling each other together) of nodes.
float equalize(float thisValue, float otherValue) {
	return (thisValue - otherValue) * NODE_EQUALIZATION_STRENGTH;
}

// Main kernel takes more parameters than theoretically necessary because of the read and write limitations of OpenCL.
__kernel void wavePropagator(__read_only image2d_t lastFieldValues, __write_only image2d_t lastFieldVels, 
							 __read_only image2d_t fieldValues,     __write_only image2d_t fieldVels, 
							 unsigned int windowWidth, unsigned int windowHeight) {

		int x = get_global_id(0);												// Get this work-item's coords and exit early if out of bounds.
		if (x >= 300) { return; }												// Reason for extra instances of kernel: global size must be
		int2 coords = (int2)(x, get_global_id(1));								// multiple of work group size.
		
		float prevValue = read_imagef(lastFieldValues, coords).x;				// Get the previous value and velocity at current position.
		float prevVel = read_imagef(lastFieldVels, coords).x;

		float vel;																// Calculate new velocity by using FIELD_PULL.
		if (prevValue > 0) { vel = prevVel - FIELD_PULL; }
		else if (prevValue < 0) { vel = prevVel + FIELD_PULL; }
		else { vel = prevVel; }


		if (x < 299) {
			if (prevValue != 0) {
				//write_imagef(fieldValues, (int2)(x + 1, y), TO_COLOR(10000));
			}

			//accel -= equalize(read_imagef(lastFieldValues, (int2)(x, y)).w, read_imagef(lastFieldValues, (int2)(x + 1, y)).w);
		}
		if (x > 0) {
			//accel -= equalize(read_imagef(lastFieldValues, (int2)(x, y)).w, read_imagef(lastFieldValues, (int2)(x - 1, y)).w);
		}

		write_imagef(fieldValues, (int2)(x, y), TO_COLOR(prevValue + vel));		// New value is equal to old value plus velocity.
		write_imagef(fieldVels, (int2)(x, y), TO_COLOR(vel));					// New velocity was already calculated, just put it in.
}