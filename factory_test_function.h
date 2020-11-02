#ifndef __ANDROID_FACTORY_TEST_FUNCTION__
#define __ANDROID_FACTORY_TEST_FUNCTION__
#include <time.h>

int diff_ms(const struct timespec *t1, const struct timespec *t2);
void printf_func_mark(char* func);
void wait_save(char* port);
void dump_data(unsigned char * b, int count);
void dump_data_ascii(unsigned char * b, int count);

#endif //__ANDROID_FACTORY_TEST_FUNCTION__
