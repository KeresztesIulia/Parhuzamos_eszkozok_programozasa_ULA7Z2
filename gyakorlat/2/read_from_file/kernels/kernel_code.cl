
__kernel void add_vectors(__global float8* buffer, __global float8* vector1, __global float8* vector2, int n)
{
    printf("kernel");
    if (get_global_id(0) < n)
    {
        buffer[get_global_id(0)] = vector1[get_global_id(0)] + vector2[get_global_id(0)];
        printf("%f", buffer[get_global_id(0)].s0);
    }
}

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