// madgwick.h
// Created on: Jul 30, 2025
// Author: Talha

#ifndef INC_MADGWICK_H_
#define INC_MADGWICK_H_


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialize the Madgwick filter state.
 * @param  beta_gain  Compensation gain (e.g., 0.1f).
 */
void Madgwick_Init(float beta_gain);

/**
 * @brief  Perform one Madgwick update step.
 * @param  gx,gy,gz  Gyroscope [rad/s]
 * @param  ax,ay,az  Accelerometer [g]
 * @param  mx,my,mz  Magnetometer [µT]
 * @param  dt        Time step [s]
 */
void Madgwick_Update(float gx, float gy, float gz,
                     float ax, float ay, float az,
                     float mx, float my, float mz,
                     float dt);

/**
 * @brief  Retrieve the current orientation as Euler angles.
 * @param  roll   Pointer to roll angle [°]
 * @param  pitch  Pointer to pitch angle [°]
 * @param  yaw    Pointer to yaw angle [°]
 */
void Madgwick_GetEuler(float* roll, float* pitch, float* yaw);

#ifdef __cplusplus
}
#endif

#endif /* INC_MADGWICK_H_ */
