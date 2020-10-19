#ifndef __ANDROID_FACTORY_TEST_SERVER_H__
#define __ANDROID_FACTORY_TEST_SERVER_H__


#define NCOMMANDS 2
enum COMMAND
{
    START,
    END,
};

struct command
{
    short int comid;
    short int count;
};

#endif //__ANDROID_FACTORY_TEST_SERVER_H__