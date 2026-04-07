#include "FCMachine.h"

/**
 @author Fakov Yuri
 @copyright LLC K-Service Volgograd city
 @cite www.funny_cake.ru

 @param argc
 @param **argv
*/

int main(int argc, char **argv)
{
    FCMachine machine(argc, argv);
    return machine.exec();
}
