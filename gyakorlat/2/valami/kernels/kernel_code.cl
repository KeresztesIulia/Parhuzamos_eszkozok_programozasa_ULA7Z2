
__kernel void multiply_vectors(__global float8* buffer, __global float8* vector, int n)
{
    int multiplier = 1;
    #ifdef MULTIPLIER
    multiplier = MULTIPLIER;
    #endif
    if (get_global_id(0) < n){
        buffer[get_global_id(0)] = vector[get_global_id(0)] * multiplier;
    }
}