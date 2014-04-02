#ifndef _SENSORS_H_
#define _SENSORS_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define M_PI 3.14159265

void SetupSensors();
void ReadAcceleration(int16_t* ax, int16_t* ay, int16_t* az);
void ReadGyroscope(int16_t* gx, int16_t* gy, int16_t* gz);
void ReadTemp(double* temp);
void ReadCompas(double* head, double* mx, double* my, double* mz);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _SENSORS_H_ */