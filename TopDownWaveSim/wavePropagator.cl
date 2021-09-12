__kernel void wavePropagator(__read_only image2d_t lastFieldVels, __write_only image2d_t fieldVels, 
    __read_only image2d_t lastFieldValues, __write_only image2d_t fieldValues) {
        if (get_global_id(0) >= 300) { return; }
        write_imagef(fieldValues, (int2)(get_global_id(0), get_global_id(1)), (0, 0, 0, 0));
}