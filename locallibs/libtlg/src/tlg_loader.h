#ifndef __LIBTLG_TLG_LOADER_H__
#define __LIBTLG_TLG_LOADER_H__

#include <string>
#include <vector>

namespace telegrams {

std::vector<int> readQueueNumbers(int argc, char *argv[], int offset);

int tlgLoader(const std::string& fileName, const std::vector<int>& queues);

}

#endif //__LIBTLG_TLG_LOADER_H__
