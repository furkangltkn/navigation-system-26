// madgwick.c
// Created on: Jul 30, 2025
// Author: Talha

#include "madgwick.h"
#include <math.h>

// ---- GLOBAL STATE ----
float beta = 0.1f;       // compensation gain
float q0 = 1.0f;         // quaternion components
float q1 = 0.0f;
float q2 = 0.0f;
float q3 = 0.0f;

// fast inverse square root
static float invSqrt(float x) {
    return 1.0f / sqrtf(x);
}

void Madgwick_Init(float beta_gain) {
    beta = beta_gain;
    q0   = 1.0f;
    q1   = 0.0f;
    q2   = 0.0f;
    q3   = 0.0f;
}

void Madgwick_Update(float gx, float gy, float gz,
                     float ax, float ay, float az,
                     float mx, float my, float mz,
                     float dt)
{
    // 1) Normalize accelerometer
    float recipNorm = invSqrt(ax*ax + ay*ay + az*az);
    ax *= recipNorm;  ay *= recipNorm;  az *= recipNorm;

    // 2) Normalize magnetometer
    recipNorm = invSqrt(mx*mx + my*my + mz*mz);
    mx *= recipNorm;  my *= recipNorm;  mz *= recipNorm;

    // 3) Compute auxiliary variables
    float _2q0 = 2.0f * q0;
    float _2q1 = 2.0f * q1;
    float _2q2 = 2.0f * q2;
    float _2q3 = 2.0f * q3;
    float q0q0 = q0*q0;
    float q1q1 = q1*q1;
    float q2q2 = q2*q2;
    float q3q3 = q3*q3;

    // 4) Reference direction of Earth's magnetic field
    float _2q0mx = _2q0 * mx;
    float _2q0my = _2q0 * my;
    float _2q0mz = _2q0 * mz;
    float _2q1mx = _2q1 * mx;

    float hx = mx*q0q0 - _2q0my*q3 + _2q0mz*q2
             + mx*q1q1 + _2q1mx*q2 + _2q1mx*q3
             - mx*q2q2 - mx*q3q3;
    float hy = _2q0mx*q3 + my*q0q0 - _2q0mz*q1
             + _2q1mx*q3 - my*q1q1 + my*q2q2
             + _2q1mx*q2 - my*q3q3;
    float _2bx = 2.0f * sqrtf(hx*hx + hy*hy);
    float _2bz = -_2q0mx*q2 + _2q0my*q1
                + mz*q0q0 + _2q1mx*q1
                - mz*q1q1 + _2q1mx*q3
                - mz*q2q2 + mz*q3q3;

    // 5) Gradient descent corrective step
    float s0, s1, s2, s3;

    s0 = -_2q2 * (2.0f*(q1*q3 - q0*q2) - ax)
         + _2q1 * (2.0f*(q0*q1 + q2*q3) - ay)
         - _2bz * q2 * (2.0f*(0.5f - q1q1 - q2q2) - az)
         + (-_2bx*q3 + _2bz*q1) * (2.0f*(_2bx*(0.5f - q2q2 - q3q3)
                                       + _2bz*(q1*q3 - q0*q2)) - mx)
         + (-_2bx*q2 + _2bz*q0) * (2.0f*(_2bx*(q1*q2 - q0*q3)
                                       + _2bz*(q0*q1 + q2*q3)) - my)
         + _2bx*q1 * (2.0f*(_2bx*(q0*q2 + q1*q3)
                          + _2bz*(0.5f - q1q1 - q2q2)) - mz);

    s1 =  _2q3 * (2.0f*(q1*q3 - q0*q2) - ax)
         + _2q0 * (2.0f*(q0*q1 + q2*q3) - ay)
         - 4.0f*q1*(2.0f*(0.5f - q1q1 - q2q2) - az)
         + (_2bz*q3 + _2bx*q2) * (2.0f*(_2bx*(0.5f - q2q2 - q3q3)
                                        + _2bz*(q1*q3 - q0*q2)) - mx)
         + (_2bx*q1 + _2bz*q0) * (2.0f*(_2bx*(q1*q2 - q0*q3)
                                        + _2bz*(q0*q1 + q2*q3)) - my)
         + (_2bx*q0 - 4.0f*_2bz*q1) * (2.0f*(_2bx*(q0*q2 + q1*q3)
                                             + _2bz*(0.5f - q1q1 - q2q2)) - mz);

    s2 = -_2q0 * (2.0f*(q1*q3 - q0*q2) - ax)
         + _2q3 * (2.0f*(q0*q1 + q2*q3) - ay)
         - 4.0f*q2*(2.0f*(0.5f - q1q1 - q2q2) - az)
         + (-4.0f*_2bx*q2 - _2bz*q0 + _2bx*q1) *
           (2.0f*(_2bx*(0.5f - q2q2 - q3q3)
                  + _2bz*(q1*q3 - q0*q2)) - mx)
         + (_2bx*q0 + _2bz*q3 + _2bx*q1) *
           (2.0f*(_2bx*(q1*q2 - q0*q3)
                  + _2bz*(q0*q1 + q2*q3)) - my)
         + (_2bx*q3 - 4.0f*_2bx*q2 - _2bz*q2) *
           (2.0f*(_2bx*(q0*q2 + q1*q3)
                  + _2bz*(0.5f - q1q1 - q2q2)) - mz);

    s3 =  _2q1 * (2.0f*(q1*q3 - q0*q2) - ax)
         + _2q2 * (2.0f*(q0*q1 + q2*q3) - ay)
         + (-4.0f*_2bx*q3 + _2bz*q1) *
           (2.0f*(_2bx*(0.5f - q2q2 - q3q3)
                  + _2bz*(q1*q3 - q0*q2)) - mx)
         + (-_2bx*q0 + _2bz*q2) *
           (2.0f*(_2bx*(q1*q2 - q0*q3)
                  + _2bz*(q0*q1 + q2*q3)) - my)
         + (_2bx*q1) *
           (2.0f*(_2bx*(q0*q2 + q1*q3)
                  + _2bz*(0.5f - q1q1 - q2q2)) - mz);

    // normalize step magnitude
    recipNorm = invSqrt(s0*s0 + s1*s1 + s2*s2 + s3*s3);
    s0 *= recipNorm;  s1 *= recipNorm;  s2 *= recipNorm;  s3 *= recipNorm;

    // 6) apply feedback step
    float qDot1 = 0.5f * (-q1*gx - q2*gy - q3*gz) - beta*s0;
    float qDot2 = 0.5f * ( q0*gx + q2*gz - q3*gy) - beta*s1;
    float qDot3 = 0.5f * ( q0*gy - q1*gz + q3*gx) - beta*s2;
    float qDot4 = 0.5f * ( q0*gz + q1*gy - q2*gx) - beta*s3;

    // 7) integrate rate of change
    q0 += qDot1 * dt;
    q1 += qDot2 * dt;
    q2 += qDot3 * dt;
    q3 += qDot4 * dt;

    // 8) normalize quaternion
    recipNorm = invSqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    q0 *= recipNorm;  q1 *= recipNorm;  q2 *= recipNorm;  q3 *= recipNorm;
}

void Madgwick_GetEuler(float* roll, float* pitch, float* yaw) {
    *roll  = atan2f(2.0f*(q0*q1 + q2*q3),
                    1.0f - 2.0f*(q1*q1 + q2*q2)) * 180.0f / M_PI;
    *pitch = asinf( 2.0f*(q0*q2 - q3*q1))   * 180.0f / M_PI;
    *yaw   = atan2f(2.0f*(q0*q3 + q1*q2),
                    1.0f - 2.0f*(q2*q2 + q3*q3)) * 180.0f / M_PI;
}
