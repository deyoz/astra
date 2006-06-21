#ifndef _ASTRA_CALLBACKS_H_
#define _ASTRA_CALLBACKS_H_

#include "jxtlib.h"

class AstraCallbacks : public jxtlib::JXTLibCallbacks
{
  public:
    AstraCallbacks() {};
    virtual void InitInterfaces();    
};

#endif /*_ASTRACALLBACKS_H_*/

