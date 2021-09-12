float equalize(float thisValue, float otherValue) {
    return (thisValue - otherValue) * 0;
}

__kernel void wavePropagator(__read_only image2d_t lastFieldVels, __write_only image2d_t fieldVels, 
    __read_only image2d_t lastFieldValues, __write_only image2d_t fieldValues) {
        if (get_global_id(0) >= 300) { return; }
        
        int x = get_global_id(0);
        int y = get_global_id(1);

        float accel;
        float prevValue = read_imagef(lastFieldValues, (int2)(x, y)).w;
        float prevAccel = read_imagef(lastFieldVels, (int2)(x, y)).w;

        if (prevValue > 0) {
            accel = prevAccel - 0;
        } else if (prevValue < 0) {
            accel = prevAccel + 0.01f;
        } else {
            accel = prevAccel;
        }


        if (x < 299) {
            if (prevValue > 100) {
                write_imagef(fieldValues, (int2)(x + 1, y), (0, 0, 0, 10000));
            }

            //accel -= equalize(read_imagef(lastFieldValues, (int2)(x, y)).w, read_imagef(lastFieldValues, (int2)(x + 1, y)).w);
        }
        if (x > 0) {
            //accel -= equalize(read_imagef(lastFieldValues, (int2)(x, y)).w, read_imagef(lastFieldValues, (int2)(x - 1, y)).w);
        }

        write_imagef(fieldVels, (int2)(x, y), (float4)(0, 0, 0, prevAccel + accel));
        //write_imagef(fieldValues, (int2)(x, y), (float4)(0, 0, 0, prevAccel + accel + prevValue));
}