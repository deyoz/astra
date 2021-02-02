#pragma once

#include <unistd.h>

#include <string>
#include <memory>
#include <vector>

struct Zone {
    const int id;
    std::string title;
    std::vector< uint8_t > data;

    friend bool operator<(const Zone& lhs, const Zone& rhs);
    friend bool operator<(const int lhs, const Zone& rhs);
    friend bool operator<(const Zone& lhs, const int rhs);

    friend bool operator==(const Zone& lhs, const Zone& rhs);

    friend std::ostream& operator<<(std::ostream& os, const Zone& zone);
};
bool operator<(const std::shared_ptr< Zone >& lhs, const std::shared_ptr< Zone >& rhs);
bool operator<(const int lhs, const std::shared_ptr< Zone >& rhs);
bool operator<(const std::shared_ptr< Zone >& lhs, const int rhs);

bool operator==(const std::shared_ptr< Zone >& lhs, const std::shared_ptr< Zone >& rhs);

using pZone = const Zone*;

void InitZoneList(const char* const pu);

// Create new zone: n - zone number, len - length of zone, head - description
Zone* NewZone(const int n, const size_t len, const char* const head);
// Create new zone: n - zone number, data - data of zone, head - description
Zone* NewZone(const int zoneNum, std::vector< uint8_t >&& data, std::string&& head);

// Save zones into stash
void SaveZoneList();
// Restore zones from stash
void RestoreZoneList();

void DropZoneList();
void UpdateZoneList(const char* const pu);

int DropAllAreas (const char* const who);

// Get zone descriptor by zone number
const Zone* getZonePtr(const int zoneNum);
// Get zone descriptor by zone number
Zone* getZonePtrForUpdate(const int zoneNum);

/* Write data to zone 'z'
     data - pointer to data
     start - starting offset
     len - length of data
     size - array element size
     index - array offset index
     count - count of array elements in the data
*/
int setZoneData(const Zone* const z, const void* const data, const size_t start, const size_t len);
int setZoneData2(const Zone* const z, const void* const data, const size_t size, const size_t index, const size_t count);

/* Read data from zone identified by pp
     data - pointer to data
     start - starting offset
     len - length of data
     size - array element size
     index - array offset index
     count - count of array elements in the data
*/
int getZoneData(const Zone* const z, void* const data, const size_t start, const size_t len);
int getZoneData2(const Zone* const z, void* const data, const size_t size, const size_t index, const size_t count);

// Resize zone 'z' to new size 'len' retaining old data
int ReallocZone(const Zone* const z, const size_t len);
// Delete zone number 'n'
int DelZone(const int n);
