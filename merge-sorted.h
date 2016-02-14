#ifndef _MERGE_SORTED_H
#define _MERGE_SORTED_H

// #include <functional>

/* Usage:

  // a and b must iterators to sorted ranges
  merge_sorted(a.childs.begin(),a.childs.end(), childs.begin(),childs.end(),
               [](const value_type &a) { // only in first
               },
               [](const value_type &b) { // only in second
               },
               [](const value_type &a,const value_type &b) { // in both
               }); // ,cmp);
*/

template <typename A,typename B,typename FirstCb,typename SecondCb,typename BothCb,typename Compare=std::less<typename A::value_type>>
void merge_sorted(A a_pos,A a_end, B b_pos,B b_end,
                  FirstCb first,SecondCb second,BothCb both,
                  Compare comp=Compare())
{
  while ( (a_pos!=a_end)&&(b_pos!=b_end) ) {
    if (comp(*a_pos,*b_pos)) {
      first(*a_pos);
      ++a_pos;
    } else if (comp(*b_pos,*a_pos)) {
      second(*b_pos);
      ++b_pos;
    } else {
      both(*a_pos,*b_pos);
      ++a_pos;
      ++b_pos;
    }
  }
  while (a_pos!=a_end) {
    first(*a_pos);
    ++a_pos;
  }
  while (b_pos!=b_end) {
    second(*b_pos);
    ++b_pos;
  }
}

#endif
