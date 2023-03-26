#ifndef PARAM_LIMIT_H
#define PARAM_LIMIT_H
namespace param_limit
{
    extern int output_en = 0;
    extern int timer1_en = 0;
    extern int timer2_en = 0;
    extern int timer3_en = 0;
    extern int timer4_en = 0;
    extern int temp_on = 25, temp_off = 30;
    extern int soil_on = 20, soil_off = 50;
    extern float ec_on = 1.0, ec_off = 2.0;
    extern float tds_on = 100, tds_off = 200;
    extern float ph_on = 6.0, ph_off = 7.0;
}
#endif