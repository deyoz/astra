#if HAVE_CONFIG_H
#endif

#include <list>
#include <string>
#include <vector>
#include <limits>

#define NICKNAME "ASM"
#include "slogger.h"
#include "malloc.h"

namespace { 

class FirstElementAddressComp : 
public std::binary_function< std::pair<const char*,size_t>, std::pair<const char*,size_t> ,bool> {
  public :
  bool operator()( const std::pair<const char*,size_t>& m1 , const std::pair<const char*,size_t>& m2 ) const {
    return m1.first < m2.first;
  }
};

} // namespace 

size_t map_of_free_mem_(int l,const char *nickname, const char *filename, 
  int line,bool printout_map) 
{
  if (printout_map)
    LogTrace( l,nickname,filename,line )<<"map_of_free_mem:";

  size_t memorySize = std::numeric_limits<size_t>::max();
  size_t memoryStep = memorySize/20;
  memorySize -= memoryStep;
  
  std::list< std::pair<const char*,size_t> > res;
  {
  std::list<std::vector<char> > matrix;
  
  
  while( memorySize >= 1000 ) {
    try {
      matrix.push_back( std::vector<char>() );
      matrix.back().reserve( memorySize );
      matrix.back().push_back(' ');
      res.push_back(std::make_pair(&matrix.back()[0],matrix.back().capacity()));
//      std::cout<<"выделили="<<memorySize<<"\n";
    }
    catch( std::bad_alloc& e ) {
//      LogTrace(TRACE1)<<"bad alloc "<<memorySize<<" memoryStep="<<memoryStep<<"\n";
      if (memorySize<=memoryStep*2)
      {
        if (memorySize>100000)
          memoryStep/=10;
        else
          memoryStep/=2;
        if (memoryStep<4*sizeof(size_t))
          memoryStep=4*sizeof(size_t);
      }
      memorySize -= memoryStep;          
    }
  }//while
  }
  //std::cout<<"\n";
  size_t capacity = 0;
  size_t block = 0;
  std::list< std::pair<const char*, size_t> >::iterator listIt = res.begin(), listEnd = res.end();
  if (printout_map)
  {
    for(; listIt != listEnd; ++listIt) 
    {
      if( (*listIt).second > 0 ) 
      {
        size_t cap = (*listIt).second;
        ProgTrace(l, nickname, filename, line, "address=%p capacity=%zi Kb\n", (*listIt).first, cap / 1000);
      }
    }//for ind 
  }
  res.sort( FirstElementAddressComp() );
  if (printout_map)
    LogTrace( l,nickname,filename,line )<<"sorted:";
  listIt = res.begin() , listEnd = res.end();
  for( ; listIt != listEnd; ++listIt ) {
    if( (*listIt).second > 0 ) {
      size_t cap = (*listIt).second;
      capacity += cap ;
      ++block;
      std::list< std::pair<const char*,size_t> >::iterator listIt2 = listIt;
      ++listIt2;
      if (printout_map)
      {
        if( listIt2 != listEnd )
          ProgTrace(l, nickname, filename, line , "address=%p capacity=%zi Kb diff=%zi\n", (*listIt).first,  cap / 1000, (*listIt2).first - ((*listIt).first + cap));
        else
          ProgTrace(l, nickname, filename, line , "address=%p capacity=%zi Kb\n", (*listIt).first,  cap / 1000);
      }
    }
  }//for ind 
  
  LogTrace( l,nickname,filename,line )<<"Full capacity="<<capacity<<" b";
  return capacity/1024;
}

size_t map_of_free_mem(int l,const char *nickname, const char *filename, int line)
{
  return map_of_free_mem_(l,nickname,filename,line,true/*printout map*/);
}

size_t size_of_allocated_mem()
{
    struct mallinfo meminfo = mallinfo();
    int totalMem = meminfo.uordblks + meminfo.hblkhd;
    LogTrace(TRACE5) <<"Memory allocated = " <<totalMem <<" b";
    return static_cast<size_t>(totalMem / 1024);
}
