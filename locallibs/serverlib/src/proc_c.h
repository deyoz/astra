#ifndef __PROC_C_H_
#define __PROC_C_H_

#include <string>
#include <vector>
#include <stdint.h>

namespace ServerFramework {

void process(std::vector<uint8_t>& reqHead, std::vector<uint8_t>& reqData,
        std::vector<uint8_t>& ansHead, std::vector<uint8_t>& ansData);

struct Cparams
{
    std::string ipc_c;
    std::string ipc_signal;
};

int proc_c_impl(int control_pipe, const Cparams& p);

} // namespace ServerFramework

#endif // __PROC_C_H_
