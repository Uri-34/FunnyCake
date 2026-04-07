#ifndef LM75A_H
#define LM75A_H

struct LM75A
{
    static const int TempRegister = 0x0; ///< регистр содержащий данные температуры
    const int ConfRegister = 0x1; ///< конфигурационный регистр
    const int ThistRegister = 0x2; ///<
    const int TosRegister = 0x3; ///<

    const int ConfStart = 0x60; ///< значение настраивающее LM75A на работу
    const int ConfShutdown = 0x0; ///< значение останавливающее работу LM75A

    const int ConfOSCompInt = 0x1; ///<
    const int ConfOSPol = 0x2; ///<
    const int ConfOSPolQue = 0x3; ///<
};

#endif // LM75A_H
